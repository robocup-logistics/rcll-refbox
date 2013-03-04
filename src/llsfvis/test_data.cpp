/*
 * test_data.cpp
 *
 *  Created on: 02.03.2013
 *      Author: daniel
 */

#include "msgs/MachineInfo.pb.h"

using namespace llsf_msgs;

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

PuckInfo* getPuckInfo() {
	PuckInfo* pInfo = new PuckInfo();
	Puck* p1 = pInfo->add_pucks();
	Puck* p2 = pInfo->add_pucks();
	Puck* p3 = pInfo->add_pucks();
	p1->set_id(1);
	p1->set_state(PuckState::S0);
	p2->set_id(2);
	p2->set_state(PuckState::S1);
	p3->set_id(3);
	p3->set_state(PuckState::S2);

	Pose2D* p1_pose = p1->mutable_pose();
	Pose2D* p2_pose = p2->mutable_pose();
	Pose2D* p3_pose = p3->mutable_pose();

	p1_pose->set_x(2.2);
	p1_pose->set_y(3.0);

	p2_pose->set_x(4.0);
	p2_pose->set_y(1.0);

	p3_pose->set_x(2.5);
	p3_pose->set_y(3.2);
	return pInfo;
}
