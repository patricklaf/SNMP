/*
 Agent

 This sketch implements an SNMP agent.

 This is a very basic example, only one OID is handled. The agent will
 accept request for SNMPv2-MIB::sysName.0 declared in SNMPv2-MIB. This
 object holds the name of the system.

 The agent can be tested with this command from a Linux command line:
 snmpget -v 2c -c public 192.168.2.2 sysName.0

 If everything is OK, the command displays:
 SNMPv2-MIB::sysName.0 = STRING: Nucleo F767ZI

 Tested on a Nucleo F767ZI board with STM32 Arduino core 2.2.0.

 Created 19/03/2022 by Patrick Lafarguette
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

// SNMPv2-MIB
//
// SNMPv2-MIB::sysName.0
// .iso.org.dod.internet.mgmt.mib-2.system.sysName.0
//
// This is the command to query this object from local agent:
// snmpget -v 2c -c public localhost sysName.0
// SNMPv2-MIB::sysName.0 = STRING: localhost.localdomain/
//
// As the library doesn't handle MIB parsing, we have to get the parsed OID
// with -On switch:
// snmpget -v 2c -c public -On localhost sysName.0
// .1.3.6.1.2.1.1.5.0 = STRING: localhost.localdomain

const char *SYSNAME_OID = "1.3.6.1.2.1.1.5.0";
const char *SYSNAME_VALUE = BOARD; // Name of the board.

// Use some SNMP classes
using SNMP::OctetStringBER;
using SNMP::VarBind;
using SNMP::VarBindList;

// Event handler to process SNMP messages
void onMessage(const SNMP::Message *message, const IPAddress remote, const uint16_t port) {
    // Get the variable binding list from the message.
    VarBindList *varbindlist = message->getVarBindList();
    for (uint8_t index = 0; index < varbindlist->count(); ++index) {
        // Each variable binding is a sequence of 2 objects:
        // - First one is and ObjectIdentifierBER. It holds the OID
        // - Second is the value of any type
        VarBind *varbind = (*varbindlist)[index];
        // There is a convenient function to get the OID as a const char*
        const char *name = varbind->getName();
        if (strcmp(SYSNAME_OID, name) == 0) {
            // System name is requested. We have to send a response.
            // Create an SNMP message for response
            SNMP::Message *response = new SNMP::Message(SNMP::Version::V2C, "public", SNMP::Type::GetResponse);
            // The response must have the same request-id as the request
            response->setRequestID(message->getRequestID());
            // SYSNAME
            // Create an OctetStringBER to hold the variable binding value
            OctetStringBER* value = new OctetStringBER(SYSNAME_VALUE);
            // Add the variable binding to the message
            response->add(SYSNAME_OID, value);
            // Send the response to remote IP and port
            snmp.send(response, remote, port);
            // Avoid memory leak
            delete response;
       }
    }
}

void setup() {
    // Serial
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
    // SNMP
    snmp.begin(udp);
    snmp.onMessage(onMessage);
}

void loop() {
    // Agent loop function must be called to process incoming messages
    snmp.loop();
}
