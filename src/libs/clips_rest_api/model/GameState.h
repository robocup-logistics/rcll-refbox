
/****************************************************************************
 *  Clips -- Schema GameState
 *  (auto-generated, do not modify directly)
 *
 *  CLIPS REST API.
 *  Enables access to CLIPS environments.
 *
 *  API Contact: Tim Niemueller <niemueller@kbsg.rwth-aachen.de>
 *  API Version: v1beta1
 *  API License: Apache 2.0
 ****************************************************************************/

#pragma once

#define RAPIDJSON_HAS_STDSTRING 1

#include <rapidjson/fwd.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

/** GameState representation for JSON transfer. */
class GameState
{
public:
	/** Constructor. */
	GameState();
	/** Constructor from JSON.
	 * @param json JSON string to initialize from
	 */
	GameState(const std::string &json);
	/** Constructor from JSON.
	 * @param v RapidJSON value object to initialize from.
	 */
	GameState(const rapidjson::Value &v);

	/** Destructor. */
	virtual ~GameState();

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

	// Schema: GameState
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
	/** Get phase value.
   * @return phase value
   */
	std::optional<std::string>
	phase() const
	{
		return phase_;
	}

	/** Set phase value.
	 * @param phase new value
	 */
	void
	set_phase(const std::string &phase)
	{
		phase_ = phase;
	}
	/** Get game-time value.
   * @return game-time value
   */
	std::optional<float>
	game_time() const
	{
		return game_time_;
	}

	/** Set game-time value.
	 * @param game_time new value
	 */
	void
	set_game_time(const float &game_time)
	{
		game_time_ = game_time;
	}
	/** Get points value.
   * @return points value
   */
	std::vector<std::string>
	points() const
	{
		return points_;
	}

	/** Set points value.
	 * @param points new value
	 */
	void
	set_points(const std::vector<std::string> &points)
	{
		points_ = points;
	}
	/** Add element to points array.
	 * @param points new value
	 */
	void
	addto_points(const std::string &&points)
	{
		points_.push_back(std::move(points));
	}

	/** Add element to points array.
	 * The move-semantics version (std::move) should be preferred.
	 * @param points new value
	 */
	void
	addto_points(const std::string &points)
	{
		points_.push_back(points);
	}
	/** Get teams value.
   * @return teams value
   */
	std::vector<std::string>
	teams() const
	{
		return teams_;
	}

	/** Set teams value.
	 * @param teams new value
	 */
	void
	set_teams(const std::vector<std::string> &teams)
	{
		teams_ = teams;
	}
	/** Add element to teams array.
	 * @param teams new value
	 */
	void
	addto_teams(const std::string &&teams)
	{
		teams_.push_back(std::move(teams));
	}

	/** Add element to teams array.
	 * The move-semantics version (std::move) should be preferred.
	 * @param teams new value
	 */
	void
	addto_teams(const std::string &teams)
	{
		teams_.push_back(teams);
	}
	/** Get over-time value.
   * @return over-time value
   */
	std::optional<bool>
	over_time() const
	{
		return over_time_;
	}

	/** Set over-time value.
	 * @param over_time new value
	 */
	void
	set_over_time(const bool &over_time)
	{
		over_time_ = over_time;
	}

private:
	std::optional<std::string> kind_;
	std::optional<std::string> apiVersion_;
	std::optional<std::string> state_;
	std::optional<std::string> phase_;
	std::optional<float>       game_time_;
	std::vector<std::string>   points_;
	std::vector<std::string>   teams_;
	std::optional<bool>        over_time_;
};