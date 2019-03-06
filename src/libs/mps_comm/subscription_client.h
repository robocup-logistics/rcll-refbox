#include "opc_utils.h"

namespace llsfrb {
#if 0
}
#endif
namespace modbus {
#if 0
}
#endif

#define stringify( name ) # name

class SubscriptionClient : public OpcUa::SubscriptionHandler
{
  std::shared_ptr<spdlog::logger> logger;

  void DataChange(uint32_t handle, const OpcUa::Node &node, const OpcUa::Variant &val, OpcUa::AttributeId attr) override
  {
    if(logger != nullptr)
    {
      if(mpsValue == nullptr)
        mpsValue = new OpcUtils::ReturnValue();
        
      mpsValue->setType(val.Type());

      //const char* msg = "DataChange: {} is now: {} ({}{})";
      if(val.IsArray())
      {
        mpsValue->setArray(val);
        /*if(logger != nullptr)
          OpcUtils::logNodeValueArray(val, msg, OpcUtils::REGISTER_NAMES[reg], logger);*/
      }
      else
      {
        mpsValue->setValue(val);
        /* if(logger != nullptr)
          OpcUtils::logNodeValue(val, msg, OpcUtils::REGISTER_NAMES[reg], logger); */
      }

      if(logger == nullptr)
        return;
      OpcUtils::logReturnValue(mpsValue, logger, reg, "DataChange: ");
    }
  }

public:
  typedef std::pair<OpcUtils::MPSRegister, SubscriptionClient*> pair;
  typedef std::map<OpcUtils::MPSRegister, SubscriptionClient*> map;
  OpcUa::Subscription::SharedPtr subscription;
  OpcUtils::ReturnValue* mpsValue = nullptr;
  uint32_t handle;
  OpcUtils::MPSRegister reg;
  OpcUa::Node node;

  SubscriptionClient(std::shared_ptr<spdlog::logger> logger_, OpcUtils::ReturnValue* mpsValue_) : logger(logger_), mpsValue(mpsValue_)
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
};
}
}