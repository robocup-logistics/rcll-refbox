#include "mqtt_client_wrapper.h"

#if HAVE_SYSTEM_SPDLOG
#	include <spdlog/sinks/basic_file_sink.h>
#	include <spdlog/sinks/stdout_sinks.h>
#endif
#include <spdlog/logger.h>

namespace llsfrb {
#if 0
}
#endif
namespace mps_comm {
#if 0
}
#endif


mqtt_client_wrapper::mqtt_client_wrapper(const std::string& client_id, std::shared_ptr<spdlog::logger> logger, std::string broker_address_)
{
	logger_ = logger;
	name_ = client_id;
	connected = false;
	broker_address = broker_address_;
    cli = new mqtt::async_client(broker_address, name_);
    subListener_ = new mqtt_action_listener(name_, logger_);
    mqtt::connect_options connOpts;
	connOpts.set_clean_session(false);
	// Install the callback(s) before connecting.

	callback_handler = new mqtt_callback(*cli, connOpts, logger_);
	cli->set_callback(*callback_handler);

	// Start the connection.
	// When completed, the callback will subscribe to topic.
	try {
		logger_->info("Connecting to the MQTT server...");
		cli->connect(connOpts, nullptr, *callback_handler)->wait_for(std::chrono::seconds(10));
	}
	catch (const mqtt::exception& exc) {
		logger_->error("ERROR: Unable to connect to MQTT server: '{}' {}",broker_address, exc.to_string());
		return;
	}
    SubscribeToTopic(MqttUtils::TOPIC_PREFIX + "/" + name_ + "/In/Status/Ready");
	SubscribeToTopic(MqttUtils::TOPIC_PREFIX + "/" + name_ + "/In/Status/Busy");
	if(name_ == "C-RS1" || name_ == "C-RS2" || name_ == "M-RS1" || name_ == "M-RS2")
	{
		logger_->info("Subscribing to the slidecounter!");
		SubscribeToTopic(MqttUtils::TOPIC_PREFIX + "/" + name_ + "/In/" + MqttUtils::registers[5]);
	}
	connected = true;
}

bool mqtt_client_wrapper::SetNodeValue(std::string topic, std::string value)
{
    try {
        logger_->info("Setting node value for {} to {}", topic, value);
		mqtt::message_ptr pubmsg = mqtt::make_message(topic, value);
        pubmsg->set_qos(MqttUtils::QOS);
        cli->publish(pubmsg)->wait_for(std::chrono::seconds(10));
	}
	catch (const mqtt::exception& exc) {
		logger_->error("ERROR: Unable to publish to MQTT server: '{}' {}", broker_address,exc.to_string());
		return false;
	}
	return true;
}

void mqtt_client_wrapper::SubscribeToTopic(std::string topic)
{
    try {
        logger_->info("Subscribing to topic [{}]", topic);
        cli->subscribe(topic, MqttUtils::QOS, nullptr, *subListener_)->wait_for(std::chrono::seconds(5));
    }
	catch (const mqtt::exception& exc) {
        logger_->error( "ERROR: Unable to Subscribe to MQTT topic: '{}' {}", topic, exc.to_string());
		return;
	}
}

mqtt_client_wrapper::~mqtt_client_wrapper()
{
    try {
		logger_->info("Disconnecting from the MQTT server...");
		cli->disconnect()->wait();
		logger_->info("OK");
	}
	catch (const mqtt::exception& exc) {
		logger_->error("{}", exc.to_string());
		return;
	}
}

bool mqtt_client_wrapper::dispatch_command(Instruction command)
{
	bool in = std::get<0>(command) < 100 ? false : true;

	//ACTION ID with command value
	if(!SetNodeValue(MqttUtils::BuildTopic(name_,MqttUtils::registers[0],in), std::to_string(std::get<0>(command))))
		return false;
	// DATA0 with payload 1
	if(!SetNodeValue(MqttUtils::BuildTopic(name_,MqttUtils::registers[2],in), std::to_string(std::get<1>(command))))
		return false;
	// DATA1 with payload 2
	if(!SetNodeValue(MqttUtils::BuildTopic(name_,MqttUtils::registers[3],in), std::to_string(std::get<2>(command))))
		return false;
	// Enabled with payload 3
	if(!SetNodeValue(MqttUtils::BuildTopic(name_,MqttUtils::registers[6] + "/" + MqttUtils::bits[2],in), std::to_string((bool)std::get<5>(command))))
		return false;
	// Enabled with payload 4
	if(!SetNodeValue(MqttUtils::BuildTopic(name_,MqttUtils::registers[6] + "/" + MqttUtils::bits[3],in), std::to_string((bool)std::get<4>(command))))
		return false;

	return true;
}
void
mqtt_client_wrapper::register_busy_callback(std::function<void(bool)> callback)
{
	callback_handler->register_busy_callback(callback);
}

void
mqtt_client_wrapper::register_ready_callback(std::function<void(bool)> callback)
{
	callback_handler->register_ready_callback(callback);
}

void
mqtt_client_wrapper::register_barcode_callback(std::function<void(unsigned long)> callback)
{
	callback_handler->register_barcode_callback(callback);	
}
void
mqtt_client_wrapper::register_slide_callback(std::function<void(unsigned int)> callback)
{
	callback_handler->register_slide_callback(callback);	
}

}
}