
/****************************************************************************
 *  GameState
 *  (auto-generated, do not modify directly)
 *
 *  CLIPS REST API.
 *  Enables access to CLIPS environments.
 *
 *  API Contact: Tim Niemueller <niemueller@kbsg.rwth-aachen.de>
 *  API Version: v1beta1
 *  API License: Apache 2.0
 ****************************************************************************/

#include "GameState.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <sstream>

GameState::GameState()
{
}

GameState::GameState(const std::string &json)
{
	from_json(json);
}

GameState::GameState(const rapidjson::Value &v)
{
	from_json_value(v);
}

GameState::~GameState()
{
}

std::string
GameState::to_json(bool pretty) const
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
GameState::to_json_value(rapidjson::Document &d, rapidjson::Value &v) const
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
	if (state_) {
		rapidjson::Value v_state;
		v_state.SetString(*state_, allocator);
		v.AddMember("state", v_state, allocator);
	}
	if (phase_) {
		rapidjson::Value v_phase;
		v_phase.SetString(*phase_, allocator);
		v.AddMember("phase", v_phase, allocator);
	}
	if (game_time_) {
		rapidjson::Value v_game_time;
		v_game_time.SetFloat(*game_time_);
		v.AddMember("game-time", v_game_time, allocator);
	}
	rapidjson::Value v_points(rapidjson::kArrayType);
	v_points.Reserve(points_.size(), allocator);
	for (const auto &e : points_) {
		rapidjson::Value v;
		v.SetString(e, allocator);
		v_points.PushBack(v, allocator);
	}
	v.AddMember("points", v_points, allocator);
	rapidjson::Value v_teams(rapidjson::kArrayType);
	v_teams.Reserve(teams_.size(), allocator);
	for (const auto &e : teams_) {
		rapidjson::Value v;
		v.SetString(e, allocator);
		v_teams.PushBack(v, allocator);
	}
	v.AddMember("teams", v_teams, allocator);
	if (over_time_) {
		rapidjson::Value v_over_time;
		v_over_time.SetBool(*over_time_);
		v.AddMember("over-time", v_over_time, allocator);
	}
}

void
GameState::from_json(const std::string &json)
{
	rapidjson::Document d;
	d.Parse(json);

	from_json_value(d);
}

void
GameState::from_json_value(const rapidjson::Value &d)
{
	if (d.HasMember("kind") && d["kind"].IsString()) {
		kind_ = d["kind"].GetString();
	}
	if (d.HasMember("apiVersion") && d["apiVersion"].IsString()) {
		apiVersion_ = d["apiVersion"].GetString();
	}
	if (d.HasMember("state") && d["state"].IsString()) {
		state_ = d["state"].GetString();
	}
	if (d.HasMember("phase") && d["phase"].IsString()) {
		phase_ = d["phase"].GetString();
	}
	if (d.HasMember("game-time") && d["game-time"].IsFloat()) {
		game_time_ = d["game-time"].GetFloat();
	}
	if (d.HasMember("points") && d["points"].IsArray()) {
		const rapidjson::Value &a = d["points"];
		points_                   = std::vector<std::string>{};

		points_.reserve(a.Size());
		for (auto &v : a.GetArray()) {
			points_.push_back(v.GetString());
		}
	}
	if (d.HasMember("teams") && d["teams"].IsArray()) {
		const rapidjson::Value &a = d["teams"];
		teams_                    = std::vector<std::string>{};

		teams_.reserve(a.Size());
		for (auto &v : a.GetArray()) {
			teams_.push_back(v.GetString());
		}
	}
	if (d.HasMember("over-time") && d["over-time"].IsBool()) {
		over_time_ = d["over-time"].GetBool();
	}
}

void
GameState::validate(bool subcall) const
{
	std::vector<std::string> missing;
	if (!kind_) {
		missing.push_back("kind");
	}
	if (!apiVersion_) {
		missing.push_back("apiVersion");
	}
	if (!state_) {
		missing.push_back("state");
	}
	if (!phase_) {
		missing.push_back("phase");
	}
	if (!game_time_) {
		missing.push_back("game-time");
	}
	if (!over_time_) {
		missing.push_back("over-time");
	}

	if (!missing.empty()) {
		if (subcall) {
			throw missing;
		} else {
			std::ostringstream s;
			s << "GameState  is missing field" << ((missing.size() > 0) ? "s" : "") << ": ";
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