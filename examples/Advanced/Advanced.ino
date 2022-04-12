/*
 Advanced

 This sketch implements an advanced SNMP agent.

 This is an example where more than one OID is handled and getrequest,
 getnextrequest and setrequest messages are processed differently.

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

 Tested on a Nucleo F429ZI board with STM32 Arduino core 2.2.0.

 Created 11/04/2022 by Patrick Lafarguette
 */

#include <STM32Ethernet.h> // Ethernet support. Replace if needed.
#include <SNMP.h>

EthernetUDP udp;
SNMP::Agent snmp;

// Use some SNMP classes
using SNMP::IntegerBER;
using SNMP::ObjectIdentifierBER;
using SNMP::OctetStringBER;
using SNMP::TimeTicksBER;
using SNMP::VarBind;
using SNMP::VarBindList;

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
        enum {
            READONLY,
            READWRITE,
            UNKNOWN,
            COUNT = UNKNOWN,
        };

        static inline const char *COMUNITIES[] = {
                "public",
                "private",
        };

        // Returns index of community equals to name
        // Returns UNKNOWN if none
        static unsigned int match(const char *name) {
            for (unsigned int index = 0; index < COUNT; ++index) {
                if (strcmp(COMUNITIES[index], name) == 0) {
                    return index;
                }
            }
            return UNKNOWN;
        }
    };

public:
    // Simple helper class to handle OID
    class OID {
    public:
        enum {
            SYSDESCR,           // Read only
            SYSOBJECTID,        // Read only
            SYSUPTIME,          // Read only
            SYSCONTACT,         // Read write
            SYSNAME,            // Read write
            SYSLOCATION,        // Read write
            LAST = SYSLOCATION, // Used for GETNEXTREQUEST
            COUNT,
        };

        static inline const char *NAMES[] = {
                "1.3.6.1.2.1.1.1.0",
                "1.3.6.1.2.1.1.2.0",
                "1.3.6.1.2.1.1.3.0",
                "1.3.6.1.2.1.1.4.0",
                "1.3.6.1.2.1.1.5.0",
                "1.3.6.1.2.1.1.6.0",
        };

        // Returns index of OID equals to name
        // Returns COUNT if none
        static unsigned int match(const char *name) {
            for (unsigned int index = 0; index < COUNT; ++index) {
                if (strcmp(NAMES[index], name) == 0) {
                    return index;
                }
            }
            return COUNT;
        }

        // Returns index of first OID starting with name
        // Returns COUNT if none
        static unsigned int start(const char *name) {
            for (unsigned int index = 0; index < COUNT; ++index) {
                if (strncmp(NAMES[index], name, strlen(name)) == 0) {
                    return index;
                }
            }
            return COUNT;
        }
    };

    // Process incoming message
    bool message(const SNMP::Message *request, SNMP::Message *response) {
        switch (request->getType()) {
        case SNMP::TYPE_GETREQUEST:
            return get(request, response);
        case SNMP::TYPE_GETNEXTREQUEST:
            return next(request, response);
        case SNMP::TYPE_SETREQUEST:
            return set(request, response);
        }
        return false;
    }

    // Build trap message
    SNMP::Message* trap(const uint8_t type) {
        SNMP::Message* message = nullptr;
        switch (type) {
            case SNMP::TYPE_TRAP:
                message = new SNMP::Message(SNMP::VERSION1, "public", SNMP::TYPE_TRAP);
                // Set enterprise OID
                message->setEnterprise("1.3.6.1.4.1.121");
                // Set agent address
                message->setAgentAddress(IPAddress(192, 168, 2, 2));
                // Set trap type, generic and specific
                message->setTrap(SNMP::COLD_START);
                break;
            case SNMP::TYPE_INFORMREQUEST:
            case SNMP::TYPE_SNMPV2TRAP:
                message = new SNMP::Message(SNMP::VERSION2C, "public", type);
                // Set mandatory snmpTrapOID.0
                // Set also sysUpTime.0 silently
                message->setSNMPTrapOID(SNMP::Message::OID::COLDSTART);
                break;
        }
         // Variable binding list
        message->add("OID", new IntegerBER(123456));
        return message;
    }

    // Setters
    void setContact(const char *contact) {
        free(_contact);
        _contact = strdup(contact);
    }

    void setLocation(const char *location) {
        free(_location);
        _location = strdup(location);
    }

    void setName(const char *name) {
        free(_name);
         _name = strdup(name);
      }

private:
    // Add a MIB node specified by its index as a variable binding to an SNMP message.
    VarBind* add(SNMP::Message *message, unsigned int index) {
        switch (index) {
        case OID::SYSDESCR:
            return message->add(OID::NAMES[index], new OctetStringBER(_descr));
        case OID::SYSOBJECTID:
            return message->add(OID::NAMES[index], new ObjectIdentifierBER(_objectID));
        case OID::SYSUPTIME:
            return message->add(OID::NAMES[index], new TimeTicksBER(millis() / 10));
        case OID::SYSCONTACT:
            return message->add(OID::NAMES[index], new OctetStringBER(_contact));
        case OID::SYSNAME:
            return message->add(OID::NAMES[index], new OctetStringBER(_name));
        case OID::SYSLOCATION:
            return message->add(OID::NAMES[index], new OctetStringBER(_location));
        }
        return nullptr;
    }

    // Set a MIB node value from a variable binding and add node to response
    void set(SNMP::Message *response, VarBind *varbind, unsigned int match, unsigned int index, MIB::Setter setter) {
        // Check only variable binding value type, should be OCTET STRING
        if (varbind->getValue()->getType() == SNMP::TYPE_OCTETSTRING) {
            // Call setter with value
            (this->*setter)(static_cast<OctetStringBER*>(varbind->getValue())->getValue());
            // Add MIB object to response
            add(response, match);
        } else {
            // Set error status and index
            response->setError(SNMP::WRONG_TYPE, index + 1);
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
            for (unsigned int index = 0; index < varbindlist->count(); ++index) {
                // Each variable binding is a sequence of 2 objects:
                // - First one is and ObjectIdentifierBER. It holds the OID
                // - Second is the value of any type
                VarBind *varbind = (*varbindlist)[index];
                // There is a convenient function to get the OID as a const char*
                const char *name = varbind->getName();
                unsigned int match = OID::match(name);
                switch (match) {
                case OID::COUNT:
                    // OID is unknown
                    switch (message->getVersion()) {
                    case SNMP::VERSION1:
                        // Set error, status and index
                        response->setError(SNMP::NO_SUCH_NAME, index + 1);
                        break;
                    case SNMP::VERSION2C:
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
            for (unsigned int index = 0; index < varbindlist->count(); ++index) {
                // Each variable binding is a sequence of 2 objects:
                // - First one is and ObjectIdentifierBER. It holds the OID
                // - Second is the value of any type
                VarBind *varbind = (*varbindlist)[index];
                // There is a convenient function to get the OID as a const char*
                const char *name = varbind->getName();
                unsigned int start = OID::start(name);
                switch (start) {
                case OID::LAST:
                    // This is the last OID of the MIB
                    switch (message->getVersion()) {
                    case SNMP::VERSION1:
                        // Set error, status and index
                        response->setError(SNMP::NO_SUCH_NAME, index + 1);
                        // Add OID to response with null value;
                        response->add(name);
                        break;
                    case SNMP::VERSION2C:
                        // End of MIB view
                        response->add(name, new SNMP::EndOfMIBViewBER());
                        break;
                    }
                    break;
                case OID::COUNT:
                    // OID is unknown
                    // Set error, status and index
                    response->setError(SNMP::GEN_ERR, index + 1);
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
            for (unsigned int index = 0; index < varbindlist->count(); ++index) {
                // Each variable binding is a sequence of 2 objects:
                // - First one is and ObjectIdentifierBER. It holds the OID
                // - Second is the value of any type
                VarBind *varbind = (*varbindlist)[index];
                // There is a convenient function to get the OID as a const char*
                const char *name = varbind->getName();
                unsigned int match = OID::match(name);
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
                case OID::COUNT:
                    // OID is unknown
                    // Set error, status and index
                    response->setError(SNMP::GEN_ERR, index + 1);
                    // Add OID to response with null value;
                    response->add(name);
                    break;
                default:
                    // The object can not be set
                    // Set error, status and index
                    response->setError(SNMP::NO_ACCESS, index + 1);
                    // Add OID to response with null value;
                    response->add(name);
                    break;
                }
            }
            return true;
        }
        return false;
    }

    // Variables to hold value of MIB nodes
    const char* _descr = "Nucleo F429ZI Arduino Core STM32 2.2.0";
    const char* _objectID = "";
    char* _contact = nullptr;
    char* _name = nullptr;
    char* _location = nullptr;
};

MIB mib;

// Event handler to process SNMP messages
void onMessage(const SNMP::Message *message, const IPAddress remote, const uint16_t port) {
    // Create an SNMP message for response
    SNMP::Message *response = new SNMP::Message(message->getVersion(),
            message->getCommunity(), SNMP::TYPE_GETRESPONSE);
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
    Serial.begin(115200);
    // Ethernet
    Ethernet.begin(IPAddress(192, 168, 2, 2), IPAddress(255, 255, 255, 0),
            IPAddress(192, 168, 2, 1), IPAddress(192, 168, 2, 1));
    // MIB
    mib.setContact("Patrick Lafarguette");
    mib.setLocation("Home");
    mib.setName("Nucleo F429ZI");
    // SNMP
    snmp.begin(&udp);
    snmp.onMessage(onMessage);
    // Start
    start = millis();
}

void loop() {
    // Agent loop function must be called to process incoming messages
    snmp.loop();
    // Send traps every second
    if (millis() - start  >= 1000) {
        start = millis();
        // Create a trap message and send it
        SNMP::Message *message = mib.trap(SNMP::TYPE_TRAP);
        snmp.send(message, IPAddress(192, 168, 2, 1), SNMP::PORT::TRAP);
        delete message;
        // Create a version 2 trap message and send it
        message = mib.trap(SNMP::TYPE_SNMPV2TRAP);
        snmp.send(message, IPAddress(192, 168, 2, 1), SNMP::PORT::TRAP);
        delete message;
        // Create an inform message and send it
        message = mib.trap(SNMP::TYPE_INFORMREQUEST);
        snmp.send(message, IPAddress(192, 168, 2, 1), SNMP::PORT::TRAP);
        delete message;
    }
}
