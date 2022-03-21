#include "SNMP.h"

#include <UDP.h>

namespace SNMP {

SNMP::SNMP(const uint16_t port) {
    _port = port;
}

bool SNMP::begin(UDP *udp) {
    if (_udp) {
        _udp->stop();
    }
    _udp = udp;
    return udp->begin(_port);
}

bool SNMP::loop() {
    char buffer[512];
    if (_udp) {
        if (_udp->parsePacket()) {
            int length = _udp->available();
            if (length) {
                _udp->read(buffer, length);
                _udp->flush();
                Message *message = new Message();
                if (message->parse(buffer, length)) {
                    _onMessage(message, _udp->remoteIP(), _udp->remotePort());
                }
                delete message;
            }
        }
    }
    return false;
}

bool SNMP::send(Message *message, const IPAddress ip, const uint16_t port) {
    // TODO dynamic buffer allocation
    // 1 - size all bindings
    // 2 - allocate buffer
    // 3 - build buffer
    // 4 - send
    // 5 - free buffer
    char buffer[512] = { 0 };
    if (unsigned int size = message->build(buffer)) {
        _udp->beginPacket(ip, port);
        _udp->write((unsigned char*) buffer, size);
        return _udp->endPacket();
    }
    return false;
}

}
