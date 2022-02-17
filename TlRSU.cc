//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include "TlRSU.h"

#include "TlMsg_m.h"

using namespace veins;

Define_Module(veins::TlRSU);

#define TL_INIT_EVT 1234
#define TL_CTRL_EVT 1235

static const std::vector<double> TimePlatoonPassVec = {0.0, 2.2, 4.0, 6.0, 
    8.0, 10.15, 12.3, 14.5, 16.7, 18.9, 21.15, 23.40, 25.70};
static const double TimeEVPass = 15.0;
static const double TimeSafe = 2.0;

// enum OriEvType {
//     Left,
//     Right,
//     Up,
//     Down
// };

void TlRSU::initialize(int stage) {
    DemoBaseApplLayer::initialize(stage);
    if (stage == 0) {
        currentOfferedServiceId = 17;
        controlPeriod = 5;
    }
    else if (stage == 1){
        trafficLightId = mapRSU2Tl(myId);
        std::cout << "RSU Id: " << myId << " is assigned a traffic light " << trafficLightId << std::endl;
        startService(Channel::sch2, currentOfferedServiceId, "Traffic Light Service");

        // initMsg is used only once at the beginning. It's some sort of a workaround: see handleSelfMsg() below
        initMsg = new cMessage("Init TL", TL_INIT_EVT);
        phaseMsg = new cMessage("TL control", TL_CTRL_EVT);
        scheduleAt(simTime(), initMsg);
        scheduleAt(simTime() + controlPeriod, phaseMsg);
    }
}

void TlRSU::onWSM(BaseFrame1609_4* frame) {
    if (TlMsg* tlm = check_and_cast<TlMsg*>(frame)) {
        int senderId = tlm->getSenderAddress();
        Coord senderPos = tlm->getSenderPos();
        Coord senderSpeed = tlm->getSenderSpeed();
        bool senderIsEV = tlm->getIsEV();
        ::omnetpp::opp_string laneId = tlm->getLaneId();

        double distance = (senderPos - curPosition).length();
        //std::cout << "RSU received channel: " << frame->getChannelNumber() << std::endl;
        //std::cout << "RSU " << myId << " received TLM from " << senderId << " in lane " << laneId << std::endl;
        //std::cout << "Distance to me is: " << distance << std::endl;

        Coord relPos = senderPos - curPosition;
        Coord relSpeed = senderSpeed - curSpeed; // curSpeed is zero of course because this is an RSU

        if ( distance < 200 ){  // don't consider vehicles which are further away than 100 m
            if ( relPos * relSpeed <= 0 ) { // think about vector inner product, this means that the vehicle is approaching the RSU
                if (senderIsEV && !evPlanGenerated) {
                    evPlanGenerated = true;
                    std::cout <<  myId << ": The EV is approaching. I have to generate a new plan" << std::endl;
                    std::vector<CarData> frontCars;
                    CarData evCar = evInit(senderId, relPos, relSpeed);
                    OriEvType evOri;

                    // checking where EV is coming from and add front cars to frontCars[]
                    bool isEvUd = checkEvisUd(relPos.x, relPos.y, frontCars, evOri);
                    bool isGreenForEV = isLightForEV(trafficLightId, isEvUd);
                    std::cout << "Number of cars in front of EV: " << frontCars.size() << std::endl;

                    // main algorithm
                    if (isGreenForEV) { // Current status of tl is green
                        // The min green time for Traffic light for letting EV pass without deacceleration
                        double timeTlGreenMin = std::max(TimePlatoonPassVec[frontCars.size()+1], TimeEVPass);
                        // check if the remaining time for current green light time is enough
                        double GreenRemainTime = (traci->trafficlight(trafficLightId).getAssumedNextSwitchTime() - simTime()).dbl();
                        bool isGreenEnough = (timeTlGreenMin < GreenRemainTime);
                        std::cout << "Min green time for EV passing without decreasing: " << timeTlGreenMin << "s" << std::endl;
                        std::cout << "Current green light remaining time until next switch red light: " << GreenRemainTime << "s" << std::endl;
                        if (isGreenEnough) {
                            std::cout << "The remaining green time is enough, stick to original plan.";
                        } else {
                            traci->trafficlight(trafficLightId).setPhaseDuration(SimTime(timeTlGreenMin));
                            std::cout << "The remaining green time is not enough, so let's reset! Now the Duration becomes " << 
                                (traci->trafficlight(trafficLightId).getAssumedNextSwitchTime() - simTime()).dbl()<< "s" << std::endl;
                        }
                    } else { // Current status of tl is red
                        if (isChased(evCar, frontCars)) { // chased
                            // set traffic light current state duration to 0
                            std::cout << "EV is able to chase. Set traffic light to green." << std::endl;
                            traci->trafficlight(trafficLightId).setPhaseDuration(SimTime(0.0));
                        } else { // not chased
                            double RedremainTime = (traci->trafficlight(trafficLightId).getAssumedNextSwitchTime() - simTime()).dbl();
                            std::cout << "EV won't chase the platoon. The remaining red light time is " << RedremainTime << "s" << std::endl;
                            if (!((TimeEVPass - TimePlatoonPassVec[frontCars.size()]) > (RedremainTime + TimeSafe))) { // not able to pass
                                traci->trafficlight(trafficLightId).setPhaseDuration(
                                    TimeEVPass - TimePlatoonPassVec[frontCars.size()] - TimeSafe
                                );
                                std::cout << "EV is able to pass with current plan. Just stick to current plan." << std::endl;
                            } else {
                                std::cout << "EV is not able to pass with current plan. Switch to green immediately." << std::endl;
                                traci->trafficlight(trafficLightId).setPhaseDuration(SimTime(0.0));
                            }
                        }
                    }
                }

                // normal vehicle, update Cardata lists
                else if ( std::abs(relPos.x) > std::abs(relPos.y) ) {
                    if ( relPos.x < 0 ) // likely coming from the left
                        checkAndAddLeft(senderId, relPos, relSpeed);
                    else    // likely coming from the right
                        checkAndAddRight(senderId, relPos, relSpeed);
                }
                else {
                    if ( relPos.y < 0 ) // likely coming from up
                        checkAndAddUp(senderId, relPos, relSpeed);
                    else    // likely coming from down
                        checkAndAddDown(senderId, relPos, relSpeed);
                }
            } else { // this car is moving away from the traffic light
                removeFromVector(senderId);
            }
        }
    }
}

void TlRSU::handleSelfMsg(cMessage* msg) {
    if (msg->getKind() == TL_INIT_EVT ){
        // we enter this only once. We want to connect this C++ code with SUMO via traCI
        // I think we should be able to do this in initialize(), but somehow it doesn't work.
        // so I do this after the simulation starts.. here..
        manager = TraCIScenarioManagerAccess().get();
        traci = manager->getCommandInterface();
    }
    else if (msg->getKind() == TL_CTRL_EVT ) {
        if (myId == 25) {
            std::cout << mapRSU2Tl(myId) << ": # waiting L, R, U, D: " << left.size() << ", " << right.size() << ", " << up.size() << ", " << down.size() << std::endl;
        }
        if ( left.size() + right.size() < up.size() + down.size() ) // basically we're counting the number of cars from each direction
            traci->trafficlight(trafficLightId).setProgram("ud");
        else
            traci->trafficlight(trafficLightId).setProgram("ud");

        scheduleAt(simTime() + controlPeriod, phaseMsg);
    } else {
        DemoBaseApplLayer::handleSelfMsg(msg);
    }
}

std::string TlRSU::mapRSU2Tl(int id){
    switch(id){
    case 13:
        {return "cross1"; break;}
    case 18:
        {return "cross3"; break;}
    case 23:
        {return "cross4"; break;}
    case 28:
        {return "cross2"; break;}
    default:
        std::cout << "Don't know which TL this RSU should be mapped to."<< std::endl;
        return "";
    }
}

void TlRSU::checkAndAddLeft(int senderId, Coord relPos, Coord relSpeed){
    bool knownCar = false;
    for (int i=0; i<left.size();++i) {
        if (left[i].myId == senderId) {
            left[i].lastPos = relPos;
            left[i].lastSpeed = relSpeed;
            knownCar = true;
            break;
        }
    }
    if ( !knownCar ){
        left.push_back(CarData{senderId, relPos, relSpeed});
    }
}

void TlRSU::checkAndAddRight(int senderId, Coord relPos, Coord relSpeed){
    bool knownCar = false;
    for (int i=0; i<right.size();++i) {
        if (right[i].myId == senderId) {
            right[i].lastPos = relPos;
            right[i].lastSpeed = relSpeed;
            knownCar = true;
            break;
        }
    }
    if ( !knownCar ){
        right.push_back(CarData{senderId, relPos, relSpeed});
    }
}

void TlRSU::checkAndAddUp(int senderId, Coord relPos, Coord relSpeed){
    bool knownCar = false;
    for (int i=0; i<up.size();++i) {
        if (up[i].myId == senderId) {
            up[i].lastPos = relPos;
            up[i].lastSpeed = relSpeed;
            knownCar = true;
            break;
        }
    }
    if ( !knownCar ){
        up.push_back(CarData{senderId, relPos, relSpeed});
    }
}

void TlRSU::checkAndAddDown(int senderId, Coord relPos, Coord relSpeed){
    bool knownCar = false;
    for (int i=0; i<down.size();++i) {
        if (down[i].myId == senderId) {
            down[i].lastPos = relPos;
            down[i].lastSpeed = relSpeed;
            knownCar = true;
            break;
        }
    }
    if ( !knownCar ){
        down.push_back(CarData{senderId, relPos, relSpeed});
    }
}

void TlRSU::removeFromLeft(int senderId){
    bool inTheVector = false;
    for (int i=0;i<left.size();++i) {
        if (left[i].myId == senderId) {
            left.erase(left.begin()+i); // looks complicated, but don't worry, this erases the i-th element in the vector. (google c++ vector, erase())
            inTheVector = true;
            break;
        }
    }
    if ( !inTheVector ) // it wasn't in the vector, something is wrong.
        EV_ERROR << "This vehicle didn't come from this direction, can't erase from vector" << std::endl;
}

void TlRSU::removeFromRight(int senderId){
    bool inTheVector = false;
    for (int i=0;i<right.size();++i) {
        if (right[i].myId == senderId) {
            right.erase(right.begin()+i); // looks complicated, but don't worry, this erases the i-th element in the vector. (google c++ vector, erase())
            inTheVector = true;
            break;
        }
    }
    if ( !inTheVector ) // it wasn't in the vector, something is wrong.
        EV_ERROR << "This vehicle didn't come from this direction, can't erase from vector" << std::endl;
}

void TlRSU::removeFromUp(int senderId){
    bool inTheVector = false;
    for (int i=0;i<up.size();++i) {
        if (up[i].myId == senderId) {
            up.erase(up.begin()+i); // looks complicated, but don't worry, this erases the i-th element in the vector. (google c++ vector, erase())
            inTheVector = true;
            break;
        }
    }
    if ( !inTheVector ) // it wasn't in the vector, something is wrong.
        EV_ERROR << "This vehicle didn't come from this direction, can't erase from vector" << std::endl;
}

void TlRSU::removeFromDown(int senderId){
    bool inTheVector = false;
    for (int i=0;i<down.size();++i) {
        if (down[i].myId == senderId) {
            down.erase(down.begin()+i); // looks complicated, but don't worry, this erases the i-th element in the vector. (google c++ vector, erase())
            inTheVector = true;
            break;
        }
    }
    if ( !inTheVector ) // it wasn't in the vector, something is wrong.
        EV_ERROR << "This vehicle didn't come from this direction, can't erase from vector" << std::endl;
}

void TlRSU::removeFromVector(int senderId){

    for (int i=0;i<left.size();++i) {
        if (left[i].myId == senderId) {
            left.erase(left.begin()+i); // looks complicated, but don't worry, this erases the i-th element in the vector. (google c++ vector, erase())
            return;
        }
    }
    for (int i=0;i<right.size();++i) {
        if (right[i].myId == senderId) {
            right.erase(right.begin()+i); // looks complicated, but don't worry, this erases the i-th element in the vector. (google c++ vector, erase())
            return;
        }
    }
    for (int i=0;i<up.size();++i) {
        if (up[i].myId == senderId) {
            up.erase(up.begin()+i); // looks complicated, but don't worry, this erases the i-th element in the vector. (google c++ vector, erase())
            return;
        }
    }
    for (int i=0;i<down.size();++i) {
        if (down[i].myId == senderId) {
            down.erase(down.begin()+i); // looks complicated, but don't worry, this erases the i-th element in the vector. (google c++ vector, erase())
            return;
        }
    }

    // if your program reaches this point, it means that you were trying to remove a vehicle which wasn't in the vector
    EV_ERROR << "Tried to remove a vehicle not in the vector" << std::endl;
}

// If coming from up/down, return true, left/right return false
bool TlRSU::checkEvisUd(float dist_x, float dist_y, std::vector<CarData>& frontCars, OriEvType& evOri) {
    if ( std::abs(dist_x) > std::abs(dist_y) ) {
        if ( dist_x < 0 ) {// likely coming from the left
            for (int i=0; i<left.size(); i++) 
                frontCars.push_back(left[i]);
            std::cout << "EV is coming from left." << std::endl;
            evOri = Left;
        } else {   // likely coming from the right
            for (int i=0; i<right.size(); i++) 
                frontCars.push_back(right[i]);
            std::cout << "EV is coming from right." << std::endl;
            evOri = Right;
        }
        return false;
            
    } else {
        if ( dist_y < 0 ) { // likely coming from up
            for (int i=0; i<left.size(); i++)
                frontCars.push_back(up[i]);
            std::cout << "EV is coming from up." << std::endl;
            evOri = Up;
        }
            
        else {   // likely coming from down
            for (int i=0; i<left.size(); i++)
                frontCars.push_back(down[i]);
            std::cout << "EV is coming from down." << std::endl;
            evOri = Down;
        }
        return true;
    }
    
}

// check whether the traffic light is green for Emergency Vehicle.
// If it is green fro EV, return true, otherwise false.
bool TlRSU::isLightForEV(std::string TlId, bool isEvUd) {
    std::string curState = traci->trafficlight(TlId).getCurrentState();
    if (isEvUd) {
        if (curState[0] == 'G' || curState[0] == 'g') 
            return true;
        else
            return false;
    } else {
        if (curState[0] == 'r') 
            return true;
        else
            return false;
    }
}

CarData TlRSU::evInit(int senderId, Coord relPos, Coord relSpeed) {
    CarData evCarData;
    evCarData.myId == senderId;
    evCarData.lastPos = relPos;
    evCarData.lastSpeed = relSpeed;
    return evCarData;
}

// if EV will chase the closest car in front of it, return true.
bool TlRSU::isChased(CarData evCar, std::vector<CarData> frontCars) {
    if (frontCars.size() == 0)
       return false;

    // first get the closest car
    CarData carLast = frontCars[0]; // the closest car in front of the EV
    double distMin = 500.0;
    for (int i=0; i<frontCars.size(); i++) {
        if ((frontCars[i].lastPos-evCar.lastPos).length() < distMin) {
            distMin = (carLast.lastPos-evCar.lastPos).length();
            carLast = frontCars[i];
        }
    }

    // then check if it can be chased
    if (std::abs(carLast.lastSpeed.length()) > 40.0/3.6) { // the last car is moving at max speed
        double timeChase = (carLast.lastPos - evCar.lastPos).length() / (carLast.lastSpeed - evCar.lastSpeed).length();
        std::cout << "timeChase: " << timeChase << std::endl;
        std::cout << "distance: " << (carLast.lastPos - evCar.lastPos).length() << std::endl;
        std::cout << "relSpeed: " << (carLast.lastSpeed - evCar.lastSpeed).length();
        
        if (timeChase > TimeEVPass + TimeSafe) // not chased when crossing the intersection
            return false;
        else
            return true;
    }
    
}
