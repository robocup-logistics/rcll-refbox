
// Licensed under GPLv2. See LICENSE file. Copyright TC of the RoboCup Logistics League

/***************************************************************************
 *  mps_placing_clips.cpp - mps_placing generator for CLIPS
 *
 *  Created: Tue Apr 16 13:51:14 2013
 *  Copyright  2013  Tim Niemueller [www.niemueller.de]
 *             2017  Tobias Neumann
 ****************************************************************************/

/*  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 * - Neither the name of the authors nor the names of its contributors
 *   may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "mps_placing.h"

#include <core/threading/mutex_locker.h>
#include <mps_placing_clips/mps_placing_clips.h>

namespace mps_placing_clips {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

/** @class MPSPlacingGenerator <mps_placing_clips/mps_placing_clips.h>
 * MPS Placing integration.
 * @author Tobias Neumann
 */

/** Constructor.
 * @param env CLIPS environment to which to provide the protobuf functionality
 * @param env_mutex mutex to lock when operating on the CLIPS environment.
 */
MPSPlacingGenerator::MPSPlacingGenerator(CLIPS::Environment *env, fawkes::Mutex &env_mutex)
: clips_(env), clips_mutex_(env_mutex)
{
	setup_clips();
	is_generation_running_ = false;
	is_field_generated_    = false;
	generator_             = nullptr;
	generator_thread_      = nullptr;
	machines_              = {BASE, CAP1, CAP2, RING1, RING2, STORAGE, DELIVERY};
	width_                 = 7;
	height_                = 8;
}

/** Destructor. */
MPSPlacingGenerator::~MPSPlacingGenerator()
{
	if (is_generation_running_) {
		generate_abort();
	}
	if (generator_thread_) {
		generator_thread_->join();
		generator_thread_.reset();
	}
	generator_.reset();
	avail_fact_.reset();
	{
		fawkes::MutexLocker lock(&clips_mutex_);

		for (auto f : functions_) {
			clips_->remove_function(f);
		}
		functions_.clear();
	}
}

#define ADD_FUNCTION(n, s)    \
	clips_->add_function(n, s); \
	functions_.push_back(n);

/** Setup CLIPS environment. */
void
MPSPlacingGenerator::setup_clips()
{
	fawkes::MutexLocker lock(&clips_mutex_);

	ADD_FUNCTION("mps-generator-set-field",
	             (sigc::slot<CLIPS::Value, int, int>(
	               sigc::mem_fun(*this, &MPSPlacingGenerator::set_field))));
	ADD_FUNCTION("mps-generator-add-machine",
	             (sigc::slot<CLIPS::Value, int>(
	               sigc::mem_fun(*this, &MPSPlacingGenerator::add_machine))));
	ADD_FUNCTION("mps-generator-remove-machine",
	             (sigc::slot<CLIPS::Value, int>(
	               sigc::mem_fun(*this, &MPSPlacingGenerator::remove_machine))));
	ADD_FUNCTION("mps-generator-start",
	             (sigc::slot<void>(sigc::mem_fun(*this, &MPSPlacingGenerator::generate_start))));
	ADD_FUNCTION("mps-generator-abort",
	             (sigc::slot<void>(sigc::mem_fun(*this, &MPSPlacingGenerator::generate_abort))));
	ADD_FUNCTION("mps-generator-running",
	             (sigc::slot<CLIPS::Value>(
	               sigc::mem_fun(*this, &MPSPlacingGenerator::generate_running))));
	ADD_FUNCTION("mps-generator-field-generated",
	             (sigc::slot<CLIPS::Value>(
	               sigc::mem_fun(*this, &MPSPlacingGenerator::field_layout_generated))));
	ADD_FUNCTION("mps-generator-get-generated-field",
	             (sigc::slot<CLIPS::Values>(
	               sigc::mem_fun(*this, &MPSPlacingGenerator::get_generated_field))));
}

void
MPSPlacingGenerator::generator_thread()
{
	is_field_generated_    = generator_->solve();
	is_generation_running_ = false;
}

CLIPS::Value
MPSPlacingGenerator::set_field(int width, int height)
{
	if (width_ > 0 && height > 0) {
		width_  = width;
		height_ = height;
		return CLIPS::Value("TRUE", CLIPS::TYPE_SYMBOL);
	} else {
		return CLIPS::Value("FALSE", CLIPS::TYPE_SYMBOL);
	}
}

CLIPS::Value
MPSPlacingGenerator::add_machine(int machine_index)
{
	auto success = machines_.insert(machine_index);
	return CLIPS::Value(success.second ? "TRUE" : "FALSE", CLIPS::TYPE_SYMBOL);
}

CLIPS::Value
MPSPlacingGenerator::remove_machine(int machine_index)
{
	auto success = machines_.erase(machine_index);
	return CLIPS::Value(success ? "TRUE" : "FALSE", CLIPS::TYPE_SYMBOL);
}

void
MPSPlacingGenerator::generate_start()
{
	if (generator_thread_) {
		generator_thread_->join();
		generator_thread_.reset();
	}
	is_generation_running_ = true;
	is_field_generated_    = false;

	generator_ = std::shared_ptr<MPSPlacing>(new MPSPlacing(width_, height_, machines_));
	generator_thread_ =
	  std::shared_ptr<std::thread>(new std::thread(&MPSPlacingGenerator::generator_thread, this));
}

void
MPSPlacingGenerator::generate_abort()
{
	if (0 == pthread_cancel(generator_thread_->native_handle())) {
		generator_thread_->join();
		generator_thread_.reset();
		is_generation_running_ = false;
		is_field_generated_    = false;
	} else {
		// TODO throw?
	}
}

CLIPS::Value
MPSPlacingGenerator::generate_running()
{
	return CLIPS::Value(is_generation_running_ ? "TRUE" : "FALSE", CLIPS::TYPE_SYMBOL);
}

CLIPS::Value
MPSPlacingGenerator::field_layout_generated()
{
	return CLIPS::Value(is_field_generated_ ? "TRUE" : "FALSE", CLIPS::TYPE_SYMBOL);
}

CLIPS::Values
MPSPlacingGenerator::get_generated_field()
{
	if (!is_field_generated_) {
		return CLIPS::Values(1, CLIPS::Value("INVALID-GENERATION", CLIPS::TYPE_SYMBOL));
	}

	std::vector<MPSPlacingPlacing> poses;
	if (!generator_->get_solution(
	      poses)) { // this should never happen since it is checked in this class
		return CLIPS::Values(1, CLIPS::Value("INVALID-GENERATION-BUT-WHY", CLIPS::TYPE_SYMBOL));
	}

	CLIPS::Values machines;
	machines.reserve(poses.size() * 3);
	for (MPSPlacingPlacing pose : poses) {
		std::string type = "";
		switch (pose.type_) {
		case BASE: type = "C-BS"; break;
		case DELIVERY: type = "C-DS"; break;
		case STORAGE: type = "C-SS"; break;
		case CAP1: type = "C-CS1"; break;
		case CAP2: type = "C-CS2"; break;
		case RING1: type = "C-RS1"; break;
		case RING2: type = "C-RS2"; break;
		default: type = "NOT-SET";
		}
		machines.push_back(CLIPS::Value(type.c_str(), CLIPS::TYPE_SYMBOL));
		std::string zone = "C_Z" + std::to_string(pose.x_) + std::to_string(pose.y_);
		machines.push_back(CLIPS::Value(zone.c_str(), CLIPS::TYPE_SYMBOL));

		int rotation = -2;
		switch (pose.angle_) {
		case ANGLE_0: rotation = 0; break;
		case ANGLE_45: rotation = 45; break;
		case ANGLE_90: rotation = 90; break;
		case ANGLE_135: rotation = 135; break;
		case ANGLE_180: rotation = 180; break;
		case ANGLE_225: rotation = 225; break;
		case ANGLE_270: rotation = 270; break;
		case ANGLE_315: rotation = 315; break;
		default: rotation = -1;
		}

		machines.push_back(CLIPS::Value(rotation));
	}

	return machines;
}

} // end namespace mps_placing_clips
