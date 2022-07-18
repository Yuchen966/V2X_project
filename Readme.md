# V2X-Emergency-Vehicle
## Project description:
In an intersection, the intelligent traffic light shouldn't let the emergency car wait for the red light. Meanwhile, the waiting time of other drivers should also be minimized. In this project, a simulation including a traffic scenario and a control algorithm based on an I2V communication system will be built on VEINS. In the simulation, we will implement the WAVE communication system, establish a network with intersections and intelligent traffic lights, and generate an emergency car as well as several normal cars. The focus of the project is the optimization of the traffic light control algorithm as well as the analysis of the influence given by the traffic flow.

## Explanation in detail:
As a network, we think the first step a straight road (1 km length) with one intersection with traffic lights (at km 0,75 ) would be easy to handle. As vehicle input, our idea is to put a random number of cars there. We have to find out how many we need, so that there will be a traffic jam in front of the intersection due to the traffic lights. Then we would define an emergency vehicle with a higher speed. 

First, we want to set a scenario that when the emergency vehicle is driving into a defined distance to the intersection, the traffic light plan should change to a plan that gives green light to all vehicles in this direction. Then we would also like to try to optimize the traffic light plans so that it would not cause severe congestion to vehicles in the other direction, under the condition of ensuring the emergency vehicle passes through without stopping or slowing down.

## Steps: 
Our goal is to reduce the time for an emergency vehicle to get through traffic lights.
1. We can set a network by using SUMO and the first network will be a straight line (1km) with an intersection with traffic lights at 0.75km. (further: a more complex scenario, for example, two intersections with traffic lights)
2. we will set a  random number of vehicles to make a traffic jam and the vehicles have the same speed(further: two or three kinds of speeds)
3. achieve the communication between traffic lights and vehicles at a certain distance and make traffic lights know the speed and position of the vehicles.
4.come up with an optimization algorithm for traffic lights controlling  to reduce time 
5. compare the time with and without optimization algorithm
6. research the average time and average throughput (second optimization term)
7. research the different influences of time in different directions.

[Google drive link](https://drive.google.com/drive/u/0/folders/1mLzypaPHBv0e057QBtl6o5c_BoqpQsQ0)
