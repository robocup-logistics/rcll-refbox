#include "mqtt_callback.h"


namespace llsfrb {
#if 0
}
#endif
namespace mps_comm {
#if 0
}
#endif

mqtt_callback::mqtt_callback(mqtt::async_client& cli, mqtt::connect_options& connOpts, std::shared_ptr<spdlog::logger> logger)
            : nretry_(0), logger_(logger), cli_(cli), connOpts_(connOpts), subListener_("Subscription", logger) 
{}

mqtt_callback::~mqtt_callback()
{}

void mqtt_callback::reconnect() {
    std::this_thread::sleep_for(std::chrono::milliseconds(2500));
    try {
        cli_.connect(connOpts_, nullptr, *this);
    }
    catch (const mqtt::exception& exec) {
        logger_->error( "Error: {}", exec.what());
        exit(1);
    }
}


void mqtt_callback::on_failure(const mqtt::token& tok) {
    logger_->info("Connection attempt failed");
    if (++nretry_ > MqttUtils::N_RETRY_ATTEMPTS)
        exit(1);
    reconnect();
}

void mqtt_callback::on_success(const mqtt::token& tok) {
}


void mqtt_callback::connected(const std::string& cause)  {
    logger_->info("Connection success");
}

void mqtt_callback::connection_lost(const std::string& cause)  {
    if (cause.empty())
    {
        logger_->info("Connection lost with unknown reason!");
    }
    else
    {
        logger_->info("Connection lost with cause: {}!", cause);
    }
    logger_->info("Reconnecting...");
    nretry_ = 0;
    reconnect();
}

void mqtt_callback::message_arrived(mqtt::const_message_ptr msg) {
    //std::cout << "Message arrived" << std::endl;
    //std::cout << "\ttopic: '" << msg->get_topic() << "'" << std::endl;
    //std::cout << "\tpayload: '" << msg->to_string() << "'\n" << std::endl;
 
    std::string topic = msg->get_topic();
    std::string value = msg->to_string();
    switch(MqttUtils::ParseTopic(topic))
    {
        case Topic::BasicNodes_Busy:
            //std::cout << "MPS sent a BasicBusy update with value " << value << std::endl;
            logger_->info("Received an updated for Basic/Busy with value " + value);
            break;
        case Topic::BasicNodes_Ready:
            //std::cout << "MPS sent a BasicEnabled update with value " << value << std::endl;
            logger_->info("Received an updated for Basic/Ready with value " + value);
            break;
        case Topic::InNodes_Busy:
        {
            //std::cout << "MPS sent a InBusy update with value [" << value << "]" << std::endl;
            logger_->info("Received an updated for In/Busy with value " + value);
            bool val = false;
            if(boost::algorithm::to_lower_copy(value).compare("true"))
                val = true;
            callbacks_[MqttUtils::bits[0]](val);
            break;
        }
        case Topic::InNodes_Ready:
        {
            //std::cout << "MPS sent a InEnabled update with value [" << value << "]" << std::endl;
            logger_->info("Received an updated for In/Ready with value " + value);
            bool val = false;
            if(boost::algorithm::to_lower_copy(value).compare("true"))
                val = true;
            callbacks_[MqttUtils::bits[1]](val);
            break;
        }
        case Topic::InNodes_Sldcnt:
        {
            logger_->info("MPS sent a InSlideCnt update with value [{}]",value);
            unsigned int count = std::stoul(value);
            callbacks_[MqttUtils::registers[5]](count);
            // TODO maybe fix and use this instead with std::any in the map std::any_cast <int (*) (int)> (mapIter->second) (5)
            break;
        }
        default:
            logger_->info("Not a known topic");
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