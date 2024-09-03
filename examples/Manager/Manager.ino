/*
 Manager

 This sketch implements an SNMP manager to monitor an UPS.

 The UPS-MIB describes the objects available from the device. Every second,
 some values are retrieved and output on serial.

 Tested on a Nucleo F767ZI board with STM32 Arduino core 2.2.0.

 Created 21/03/2022 by Patrick Lafarguette
 */

#if ARDUINO_ARCH_AVR
#include <Ethernet.h> // Ethernet support. Replace if needed.

EthernetUDP udp;
#endif

#if ARDUINO_ARCH_STM32
#include <STM32Ethernet.h> // Ethernet support. Replace if needed.

EthernetUDP udp;
#endif

#if ARDUINO_ARCH_ESP32
#include <ETH.h> // Ethernet support. Replace if needed.

NetworkUDP udp;
#endif

#include <SNMP.h>

SNMP::Manager snmp;

// Use some SNMP classes
using SNMP::IntegerBER;
using SNMP::NullBER;
using SNMP::VarBind;
using SNMP::VarBindList;

// This class encapsulates the UPS SNMP interface
class UPS {
public:
    enum {
        OUTPUT_SOURCE_OTHER = 1,
        OUTPUT_SOURCE_NONE,
        OUTPUT_SOURCE_NORMAL,
        OUTPUT_SOURCE_BYPASS,
        OUTPUT_SOURCE_BATTERY,
        OUTPUT_SOURCE_BOOSTER,
        OUTPUT_SOURCE_REDUCER
    };

    enum {
        BATTERY_STATUS_UNKNOWN = 1,
        BATTERY_STATUS_NORMAL,
        BATTERY_STATUS_LOW,
        BATTERY_STATUS_DEPLETED
    };

    // Simple helper class to handle OIDs
    class OID {
    public:
        enum {
            OUTPUTSOURCE,
            BATTERYSTATUS,
            ESTIMATEDMINUTESREMAINING,
            ESTIMATEDCHARGEREMAINING,
            UNKNOWN,
            COUNT = UNKNOWN,
        };

        static inline const char *NAMES[] = {
                "1.3.6.1.2.1.33.1.4.1.0",
                "1.3.6.1.2.1.33.1.2.1.0",
                "1.3.6.1.2.1.33.1.2.3.0",
                "1.3.6.1.2.1.33.1.2.4.0",
        };

        // Returns index of OID equals to name
        // Returns UNKNOWN if none
       static unsigned int match(const char *name) {
            for (unsigned int index = 0; index < COUNT; ++index) {
                if (strcmp(NAMES[index], name) == 0) {
                    return index;
                }
            }
            return UNKNOWN;
        }
    };

    // Create an SNMP GETREQUEST message
    SNMP::Message* read() {
        SNMP::Message *message = new SNMP::Message(SNMP::Version::V2C, "public", SNMP::Type::GetRequest);
        // Loop for all objects to request
        for (unsigned int index = 0; index < OID::COUNT; ++index) {
            // In GETREQUEST, values are always of type NULL.
            message->add(OID::NAMES[index], new NullBER);
        }
        return message;
    }

    // Parse incoming message
    bool message(const SNMP::Message *message) {
        unsigned int found = 0;
        unsigned int index = 0;
        // Get the variable binding list from the message.
        VarBindList *varbindlist = message->getVarBindList();
        for (unsigned int index = 0; index < varbindlist->count(); ++index) {
            // Each variable binding is a sequence of 2 objects:
            // - First one is and ObjectIdentifierBER. It holds the OID
            // - Second is the value of any type
            VarBind *varbind = (*varbindlist)[index];
            // There is a convenient function to get the OID as a const char*
            const char *name = varbind->getName();
            switch (OID::match(name)) {
            case OID::OUTPUTSOURCE:
                // This object is an integer so first cast the variable value as IntegerBER,
                // then get the object value.
                _outputSource = static_cast<IntegerBER*>(varbind->getValue())->getValue();
                // Increments count of found objects
                found++;
                break;
            case OID::BATTERYSTATUS:
                _batteryStatus = static_cast<IntegerBER*>(varbind->getValue())->getValue();
                found++;
                break;
            case OID::ESTIMATEDMINUTESREMAINING:
                _estimatedMinutesRemaining = static_cast<IntegerBER*>(varbind->getValue())->getValue();
                found++;
                break;
            case OID::ESTIMATEDCHARGEREMAINING:
                _estimatedChargeRemaining = static_cast<IntegerBER*>(varbind->getValue())->getValue();
                found++;
                break;
            }
        }
        // Return true if nodes found, that means this is a valid response from UPS
        return found;
    }

    // Getters
    uint8_t getOutputSource() const {
        return _outputSource;
    }

    uint8_t getBatteryStatus() const {
        return _batteryStatus;
    }

    uint8_t getEstimatedMinutesRemaining() const {
        return _estimatedMinutesRemaining;
    }

    uint8_t getEstimatedChargeRemaining() const {
        return _estimatedChargeRemaining;
    }

private:
    uint8_t _outputSource = 0;
    uint8_t _batteryStatus = 0;
    uint8_t _estimatedMinutesRemaining = 0;
    uint8_t _estimatedChargeRemaining = 0;
};

UPS ups;

// Event handler to process SNMP messages
void onMessage(const SNMP::Message *message, const IPAddress remote, const uint16_t port) {
    const char *OUTPUTSOURCE[] = { "OTHER", "NONE", "NORMAL", "BYPASS",
            "BATTERY", "BOOSTER", "REDUCER" };
    const char *BATTERYSTATUS[] = { "UNKNOWN", "NORMAL", "LOW", "DEPLETED" };
    // Check if message is from UPS
    if (ups.message(message)) {
        Serial.println();
        Serial.print("UPS output source ");
        Serial.println(OUTPUTSOURCE[ups.getOutputSource() - 1]);
        Serial.print("UPS battery status ");
        Serial.println(BATTERYSTATUS[ups.getBatteryStatus() - 1]);
        Serial.print("UPS estimated minutes remaining ");
        Serial.print(ups.getEstimatedMinutesRemaining());
        Serial.println(" minutes");
        Serial.print("UPS estimated charge remaining ");
        Serial.print(ups.getEstimatedChargeRemaining());
        Serial.println(" %");
    }
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
    // SNMP
    snmp.begin(udp);
    snmp.onMessage(onMessage);
    // Start
    start = millis();
}

void loop() {
    // Manager loop function must be called to process incoming messages
    snmp.loop();
    // Send a request every second
    if (millis() - start >= 1000) {
        start = millis();
        // Create message to query UPS and send it
        SNMP::Message *message = ups.read();
        snmp.send(message, IPAddress(192, 168, 2, 1), SNMP::Port::SNMP);
        delete message;
    }
}
