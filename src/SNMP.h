#ifndef SNMP_H_
#define SNMP_H_

#include "SNMPMessage.h"

class UDP;

namespace SNMP {

#define PORT_SNMP 161
#define PORT_SNMPTRAP 162

class SNMP {
    using MessageEvent = void (*)(const Message*, const IPAddress, const uint16_t);

    friend class Agent;
    friend class Manager;

public:
    bool begin(UDP *udp);
    bool loop();

    bool send(Message *message, const IPAddress ip, const uint16_t port);

    void onMessage(MessageEvent event) {
        _onMessage = event;
    }

private:
    SNMP(const uint16_t port);

    uint16_t _port;
    UDP *_udp = nullptr;
    MessageEvent _onMessage = nullptr;
};

class Agent: public SNMP {
public:
    Agent() :
            SNMP(PORT_SNMP) {
    }
};

class Manager: public SNMP {
public:
    Manager() :
            SNMP(PORT_SNMPTRAP) {
    }
};

}

#endif /* SNMP_H_ */
