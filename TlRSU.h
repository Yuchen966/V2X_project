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

#ifndef TLRSU_H_
#define TLRSU_H_

#include "veins/modules/application/ieee80211p/DemoBaseApplLayer.h"

namespace veins{

typedef struct{
    int myId;
    Coord lastPos;
    Coord lastSpeed;
    double dist;
}CarData;

enum OriEvType {
    Left,
    Right,
    Up,
    Down
};

class VEINS_API TlRSU: public DemoBaseApplLayer {
    public:
    void initialize(int stage) override;

    protected:
    void onWSM(BaseFrame1609_4* wsm) override;
    void handleSelfMsg(cMessage* msg) override;
    std::string mapRSU2Tl(int myId);
    TraCIScenarioManager* manager;
    std::string trafficLightId;
    cMessage *initMsg, *phaseMsg;
    simtime_t controlPeriod;
    bool evPlanGenerated = false;

        // variables and functions related to traffic light control algorithm
    std::vector<CarData> left, right, up, down; // vectors of meta data of cars from each direction. If you don't know about vector, you could google standard c++ library vector.
    void checkAndAddLeft(int senderId, Coord senderPos, Coord senderSpeed, double distance);
    void checkAndAddRight(int senderId, Coord senderPos, Coord senderSpeed, double distance);
    void checkAndAddUp(int senderId, Coord senderPos, Coord senderSpeed, double distance);
    void checkAndAddDown(int senderId, Coord senderPos, Coord senderSpeed, double distance);

    void removeFromLeft(int senderId);
    void removeFromRight(int senderId);
    void removeFromUp(int senderId);
    void removeFromDown(int senderId);
    void removeFromVector(int senderId);

    // Project functions
    bool checkEvisUd(float dist_x, float dist_y, std::vector<CarData> &frontCars, OriEvType& evOri);
    bool isLightForEV(std::string TlId, bool EVfromUD);
    CarData evInit(int senderId, Coord relPos, Coord relSpeed);
    
    // Project variables
};

}

#endif /* TLRSU_H_ */
