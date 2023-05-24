#include "BER.h"

namespace SNMP {

//////////
// Type //
//////////

Type::Type(uint32_t type) {
    _size = 1;
    _type = type;
    if (_type > 30) {
        do {
            _type >>= 7;
            _size++;
        } while (_type);
    }
    _type = type;
}

const uint32_t Type::encode(unsigned char *buffer) {
	unsigned char *pointer = buffer;
    if (_type > 30) {
        pointer += _size;
        uint32_t value = _type;
        for (uint32_t index = 0; index < _size; ++index) {
            *--pointer = (value & 0x7F) | (index ? 0x80 : 0x00);
            value >>= 7;
        }
        *pointer = 0x9F;
    } else {
        *pointer++ = _type;
    }
    return _size;
}

const uint32_t Type::decode(unsigned char *buffer) {
	unsigned char *pointer = buffer;
    _type = *pointer++;
    if (_type & 0x1F > 30) {
        _type = 0;
        while (*pointer & 0x80) {
            _type <<= 7;
            _type += (*pointer++) & 0x7F;
        }
        _type += *pointer++;
    }
    _size = pointer - buffer;
    return _size;
}

////////////
// Length //
////////////

Length::Length(uint32_t length) {
    _length = length;
    _size = 1;
    if (length > 127) {
        uint32_t value = length;
        do {
            value >>= 8;
            _size++;
        } while (value);
    }
}

const uint32_t Length::encode(unsigned char *buffer) {
	unsigned char *pointer = buffer;
    uint32_t value = _length;
    if (_length > 127) {
        *pointer = 0x80 | _size;
        pointer += _size;
        for (uint32_t index = 0; index < _size; ++index) {
            *pointer-- = value & 0xFF;
            value >>= 8;
        }
    } else {
        *pointer++ = _length;
    }
    return _size;
}

const uint32_t Length::decode(unsigned char *buffer) {
	unsigned char *pointer = buffer;
    _length = *pointer++;
    if (_length > 127) {
        _size = _length & 0x7F;
        _length = 0;
        for (uint32_t index = 0; index < _size; ++index) {
            _length <<= 8;
            _length += *pointer++;
        }
    }
    _size = pointer - buffer;
    return _size;
}

/////////////
// Boolean //
/////////////

// Fixed length
// 01 01 XX
BooleanBER::BooleanBER(const bool value) :
        BER(TYPE_BOOLEAN) {
    _length = 1;
    _value = value;
    _size = 3;
}

const uint32_t BooleanBER::encode(unsigned char *buffer) {
    unsigned char *pointer = buffer;
    // T, L, V
    *pointer++ = _type;
    *pointer++ = _length;
    *pointer = _value ? 0xFF : 0x00;
    return _size;
}

const uint32_t BooleanBER::decode(unsigned char *buffer) {
	unsigned char *pointer = buffer;
    // T, L, V
    if (*pointer++ == TYPE_BOOLEAN) {
        if (*pointer++ == _length) {
            _value = *pointer++;
            _size = pointer - buffer;
        }
    }
    return _size;
}

/////////////
// Integer //
/////////////

// Variable length
// 02 XX YY YY
IntegerBER::IntegerBER(const int32_t value) :
        BER(TYPE_INTEGER) {
    setValue(value);
}

const uint32_t IntegerBER::encode(unsigned char *buffer) {
    int32_t value = _value;
    unsigned char *pointer = buffer;
    // T, L
    *pointer++ = _type;
    *pointer = _length;
    // V
    value = _value;
    pointer += _length;
    for (uint32_t index = 0; index < _length; ++index) {
        *pointer-- = value & 0xFF;
        value >>= 8;
    }
    return _size;
}

const uint32_t IntegerBER::decode(unsigned char *buffer) {
	unsigned char *pointer = buffer;
    // T, L, V
    if (*pointer++ == _type) {
        _length = *pointer++;
        _value = 0;
        for (uint32_t index = 0; index < _length; ++index) {
            _value <<= 8;
            _value |= static_cast<uint8_t>(*pointer++);
        }
        _size = pointer - buffer;
    }
    return _size;
}

//////////////////
// Octet String //
//////////////////

// Variable length
OctetStringBER::OctetStringBER(const char *value) :
        OctetStringBER(value, strlen(value)) {
}

OctetStringBER::OctetStringBER(const char *value, const uint32_t length) :
        BER(TYPE_OCTETSTRING) {
    setValue(value, length);
}

const uint32_t OctetStringBER::encode(unsigned char *buffer) {
	unsigned char *pointer = buffer;
    // T
    *pointer++ = _type;
    // L
    Length __length(_length);
    pointer += __length.encode(pointer);
    // V
    memcpy(pointer, _value, _length);
    return _size;
}

const uint32_t OctetStringBER::decode(unsigned char *buffer) {
	unsigned char *pointer = buffer;
    // T
    if (*pointer++ == TYPE_OCTETSTRING) {
        // L
        // TODO decode length
        _length = *pointer++;
        if (_length & 0x80) {
            uint32_t count = _length & 0x7F;
            for (uint32_t index = _length = 0; index < count & 0x7F;
                    ++index) {
                _length <<= 8;
                _length |= *pointer++;
            }
        }
        // V
        memcpy(_value, pointer, _length < SIZE_OCTETSTRING ? _length : SIZE_OCTETSTRING);
        // TODO check this
        _size = pointer - buffer + _length;
    }
    return _size;
}

//////////
// Null //
//////////

// Fixed length
// 05 00
NullBER::NullBER() :
        NullBER(TYPE_NULL) {
}

NullBER::NullBER(const uint8_t type) :
        BER(type) {
    _size = 2;
}

const uint32_t NullBER::encode(unsigned char *buffer) {
	unsigned char *pointer = buffer;
    // T, L, no value
    *pointer++ = _type;
    *pointer++ = _length;
    return _size;
}

const uint32_t NullBER::decode(unsigned char *buffer) {
	unsigned char *pointer = buffer;
    // T, L, no value
    if (*pointer++ == _type) {
        if (*pointer++ == _length) {
            _size = pointer - buffer;
        }
    }
    return _size;
}

///////////////////////
// Object Identifier //
///////////////////////

// Variable length
ObjectIdentifierBER::ObjectIdentifierBER(const char *value) :
        BER(TYPE_OBJECTIDENTIFIER) {
    setValue(value);
}

const uint32_t ObjectIdentifierBER::encode(unsigned char *buffer) {
    uint32_t index = 0;
    uint8_t subidentifier = 0;
    unsigned char *pointer = buffer;
    // T, L
    *pointer++ = _type;
    *pointer++ = _length;
    // V
    char *token = (char*) _value;
    while (token != NULL) {
        switch (index) {
        case 0:
            subidentifier = atoi(token);
            break;
        case 1:
            subidentifier = subidentifier * 40 + atoi(++token);
            *pointer = subidentifier;
            break;
            ;
        default: {
            subidentifier = atoi(++token);
            uint32_t value = subidentifier;
            uint32_t length = 0;
            do {
                value >>= 7;
                length++;
            } while (value);
            pointer += length;
            value = subidentifier;
            for (uint32_t index = 0; index < length; ++index) {
                *pointer-- = (value & 0x7F) | (index ? 0x80 : 0x00);
                value >>= 7;
            }
            pointer += length;
        }
            break;
        }
        token = strchr(token, '.');
        index++;
    }
    return _size;
}

const uint32_t ObjectIdentifierBER::decode(unsigned char *buffer) {
    uint32_t index = 0;
    uint8_t subidentifier = 0;
    char oid[SIZE_OBJECTIDENTIFIER];
    unsigned char *pointer = buffer;
    unsigned char *end = NULL;
    // T
    if (*pointer++ == _type) {
        // L
        _length = *pointer++;
        end = pointer + _length;
        // V
        do {
            if (index++) {
                subidentifier = 0;
                do {
                    subidentifier <<= 7;
                    subidentifier += *pointer & 0x7F;
                } while (*pointer++ & 0x80);
                sprintf(oid + strlen(oid), ".%d", subidentifier);
            } else {
                subidentifier = *pointer++;
                sprintf(oid, "%d.%d", subidentifier / 40, subidentifier % 40);
            }
        } while (pointer < end);
        strncpy(_value, oid, SIZE_OBJECTIDENTIFIER);
        _size = pointer - buffer;
    }
    return _size;
}

//////////////
// Sequence //
//////////////

// Variable length
SequenceBER::SequenceBER() :
        BER(TYPE_SEQUENCE) {
    _count = 0;
    memset(_bers, 0, SIZE_SEQUENCE);
    _size = 2;
}

SequenceBER::SequenceBER(const uint8_t type) :
        BER(type) {
}

SequenceBER::~SequenceBER() {
//	for (auto ber : _bers) {
//		delete ber;
//	}
//	_bers.clear();
    for (uint32_t index = 0; index < _count; ++index) {
        delete _bers[index];
    }
    _count = 0;
    memset(_bers, 0, SIZE_SEQUENCE);
}

const uint32_t SequenceBER::encode(unsigned char *buffer) {
	unsigned char *pointer = buffer;
    // T
    *pointer++ = _type;
    // L
    // TODO encode length
    *pointer++ = _length;
    // V
    for (uint32_t index = 0; index < _count; ++index) {
        pointer += _bers[index]->encode(pointer);
    }
    return _size;
}

const uint32_t SequenceBER::decode(unsigned char *buffer) {
	unsigned char *pointer = buffer;
	// T
	uint8_t type = *pointer++;
	if (type == _type) {
        // L
        Length __length(0);
        pointer += __length.decode(pointer);
        _length = __length.getLength();
        unsigned char *end = pointer + _length;
		if (_length) {
            do {
                uint8_t type = *pointer;
                BER *ber = nullptr;
                switch (type) {
                case TYPE_BOOLEAN:
                    ber = new BooleanBER(false);
                    break;
                case TYPE_INTEGER:
                    ber = new IntegerBER(0);
                    break;
                case TYPE_OCTETSTRING:
                    ber = new OctetStringBER(nullptr, 0);
                    break;
                case TYPE_NULL:
                    ber = new NullBER();
                    break;
                case TYPE_OBJECTIDENTIFIER:
                    ber = new ObjectIdentifierBER(nullptr);
                    break;
                case TYPE_TIMETICKS:
                    ber = new TimeTicksBER(0);
                    break;
                case TYPE_OPAQUE:
                    ber = new OpaqueBER(nullptr);
                    break;
                case TYPE_NOSUCHOBJECT:
                case TYPE_NOSUCHINSTANCE:
                    ber = new NullBER(type);
                    break;
                case TYPE_SEQUENCE:
                case TYPE_GETREQUEST:
                case TYPE_GETNEXTREQUEST:
                case TYPE_GETRESPONSE:
                case TYPE_SETREQUEST:
                    ber = new SequenceBER(type);
                    break;
                default:
                    Serial.print("[ERROR] unknown type: 0x");
                    if (type < 16) {
                        Serial.print("0");
                    }
                    Serial.println(type, HEX);
                    while (1) {
                    }
                }
                if (ber) {
                    pointer += ber->decode(pointer);
//                    _bers.push_back(ber);
                    add(ber);
                } else {
                    Serial.print("[ERROR] decode type 0x");
                    if (type < 16) {
                        Serial.print("0");
                    }
                    Serial.println(type, HEX);
                    while (1) {
                    }
                }
			} while (pointer < end);
        }
        _size = pointer - buffer;
    } else {
		Serial.print("[ERROR] sequence BER type 0x");
		Serial.println(type, HEX);
		while (1) {
		}
    }
    return _size;
}

////////////////
// IP Address //
////////////////

// Fixed length
IPAddressBER::IPAddressBER(IPAddress value) :
        BER(TYPE_IPADDRESS) {
    _length = SIZE_IPADDRESS;
    for (uint32_t index = 0; index < SIZE_IPADDRESS; ++index) {
        _value[index] = value[index];
    }
    _size = 6;
}

const uint32_t IPAddressBER::encode(unsigned char *buffer) {
	unsigned char *pointer = buffer;
    // T, L
    *pointer++ = _type;
    *pointer++ = _length;
    // V
    for (uint32_t index = 0; index < SIZE_IPADDRESS; ++index) {
        *pointer++ = _value[index];
    }
    return _size;
}

const uint32_t IPAddressBER::decode(unsigned char *buffer) {
	unsigned char *pointer = buffer;
    // T
    if (*pointer++ == _type) {
        // L
        _length = *pointer++;
        // V
        for (uint32_t index = 0; index < _length; ++index) {
            _value[index] = *pointer++;
        }
        _size = pointer - buffer;
    }
    return _size;
}

////////////////
// Counter 32 //
////////////////

Counter32BER::Counter32BER(const uint32_t value) :
        IntegerBER(value) {
    _type = TYPE_COUNTER32;
}

//////////////
// Gauge 32 //
//////////////

Gauge32BER::Gauge32BER(const uint32_t value) :
        IntegerBER(value) {
    _type = TYPE_GAUGE32;
}

////////////////
// Time ticks //
////////////////

TimeTicksBER::TimeTicksBER(const uint32_t value) :
        IntegerBER(value) {
    _type = TYPE_TIMETICKS;
}

////////////
// Opaque //
////////////

// Variable length
// 44 07
//       9F 78 04
//                C2 C7 FF FE
OpaqueBER::OpaqueBER(BER *ber) :
        BER(TYPE_OPAQUE) {
    if (ber) {
        _ber = ber;
        _length = _ber->getSize();
        _size = _length + 2;
    }
}

OpaqueBER::~OpaqueBER() {
    delete _ber;
}

const uint32_t OpaqueBER::encode(unsigned char *buffer) {
	unsigned char *pointer = buffer;
    // T, L, V
    *pointer++ = _type;
    *pointer++ = _length;
    _ber->encode(pointer);
    return _size;
}

const uint32_t OpaqueBER::decode(unsigned char *buffer) {
	unsigned char *pointer = buffer;
    // T
    if (*pointer++ == TYPE_OPAQUE) {
        // L
        // TODO decode length
        _length = *pointer++;
        unsigned char *end = pointer + _length;
        if (_length) {
            do {
                Type __type(0);
                __type.decode(pointer);
                uint8_t type = __type.getType();
                switch (type) {
                case TYPE_OPAQUEFLOAT:
                    _ber = new OpaqueFloatBER(0);
                    break;
                default:
                    Serial.print("[ERROR] opaque unknown type: 0x");
                    if (type < 16) {
                        Serial.print("0");
                    }
                    Serial.println(type, HEX);
                    while (1) {
                    }
                }
                if (_ber) {
                    pointer += _ber->decode(pointer);
                } else {
                    Serial.print("[ERROR] opaque decode type 0x");
                    if (type < 16) {
                        Serial.print("0");
                    }
                    Serial.println(type, HEX);
                    while (1) {
                    }
                }
            } while (pointer < end);
            _size = pointer - buffer;
        }
    }
    return _size;
}

///////////
// Float //
///////////

// Fixed length
// 48 04 XX XX XX XX
FloatBER::FloatBER(const float value) :
        BER(TYPE_FLOAT) {
    _length = 4;
    _value = value;
    _size = 6;
}

const uint32_t FloatBER::encode(unsigned char *buffer) {
	unsigned char *pointer = buffer;
    // T
    Type __type(_type);
    pointer += __type.encode(pointer);
    // L
    *pointer++ = _length;
    // V
    uint8_t *value = (uint8_t*) &_value;
    value += _length;
    for (uint32_t index = 0; index < _length; ++index) {
        *pointer++ = *--value;
    }
    return _size;
}

const uint32_t FloatBER::decode(unsigned char *buffer) {
	unsigned char *pointer = buffer;
    // T
    Type __type(0);
    pointer += __type.decode(pointer);
    _type = __type.getType();
    if ((_type == TYPE_FLOAT) || (_type == TYPE_OPAQUEFLOAT)) {
        // L
        _length = *pointer++;
        // V
        uint8_t *value = (uint8_t*) &_value;
        value += _length;
        for (uint32_t index = 0; index < _length; ++index) {
            *--value = *pointer++;
        }
        _size = pointer - buffer;
    }
    return _size;
}

////////////////////
// No Such Object //
////////////////////

NoSuchObjectBER::NoSuchObjectBER() : NullBER() {
    _type = TYPE_NOSUCHOBJECT;
}

//////////////////////
// No Such Instance //
//////////////////////

NoSuchInstanceBER::NoSuchInstanceBER() : NullBER() {
    _type = TYPE_NOSUCHINSTANCE;
}

/////////////////////
// End Of MIB View //
/////////////////////

EndOfMIBViewBER::EndOfMIBViewBER() : NullBER() {
    _type = TYPE_ENDOFMIBVIEW;
}

//////////////////
// Opaque Float //
//////////////////

// Fixed length
// 9F 78 04 XX XX XX XX
OpaqueFloatBER::OpaqueFloatBER(const float value) :
        FloatBER(value) {
    _type = TYPE_OPAQUEFLOAT;
    _size += 1; // Extra byte for length encoding
}

}
