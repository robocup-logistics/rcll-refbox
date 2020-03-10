
/****************************************************************************
 *  Points
 *  (auto-generated, do not modify directly)
 *
 *  CLIPS REST API.
 *  Enables access to CLIPS environments.
 *
 *  API Contact: Tim Niemueller <niemueller@kbsg.rwth-aachen.de>
 *  API Version: v1beta1
 *  API License: Apache 2.0
 ****************************************************************************/

#include "Points.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <sstream>

Points::Points()
{
}

Points::Points(const std::string &json)
{
	from_json(json);
}

Points::Points(const rapidjson::Value &v)
{
	from_json_value(v);
}

Points::~Points()
{
}

std::string
Points::to_json(bool pretty) const
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
Points::to_json_value(rapidjson::Document &d, rapidjson::Value &v) const
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
	if (points_) {
		rapidjson::Value v_points;
		v_points.SetInt64(*points_);
		v.AddMember("points", v_points, allocator);
	}
	if (phase_) {
		rapidjson::Value v_phase;
		v_phase.SetString(*phase_, allocator);
		v.AddMember("phase", v_phase, allocator);
	}
	if (team_) {
		rapidjson::Value v_team;
		v_team.SetString(*team_, allocator);
		v.AddMember("team", v_team, allocator);
	}
	if (game_time_) {
		rapidjson::Value v_game_time;
		v_game_time.SetFloat(*game_time_);
		v.AddMember("game-time", v_game_time, allocator);
	}
	if (reason_) {
		rapidjson::Value v_reason;
		v_reason.SetString(*reason_, allocator);
		v.AddMember("reason", v_reason, allocator);
	}
}

void
Points::from_json(const std::string &json)
{
	rapidjson::Document d;
	d.Parse(json);

	from_json_value(d);
}

void
Points::from_json_value(const rapidjson::Value &d)
{
	if (d.HasMember("kind") && d["kind"].IsString()) {
		kind_ = d["kind"].GetString();
	}
	if (d.HasMember("apiVersion") && d["apiVersion"].IsString()) {
		apiVersion_ = d["apiVersion"].GetString();
	}
	if (d.HasMember("points") && d["points"].IsInt64()) {
		points_ = d["points"].GetInt64();
	}
	if (d.HasMember("phase") && d["phase"].IsString()) {
		phase_ = d["phase"].GetString();
	}
	if (d.HasMember("team") && d["team"].IsString()) {
		team_ = d["team"].GetString();
	}
	if (d.HasMember("game-time") && d["game-time"].IsFloat()) {
		game_time_ = d["game-time"].GetFloat();
	}
	if (d.HasMember("reason") && d["reason"].IsString()) {
		reason_ = d["reason"].GetString();
	}
}

void
Points::validate(bool subcall) const
{
	std::vector<std::string> missing;
	if (!kind_) {
		missing.push_back("kind");
	}
	if (!apiVersion_) {
		missing.push_back("apiVersion");
	}
	if (!points_) {
		missing.push_back("points");
	}
	if (!phase_) {
		missing.push_back("phase");
	}
	if (!team_) {
		missing.push_back("team");
	}
	if (!game_time_) {
		missing.push_back("game-time");
	}
	if (!reason_) {
		missing.push_back("reason");
	}

	if (!missing.empty()) {
		if (subcall) {
			throw missing;
		} else {
			std::ostringstream s;
			s << "Points  is missing field" << ((missing.size() > 0) ? "s" : "") << ": ";
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