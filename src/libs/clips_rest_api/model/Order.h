
/****************************************************************************
 *  Clips -- Schema Order
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

/** Order representation for JSON transfer. */
class Order
{
public:
	/** Constructor. */
	Order();
	/** Constructor from JSON.
	 * @param json JSON string to initialize from
	 */
	Order(const std::string &json);
	/** Constructor from JSON.
	 * @param v RapidJSON value object to initialize from.
	 */
	Order(const rapidjson::Value &v);

	/** Destructor. */
	virtual ~Order();

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

	// Schema: Order
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
	/** Get id value.
   * @return id value
   */
	std::optional<int64_t>
	id() const
	{
		return id_;
	}

	/** Set id value.
	 * @param id new value
	 */
	void
	set_id(const int64_t &id)
	{
		id_ = id;
	}
	/** Get complexity value.
   * @return complexity value
   */
	std::optional<std::string>
	complexity() const
	{
		return complexity_;
	}

	/** Set complexity value.
	 * @param complexity new value
	 */
	void
	set_complexity(const std::string &complexity)
	{
		complexity_ = complexity;
	}
	/** Get competitive value.
   * @return competitive value
   */
	std::optional<bool>
	competitive() const
	{
		return competitive_;
	}

	/** Set competitive value.
	 * @param competitive new value
	 */
	void
	set_competitive(const bool &competitive)
	{
		competitive_ = competitive;
	}
	/** Get base-color value.
   * @return base-color value
   */
	std::optional<std::string>
	base_color() const
	{
		return base_color_;
	}

	/** Set base-color value.
	 * @param base_color new value
	 */
	void
	set_base_color(const std::string &base_color)
	{
		base_color_ = base_color;
	}
	/** Get ring-colors value.
   * @return ring-colors value
   */
	std::vector<std::string>
	ring_colors() const
	{
		return ring_colors_;
	}

	/** Set ring-colors value.
	 * @param ring_colors new value
	 */
	void
	set_ring_colors(const std::vector<std::string> &ring_colors)
	{
		ring_colors_ = ring_colors;
	}
	/** Add element to ring-colors array.
	 * @param ring_colors new value
	 */
	void
	addto_ring_colors(const std::string &&ring_colors)
	{
		ring_colors_.push_back(std::move(ring_colors));
	}

	/** Add element to ring-colors array.
	 * The move-semantics version (std::move) should be preferred.
	 * @param ring_colors new value
	 */
	void
	addto_ring_colors(const std::string &ring_colors)
	{
		ring_colors_.push_back(ring_colors);
	}
	/** Get cap-color value.
   * @return cap-color value
   */
	std::optional<std::string>
	cap_color() const
	{
		return cap_color_;
	}

	/** Set cap-color value.
	 * @param cap_color new value
	 */
	void
	set_cap_color(const std::string &cap_color)
	{
		cap_color_ = cap_color;
	}
	/** Get quantity-requested value.
   * @return quantity-requested value
   */
	std::optional<int64_t>
	quantity_requested() const
	{
		return quantity_requested_;
	}

	/** Set quantity-requested value.
	 * @param quantity_requested new value
	 */
	void
	set_quantity_requested(const int64_t &quantity_requested)
	{
		quantity_requested_ = quantity_requested;
	}
	/** Get quantity-delivered value.
   * @return quantity-delivered value
   */
	std::vector<std::string>
	quantity_delivered() const
	{
		return quantity_delivered_;
	}

	/** Set quantity-delivered value.
	 * @param quantity_delivered new value
	 */
	void
	set_quantity_delivered(const std::vector<std::string> &quantity_delivered)
	{
		quantity_delivered_ = quantity_delivered;
	}
	/** Add element to quantity-delivered array.
	 * @param quantity_delivered new value
	 */
	void
	addto_quantity_delivered(const std::string &&quantity_delivered)
	{
		quantity_delivered_.push_back(std::move(quantity_delivered));
	}

	/** Add element to quantity-delivered array.
	 * The move-semantics version (std::move) should be preferred.
	 * @param quantity_delivered new value
	 */
	void
	addto_quantity_delivered(const std::string &quantity_delivered)
	{
		quantity_delivered_.push_back(quantity_delivered);
	}
	/** Get delivery-period value.
   * @return delivery-period value
   */
	std::vector<std::string>
	delivery_period() const
	{
		return delivery_period_;
	}

	/** Set delivery-period value.
	 * @param delivery_period new value
	 */
	void
	set_delivery_period(const std::vector<std::string> &delivery_period)
	{
		delivery_period_ = delivery_period;
	}
	/** Add element to delivery-period array.
	 * @param delivery_period new value
	 */
	void
	addto_delivery_period(const std::string &&delivery_period)
	{
		delivery_period_.push_back(std::move(delivery_period));
	}

	/** Add element to delivery-period array.
	 * The move-semantics version (std::move) should be preferred.
	 * @param delivery_period new value
	 */
	void
	addto_delivery_period(const std::string &delivery_period)
	{
		delivery_period_.push_back(delivery_period);
	}
	/** Get delivery-gate value.
   * @return delivery-gate value
   */
	std::optional<int64_t>
	delivery_gate() const
	{
		return delivery_gate_;
	}

	/** Set delivery-gate value.
	 * @param delivery_gate new value
	 */
	void
	set_delivery_gate(const int64_t &delivery_gate)
	{
		delivery_gate_ = delivery_gate;
	}
	/** Get active value.
   * @return active value
   */
	std::optional<bool>
	active() const
	{
		return active_;
	}

	/** Set active value.
	 * @param active new value
	 */
	void
	set_active(const bool &active)
	{
		active_ = active;
	}

private:
	std::optional<std::string> kind_;
	std::optional<std::string> apiVersion_;
	std::optional<int64_t>     id_;
	std::optional<std::string> complexity_;
	std::optional<bool>        competitive_;
	std::optional<std::string> base_color_;
	std::vector<std::string>   ring_colors_;
	std::optional<std::string> cap_color_;
	std::optional<int64_t>     quantity_requested_;
	std::vector<std::string>   quantity_delivered_;
	std::vector<std::string>   delivery_period_;
	std::optional<int64_t>     delivery_gate_;
	std::optional<bool>        active_;
};