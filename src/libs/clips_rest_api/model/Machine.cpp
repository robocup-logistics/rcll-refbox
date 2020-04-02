
/****************************************************************************
 *  Machine
 *  (auto-generated, do not modify directly)
 *
 *  CLIPS REST API.
 *  Enables access to CLIPS environments.
 *
 *  API Contact: Tim Niemueller <niemueller@kbsg.rwth-aachen.de>
 *  API Version: v1beta1
 *  API License: Apache 2.0
 ****************************************************************************/

#include "Machine.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <sstream>

Machine::Machine()
{
}

Machine::Machine(const std::string &json)
{
	from_json(json);
}

Machine::Machine(const rapidjson::Value &v)
{
	from_json_value(v);
}

Machine::~Machine()
{
}

std::string
Machine::to_json(bool pretty) const
{
	rapidjson::Document d;

	to_json_value(d, d);

	rapidjson::StringBuffer buffer;
	if (pretty) {
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
		d.Accept(writer);
	} else {
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		d.Accept(writer);
	}

	return buffer.GetString();
}

void
Machine::to_json_value(rapidjson::Document &d, rapidjson::Value &v) const
{
	rapidjson::Document::AllocatorType &allocator = d.GetAllocator();
	v.SetObject();
	// Avoid unused variable warnings
	(void)allocator;

	if (name_) {
		rapidjson::Value v_name;
		v_name.SetString(*name_, allocator);
		v.AddMember("name", v_name, allocator);
	}
	if (team_) {
		rapidjson::Value v_team;
		v_team.SetString(*team_, allocator);
		v.AddMember("team", v_team, allocator);
	}
	if (mtype_) {
		rapidjson::Value v_mtype;
		v_mtype.SetString(*mtype_, allocator);
		v.AddMember("mtype", v_mtype, allocator);
	}
	rapidjson::Value v_actual_lights(rapidjson::kArrayType);
	v_actual_lights.Reserve(actual_lights_.size(), allocator);
	for (const auto &e : actual_lights_) {
		rapidjson::Value v;
		v.SetString(e, allocator);
		v_actual_lights.PushBack(v, allocator);
	}
	v.AddMember("actual-lights", v_actual_lights, allocator);
	if (state_) {
		rapidjson::Value v_state;
		v_state.SetString(*state_, allocator);
		v.AddMember("state", v_state, allocator);
	}
	if (zone_) {
		rapidjson::Value v_zone;
		v_zone.SetString(*zone_, allocator);
		v.AddMember("zone", v_zone, allocator);
	}
	if (rotation_) {
		rapidjson::Value v_rotation;
		v_rotation.SetInt64(*rotation_);
		v.AddMember("rotation", v_rotation, allocator);
	}
	if (bases_added_) {
		rapidjson::Value v_bases_added;
		v_bases_added.SetInt64(*bases_added_);
		v.AddMember("bases-added", v_bases_added, allocator);
	}
	if (bases_used_) {
		rapidjson::Value v_bases_used;
		v_bases_used.SetInt64(*bases_used_);
		v.AddMember("bases-used", v_bases_used, allocator);
	}
	if (bs_side_) {
		rapidjson::Value v_bs_side;
		v_bs_side.SetString(*bs_side_, allocator);
		v.AddMember("bs-side", v_bs_side, allocator);
	}
	if (bs_color_) {
		rapidjson::Value v_bs_color;
		v_bs_color.SetString(*bs_color_, allocator);
		v.AddMember("bs-color", v_bs_color, allocator);
	}
	if (ds_order_) {
		rapidjson::Value v_ds_order;
		v_ds_order.SetInt64(*ds_order_);
		v.AddMember("ds-order", v_ds_order, allocator);
	}
	if (rs_ring_color_) {
		rapidjson::Value v_rs_ring_color;
		v_rs_ring_color.SetString(*rs_ring_color_, allocator);
		v.AddMember("rs-ring-color", v_rs_ring_color, allocator);
	}
	rapidjson::Value v_rs_ring_colors(rapidjson::kArrayType);
	v_rs_ring_colors.Reserve(rs_ring_colors_.size(), allocator);
	for (const auto &e : rs_ring_colors_) {
		rapidjson::Value v;
		v.SetString(e, allocator);
		v_rs_ring_colors.PushBack(v, allocator);
	}
	v.AddMember("rs-ring-colors", v_rs_ring_colors, allocator);
	if (cs_operation_) {
		rapidjson::Value v_cs_operation;
		v_cs_operation.SetString(*cs_operation_, allocator);
		v.AddMember("cs-operation", v_cs_operation, allocator);
	}
	if (cs_retrieved_) {
		rapidjson::Value v_cs_retrieved;
		v_cs_retrieved.SetBool(*cs_retrieved_);
		v.AddMember("cs-retrieved", v_cs_retrieved, allocator);
	}
}

void
Machine::from_json(const std::string &json)
{
	rapidjson::Document d;
	d.Parse(json);

	from_json_value(d);
}

void
Machine::from_json_value(const rapidjson::Value &d)
{
	if (d.HasMember("name") && d["name"].IsString()) {
		name_ = d["name"].GetString();
	}
	if (d.HasMember("team") && d["team"].IsString()) {
		team_ = d["team"].GetString();
	}
	if (d.HasMember("mtype") && d["mtype"].IsString()) {
		mtype_ = d["mtype"].GetString();
	}
	if (d.HasMember("actual-lights") && d["actual-lights"].IsArray()) {
		const rapidjson::Value &a = d["actual-lights"];
		actual_lights_            = std::vector<std::string>{};

		actual_lights_.reserve(a.Size());
		for (auto &v : a.GetArray()) {
			actual_lights_.push_back(v.GetString());
		}
	}
	if (d.HasMember("state") && d["state"].IsString()) {
		state_ = d["state"].GetString();
	}
	if (d.HasMember("zone") && d["zone"].IsString()) {
		zone_ = d["zone"].GetString();
	}
	if (d.HasMember("rotation") && d["rotation"].IsInt64()) {
		rotation_ = d["rotation"].GetInt64();
	}
	if (d.HasMember("bases-added") && d["bases-added"].IsInt64()) {
		bases_added_ = d["bases-added"].GetInt64();
	}
	if (d.HasMember("bases-used") && d["bases-used"].IsInt64()) {
		bases_used_ = d["bases-used"].GetInt64();
	}
	if (d.HasMember("bs-side") && d["bs-side"].IsString()) {
		bs_side_ = d["bs-side"].GetString();
	}
	if (d.HasMember("bs-color") && d["bs-color"].IsString()) {
		bs_color_ = d["bs-color"].GetString();
	}
	if (d.HasMember("ds-order") && d["ds-order"].IsInt64()) {
		ds_order_ = d["ds-order"].GetInt64();
	}
	if (d.HasMember("rs-ring-color") && d["rs-ring-color"].IsString()) {
		rs_ring_color_ = d["rs-ring-color"].GetString();
	}
	if (d.HasMember("rs-ring-colors") && d["rs-ring-colors"].IsArray()) {
		const rapidjson::Value &a = d["rs-ring-colors"];
		rs_ring_colors_           = std::vector<std::string>{};

		rs_ring_colors_.reserve(a.Size());
		for (auto &v : a.GetArray()) {
			rs_ring_colors_.push_back(v.GetString());
		}
	}
	if (d.HasMember("cs-operation") && d["cs-operation"].IsString()) {
		cs_operation_ = d["cs-operation"].GetString();
	}
	if (d.HasMember("cs-retrieved") && d["cs-retrieved"].IsBool()) {
		cs_retrieved_ = d["cs-retrieved"].GetBool();
	}
}

void
Machine::validate(bool subcall) const
{
	std::vector<std::string> missing;
	if (!name_) {
		missing.push_back("name");
	}
	if (!team_) {
		missing.push_back("team");
	}
	if (!mtype_) {
		missing.push_back("mtype");
	}
	if (!state_) {
		missing.push_back("state");
	}
	if (!zone_) {
		missing.push_back("zone");
	}
	if (!rotation_) {
		missing.push_back("rotation");
	}
	if (!bases_added_) {
		missing.push_back("bases-added");
	}
	if (!bases_used_) {
		missing.push_back("bases-used");
	}
	if (!bs_side_) {
		missing.push_back("bs-side");
	}
	if (!bs_color_) {
		missing.push_back("bs-color");
	}
	if (!ds_order_) {
		missing.push_back("ds-order");
	}
	if (!rs_ring_color_) {
		missing.push_back("rs-ring-color");
	}
	if (!cs_operation_) {
		missing.push_back("cs-operation");
	}
	if (!cs_retrieved_) {
		missing.push_back("cs-retrieved");
	}

	if (!missing.empty()) {
		if (subcall) {
			throw missing;
		} else {
			std::ostringstream s;
			s << "Machine  is missing field" << ((missing.size() > 0) ? "s" : "") << ": ";
			for (std::vector<std::string>::size_type i = 0; i < missing.size(); ++i) {
				s << missing[i];
				if (i < (missing.size() - 1)) {
					s << ", ";
				}
			}
			throw std::runtime_error(s.str());
		}
	}
}