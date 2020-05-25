
/****************************************************************************
 *  Order
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

#include "Order.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <sstream>

Order::Order()
{
}

Order::Order(const std::string &json)
{
	from_json(json);
}

Order::Order(const rapidjson::Value &v)
{
	from_json_value(v);
}

Order::~Order()
{
}

std::string
Order::to_json(bool pretty) const
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
Order::to_json_value(rapidjson::Document &d, rapidjson::Value &v) const
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
	if (id_) {
		rapidjson::Value v_id;
		v_id.SetInt64(*id_);
		v.AddMember("id", v_id, allocator);
	}
	if (complexity_) {
		rapidjson::Value v_complexity;
		v_complexity.SetString(*complexity_, allocator);
		v.AddMember("complexity", v_complexity, allocator);
	}
	if (competitive_) {
		rapidjson::Value v_competitive;
		v_competitive.SetBool(*competitive_);
		v.AddMember("competitive", v_competitive, allocator);
	}
	if (base_color_) {
		rapidjson::Value v_base_color;
		v_base_color.SetString(*base_color_, allocator);
		v.AddMember("base-color", v_base_color, allocator);
	}
	rapidjson::Value v_ring_colors(rapidjson::kArrayType);
	v_ring_colors.Reserve(ring_colors_.size(), allocator);
	for (const auto &e : ring_colors_) {
		rapidjson::Value v;
		v.SetString(e, allocator);
		v_ring_colors.PushBack(v, allocator);
	}
	v.AddMember("ring-colors", v_ring_colors, allocator);
	if (cap_color_) {
		rapidjson::Value v_cap_color;
		v_cap_color.SetString(*cap_color_, allocator);
		v.AddMember("cap-color", v_cap_color, allocator);
	}
	if (quantity_requested_) {
		rapidjson::Value v_quantity_requested;
		v_quantity_requested.SetInt64(*quantity_requested_);
		v.AddMember("quantity-requested", v_quantity_requested, allocator);
	}
	rapidjson::Value v_quantity_delivered(rapidjson::kArrayType);
	v_quantity_delivered.Reserve(quantity_delivered_.size(), allocator);
	for (const auto &e : quantity_delivered_) {
		rapidjson::Value v;
		v.SetString(e, allocator);
		v_quantity_delivered.PushBack(v, allocator);
	}
	v.AddMember("quantity-delivered", v_quantity_delivered, allocator);
	rapidjson::Value v_delivery_period(rapidjson::kArrayType);
	v_delivery_period.Reserve(delivery_period_.size(), allocator);
	for (const auto &e : delivery_period_) {
		rapidjson::Value v;
		v.SetString(e, allocator);
		v_delivery_period.PushBack(v, allocator);
	}
	v.AddMember("delivery-period", v_delivery_period, allocator);
	if (delivery_gate_) {
		rapidjson::Value v_delivery_gate;
		v_delivery_gate.SetInt64(*delivery_gate_);
		v.AddMember("delivery-gate", v_delivery_gate, allocator);
	}
	if (active_) {
		rapidjson::Value v_active;
		v_active.SetBool(*active_);
		v.AddMember("active", v_active, allocator);
	}
}

void
Order::from_json(const std::string &json)
{
	rapidjson::Document d;
	d.Parse(json);

	from_json_value(d);
}

void
Order::from_json_value(const rapidjson::Value &d)
{
	if (d.HasMember("kind") && d["kind"].IsString()) {
		kind_ = d["kind"].GetString();
	}
	if (d.HasMember("apiVersion") && d["apiVersion"].IsString()) {
		apiVersion_ = d["apiVersion"].GetString();
	}
	if (d.HasMember("id") && d["id"].IsInt64()) {
		id_ = d["id"].GetInt64();
	}
	if (d.HasMember("complexity") && d["complexity"].IsString()) {
		complexity_ = d["complexity"].GetString();
	}
	if (d.HasMember("competitive") && d["competitive"].IsBool()) {
		competitive_ = d["competitive"].GetBool();
	}
	if (d.HasMember("base-color") && d["base-color"].IsString()) {
		base_color_ = d["base-color"].GetString();
	}
	if (d.HasMember("ring-colors") && d["ring-colors"].IsArray()) {
		const rapidjson::Value &a = d["ring-colors"];
		ring_colors_              = std::vector<std::string>{};

		ring_colors_.reserve(a.Size());
		for (auto &v : a.GetArray()) {
			ring_colors_.push_back(v.GetString());
		}
	}
	if (d.HasMember("cap-color") && d["cap-color"].IsString()) {
		cap_color_ = d["cap-color"].GetString();
	}
	if (d.HasMember("quantity-requested") && d["quantity-requested"].IsInt64()) {
		quantity_requested_ = d["quantity-requested"].GetInt64();
	}
	if (d.HasMember("quantity-delivered") && d["quantity-delivered"].IsArray()) {
		const rapidjson::Value &a = d["quantity-delivered"];
		quantity_delivered_       = std::vector<std::string>{};

		quantity_delivered_.reserve(a.Size());
		for (auto &v : a.GetArray()) {
			quantity_delivered_.push_back(v.GetString());
		}
	}
	if (d.HasMember("delivery-period") && d["delivery-period"].IsArray()) {
		const rapidjson::Value &a = d["delivery-period"];
		delivery_period_          = std::vector<std::string>{};

		delivery_period_.reserve(a.Size());
		for (auto &v : a.GetArray()) {
			delivery_period_.push_back(v.GetString());
		}
	}
	if (d.HasMember("delivery-gate") && d["delivery-gate"].IsInt64()) {
		delivery_gate_ = d["delivery-gate"].GetInt64();
	}
	if (d.HasMember("active") && d["active"].IsBool()) {
		active_ = d["active"].GetBool();
	}
}

void
Order::validate(bool subcall) const
{
	std::vector<std::string> missing;
	if (!kind_) {
		missing.push_back("kind");
	}
	if (!apiVersion_) {
		missing.push_back("apiVersion");
	}
	if (!id_) {
		missing.push_back("id");
	}
	if (!complexity_) {
		missing.push_back("complexity");
	}
	if (!competitive_) {
		missing.push_back("competitive");
	}
	if (!base_color_) {
		missing.push_back("base-color");
	}
	if (!cap_color_) {
		missing.push_back("cap-color");
	}
	if (!quantity_requested_) {
		missing.push_back("quantity-requested");
	}
	if (!delivery_gate_) {
		missing.push_back("delivery-gate");
	}
	if (!active_) {
		missing.push_back("active");
	}

	if (!missing.empty()) {
		if (subcall) {
			throw missing;
		} else {
			std::ostringstream s;
			s << "Order  is missing field" << ((missing.size() > 0) ? "s" : "") << ": ";
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