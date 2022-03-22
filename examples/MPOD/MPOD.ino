/*
 MPOD

 This sketch implements an SNMP manager to monitor an MPOD WIENER crate.

 The WIENER-CRATE-MIB describes the objects available from the device.
 HV channel #0 is setup, then user can switch on or off from serial.

 Tested on a Nucleo F767ZI board with STM32 Arduino core 2.2.0.

 Created 22/03/2022 by Patrick Lafarguette
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
using SNMP::OpaqueBER;
using SNMP::OpaqueFloatBER;
using SNMP::OctetStringBER;

// This class encapsulates the MPOD SNMP interface
class MPOD {
public:
    // Simple helper class to handle OID
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
            COUNT,
        };

        static inline const char *STRING[] = {
                "1.3.6.1.4.1.19947.1.3.2.1.4.1",
                "1.3.6.1.4.1.19947.1.3.2.1.5.1",
                "1.3.6.1.4.1.19947.1.3.2.1.7.1",
                "1.3.6.1.4.1.19947.1.3.2.1.9.1",
                "1.3.6.1.4.1.19947.1.3.2.1.10.1",
                "1.3.6.1.4.1.19947.1.3.2.1.12.1",
                "1.3.6.1.4.1.19947.1.3.2.1.13.1",
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

    // Create an SNMP SETREQUEST message to setup MPOD
    SNMP::Message* setup() {
        // Use read/write community, not read-only
        SNMP::Message* message = new SNMP::Message(SNMP::VERSION2C, "guru", SNMP::TYPE_SETREQUEST);
        // OUTPUT SWITCH
        // Create a variable, an ASN.1 sequence
        SequenceBER* outputswitch = new SequenceBER();
        // Add the OID
        outputswitch->add(new ObjectIdentifierBER(OID::STRING[OID::OUTPUTSWITCH]));
        // Add the value. In a SETREQUEST, use object type and set the value.
        // For OUTPUT SWITCH, INTEGER type, 0 is OFF and 1 is ON. 
        outputswitch->add(new IntegerBER(0));
        // And finally add the variable to the message
        message->add(outputswitch);
        // OUTPUT VOLTAGE
        SequenceBER* outputvoltage = new SequenceBER();
        outputvoltage->add(new ObjectIdentifierBER(OID::STRING[OID::OUTPUTVOLTAGE]));
        // OUTPUT VOLTAGE is an opaque float embedded in an opaque object.
        outputvoltage->add(new OpaqueBER(new OpaqueFloatBER(10.0)));
        message->add(outputvoltage);
        // OUTPUT CURRENT
        SequenceBER* outputcurrent = new SequenceBER();
        outputcurrent->add(new ObjectIdentifierBER(OID::STRING[OID::OUTPUTCURRENT]));
        outputcurrent->add(new OpaqueBER(new OpaqueFloatBER(0.001)));
        message->add(outputcurrent);
        // OUTPUT VOLTAGE RISE RATE
        SequenceBER* outputvoltageriserate = new SequenceBER();
        outputvoltageriserate->add(new ObjectIdentifierBER(OID::STRING[OID::OUTPUTVOLTAGERISERATE]));
        // OUTPUT VOLTAGE RISE RATE is negative. Don't ask me why...
        outputvoltageriserate->add(new OpaqueBER(new OpaqueFloatBER(-1.0)));
        message->add(outputvoltageriserate);
        return message;
    }

    // Create an SNMP GETREQUEST message
    SNMP::Message* read() {
        SNMP::Message* message = new SNMP::Message(SNMP::VERSION2C, "public", SNMP::TYPE_GETREQUEST);
        // OUTPUT STATUS
        SequenceBER* outputstatus = new SequenceBER();
        outputstatus->add(new ObjectIdentifierBER(OID::STRING[OID::OUTPUTSTATUS]));
        outputstatus->add(new NullBER);
        message->add(outputstatus);
        // OUTPUT MEASUREMENT SENSE VOLTAGE
        SequenceBER* outputmeasurementsensevoltage = new SequenceBER();
        outputmeasurementsensevoltage->add(new ObjectIdentifierBER(OID::STRING[OID::OUTPUTMEASUREMENTSENSEVOLTAGE]));
        outputmeasurementsensevoltage->add(new NullBER);
        message->add(outputmeasurementsensevoltage);
        // OUTPUT MEASUREMENT CURRENT
        SequenceBER* outputmeasurementcurrent = new SequenceBER();
        outputmeasurementcurrent->add(new ObjectIdentifierBER(OID::STRING[OID::OUTPUTMEASUREMENTCURRENT]));
        outputmeasurementcurrent->add(new NullBER);
        message->add(outputmeasurementcurrent);
        return message;
    }

    // Create an SNMP SETREQUEST message to switch on or off the MPOD
    SNMP::Message* output(const bool on) {
        SNMP::Message* message = new SNMP::Message(SNMP::VERSION2C, "guru", SNMP::TYPE_SETREQUEST);
        // OUTPUT SWITCH
        SequenceBER* outputswitch = new SequenceBER();
        outputswitch->add(new ObjectIdentifierBER(OID::STRING[OID::OUTPUTSWITCH]));
        outputswitch->add(new IntegerBER(on ? 1 : 0));
        message->add(outputswitch);
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
            case OID::OUTPUTSTATUS: {
                // OUTPUTSTATUS is defined in MIB as BITS but encoded as OCTETSTRING by MPOD
                OctetStringBER *status = (OctetStringBER*) variable->getValue();
                _on = status->getBit(0);
                _up = status->getBit(11);
                _down = status->getBit(12);
            }
                found++;
                break;
            case OID::OUTPUTMEASUREMENTSENSEVOLTAGE:
                // Use appropiate casts to get the final embeded opaque float value
                _measurementSenseVoltage = ((OpaqueFloatBER*)((OpaqueBER*) variable->getValue())->getBER())->getValue();
                found++;
                break;
            case OID::OUTPUTMEASUREMENTCURRENT:
                _measurementCurrent = ((OpaqueFloatBER*)((OpaqueBER*) variable->getValue())->getBER())->getValue();
                found++;
                break;
            case OID::OUTPUTSWITCH:
                _on = ((IntegerBER*) variable->getValue())->getValue();
                found++;
                break;
            case OID::OUTPUTVOLTAGE:
                _voltage = ((OpaqueFloatBER*)((OpaqueBER*) variable->getValue())->getBER())->getValue();
                found++;
                break;
            case OID::OUTPUTCURRENT:
                _current = ((OpaqueFloatBER*)((OpaqueBER*) variable->getValue())->getBER())->getValue();
                found++;
                break;
            case OID::OUTPUTVOLTAGERISERATE:
                _voltageRiseRate = ((OpaqueFloatBER*)((OpaqueBER*) variable->getValue())->getBER())->getValue();
                found++;
                break;
            }
        }
        return found;
    }

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
    bool _on = false;
    bool _up= false;
    bool _down = false;
    float _measurementSenseVoltage = 0;
    float _measurementCurrent = 0;
    float _voltage = 0;
    float _current = 0;
    float _voltageRiseRate = 0;
};

MPOD mpod;

// Event handler to process SNMP messages
void onMessage(const SNMP::Message *message, const IPAddress from,
        const uint16_t port) {
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
    snmp.send(message, IPAddress(192, 168, 2, 3), PORT_SNMP);
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
            snmp.send(message, IPAddress(192, 168, 2, 3), PORT_SNMP);
            delete message;
        }
    }
    // Send a request every second
    if (millis() - start  >= 1000) {
        start = millis();
        // Create message to query MPOD and send it
        SNMP::Message* message = mpod.read();
        snmp.send(message, IPAddress(192, 168, 2, 3), PORT_SNMP);
        delete message;
    }
}
