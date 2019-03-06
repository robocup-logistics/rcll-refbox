#include "machine.h"
#include "mps_io_mapping.h"
#include "time_utils.h"
#include "exceptions.h"
#include <stdexcept>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sstream>

namespace llsfrb {
#if 0
}
#endif
namespace modbus {
#if 0
}
#endif

const std::vector<OpcUtils::MPSRegister> Machine::SUB_REGISTERS({ OpcUtils::MPSRegister::BARCODE_IN, OpcUtils::MPSRegister::ERROR_IN, OpcUtils::MPSRegister::STATUS_BUSY_IN, OpcUtils::MPSRegister::STATUS_ENABLE_IN, OpcUtils::MPSRegister::STATUS_ERROR_IN, OpcUtils::MPSRegister::STATUS_READY_IN });
// CHANGE
const std::string Machine::LOG_PATH = ""; /*"./logs/log.txt"; */

Machine::Machine(unsigned short int machine_type) : abort_operation_(false), machine_type_(machine_type) {//, in_registers_(4), out_registers_(4) {
  initLogger();
}


void Machine::send_command(unsigned short command, unsigned short payload1, unsigned short payload2, int timeout, unsigned char status, unsigned char error) {
  logger->info("Sending command: {} {} {} {}", command, payload1, payload2, status);
  OpcUtils::MPSRegister registerOffset;
  if(command < Station::STATION_BASE)
    registerOffset = OpcUtils::MPSRegister::ACTION_ID_BASIC;
  else
    registerOffset = OpcUtils::MPSRegister::ACTION_ID_IN;

  bool statusBit = (bool)(status & Status::STATUS_BUISY); /* CHECK */
  OpcUtils::MPSRegister reg;
  reg = registerOffset + OpcUtils::MPSRegister::ACTION_ID_IN;
  setNodeValue(registerNodes[reg], (uint16_t)command, reg);
  reg = registerOffset + OpcUtils::MPSRegister::DATA_IN;
  setNodeValue(registerNodes[reg].GetChildren()[0], (uint16_t)payload1, reg);
  reg = registerOffset + OpcUtils::MPSRegister::DATA_IN;
  setNodeValue(registerNodes[reg].GetChildren()[1], (uint16_t)payload2, reg);
  reg = registerOffset + OpcUtils::MPSRegister::STATUS_ENABLE_IN;
  setNodeValue(registerNodes[reg], statusBit, reg);
  reg = registerOffset + OpcUtils::MPSRegister::ERROR_IN;
  setNodeValue(registerNodes[reg], (uint8_t)error, reg);

  if(command < Station::STATION_BASE)
    return;

  reg = OpcUtils::MPSRegister::STATUS_BUSY_IN;
  getReturnValue(reg)->setValue(OpcUtils::getNodeValueWithCorrectType(registerNodes[reg], statusBit));

  if (status & Status::STATUS_BUISY) {
    wait_for_buisy();
  }
  if (timeout and (not wait_for_ready(timeout))) {
    std::ostringstream str;
    str << "Timeout during command #" << command << " (Timeout was " << timeout << ")";
    throw timeout_exception(str.str());
  }
}

void Machine::reset() {
  send_command(machine_type_ | Command::COMMAND_RESET);
}

bool Machine::wait_for_ready(int timeout) {
  struct timespec time_c, time_0, time_d;
  if (timeout >= 0) {
    clock_gettime(CLOCK_MONOTONIC, &time_0);
  }
    
  while (!getReturnValue(OpcUtils::MPSRegister::STATUS_READY_IN)->ToInt())
  {
    {
      std::lock_guard<std::mutex> l(lock_);
      if (abort_operation_) {
        std::cout << "Abort operation" << std::endl;
        return false;
      }
    }
    if (timeout >= 0) {
      clock_gettime(CLOCK_MONOTONIC, &time_c);
      timespec_diff( &time_0, &time_c, &time_d);
      if( time_d.tv_sec * 1000 + time_d.tv_nsec / 1000000 > timeout*10) { /* REMOVE *10 from timeout */
        return false;
      }
    }
  }
  if (getReturnValue(OpcUtils::MPSRegister::STATUS_ENABLE_IN)->ToInt())
    return true;
  else
    return false;
}

void Machine::wait_for_buisy() {
  struct timespec time_c, time_0, time_d;
  clock_gettime(CLOCK_MONOTONIC, &time_0);
    
  while ((getReturnValue(OpcUtils::MPSRegister::STATUS_BUSY_IN)->ToInt())) {
    clock_gettime(CLOCK_MONOTONIC, &time_c);
    timespec_diff( &time_0, &time_c, &time_d);
    if( time_d.tv_sec * 1000 + time_d.tv_nsec / 1000000 > Timeout::TIMEOUT_BUISY*10) { /* REMOVE *10 from timeout */
      throw timeout_exception("Machine did not reset buisy flag within time limit");
    }
  }
}

bool Machine::wait_for_free() {
  for(;;) {
    if (!getReturnValue(OpcUtils::MPSRegister::STATUS_BUSY_IN)->ToInt()) {
      return true;
    }
  }
  return false;
}

bool Machine::connect_PLC(const std::string& ip, unsigned short port) {
  if(!reconnect(ip.c_str(), port))
    return false;

  subscribe(SUB_REGISTERS, outs);
  identify();
  return true;
}

Machine::~Machine() {
  disconnect();
}
  

void Machine::set_light(llsf_msgs::LightColor color, llsf_msgs::LightState state, unsigned short time) {
  LightColor m_color;
  switch (color) {
    case llsf_msgs::LightColor::RED:
      m_color = LightColor::LIGHT_COLOR_RED;
      break;
    case llsf_msgs::LightColor::YELLOW:
      m_color = LightColor::LIGHT_COLOR_YELLOW;
      break;
    case llsf_msgs::LightColor::GREEN:
      m_color = LightColor::LIGHT_COLOR_GREEN;
      break;
  }
  switch (m_color) {
    case LightColor::LIGHT_COLOR_RESET:
    case LightColor::LIGHT_COLOR_RED:
    case LightColor::LIGHT_COLOR_YELLOW:
    case LightColor::LIGHT_COLOR_GREEN:
      break;
    default: throw std::invalid_argument("Illegal color! See MPSIoMapping.h for choices.");
  }
  unsigned short int plc_state;
  switch (state) {
    case llsf_msgs::ON:
      plc_state = LightState::LIGHT_STATE_ON;
      break;
    case llsf_msgs::OFF:
      plc_state = LightState::LIGHT_STATE_OFF;
      break;
    case llsf_msgs::BLINK:
      plc_state = LightState::LIGHT_STATE_BLINK;
      break;
    default:
      plc_state = LightState::LIGHT_STATE_OFF;
      // TODO error
  }
  send_command( m_color, plc_state, time);
}

void Machine::conveyor_move(llsf_msgs::ConveyorDirection direction, llsf_msgs::SensorOnMPS sensor)
{
  unsigned short sensor_sps = -1;
  switch (sensor) {
    case llsf_msgs::SensorOnMPS::SENSOR_INPUT:
      sensor_sps = 1;
      break;
    case llsf_msgs::SensorOnMPS::SENSOR_MIDDLE:
      sensor_sps = 2;
      break;
    case llsf_msgs::SensorOnMPS::SENSOR_OUTPUT:
      sensor_sps = 3;
      break;
    default:
      std::cout << "Wrong sensor send" << std::endl;
      break;
  }
  send_command(Command::COMMAND_MOVE_CONVEYOR + machine_type_, sensor_sps, direction, Timeout::TIMEOUT_BAND);
}

void Machine::reset_light() {
  set_light(llsf_msgs::LightColor::RED, llsf_msgs::OFF);
}

void Machine::initLogger()
{
  if(LOG_PATH.empty() || LOG_PATH.length() < 1)  /* stdout redirected logging ... */
    logger = spdlog::stdout_logger_mt("client");
  else /* ... or logging to file */
    logger = spdlog::basic_logger_mt("client", LOG_PATH);

  logger->info("\n\n\nNew logging session started");

  if(false) /* more information needed in log? */
    logger->set_level(spdlog::level::debug);
}

bool Machine::reconnect(const char* ip, unsigned short port)
{
  try
  {
    OpcUa::EndpointDescription* endpoint = OpcUtils::getEndpoint(ip, port);
    logger->info("Connecting to: {}", endpoint->EndpointUrl);

    client = new OpcUa::UaClient(logger);
    client->Connect(*endpoint);

    nodeBasic = OpcUtils::getBasicNode(client);
    nodeIn = OpcUtils::getInNode(client);

    for(int i = 0; i < OpcUtils::MPSRegister::STATUS_READY_BASIC; i++)
      registerNodes[i] = OpcUtils::getNode(client, (OpcUtils::MPSRegister)i);
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

bool Machine::disconnect()
{
  cancelAllSubscriptions(true);

  logger->info("Disconnecting");
  try
  {
    client->Disconnect();
    logger->flush();
    return true;
  }
  catch(...)
  {
    try
    {
      client->Abort();
      logger->flush();
      return true;
    }
    catch(...)
    {
      try
      {
        delete client;
        client = nullptr;
        logger->flush();
        return true;
      }
      catch(...)
      {

      }
    }
  }
  return false;
}

void Machine::subscribeAll()
{
  for(int i = OpcUtils::MPSRegister::ACTION_ID_IN; i != OpcUtils::MPSRegister::STATUS_READY_BASIC; i++)
    subscribe(static_cast<OpcUtils::MPSRegister>(i));
}

void Machine::subscribe(std::vector<OpcUtils::MPSRegister> registers, OpcUtils::ReturnValue* retVals)
{
  for(OpcUtils::MPSRegister reg : registers)
    subscribe(reg, retVals == nullptr ? nullptr : &retVals[reg]);
}

SubscriptionClient* Machine::subscribe(OpcUtils::MPSRegister reg, OpcUtils::ReturnValue* retVal)
{
  auto it = subscriptions.end();
  if((it = subscriptions.find(reg)) != subscriptions.end())
    return it->second;
  OpcUa::Node node = OpcUtils::getNode(client, reg);
  SubscriptionClient* sub = new SubscriptionClient(logger, retVal);
  sub->reg = reg;
  sub->node = node;
  sub->subscription = client->CreateSubscription(100, *sub);
  sub->handle = sub->subscription->SubscribeDataChange(node);
  logger->info("Subscribed to {} (name: {}, handle: {})", OpcUtils::REGISTER_NAMES[reg], node.GetBrowseName().Name, sub->handle);
  subscriptions.insert(SubscriptionClient::pair(reg, sub));
  return sub;
}

void Machine::cancelAllSubscriptions(bool log)
{
  if(log)
    printFinalSubscribtions();

  for(SubscriptionClient::map::iterator it = subscriptions.begin(); it != subscriptions.end();)
  {
    OpcUtils::MPSRegister reg = it->first;
    SubscriptionClient* sub = it->second;
    sub->subscription->UnSubscribe(sub->handle);
    logger->info("Unsubscribed from {} (name: {}, handle: {})", OpcUtils::REGISTER_NAMES[reg], sub->node.GetBrowseName().Name, sub->handle);
    it = subscriptions.erase(it);
  }
}

SubscriptionClient::map::iterator Machine::cancelSubscription(OpcUtils::MPSRegister reg, bool log)
{
  auto it = subscriptions.find(reg);
  if(it != subscriptions.end())
  {
    SubscriptionClient* sub = it->second;
    sub->subscription->UnSubscribe(sub->handle);
    logger->info("Unsubscribed from {} (name: {}, handle: {})", OpcUtils::REGISTER_NAMES[reg], sub->node.GetBrowseName().Name, sub->handle);
    if(log)
      OpcUtils::logReturnValue(getReturnValue(reg), logger, reg);
    return subscriptions.erase(it);
  }
  return it;
}

OpcUtils::ReturnValue* Machine::getReturnValue(OpcUtils::MPSRegister reg)
{
  if(subscriptions.size() == 0)
    return &outs[reg];
  else
  {
    auto it = subscriptions.find(reg);
    if(it != subscriptions.end())
      return it->second->mpsValue;
  }
  return nullptr;
}

bool Machine::setNodeValue(OpcUa::Node node, boost::any val, OpcUtils::MPSRegister reg)
{
  SubscriptionClient::map::iterator it = subscriptions.find(reg);
  if(it != subscriptions.end())
    return OpcUtils::setNodeValue(node, val, it->second->mpsValue);
  return OpcUtils::setNodeValue(node, val);
}

void Machine::printFinalSubscribtions()
{
  if(subscriptions.size() > 0)
    logger->info("Final values of subscribed registers:");
  for(int i = 0; i < OpcUtils::MPSRegister::STATUS_READY_BASIC; i++)
    OpcUtils::logReturnValue(getReturnValue((OpcUtils::MPSRegister)i), logger, (OpcUtils::MPSRegister)i);
}

}
}
