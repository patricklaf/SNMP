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

#include <STM32Ethernet.h> // Ethernet support. Replace if needed.
#include <SNMP.h>

EthernetUDP udp;
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
const char *SYSNAME_VALUE = "Nucleo F767ZI";

// Use some SNMP classes
using SNMP::SequenceBER;
using SNMP::ObjectIdentifierBER;
using SNMP::OctetStringBER;

// Event handler to process SNMP messages
void onMessage(const SNMP::Message *message, const IPAddress from, const uint16_t port) {
    bool send = false;
    // Get the variables from the message.
    // This is an ASN.1 sequence. See BER.h and BER.cpp.
    SequenceBER *variables = message->getVariables();
    for (unsigned int index = 0; index < variables->count(); ++index) {
        // Each variable is also a sequence of 2 objects:
        // - First one is and ObjectIdentifierBER. It holds the OID
        // - Second is the value of any type, not used here
        SequenceBER *variable = (SequenceBER*) (*variables)[index];
        // There is a convenient function to get the OID as a const char*
        const char *oid = variable->getOID();
        if (strcmp(SYSNAME_OID, oid) == 0) {
            // System name is requested. We have to send a response.
            send = true;
        }
    }
    if (send) {
        // The response must have the same request-id as the request
        sendResponse(from, port, message->getRequestID());
    }
}

// Send an SNMP response
void sendResponse(const IPAddress to, const uint16_t port, const unsigned int requestID) {
    // Create an SNMP message
    SNMP::Message *message = new SNMP::Message(SNMP::VERSION2C, "public", SNMP::TYPE_GETRESPONSE);
    // Set request-id as the request
    message->setRequestID(requestID);
    // SYSNAME
    // Create a sequence to hold the variable
    SequenceBER *sysname = new SequenceBER();
    // Add the OID
    sysname->add(new ObjectIdentifierBER(SYSNAME_OID));
    // Add the system name value. An octet string is not null terminated, so we need to explicitly
    // set the length
    sysname->add(new OctetStringBER(SYSNAME_VALUE, strlen(SYSNAME_VALUE)));
    message->add(sysname);
    // Send the response to remote IP and port
    snmp.send(message, to, port);
    delete message;
}

void setup() {
    Serial.begin(115200);
    // Ethernet
    Ethernet.begin(IPAddress(192, 168, 2, 2), IPAddress(255, 255, 255, 0),
            IPAddress(192, 168, 2, 1), IPAddress(192, 168, 2, 1));
    // SNMP
    snmp.begin(&udp);
    snmp.onMessage(onMessage);
}

void loop() {
    // Agent loop function must be called to process incoming messages
    snmp.loop();
}
