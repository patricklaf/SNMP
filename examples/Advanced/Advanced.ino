/*
 Advanced

 This sketch implements an advanced SNMP agent.

 This is an example where more than one OID is handled and getrequest,
 getnextrequest, getbulkrequest and setrequest messages are processed differently.

 It also shows how to build and send trap and inform messages.

 The agent will accept request for sysDescr.0, sysObjectID.0, sysUpTime.0,
 sysContact.0, sysName.0 and sysLocation.0 declared in SNMPv2-MIB.

 The agent can be tested with commands from a Linux command line.

 snmpget -v 2c -c public 192.168.2.2 sysName.0

 If everything is OK, the command displays:
 SNMPv2-MIB::sysName.0 = STRING: Nucleo F429ZI

 snmpgetnext -v 2c -c public 192.168.2.2 sysContact.0

 If everything is OK, the command displays:
 SNMPv2-MIB::sysName.0 = STRING: Nucleo F429ZI

 1.3.6.1.2.1.1 is the root of implemented MIB.

 snmpwalk -v 1 -c public 192.168.2.2 1.3.6.1.2.1.1

 If everything is OK, the command displays:
 SNMPv2-MIB::sysObjectID.0 = OID: ccitt
 DISMAN-EVENT-MIB::sysUpTimeInstance = Timeticks: (8596) 0:01:25.96
 SNMPv2-MIB::sysContact.0 = STRING: Patrick Lafarguette
 SNMPv2-MIB::sysName.0 = STRING: Nucleo F429ZI
 SNMPv2-MIB::sysLocation.0 = STRING: Home
 End of MIB

 The same command with protocol version 2c:

 snmpwalk -v 2c -c public 192.168.2.2 1.3.6.1.2.1.1

 If everything is OK, the command displays:
 SNMPv2-MIB::sysObjectID.0 = OID: ccitt
 DISMAN-EVENT-MIB::sysUpTimeInstance = Timeticks: (6916) 0:01:09.16
 SNMPv2-MIB::sysContact.0 = STRING: Patrick Lafarguette
 SNMPv2-MIB::sysName.0 = STRING: Nucleo F429ZI
 SNMPv2-MIB::sysLocation.0 = STRING: Home
 SNMPv2-MIB::sysLocation.0 = No more variables left in this MIB View (It is past the end of the MIB tree)

 snmpset -v 2c -c private 192.168.2.2 sysContact.0 s patricklaf

 If everything is OK, the command displays:
 SNMPv2-MIB::sysContact.0 = STRING: patricklaf

 snmpbulkget -v 2c -c public -Cn1 -Cr2 192.168.2.2 sysDescr sysUpTime

 If everything is OK, the command displays:
 SNMPv2-MIB::sysDescr.0 = STRING: Mega 2560
 SNMPv2-MIB::sysUpTime.0 = Timeticks: (18628) 0:03:06.28
 SNMPv2-MIB::sysContact.0 = STRING: patricklaf

 Tested on a Nucleo F429ZI board with STM32 Arduino core 2.2.0.

 Created 11/04/2022 by Patrick Lafarguette
 */

#if ARDUINO_ARCH_AVR
#if ARDUINO_AVR_UNO
#define BOARD "Uno"
#endif
#if ARDUINO_AVR_MEGA2560
#define BOARD "Mega 2560"
#endif

#include <Ethernet.h> // Ethernet support. Replace if needed.

EthernetUDP udp;
#endif

#if ARDUINO_ARCH_STM32
#define BOARD "Nucleo F767ZI"

#include <STM32Ethernet.h> // Ethernet support. Replace if needed.

EthernetUDP udp;
#endif

#if ARDUINO_ARCH_ESP32
#define BOARD "ESP32-POE"

#include <ETH.h> // Ethernet support. Replace if needed.

NetworkUDP udp;
#endif

#include <SNMP.h>

SNMP::Agent snmp;

// Use some SNMP classes
using SNMP::IntegerBER;
using SNMP::ObjectIdentifierBER;
using SNMP::OctetStringBER;
using SNMP::TimeTicksBER;
using SNMP::Counter32BER;
using SNMP::VarBind;
using SNMP::VarBindList;

// Communities
const char PUBLIC[] PROGMEM = "public";
const char PRIVATE[] PROGMEM = "private";
const char *const COMMUNITIES[] PROGMEM = { PUBLIC, PRIVATE };

// OIDs
const char SYSDESCR[] PROGMEM = "1.3.6.1.2.1.1.1.0";
const char SYSOBJECTID[] PROGMEM = "1.3.6.1.2.1.1.2.0";
const char SYSUPTIME[] PROGMEM = "1.3.6.1.2.1.1.3.0";
const char SYSCONTACT[] PROGMEM = "1.3.6.1.2.1.1.4.0";
const char SYSNAME[] PROGMEM = "1.3.6.1.2.1.1.5.0";
const char SYSLOCATION[] PROGMEM = "1.3.6.1.2.1.1.6.0";

const char *const NAMES[] PROGMEM = { SYSDESCR, SYSOBJECTID, SYSUPTIME, SYSCONTACT, SYSNAME, SYSLOCATION };

const char *OUTTRAPS = "1.3.6.1.2.1.11.29";
const char *ENTERPRISE = "1.3.6.1.4.1.121";

// This class encapsulates the MIB objects interface
class MIB {
    using Setter = void (MIB::*)(const char*);

    // Simple helper class to handle communities
    class Community {
    public:
        static bool isKnown(const char *name) {
            return match(name) != UNKNOWN;
        }

        static bool isReadWrite(const char *name) {
            return match(name) == READWRITE;
        }

    private:
        enum : uint8_t {
            READONLY,
            READWRITE,
            UNKNOWN,
            COUNT = UNKNOWN,
        };

        // Returns index of community equals to name
        // Returns UNKNOWN if none
        static uint8_t match(const char *name) {
            for (uint8_t index = 0; index < COUNT; ++index) {
                if (strcmp_P(name, static_cast<char*>(pgm_read_ptr(&COMMUNITIES[index]))) == 0) {
                    return index;
                }
            }
            return UNKNOWN;
        }
    };

public:
    // Simple helper class to handle OIDs
    class OID {
    public:
        enum : uint8_t {
            SYSDESCR,           // Read only
            SYSOBJECTID,        // Read only
            SYSUPTIME,          // Read only
            SYSCONTACT,         // Read write
            SYSNAME,            // Read write
            SYSLOCATION,        // Read write
            LAST = SYSLOCATION, // Used for GETNEXTREQUEST
            UNKNOWN,
            COUNT = UNKNOWN,
        };

        // Returns index of OID equals to name
        // Returns UNKNOWN if none
        static uint8_t match(const char *name) {
            for (uint8_t index = 0; index < COUNT; ++index) {
                if (strcmp_P(name, static_cast<char*>(pgm_read_ptr(&NAMES[index]))) == 0) {
                    return index;
                }
            }
            return UNKNOWN;
        }

        // Returns index of first OID starting with name
        // Returns UNKNOWN if none
        static uint8_t start(const char *name) {
            for (uint8_t index = 0; index < COUNT; ++index) {
                if (strncmp_P(name, static_cast<char*>(pgm_read_ptr(&NAMES[index])), strlen(name)) == 0) {
                    return index;
                }
            }
            return UNKNOWN;
        }

        // Returns a copy into RAM of an OID stored with PROGMEM attribute..
        static char* name(const int index) {
            char *pointer = static_cast<char*>(pgm_read_ptr(&NAMES[index]));
            char *name = static_cast<char*>(malloc(strlen_P(pointer) + 1));
            strcpy_P(name, pointer);
            return name;
        }
    };

    // Process incoming message
    bool message(const SNMP::Message *request, SNMP::Message *response) {
        switch (request->getType()) {
        case SNMP::Type::GetRequest:
            return get(request, response);
        case SNMP::Type::GetNextRequest:
            return next(request, response);
        case SNMP::Type::SetRequest:
            return set(request, response);
        case SNMP::Type::GetBulkRequest:
            return bulk(request, response);
        }
        return false;
    }

    // Build trap message
    SNMP::Message* trap(const uint8_t type) {
        SNMP::Message* message = nullptr;
        switch (type) {
            case SNMP::Type::Trap:
                message = new SNMP::Message(SNMP::Version::V1, "public", SNMP::Type::Trap);
                // Set enterprise OID
                message->setEnterprise(ENTERPRISE);
                // Set agent address
                message->setAgentAddress(IPAddress(192, 168, 2, 2));
                // Set trap type, generic and specific
                message->setTrap(SNMP::Trap::ColdStart);
                break;
            case SNMP::Type::InformRequest:
            case SNMP::Type::SNMPv2Trap:
                message = new SNMP::Message(SNMP::Version::V2C, "public", type);
                // Set mandatory snmpTrapOID.0
                // Set also sysUpTime.0 silently
                message->setSNMPTrapOID(SNMP::Message::OID::COLDSTART);
                break;
        }
        // Variable binding list
        // snmpOutTraps OBJECT-TYPE
        // SYNTAX Counter
        // ACCESS read-only
        // STATUS mandatory
        // DESCRIPTION
        // "The total number of SNMP Trap PDUs which have
        // been generated by the SNMP protocol entity."
        message->add(OUTTRAPS, new Counter32BER(++_snmpOutTraps));
        return message;
    }

    // Setters
    void setContact(const char *contact) {
        update(&_contact, contact);
    }

    void setLocation(const char *location) {
        update(&_location, location);
    }

    void setName(const char *name) {
        update(&_name, name);
      }

private:
    // Add a MIB node specified by its index as a variable binding to an SNMP message.
    VarBind* add(SNMP::Message *message, uint8_t index) {
        VarBind* varbind = nullptr;
        char *name = OID::name(index);
        switch (index) {
        case OID::SYSDESCR:
            varbind = message->add(name, new OctetStringBER(_descr));
            break;
        case OID::SYSOBJECTID:
            varbind = message->add(name, new ObjectIdentifierBER(_objectID));
            break;
        case OID::SYSUPTIME:
            varbind = message->add(name, new TimeTicksBER(millis() / 10));
            break;
        case OID::SYSCONTACT:
            varbind = message->add(name, new OctetStringBER(_contact));
            break;
        case OID::SYSNAME:
            varbind = message->add(name, new OctetStringBER(_name));
            break;
        case OID::SYSLOCATION:
            varbind = message->add(name, new OctetStringBER(_location));
            break;
        }
        // Avoid memory leak
        free(name);
        return varbind;
    }

    // Set a MIB node value from a variable binding and add node to response
    void set(SNMP::Message *response, VarBind *varbind, uint8_t match, uint8_t index, MIB::Setter setter) {
        // Check only variable binding value type, should be OCTET STRING
        if (varbind->getValue()->getType() == SNMP::Type::OctetString) {
            // Call setter with value
            (this->*setter)(static_cast<OctetStringBER*>(varbind->getValue())->getValue());
            // Add MIB object to response
            add(response, match);
        } else {
            // Set error status and index
            response->setError(SNMP::Error::WrongType, index + 1);
            // Add OID to response with null value
            response->add(varbind->getName());
        }
    }

    // Process incoming GETREQUEST message and build response
    bool get(const SNMP::Message *message, SNMP::Message *response) {
        // Check if community is known
        if (Community::isKnown(message->getCommunity())) {
            // Get the variable binding list from the message.
            VarBindList *varbindlist = message->getVarBindList();
            const uint8_t count = varbindlist->count();
            for (uint8_t index = 0; index < count; ++index) {
                // Each variable binding is a sequence of 2 objects:
                // - First one is and ObjectIdentifierBER. It holds the OID
                // - Second is the value of any type
                VarBind *varbind = (*varbindlist)[index];
                // There is a convenient function to get the OID as a const char*
                const char *name = varbind->getName();
                uint8_t match = OID::match(name);
                switch (match) {
                case OID::UNKNOWN:
                    // OID is unknown
                    switch (message->getVersion()) {
                    case SNMP::Version::V1:
                        // Set error, status and index
                        response->setError(SNMP::Error::NoSuchName, index + 1);
                        break;
                    case SNMP::Version::V2C:
                        // No such object
                        response->add(name, new SNMP::NoSuchObjectBER());
                        break;
                    }
                    break;
                default:
                    // OID is identified
                    add(response, match);
                    break;
                }
            }
            return true;
        }
        return false;
    }

    // Process incoming GETNEXTREQUEST message and build response
    bool next(const SNMP::Message *message, SNMP::Message *response) {
        // Check if community is known
        if (Community::isKnown(message->getCommunity())) {
            // Get the variable binding list from the message.
            VarBindList *varbindlist = message->getVarBindList();
            const uint8_t count = varbindlist->count();
            for (uint8_t index = 0; index < count; ++index) {
                // Each variable binding is a sequence of 2 objects:
                // - First one is and ObjectIdentifierBER. It holds the OID
                // - Second is the value of any type
                VarBind *varbind = (*varbindlist)[index];
                // There is a convenient function to get the OID as a const char*
                const char *name = varbind->getName();
                uint8_t start = OID::start(name);
                switch (start) {
                case OID::LAST:
                    // This is the last OID of the MIB
                    switch (message->getVersion()) {
                    case SNMP::Version::V1:
                        // Set error, status and index
                        response->setError(SNMP::Error::NoSuchName, index + 1);
                        // Add OID to response with null value;
                        response->add(name);
                        break;
                    case SNMP::Version::V2C:
                        // End of MIB view
                        response->add(name, new SNMP::EndOfMIBViewBER());
                        break;
                    }
                    break;
                case OID::UNKNOWN:
                    // OID is unknown
                    // Set error, status and index
                    response->setError(SNMP::Error::GenErr, index + 1);
                    // Add OID to response with null value;
                    response->add(name);
                    break;
                default:
                    // Add next object of the MIB
                    add(response, start + 1);
                    break;
                }
            }
            return true;
        }
        return false;
    }

    // Process incoming SETTREQUEST message and build response
    bool set(const SNMP::Message *message, SNMP::Message *response) {
        // Check if community is known
        if (Community::isReadWrite(message->getCommunity())) {
            // Get the variable binding list from the message.
            VarBindList *varbindlist = message->getVarBindList();
            const uint8_t count = varbindlist->count();
            for (uint8_t index = 0; index < count; ++index) {
                // Each variable binding is a sequence of 2 objects:
                // - First one is and ObjectIdentifierBER. It holds the OID
                // - Second is the value of any type
                VarBind *varbind = (*varbindlist)[index];
                // There is a convenient function to get the OID as a const char*
                const char *name = varbind->getName();
                uint8_t match = OID::match(name);
                switch (match) {
                case OID::SYSCONTACT:
                    // Set value for MIB node SYSCONTACT
                    // Set error on failure
                    set(response, varbind, match, index, &MIB::setContact);
                    break;
                case OID::SYSNAME:
                    // Set value for MIB node SYSNAME
                    // Set error on failure
                    set(response, varbind, match, index, &MIB::setName);
                    break;
                case OID::SYSLOCATION:
                    // Set value for MIB node SYSLOCATION
                    // Set error on failure
                    set(response, varbind, match, index, &MIB::setLocation);
                    break;
                case OID::UNKNOWN:
                    // OID is unknown
                    // Set error, status and index
                    response->setError(SNMP::Error::GenErr, index + 1);
                    // Add OID to response with null value;
                    response->add(name);
                    break;
                default:
                    // The object can not be set
                    // Set error, status and index
                    response->setError(SNMP::Error::NoAccess, index + 1);
                    // Add OID to response with null value;
                    response->add(name);
                    break;
                }
            }
            return true;
        }
        return false;
    }

    // Process incoming GETBULKREQUEST message and build response
    bool bulk(const SNMP::Message *message, SNMP::Message *response) {
        // Check if community is known
        if (Community::isKnown(message->getCommunity())) {
            // Get the variable binding list from the message.
            VarBindList *varbindlist = message->getVarBindList();
            const uint8_t count = varbindlist->count();
            for (uint8_t index = 0; index < count; ++index) {
                // Each variable binding is a sequence of 2 objects:
                // - First one is and ObjectIdentifierBER. It holds the OID
                // - Second is the value of any type
                VarBind *varbind = (*varbindlist)[index];
                // There is a convenient function to get the OID as a const char*
                const char *name = varbind->getName();
                uint8_t start = OID::start(name);
                switch (start) {
                case OID::UNKNOWN:
                    break;
                default:
                    // OID is identified
                    if (index < message->getNonRepeaters()) {
                        add(response, start);
                    } else {
                        for (uint8_t repetition = 0; repetition < message->getMaxRepetition(); ++repetition) {
                            add(response, start);
                            if (++start == OID::COUNT) {
                                break;
                            }
                        }
                    }
                    break;
                }
            }
            return true;
        }
        return false;
    }

    void update(char **member, const char *value) {
        free(*member);
        *member = strdup(value);
    }

    // Variables to hold value of MIB nodes
    const char* _descr = BOARD;
    const char* _objectID = nullptr;
    char* _contact = nullptr;
    char* _name = nullptr;
    char* _location = nullptr;
    uint32_t _snmpOutTraps = 0;
};

MIB mib;

// Event handler to process SNMP messages
void onMessage(const SNMP::Message *message, const IPAddress remote, const uint16_t port) {
    // Create an SNMP message for response
    SNMP::Message *response = new SNMP::Message(message->getVersion(),
            message->getCommunity(), SNMP::Type::GetResponse);
    // The response must have the same request-id as the request
    response->setRequestID(message->getRequestID());
    // Build response for message
    if (mib.message(message, response)) {
        // Send response
        snmp.send(response, remote, port);
    }
    // Avoid memory leak
    delete response;
}

unsigned long start;

void setup() {
#if ARDUINO_ARCH_AVR
    Serial.begin(115200);
#else
    Serial.begin(921600);
#endif
    // Ethernet
#if ARDUINO_ARCH_AVR
    byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
    Ethernet.begin(mac, IPAddress(192, 168, 2, 2));
#endif
#if ARDUINO_ARCH_STM32
    Ethernet.begin(IPAddress(192, 168, 2, 2));
#endif
#if ARDUINO_ARCH_ESP32
    ETH.begin();
    ETH.config(IPAddress(192, 168, 2, 2), IPAddress(192, 168, 2, 1),
            IPAddress(255, 255, 255, 0), IPAddress(192, 168, 2, 1));
#endif
    // MIB
    mib.setContact("Patrick Lafarguette");
    mib.setLocation("Home");
    mib.setName(BOARD);
    // SNMP
    snmp.begin(udp);
    snmp.onMessage(onMessage);
    // Start
    start = millis();
}

void loop() {
    // Agent loop function must be called to process incoming messages
    snmp.loop();
    // Send traps every 5 seconds
    if (millis() - start >= 5000) {
        start = millis();
        // Create a trap message and send it
        SNMP::Message *message = mib.trap(SNMP::Type::Trap);
        snmp.send(message, IPAddress(192, 168, 2, 1), SNMP::Port::Trap);
        delete message;
        // Create a version 2 trap message and send it
        message = mib.trap(SNMP::Type::SNMPv2Trap);
        snmp.send(message, IPAddress(192, 168, 2, 1), SNMP::Port::Trap);
        delete message;
        // Create an inform message and send it
        message = mib.trap(SNMP::Type::InformRequest);
        snmp.send(message, IPAddress(192, 168, 2, 1), SNMP::Port::Trap);
        delete message;
    }
}
