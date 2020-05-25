
/****************************************************************************
 *  Clips -- Schema Robot
 *  (auto-generated, do not modify directly)
 *
 *  CLIPS REST API.
 *  Enables access to CLIPS environments.
 *
 *  API Contact: Tim Niemueller <niemueller@kbsg.rwth-aachen.de>
 *  API Version: v1beta1
 *  API License: Apache 2.0
 ****************************************************************************/
/*  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version. A runtime exception applies to
 *  this software (see LICENSE.GPL_WRE file mentioned below for details).
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  Read the full text in the LICENSE.GPL_WRE file in the doc directory.
 */

#pragma once

#define RAPIDJSON_HAS_STDSTRING 1

#include <rapidjson/fwd.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

/** Robot representation for JSON transfer. */
class Robot
{
public:
	/** Constructor. */
	Robot();
	/** Constructor from JSON.
	 * @param json JSON string to initialize from
	 */
	Robot(const std::string &json);
	/** Constructor from JSON.
	 * @param v RapidJSON value object to initialize from.
	 */
	Robot(const rapidjson::Value &v);

	/** Destructor. */
	virtual ~Robot();

	/** Get version of implemented API.
	 * @return string representation of version
	 */
	static std::string
	api_version()
	{
		return "v1beta1";
	}

	/** Render object to JSON.
	 * @param pretty true to enable pretty printing (readable spacing)
	 * @return JSON string
	 */
	virtual std::string to_json(bool pretty = false) const;
	/** Render object to JSON.
	 * @param d RapidJSON document to retrieve allocator from
	 * @param v RapidJSON value to add data to
	 */
	virtual void to_json_value(rapidjson::Document &d, rapidjson::Value &v) const;
	/** Retrieve data from JSON string.
	 * @param json JSON representation suitable for this object.
	 * Will allow partial assignment and not validate automaticaly.
	 * @see validate()
	 */
	virtual void from_json(const std::string &json);
	/** Retrieve data from JSON string.
	 * @param v RapidJSON value suitable for this object.
	 * Will allow partial assignment and not validate automaticaly.
	 * @see validate()
	 */
	virtual void from_json_value(const rapidjson::Value &v);

	/** Validate if all required fields have been set.
	 * @param subcall true if this is called from another class, e.g.,
	 * a sub-class or array holder. Will modify the kind of exception thrown.
	 * @exception std::vector<std::string> thrown if required information is
	 * missing and @p subcall is set to true. Contains a list of missing fields.
	 * @exception std::runtime_error informative message describing the missing
	 * fields
	 */
	virtual void validate(bool subcall = false) const;

	// Schema: Robot
public:
	/** Get kind value.
   * @return kind value
   */
	std::optional<std::string>
	kind() const
	{
		return kind_;
	}

	/** Set kind value.
	 * @param kind new value
	 */
	void
	set_kind(const std::string &kind)
	{
		kind_ = kind;
	}
	/** Get apiVersion value.
   * @return apiVersion value
   */
	std::optional<std::string>
	apiVersion() const
	{
		return apiVersion_;
	}

	/** Set apiVersion value.
	 * @param apiVersion new value
	 */
	void
	set_apiVersion(const std::string &apiVersion)
	{
		apiVersion_ = apiVersion;
	}
	/** Get number value.
   * @return number value
   */
	std::optional<int64_t>
	number() const
	{
		return number_;
	}

	/** Set number value.
	 * @param number new value
	 */
	void
	set_number(const int64_t &number)
	{
		number_ = number;
	}
	/** Get state value.
   * @return state value
   */
	std::optional<std::string>
	state() const
	{
		return state_;
	}

	/** Set state value.
	 * @param state new value
	 */
	void
	set_state(const std::string &state)
	{
		state_ = state;
	}
	/** Get team value.
   * @return team value
   */
	std::optional<std::string>
	team() const
	{
		return team_;
	}

	/** Set team value.
	 * @param team new value
	 */
	void
	set_team(const std::string &team)
	{
		team_ = team;
	}
	/** Get team-color value.
   * @return team-color value
   */
	std::optional<std::string>
	team_color() const
	{
		return team_color_;
	}

	/** Set team-color value.
	 * @param team_color new value
	 */
	void
	set_team_color(const std::string &team_color)
	{
		team_color_ = team_color;
	}
	/** Get name value.
   * @return name value
   */
	std::optional<std::string>
	name() const
	{
		return name_;
	}

	/** Set name value.
	 * @param name new value
	 */
	void
	set_name(const std::string &name)
	{
		name_ = name;
	}
	/** Get host value.
   * @return host value
   */
	std::optional<std::string>
	host() const
	{
		return host_;
	}

	/** Set host value.
	 * @param host new value
	 */
	void
	set_host(const std::string &host)
	{
		host_ = host;
	}
	/** Get port value.
   * @return port value
   */
	std::optional<int64_t>
	port() const
	{
		return port_;
	}

	/** Set port value.
	 * @param port new value
	 */
	void
	set_port(const int64_t &port)
	{
		port_ = port;
	}
	/** Get last-seen value.
   * @return last-seen value
   */
	std::vector<std::string>
	last_seen() const
	{
		return last_seen_;
	}

	/** Set last-seen value.
	 * @param last_seen new value
	 */
	void
	set_last_seen(const std::vector<std::string> &last_seen)
	{
		last_seen_ = last_seen;
	}
	/** Add element to last-seen array.
	 * @param last_seen new value
	 */
	void
	addto_last_seen(const std::string &&last_seen)
	{
		last_seen_.push_back(std::move(last_seen));
	}

	/** Add element to last-seen array.
	 * The move-semantics version (std::move) should be preferred.
	 * @param last_seen new value
	 */
	void
	addto_last_seen(const std::string &last_seen)
	{
		last_seen_.push_back(last_seen);
	}
	/** Get has-pose value.
   * @return has-pose value
   */
	std::optional<bool>
	has_pose() const
	{
		return has_pose_;
	}

	/** Set has-pose value.
	 * @param has_pose new value
	 */
	void
	set_has_pose(const bool &has_pose)
	{
		has_pose_ = has_pose;
	}
	/** Get pose value.
   * @return pose value
   */
	std::vector<std::string>
	pose() const
	{
		return pose_;
	}

	/** Set pose value.
	 * @param pose new value
	 */
	void
	set_pose(const std::vector<std::string> &pose)
	{
		pose_ = pose;
	}
	/** Add element to pose array.
	 * @param pose new value
	 */
	void
	addto_pose(const std::string &&pose)
	{
		pose_.push_back(std::move(pose));
	}

	/** Add element to pose array.
	 * The move-semantics version (std::move) should be preferred.
	 * @param pose new value
	 */
	void
	addto_pose(const std::string &pose)
	{
		pose_.push_back(pose);
	}
	/** Get maintenance-start-time value.
   * @return maintenance-start-time value
   */
	std::optional<float>
	maintenance_start_time() const
	{
		return maintenance_start_time_;
	}

	/** Set maintenance-start-time value.
	 * @param maintenance_start_time new value
	 */
	void
	set_maintenance_start_time(const float &maintenance_start_time)
	{
		maintenance_start_time_ = maintenance_start_time;
	}
	/** Get maintenance-cycles value.
   * @return maintenance-cycles value
   */
	std::optional<int64_t>
	maintenance_cycles() const
	{
		return maintenance_cycles_;
	}

	/** Set maintenance-cycles value.
	 * @param maintenance_cycles new value
	 */
	void
	set_maintenance_cycles(const int64_t &maintenance_cycles)
	{
		maintenance_cycles_ = maintenance_cycles;
	}
	/** Get maintenance-warning-sent value.
   * @return maintenance-warning-sent value
   */
	std::optional<bool>
	maintenance_warning_sent() const
	{
		return maintenance_warning_sent_;
	}

	/** Set maintenance-warning-sent value.
	 * @param maintenance_warning_sent new value
	 */
	void
	set_maintenance_warning_sent(const bool &maintenance_warning_sent)
	{
		maintenance_warning_sent_ = maintenance_warning_sent;
	}

private:
	std::optional<std::string> kind_;
	std::optional<std::string> apiVersion_;
	std::optional<int64_t>     number_;
	std::optional<std::string> state_;
	std::optional<std::string> team_;
	std::optional<std::string> team_color_;
	std::optional<std::string> name_;
	std::optional<std::string> host_;
	std::optional<int64_t>     port_;
	std::vector<std::string>   last_seen_;
	std::optional<bool>        has_pose_;
	std::vector<std::string>   pose_;
	std::optional<float>       maintenance_start_time_;
	std::optional<int64_t>     maintenance_cycles_;
	std::optional<bool>        maintenance_warning_sent_;
};