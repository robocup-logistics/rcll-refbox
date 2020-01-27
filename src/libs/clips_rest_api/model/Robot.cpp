
/****************************************************************************
 *  Robot
 *  (auto-generated, do not modify directly)
 *
 *  CLIPS REST API.
 *  Enables access to CLIPS environments.
 *
 *  API Contact: Tim Niemueller <niemueller@kbsg.rwth-aachen.de>
 *  API Version: v1beta1
 *  API License: Apache 2.0
 ****************************************************************************/

#include "Robot.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <sstream>

Robot::Robot()
{
}

Robot::Robot(const std::string &json)
{
	from_json(json);
}

Robot::Robot(const rapidjson::Value &v)
{
	from_json_value(v);
}

Robot::~Robot()
{
}

std::string
Robot::to_json(bool pretty) const
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
Robot::to_json_value(rapidjson::Document &d, rapidjson::Value &v) const
{
	rapidjson::Document::AllocatorType &allocator = d.GetAllocator();
	v.SetObject();
	// Avoid unused variable warnings
	(void)allocator;

	if (kind_) {
		rapidjson::Value v_kind;
		v_kind.SetString(*kind_, allocator);
		v.AddMember("kind", v_kind, allocator);
	}
	if (apiVersion_) {
		rapidjson::Value v_apiVersion;
		v_apiVersion.SetString(*apiVersion_, allocator);
		v.AddMember("apiVersion", v_apiVersion, allocator);
	}
	if (number_) {
		rapidjson::Value v_number;
		v_number.SetInt64(*number_);
		v.AddMember("number", v_number, allocator);
	}
	if (state_) {
		rapidjson::Value v_state;
		v_state.SetString(*state_, allocator);
		v.AddMember("state", v_state, allocator);
	}
	if (team_) {
		rapidjson::Value v_team;
		v_team.SetString(*team_, allocator);
		v.AddMember("team", v_team, allocator);
	}
	if (team_color_) {
		rapidjson::Value v_team_color;
		v_team_color.SetString(*team_color_, allocator);
		v.AddMember("team-color", v_team_color, allocator);
	}
	if (name_) {
		rapidjson::Value v_name;
		v_name.SetString(*name_, allocator);
		v.AddMember("name", v_name, allocator);
	}
	if (host_) {
		rapidjson::Value v_host;
		v_host.SetString(*host_, allocator);
		v.AddMember("host", v_host, allocator);
	}
	if (port_) {
		rapidjson::Value v_port;
		v_port.SetInt64(*port_);
		v.AddMember("port", v_port, allocator);
	}
	rapidjson::Value v_last_seen(rapidjson::kArrayType);
	v_last_seen.Reserve(last_seen_.size(), allocator);
	for (const auto &e : last_seen_) {
		rapidjson::Value v;
		v.SetString(e, allocator);
		v_last_seen.PushBack(v, allocator);
	}
	v.AddMember("last-seen", v_last_seen, allocator);
	if (has_pose_) {
		rapidjson::Value v_has_pose;
		v_has_pose.SetBool(*has_pose_);
		v.AddMember("has-pose", v_has_pose, allocator);
	}
	rapidjson::Value v_pose(rapidjson::kArrayType);
	v_pose.Reserve(pose_.size(), allocator);
	for (const auto &e : pose_) {
		rapidjson::Value v;
		v.SetString(e, allocator);
		v_pose.PushBack(v, allocator);
	}
	v.AddMember("pose", v_pose, allocator);
	if (maintenance_start_time_) {
		rapidjson::Value v_maintenance_start_time;
		v_maintenance_start_time.SetFloat(*maintenance_start_time_);
		v.AddMember("maintenance-start-time", v_maintenance_start_time, allocator);
	}
	if (maintenance_cycles_) {
		rapidjson::Value v_maintenance_cycles;
		v_maintenance_cycles.SetInt64(*maintenance_cycles_);
		v.AddMember("maintenance-cycles", v_maintenance_cycles, allocator);
	}
	if (maintenance_warning_sent_) {
		rapidjson::Value v_maintenance_warning_sent;
		v_maintenance_warning_sent.SetBool(*maintenance_warning_sent_);
		v.AddMember("maintenance-warning-sent", v_maintenance_warning_sent, allocator);
	}
}

void
Robot::from_json(const std::string &json)
{
	rapidjson::Document d;
	d.Parse(json);

	from_json_value(d);
}

void
Robot::from_json_value(const rapidjson::Value &d)
{
	if (d.HasMember("kind") && d["kind"].IsString()) {
		kind_ = d["kind"].GetString();
	}
	if (d.HasMember("apiVersion") && d["apiVersion"].IsString()) {
		apiVersion_ = d["apiVersion"].GetString();
	}
	if (d.HasMember("number") && d["number"].IsInt64()) {
		number_ = d["number"].GetInt64();
	}
	if (d.HasMember("state") && d["state"].IsString()) {
		state_ = d["state"].GetString();
	}
	if (d.HasMember("team") && d["team"].IsString()) {
		team_ = d["team"].GetString();
	}
	if (d.HasMember("team-color") && d["team-color"].IsString()) {
		team_color_ = d["team-color"].GetString();
	}
	if (d.HasMember("name") && d["name"].IsString()) {
		name_ = d["name"].GetString();
	}
	if (d.HasMember("host") && d["host"].IsString()) {
		host_ = d["host"].GetString();
	}
	if (d.HasMember("port") && d["port"].IsInt64()) {
		port_ = d["port"].GetInt64();
	}
	if (d.HasMember("last-seen") && d["last-seen"].IsArray()) {
		const rapidjson::Value &a = d["last-seen"];
		last_seen_                = std::vector<std::string>{};

		last_seen_.reserve(a.Size());
		for (auto &v : a.GetArray()) {
			last_seen_.push_back(v.GetString());
		}
	}
	if (d.HasMember("has-pose") && d["has-pose"].IsBool()) {
		has_pose_ = d["has-pose"].GetBool();
	}
	if (d.HasMember("pose") && d["pose"].IsArray()) {
		const rapidjson::Value &a = d["pose"];
		pose_                     = std::vector<std::string>{};

		pose_.reserve(a.Size());
		for (auto &v : a.GetArray()) {
			pose_.push_back(v.GetString());
		}
	}
	if (d.HasMember("maintenance-start-time") && d["maintenance-start-time"].IsFloat()) {
		maintenance_start_time_ = d["maintenance-start-time"].GetFloat();
	}
	if (d.HasMember("maintenance-cycles") && d["maintenance-cycles"].IsInt64()) {
		maintenance_cycles_ = d["maintenance-cycles"].GetInt64();
	}
	if (d.HasMember("maintenance-warning-sent") && d["maintenance-warning-sent"].IsBool()) {
		maintenance_warning_sent_ = d["maintenance-warning-sent"].GetBool();
	}
}

void
Robot::validate(bool subcall) const
{
	std::vector<std::string> missing;
	if (!kind_) {
		missing.push_back("kind");
	}
	if (!apiVersion_) {
		missing.push_back("apiVersion");
	}
	if (!number_) {
		missing.push_back("number");
	}
	if (!state_) {
		missing.push_back("state");
	}
	if (!team_) {
		missing.push_back("team");
	}
	if (!team_color_) {
		missing.push_back("team-color");
	}
	if (!name_) {
		missing.push_back("name");
	}
	if (!host_) {
		missing.push_back("host");
	}
	if (!port_) {
		missing.push_back("port");
	}
	if (!has_pose_) {
		missing.push_back("has-pose");
	}
	if (!maintenance_start_time_) {
		missing.push_back("maintenance-start-time");
	}
	if (!maintenance_cycles_) {
		missing.push_back("maintenance-cycles");
	}
	if (!maintenance_warning_sent_) {
		missing.push_back("maintenance-warning-sent");
	}

	if (!missing.empty()) {
		if (subcall) {
			throw missing;
		} else {
			std::ostringstream s;
			s << "Robot  is missing field" << ((missing.size() > 0) ? "s" : "") << ": ";
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