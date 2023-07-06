#include "mqtt_callback.h"


namespace llsfrb {
#if 0
}
#endif
namespace mps_comm {
#if 0
}
#endif

mqtt_callback::mqtt_callback(mqtt::async_client& cli, mqtt::connect_options& connOpts)
            : nretry_(0), cli_(cli), connOpts_(connOpts), subListener_("Subscription") 
{}

mqtt_callback::~mqtt_callback()
{}

void mqtt_callback::reconnect() {
    std::this_thread::sleep_for(std::chrono::milliseconds(2500));
    try {
        cli_.connect(connOpts_, nullptr, *this);
    }
    catch (const mqtt::exception& exc) {
        std::cerr << "Error: " << exc.what() << std::endl;
        exit(1);
    }
}


void mqtt_callback::on_failure(const mqtt::token& tok) {
    std::cout << "Connection attempt failed" << std::endl;
    if (++nretry_ > MqttUtils::N_RETRY_ATTEMPTS)
        exit(1);
    reconnect();
}

void mqtt_callback::on_success(const mqtt::token& tok) {
}


void mqtt_callback::connected(const std::string& cause)  {
    std::cout << "\nConnection success" << std::endl;
}

void mqtt_callback::connection_lost(const std::string& cause)  {
    std::cout << "\nConnection lost" << std::endl;
    if (!cause.empty())
        std::cout << "\tcause: " << cause << std::endl;

    std::cout << "Reconnecting..." << std::endl;
    nretry_ = 0;
    reconnect();
}

void mqtt_callback::message_arrived(mqtt::const_message_ptr msg) {
    std::cout << "Message arrived" << std::endl;
    std::cout << "\ttopic: '" << msg->get_topic() << "'" << std::endl;
    std::cout << "\tpayload: '" << msg->to_string() << "'\n" << std::endl;
    std::string topic = msg->get_topic();
    std::string value = msg->to_string();
    switch(MqttUtils::ParseTopic(topic))
    {
        case Topic::BasicNodes_Busy:
            std::cout << "MPS sent a BasicBusy update with value " << value << std::endl;
            break;
        case Topic::BasicNodes_Ready:
            std::cout << "MPS sent a BasicEnabled update with value " << value << std::endl;
            break;
        case Topic::InNodes_Busy:
        {
            std::cout << "MPS sent a InBusy update with value [" << value << "]" << std::endl;
            bool val = false;
            if(value.compare("true"))
                val = true;
            callbacks_[MqttUtils::bits[0]](val);
            break;
        }
        case Topic::InNodes_Ready:
        {
            std::cout << "MPS sent a InEnabled update with value [" << value << "]" << std::endl;
            bool val = false;
            if(value.compare("true"))
                val = true;
            callbacks_[MqttUtils::bits[1]](val);
            break;
        }
        case Topic::InNodes_Sldcnt:
        {
            std::cout << "MPS sent a InSlideCnt update with value [" << value << "]" << std::endl;
            callbacks_[MqttUtils::bits[1]](std::stoi(value));
            break;
        }
        default:
            std::cout << "Not a known topic" << std::endl;
    }
}

void mqtt_callback::delivery_complete(mqtt::delivery_token_ptr token)
{
}

void
mqtt_callback::register_busy_callback(std::function<void(bool)> callback)
{
	callbacks_[MqttUtils::bits[0]] = callback;
}

void
mqtt_callback::register_ready_callback(std::function<void(bool)> callback)
{
	callbacks_[MqttUtils::bits[1]] = callback;}

void
mqtt_callback::register_barcode_callback(std::function<void(unsigned long)> callback)
{
	callbacks_[MqttUtils::registers[1]] = callback;
}
void
mqtt_callback::register_slide_callback(std::function<void(unsigned int)> callback)
{
	callbacks_[MqttUtils::registers[5]] = callback;
}




}
}