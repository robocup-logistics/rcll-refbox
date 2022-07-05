
/***************************************************************************
 *  clips-rest-api.h -  CLIPS REST API
 *
 *  Created: Sat Mar 31 01:35:21 2018
 *  Copyright  2006-2018  Tim Niemueller [www.niemueller.de]
 *  Copyright  2020       Mostafa Gomaa  [mostafa.go@gmail.com]
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

#include "model/Environment.h"
#include "model/Fact.h"
#include "model/GameState.h"
#include "model/Machine.h"
#include "model/Order.h"
#include "model/Points.h"
#include "model/RingSpec.h"
#include "model/Robot.h"

#include <core/utils/lockptr.h>
#include <webview/rest_api.h>
#include <webview/rest_array.h>

#include <clipsmm.h>

namespace fawkes {
//from fawkes::WebviewAspect
class WebviewRestApiManager;
//class WebviewRestArray;
class WebviewRestParams;
class WebviewRestApi;
} // namespace fawkes

using namespace fawkes;
namespace llsfrb {

class Logger;

class ClipsRestApi : public WebviewRestApi
{
public:
	ClipsRestApi(CLIPS::Environment *env, fawkes::Mutex &env_mutex, Logger *logger);
	~ClipsRestApi();

private:
	fawkes::WebviewRestArray<Environment> cb_list_environments();
	fawkes::WebviewRestArray<Fact>        cb_get_facts(fawkes::WebviewRestParams &params);
	fawkes::WebviewRestArray<Fact> cb_get_facts_by_tmpl_and_slots(fawkes::WebviewRestParams &params);
	fawkes::WebviewRestArray<Machine>   cb_get_machines(fawkes::WebviewRestParams &params);
	fawkes::WebviewRestArray<Order>     cb_get_orders(fawkes::WebviewRestParams &params);
	fawkes::WebviewRestArray<Robot>     cb_get_robots(fawkes::WebviewRestParams &params);
	fawkes::WebviewRestArray<GameState> cb_get_game_state(fawkes::WebviewRestParams &params);
	fawkes::WebviewRestArray<RingSpec>  cb_get_ring_spec(fawkes::WebviewRestParams &params);
	fawkes::WebviewRestArray<Points>    cb_get_points(fawkes::WebviewRestParams &params);
	template <typename T>
	fawkes::WebviewRestArray<T> cb_get_tmpl(fawkes::WebviewRestParams &params, std::string tmpl_name);

	Fact      gen_fact(CLIPS::Fact::pointer &fact, bool formatted);
	Machine   gen_machine(CLIPS::Fact::pointer &fact);
	Order     gen_order(CLIPS::Fact::pointer &fact);
	Robot     gen_robot(CLIPS::Fact::pointer &fact);
	GameState gen_game_state(CLIPS::Fact::pointer &fact);
	RingSpec  gen_ring_spec(CLIPS::Fact::pointer &fact);
	Points    gen_points(CLIPS::Fact::pointer &fact);

	bool match(CLIPS::Fact::pointer &fact, std::string tmpl_name, fawkes::WebviewRestParams &params);

private:
	CLIPS::Environment *env_;

	fawkes::Mutex &env_mutex_;
	Logger        *logger_;
};
} //end namespace llsfrb
