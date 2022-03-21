#ifndef BER_H_
#define BER_H_

#include <Arduino.h>
#include <IPAddress.h>

namespace SNMP {

// TODO use vectors
//#include <vector>

enum {
    VERSION1, VERSION2C,
};

// Class ..
// Form     .
// Tag        .....
enum {
    CLASS_UNIVERSAL = 0x00,   // 00 . .....
    CLASS_APPLICATION = 0x40, // 01 . .....
    CLASS_CONTEXT = 0x80,     // 10 . .....
    CLASS_PRIVATE = 0xC0,     // 11 . .....
};

enum {
    FORM_PRIMITIVE = 0x00,    // .. 0 .....
    FORM_CONSTRUCTED = 0x20,  // .. 1 .....
};

enum {
    // Universal
    TYPE_BOOLEAN = CLASS_UNIVERSAL | FORM_PRIMITIVE | 0x01,    // 0x01
    TYPE_INTEGER,
    TYPE_BITSTRING,
    TYPE_OCTETSTRING,
    TYPE_NULL,
    TYPE_OBJECTIDENTIFIER,
    TYPE_SEQUENCE = CLASS_UNIVERSAL | FORM_CONSTRUCTED | 0x10, // 0x30
    // Application
    TYPE_IPADDRESS = CLASS_APPLICATION | 0x00,                 // 0x40
    TYPE_COUNTER32,                                            // 0x41
    TYPE_GAUGE32,                                              // 0x42
    TYPE_TIMETICKS,                                            // 0x43
    TYPE_OPAQUE,                                               // 0x44
    TYPE_COUNTER64 = CLASS_APPLICATION | 0x06,                 // 0x46
    TYPE_FLOAT = CLASS_APPLICATION | 0x08,                     // 0x48
    // Context
    TYPE_NOSUCHOBJECT = CLASS_CONTEXT | 0x00,                  // 0x80
    TYPE_NOSUCHINSTANCE,                                       // 0x81
    TYPE_GETREQUEST = CLASS_CONTEXT | FORM_CONSTRUCTED | 0x00, // 0xA0
    TYPE_GETNEXTREQUEST,                                       // 0xA1
    TYPE_GETRESPONSE,                                          // 0xA2
    TYPE_SETREQUEST,                                           // 0xA3
    // Opaque type
    TYPE_OPAQUEFLOAT = 0x30 | TYPE_FLOAT,                      // 0x78
};

// Buffer sizes
#define SIZE_OCTETSTRING 128
#define SIZE_OBJECTIDENTIFIER 64
#define SIZE_SEQUENCE 16
#define SIZE_IPADDRESS 4

class Type {
public:
    Type(unsigned int type);
    ~Type() {
    }

    const int encode(char *buffer);
    const int decode(char *buffer);

    unsigned int getType() {
        return _type;
    }

    unsigned int getSize() {
        return _size;
    }

private:
    unsigned int _type;
    unsigned int _size = 0;
};

class Length {
public:
    Length(unsigned int length);
    ~Length() {
    }

    const int encode(char *buffer);
    const int decode(char *buffer);

    unsigned int getLength() {
        return _length;
    }

    unsigned int getSize() {
        return _size;
    }

private:
    unsigned int _length;
    unsigned int _size = 0;
};

class BER {
public:
    BER(const uint8_t type) {
        _type = type;
    }

    virtual ~BER() {
    }

    virtual const int encode(char *buffer) = 0;
    virtual const int decode(char *buffer) = 0;

    const uint8_t getType() {
        return _type;
    }

    const unsigned int getLength() {
        return _length;
    }

    virtual const int getSize() {
        return _size;
    }

protected:
    uint8_t _type = 0;
    unsigned int _length = 0;
    unsigned int _size = 0;
};

class BooleanBER: public BER {
public:
    BooleanBER(const bool value);
    virtual ~BooleanBER() {
    }

    virtual const int encode(char *buffer);
    virtual const int decode(char *buffer);

    bool getValue() const {
        return _value;
    }

    void setValue(bool value) {
        _value = value;
    }

private:
    bool _value;
};

class IntegerBER: public BER {
public:
    IntegerBER(const int value);
    virtual ~IntegerBER() {
    }

    virtual const int encode(char *buffer);
    virtual const int decode(char *buffer);

    const int getValue() const {
        return _value;
    }

    void setValue(const int value) {
        _value = value;
    }

private:
    int _value;
};

class OctetStringBER: public BER {
public:
    OctetStringBER(const char *value, const unsigned int length);
    virtual ~OctetStringBER() {
    }

    virtual const int encode(char *buffer);
    virtual const int decode(char *buffer);

    const char* getValue() const {
        return _value;
    }

    const bool getBit(const int index) const {
        unsigned int byte = index / 8;
        unsigned int bit = index % 8;
        return _value[byte] & (0x80 >> bit);
    }

private:
    char _value[SIZE_OCTETSTRING] = { 0 };
};

class NullBER: public BER {
public:
    NullBER();
    NullBER(const uint8_t type);
    virtual ~NullBER() {
    }

    virtual const int encode(char *buffer);
    virtual const int decode(char *buffer);
};

class ObjectIdentifierBER: public BER {
public:
    ObjectIdentifierBER(const char *value);
    virtual ~ObjectIdentifierBER() {
    }

    virtual const int encode(char *buffer);
    virtual const int decode(char *buffer);

    const char* getValue() const {
        return _value;
    }

private:
    char _value[SIZE_OBJECTIDENTIFIER] = { 0 };
};

class SequenceBER: public BER {
public:
    SequenceBER();
    SequenceBER(const uint8_t type);
    virtual ~SequenceBER();

    virtual const int encode(char *buffer);
    virtual const int decode(char *buffer);

    void add(BER *ber) {
//		_bers.push_back(ber);
        _bers[_count++] = ber;
        _length += ber->getSize();
        _size = _length + 2;
    }

    void remove() {
        if (_count) {
            _length -= _bers[--_count]->getSize();
            _size = _length + 2;
//			delete  _bers[_count];
        }
    }

    BER* operator [](const unsigned int index) {
        return _bers[index];
    }

    const unsigned int count() const {
//		return _bers.size();
        return _count;
    }

    const char* getOID() const {
        // FIXME 'dynamic_cast' not permitted with '-fno-rtti'
        //		ObjectIdentifierBER *ber = dynamic_cast<ObjectIdentifierBER*>(_bers[0]);
        //		if (ber) {
        //			return ber->getValue();
        //		}
        if (_bers[0]->getType() == TYPE_OBJECTIDENTIFIER) {
            return ((ObjectIdentifierBER*) _bers[0])->getValue();
        }
        return nullptr;
    }

    BER* getValue() const {
        return _bers[1];
    }

private:
//	std::vector<BER*> _bers;
    unsigned int _count = 0;
    BER *_bers[SIZE_SEQUENCE];
};

class IPAddressBER: public BER {
public:
    IPAddressBER(IPAddress value);
    virtual ~IPAddressBER() {
    }

    virtual const int encode(char *buffer);
    virtual const int decode(char *buffer);

private:
    char _value[SIZE_IPADDRESS] = { 0 };
};

class Counter32BER: public IntegerBER {
public:
    Counter32BER(const unsigned int value);
    virtual ~Counter32BER() {
    }
};

class Gauge32BER: public IntegerBER {
public:
    Gauge32BER(const unsigned int value);
    virtual ~Gauge32BER() {
    }
};

class TimeTicksBER: public IntegerBER {
public:
    TimeTicksBER(const unsigned int value);
    virtual ~TimeTicksBER() {
    }
};

class OpaqueBER: public BER {
public:
    OpaqueBER(BER *ber);
    virtual ~OpaqueBER();

    virtual const int encode(char *buffer);
    virtual const int decode(char *buffer);

    BER* getBER() {
        return _ber;
    }

private:
    BER *_ber = nullptr;
};

class FloatBER: public BER {
public:
    FloatBER(const float value);
    virtual ~FloatBER() {
    }

    virtual const int encode(char *buffer);
    virtual const int decode(char *buffer);

    const float getValue() const {
        return _value;
    }

protected:
    float _value = 0;
};

class OpaqueFloatBER: public FloatBER {
public:
    OpaqueFloatBER(const float value);
    virtual ~OpaqueFloatBER() {
    }
};

}

#endif /* BER_H_ */
