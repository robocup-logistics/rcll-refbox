// Abstract base class for the stations
#pragma once
#include <vector>
#include <string>
#include <modbus.h>
// modbus defines ON and OFF, which causes conflict with protobuf
#undef OFF
#undef ON
#include <mutex>
#include <msgs/MachineInstructions.pb.h>

class Machine {
  public:
    Machine();

    virtual ~Machine();

    // This method will send a command over the modbus connection
    // A Command consist of
    //   *) command word
    //   *) payload1
    //   *) payload2
    //   *) status flag (shall be 1 = BUISY)
    void sendCommand( unsigned short command, unsigned short payload1 = 0,
                      unsigned short payload2 = 0, int timeout = 0, unsigned char status = 1);

    // Create a modbus connection to machine
    void connectPLC(const std::string& ip, unsigned short port);

    // Set the light of specified color to specified state
    // color: 1 - 3, state 0 - 2
    void setLight(unsigned short color, llsf_msgs::LightState state = llsf_msgs::ON,
        unsigned short time = 0);
    // Sends reset light command
    void resetLight();

    // abstract methods
    // Reset: send the reset command (which is different for each machine type)
    // Identify: The PLC does not know, which machine it runs. This command tells it the type.
    virtual void reset() = 0;
    virtual void identify() = 0;

    // when abort_operation_ is set, the current command is aborted.
    // lock_ protects just this one boolean
    std::mutex lock_;
    bool abort_operation_;

    // Needed to check, if a product was retrieved.
    bool waitForFree();

  protected:
    // wait, until the ready bit is set.
    // That is, when the command is handled completely
    // timeout in ms, 0 -> no timeout
    bool waitForReady(int timeout = 0);
    // wait, until the buisy flag is set.
    // This is nescessary, because it has to be unset, when it gets set.
    void waitForBuisy();
    // Update register values from PLC via modbus
    void updateRegisters();
    // push registers to PLC via modbus
    void pushRegisters();
    // Input registers
    std::vector<unsigned short> in_registers_;
    // Output registers
    std::vector<unsigned short> out_registers_;
    // modbus struct
    modbus_t* connection_;
};
    


