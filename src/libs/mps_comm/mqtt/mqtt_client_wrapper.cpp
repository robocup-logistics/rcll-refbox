#include "mqtt_client_wrapper.h"


namespace llsfrb {
#if 0
}
#endif
namespace mps_comm {
#if 0
}
#endif


mqtt_client_wrapper::mqtt_client_wrapper(const std::string& client_id)
{
	name_ = client_id;
	connected = false;
    cli = new mqtt::async_client(MqttUtils::BROKER_ADDRESS, name_);
    subListener_ = new mqtt_action_listener(name_);
    mqtt::connect_options connOpts;
	connOpts.set_clean_session(false);
	// Install the callback(s) before connecting.
	
	callback_handler = new mqtt_callback(*cli, connOpts);
	cli->set_callback(*callback_handler);

	// Start the connection.
	// When completed, the callback will subscribe to topic.

	try {
		std::cout << "Connecting to the MQTT server..." << std::flush;
		cli->connect(connOpts, nullptr, *callback_handler)->wait_for(std::chrono::seconds(10));
	}
	catch (const mqtt::exception& exc) {
		std::cerr << "\nERROR: Unable to connect to MQTT server: '"
			<< MqttUtils::BROKER_ADDRESS << "'" << exc << std::endl;
		return;
	}
    SubscribeToTopic("MPS/C-BS/In/Status/Ready");
	SubscribeToTopic("MPS/C-BS/In/Status/Busy");
	SubscribeToTopic("MPS/C-BS/Basic/Status/Ready");
	SubscribeToTopic("MPS/C-BS/Basic/Status/Busy");
    //SetNodeValue("TestPublishfromRefbox");
    //std::this_thread::sleep_for(std::chrono::milliseconds(5000));
	connected = true;
}

bool mqtt_client_wrapper::SetNodeValue(std::string topic, std::string value)
{
    try {
        //std::cout << "Setting node value!" << std::endl;
		mqtt::message_ptr pubmsg = mqtt::make_message(topic, value);
        pubmsg->set_qos(MqttUtils::QOS);
        cli->publish(pubmsg)->wait_for(std::chrono::seconds(10));
	}
	catch (const mqtt::exception& exc) {
		std::cerr << "\nERROR: Unable to publish to MQTT server: '"
			<< MqttUtils::BROKER_ADDRESS << "' " << exc << std::endl;
		return false;
	}
	return true;
}

void mqtt_client_wrapper::SubscribeToTopic(std::string topic)
{
    try {
        std::cout << "Subscribing to topic [" << topic << "]" << std::endl;
        cli->subscribe(topic, MqttUtils::QOS, nullptr, *subListener_)->wait_for(std::chrono::seconds(5));
        //cli->subscribe();
        std::cout << "Subscribed to topic" << std::endl;
    }
	catch (const mqtt::exception& exc) {
        std::cerr << "\nERROR: Unable to publish to MQTT server: '" << MqttUtils::BROKER_ADDRESS << "' " << exc << std::endl;
		return;
	}
}

mqtt_client_wrapper::~mqtt_client_wrapper()
{
    try {
		std::cout << "\nDisconnecting from the MQTT server..." << std::flush;
		cli->disconnect()->wait();
		std::cout << "OK" << std::endl;
	}
	catch (const mqtt::exception& exc) {
		std::cerr << exc << std::endl;
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
	if(!SetNodeValue(MqttUtils::BuildTopic(name_,MqttUtils::registers[6] + "/" + MqttUtils::bits[3],in), std::to_string((bool)std::get<3>(command))))
		return false;
	// Enabled with payload 4
	if(!SetNodeValue(MqttUtils::BuildTopic(name_,MqttUtils::registers[6] + "/" + MqttUtils::bits[2],in), std::to_string((bool)std::get<4>(command))))
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