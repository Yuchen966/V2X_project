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
        ::omnetpp::opp_string laneId = tlm->getLaneId();

        double distance = (senderPos - curPosition).length();
        //std::cout << "RSU received channel: " << frame->getChannelNumber() << std::endl;
        //std::cout << "RSU " << myId << " received TLM from " << senderId << " in lane " << laneId << std::endl;
        //std::cout << "Distance to me is: " << distance << std::endl;

        Coord relPos = senderPos - curPosition;
        Coord relSpeed = senderSpeed - curSpeed; // curSpeed is zero of course because this is an RSU

        if ( distance < 200 ){  // don't consider vehicles which are further away than 100 m
            //if ( myId == 25 ) {
            //    std::cout << "debug" << std::endl;
            //}
            if ( relPos * relSpeed <= 0 ) { // think about vector inner product, this means that the vehicle is approaching the RSU
                if ( std::abs(relPos.x) > std::abs(relPos.y) ) {
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
            traci->trafficlight(trafficLightId).setProgram("lr");

        scheduleAt(simTime() + controlPeriod, phaseMsg);
    } else {
        DemoBaseApplLayer::handleSelfMsg(msg);
    }
}

std::string TlRSU::mapRSU2Tl(int id){
    switch(id){
    case 25:
        return "j1";
    case 30:
        return "j2";
    case 35:
        return "j3";
    case 40:
        return "j4";
    case 45:
        return "j5";
    case 50:
        return "j6";
    case 55:
        return "j7";
    case 60:
        return "j8";
    case 65:
        return "j9";
    case 70:
        return "j10";
    case 75:
        return "j11";
    case 80:
        return "j12";
    case 85:
        return "j13";
    case 90:
        return "j14";
    case 95:
        return "j15";
    case 100:
        return "j16";
    default:
        EV_ERROR << "Don't know which TL this RSU should be mapped to."<< std::endl;
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
