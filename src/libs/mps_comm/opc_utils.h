#pragma once
#include <opc/ua/subscription.h>
#include <opc/ua/client/client.h>
#include <opc/ua/node.h>

namespace llsfrb {
#if 0
}
#endif
namespace modbus {
#if 0
}
#endif

// when printing recursively, indent is used to make the hirarchy more visible
#define logIndent(i)                      \
{                                 \
  i * 2, (i + 1) * 2, (i + 2) * 2 \
}

class OpcUtils {
  public:
  // Return value container for a subscription to a OPC UA Node
  struct ReturnValue
  {
    private:
    bool isEmpty_ = true;
    bool isVector_ = false;
    OpcUa::VariantType type_;
    
    public:
    uint16_t uint16_s;
    uint32_t uint32_s;
    uint64_t uint64_s;
    int16_t int16_s;
    int32_t int32_s;
    int64_t int64_s;
    float_t float_s;
    double_t double_s;
    bool bool_s;
    uint8_t uint8_s;
    std::string string_s;

    std::vector<uint16_t> uint16_v;
    std::vector<uint32_t> uint32_v;
    std::vector<uint64_t> uint64_v;
    std::vector<int16_t> int16_v;
    std::vector<int32_t> int32_v;
    std::vector<int64_t> int64_v;
    std::vector<float_t> float_v;
    std::vector<double_t> double_v;
    std::vector<bool> bool_v;
    std::vector<uint8_t> uint8_v;
    std::vector<std::string> string_v;

    // Checks if the current return value has already been set once
    bool isEmpty();
    // Checks if the current return value is a vector
    bool isVector();
    // Checks if the current return value is a scalar
    bool isScalar();
    // Get the current return value type as OPC UA Variant Type
    OpcUa::VariantType getType();
    // Set current return value type 
    void setType(OpcUa::VariantType type);
    // Converts set value to string
    std::string ToString();
    // Converts set vector to a string vector
    std::vector<std::string> ToStringVector();
    // Converts set value to size_t
    size_t ToInt();
    // Converts set value to size_t vector
    std::vector<size_t> ToIntVector();
    // Set array value
    void setArray(const OpcUa::Variant &val);
    // Set scalar value
    void setValue(const OpcUa::Variant &val);
  };

  // Registers existing in the MPS, to which it is possible to subscribe to
  enum MPSRegister{
    ACTION_ID_IN = 0,
    BARCODE_IN,
    DATA_IN,
    ERROR_IN,
    SLIDECOUNT_IN,
    STATUS_BUSY_IN,
    STATUS_ENABLE_IN,
    STATUS_ERROR_IN,
    STATUS_READY_IN,
    ACTION_ID_BASIC,
    BARCODE_BASIC,
    DATA_BASIC,
    ERROR_BASIC,
    SLIDECOUNT_BASIC,
    STATUS_BUSY_BASIC,
    STATUS_ENABLE_BASIC,
    STATUS_ERROR_BASIC,
    STATUS_READY_BASIC
  };
  // MPS Register addition operator overload
  friend MPSRegister operator+(const MPSRegister a, const MPSRegister b);
  
  // static vector containing all the names corresponding to the MPSRegisters; usage example: REGISTER_NAMES[MPSRegister::ACTION_ID_IN]
  static const std::vector<std::string> REGISTER_NAMES;
  // static vector containing the path from root node to the "basic" node in the MPS
  static const std::vector<std::string> BASIC_NODE_PATH;
  // static vector containing the path from root node to the "in" node in the MPS
  static const std::vector<std::string> IN_NODE_PATH;
  // static vector containing the path from root node to the "basic" node in the 64 bit MPS
  static const std::vector<std::string> BASIC_NODE_PATH_64;
  // static vector containing the path from root node to the "in" node in the 64 bit MPS
  static const std::vector<std::string> IN_NODE_PATH_64;
  
  // Concatenates the two parameters by separating them with ", "
  static std::string strLst(std::string l, std::string r);
  // Concatenated the outputs of str2hexStr and str2bitsStr with a space
  static std::string str2byteStr(std::string data);
  // Converts a original value in string format to a readable hex number string
  static std::string str2hexStr(std::string data);
  // Converts a original value in string format to a readable bit number string
  static std::string str2bitsStr(std::string data);

  // Set OPC UA node value; if retVal is set, the SubscriptionClient internal return value is overridden
  static bool setNodeValue(OpcUa::Node node, boost::any val, OpcUtils::ReturnValue* retVal = nullptr);

  // Get OPC UA Endpoint given by IP and port
  static OpcUa::EndpointDescription* getEndpoint(const char* ip, unsigned short port);
  // Get OPC UA node using MPSRegister
  static OpcUa::Node getNode(OpcUa::UaClient* client, MPSRegister reg);
  // Get OPC UA Node value as OPC UA Variant with the needed type
  static OpcUa::Variant getNodeValueWithCorrectType(OpcUa::Node node, boost::any val);
  // Get "basic" OPC UA node
  static OpcUa::Node getBasicNode(OpcUa::UaClient* client);
  // Get "in" OPC UA node
  static OpcUa::Node getInNode(OpcUa::UaClient* client);
  // Get OPC UA node relative path (path after in or basic node) by MPSRegister
  static const std::vector<std::string> getNodeRelativePath(MPSRegister reg);
  // Get OPC UA node full path by MPSRegister
  static const std::vector<std::string> getNodeFullPath(MPSRegister reg);
  // Get OPC UA node full path by MPSRegister for a 64 bit MPS
  static const std::vector<std::string> getNodeFullPath64(MPSRegister reg);
  // Get node class name as string
  static std::string GetNodeClassName(OpcUa::NodeClass nodeClass);
  // Get node access level as string
  static std::string GetNodeAccessLevelName(OpcUa::VariableAccessLevel accessLevel);

  // Log node info (recursively if set)
  static bool logNodeInfo(OpcUa::Node node, std::shared_ptr<spdlog::logger> logger, bool recursive = false, int depth = -1);
  // Log node info with indent
  static bool logNodeInfo(OpcUa::Node node, std::shared_ptr<spdlog::logger> logger, int indent, int depth);
  // Log node value ny OPC UA Node
  static bool logNodeValue(OpcUa::Node node, std::string nodeName, std::shared_ptr<spdlog::logger> logger, int indent);
  // Log node value by OPC UA Variant
  static void logNodeValue(OpcUa::Variant val, const char* msg, std::string nodeName, std::shared_ptr<spdlog::logger> logger, int indent = -1);
  // Log node array value
  static std::string logNodeValueArray(OpcUa::Node node, std::string nodeName, std::shared_ptr<spdlog::logger> logger, int indent);
  // Log node array value
  static std::string logNodeValueArray(OpcUa::Variant val, const char* msg, std::string nodeName, std::shared_ptr<spdlog::logger> logger, int indent = -1);
  // Log return value
  static bool logReturnValue(ReturnValue* val, std::shared_ptr<spdlog::logger> logger, MPSRegister reg, std::string msg = "");
  
  // browses the server and loggs every node; mostly an example usage of the helper functions
  static bool browseServer(OpcUa::UaClient* client, std::shared_ptr<spdlog::logger> logger);
};
}
}