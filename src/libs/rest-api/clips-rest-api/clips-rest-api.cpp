
/***************************************************************************
 *  clips-rest-api.cpp -  CLIPS REST API
 *
 *  Created: Sat Mar 31 01:36:11 2018
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

#include "clips-rest-api.h"

extern "C" {
#include <clips/clips.h>
}
#include <core/threading/mutex_locker.h>

#include <type_traits>
using namespace fawkes;

/** @class ClipsRestApi "clips-rest-api.h"
 * REST API backend for CLIPS.
 * @author Tim Niemueller
 */

namespace llsfrb {
/** Constructor. */
ClipsRestApi::ClipsRestApi(CLIPS::Environment *env, fawkes::Mutex &env_mutex, Logger *logger)
: WebviewRestApi("clips", logger), env_(env), env_mutex_(env_mutex), logger_(logger)
{
	add_handler<WebviewRestArray<Environment>>(WebRequest::METHOD_GET,
	                                           "/",
	                                           std::bind(&ClipsRestApi::cb_list_environments, this));
	add_handler<WebviewRestArray<Fact>>(WebRequest::METHOD_GET,
	                                    "/facts",
	                                    std::bind(&ClipsRestApi::cb_get_facts,
	                                              this,
	                                              std::placeholders::_1));
	add_handler<WebviewRestArray<Fact>>(WebRequest::METHOD_GET,
	                                    "/facts/{tmpl-name}",
	                                    std::bind(&ClipsRestApi::cb_get_facts_by_tmpl_and_slots,
	                                              this,
	                                              std::placeholders::_1));

	add_handler<WebviewRestArray<Machine>>(WebRequest::METHOD_GET,
	                                       "/machines",
	                                       std::bind(&ClipsRestApi::cb_get_machines,
	                                                 this,
	                                                 std::placeholders::_1));
	add_handler<WebviewRestArray<Order>>(WebRequest::METHOD_GET,
	                                     "/orders",
	                                     std::bind(&ClipsRestApi::cb_get_orders,
	                                               this,
	                                               std::placeholders::_1));
	add_handler<WebviewRestArray<Robot>>(WebRequest::METHOD_GET,
	                                     "/robots",
	                                     std::bind(&ClipsRestApi::cb_get_robots,
	                                               this,
	                                               std::placeholders::_1));
	add_handler<WebviewRestArray<GameState>>(WebRequest::METHOD_GET,
	                                         "/game-state",
	                                         std::bind(&ClipsRestApi::cb_get_game_state,
	                                                   this,
	                                                   std::placeholders::_1));
	add_handler<WebviewRestArray<RingSpec>>(WebRequest::METHOD_GET,
	                                        "/ring-spec",
	                                        std::bind(&ClipsRestApi::cb_get_ring_spec,
	                                                  this,
	                                                  std::placeholders::_1));
	add_handler<WebviewRestArray<Points>>(WebRequest::METHOD_GET,
	                                      "/points",
	                                      std::bind(&ClipsRestApi::cb_get_points,
	                                                this,
	                                                std::placeholders::_1));
}

/** Destructor. */
ClipsRestApi::~ClipsRestApi()
{
}

/** Get a value from a fact.
 * @param fact pointer to CLIPS fact
 * @param slot_name name of field to retrieve
 * @return template-specific return value
 */
template <typename T>
T
get_value(const CLIPS::Fact::pointer &fact, const std::string &slot_name)
{
	CLIPS::Values v = fact->slot_value(slot_name);
	if (v.empty()) {
		throw Exception("No value for slot '%s'", slot_name.c_str());
	}
	if (v[0].type() == CLIPS::TYPE_SYMBOL && v[0].as_string() == "nil") {
		return T();
	}
	return v[0];
}

/** Specialization for bool.
 * @param fact pointer to CLIPS fact
 * @param slot_name name of field to retrieve
 * @return boolean value
 */
template <>
bool
get_value(const CLIPS::Fact::pointer &fact, const std::string &slot_name)
{
	CLIPS::Values v = fact->slot_value(slot_name);
	if (v.empty()) {
		throw Exception("No value for slot '%s'", slot_name.c_str());
	}
	if (v[0].type() != CLIPS::TYPE_SYMBOL) {
		throw Exception("Value for slot '%s' is not a boolean", slot_name.c_str());
	}
	return (v[0].as_string() == "TRUE");
}

/** Get value array.
 * This is not a template because the overly verbose operator API
 * of CLIPS::Value can lead to ambiguous overloads, e.g., resolving
 * std::string to std::string or const char * operators.
 * @param fact pointer to CLIPS fact
 * @param slot_name name of field to retrieve
 * @return vector of strings from multislot
 */
static std::vector<std::string>
get_values(const CLIPS::Fact::pointer &fact, const std::string &slot_name)
{
	CLIPS::Values            v = fact->slot_value(slot_name);
	std::vector<std::string> rv(v.size());
	for (size_t i = 0; i < v.size(); ++i) {
		switch (v[i].type()) {
		case CLIPS::TYPE_FLOAT: rv[i] = std::to_string(static_cast<double>(v[i])); break;
		case CLIPS::TYPE_INTEGER: rv[i] = std::to_string(static_cast<long long int>(v[i])); break;
		case CLIPS::TYPE_SYMBOL:
		case CLIPS::TYPE_STRING:
		case CLIPS::TYPE_INSTANCE_NAME: rv[i] = static_cast<std::string &>(v[i]); break;
		default: rv[i] = "CANNOT-REPRESENT"; break;
		}
	}
	return rv;
}

Fact
ClipsRestApi::gen_fact(CLIPS::Fact::pointer &fact, bool formatted)
{
	Fact retf;
	retf.set_kind("Fact");
	retf.set_apiVersion(Fact::api_version());
	retf.set_index(fact->index());
	CLIPS::Template::pointer fact_template = fact->get_template();
	if (fact_template) {
		retf.set_template_name(fact_template->name());
	} else {
		retf.set_template_name("implied");
	}

	if (formatted) {
		char tmp[16384];
		tmp[16383] = 0;
		OpenStringDestination(env_->cobj(), (char *)"ProcPPForm", tmp, 16383);
#ifdef CLIPS_OLD_63_API
		PrintFact(env_->cobj(), (char *)"ProcPPForm", (struct fact *)fact->cobj(), FALSE, FALSE);
#else
		PrintFact(env_->cobj(), (const char *)"ProcPPForm", (struct fact *)fact->cobj(), FALSE, FALSE);
#endif
		CloseStringDestination(env_->cobj(), (char *)"ProcPPForm");
		retf.set_formatted(tmp);
	} else {
		std::vector<std::string> slots = fact->slot_names();
		for (const auto &s : slots) {
			CLIPS::Values fval = fact->slot_value(s);
			SlotValue     sval;
			sval.set_name(s);
			sval.set_is_multifield(fact_template ? fact_template->is_multifield_slot(s)
			                                     : (fval.size() > 1));
			for (const auto &v : fval) {
				switch (v.type()) {
				case CLIPS::TYPE_FLOAT: sval.addto_values(std::to_string(v.as_float())); break;
				case CLIPS::TYPE_INTEGER: sval.addto_values(std::to_string(v.as_integer())); break;
				case CLIPS::TYPE_SYMBOL:
				case CLIPS::TYPE_STRING:
				case CLIPS::TYPE_INSTANCE_NAME: sval.addto_values(v.as_string()); break;
				default: sval.addto_values("ADDR"); break;
				}
			}
			retf.addto_slots(std::move(sval));
		}
	}

	return retf;
}

Machine
ClipsRestApi::gen_machine(CLIPS::Fact::pointer &fact)
{
	Machine m;
	m.set_name(get_value<std::string>(fact, "name"));
	m.set_team(get_value<std::string>(fact, "team"));
	m.set_mtype(get_value<std::string>(fact, "mtype"));
	m.set_actual_lights(get_values(fact, "actual-lights"));
	m.set_state(get_value<std::string>(fact, "state"));
	m.set_zone(get_value<std::string>(fact, "zone"));
	m.set_rotation(get_value<int64_t>(fact, "rotation"));
	m.set_bases_added(get_value<int64_t>(fact, "bases-added"));
	m.set_bases_used(get_value<int64_t>(fact, "bases-used"));
	m.set_bs_side(get_value<std::string>(fact, "bs-side"));
	m.set_bs_color(get_value<std::string>(fact, "bs-color"));
	m.set_ds_order(get_value<int64_t>(fact, "ds-order"));
	m.set_rs_ring_color(get_value<std::string>(fact, "rs-ring-color"));
	m.set_rs_ring_colors(get_values(fact, "rs-ring-colors"));
	m.set_cs_operation(get_value<std::string>(fact, "cs-operation"));
	m.set_cs_retrieved(get_value<bool>(fact, "cs-retrieved"));
	return m;
}

Order
ClipsRestApi::gen_order(CLIPS::Fact::pointer &fact)
{
	Order o;
	o.set_kind("Order");
	o.set_apiVersion(Environment::api_version());
	o.set_id(get_value<int64_t>(fact, "id"));
	o.set_complexity(get_value<std::string>(fact, "complexity"));
	o.set_competitive(get_value<bool>(fact, "competitive"));
	o.set_base_color(get_value<std::string>(fact, "base-color"));
	o.set_cap_color(get_value<std::string>(fact, "cap-color"));
	o.set_ring_colors(get_values(fact, "ring-colors"));
	o.set_quantity_requested(get_value<int64_t>(fact, "quantity-requested"));
	o.set_quantity_delivered(get_values(fact, "quantity-delivered"));
	o.set_delivery_period(get_values(fact, "delivery-period"));
	o.set_delivery_gate(get_value<int64_t>(fact, "delivery-gate"));
	o.set_active(get_value<bool>(fact, "active"));
	return o;
}

Robot
ClipsRestApi::gen_robot(CLIPS::Fact::pointer &fact)
{
	Robot o;
	o.set_kind("Robot");
	o.set_apiVersion(Environment::api_version());
	o.set_number(get_value<int64_t>(fact, "number"));
	o.set_state(get_value<std::string>(fact, "state"));
	o.set_team(get_value<std::string>(fact, "team"));
	o.set_team_color(get_value<std::string>(fact, "team-color"));
	o.set_name(get_value<std::string>(fact, "name"));
	o.set_host(get_value<std::string>(fact, "host"));
	o.set_port(get_value<int64_t>(fact, "port"));
	o.set_last_seen(get_values(fact, "last-seen"));
	o.set_has_pose(get_value<bool>(fact, "has-pose"));
	o.set_pose(get_values(fact, "pose"));
	o.set_maintenance_start_time(get_value<int64_t>(fact, "maintenance-start-time"));
	o.set_maintenance_cycles(get_value<int64_t>(fact, "maintenance-cycles"));
	o.set_maintenance_warning_sent(get_value<bool>(fact, "maintenance-warning-sent"));
	return o;
}

GameState
ClipsRestApi::gen_game_state(CLIPS::Fact::pointer &fact)
{
	GameState o;
	o.set_kind("GameState");
	o.set_apiVersion(Environment::api_version());
	o.set_state(get_value<std::string>(fact, "state"));
	o.set_phase(get_value<std::string>(fact, "phase"));
	o.set_game_time(get_value<float>(fact, "game-time"));
	o.set_points(get_values(fact, "points"));
	o.set_teams(get_values(fact, "teams"));
	o.set_over_time(get_value<bool>(fact, "over-time"));
	return o;
}

RingSpec
ClipsRestApi::gen_ring_spec(CLIPS::Fact::pointer &fact)
{
	RingSpec o;
	o.set_kind("RingSpec");
	o.set_apiVersion(Environment::api_version());
	o.set_color(get_value<std::string>(fact, "color"));
	o.set_req_bases(get_value<int64_t>(fact, "req-bases"));
	return o;
}

Points
ClipsRestApi::gen_points(CLIPS::Fact::pointer &fact)
{
	Points o;
	o.set_kind("Points");
	o.set_apiVersion(Environment::api_version());
	o.set_points(get_value<int64_t>(fact, "points"));
	o.set_team(get_value<std::string>(fact, "team"));
	o.set_game_time(get_value<float>(fact, "game-time"));
	o.set_phase(get_value<std::string>(fact, "phase"));
	o.set_reason(get_value<std::string>(fact, "reason"));
	return o;
}

bool
ClipsRestApi::match(CLIPS::Fact::pointer &fact, std::string tmpl_name, WebviewRestParams &params)

{
	std::map<std::string, std::string> slots_to_match = params.get_query_args();
	CLIPS::Template::pointer           tmpl           = fact->get_template();
	if (tmpl->name() != tmpl_name)
		return false;
	if (slots_to_match.size() == 0)
		return true;

	std::vector<std::string> slots = fact->slot_names();
	for (auto &si : slots_to_match) {
		if (std::find(slots.begin(), slots.end(), si.first) == slots.end())
			throw Exception("No slot named %s for template %s", si.first.c_str(), tmpl->name().c_str());
		else {
			std::vector<std::string> v = get_values(fact, si.first);
			// for now only single values are allowed as param
			if (v.size() > 1)
				throw Exception("Slot %s for template %s is multifield (not supported)",
				                si.first.c_str(),
				                tmpl->name().c_str());

			if (v.size() > 0 && v[0] != si.second)
				return false;
		}
	}
	return true;
}

WebviewRestArray<Environment>
ClipsRestApi::cb_list_environments()
{
	WebviewRestArray<Environment> rv;

	Environment env;
	env.set_kind("Environment");
	env.set_apiVersion(Environment::api_version());
	env.set_name("Refbox Game CLIPS Environment");
	rv.push_back(std::move(env));

	return rv;
}

WebviewRestArray<Fact>
ClipsRestApi::cb_get_facts(WebviewRestParams &params)
{
	bool formatted = (params.query_arg("formatted") == "true");

	WebviewRestArray<Fact> rv;

	MutexLocker          lock(&env_mutex_);
	CLIPS::Fact::pointer fact = env_->get_facts();
	while (fact) {
		rv.push_back(std::move(gen_fact(fact, formatted)));
		fact = fact->next();
	}

	return rv;
}

WebviewRestArray<Machine>
ClipsRestApi::cb_get_machines(WebviewRestParams &params)
{
	MutexLocker               lock(&env_mutex_);
	WebviewRestArray<Machine> rv;

	CLIPS::Fact::pointer fact = env_->get_facts();
	while (fact) {
		if (match(fact, "machine", params))
			rv.push_back(std::move(gen_machine(fact)));
		fact = fact->next();
	}
	return rv;
}

WebviewRestArray<Order>
ClipsRestApi::cb_get_orders(WebviewRestParams &params)
{
	MutexLocker             lock(&env_mutex_);
	WebviewRestArray<Order> rv;

	CLIPS::Fact::pointer fact = env_->get_facts();
	while (fact) {
		if (match(fact, "order", params))
			rv.push_back(std::move(gen_order(fact)));
		fact = fact->next();
	}
	return rv;
}

WebviewRestArray<Robot>
ClipsRestApi::cb_get_robots(WebviewRestParams &params)
{
	MutexLocker             lock(&env_mutex_);
	WebviewRestArray<Robot> rv;

	CLIPS::Fact::pointer fact = env_->get_facts();
	while (fact) {
		if (match(fact, "robot", params))
			rv.push_back(std::move(gen_robot(fact)));
		fact = fact->next();
	}
	return rv;
}

WebviewRestArray<GameState>
ClipsRestApi::cb_get_game_state(WebviewRestParams &params)
{
	MutexLocker                 lock(&env_mutex_);
	WebviewRestArray<GameState> rv;

	CLIPS::Fact::pointer fact = env_->get_facts();
	while (fact) {
		if (match(fact, "gamestate", params))
			rv.push_back(std::move(gen_game_state(fact)));
		fact = fact->next();
	}
	return rv;
}

WebviewRestArray<RingSpec>
ClipsRestApi::cb_get_ring_spec(WebviewRestParams &params)
{
	MutexLocker                lock(&env_mutex_);
	WebviewRestArray<RingSpec> rv;

	CLIPS::Fact::pointer fact = env_->get_facts();
	while (fact) {
		if (match(fact, "ring-spec", params))
			rv.push_back(std::move(gen_ring_spec(fact)));
		fact = fact->next();
	}
	return rv;
}

WebviewRestArray<Points>
ClipsRestApi::cb_get_points(fawkes::WebviewRestParams &params)
{
	MutexLocker              lock(&env_mutex_);
	WebviewRestArray<Points> rv;

	CLIPS::Fact::pointer fact = env_->get_facts();
	while (fact) {
		if (match(fact, "points", params))
			rv.push_back(std::move(gen_points(fact)));
		fact = fact->next();
	}
	return rv;
}

WebviewRestArray<Fact>
ClipsRestApi::cb_get_facts_by_tmpl_and_slots(WebviewRestParams &params)
{
	bool formatted = (params.consum_query_arg("formatted") == "true");

	WebviewRestArray<Fact> rv;

	MutexLocker          lock(&env_mutex_);
	CLIPS::Fact::pointer fact = env_->get_facts();

	while (fact) {
		if (match(fact, params.path_arg("tmpl-name"), params))
			rv.push_back(std::move(gen_fact(fact, formatted)));
		fact = fact->next();
	}
	return rv;
}

template <typename T>
WebviewRestArray<T>
ClipsRestApi::cb_get_tmpl(WebviewRestParams &params, std::string tmpl_name)
{
	//	MutexLocker lock(&env_mutex_);
	//	WebviewRestArray<T> rv;
	//
	//	CLIPS::Fact::pointer fact =  env_->get_facts();
	//	while(fact) {
	//		 CLIPS::Template::pointer tmpl = fact->get_template();
	//		 if(tmpl->name() == tmpl_name)
	//			 if(std::is_same<T, Machine>::value)
	//			 rv.push_back(std::move(gen_machine(fact)));
	//             if(std::is_same<T, Order>::value)
	//			 rv.push_back(std::move(gen_order(fact)));
	//		 fact = fact->next();
	//    }
	//
	//	return rv;
}

} //end namespace llsfrb
