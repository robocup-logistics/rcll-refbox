#include "mqtt_action_listener.h"


namespace llsfrb {
#if 0
}
#endif
namespace mps_comm {
#if 0
}
#endif

void mqtt_action_listener::on_failure(const mqtt::token& tok) {
    logger_->info("Failure for token: [{}]",  std::to_string(tok.get_message_id()));

}

void mqtt_action_listener::on_success(const mqtt::token& tok) {
    auto top = tok.get_topics();
    if (top && !top->empty())
    {
        return;
    }
    logger_->info("Success for token: [{}] with topic {}",std::to_string(tok.get_message_id()),(*top)[0]);
}

mqtt_action_listener::mqtt_action_listener(const std::string& name, std::shared_ptr<spdlog::logger> logger) : name_(name), logger_(logger)
{

}

mqtt_action_listener::~mqtt_action_listener()
{
    
}


}
}