
/****************************************************************************
 *  Clips -- Schema Machine
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

/** Machine representation for JSON transfer. */
class Machine
{
public:
	/** Constructor. */
	Machine();
	/** Constructor from JSON.
	 * @param json JSON string to initialize from
	 */
	Machine(const std::string &json);
	/** Constructor from JSON.
	 * @param v RapidJSON value object to initialize from.
	 */
	Machine(const rapidjson::Value &v);

	/** Destructor. */
	virtual ~Machine();

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

	// Schema: Machine
public:
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
	/** Get mtype value.
   * @return mtype value
   */
	std::optional<std::string>
	mtype() const
	{
		return mtype_;
	}

	/** Set mtype value.
	 * @param mtype new value
	 */
	void
	set_mtype(const std::string &mtype)
	{
		mtype_ = mtype;
	}
	/** Get actual-lights value.
   * @return actual-lights value
   */
	std::vector<std::string>
	actual_lights() const
	{
		return actual_lights_;
	}

	/** Set actual-lights value.
	 * @param actual_lights new value
	 */
	void
	set_actual_lights(const std::vector<std::string> &actual_lights)
	{
		actual_lights_ = actual_lights;
	}
	/** Add element to actual-lights array.
	 * @param actual_lights new value
	 */
	void
	addto_actual_lights(const std::string &&actual_lights)
	{
		actual_lights_.push_back(std::move(actual_lights));
	}

	/** Add element to actual-lights array.
	 * The move-semantics version (std::move) should be preferred.
	 * @param actual_lights new value
	 */
	void
	addto_actual_lights(const std::string &actual_lights)
	{
		actual_lights_.push_back(actual_lights);
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
	/** Get zone value.
   * @return zone value
   */
	std::optional<std::string>
	zone() const
	{
		return zone_;
	}

	/** Set zone value.
	 * @param zone new value
	 */
	void
	set_zone(const std::string &zone)
	{
		zone_ = zone;
	}
	/** Get rotation value.
   * @return rotation value
   */
	std::optional<int64_t>
	rotation() const
	{
		return rotation_;
	}

	/** Set rotation value.
	 * @param rotation new value
	 */
	void
	set_rotation(const int64_t &rotation)
	{
		rotation_ = rotation;
	}
	/** Get bases-added value.
   * @return bases-added value
   */
	std::optional<int64_t>
	bases_added() const
	{
		return bases_added_;
	}

	/** Set bases-added value.
	 * @param bases_added new value
	 */
	void
	set_bases_added(const int64_t &bases_added)
	{
		bases_added_ = bases_added;
	}
	/** Get bases-used value.
   * @return bases-used value
   */
	std::optional<int64_t>
	bases_used() const
	{
		return bases_used_;
	}

	/** Set bases-used value.
	 * @param bases_used new value
	 */
	void
	set_bases_used(const int64_t &bases_used)
	{
		bases_used_ = bases_used;
	}
	/** Get bs-side value.
   * @return bs-side value
   */
	std::optional<std::string>
	bs_side() const
	{
		return bs_side_;
	}

	/** Set bs-side value.
	 * @param bs_side new value
	 */
	void
	set_bs_side(const std::string &bs_side)
	{
		bs_side_ = bs_side;
	}
	/** Get bs-color value.
   * @return bs-color value
   */
	std::optional<std::string>
	bs_color() const
	{
		return bs_color_;
	}

	/** Set bs-color value.
	 * @param bs_color new value
	 */
	void
	set_bs_color(const std::string &bs_color)
	{
		bs_color_ = bs_color;
	}
	/** Get ds-order value.
   * @return ds-order value
   */
	std::optional<int64_t>
	ds_order() const
	{
		return ds_order_;
	}

	/** Set ds-order value.
	 * @param ds_order new value
	 */
	void
	set_ds_order(const int64_t &ds_order)
	{
		ds_order_ = ds_order;
	}
	/** Get rs-ring-color value.
   * @return rs-ring-color value
   */
	std::optional<std::string>
	rs_ring_color() const
	{
		return rs_ring_color_;
	}

	/** Set rs-ring-color value.
	 * @param rs_ring_color new value
	 */
	void
	set_rs_ring_color(const std::string &rs_ring_color)
	{
		rs_ring_color_ = rs_ring_color;
	}
	/** Get cs-operation value.
   * @return cs-operation value
   */
	std::optional<std::string>
	cs_operation() const
	{
		return cs_operation_;
	}

	/** Set cs-operation value.
	 * @param cs_operation new value
	 */
	void
	set_cs_operation(const std::string &cs_operation)
	{
		cs_operation_ = cs_operation;
	}
	/** Get cs-retrieved value.
   * @return cs-retrieved value
   */
	std::optional<bool>
	cs_retrieved() const
	{
		return cs_retrieved_;
	}

	/** Set cs-retrieved value.
	 * @param cs_retrieved new value
	 */
	void
	set_cs_retrieved(const bool &cs_retrieved)
	{
		cs_retrieved_ = cs_retrieved;
	}

private:
	std::optional<std::string> name_;
	std::optional<std::string> team_;
	std::optional<std::string> mtype_;
	std::vector<std::string>   actual_lights_;
	std::optional<std::string> state_;
	std::optional<std::string> zone_;
	std::optional<int64_t>     rotation_;
	std::optional<int64_t>     bases_added_;
	std::optional<int64_t>     bases_used_;
	std::optional<std::string> bs_side_;
	std::optional<std::string> bs_color_;
	std::optional<int64_t>     ds_order_;
	std::optional<std::string> rs_ring_color_;
	std::optional<std::string> cs_operation_;
	std::optional<bool>        cs_retrieved_;
};
