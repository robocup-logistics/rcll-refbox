#include "opc_utils.h"

namespace llsfrb
{
#if 0
}
#endif
namespace modbus
{
#if 0
}
#endif

// MPS Register related stuff

OpcUtils::MPSRegister operator+(const OpcUtils::MPSRegister a, const OpcUtils::MPSRegister b)
{
  return (OpcUtils::MPSRegister)((int)a + (int)b);
}


// ReturnValue related stuff

bool OpcUtils::ReturnValue::isEmpty()
{
  return isEmpty_;
}

bool OpcUtils::ReturnValue::isVector()
{
  return isVector_;
}

bool OpcUtils::ReturnValue::isScalar()
{
  return !isVector_;
}

OpcUa::VariantType OpcUtils::ReturnValue::getType()
{
  return type_;
}

void OpcUtils::ReturnValue::setType(OpcUa::VariantType type)
{
  type_ = type;
}

std::string OpcUtils::ReturnValue::ToString()
{
  if (!isVector_)
  {
    switch (type_)
    {
    case OpcUa::VariantType::UINT16:
      return std::to_string(uint16_s);
    case OpcUa::VariantType::UINT32:
      return std::to_string(uint32_s);
    case OpcUa::VariantType::UINT64:
      return std::to_string(uint64_s);
    case OpcUa::VariantType::INT16:
      return std::to_string(int16_s);
    case OpcUa::VariantType::INT32:
      return std::to_string(int32_s);
    case OpcUa::VariantType::INT64:
      return std::to_string(int64_s);
    case OpcUa::VariantType::FLOAT:
      return std::to_string(float_s);
    case OpcUa::VariantType::DOUBLE:
      return std::to_string(double_s);
    case OpcUa::VariantType::BOOLEAN:
      return std::to_string(bool_s);
    case OpcUa::VariantType::BYTE:
      return str2byteStr(OpcUa::Variant(uint8_s).ToString());
    default:
      return string_s;
    }
  }
  throw std::runtime_error("This value is not a scalar but an array");
}

std::vector<std::string> OpcUtils::ReturnValue::ToStringVector()
{
  if (isVector_)
  {
    std::vector<std::string> ret;
    switch (type_)
    {
    case OpcUa::VariantType::UINT16:
      for (auto i : uint16_v)
        ret.push_back(std::to_string(i));
      return ret;
    case OpcUa::VariantType::UINT32:
      for (auto i : uint32_v)
        ret.push_back(std::to_string(i));
      return ret;
    case OpcUa::VariantType::UINT64:
      for (auto i : uint64_v)
        ret.push_back(std::to_string(i));
      return ret;
    case OpcUa::VariantType::INT16:
      for (auto i : int16_v)
        ret.push_back(std::to_string(i));
      return ret;
    case OpcUa::VariantType::INT32:
      for (auto i : int32_v)
        ret.push_back(std::to_string(i));
      return ret;
    case OpcUa::VariantType::INT64:
      for (auto i : int64_v)
        ret.push_back(std::to_string(i));
      return ret;
    case OpcUa::VariantType::FLOAT:
      for (auto i : float_v)
        ret.push_back(std::to_string(i));
      return ret;
    case OpcUa::VariantType::DOUBLE:
      for (auto i : double_v)
        ret.push_back(std::to_string(i));
      return ret;
    case OpcUa::VariantType::BOOLEAN:
      for (auto i : uint16_v)
        ret.push_back(std::to_string(i));
      return ret;
    case OpcUa::VariantType::BYTE:
      for (auto i : uint8_v)
        ret.push_back(str2byteStr(OpcUa::Variant(i).ToString()));
      return ret;
    default:
      return string_v;
    }
  }
  throw std::runtime_error("This value is not an array but a scalar");
}

size_t OpcUtils::ReturnValue::ToInt()
{
  if (!isVector_)
  {
    switch (type_)
    {
    case OpcUa::VariantType::UINT16:
      return uint16_s;
    case OpcUa::VariantType::UINT32:
      return uint32_s;
    case OpcUa::VariantType::UINT64:
      return uint64_s;
    case OpcUa::VariantType::INT16:
      return int16_s;
    case OpcUa::VariantType::INT32:
      return int32_s;
    case OpcUa::VariantType::INT64:
      return int64_s;
    case OpcUa::VariantType::FLOAT:
      return float_s;
    case OpcUa::VariantType::DOUBLE:
      return double_s;
    case OpcUa::VariantType::BOOLEAN:
      return bool_s;
    case OpcUa::VariantType::BYTE:
      return uint8_s;
    default:
      try
      {
        return atoi(string_s.c_str());
      }
      catch (const std::exception &e)
      {
      }
      throw std::runtime_error("This value is convertable to int");
      return 0;
    }
  }
  throw std::runtime_error("This value is not a scalar but an array");
}

std::vector<size_t> OpcUtils::ReturnValue::ToIntVector()
{
  if (isVector_)
  {
    std::vector<size_t> ret;
    switch (type_)
    {
    case OpcUa::VariantType::UINT16:
      for (auto i : uint16_v)
        ret.push_back(i);
      return ret;
    case OpcUa::VariantType::UINT32:
      for (auto i : uint32_v)
        ret.push_back(i);
      return ret;
    case OpcUa::VariantType::UINT64:
      for (auto i : uint64_v)
        ret.push_back(i);
      return ret;
    case OpcUa::VariantType::INT16:
      for (auto i : int16_v)
        ret.push_back(i);
      return ret;
    case OpcUa::VariantType::INT32:
      for (auto i : int32_v)
        ret.push_back(i);
      return ret;
    case OpcUa::VariantType::INT64:
      for (auto i : int64_v)
        ret.push_back(i);
      return ret;
    case OpcUa::VariantType::FLOAT:
      for (auto i : float_v)
        ret.push_back(i);
      return ret;
    case OpcUa::VariantType::DOUBLE:
      for (auto i : double_v)
        ret.push_back(i);
      return ret;
    case OpcUa::VariantType::BOOLEAN:
      for (auto i : uint16_v)
        ret.push_back(i);
      return ret;
    case OpcUa::VariantType::BYTE:
      for (auto i : uint8_v)
        ret.push_back(i);
      return ret;
    default:
      try
      {
        for (auto i : string_v)
          ret.push_back(std::atoi(i.c_str()));
        return ret;
      }
      catch (const std::exception &e)
      {
        ret.clear();
        return ret;
      }
    }
  }
  throw std::runtime_error("This value is not an array but a scalar");
}

void OpcUtils::ReturnValue::setArray(const OpcUa::Variant &val)
{
  isVector_ = true;
  isEmpty_ = false;
  switch (val.Type())
  {
  case OpcUa::VariantType::UINT16:
    uint16_v = val.As<std::vector<uint16_t>>();
    break;
  case OpcUa::VariantType::UINT32:
    uint32_v = val.As<std::vector<uint32_t>>();
    break;
  case OpcUa::VariantType::UINT64:
    uint64_v = val.As<std::vector<uint64_t>>();
    break;
  case OpcUa::VariantType::INT16:
    int16_v = val.As<std::vector<int16_t>>();
    break;
  case OpcUa::VariantType::INT32:
    int32_v = val.As<std::vector<int32_t>>();
    break;
  case OpcUa::VariantType::INT64:
    int64_v = val.As<std::vector<int64_t>>();
    break;
  case OpcUa::VariantType::FLOAT:
    float_v = val.As<std::vector<float>>();
    break;
  case OpcUa::VariantType::DOUBLE:
    double_v = val.As<std::vector<double>>();
    break;
  case OpcUa::VariantType::BOOLEAN:
    bool_v = val.As<std::vector<bool>>();
    break;
  case OpcUa::VariantType::BYTE:
    uint8_v = val.As<std::vector<uint8_t>>();
    break;
  default:
    string_v = val.As<std::vector<std::string>>();
    break;
  }
}

void OpcUtils::ReturnValue::setValue(const OpcUa::Variant &val)
{
  isVector_ = false;
  isEmpty_ = false;
  switch (val.Type())
  {
  case OpcUa::VariantType::UINT16:
    uint16_s = static_cast<uint16_t>(val);
    break;
  case OpcUa::VariantType::UINT32:
    uint32_s = static_cast<uint32_t>(val);
    break;
  case OpcUa::VariantType::UINT64:
    uint64_s = static_cast<uint64_t>(val);
    break;
  case OpcUa::VariantType::INT16:
    int16_s = static_cast<int16_t>(val);
    break;
  case OpcUa::VariantType::INT32:
    int32_s = static_cast<int32_t>(val);
    break;
  case OpcUa::VariantType::INT64:
    int64_s = static_cast<int64_t>(val);
    break;
  case OpcUa::VariantType::FLOAT:
    float_s = static_cast<float>(val);
    break;
  case OpcUa::VariantType::DOUBLE:
    double_s = static_cast<double>(val);
    break;
  case OpcUa::VariantType::BOOLEAN:
    bool_s = static_cast<bool>(val);
    break;
  case OpcUa::VariantType::BYTE:
    uint8_s = static_cast<uint8_t>(val);
    break;
  default:
    string_s = static_cast<std::string>(val);
    break;
  }
}


// OpcUtils related stuff

// Static values

const std::vector<std::string> OpcUtils::REGISTER_NAMES({ "ACTION_ID_IN", "BARCODE_IN", "DATA_IN", "ERROR_IN", "SLIDECOUNT_IN", "STATUS_BUSY_IN", "STATUS_ENABLE_IN", "STATUS_ERROR_IN", "STATUS_READY_IN", "ACTION_ID_BASIC", "BARCODE_BASIC", "DATA_BASIC", "ERROR_BASIC", "SLIDECOUNT_BASIC", "STATUS_BUSY_BASIC", "STATUS_ENABLE_BASIC", "STATUS_ERROR_BASIC", "STATUS_READY_BASIC" });
const std::vector<std::string> OpcUtils::BASIC_NODE_PATH({"Objects", "2:DeviceSet", "4:CODESYS Control Win V3 x64", "3:Resources", "4:Application", "3:GlobalVars", "4:G", "4:Basic"});
const std::vector<std::string> OpcUtils::IN_NODE_PATH({"Objects", "2:DeviceSet", "4:CODESYS Control Win V3 x64", "3:Resources", "4:Application", "3:GlobalVars", "4:G", "4:In"});

// Formatting functions

std::string OpcUtils::strLst(std::string l, std::string r)
{
  if (l.length() > 0 && !l.empty())
    l += ", ";
  return l += r;
}

std::string OpcUtils::str2byteStr(std::string data)
{
  return str2hexStr(data) + " " + str2bitsStr(data);
}

std::string OpcUtils::str2hexStr(std::string data)
{
  std::stringstream ss;
  ss << std::hex;
  for (size_t i = 0; i < data.length(); ++i)
    ss << "0x" << std::setw(2) << std::setfill('0') << (int)data[i];
  return ss.str();
}

std::string OpcUtils::str2bitsStr(std::string data)
{
  std::stringstream ss;
  ss << std::hex;
  for (size_t i = 0; i < data.length(); ++i)
    ss << "0b" << std::bitset<8>((int)data[i]);
  return ss.str();
}


// Set functions

bool OpcUtils::setNodeValue(OpcUa::Node node, boost::any val, OpcUtils::ReturnValue* retVal)
{
  OpcUa::Variant var = OpcUtils::getNodeValueWithCorrectType(node, val);
  if(retVal != nullptr)
    retVal->setValue(var);
  node.SetValue(var);
  return true;
}


// Get functions

OpcUa::EndpointDescription* OpcUtils::getEndpoint(const char *ip, unsigned short port)
{
  OpcUa::EndpointDescription* endpoint = new OpcUa::EndpointDescription();
  endpoint->EndpointUrl = std::string("opc.tcp://") + ip + std::string(":") + std::to_string(port) + std::string("/");
  /* // if connection requires user and password:
  endpoint->EndpointUrl = std::string("opc.tcp://") + this->user + ":" + this->password + std::string("@") + this->ip + ":" + std::to_string(this->port) + std::string("/");*/
  return endpoint;
}

OpcUa::Node OpcUtils::getNode(OpcUa::UaClient *client, MPSRegister reg)
{
  return client->GetRootNode().GetChild(OpcUtils::getNodeFullPath(reg));
}

OpcUa::Variant OpcUtils::getNodeValueWithCorrectType(OpcUa::Node node, boost::any val)
{
  switch (node.GetValue().Type())
  {
  case OpcUa::VariantType::UINT16:
    return static_cast<uint16_t>(boost::any_cast<uint16_t>(val));
  case OpcUa::VariantType::UINT32:
    return static_cast<uint32_t>(boost::any_cast<uint32_t>(val));
  case OpcUa::VariantType::UINT64:
    return static_cast<uint64_t>(boost::any_cast<uint64_t>(val));
  case OpcUa::VariantType::INT16:
    return static_cast<int16_t>(boost::any_cast<int16_t>(val));
  case OpcUa::VariantType::INT32:
    return static_cast<int32_t>(boost::any_cast<int32_t>(val));
  case OpcUa::VariantType::INT64:
    return static_cast<int64_t>(boost::any_cast<int64_t>(val));
  case OpcUa::VariantType::FLOAT:
    return static_cast<float_t>(boost::any_cast<float_t>(val));
  case OpcUa::VariantType::DOUBLE:
    return static_cast<double_t>(boost::any_cast<double_t>(val));
  case OpcUa::VariantType::BOOLEAN:
    return static_cast<bool>(boost::any_cast<bool>(val));
  case OpcUa::VariantType::BYTE:
    return static_cast<uint8_t>(boost::any_cast<uint8_t>(val));
  default:
    return static_cast<int>(boost::any_cast<int>(val));
  }
}

OpcUa::Node OpcUtils::getBasicNode(OpcUa::UaClient* client)
{
  return client->GetRootNode().GetChild(BASIC_NODE_PATH);
}

OpcUa::Node OpcUtils::getInNode(OpcUa::UaClient* client)
{
  return client->GetRootNode().GetChild(IN_NODE_PATH);
}

const std::vector<std::string> OpcUtils::getNodeRelativePath(MPSRegister reg)
{
  switch (reg)
  {
  case MPSRegister::ACTION_ID_IN:
  case MPSRegister::ACTION_ID_BASIC:
    return {"4:p", "4:ActionId"};
  case MPSRegister::BARCODE_IN:
  case MPSRegister::BARCODE_BASIC:
    return {"4:p", "4:BarCode"};
  case MPSRegister::DATA_IN:
  case MPSRegister::DATA_BASIC:
    return {"4:p", "4:Data"};
  case MPSRegister::ERROR_IN:
  case MPSRegister::ERROR_BASIC:
    return {"4:p", "4:Error"};
  case MPSRegister::SLIDECOUNT_IN:
  case MPSRegister::SLIDECOUNT_BASIC:
    return {"4:p", "4:SlideCnt"};
  case MPSRegister::STATUS_BUSY_IN:
  case MPSRegister::STATUS_BUSY_BASIC:
    return {"4:p", "4:Status", "4:Busy"};
  case MPSRegister::STATUS_ENABLE_IN:
  case MPSRegister::STATUS_ENABLE_BASIC:
    return {"4:p", "4:Status", "4:Enable"};
  case MPSRegister::STATUS_ERROR_IN:
  case MPSRegister::STATUS_ERROR_BASIC:
    return {"4:p", "4:Status", "4:Error"};
  case MPSRegister::STATUS_READY_IN:
  case MPSRegister::STATUS_READY_BASIC:
    return {"4:p", "4:Status", "4:Ready"};
  default:
    return {};
  }
}

const std::vector<std::string> OpcUtils::getNodeFullPath(MPSRegister reg)
{
  std::vector<std::string> ret;
  if (reg >= MPSRegister::ACTION_ID_BASIC)
    ret = std::vector<std::string>(BASIC_NODE_PATH);
  else
    ret = std::vector<std::string>(IN_NODE_PATH);

  for (std::string p : getNodeRelativePath(reg))
    ret.push_back(p);
  return ret;
}

std::string OpcUtils::GetNodeClassName(OpcUa::NodeClass nodeClass)
{
  std::string result;
  if (nodeClass == OpcUa::NodeClass::Object)
    result += "object";
  if (nodeClass == OpcUa::NodeClass::Variable)
    result = strLst(result, "variable");
  if (nodeClass == OpcUa::NodeClass::Method)
    result = strLst(result, "method");
  if (nodeClass == OpcUa::NodeClass::ObjectType)
    result = strLst(result, "object type");
  if (nodeClass == OpcUa::NodeClass::VariableType)
    result = strLst(result, "variable type");
  if (nodeClass == OpcUa::NodeClass::ReferenceType)
    result = strLst(result, "reference type");
  if (nodeClass == OpcUa::NodeClass::DataType)
    result = strLst(result, "data type");
  if (nodeClass == OpcUa::NodeClass::View)
    result = strLst(result, "view");
  if (result.empty())
    result += "NULL";
  return result;
}

std::string OpcUtils::GetNodeAccessLevelName(OpcUa::VariableAccessLevel accessLevel)
{
  std::string result;
  if ((accessLevel | OpcUa::VariableAccessLevel::CurrentRead) == accessLevel)
    result += "CurrentRead";
  if ((accessLevel | OpcUa::VariableAccessLevel::CurrentWrite) == accessLevel)
    result = strLst(result, "CurrentWrite");
  if ((accessLevel | OpcUa::VariableAccessLevel::HistoryRead) == accessLevel)
    result = strLst(result, "HistoryRead");
  if ((accessLevel | OpcUa::VariableAccessLevel::HistoryWrite) == accessLevel)
    result = strLst(result, "HistoryWrite");
  if ((accessLevel | OpcUa::VariableAccessLevel::SemanticChange) == accessLevel)
    result = strLst(result, "SemanticChange");
  if (result.empty())
    result += "NULL";
  return result;
}


// Log functions

bool OpcUtils::logNodeInfo(OpcUa::Node node, std::shared_ptr<spdlog::logger> logger, bool recursive, int depth)
{
  if (recursive)
    return logNodeInfo(node, logger, 0, depth);
  else
    return logNodeInfo(node, logger, -1, -1);
}

bool OpcUtils::logNodeInfo(OpcUa::Node node, std::shared_ptr<spdlog::logger> logger, int indent, int depth)
{
  try
  {
    bool recursive = indent >= 0;

    if (!recursive)
      indent = 0;

    const int lvl[3] = logIndent(indent);
    std::string tmp;

    logger->info("{}>{}", std::string(lvl[0], ' '), node);

    try
    {
      tmp = "Not defined";
      tmp = node.GetBrowseName().Name;
    }
    catch (const std::exception &e)
    {
    }
    logger->info("{}Browse name:  {}", std::string(lvl[1], ' '), tmp);

    try
    {
      tmp = "Not defined";
      tmp = GetNodeClassName(node.GetNodeClass());
    }
    catch (const std::exception &e)
    {
    }
    logger->info("{}Class type:   {}", std::string(lvl[1], ' '), tmp);

    try
    {
      tmp = "Not defined";
      tmp = OpcUa::ToString(node.GetId());
    }
    catch (const std::exception &e)
    {
    }
    logger->info("{}ID:           {}", std::string(lvl[1], ' '), tmp);

    try
    {
      tmp = "Not defined";
      tmp = OpcUtils::GetNodeAccessLevelName(node.GetAccessLevel());
    }
    catch (const std::exception &e)
    {
      logger->warn(e.what());
    }
    logger->info("{}Access level: {} ({})", std::string(lvl[1], ' '), tmp, OpcUa::ToString(node.GetAttribute(OpcUa::AttributeId::UserAccessLevel).Value.Type()));

    logNodeValue(node, node.GetBrowseName().Name, logger, indent);

    /*tmp = "Not defined";
    try
    {
      tmp = "'" + node.GetDescription().Text + "' (" + node.GetDescription().Locale + ")";
    }
    catch (const std::exception &e)
    {
      logger->trace("Warning: {} (@{}:{})", e.what(), __FILE__, __LINE__);
    }
    logger->info("{}Description:  {}", std::string(lvl[1], ' '), tmp);*/

    if (recursive && depth != 0)
    {
      logger->info("");
      indent++;
      if (depth > 0)
        depth--;
      if (depth != 0)
        for (OpcUa::Node child : node.GetChildren())
          if (!logNodeInfo(child, logger, indent, depth))
            return false;
    }
  }
  catch (const std::exception &exc)
  {
    logger->error("Error: {} (@{}:{})", exc.what(), __FILE__, __LINE__);
    return false;
  }
  catch (...)
  {
    logger->error("Unknown error.");
    return false;
  }
  return true;
}

bool OpcUtils::logNodeValue(OpcUa::Node node, std::string nodeName, std::shared_ptr<spdlog::logger> logger, int indent)
{
  const int lvl[3] = logIndent(indent);
  std::string tmp;
  OpcUa::VariantType type = OpcUa::VariantType::NUL;
  try
  {
    OpcUa::Variant val = node.GetValue();
    type = val.Type();

    if (!val.IsNul() && type != OpcUa::VariantType::NUL)
    {
      if (val.IsArray())
        logNodeValueArray(node, nodeName, logger, indent);
      else
      {
        switch (val.Type())
        {
        case OpcUa::VariantType::LOCALIZED_TEXT:
          tmp = val.As<OpcUa::LocalizedText>().Text;
          break;
        case OpcUa::VariantType::BYTE:
          tmp = str2byteStr(val.ToString());
          break;
        default:
          try
          {
            tmp = val.ToString();
          }
          catch (const std::exception &e)
          {
            tmp = "Conversion error!";
          }
          break;
        }
        logger->info("{}{}:       {} ({}{})", std::string(lvl[1], ' '), nodeName, tmp, OpcUa::ToString(val.Type()), val.IsArray() ? "[]" : "");
      }
    }
  }
  catch (const std::exception &e)
  {
    try
    {
      std::string msg = e.what();
      msg = msg.substr(msg.find("'") + 1, msg.length());
      msg = msg.substr(0, msg.find("'"));
      logger->warn("{}Value:        ? ({})", std::string(lvl[1], ' '), msg);
    }
    catch (const std::exception &e1)
    {
      logger->warn("Warning: {} (@{}:{})", e.what(), __FILE__, __LINE__);
      return false;
    }
  }
  return true;
}

void OpcUtils::logNodeValue(OpcUa::Variant val, const char *msg, std::string nodeName, std::shared_ptr<spdlog::logger> logger, int indent)
{
  const int lvl[3] = logIndent(indent);
  std::string tmp;
  switch (val.Type())
  {
  case OpcUa::VariantType::LOCALIZED_TEXT:
    tmp = val.As<OpcUa::LocalizedText>().Text;
    break;
  case OpcUa::VariantType::BYTE:
    tmp = str2byteStr(val.ToString());
    break;
  default:
    try
    {
      tmp = val.ToString();
    }
    catch (const std::exception &e)
    {
      tmp = "Conversion error!";
    }
    break;
  }
  if (indent >= 0)
    logger->info(("{}" + std::string(msg)).c_str(), std::string(lvl[1], ' '), nodeName, tmp, OpcUa::ToString(val.Type()), val.IsArray() ? "[]" : "");

  else
    logger->info(msg, nodeName, tmp, OpcUa::ToString(val.Type()), val.IsArray() ? "[]" : "");
}

std::string OpcUtils::logNodeValueArray(OpcUa::Node node, std::string nodeName, std::shared_ptr<spdlog::logger> logger, int indent)
{
  return logNodeValueArray(node.GetValue(), "{}:       {} ({}{})", nodeName, logger, indent);
}

std::string OpcUtils::logNodeValueArray(OpcUa::Variant val, const char *msg, std::string nodeName, std::shared_ptr<spdlog::logger> logger, int indent)
{
  const int lvl[3] = logIndent(indent);
  std::string array;
  switch (val.Type())
  {
  case OpcUa::VariantType::STRING:
  case OpcUa::VariantType::QUALIFIED_NAME:
    for (auto d : val.As<std::vector<std::string>>())
      array = strLst(array, d);
    break;
  case OpcUa::VariantType::LOCALIZED_TEXT:
    for (auto d : val.As<std::vector<OpcUa::LocalizedText>>())
      array = strLst(array, d.Text);
    break;
  case OpcUa::VariantType::BYTE:
    for (auto d : val.As<std::vector<std::string>>())
      array = strLst(array, str2byteStr(d));
    break;
  case OpcUa::VariantType::UINT16:
    for (auto d : val.As<std::vector<uint16_t>>())
      array = strLst(array, std::to_string(d));
    break;
  case OpcUa::VariantType::UINT32:
    for (auto d : val.As<std::vector<uint32_t>>())
      array = strLst(array, std::to_string(d));
    break;
  case OpcUa::VariantType::UINT64:
    for (auto d : val.As<std::vector<uint64_t>>())
      array = strLst(array, std::to_string(d));
    break;
  case OpcUa::VariantType::INT16:
    for (auto d : val.As<std::vector<int16_t>>())
      array = strLst(array, std::to_string(d));
    break;
  case OpcUa::VariantType::INT32:
    for (auto d : val.As<std::vector<int32_t>>())
      array = strLst(array, std::to_string(d));
    break;
  case OpcUa::VariantType::INT64:
    for (auto d : val.As<std::vector<int64_t>>())
      array = strLst(array, std::to_string(d));
    break;
  case OpcUa::VariantType::DOUBLE:
    for (auto d : val.As<std::vector<double_t>>())
      array = strLst(array, boost::any_cast<std::string>(d));
    break;
  default:
    try
    {
      for (auto d : val.As<std::vector<std::string>>())
        array = strLst(array, d);
    }
    catch (const std::exception &e)
    {
      logger->info("Array of type {}", OpcUa::ToString(val.Type()));
      logger->error("Error: {} (@{}:{})", e.what(), __FILE__, __LINE__);
    }
    break;
  }
  if (indent >= 0)
    logger->info(("{}" + std::string(msg)).c_str(), std::string(lvl[1], ' '), nodeName, array, OpcUa::ToString(val.Type()), val.IsArray() ? "[]" : "");
  else
    logger->info(msg, nodeName, array, OpcUa::ToString(val.Type()), val.IsArray() ? "[]" : "");
  return array;
}

bool OpcUtils::logReturnValue(ReturnValue* val, std::shared_ptr<spdlog::logger> logger, OpcUtils::MPSRegister reg, std::string msg)
{
  if(val == nullptr || val->isEmpty())
      return false;
  int lvl[] = logIndent(1);
  std::string retVal;
  if(val->isVector())
    for(std::string s : val->ToStringVector())
      retVal = OpcUtils::strLst(retVal, s);
  else
    retVal = val->ToString();
  logger->info("{}{}{}: {} ({}{})", msg, msg.empty() ? "" : std::string(" ", lvl[0]), OpcUtils::REGISTER_NAMES[reg], retVal, OpcUa::ToString(val->getType()), val->isVector() ? "[]" : "");
  return true;
}


// example usage to browse server

bool OpcUtils::browseServer(OpcUa::UaClient *client, std::shared_ptr<spdlog::logger> logger)
{
  try
  {
    /* get and browse Objects node */
    logger->info("Child of objects node are:");
    OpcUa::Node objects = client->GetObjectsNode();
    logNodeInfo(objects, logger, true, 2);

    /* URI data */
    logger->info("Application URI: {}", client->GetApplicationURI());
    logger->info("Product URI: {}", client->GetProductURI());

    /* get a node from standard namespace using objectId */
    logger->info("NamespaceArray is:");
    OpcUa::Node nsnode = client->GetNode(OpcUa::ObjectId::Server_NamespaceArray);
    OpcUa::Variant ns = nsnode.GetValue();
    for (std::string d : ns.As<std::vector<std::string>>())
    {
      logger->info("  {}", d);
    }
    logger->info("\n");

    /* get root node and all other nodes */
    logger->info("/*------ Begin all nodes: ------*/");
    logNodeInfo(client->GetRootNode(), logger, true);
    logger->info("/*------ End  all  nodes: ------*/");
  }
  catch (const std::exception &exc)
  {
    logger->error("Error: {} (@{}:{})", exc.what(), __FILE__, __LINE__);
    return false;
  }
  catch (...)
  {
    logger->error("Unknown error.");
    return false;
  }
  return true;
}

/*OpcUa::Node Machine::findChildNodeByBrowseName(OpcUa::Node node, std::string browseName)
{
  for(OpcUa::Node child : node.GetChildren())
  {
    if(child.GetBrowseName().Name == browseName)
      return child;
    else
    {
      OpcUa::Node tmp = findChildNodeByBrowseName(child, browseName);
      if(tmp != child)
        return tmp;
    }
  }
  return node;
}*/
} // namespace modbus
} // namespace llsfrb