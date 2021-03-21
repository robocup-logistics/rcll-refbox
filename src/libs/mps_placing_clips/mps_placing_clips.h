
/***************************************************************************
 *  mps_placing_clips.h - mps_placing generator for CLIPS
 *
 *  Created: Tue Apr 16 13:41:13 2013
 *  Copyright  2013-2014  Tim Niemueller [www.niemueller.de]
 *                  2017  Tobias Neumann
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

#ifndef __MPS_PLACING_CLIPS_H_
#define __MPS_PLACING_CLIPS_H_

#include <core/threading/mutex.h>

#include <clipsmm.h>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <thread>

class MPSPlacing;

namespace mps_placing_clips {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

class MPSPlacingGenerator
{
public:
	MPSPlacingGenerator(CLIPS::Environment *env, fawkes::Mutex &env_mutex);
	~MPSPlacingGenerator();

	void          generate_start();
	void          generate_abort();
	CLIPS::Value  add_machine(int machine_index);
	CLIPS::Value  remove_machine(int machine_index);
	CLIPS::Value  generate_running();
	CLIPS::Value  field_layout_generated();
	CLIPS::Values get_generated_field();

private:
	void                setup_clips();
	CLIPS::Environment *clips_;
	fawkes::Mutex &     clips_mutex_;
	std::set<int>       machines_;

	void generator_thread();

	std::shared_ptr<std::thread> generator_thread_;
	std::shared_ptr<MPSPlacing>  generator_;
	bool                         is_generation_running_;
	bool                         is_field_generated_;

	fawkes::Mutex map_mutex_;

	std::list<std::string> functions_;
	CLIPS::Fact::pointer   avail_fact_;
};

} // namespace mps_placing_clips

#endif
