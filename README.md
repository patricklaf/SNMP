# SNMP

Simple Network Managment Protocol library to write agent or manager for Arduino boards.

The library supports:

- SNMP protocol version
  - v1
  - v2c

- SNMP messages
  - GETREQUEST
  - SETREQUEST
  - GETRESPONSE

- SNMP objects
  - Boolean
  - Integer
  - OctetString
  - Null
  - ObjectIdentifier
  - Sequence
  - Counter32
  - Gauge32
  - TimeTicks
  - Opaque
  - Float
  - OpaqueFloat
 
## Usage

There is only one header to include to use the library.

```cpp
#include <SNMP.h>
```

### Agent

An SNMP agent receive request from and send response to an SNMP manager.

Declare an SNMP agent like this. An UDP variable is also needed.

```cpp
EthernetUDP udp; // STM32, use your own platform UDP class
SNMP::Agent snmp;
```

In Arduino *setup()* function, add code to setup agent.

```cpp
void setup() {
    Serial.begin(115200);
    // Ethernet
    Ethernet.begin(IPAddress(192, 168, 2, 2), IPAddress(255, 255, 255, 0),
            IPAddress(192, 168, 2, 1), IPAddress(192, 168, 2, 1));
    // SNMP
    snmp.begin(&udp);
    snmp.onMessage(onMessage);
}
```

The function *onMessage()* will be called everytime an SNMP message is received. 

```cpp
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
```

The function *sendResponse()*, used in *onMessage()* show how to create and send an SNMP message.

```cpp
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
```

In Arduino *loop()* function, the agent *loop()* function must be called frequently.

```cpp
void loop() {
    // Agent loop function must be called to process incoming messages
    snmp.loop();
}
```

[Agent.ino](examples/Agent/Agent.ino) is a complete exemple of an SNMP agent implementation.

### Manager

An SNMP manager send request to and receive response from an SNMP agent.

Declare an SNMP manager like this. An UDP variable is also needed.

```cpp
EthernetUDP udp; // STM32, use your own platform UDP class
SNMP::Manager snmp;
```

In Arduino *setup()* function, add code to setup manager.

```cpp
void setup() {
    Serial.begin(115200);
    // Ethernet
    Ethernet.begin(IPAddress(192, 168, 2, 2), IPAddress(255, 255, 255, 0),
            IPAddress(192, 168, 2, 1), IPAddress(192, 168, 2, 1));
    // SNMP
    snmp.begin(&udp);
    snmp.onMessage(onMessage);
}
```

The function *onMessage()* will be called everytime an SNMP message is received. 

In Arduino *loop()* function, the manager *loop()* function must be called frequently.

```cpp
void loop() {
    // Manager loop function must be called to process incoming messages
    snmp.loop();
}
```

[Manager.ino](examples/Manager/Manager.ino) is a complete exemple of an SNMP manager implementation.

[MPOD.ino](examples/MPOD/MPOD.ino) is another exemple of an SNMP manager implementation with use of SETREQUEST.

## Limitations
- Message size is limited to 512 bytes
- SequenceBER is limited to 16 objects, so 8 variables (OID and value)
- OctetString is limited to 128 octets
- ObjectIdentifier is limited to 63 chars null-terminated string

These limitations should be removed in future releases.

## TODO
- Use dynamic allocation for buffers (Message, OctetString and ObjectIdntifier)
- Use vector for dynamic arrays (SequenceBER)
- Add traps support

## Acknowledgements
This library is inspired by [SNMP Manager For ESP8266/ESP32/Arduino (and more)](https://github.com/shortbloke/Arduino_SNMP_Manager) authored by Martin Rowan.