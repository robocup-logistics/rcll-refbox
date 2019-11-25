/***************************************************************************
 *  subscription_client.h - An OPC-UA subscription client
 *
 *  Created: Thu 21 Feb 2019 13:29:11 CET 13:29
 *  Copyright  2019  Alex Maestrini <maestrini@student.tugraz.at>
 *                   Till Hofmann <hofmann@kbsg.rwth-aachen.de>
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

#include "opc_utils.h"

namespace llsfrb {
#if 0
}
#endif
namespace mps_comm {
#if 0
}
#endif

class SubscriptionClient : public OpcUa::SubscriptionHandler
{
public:
	typedef std::function<void(OpcUtils::ReturnValue *)>           ReturnValueCallback;
	typedef std::pair<OpcUtils::MPSRegister, SubscriptionClient *> pair;
	typedef std::map<OpcUtils::MPSRegister, SubscriptionClient *>  map;
	OpcUa::Subscription::SharedPtr                                 subscription;
	OpcUtils::ReturnValue *                                        mpsValue = nullptr;
	uint32_t                                                       handle;
	OpcUtils::MPSRegister                                          reg;
	OpcUa::Node                                                    node;

	SubscriptionClient(std::shared_ptr<spdlog::logger> logger_, OpcUtils::ReturnValue *mpsValue_)
	: mpsValue(mpsValue_), logger(logger_)
	{
	}
	SubscriptionClient(std::shared_ptr<spdlog::logger> logger_) : SubscriptionClient(logger_, nullptr)
	{
	}
	SubscriptionClient() : SubscriptionClient(nullptr)
	{
	}
	~SubscriptionClient()
	{
		delete mpsValue;
	}

	void
	add_callback(ReturnValueCallback callback)
	{
		callbacks.push_back(callback);
	}

protected:
	std::shared_ptr<spdlog::logger>  logger;
	std::vector<ReturnValueCallback> callbacks;

	void
	DataChange(uint32_t              handle,
	           const OpcUa::Node &   node,
	           const OpcUa::Variant &val,
	           OpcUa::AttributeId    attr) override
	{
		if (logger != nullptr) {
			if (mpsValue == nullptr)
				mpsValue = new OpcUtils::ReturnValue();

			mpsValue->setType(val.Type());

			//const char* msg = "DataChange: {} is now: {} ({}{})";
			if (val.IsArray()) {
				mpsValue->setArray(val);
				/*if(logger != nullptr)
          OpcUtils::logNodeValueArray(val, msg, OpcUtils::REGISTER_NAMES[reg], logger);*/
			} else {
				mpsValue->setValue(val);
				/* if(logger != nullptr)
          OpcUtils::logNodeValue(val, msg, OpcUtils::REGISTER_NAMES[reg], logger); */
			}

			if (logger != nullptr)
				OpcUtils::logReturnValue(mpsValue, logger, reg, "DataChange: ");

			if (callbacks.size() > 0)
				for (ReturnValueCallback callback : callbacks)
					callback(mpsValue);
		}
	}
};
} // namespace mps_comm
} // namespace llsfrb
