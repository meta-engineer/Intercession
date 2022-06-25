#include "pleep_net.h"

#include "logging/pleep_log.h"
#include "networking/net_message.h"

namespace pleep
{
namespace net
{
    void test_net()
    {
        net::Message<PleepMessageType> msg;
        msg.header.id = PleepMessageType::intercession;

        int a = 1;
        bool b = true;
        float c = 1.7320508079f;

        struct
        {
            float x;
            float y;
        } d[3];

        msg << a << b << c << d;

        PLEEPLOG_DEBUG("Message constructed " + msg.info());
        
        a = 0;
        b = false;
        c = 0.0f;

        msg >> d >> c >> b >> a;

        PLEEPLOG_DEBUG(std::to_string(a) + ", " + std::to_string(b) + ", " + std::to_string(c));
    }
}
}