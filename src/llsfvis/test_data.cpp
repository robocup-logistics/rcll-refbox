/*
 * test_data.cpp
 *
 *  Created on: 02.03.2013
 *      Author: daniel
 */

#include "msgs/MachineInfo.pb.h"

using namespace llsf_msgs;
using namespace std;

float random_coordinate() {
	return (float) rand() / ((float) RAND_MAX / 5.6f);
}

MachineInfo* getMachineInfo() {
	MachineInfo* mSpecs = new MachineInfo();
	Machine* m1 = mSpecs->add_machines();
	m1->set_name("M1");
	m1->set_type("T0");
	Pose2D* pose = m1->mutable_pose();
	pose->set_x(3.92);
	pose->set_y(1.68);
	pose->set_ori(0);
	LightSpec* red = m1->add_lights();
	red->set_color(LightColor::RED);
	red->set_state(LightState::ON);
	LightSpec* yellow = m1->add_lights();
	yellow->set_color(LightColor::YELLOW);
	yellow->set_state(LightState::ON);
	LightSpec* green = m1->add_lights();
	green->set_color(LightColor::GREEN);
	green->set_state(LightState::BLINK);
	Puck* p1_m = m1->add_loaded_with();
	p1_m->set_id(42);
	p1_m->set_state(PuckState::S0);
	Puck* p2_m = m1->add_loaded_with();
	p2_m->set_id(42123);
	p2_m->set_state(PuckState::S1);
	return mSpecs;
}

RobotInfo* getRobotInfo() {
	RobotInfo* rInfo = new RobotInfo();
	Robot* r = rInfo->add_robots();
	r->set_name("Tick");
	r->set_team("Carologistics");
	Time* rtime = r->mutable_last_seen();
	rtime->set_sec(3);
	rtime->set_nsec(14);
	Pose2D* rpose = r->mutable_pose();
	rpose->set_x(2.8);
	rpose->set_y(2.8);
	rpose->set_ori(0);
	return rInfo;
}

OrderInfo* getOrderInfo() {
	OrderInfo* oInfo = new OrderInfo();
	for (unsigned int i = 0; i< 9; ++i){
		Order* o = oInfo->add_orders();
		o->set_id(i);
		o->set_delivery_gate(Order_DeliveryGate_D1);
		o->set_product(Order_ProductType_P3);
		int delivery_time=rand()%120;
		o->set_delivery_period_begin(delivery_time);
		delivery_time+=rand()%20;
		o->set_delivery_period_end(delivery_time);
		o->set_quantity_requested(10);
		o->set_quantity_delivered(1);
	}
	return oInfo;
}

PuckInfo* getPuckInfo() {
	const PuckState states[] = { PuckState::S0, PuckState::S1, PuckState::S2,
			PuckState::P1, PuckState::P2, PuckState::P3, PuckState::CONSUMED };
	PuckInfo* pInfo = new PuckInfo();
	for (unsigned int i = 0; i < 22; ++i) {

		Puck* p = pInfo->add_pucks();
		p->set_id(i);

		p->set_state(states[rand() % 7]);
		Pose2D* p_pose = p->mutable_pose();
		p_pose->set_x(random_coordinate());
		p_pose->set_y(random_coordinate());
	}
	return pInfo;
}
