// This file contains various defines.
// They are related to the communication to the PLC.
#pragma once

// Base commands for modbus communication:
// Each machine type has a base prefix.
// The sub commands are added to these base prefixes.
#define BASE_STATION_CMD 100
#define RING_STATION_CMD 200
#define CAP_STATION_CMD 300
#define DELIVERY_STATION_CMD 400
#define STORAGE_STATION_CMD 500

// all stations, no combining with Station code.
// These commands will be sent, as they are.
#define NO_CMD 0
#define SET_TYPE_CMD 10
#define LIGHT_RESET_CMD 20
#define LIGHT_RED_CMD 21
#define LIGHT_YELLOW_CMD 22
#define LIGHT_GREEN_CMD 23
// Payload for set_type_cmd
#define TYPE_BS 1
#define TYPE_RS 2
#define TYPE_CS 3
#define TYPE_DS 4
#define TYPE_SS 5
// payload for light command
#define LIGHT_OFF 0
#define LIGHT_ON 1
#define LIGHT_BLINK 2
// For all stations, combine with staition code
// (eg BASE_STATION_CMD + RESET_CMD)
#define RESET_CMD 0

// base, ring and cup station
#define MOVE_BAND_CMD 2
#define DIR_OUT 3
#define DIR_MID 2
#define DIR_IN 1

// base station
#define GET_BASE_CMD 1

// ring station
#define WAIT_FOR_BASES_CMD 1
#define MOUNT_RING_CMD 3

// cap station
#define CAP_ACTION_CMD 1
#define CAP_RETRIEVE 1
#define CAP_MOUNT 2

// deliver station
#define DELIVER_CMD 1

// storage station station
#define GET_F_PRODUCT_CMD 1

// The status flags.
// To be combined with bitwise | and read with bitwise &
#define STATUS_ERR 4
#define STATUS_READY 2
#define STATUS_BUISY 1

// TBD: timeout for the band to reach the end position
// in ms
// 0 is no timeout
// NOTE: The timeout shall be handeled on refbox side
#define TIMEOUT_BAND 0
// TBD: time out for the msp to unset the buisy flag in ms
// Normally this shall be within one PLC cycle.
// (a couple of milli seconds).
// However, due to protocol overhead, I use 1 sec as limit.
#define TIMEOUT_BUISY 1000


/*
// This file contains a couple of define statments for the modbus communication.
// Also there are two very simple inline functions for translating the bit number
// to the correct register and bit number within the 2 byte word.

// There are all in all 12 registers, but currently only
// register 0, 1 and 10 are used. (As far as I interpreted things)
// However, there is no need to send register 10 all the time.
// Therefore, only two registers are needed
#define MACHINE_NR_REGISTERS_OUT 2
// For inputs currently only register 0 is used.
#define MACHINE_NR_REGISTERS_IN 1

// Outputs
// Taken from ModbusIODefinition.xlsx
// from Rohr Alain (March 2017)
// Some pins are for all machines, some only for specific machines.
// That is, why there are mutliple names for the same pin.
// From what I could observe, The numbers are bit numbers.
// Therefore bit 17 means, the 18th bit in the first word.
// Words are 18 bits -> 20 is the 5th bit in the second word.


// Start/Stop the motor to move the object to deliver it
#define BIT_MOTOR_TO_OUT      0
// Not 100% clear
// Probably Start/Stop the motor to move the object in the opposite direction.
#define BIT_MOTOR_TO_IN       1
// Not 100% clear
// For all but base station
#define BIT_SEPARATOR1        2
// Not 100% clear
// Only for delivery station
#define BIT_SEPARATOR2        3
// Not 100% clear
// only for delivery station.
// In the excel sheet, it is in brackets (whatever that means)
#define BIT_LOCK              4
// Address for the red signal light on top of the machine
// The machine will be identified by the light combination.
#define BIT_LIGHT_RED         5
// Same for yellow and green
#define BIT_LIGHT_YELLOW      6
#define BIT_LIGHT_GREEN       7
// For base station:
// Is there an object at output[i]?
#define BIT_OUTPUT1           8
#define BIT_OUTPUT2           9
#define BIT_OUTPUT3          10
// Not sure, what these outputs are used for
#define BIT_SLIDE1_RETRACT    8
#define BIT_SLIDE1_EXTRACT    9
#define BIT_SLIDE1_DOWN      10
// Not 100% sure, what these outputs are used for. They are for ring and
// cup station. So I guess, they are used to grab some items with vacuum
// (or so).
#define BIT_VACUUM1_ON       11
#define BIT_VACUUM1_OFF      12
// Not sure, what these outputs are good for
#define BIT_SLIDE2_RETRACT   16
#define BIT_SLIDE2_EXTRACT   17
#define BIT_SLIDE2_DOWN      18
// A second vacuum for the ring station only
#define BIT_VACUUM2_ON       19
#define BIT_VACUUM2_OFF      20

// Inputs
// Taken from ModbusIODefinition.xlsx
// from Rohr Alain (March 2017)
// I assumed, that the input board starts at address 0. This would conclude,
// that reading and writing from an address trigger different memory regions.
// TODO: verify, if this assumtion is correct. In the SPS modbus block,
//       there is the checkbox "Holding- and Input-Register Data Areas overlay"
//       (whatever this means);
// Some pins are for all machines, some only for specific machines.
// That is, why there are mutliple names for the same pin.

// Is a part in the input side?
#define BIT_PART_AT_IN1       0
// Only for delivery station and in brackets
// not 100% sure
#define BIT_PART_AT_IN2       1
// Base station only and in brackets
// Probably not needed
#define BIT_PART_AT_MID       1
// Ring and cap station only
// The object is at the separator
#define BIT_PART_AT_SEP       1
// The part is at the output -> ready to pick up
#define BIT_PART_AT_OUT       2
// Part is at slide (ring station)
// Not 100% sure, what that means
#define BIT_PART_AT_SLIDE     3
// Only for cap station:
// Not 100% sure
#define BIT_SLIDE_IS_UP       3
// Only for delivery station and in brackets
#define BIT_PART_AT_IN_IND    3
#define BIT_B1_RETRACTED      4
#define BIT_B1_EXTRACTED      5
#define BIT_SLIDE1_RETRACTED  4
#define BIT_SLIDE1_EXTRACTED  5
#define BIT_SEP1_RETRACTED    4
#define BIT_SEP1_EXTRACTED    5
#define BIT_B2_RETRACTED      6
#define BIT_B2_EXTRACTED      7
#define BIT_SLIDE2_RETRACTED  6
#define BIT_SLIDE2_EXTRACTED  7
#define BIT_SEP2_RETRACTED    6
#define BIT_SEP2_EXTRACTED    7
#define BIT_B3_RETRACTED      8
#define BIT_B3_EXTRACTED      9
#define BIT_BASE1_AV         10
#define BIT_VACUUM1_IS_ON    10
#define BIT_BASE2_AV         11
#define BIT_VACUUM2_IS_ON    11
#define BIT_BASE3_AV         12

// Type of the machine is currently set in the 10nth word.
#define REG_MACHINE_TYPE 10

// the value for the type register:
// Base station
#define MACHINE_ID_BS 1
// Ring station
#define MACHINE_ID_RS 2
// Cap station
#define MACHINE_ID_CS 3
// Deliver station
#define MACHINE_ID_DS 4
// Dont know, not implemented yet (at PLC side)
#define MACHINE_ID_SS 5

// Get the register number of a given pin
inline unsigned int modbusPin2Register( unsigned int pin) {
  return pin >> 4;
}

// set/unset a pin in the modbus registers.
// @param reg    An array of unsigned short register values.
// @param pin    The overall bit number. eg 16 would be the first bit in the second register
// @param value  true to set the bit, false to unset it.
inline void modbusSetPin( unsigned short* reg, unsigned int pin, bool value = true) {
  if(value)
    reg[(pin<<4)]  |= 1 << (pin & 15);
  else
    reg[(pin<<4)]  &= ~ (1 << (pin & 15));
}

// Get the value under the bit number pin.
inline bool modbusGetPin( const unsigned short* reg, unsigned int pin) {
  return (bool) reg[pin << 4] | (1 << (pin & 15));
}
*/
