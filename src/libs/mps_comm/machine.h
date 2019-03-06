// Abstract base class for the stations
#pragma once
#include <vector>
#include <string>
//#include <modbus.h>
// modbus defines ON and OFF, which causes conflict with protobuf
//#undef OFF
//#undef ON
#include <mutex>

//#include <msgs/MachineInstructions.pb.h>
/* REMOVE */
#include "msgs/MachineInstructions.pb.h"
#include "subscription_client.h"
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

class Machine
{
public:
  Machine(unsigned short int machine_type);

  virtual ~Machine();

  // This method will send a command over the modbus connection
  // A Command consist of
  //   *) command word
  //   *) payload1
  //   *) payload2
  //   *) status flag (shall be 1 = BUISY)
  void send_command(unsigned short command, unsigned short payload1 = 0,
                    unsigned short payload2 = 0, int timeout = 0, unsigned char status = 1, unsigned char error = 0);

  // Create a modbus connection to machine
  bool connect_PLC(const std::string &ip, unsigned short port);

  // Set the light of specified color to specified state
  // color: 1 - 3, state 0 - 2
  void set_light(llsf_msgs::LightColor color, llsf_msgs::LightState state = llsf_msgs::ON,
                 unsigned short time = 0);

  void conveyor_move(llsf_msgs::ConveyorDirection direction, llsf_msgs::SensorOnMPS sensor);

  // Sends reset light command
  void reset_light();

  // abstract methods

  // Reset: send the reset command (which is different for each machine type)
  void reset();
  // Identify: The PLC does not know, which machine it runs. This command tells it the type.
  virtual void identify() = 0;

  // when abort_operation_ is set, the current command is aborted.
  // lock_ protects just this one boolean
  std::mutex lock_;
  bool abort_operation_;

  // Needed to check, if a product was retrieved.
  bool wait_for_free();

protected:
  // machine type
  const unsigned short int machine_type_;
  // wait, until the ready bit is set.
  // That is, when the command is handled completely
  // timeout in ms, 0 -> no timeout
  bool wait_for_ready(int timeout = 0);
  // wait, until the buisy flag is set.
  // This is nescessary, because it has to be unset, when it gets set.
  void wait_for_buisy();


  // OPC UA related variables

  /* OVERRIDE */
  // OVERRIDE: static vector containing the MPSRegisters to automatically subscribe; This should be overwritten if needed
  static const std::vector<OpcUtils::MPSRegister> SUB_REGISTERS;
  // OVERRIDE: static string containing the log file path; If this variable is not overwritten, logging is redirected to std::cout
  static const std::string LOG_PATH;

  // OPC UA logger
  std::shared_ptr<spdlog::logger> logger;
  // OPC UA Client pointer
  OpcUa::UaClient *client;                
  // OPC UA Nodes for each subscribable MPSRegister
  OpcUa::Node registerNodes[OpcUtils::MPSRegister::STATUS_READY_BASIC]; 
  // OPC UA Input Register for station Jobs
  OpcUa::Node nodeIn;    
  // OPC UA Input Register for Basic Jobs
  OpcUa::Node nodeBasic; 
  // All subscriptions to MPSRegisters in form map<MPSRegister, Subscription>
  SubscriptionClient::map subscriptions; 
  // Return values from MPS after subscribtion; If passed as argument to function subscribe, the SubscriptionClient internal ReturnValue is overridden
  OpcUtils::ReturnValue *outs = new OpcUtils::ReturnValue[OpcUtils::MPSRegister::STATUS_READY_BASIC]();


  // OPC UA related methods

  // Connect to OPC UA Server using IP and PORT
  bool reconnect(const char *ip, unsigned short port);
  // Disconnect from OPC UA Server
  bool disconnect();
  // Initialize logger; If LOG_PATH is empty, the logs are redirected to std::cout, else they are saved to the in LOG_PATH specified file
  void initLogger();
  // Helper function to set OPC UA Node value correctly
  bool setNodeValue(OpcUa::Node node, boost::any val, OpcUtils::MPSRegister reg);
  // Helper function to get ReturnValue correctly
  OpcUtils::ReturnValue *getReturnValue(OpcUtils::MPSRegister reg);

  // Subscribe to a specified MPSRegister; If ReturnValue is set, the SubscriptionClient internal ReturnValue is overridden
  SubscriptionClient *subscribe(OpcUtils::MPSRegister reg, OpcUtils::ReturnValue *retVal = nullptr);
  // Subscribe to multiple specified MPSRegisters; If ReturnValues are set, the SubscriptionClients internal ReturnValues are overridden
  void subscribe(std::vector<OpcUtils::MPSRegister> registers, OpcUtils::ReturnValue *retVals = nullptr);
  // Subscribe to all existing MPSRegisters
  void subscribeAll();
  // Cancel a subscription given a specific MPSRegister; If default argument is true, final subscription value will be printed before deleting
  SubscriptionClient::map::iterator cancelSubscription(OpcUtils::MPSRegister reg, bool log = false);
  // Cancel all existing subscriptions; If default argument is true, final subscription values will be printed before deleting
  void cancelAllSubscriptions(bool log = false);
  // Print the final subscription values
  void printFinalSubscribtions();
};

} // namespace modbus
} // namespace llsfrb
