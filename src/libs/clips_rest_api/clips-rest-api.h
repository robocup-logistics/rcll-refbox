
/***************************************************************************
 *  clips-rest-api.h -  CLIPS REST API
 *
 *  Created: Sat Mar 31 01:35:21 2018
 *  Copyright  2006-2018  Tim Niemueller [www.niemueller.de]
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
#include "model/Machine.h"
#include "model/Order.h"

#include <core/threading/thread.h>
#include <core/utils/lockptr.h>
#include <webview/rest_api.h>
#include <webview/rest_array.h>

#include <clipsmm.h>

namespace fawkes {
//from fawkes::WebviewAspect
class WebviewRestApiManager;
} // namespace fawkes

namespace llsfrb {

class Logger;

class ClipsRestApi : public fawkes::Thread
{
public:
	ClipsRestApi(CLIPS::Environment *           env,
	             fawkes::Mutex &                env_mutex,
	             fawkes::WebviewRestApiManager *webview_rest_api_manager,
	             Logger *                       logger);
	~ClipsRestApi();

	virtual void init();
	virtual void loop();
	virtual void finalize();

private:
	template <typename T>
	struct FactMap
	{
		std::string                       template_name;
		std::string                       id_slot_name;
		std::map<T, CLIPS::Fact::pointer> fact_by_id;
	};

private:
	template <typename T>
	void gen_precompute_factmap(FactMap<T> &fm);

	WebviewRestArray<Environment> cb_list_environments();
	WebviewRestArray<Fact>        cb_get_facts(fawkes::WebviewRestParams &params);
	WebviewRestArray<Machine>     cb_get_machines();
	WebviewRestArray<Order>       cb_get_orders();

	Fact    gen_fact(CLIPS::Fact::pointer &fact, bool formatted);
	Machine gen_machine(CLIPS::Fact::pointer &fact);
	Order   gen_order(CLIPS::Fact::pointer &fact);

private:
	fawkes::WebviewRestApiManager *webview_rest_api_manager_;
	fawkes::WebviewRestApi *       rest_api_;
	CLIPS::Environment *           env_;

	fawkes::Mutex &env_mutex_;
	Logger *       logger_;
};
} //end namespace llsfrb
