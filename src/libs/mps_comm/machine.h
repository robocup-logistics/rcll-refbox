// Abstract base class for the stations
#pragma once
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <tuple>
#include <vector>

//#include <msgs/MachineInstructions.pb.h>
/* REMOVE */
#include "msgs/MachineInstructions.pb.h"
#include "opc_utils.h"
#include "subscription_client.h"

namespace llsfrb {
#if 0
}
#endif
namespace mps_comm {
#if 0
}
#endif

enum ConveyorDirection { FORWARD = 1, BACKWARD = 2 };

enum MPSSensor { INPUT = 1, MIDDLE = 2, OUTPUT = 3 };

class Machine
{
	typedef std::
	  tuple<SubscriptionClient::ReturnValueCallback, OpcUtils::MPSRegister, OpcUtils::ReturnValue *>
	    Callback;

public:
	enum ConnectionMode {
		MOCKUP,
		SIMULATION,
		PLC,
	};

	Machine(std::string        name,
	        unsigned short int machine_type,
	        std::string        ip,
	        unsigned short     port,
	        ConnectionMode = PLC);

	virtual ~Machine();

	// This method will send a command to the machine
	// A Command consist of
	//   *) command word
	//   *) payload1
	//   *) payload2
	//   *) status flag (shall be 1 = BUSY)
	void send_command(unsigned short command,
	                  unsigned short payload1 = 0,
	                  unsigned short payload2 = 0,
	                  int            timeout  = 0,
	                  unsigned char  status   = 1,
	                  unsigned char  error    = 0);

	// Create a OPC connection to a machine
	bool connect_PLC();

	// Set the light of specified color to specified state
	// color: 1 - 3, state 0 - 2
	void set_light(llsf_msgs::LightColor color,
	               llsf_msgs::LightState state = llsf_msgs::ON,
	               unsigned short        time  = 0);

	void conveyor_move(ConveyorDirection direction, MPSSensor sensor);

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
	bool       abort_operation_;

protected:
	const std::string name_;
	// machine type
	const unsigned short int machine_type_;
	std::string              ip_;
	unsigned short           port_;

	const ConnectionMode                  connection_mode_;
	static constexpr std::chrono::seconds mock_busy_duration_{3};
	static constexpr std::chrono::seconds mock_ready_duration_{5};

	std::atomic<bool>                     shutdown_;
	std::mutex                            command_queue_mutex_;
	std::mutex                            command_mutex_;
	std::condition_variable               queue_condition_;
	std::queue<std::function<void(void)>> command_queue_;
	std::thread                           worker_thread_;
	std::thread                           heartbeat_thread_;
	std::atomic<bool>                     heartbeat_active_;
	void                                  register_callback(Callback, bool simulation = false);
	void mock_callback(OpcUtils::MPSRegister reg, OpcUtils::ReturnValue *ret);
	void mock_callback(OpcUtils::MPSRegister reg, bool ret);
	void dispatch_command_queue();
	void heartbeat();

	std::vector<Callback> callbacks_;

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
	OpcUtils::ReturnValue *outs =
	  new OpcUtils::ReturnValue[OpcUtils::MPSRegister::STATUS_READY_BASIC]();

	// OPC UA related methods

	// Connect to OPC UA Server using IP and PORT
	bool reconnect(const char *ip, unsigned short port, bool simulation = false);
	// Disconnect from OPC UA Server
	bool disconnect();
	// Initialize logger; If LOG_PATH is empty, the logs are redirected to std::cout, else they are saved to the in LOG_PATH specified file
	void initLogger();
	// Helper function to set OPC UA Node value correctly
	bool setNodeValue(OpcUa::Node node, boost::any val, OpcUtils::MPSRegister reg);
	// Helper function to get ReturnValue correctly
	OpcUtils::ReturnValue *getReturnValue(OpcUtils::MPSRegister reg);

	// Subscribe to a specified MPSRegister; If ReturnValue is set, the SubscriptionClient internal ReturnValue is overridden
	SubscriptionClient *subscribe(OpcUtils::MPSRegister  reg,
	                              OpcUtils::ReturnValue *retVal     = nullptr,
	                              bool                   simulation = false);
	// Subscribe to multiple specified MPSRegisters; If ReturnValues are set, the SubscriptionClients internal ReturnValues are overridden
	void subscribe(std::vector<OpcUtils::MPSRegister> registers,
	               OpcUtils::ReturnValue *            retVals    = nullptr,
	               bool                               simulation = false);
	// Subscribe to all existing MPSRegisters
	void subscribeAll(bool simulation = false);
	// Cancel a subscription given a specific MPSRegister; If default argument is true, final subscription value will be printed before deleting
	SubscriptionClient::map::iterator cancelSubscription(OpcUtils::MPSRegister reg, bool log = false);
	// Cancel all existing subscriptions; If default argument is true, final subscription values will be printed before deleting
	void cancelAllSubscriptions(bool log = false);
	// Print the final subscription values
	void printFinalSubscribtions();

public:
	void addCallback(SubscriptionClient::ReturnValueCallback callback,
	                 OpcUtils::MPSRegister,
	                 OpcUtils::ReturnValue *retVal     = nullptr,
	                 bool                   simulation = false);
};

} // namespace mps_comm
} // namespace llsfrb
