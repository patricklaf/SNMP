#ifndef SNMP_H_
#define SNMP_H_

#include "SNMPMessage.h"

#include <Udp.h>

/**
 * @namespace SNMP
 * @brief %SNMP library namespace.
 */
namespace SNMP {

/**
 * @struct Port
 * @brief Helper struct to handle UDP ports.
 *
 * %SNMP uses 2 well-known UDP ports.
 *
 * - Manager uses port 161 to communicate with an agent.
 * - Agent uses port 162 to send trap messages to the manager.
 */
struct Port {
    static constexpr uint16_t SNMP = 161; /**< SNMP default UDP port. */
    static constexpr uint16_t Trap = 162; /**< SNMP default UDP port for TRAP, INFORMREQUEST and SNMPV2TRAP messages. */
};

/**
 * @class SNMP
 * @brief Base class for Agent and Manager.
 */
class SNMP {
    /**
     * @brief On message event user handler type.
     *
     * The sketch must define an event handler function to process incoming message.
     *
     * Example
     *
     * ```cpp
     * void onMessage(const SNMP::Message *message, const IPAddress remote, const uint16_t port) {
     *     // User code here...
     * }
     * ```
     *
     * @param message %SNMP message to process.
     * @param remote IP address of the sender.
     * @param port UDP port of the sender.
     */
    using Event = void (*)(const Message*, const IPAddress, const uint16_t);

public:
    /**
     * @brief Initializes network.
     *
     * @param udp UDP client.
     * @return 1 if success, 0 if failure.
     */
    bool begin(UDP& udp) {
        _udp = &udp;
        return _udp->begin(_port);
    }

    /**
     * @brief Network read operation.
     *
     * Read incoming packet, parses as an %SNMP message an calls user message handler.
     *
     * @warning This function must be called frequently from the sketch %loop()
     * function.
     */
    void loop() {
#if SNMP_STREAM
        if (_udp->parsePacket()) {
            Message *message = new Message();
            message->parse(*_udp);
            _onMessage(message, _udp->remoteIP(), _udp->remotePort());
            delete message;
        }
#else
        if (_udp->parsePacket()) {
            uint32_t length = _udp->available();
            uint8_t *buffer = static_cast<uint8_t*>(malloc(length));
            if (buffer) {
                _udp->read(buffer, length);
                Message *message = new Message();
                message->parse(buffer);
                free(buffer);
                _onMessage(message, _udp->remoteIP(), _udp->remotePort());
                delete message;
            }
        }
#endif
    }

    /**
     * @brief Network write operation
     *
     * Builds message and write outgoing packet.
     *
     * @param message %SNMP message to send.
     * @param ip IP address to send to.
     * @param port UDP port to send to
     * @return 1 if success, 0 if failure.
     */
    bool send(Message *message, const IPAddress ip, const uint16_t port) {
#if SNMP_STREAM
        _udp->beginPacket(ip, port);
        message->build(*_udp);
        return _udp->endPacket();
#else
        uint32_t length = message->getSize(true);
        uint8_t *buffer = static_cast<uint8_t*>(malloc(length));
        message->build(buffer);
        _udp->beginPacket(ip, port);
        _udp->write(buffer, length);
        free(buffer);
        return _udp->endPacket();
#endif
    }

    /**
     * @brief Sets on message event user handler.
     *
     * @param event Event handler.
     */
    void onMessage(Event event) {
        _onMessage = event;
    }

private:
    /**
     * @brief Creates an SNMP object.
     *
     * @param port UDP port.
     */
    SNMP(const uint16_t port) {
        _port = port;
    }

    /** UDP port .*/
    uint16_t _port = Port::SNMP;
    /** UDP client. */
    UDP *_udp = nullptr;
    /** On message event user handler. */
    Event _onMessage = nullptr;

    friend class Agent;
    friend class Manager;
};

/**
 * @class Agent
 * @brief %SNMP agent.
 *
 * An %SNMP agent listen to UDP port Port::SNMP.
 */
class Agent: public SNMP {
public:
    /**
     * @brief Creates an %SNMP agent.
     *
     */
    Agent() :
            SNMP(Port::SNMP) {
    }
};

/**
 * @class Manager
 * @brief %SNMP manager.
 *
 * An %SNMP manager listen to UDP port Port::Trap.
 */
class Manager: public SNMP {
public:
    /**
     * @brief Creates an %SNMP manager.
     *
     */
    Manager() :
            SNMP(Port::Trap) {
    }
};

} // namespace SNMP

#endif /* SNMP_H_ */
