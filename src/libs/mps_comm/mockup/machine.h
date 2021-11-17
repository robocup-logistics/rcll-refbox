/***************************************************************************
 *  machine.h - Mockup communication with an MPS
 *
 *  Created: Mon 27 Jan 2020 21:22:26 CET 21:22
 *  Copyright  2020  Till Hofmann <hofmann@kbsg.rwth-aachen.de>
 ****************************************************************************/

/*  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  Read the full text in the LICENSE.GPL file in the doc directory.
 */

#pragma once

#include "../machine.h"

#include <chrono>
#include <condition_variable>
#include <future>
#include <queue>

namespace llsfrb {
namespace mps_comm {

class MockupMachine : public virtual Machine
{
public:
	MockupMachine(const std::string &name, float exec_speed);
	~MockupMachine() override;
	void         set_light(llsf_msgs::LightColor color,
	                       llsf_msgs::LightState state = llsf_msgs::ON,
	                       unsigned short        time  = 0) override{};
	void         conveyor_move(ConveyorDirection direction, MPSSensor sensor) override;
	void         reset_light() override{};
	void         reset() override{};
	void         register_busy_callback(std::function<void(bool)>) override;
	void         register_ready_callback(std::function<void(bool)>) override;
	void         register_barcode_callback(std::function<void(unsigned long)>) override;
	virtual void identify() = 0;

protected:
	void                    queue_worker();
	std::mutex              queue_mutex_;
	float                   exec_speed_;
	std::condition_variable queue_condition_;
	std::queue<std::tuple<std::function<void()>, std::chrono::time_point<std::chrono::system_clock>>>
	                                   queue_;
	bool                               shutdown_;
	std::thread                        worker_thread_;
	std::function<void(bool)>          callback_busy_;
	std::function<void(bool)>          callback_ready_;
	std::function<void(unsigned long)> callback_barcode_;
};

} // namespace mps_comm
} // namespace llsfrb
