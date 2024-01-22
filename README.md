# SNMP

[![GitHub release](https://img.shields.io/github/v/release/patricklaf/SNMP)](https://github.com/patricklaf/SNMP/releases/latest)

Simple Network Management Protocol library to write agent or manager for Arduino boards.

The library supports:

- SNMP protocol version
  - v1
  - v2c

- SNMP messages
  - GETREQUEST
  - GETNEXTREQUEST
  - GETRESPONSE
  - SETREQUEST
  - TRAP
  - INFORMREQUEST
  - SNMPV2TRAP

- SNMP objects
  - Boolean
  - Integer
  - OctetString
  - Null
  - ObjectIdentifier
  - Sequence
  - IPAddress
  - Counter32
  - Gauge32
  - TimeTicks
  - Opaque
  - Counter64
  - Float
  - OpaqueFloat
 
## Compatibility

The library is developed and tested on several boards and architectures:

- Adafruit Huzzah32 (ESP32).
- Arduino Mega 2560 (AVR).
- NUCLEO-F429ZI and NUCLEO-F767ZI (STM32).

## Usage

There is only one header to include to use the library.

```cpp
#include <SNMP.h>
```

### Agent

An SNMP agent receives request from and sends response to an SNMP manager.

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

The function *onMessage()* will be called every time an SNMP message is received.

```cpp
void onMessage(const SNMP::Message *message, const IPAddress remote, const uint16_t port) {
    // Get the variable binding list from the message.
    VarBindList *varbindlist = message->getVarBindList();
    for (unsigned int index = 0; index < varbindlist->count(); ++index) {
        // Each variable binding is a sequence of 2 objects:
        // - First one is and ObjectIdentifierBER. It holds the OID
        // - Second is the value of any type
        VarBind *varbind = (*varbindlist)[index];
        // There is a convenient function to get the OID as a const char*
        const char *name = varbind->getName();
        if (strcmp(SYSNAME_OID, name) == 0) {
            // System name is requested. Need to send a response.
            // Create an SNMP message for response
            SNMP::Message *response = new SNMP::Message(SNMP::VERSION2C, "public", SNMP::TYPE_GETRESPONSE);
            // The response must have the same request-id as the request
            response->setRequestID(message->getRequestID());
            // SYSNAME
            // Create an OctetStringBER to hold the variable binding value
            OctetStringBER* value = new OctetStringBER(SYSNAME_VALUE, strlen(SYSNAME_VALUE));
            // Add the variable binding to the message
            response->add(SYSNAME_OID, value);
            // Send the response to remote IP and port
            snmp.send(response, remote, port);
            // Avoid memory leak
            delete response; 
       }
    }
}
```

In Arduino *loop()* function, the agent *loop()* function must be called frequently.

```cpp
void loop() {
    // Agent loop function must be called to process incoming messages
    snmp.loop();
}
```

[Agent.ino](examples/Agent/Agent.ino) is a complete example of an SNMP agent implementation.

### Manager

An SNMP manager sends request to and receives response from an SNMP agent.

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

The function *onMessage()* will be called every time an SNMP message is received. 

In Arduino *loop()* function, the manager *loop()* function must be called frequently.

```cpp
void loop() {
    // Manager loop function must be called to process incoming messages
    snmp.loop();
}
```

[Manager.ino](examples/Manager/Manager.ino) is a complete example of an SNMP manager implementation.

[MPOD.ino](examples/MPOD/MPOD.ino) is another example of an SNMP manager implementation with use of SETREQUEST.

[Advanced.ino](examples/Advanced/Advanced.ino) is the more complex example. It shows how to write an SNMP agent able to handle GETREQUEST, GETNEXTREQUEST and SETREQUEST and generate TRAP, INFORMREQUEST and SNMPV2TRAP.

## Limitations
- Message size is limited to 512 bytes
- SequenceBER is limited to 16 objects, so 8 variables (OID and value)
- OctetString is limited to 128 bytes
- ObjectIdentifier is limited to 63 chars null-terminated string

These limitations should be removed in future releases.

## TODO
- Use dynamic allocation for buffers (Message, OctetString and ObjectIdentifier)
- Use vector for dynamic arrays (SequenceBER)

## Acknowledgements
This library is inspired by [SNMP Manager For ESP8266/ESP32/Arduino (and more)](https://github.com/shortbloke/Arduino_SNMP_Manager) authored by Martin Rowan.
