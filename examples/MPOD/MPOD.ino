/*
 MPOD

 This sketch implements an SNMP manager to monitor an MPOD WIENER crate.

 The WIENER-CRATE-MIB describes the objects available from the device.
 HV channel #0 is setup, then user can switch on or off from serial.

 Tested on a Nucleo F767ZI board with STM32 Arduino core 2.2.0.

 Created 22/03/2022 by Patrick Lafarguette
 */

#if ARDUINO_ARCH_AVR
#include <Ethernet.h> // Ethernet support. Replace if needed.
#endif

#if ARDUINO_ARCH_STM32
#include <STM32Ethernet.h> // Ethernet support. Replace if needed.
#endif

#include <SNMP.h>

EthernetUDP udp;
SNMP::Manager snmp;

// Use some SNMP classes
using SNMP::IntegerBER;
using SNMP::OctetStringBER;
using SNMP::OpaqueBER;
using SNMP::OpaqueFloatBER;
using SNMP::VarBind;
using SNMP::VarBindList;

// This class encapsulates the MPOD SNMP interface
class MPOD {
public:
    // Simple helper class to handle OIDs
    class OID {
    public:
        enum {
            OUTPUTSTATUS,
            OUTPUTMEASUREMENTSENSEVOLTAGE,
            OUTPUTMEASUREMENTCURRENT,
            OUTPUTSWITCH,
            OUTPUTVOLTAGE,
            OUTPUTCURRENT,
            OUTPUTVOLTAGERISERATE,
            UNKNOWN,
            COUNT = UNKNOWN,
        };

        static inline const char *NAMES[] = {
                "1.3.6.1.4.1.19947.1.3.2.1.4.1",
                "1.3.6.1.4.1.19947.1.3.2.1.5.1",
                "1.3.6.1.4.1.19947.1.3.2.1.7.1",
                "1.3.6.1.4.1.19947.1.3.2.1.9.1",
                "1.3.6.1.4.1.19947.1.3.2.1.10.1",
                "1.3.6.1.4.1.19947.1.3.2.1.12.1",
                "1.3.6.1.4.1.19947.1.3.2.1.13.1",
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

    // Create an SNMP SETREQUEST message to setup MPOD
    SNMP::Message* setup() {
        // Use read/write community, not read-only
        SNMP::Message* message = new SNMP::Message(SNMP::VERSION2C, "guru", SNMP::TYPE_SETREQUEST);
        // In SETREQUEST, use node type and set the value.
        // OUTPUT SWITCH, integer type, 0 is OFF and 1 is ON.
        message->add(OID::NAMES[OID::OUTPUTSWITCH], new IntegerBER(0));
        // OUTPUT VOLTAGE is an opaque float embedded in an opaque node
        message->add(OID::NAMES[OID::OUTPUTVOLTAGE], new OpaqueBER(new OpaqueFloatBER(10.0)));
        // OUTPUT CURRENT is an opaque float embedded in an opaque node
        message->add(OID::NAMES[OID::OUTPUTCURRENT], new OpaqueBER(new OpaqueFloatBER(0.001)));
        // OUTPUT VOLTAGE RISE RATE is negative. Don't ask me why...
        message->add(OID::NAMES[OID::OUTPUTVOLTAGERISERATE], new OpaqueBER(new OpaqueFloatBER(-1.0)));
        return message;
    }

    // Create an SNMP GETREQUEST message
    SNMP::Message* read() {
        SNMP::Message* message = new SNMP::Message(SNMP::VERSION2C, "public", SNMP::TYPE_GETREQUEST);
        // In GETREQUEST, values are always of type NULL.
        message->add(OID::NAMES[OID::OUTPUTSTATUS]);
        message->add(OID::NAMES[OID::OUTPUTMEASUREMENTSENSEVOLTAGE]);
        message->add(OID::NAMES[OID::OUTPUTMEASUREMENTCURRENT]);
        return message;
    }

    // Create an SNMP SETREQUEST message to switch on or off the MPOD
    SNMP::Message* output(const bool on) {
        SNMP::Message* message = new SNMP::Message(SNMP::VERSION2C, "guru", SNMP::TYPE_SETREQUEST);
        // In SETREQUEST, use node type and set the value.
        // OUTPUT SWITCH, integer type, 0 is OFF and 1 is ON.
        message->add(OID::NAMES[OID::OUTPUTSWITCH], new IntegerBER(on ? 1 : 0));
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
            case OID::OUTPUTSTATUS: {
                // OUTPUTSTATUS is defined in MIB as BITS but encoded as OCTETSTRING by MPOD
                OctetStringBER *status = static_cast<OctetStringBER*>(varbind->getValue());
                _on = status->getBit(0);
                _up = status->getBit(11);
                _down = status->getBit(12);
            }
                found++;
                break;
            case OID::OUTPUTMEASUREMENTSENSEVOLTAGE:
                // Use private helper function to extract float value
                _measurementSenseVoltage = getFloatFromVarBind(varbind);
                found++;
                break;
            case OID::OUTPUTMEASUREMENTCURRENT:
                _measurementCurrent = getFloatFromVarBind(varbind);
                found++;
                break;
            case OID::OUTPUTSWITCH:
                // Use private helper function to extract integer value
                _on = getIntegerFromVarBind(varbind);
                found++;
                break;
            case OID::OUTPUTVOLTAGE:
                _voltage = getFloatFromVarBind(varbind);
                found++;
                break;
            case OID::OUTPUTCURRENT:
                _current = getFloatFromVarBind(varbind);
                found++;
                break;
            case OID::OUTPUTVOLTAGERISERATE:
                _voltageRiseRate = getFloatFromVarBind(varbind);
                found++;
                break;
            }
        }
        // Return true if nodes found, that means this is a valid response from MPOD
        return found;
    }

    // Getters
    bool isOn() const {
        return _on;
    }

    bool isUp() const {
        return _up;
    }

    bool isDown() const {
        return _down;
    }

    float getMeasurementSenseVoltage() const {
        return _measurementSenseVoltage;
    }

    float getMeasurementCurrent() const {
        return _measurementCurrent;
    }

    float getVoltage() const {
        return _voltage;
    }

    float getCurrent() const {
        return _current;
    }

    float getVoltageRiseRate() const {
        return _voltageRiseRate;
    }

private:
    // Use appropriate cast to get integer value
    unsigned int getIntegerFromVarBind(const VarBind *varbind) {
        return static_cast<IntegerBER*>(varbind->getValue())->getValue();
    }

    // Use appropriate casts to get embedded opaque float value
    float getFloatFromVarBind(const VarBind *varbind) {
        return static_cast<OpaqueFloatBER*>(static_cast<OpaqueBER*>(varbind->getValue())->getBER())->getValue();
    }

    bool _on = false;
    bool _up = false;
    bool _down = false;
    float _measurementSenseVoltage = 0;
    float _measurementCurrent = 0;
    float _voltage = 0;
    float _current = 0;
    float _voltageRiseRate = 0;
};

MPOD mpod;

// Event handler to process SNMP messages
void onMessage(const SNMP::Message *message, const IPAddress remote, const uint16_t port) {
    // Check if message is from MPOD
    if (mpod.message(message)) {
        Serial.println();
        Serial.print("MPOD status");
        Serial.print(mpod.isOn() ? " on" : " off");
        if (mpod.isUp()) {
            Serial.print(" up");
        }
        if (mpod.isDown()) {
            Serial.print(" down");
        }
        Serial.println();
        Serial.print("HV voltage ");
        Serial.print(mpod.getMeasurementSenseVoltage());
        Serial.print(" V (");
        Serial.print(mpod.getVoltage());
        Serial.print(") current ");
        Serial.print(mpod.getMeasurementCurrent());
        Serial.print(" A (");
        Serial.print(mpod.getCurrent());
        Serial.print(") rise rate ");
        Serial.print(mpod.getVoltageRiseRate());
        Serial.println(" V/s");
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
    // MPOD
    SNMP::Message *message = mpod.setup();
    snmp.send(message, IPAddress(192, 168, 2, 3), SNMP::PORT::SNMP);
    delete message;
}

enum {
    NONE,
    ON,
    OFF,
};

void loop() {
    // Manager loop function must be called to process incoming messages
    snmp.loop();
    // Serial
    if (Serial.available()) {
        uint8_t output = NONE;
        // Read command from serial
        String string = Serial.readString();
        string.toLowerCase();
        // Only two commands
        if (string == "on") {
            output = ON;
        } else if (string == "off") {
            output = OFF;
        }
        if (output != NONE) {
            // If ON or OFF, send SETREQUEST to MPOD
            SNMP::Message *message = mpod.output(output == ON);
            snmp.send(message, IPAddress(192, 168, 2, 3), SNMP::PORT::SNMP);
            delete message;
        }
    }
    // Send a request every second
    if (millis() - start  >= 1000) {
        start = millis();
        // Create message to query MPOD and send it
        SNMP::Message* message = mpod.read();
        snmp.send(message, IPAddress(192, 168, 2, 3), SNMP::PORT::SNMP);
        delete message;
    }
}
