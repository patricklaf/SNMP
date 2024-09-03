# SNMP

[TOC]

[![GitHub release](https://img.shields.io/github/v/release/patricklaf/SNMP)](https://github.com/patricklaf/SNMP/releases/latest)
[![GitHub issues](https://img.shields.io/github/issues/patricklaf/SNMP)](https://github.com/patricklaf/SNMP/issues)


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
  - GETBULKREQUEST
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

- Adafruit Huzzah32 and Olimex ESP32-PoE (ESP32)
- Arduino Uno and Arduino Mega 2560 (AVR)
- NUCLEO-F429ZI and NUCLEO-F767ZI (STM32)

## Configuration

Some conditional defines are used to set options at compile time.

- *SNMP_STREAM*
<br/>If set to 1, packets are parsed and built with stream functions, with low memory usage.
<br/>If set to 0 or undefined, a buffer in RAM is allocated to read or write the entire packet, with high memory requirement.
<br/>It **must** be set to 1 for Arduino Uno board. The default is 1.
- *SNMP_VECTOR*
<br/>If set to 1, vectors are used.
<br/>If set to 0 or undefined, fixed size  arrays are used.
<br/>As vectors are part of the STL, if you want to use them with Arduino Mega board the [ArduinoSTL](https://www.arduino.cc/reference/en/libraries/arduinostl/) library is required.
<br/>It **must not** be used with Arduino Uno board. The default is 0.
- *SNMP_CAPACITY*
<br/>If arrays are used, this symbol defines the maximum number of items contained in a Sequence object.
<br/>The default is 6.

A convenient way to configure the library is to use an optional *SNMPcfg.h* file at sketch level.
The library will include it automatically and apply the configuration. This is an example of such a file.

```cpp
#ifndef SNMPCFG_H_
#define SNMPCFG_H_

#define SNMP_STREAM 0 // Buffers are used.

#define SNMP_VECTOR 1 // Vectors are used.

#define SNMP_CAPACITY 6 // Ignored, as vectors are used.

#endif /* SNMPCFG_H_ */
```

## Usage

There is only one header to include to use the library.

```cpp
#include <SNMP.h>
```

### Agent

An SNMP agent receives request from and sends response to an SNMP manager.

Declare an SNMP agent like this. An UDP client is also needed.

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
    snmp.begin(udp);
    snmp.onMessage(onMessage);
}
```

The user function *onMessage()* will be called every time an SNMP message is received.

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
```

In Arduino *loop()* function, the agent *loop()* function must be called frequently.

```cpp
void loop() {
    // Agent loop function must be called to process incoming messages
    snmp.loop();
}
```

[Agent.ino](https://github.com/patricklaf/SNMP/blob/master/examples/Agent/Agent.ino) is a complete example of an SNMP agent implementation.

### Manager

An SNMP manager sends request to and receives response from an SNMP agent.

Declare an SNMP manager like this. An UDP client is also needed.

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
    snmp.begin(udp);
    snmp.onMessage(onMessage);
}
```

The user function *onMessage()* will be called every time an SNMP message is received. 

In Arduino *loop()* function, the manager *loop()* function must be called frequently.

```cpp
void loop() {
    // Manager loop function must be called to process incoming messages
    snmp.loop();
}
```

[Manager.ino](https://github.com/patricklaf/SNMP/blob/master/examples/Manager/Manager.ino) is a complete example of an SNMP manager implementation.

[MPOD.ino](https://github.com/patricklaf/SNMP/blob/master/examples/MPOD/MPOD.ino) is another example of an SNMP manager implementation with use of SETREQUEST.

[Advanced.ino](https://github.com/patricklaf/SNMP/blob/master/examples/Advanced/Advanced.ino) is the more complete example.
It shows how to write an SNMP agent able to handle GETREQUEST, GETNEXTREQUEST, GETBULKREQUEST and SETREQUEST and to generate 
TRAP, INFORMREQUEST and SNMPV2TRAP.

It uses PROGMEM to save enough RAM to allow the sketch to be run even on a basic Arduino Uno board.

## Limitations

Limitations depend on library configuration and available RAM.

If STL vectors are used, there is virtually no limit to the size of messages and the number of items in Sequence objects.

The values ​​of OctetString and ObjectIdentifier objects are now dynamically allocated. The size is limited only by the available memory.

To achieve compatibility with low-end boards like Arduino Uno, the code has been stripped down and almost no checking is done.

For example, when creating an OpaqueBER, the library assumes that the embedded BER parameter passed to the constructor is not null. If null, code will crash as soon as embedded BER members or functions are accessed. 

<!-- 
| Version            | .data | .text | .bss | RAM  | Total | Note        |
|--------------------|------:|------:|-----:|-----:|------:|-------------|
| 1.4.1              |   798 | 23206 |  320 | 1118 | 24324 | Not working |
| 2.0.0 ~~no bulk~~  |   572 | 21580 |  308 |  880 | 22460 | Working     |
|                    |   72% |   93% |  96% |  79% |   92% |             |
| 2.0.0              |   572 | 21750 |  308 |  880 | 22630 | Working     |
|                    |   72% |   94% |  96% |  79% |   93% |             |
-->

## Migrate to version 2.0.0

Minor changes are required when migrating existing code to version 2.0.0 of the library.

This is mainly due to the change from C-like enum to C++ syntax.

- All version constants like *VERSION2C* are now defined in *struct Version*.<br/>Replace by *Version::V2C*.
- All error constants like *NO_ERROR* are now defined in *struct Error*.<br/>Replace by *Error::NoError*.
- All trap constants like *COLDSTART* are now defined in *struct Trap*.<br/>Replace by *Trap::ColdStart*.
- All type constants like *TYPE_GETRESPONSE* are now defined in *class Type*.<br/>Replace by *Type::GetResponse*.
- *Class PORT* is replaced by *struct Port*. Port constant *TRAP* is renamed as *Trap*.<br/>Replace *PORT::SNMP* by *Port::SNMP* and *PORT::TRAP* by *Port::Trap*.

```cpp
// Version 1.4.1
SNMP::Message *response = new SNMP::Message(SNMP::VERSION2C, "public", SNMP::TYPE_GETRESPONSE);

// Version 2.0.0
SNMP::Message *response = new SNMP::Message(SNMP::Version::V2C, "public", SNMP::Type::GetResponse);
```

The  API changed for *SNMP::begin()*. The UDP parameter is now a reference and no more a pointer.

```cpp
// Version 1.4.1
snmp.begin(&udp);

// Version 2.0.0
snmp.begin(udp);
```

## Acknowledgements

This library is inspired by [SNMP Manager For ESP8266/ESP32/Arduino (and more)](https://github.com/shortbloke/Arduino_SNMP_Manager) authored by Martin Rowan.
