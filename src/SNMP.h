#ifndef SNMP_H_
#define SNMP_H_

#include "SNMPMessage.h"

class UDP;

namespace SNMP {

class PORT {
public:
    static constexpr uint16_t SNMP = 161;
    static constexpr uint16_t TRAP = 162;
};

class SNMP {
    using Event = void (*)(const Message*, const IPAddress, const uint16_t);

    friend class Agent;
    friend class Manager;

public:
    bool begin(UDP *udp);
    bool loop();

    bool send(Message *message, const IPAddress ip, const uint16_t port);

    void onMessage(Event event) {
        _onMessage = event;
    }

private:
    SNMP(const uint16_t port);

    uint16_t _port;
    UDP *_udp = nullptr;
    Event _onMessage = nullptr;
};

class Agent: public SNMP {
public:
    Agent() :
            SNMP(PORT::SNMP) {
    }
};

class Manager: public SNMP {
public:
    Manager() :
            SNMP(PORT::TRAP) {
    }
};

}

#endif /* SNMP_H_ */
