#include <iostream>
#include "mps.h"
#include "mps_refbox_interface.h"
#include "mps_deliver.h"
#include "mps_pick_place_1.h"
#include "mps_pick_place_2.h"
#include "mps_incoming_station.h"

int main(int argc, char** argv) {
  MPSRefboxInterface* ref = new MPSRefboxInterface();

  while(true) {
    std::cout << "Menu: " << std::endl;
    std::cout << "1. Add Machine" << std::endl;
    std::cout << "2. Send/Receive Message" << std::endl;
    std::cout << "3. Show Mashines" << std::endl;
    int input = 0;
    std::cin >> input;
    std::cout << input << std::endl;

    // add a machine type to list
    if(input == 1) {
      std::cout << "What kind of Mashine you want add?" << std::endl;
      std::cout << "1. Incoming station" << std::endl;
      std::cout << "2. Singel Pick and Place" << std::endl;
      std::cout << "3. Double Pick and Place" << std::endl;
      std::cout << "4. Delivery station" << std::endl;

      int mashine = 0;
      char ip[13]; // that have to be dynamic
      int port;
      std::cin >> mashine;
      std::cout << "IP: ";
      std::cin >> ip;
      std::cout << std::endl << "Port: ";
      std::cin >> port;
      std::cout << std::endl;

      // add machine of type incoming station
      if(mashine == 1) {
        MPSIncomingStation *is = new MPSIncomingStation(ip, port);
        ref->mpsList.push_back(is);
      }
      // add machine of type single pick and place
      else if(mashine == 2) {
        MPSPickPlace1 *pp1 = new MPSPickPlace1(ip, port);
        ref->mpsList.push_back(pp1);
      }
      // add machine of type double pick and place
      else if(mashine == 3) {
        MPSPickPlace2 *pp2 = new MPSPickPlace2(ip, port);
        ref->mpsList.push_back(pp2);
      }
      // add machine of type delivery station
      else if (mashine == 4) {
        MPSDeliver *de = new MPSDeliver(ip, port);
        ref->mpsList.push_back(de);
      }
      else {
        std::cout << "Mashintype not available" << std::endl;
      }
    }
    // send or reveive a message from machine
    else if(input == 2) {
      std::cout << "List of all Machins: " << std::endl;
      for(int i = 0; i < ref->mpsList.size(); i++) {
        std::cout << i << ". " << static_cast<MPS*>(ref->mpsList.at(i))->getIp() << std::endl;
      }

      std::cout << "What machine you want use" << std::endl;
      int machine = 0;
      std::cin >> machine;

      // available operations for machine of type incoming station
      if(static_cast<MPS*>(ref->mpsList.at(machine))->getType() == 1) {
        std::cout << "1. getCap(color, side)" << std::endl;
        std::cout << "2. capReady" << std::endl;
        std::cout << "3. setLight(light, state)" << std::endl;
        
        int operation = 0;
        std::cout << "Number: ";
        std::cin >> operation;
        std::cout << std::endl;

        // get cap command / send command
        if(operation == 1) {
          int color = 0;
          int side = 0;
          std::cout << "color: ";
          std::cin >> color;
          std::cout << std::endl << "side: ";
          std::cin >> side;
          std::cout << std::endl;
        
          static_cast<MPSIncomingStation*> (ref->mpsList.at(machine))->getCap(color, side);
        }
        // cap ready for deliver command / reveive command
        else if(operation == 2) {
          std::cout << static_cast<MPSIncomingStation*>(ref->mpsList.at(machine))->capReady() << std::endl;
        }
        // set light command for incoming station
        else if(operation == 3) {
          int light = 0;
          int state = 0;
          std::cout << "light: ";
          std::cin >> light;
          std::cout << std::endl << "state: ";
          std::cin >> state;
          std::cout << std::endl;

          static_cast<MPSIncomingStation*> (ref->mpsList.at(machine))->setLight(light, state);
        }
      }
      // available operations for machine of type single pick and place
      else if(static_cast<MPS*>(ref->mpsList.at(machine))->getType() == 2) {
        std::cout << "1. produceEnd(updown)" << std::endl;
        std::cout << "2. isReady" << std::endl;
        std::cout << "3. setLight(light, state)" << std::endl;

        int operation = 0;
        std::cout << "Number: ";
        std::cin >> operation;
        std::cout << std::endl;

        // produce end command / send command
        if(operation == 1) {
          int updown = 0;
          std::cout << "up or down: ";
          std::cin >> updown;
          std::cout << std::endl;

          static_cast<MPSPickPlace1*>(ref->mpsList.at(machine))->produceEnd(updown);
        }
        // workpiece is ready command / receive command
        else if(operation == 2) {
          std::cout << static_cast<MPSPickPlace1*>(ref->mpsList.at(machine))->isReady() << std::endl;
        }
        // set light for pick and place 1
        else if(operation == 3) {
          int light = 0;
          int state = 0;
          std::cout << "light: ";
          std::cin >> light;
          std::cout << std::endl << "state: ";
          std::cin >> state;
          std::cout << std::endl;

          static_cast<MPSIncomingStation*> (ref->mpsList.at(machine))->setLight(light, state);
        }
      }
      // available operations for machine of type double pick and place
      else if(static_cast<MPS*>(ref->mpsList.at(machine))->getType() == 3) {
        std::cout << "1. produceRing(arm)" << std::endl;
        std::cout << "2. ringReady" << std::endl;
        std::cout << "3. setLight(light, state)" << std::endl;

        int operation = 0;
        std::cout << "Number: " << std::endl;
        std::cin >> operation;
        std::cout << std::endl;

        // set a ring on workpiece command / send command
        if(operation == 1) {
          int arm = 0;
          std::cout << "which arm: ";
          std::cin >> arm;
          std::cout << std::endl;

          static_cast<MPSPickPlace2*>(ref->mpsList.at(machine))->produceRing(arm);
        }
        // ring is ready command / reveive command
        else if(operation == 2) {
          std::cout << static_cast<MPSPickPlace2*>(ref->mpsList.at(machine))->ringReady() << std::endl;
        }
        // set light for pick and place 2
        else if(operation == 3) {
          int light = 0;
          int state = 0;
          std::cout << "light: ";
          std::cin >> light;
          std::cout << std::endl << "state: ";
          std::cin >> state;
          std::cout << std::endl;

          static_cast<MPSIncomingStation*> (ref->mpsList.at(machine))->setLight(light, state);
        }
      }
      // available operations for machine of type delivery station
      else if(static_cast<MPS*>(ref->mpsList.at(machine))->getType() == 4) {
        std::cout << "1. sendDeliver(lane)" << std::endl;
        std::cout << "2. isDelivered" << std::endl;
        std::cout << "3. setLight(light, state)" << std::endl;

        int operation = 0;
        std::cout << "Number: " << std::endl;
        std::cin >> operation;
        std::cout << std::endl;

        // deliver workpiece command / send command
        if(operation == 1) {
          int lane = 0;
          std::cout << "Which lane: " << std::endl;
          std::cin >> lane;
          std::cout << std::endl;

          static_cast<MPSDeliver*>(ref->mpsList.at(machine))->sendDeliver(lane);
        }
        // workpiece is delivered command / receive command
        else if(operation == 2) {
          std::cout << static_cast<MPSDeliver*>(ref->mpsList.at(machine))->isDelivered() << std::endl;
        }
        // set light for delivery station
        else if(operation == 3) {
          int light = 0;
          int state = 0;
          std::cout << "light: ";
          std::cin >> light;
          std::cout << std::endl << "state: ";
          std::cin >> state;
          std::cout << std::endl;

          static_cast<MPSIncomingStation*> (ref->mpsList.at(machine))->setLight(light, state);
        }
      }
    }
    // print list of all registred mps
    else if(input == 3) {
      for(int i = 0; i < ref->mpsList.size(); i++) {
        std::cout << static_cast<MPS*>(ref->mpsList.at(i))->getIp() << std::endl;
      }
    }
  }

  return 0;
}

