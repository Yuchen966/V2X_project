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

#ifndef TLVEHICLE_H_
#define TLVEHICLE_H_

#include "veins/modules/application/ieee80211p/DemoBaseApplLayer.h"
#include "TlMsg_m.h"

namespace veins{
class VEINS_API TlVehicle: public DemoBaseApplLayer {
public:
    void initialize(int stage) override;

protected:
    int currentSubscribedServiceId;
    int currentSubscribedChannel;
    cMessage* wsmSendEvt;
    simtime_t sendPeriod;

    void onWSM(BaseFrame1609_4* wsm) override;
    void onWSA(DemoServiceAdvertisment* wsa) override;

    void handleSelfMsg(cMessage* msg) override;
};
}

#endif /* TLVEHICLE_H_ */
