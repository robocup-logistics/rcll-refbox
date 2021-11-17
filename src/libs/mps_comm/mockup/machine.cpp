/***************************************************************************
 *  machine.cpp - Mockup communication with an MPS
 *
 *  Created: Thu 30 Jan 2020 17:39:02 CET 17:39
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

#include "machine.h"

#include "durations.h"

#include <config/yaml.h>

#include <chrono>
#include <thread>

namespace llsfrb {
namespace mps_comm {

MockupMachine::MockupMachine(const std::string &name, float exec_speed)
: Machine(name), exec_speed_(exec_speed), shutdown_(false)
{
	worker_thread_ = std::thread(&MockupMachine::queue_worker, this);
}

MockupMachine::~MockupMachine()
{
	std::unique_lock<std::mutex> lock(queue_mutex_);
	shutdown_ = true;
	lock.unlock();
	queue_condition_.notify_all();
	if (worker_thread_.joinable()) {
		worker_thread_.join();
	}
}

void
MockupMachine::register_busy_callback(std::function<void(bool)> callback)
{
	callback_busy_ = callback;
}

void
MockupMachine::register_ready_callback(std::function<void(bool)> callback)
{
	callback_ready_ = callback;
}

void
MockupMachine::register_barcode_callback(std::function<void(unsigned long)> callback)
{
	callback_barcode_ = callback;
}

void
MockupMachine::queue_worker()
{
	std::unique_lock<std::mutex> lock(queue_mutex_);
	while (!shutdown_) {
		if (queue_.empty()) {
			queue_condition_.wait(lock);
			continue;
		}
		auto cmd        = std::get<0>(queue_.front());
		auto time_point = std::get<1>(queue_.front());
		queue_.pop();
		while (!queue_condition_.wait_until(lock, time_point, [&] {
			return shutdown_ || std::chrono::system_clock::now() >= time_point;
		})) {}
		if (!shutdown_) {
			lock.unlock();
			cmd();
			lock.lock();
		}
	}
}

void
MockupMachine::conveyor_move(ConveyorDirection direction, MPSSensor sensor)
{
	using std::chrono::milliseconds;
	using std::chrono::round;
	using std::chrono::system_clock;
	callback_busy_(true);
	std::lock_guard<std::mutex> lg(queue_mutex_);
	queue_.push(
	  std::make_tuple([this] { callback_busy_(false); },
	                  system_clock::now()
	                    + std::max(min_operation_duration_,
	                               round<milliseconds>(duration_band_input_to_mid_ / exec_speed_))));
	if (sensor == INPUT || sensor == OUTPUT) {
		queue_.push(std::make_tuple([this] { callback_ready_(true); },
		                            system_clock::now()
		                              + std::max(min_operation_duration_,
		                                         round<milliseconds>(duration_band_mid_to_output_
		                                                             / exec_speed_))));
		queue_.push(
		  std::make_tuple([this] { callback_ready_(false); },
		                  system_clock::now()
		                    + std::max(min_operation_duration_,
		                               round<milliseconds>(duration_ready_at_output_ / exec_speed_))));
	}
	queue_condition_.notify_one();
}
} // namespace mps_comm
} // namespace llsfrb
