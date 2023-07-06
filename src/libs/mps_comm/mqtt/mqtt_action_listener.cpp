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
    std::cout << "mqtt_action_listener::on_failure" << std::endl;
    std::cout << name_ << " failure";
    if (tok.get_message_id() != 0)
        std::cout << " for token: [" << tok.get_message_id() << "]" << std::endl;
    std::cout << std::endl;
}

void mqtt_action_listener::on_success(const mqtt::token& tok) {
    std::cout << "mqtt_action_listener::on_success" << std::endl;
    std::cout << name_ << " success";
    if (tok.get_message_id() != 0)
        std::cout << " for token: [" << tok.get_message_id() << "]" << std::endl;
    auto top = tok.get_topics();
    if (top && !top->empty())
        std::cout << "\ttoken topic: '" << (*top)[0] << "', ..." << std::endl;
    std::cout << std::endl;
}

mqtt_action_listener::mqtt_action_listener(const std::string& name) : name_(name)
{

}

mqtt_action_listener::~mqtt_action_listener()
{
    
}


}
}