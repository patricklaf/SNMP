/*
 Manager

 This sketch implements an SNMP manager to monitor an UPS.

 The UPS-MIB describes the objects available from the device. Every second,
 some values are retrieved and output on serial.

 Tested on a Nucleo F767ZI board with STM32 Arduino core 2.2.0.

 Created 21/03/2022 by Patrick Lafarguette
 */

#include <STM32Ethernet.h> // Ethernet support. Replace if needed.
#include <SNMP.h>

EthernetUDP udp;
SNMP::Manager snmp;

// Use some SNMP classes
using SNMP::SequenceBER;
using SNMP::ObjectIdentifierBER;
using SNMP::NullBER;
using SNMP::IntegerBER;

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

    // Simple helper class to handle OID
    class OID {
    public:
        enum {
            OUTPUTSOURCE,
            BATTERYSTATUS,
            ESTIMATEDMINUTESREMAINING,
            ESTIMATEDCHARGEREMAINING,
            COUNT,
        };

        static inline const char *STRING[] = {
                "1.3.6.1.2.1.33.1.4.1.0",
                "1.3.6.1.2.1.33.1.2.1.0",
                "1.3.6.1.2.1.33.1.2.3.0",
                "1.3.6.1.2.1.33.1.2.4.0",
        };

        static unsigned int match(const char *oid) {
            for (unsigned int index = 0; index < COUNT; ++index) {
                if (strcmp(STRING[index], oid) == 0) {
                    return index;
                }
            }
            return COUNT;
        }
    };

    // Create a SNMP GETREQUEST message
    SNMP::Message* read() {
        SNMP::Message *message = new SNMP::Message(SNMP::VERSION2C, "public", SNMP::TYPE_GETREQUEST);
        // Loop for all objects to request
        for (unsigned int index; index < OID::COUNT; ++index) {
            // Create a variable, an ASN.1 sequence
            SequenceBER *variable = new SequenceBER();
            // Add the OID
            variable->add(new ObjectIdentifierBER(OID::STRING[index]));
            // Add the value. In a GETREQUEST, values are always of type NULL.
            variable->add(new NullBER);
            // And finally add the variable to the message
            message->add(variable);
        }
        return message;
    }

    // Parse incoming message
    bool message(const SNMP::Message *message) {
        unsigned int found = 0;
        unsigned int index = 0;
        // Get the variables from the message.
        // This is an ASN.1 sequence. See BER.h and BER.cpp.
        SequenceBER *variables = message->getVariables();
        for (unsigned int index = 0; index < variables->count(); ++index) {
            // Each variable is also a sequence of 2 objects:
            // - First one is and ObjectIdentifierBER. It holds the OID
            // - Second is the value of any type
            SequenceBER *variable = (SequenceBER*) (*variables)[index];
            // There is a convenient function to get the OID as a const char*
            const char *oid = variable->getOID();
            switch (OID::match(oid)) {
            case OID::OUTPUTSOURCE:
                // This object is an integer so first cast the variable value as IntegerBER,
                // then get the object value.
                _outputSource = ((IntegerBER*) variable->getValue())->getValue();
                // Increments count of found objects
                found++;
                break;
            case OID::BATTERYSTATUS:
                _batteryStatus = ((IntegerBER*) variable->getValue())->getValue();
                found++;
                break;
            case OID::ESTIMATEDMINUTESREMAINING:
                _estimatedMinutesRemaining = ((IntegerBER*) variable->getValue())->getValue();
                found++;
                break;
            case OID::ESTIMATEDCHARGEREMAINING:
                _estimatedChargeRemaining = ((IntegerBER*) variable->getValue())->getValue();
                found++;
                break;
            }
        }
        // Return true if objects found, that means this is a valid response from UPS
        return found;
    }

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
void onMessage(const SNMP::Message *message, const IPAddress from,
        const uint16_t port) {
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
    Serial.begin(115200);
    // Ethernet
    Ethernet.begin(IPAddress(192, 168, 2, 2), IPAddress(255, 255, 255, 0),
            IPAddress(192, 168, 2, 1), IPAddress(192, 168, 2, 1));
    // SNMP
    snmp.begin(&udp);
    snmp.onMessage(onMessage);
    // Start
    start = millis();
}

void loop() {
    // Manager loop function must be called to process incoming messages
    snmp.loop();
    // Send a request every second
    if (millis() - start  >= 1000) {
        start = millis();
        // Create message to query UPS and send it
        SNMP::Message* message = ups.read();
        snmp.send(message, IPAddress(192, 168, 2, 1), PORT_SNMP);
        delete message;
    }
}
