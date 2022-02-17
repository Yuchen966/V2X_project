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

#include "TlVehicle.h"

using namespace veins;

Define_Module(veins::TlVehicle);

#define WSM_SEND_TASK 14

void TlVehicle::initialize(int stage) {
    DemoBaseApplLayer::initialize(stage);

    if (myId!=0 && traciVehicle->getTypeId() == "EmergencyVehicle") {
        std::cout << "I am the emergency vehicle!!! Get out of my way!!! My id is " << myId << std::endl;
    }

    if (stage ==0 ) {
        currentSubscribedServiceId = -1;
        sendPeriod = 1;
        wsmSendEvt = new cMessage("wsm send task", WSM_SEND_TASK);
    }
    else if (stage == 1) {
        //scheduleAt(simTime()+sendPeriod, wsmSendEvt);
    }
}

void TlVehicle::onWSM(BaseFrame1609_4* frame){
    //std::cout << "vid: " << myId << ", subs id: " << currentSubscribedServiceId << ", ch: " << currentSubscribedChannel << std::endl;
}

void TlVehicle::onWSA(DemoServiceAdvertisment* wsa){
    //std::cout << "WSA received at T: " << simTime() << ", psid: " << wsa->getPsid() << ", curSubs: " << currentSubscribedServiceId << std::endl;
    if ( currentSubscribedServiceId == -1) {
        currentSubscribedServiceId = wsa->getPsid();
        currentSubscribedChannel = wsa->getTargetChannel();
        mac->changeServiceChannel((Channel)(currentSubscribedChannel));
        //std::cout << "Vehicle " << myId << ", psid: " << currentSubscribedServiceId << ", subscribed to channel " << currentSubscribedChannel << std::endl;

        simtime_t wsm_time = computeAsynchronousSendingTime(sendPeriod, ChannelType::service);
        scheduleAt(wsm_time, wsmSendEvt);
    }
}

void TlVehicle::handleSelfMsg(cMessage* msg) {
    if (msg->getKind() == WSM_SEND_TASK ) {
        TlMsg* tlMsg = new TlMsg();
        tlMsg->setSenderAddress(myId);
        tlMsg->setSenderPos(curPosition);
        tlMsg->setSenderSpeed(curSpeed);
        tlMsg->setTimeStampP(simTime());
        tlMsg->setLaneId(traciVehicle->getLaneId().c_str());
        tlMsg->setChannelNumber(static_cast<int>(currentSubscribedChannel));
        if (traciVehicle->getTypeId() == "EmergencyVehicle") {
            tlMsg->setIsEV(true);
        }
        sendDown(tlMsg->dup());
        delete tlMsg;
        //std::cout << "WSM sent at " << simTime() << std::endl;
        simtime_t wsm_time = computeAsynchronousSendingTime(sendPeriod, ChannelType::service);
        scheduleAt(wsm_time, wsmSendEvt);
        //scheduleAt(simTime() + sendPeriod, wsmSendEvt);
    }
    else {
        DemoBaseApplLayer::handleSelfMsg(msg);
    }
}
