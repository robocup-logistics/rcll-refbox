#include "mqtt_utils.h"


namespace llsfrb {
#if 0
}
#endif
namespace mps_comm {
#if 0
}
#endif

Topic MqttUtils::ParseTopic(std::string topic){
    bool in = false;
    bool basic = false;
    bool ready = false;
    bool busy = false;
    bool slidecnt = false;
    if (topic.find("/In/") != std::string::npos)
    {
        in = true;
    }
    if(topic.find("/Basic/") != std::string::npos)
    {
        basic = true;
    }
    if(topic.find("/"+MqttUtils::bits[0]) != std::string::npos)
    {
        busy = true;
    }
    if(topic.find("/"+MqttUtils::bits[1]) != std::string::npos)
    {
        ready = true;
    }
    if(topic.find("/"+MqttUtils::registers[5]) != std::string::npos)
    {
        slidecnt = true;
    }

    if(in && ready)
        return Topic::InNodes_Ready;
    if(in && busy)
        return Topic::InNodes_Busy;
    if(basic && ready)
        return Topic::InNodes_Ready;
    if(basic && busy)
        return Topic::InNodes_Busy;
    if(in && slidecnt)
        return Topic::InNodes_Sldcnt;
    return Topic::Ignored;
}

std::string MqttUtils::BuildTopic(std::string name, std::string target_register, bool in)
{
    std::stringstream s;
    s << TOPIC_PREFIX << "/" << name << "/" << (in ? folders[0] : folders[1]) << "/" << target_register;
    return s.str();
}


}
}
