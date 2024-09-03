#ifndef BER_H_
#define BER_H_

#include <Arduino.h>
#include <IPAddress.h>

#ifdef __has_include
#if __has_include("SNMPcfg.h")
#include "SNMPcfg.h"
#else
/**
 * @def SNMP_STREAM
 * @brief Defines read and write operations.
 */
#define SNMP_STREAM 1

/**
 * @def SNMP_VECTOR
 * @brief Defines storage for ArrayBER.
 */
#define SNMP_VECTOR 0

/**
 * @def SNMP_CAPACITY
 * @brief Defines capacity of SequenceBER.
 */
#define SNMP_CAPACITY 6
#endif
#endif

#if SNMP_STREAM
#include <Stream.h>
#endif

#if SNMP_VECTOR
#include <vector>
#endif

/**
 * @namespace SNMP
 * @brief %SNMP library namespace.
 */
namespace SNMP {

/**
 * @struct Version
 * @brief Helper struct to handle SNMP versions.
 *
 * The library supports SNMP version 1 and version 2c.
 */
struct Version {
    /**
     * @brief Enumerates all supported versions.
     */
    enum : uint8_t {
        V1,     /**< 0 */
        V2C,    /**< 1 */
    };
};

/**
 * @struct Error
 * @brief Helper struct to handle SNMP error.
 *
* An error has 2 elements.
 *
 * - Status sets the error code.
 * - Index identifies the variable binding that caused the error.
 *
 * @note Index starts at 1.
 */
struct Error {
    /**
     * @brief Enumerates all possible error codes.
     */
    enum : uint8_t {
        // Version 1
        NoError,                /**< 0 */
        TooBig,                 /**< 1 */
        NoSuchName,             /**< 2 */
        BadValue,               /**< 3 */
        ReadOnly,               /**< 4 */
        GenErr,                 /**< 5 */
        // Version 2C
        NoAccess,               /**< 6 */
        WrongType,              /**< 7 */
        WrongLength,            /**< 8 */
        WrongEncoding,          /**< 9 */
        WrongValue,             /**< 10 */
        NoCreation,             /**< 11 */
        InconsistentValue,      /**< 12 */
        ResourceUnavailable,    /**< 13 */
        CommitFailed,           /**< 14 */
        UndoFailed,             /**< 15 */
        AuthorizationError,     /**< 16 */
        NotWritable,            /**< 17 */
        InconsistentName,       /**< 18 */
    };

    /**
     * @brief Initializes to default values.
     *
     * - Status is set to NoError.
     * - Index is set to 0.
     */
    Error() {
        _status = NoError;
        _index = 0;
    }

    /** Error status. */
    uint8_t _status;
    /** Error index. */
    uint8_t _index;
};

/**
 * @struct Trap
 * @brief Helper struct to handle trap PDU.
 *
 * Defines all needed to create an SNMP trap.
 */
struct Trap {
    /**
     * @brief Enumerates all possible generic trap codes.
     */
    enum : uint8_t {
        ColdStart,              /**< 0 */
        WarmStart,              /**< 1 */
        LinkDown,               /**< 2 */
        LinkUp,                 /**< 3 */
        AuthenticationFailure,  /**< 4 */
        EGPNeighborLoss,        /**< 5 */
        EnterpriseSpecific,     /**< 6 */
    };

    /**
     * @brief Initializes to default values.
     *
     * - Enterprise is set to null.
     * - Generic trap is set to ColdStart.
     * - Specific trap is set to 0.
     * - Timestamp is set to 0;
     */
    Trap() {
        _enterprise = nullptr;
        _genericTrap = ColdStart;
        _specificTrap = 0;
        _timeStamp = 0;
    }

    /** Enterprise OID. */
    const char *_enterprise;
    /** Network address of the agent. */
    IPAddress _agentAddr;
    /** Generic trap code. */
    uint8_t _genericTrap;
    /** Specific trap code. */
    uint8_t _specificTrap;
    /** Time elapsed since device startup. */
    uint32_t _timeStamp;
};

/**
 * @struct Class
 * @brief Helper struct to handle class of BER type.
 *
 * Class is bits 7 and 6 in MSB of BER type.
 */
struct Class {
    /**
     * @brief Enumerates all possible class values.
     */
    enum : uint8_t {
        Universal = 0x00,   /**< 00 . ..... */
        Application = 0x40, /**< 01 . ..... */
        Context = 0x80,     /**< 10 . ..... */
        Private = 0xC0,     /**< 11 . ..... */
    };
};

/**
 * @struct Form
 * @brief Helper struct to handle form of BER type.
 *
 * Form is bit 5 in MSB of BER type.
 */
struct Form {
    /**
     * @brief Enumerates all possible form values.
     */
    enum : uint8_t {
        Primitive = 0x00,   /**< .. 0 ..... */
        Constructed = 0x20, /**< .. 1 ..... */
    };
};

/**
 * @class Flag
 * @brief Helper class for internal flag.
 */
class Flag {
public:
    /**
     * @brief Defines some internal flag bits.
     */
    enum : uint8_t {
        None = 0,           /**< No flag. */
        Typed = (1 << 0),   /**< Type is already decoded, don't decode. */
    };
};

/**
 * @class Base
 * @brief Base class for BER, Length and Type.
 *
 * This abstract class is the base class for all BER objects.
 */
class Base {
public:
#if SNMP_STREAM
    /**
     * @brief Encodes integer value to stream.
     *
     * If the integer is greater than 127 (0x7F in hexadecimal) it is multibyte encoded
     * with the following rules.
     *
     * - The integer bytes are encoded from MSB to LSB into bits 6 to 0 of each encoded bytes.
     * - Bit 7 of each encoded bytes is set to 1, except for the last one where it is set to 0.
     *
     * @param value Integer value to encode.
     * @param stream Stream to write to.
     * @param size Number of encoded bytes.
     */
    static void encode7bits(uint32_t value, Stream &stream, const uint8_t size) {
        for (uint8_t index = 0; index < size; ++index) {
            stream.write((value >> (7 * (size - index - 1)) & 0x7F) | ((size - index - 1) ? 0x80 : 0x00));
        }
    }
#else
    /**
     * @brief Encodes integer value to memory buffer.
     *
     * If the integer is greater than 127 (0x7F in hexadecimal) it is multibyte encoded
     * with the following rules.
     *
     * - The integer bytes are encoded from MSB to LSB into bits 6 to 0 of each encoded bytes.
     * - Bit 7 of each encoded bytes is set to 1, except for the last one where it is set to 0.
     *
     * @param value Integer value to encode.
     * @param buffer Pointer to the buffer.
     * @param size Number of encoded bytes.
     */
    void encode7bits(uint32_t value, uint8_t *buffer, const uint8_t size) {
        buffer += size;
        for (uint8_t index = 0; index < size; ++index) {
            *buffer-- = (value & 0x7F) | (index ? 0x80 : 0x00);
            value >>= 7;
        }
    }
#endif

protected:
    /** Size in bytes of the encoded object. */
    unsigned int _size = 0;

    friend class BER;
};

/**
 * @class Type
 * @brief Type of a BER object.
 *
 * This class handles the type of a BER object.
 *
 * A type has 3 elements.
 *
 * - Class.
 * - Form.
 * - Tag.
 */
class Type: public Base {
public:
    /**
     * @brief Enumerates most of type values.
     */
    enum : uint16_t {
        // Universal
        Boolean = Class::Universal | Form::Primitive | 0x01,    /**< 0x01 */
        Integer,                                                /**< 0x02 */
        BitString,                                              /**< 0x03 */
        OctetString,                                            /**< 0x04 */
        Null,                                                   /**< 0x05 */
        ObjectIdentifier,                                       /**< 0x06 */
        Sequence = Class::Universal | Form::Constructed | 0x10, /**< 0x30 */
        // Application
        IPAddress = Class::Application | 0x00,                  /**< 0x40 */
        Counter32,                                              /**< 0x41 */
        Gauge32,                                                /**< 0x42 */
        TimeTicks,                                              /**< 0x43 */
        Opaque,                                                 /**< 0x44 */
        Counter64 = Class::Application | 0x06,                  /**< 0x46 */
        Float = Class::Application | 0x08,                      /**< 0x48 */
        // Context
        NoSuchObject = Class::Context | 0x00,                   /**< 0x80 */
        NoSuchInstance,                                         /**< 0x81 */
        EndOfMIBView,                                           /**< 0x82 */
        // Version 1
        GetRequest = Class::Context | Form::Constructed | 0x00, /**< 0xA0 */
        GetNextRequest,                                         /**< 0xA1 */
        GetResponse,                                            /**< 0xA2 */
        SetRequest,                                             /**< 0xA3 */
        Trap,                                                   /**< 0xA4 */
        // Version 2C
        GetBulkRequest,                                         /**< 0xA5 */
        InformRequest,                                          /**< 0xA6 */
        SNMPv2Trap,                                             /**< 0xA7 */
        // Version 3
        Report,                                                 /**< 0xA8 */
        // Opaque type
        OpaqueFloat = 0x9F78                                    /**< 0x9F78 */
    };

    /**
     * @brief Creates a BER type from a type value.
     *
     * @param type BER Type.
     */
    Type(const unsigned int type = 0) :
            Base() {
        setType(type);
    }

    /**
     * @brief Creates a BER type from a composite type value.
     *
     * Type value is computed from flag (class and form) and tag.
     *
     * @param flag %Flag (class and form).
     * @param tag Tag.
     */
    Type(const uint8_t flag, const unsigned int tag) :
            Base() {
        setFlagTag(flag, tag);
    }

#if SNMP_STREAM
    /**
     * @brief Encodes BER type to stream.
     *
     * There is two ways to encode a BER type.
     *
     * - Short form if tag < 31.
     * - Long form otherwise.
     *
     * @param stream Stream to write to.
     */
    void encode(Stream &stream) {
        if (_size == 1) {
            stream.write(_class | _form | _tag);
        } else {
            stream.write(_class | _form | 0x1F);
            Base::encode7bits(_tag, stream, _size - 1);
        }
    }

    /**
     * @brief Decodes BER type from stream.
     *
     * @param stream Stream to read from.
     */
    void decode(Stream &stream) {
        _size = 1;
        _type = stream.read();
        _class = _type & 0xC0;
        _form = _type & 0x20;
        _tag = _type & 0x1F;
        if (_tag == 0x1F) {
            _tag = 0;
            do {
                _size++;
                _type <<= 8;
                _type |= stream.read();
                _tag <<= 7;
                _tag |= _type & 0x7F;
            } while (_type & 0x80);
        }
    }
#else
    /**
     * @brief Encodes BER type to memory buffer.
     *
     * There is two ways to encode a BER type.
     *
     * - Short form if tag < 31.
     * - Long form otherwise.
     *
     * @param buffer Pointer to the buffer.
     * @return Next position to be written in buffer.
     */
    uint8_t* encode(uint8_t *buffer) {
        if (_size == 1) {
            *buffer = _class | _form | _tag;
        } else {
            *buffer = _class | _form | 0x1F;
            Base::encode7bits(_tag, buffer, _size - 1);
        }
        return buffer + _size;
    }

    /**
     * @brief Decodes BER type from memory buffer.
     *
     * @param buffer Pointer to the buffer.
     * @return Next position to be read in buffer.
     */
    uint8_t* decode(uint8_t *buffer) {
        _type = *buffer++;
        _class = _type & 0xC0;
        _form = _type & 0x20;
        _tag = _type & 0x1F;
        if ((_tag & 0x1F) == 0x1F) {
            _tag = 0;
            do {
                _type <<= 8;
                _type |= *buffer++;
                _tag <<= 7;
                _tag |= _type & 0x7F;
            } while (_type & 0x80);
        }
        return buffer;
    }
#endif

    /**
     * @brief Unsigned int conversion function.
     *
     * Gets type as integer.
     *
     * @return BER type.
     */
    operator unsigned int() const {
        return _type;
    }

    /**
     * @brief Gets class from BER type.
     *
     * @return Class from BER type.
     */
    const uint8_t getClass() const {
        return _class;
    }

    /**
     * @brief Gets form from BER type.
     *
     * @return Form from BER type.
     */
    const uint8_t getForm() const {
        return _form;
    }

    /**
     * @brief Gets tag from BER type.
     *
     * @return Tag from BER type.
     */
    const unsigned int getTag() const {
        return _tag;
    }

private:
    /** Class of the BER type. */
    uint8_t _class;
    /** Form of the BER type. */
    uint8_t _form;
    /** Tag of the BER type. */
    unsigned int _tag;
    /** Type of the BER object. */
    unsigned int _type;

    /**
     * @brief Sets BER type from type value.
     *
     * Type is set and the size is computed again.
     *
     * @param type BER type.
     */
    void setType(unsigned int type) {
        _size = 1;
        _type = type;
        while (type >>= 8) {
            ++_size;
        }
        type = _type;
        if (_size == 1) {
            _tag = type & 0x1F;
        } else {
            _tag = 0;
            for (uint8_t index = 0; index < _size - 1; ++index) {
                _tag <<= 7;
                _tag |= type & 0x7F;
                type >>= 8;
            }
        }
        _class = type & 0xC0;
        _form = type & 0x20;
    }

    /**
     * @brief Creates a BER type from a composite type value.
     *
     * type value is computed from flag (class and form) and tag.
     *
     * Type is set and the size is computed again.
     *
     * @param flag %Flag (class and form).
     * @param tag Tag.
     */
    void setFlagTag(uint8_t flag, unsigned int tag) {
        _size = 1;
        _class = flag & 0xC0;
        _form = flag & 0x20;
        _tag = tag;
        if (tag < 0x1F) {
            _type = _class | _form | _tag;
        } else {
            _type = _class | _form | 0x1F;
            do {
                ++_size;
            } while (tag >>= 7);
            tag = _tag;
            for (int32_t index = _size - 2; index > -1; --index) {
                _type <<= 8;
                _type |= tag >> (index << 3) & 0x7F;
                _type |= index ? 0x80 : 0x00;
            }
        }
    }
};

/**
 * @class Length
 * @brief Length of a BER object.
 *
 * This class handles the length of a BER object.
 */
class Length: public Base {
public:
    /**
     * @brief Creates a BER length from a length value.
     *
     * @param length
     */
    Length(unsigned int length = 0) :
            Base() {
        setLength(length);
    }

#if SNMP_STREAM
    /**
     * @brief Encodes BER length to stream.
     *
     * There is two ways to encode a BER length.
     *
     * - Short form if length < 127.
     * - Long form otherwise.
     *
     * @param stream Stream to write to.
     */
    void encode(Stream &stream) {
        if (_length > 0x7F) {
            stream.write(0x80 | _size);
            unsigned int length = _length;
            for (uint8_t index = 0; index < _size; ++index) {
                stream.write(length >> ((_size - index - 1) << 3));
            }
        } else {
            stream.write(_length);
        }
    }

    /**
     * @brief Decodes BER length from stream.
     *
     * @param stream Stream to read from.
     */
    void decode(Stream &stream) {
        _length = stream.read();
        if (_length & 0x80) {
            _size = _length & 0x7F;
            _length = 0;
            for (uint8_t index = 0; index < _size; ++index) {
                _length <<= 8;
                _length += stream.read();
            }
            _size++;
        } else {
            _size = 1;
        }
    }
#else
    /**
     * @brief Encodes BER length to memory buffer.
     *
     * There is two ways to encode a BER length.
     *
     * - Short form if length < 127.
     * - Long form otherwise.
     *
     * @param buffer Pointer to the buffer.
     * @return Next position to be written in buffer.
     */
    uint8_t* encode(uint8_t *buffer) {
        uint8_t *pointer = buffer;
        if (_length > 0x7F) {
            *pointer = 0x80 | _size;
            pointer += _size;
            unsigned int value = _length;
            for (uint8_t index = 0; index < _size; ++index) {
                *pointer-- = value;
                value >>= 8;
            }
        } else {
            *pointer++ = _length;
        }
        return buffer + _size;
    }

    /**
     * @brief Decodes BER length from memory buffer.
     *
     * @param buffer Pointer to the buffer.
     * @return Next position to be read in buffer.
     */
    uint8_t* decode(uint8_t *buffer) {
        uint8_t *pointer = buffer;
        _length = *pointer++;
        if (_length & 0x80) {
            _size = _length & 0x7F;
            _length = 0;
            for (uint8_t index = 0; index < _size; ++index) {
                _length <<= 8;
                _length += *pointer++;
            }
        }
        return pointer;
    }
#endif

    /**
     * @brief Unsigned int conversion function.
     *
     * Gets length as integer.
     *
     * ```cpp
     * Length length;
     * unsigned int a = length;
     * ```
     *
     * @return BER length.
     */
    operator unsigned int() const {
        return _length;
    }

    /**
     * @brief Unsigned int assignment operator.
     *
     * Sets length from integer.
     *
     * ```cpp
     * Length length;
     * length = 10;
     * ```
     *
     * @param length
     * @return Length object.
     */
    Length& operator=(const unsigned int length) {
        setLength(length);
        return *this;
    }

    /**
     * @brief Addition operator.
     *
     * Adds integer to length.
     *
     * ```cpp
     * Length length;
     * length += 10;
     * ```
     *
     * @param length Value to add.
     * @return Length object.
     */
    Length& operator+=(const unsigned int length) {
        setLength(_length + length);
        return *this;
    }

    /**
     * @brief Subtraction operator.
     *
     * Subtracts integer from length.
     *
     * ```cpp
     * Length length;
     * length -= 10;
     * ```
     *
     * @param length Value to subtract.
     * @return Length object.
     */
    Length& operator-=(const unsigned int length) {
        setLength(_length - length);
        return *this;
    }

    /**
     * @brief Prefix increment operator.
     *
     * Increments length and return incremented value.
     *
     * ```cpp
     * Length length;
     * ++length;
     * ```
     *
     * @return Length value.
     */
    Length& operator ++() {
        setLength(++_length);
        return *this;
    }

    /**
     * @brief Postfix increment operator.
     *
     * Increments length and return value before increment.
     *
     * ```cpp
     * Length length;
     * length++;
     * ```
     *
     * @return Length value.
     */
    Length operator ++(int) {
        Length length = *this;
        operator++();
        return length;
    }

private:
    /** Length of the BER object. */
    unsigned int _length;

    /**
     * @brief Sets length from integer.
     *
     * Length is set and the size is computed again.
     *
     * @param length
     */
    void setLength(unsigned int length) {
        _size = 1;
        _length = length;
        if (length > 0x7F) {
            do {
                length >>= 8;
                _size++;
            } while (length);
        }
    }
};

/**
 * @class BER
 * @brief BER object.
 *
 * This class is the base class of all the BER objects.
 *
 * A BER object uses the TLV format. TLV is an acronym for type, length and value.
 *
 * - Type is the type of the object.
 * - Length is the length of the value of the object.
 * - Value is the value of the object.
 */
class BER: public Base {
public:
    /**
     * @brief Creates a BER object.
     *
     * @param type BER type.
     */
    BER(const unsigned int type) :
            Base() {
        _type = Type(type);
        _length = Length(0);
    }

    /**
     * @brief BER destructor.
     */
    virtual ~BER() {
    }

#if SNMP_STREAM
    /**
     * @brief Encodes BER type and length to stream.
     *
     * @warning Must be called first by derived class in overridden functions.
     *
     * @param stream Stream to write to.
     */
    virtual void encode(Stream &stream) {
        _type.encode(stream);
        _length.encode(stream);
    }

    /**
     * @brief Encodes BER numeric value to stream.
     *
     * SNMP numeric values are big-endian. Most significants bytes are written first.
     *
     * @tparam T C++ type of the integer.
     * @param value Integer value to encode.
     * @param stream Stream to write to.
     */
    template<typename T>
    void encodeNumeric(T value, Stream &stream) {
        BER::encode(stream);
        for (uint8_t index = 0; index < _length; ++index) {
            stream.write(value >> ((_length - index - 1) << 3));
        }
    }

    /**
     * @brief Decodes BER type and length from stream.
     *
     * @warning Must be called first by derived class in overridden functions.
     *
     * @param stream Stream to read from.
     * @param flag Decoding flag.
     */
    virtual void decode(Stream &stream, const uint8_t flag = Flag::None) {
        if ((flag & Flag::Typed) == Flag::None) {
            _type.decode(stream);
        }
        _length.decode(stream);
    }

    /**
     * @brief Decodes BER numeric value from stream.
     *
     * SNMP numeric values are big-endian. Most significants bytes are read first.
     *
     * @tparam T C++ type of the numeric value.
     * @param value Numeric value to decode.
     * @param stream Stream to read from.
     * @param flag Decoding flag.
     */
    template<typename T>
    void decodeNumeric(T *value, Stream &stream, const uint8_t flag = Flag::None) {
        BER::decode(stream, flag);
        if (T() - 1 < 0) {
            *value = stream.peek() & 0x80 ? 0xFFFFFFFF : 0;
        } else {
            *value = 0;
        }
        for (uint8_t index = 0; index < _length; ++index) {
            *value <<= 8;
            *value |= static_cast<uint8_t>(stream.read());
        }
    }
#else
    /**
     * @brief Encodes BER type and length to memory buffer.
     *
     * @warning Must be called first by derived class in overridden functions.
     *
     * @param buffer Pointer to the buffer.
     * @return Next position to be written in buffer.
     */
    virtual uint8_t* encode(uint8_t *buffer) {
        uint8_t *pointer = _type.encode(buffer);
        pointer = _length.encode(pointer);
        return pointer;
    }

    /**
     * @brief Encodes BER numeric value to memory buffer.
     *
     * SNMP numeric values are big-endian. Most significants bytes are written first.
     *
     * @tparam T C++ type of the numeric value.
     * @param value Numeric value to encode.
     * @param buffer Pointer to the buffer.
     * @return Next position to be written in buffer.
     */
    template<typename T>
    uint8_t* encodeNumeric(T value, uint8_t *buffer) {
        uint8_t *pointer = BER::encode(buffer);
        for (uint8_t index = 0; index < _length; ++index) {
            *pointer++ = value >> ((_length - index - 1) << 3);
        }
        return pointer;
    }

    /**
     * @brief Decodes BER type and length from memory buffer.
     *
     * @warning Must be called first by derived class in overridden functions.
     *
     * @param buffer Pointer to the buffer.
     * @return Next position to be read in buffer.
     */
    virtual uint8_t* decode(uint8_t *buffer) {
        uint8_t *pointer = _type.decode(buffer);
        pointer = _length.decode(pointer);
        return pointer;
    }

    /**
     * @brief Decodes BER numeric value from memory buffer.
     *
     * SNMP numeric values are big-endian. Most significants bytes are read first.
     *
     * @tparam T C++ type of the numeric value.
     * @param value Numeric value to decode.
     * @param buffer Pointer to the buffer.
     * @param flag Decoding flag.
     * @return Next position to be read in buffer.
     */
    template<typename T>
    uint8_t* decodeNumeric(T *value, uint8_t *buffer, const uint8_t flag =
            Flag::None) {
        uint8_t *pointer = BER::decode(buffer);
        if (T() - 1 < 0) {
            *value = *pointer & 0x80 ? 0xFFFFFFFF : 0;
        } else {
            *value = 0;
        }
        for (unsigned int index = 0; index < _length; ++index) {
            *value <<= 8;
            *value |= static_cast<uint8_t>(*pointer++);
        }
        return pointer;
    }
#endif

    /**
     * @brief Computes BER length of a negative integer.
     *
     * @tparam T C++ type of the integer.
     * @param value Integer value.
     */
    template<typename T>
    void setNegative(T value) {
        unsigned int length = sizeof(T);
        for (; length > 1; --length) {
            uint16_t word = value >> ((length - 2) << 3);
            if ((word & 0xFF80) != 0xFF80) {
                break;
            }
        }
        _length = length;
    }

    /**
     * @brief Computes BER length of a positive integer.
     *
     * @tparam T C++ type of the integer.
     * @param value Integer value.
     */
    template<typename T>
    void setPositive(T value) {
        unsigned int length = 0;
        uint8_t carry = 0;
        do {
            carry = value & 0x80;
            value >>= 8;
            length++;
        } while (value | carry);
        _length = length;
    }

    /**
     * @brief Gets the BER type.
     *
     * @return BER type.
     */
    const unsigned int getType() const {
        return _type;
    }

    /**
     * @brief Gets the BER length.
     *
     * @return BER length.
     */
    const unsigned int getLength() const {
        return _length;
    }

    /**
     * @brief Gets the size of the BER.
     *
     * The size of the BER is computed from:
     *
     * - Type size.
     * - Length size.
     * - Length.
     *
     * @param refresh Unused.
     * @return BER size.
     */
    virtual const unsigned int getSize(const bool refresh = false) {
        _size = _type._size + _length._size + _length;
        return _size;
    }

protected:
    /** BER length. */
    Length _length;
    /** BER type. */
    Type _type;

    /**
     * @brief Creates a BER of given type.
     *
     * @param type BER type.
     * @return Pointer to created BER or nullptr if the type is unknown.
     */
    BER* create(const Type &type);
};

/**
 * @class BooleanBER
 * @brief BER object to handle boolean.
 *
 * - Type is TYPE_BOOLEAN.
 * - Length is fixed, 1 byte, short form encoding.
 * - Size is fixed, 3 bytes.
 *
 * Example
 *
 * | Boolean | Encoding |
 * |:--------|:---------|
 * | true    | 01 01 FF |
 * | false   | 01 01 00 |
 */
class BooleanBER: public BER {
public:
    /**
     * @brief Creates a BooleanBER object.
     *
     * @param value BooleanBER boolean value.
     */
    BooleanBER(const bool value) :
            BER(Type::Boolean) {
        _length = LENGTH;
        _value = value;
    }

#if SNMP_STREAM
    /**
     * @brief Encodes BooleanBER to stream.
     *
     * Type and length are encoded by the inherited BER::encode() then BooleanBER
     * value is encoded.
     *
     * @param stream Stream to write to.
     */
    virtual void encode(Stream &stream) {
        BER::encode(stream);
        stream.write(_value ? 0xFF : 0x00);
    }

    /**
     * @brief Decodes BooleanBER from stream.
     *
     * Type and length are decoded by the inherited BER::decode() then BooleanBER
     * value is decoded.
     *
     * @param stream Stream to read from.
     * @param flag Decoding flag.
     */
    virtual void decode(Stream &stream, const uint8_t flag = Flag::None) {
        BER::decode(stream, flag);
        _value = stream.read();
    }
#else
    /**
     * @brief Encodes BooleanBER to memory buffer.
     *
     * Type and length are encoded by the inherited BER::encode() then BooleanBER
     * value is encoded.
     *
     * @param buffer Pointer to the buffer.
     * @return Next position to be written in buffer.
     */
    virtual uint8_t* encode(uint8_t *buffer) {
        uint8_t *pointer = BER::encode(buffer);
        *pointer = _value ? 0xFF : 0x00;
        return buffer + SIZE;
    }

    /**
     * @brief Decodes BooleanBER from memory buffer.
     *
     * Type and length are decoded by the inherited BER::decode() then BooleanBER
     * value is decoded.
     *
     * @param buffer Pointer to the buffer.
     * @return Next position to be read in buffer.
     */
    virtual uint8_t* decode(uint8_t *buffer) {
        uint8_t *pointer = BER::decode(buffer);
        _value = *pointer++;
        return buffer + SIZE;
    }
#endif

    /**
     * @brief Gets the BooleanBER value.
     *
     * @return BER boolean value.
     */
    const bool getValue() const {
        return _value;
    }

    /**
     * @brief Sets the BooleanBER value.
     *
     * @note No needs to update fixed length.
     *
     * @param value IntegerBER boolean value.
     */
    void setValue(const bool value) {
        _value = value;
    }

private:
    /** Fixed length of the BooleanBER. */
    static constexpr uint8_t LENGTH = 1;
    /** Fixed size of the BooleanBER. */
    static constexpr uint8_t SIZE = 3;
    /** BooleanBER boolean value. */
    bool _value;
};

/**
 * @class IntegerBER
 * @brief BER object to handle integer.
 *
 * - Type is TYPE_INTEGER.
 * - Length is variable, from 1 to 4 byte(s), short form encoding.
 * - Size is variable, from 3 to 6 bytes.
 *
 * Example
 *
 * | Integer      | Encoding          |
 * |-------------:|:------------------|
 * |            0 | 02 01 00          |
 * |          127 | 02 01 7F          |
 * |          128 | 02 02 00 80       |
 * |          256 | 02 02 01 00       |
 * |   2147483647 | 02 04 7F FF FF FF |
 * |         -128 | 02 01 80          |
 * |         -129 | 02 02 FF 7F       |
 * |  -2147483648 | 02 04 80 00 00 00 |
 */
class IntegerBER: public BER {
public:
    /**
     * @brief Creates an IntegerBER object.
     *
     * @param value IntegerBER integer value.
     */
    IntegerBER(const int32_t value) :
            BER(Type::Integer) {
        setValue(value);
    }

#if SNMP_STREAM
    /**
     * @brief Encodes IntegerBER to stream.
     *
     * The inherited BER::encodeNumeric<T>() template function encodes type, length
     * and value.
     *
     * @param stream Stream to write to.
     */
    virtual void encode(Stream &stream) {
        BER::encodeNumeric<int32_t>(_value, stream);
    }

    /**
     * @brief Decodes IntegerBER from stream.
     *
     * The inherited BER::decodeNumeric<T>() template function decodes type, length
     * and value.
     *
     * @param stream Stream to read from.
     * @param flag Decoding flag.
     */
    virtual void decode(Stream &stream, const uint8_t flag = Flag::None) {
        BER::decodeNumeric<int32_t>(&_value, stream, flag);
    }

#else
    /**
     * @brief Encodes IntegerBER to memory buffer.
     *
     * The inherited BER::encodeNumeric<T>() template function encodes type, length
     * and value.
     *
     * @param buffer Pointer to the buffer.
     * @return Next position to be written in buffer.
     */
    virtual uint8_t* encode(uint8_t *buffer) {
        return BER::encodeNumeric<int32_t>(_value, buffer);
    }

    /**
     * @brief Decodes IntegerBER from memory buffer.
     *
     * The inherited BER::decodeNumeric<T>() template function decodes type, length
     * and value.
     *
     * @param buffer Pointer to the buffer.
     * @return Next position to be read in buffer.
     */
    virtual uint8_t* decode(uint8_t *buffer) {
        return BER::decodeNumeric<int32_t>(&_value, buffer);
    }
#endif

    /**
     * @brief Gets the IntegerBER value.
     *
     * @return IntegerBER integer value.
     */
    const int32_t getValue() const {
        return _value;
    }

    /**
     * @brief Sets the IntegerBER value.
     *
     * @note Length is updated.
     *
     * @param value IntegerBER integer value.
     */
    void setValue(const int32_t value) {
        _value = value;
        if (_value < 0) {
            BER::setNegative(value);
        } else {
            BER::setPositive(value);
        }
    }

private:
    /** IntegerBER integer value. */
    int32_t _value;
};

/**
 * @class OctetStringBER
 * @brief BER object to handle octet string.
 *
 * - Type is TYPE_OCTETSTRING.
 * - Length is variable.
 * - Size is variable.
 *
 * Example
 *
 * | Octet String            | Encoding                      |
 * |:------------------------|:------------------------------|
 * | 01 23 45 67 89 AB CD EF | 04 08 01 23 45 67 89 AB CD EF |
 */
class OctetStringBER: public BER {
public:
    /**
     * @brief Creates an OctetStringBER object.
     *
     * Creates an OctetStringBER object from a pointer to a null-terminated array of
     * char.
     *
     * The constructor allocates a char array and copies the value parameter.
     *
     * @param value OctetStringBER char pointer value.
     */
    OctetStringBER(const char *value) :
            OctetStringBER(value, strlen(value)) {
    }

    /**
     * @brief Creates an OctetStringBER object.
     *
     * Creates an OctetStringBER object from a pointer to an array of char and the
     * array length.
     *
     * The constructor allocates a char array and copies the value parameter.
     *
     * @param value OctetStringBER char pointer value.
     * @param length Value length.
     */
    OctetStringBER(const char *value, const uint32_t length);

    /**
     * @brief OctetStringBER destructor.
     *
     *  Releases the char array.
     */
    virtual ~OctetStringBER() {
        free(_value);
    }

#if SNMP_STREAM
    /**
     * @brief Encodes OctetStringBER to stream.
     *
     * Type and length are encoded by the inherited BER::encode() then OctetStringBER
     * value is encoded.
     *
     * @param stream Stream to write to.
     */
    virtual void encode(Stream &stream) {
        BER::encode(stream);
        stream.write(_value, _length);
    }

    /**
     * @brief Decodes OctetStringBER from stream.
     *
     * Type and length are decoded by the inherited BER::decode() then OctetStringBER
     * value is decoded.
     *
     * @param stream Stream to read from.
     * @param flag Decoding flag.
     */
    virtual void decode(Stream &stream, const uint8_t flag = Flag::None) {
        BER::decode(stream, flag);
        allocate();
        stream.readBytes(_value, _length);
    }
#else
    /**
     * @brief Encodes OctetStringBER to memory buffer.
     *
     * Type and length are encoded by the inherited BER::encode() then OctetStringBER
     * value is encoded.
     *
     * @param buffer Pointer to the buffer.
     * @return Next position to be written in buffer.
     */
    virtual uint8_t* encode(uint8_t *buffer) {
        uint8_t *pointer = BER::encode(buffer);
        memcpy(pointer, _value, _length);
        return pointer + _length;
    }

    /**
     * @brief Decodes OctetStringBER from memory buffer.
     *
     * Type and length are decoded by the inherited BER::decode() then OctetStringBER
     * value is decoded.
     *
     * @param buffer Pointer to the buffer.
     * @return Next position to be read in buffer.
     */
    virtual uint8_t* decode(uint8_t *buffer) {
        uint8_t *pointer = BER::decode(buffer);
        setValue(reinterpret_cast<const char*>(pointer), _length);
        return pointer + _length;
    }
#endif

    /**
     * @brief Gets the OctetStringBER value.
     *
     * @return OctetStringBER char array pointer value.
     */
    const char* getValue() const {
        return _value;
    }

    /**
     * @brief Sets the OctetStringBER value.
     *
     * @note Length is updated.
     *
     * @param value OctetStringBER char pointer value.
     * @param length Value length.
     */
    void setValue(const char *value, const unsigned int length) {
        _length = length;
        allocate();
        memcpy(_value, value, length);
    }

    /**
     * @brief Gets bit at a given index in the OctetStringBER value.
     *
     * @param index Index of the bit.
     * @return Bit as boolean.
     */
    const bool getBit(const unsigned int index) const {
        const unsigned int byte = index / 8;
        const uint8_t bit = index % 8;
        return _value[byte] & (0x80 >> bit);
    }

protected:
    /** OctetStringBER char array pointer value. */
    char *_value;

    /**
     * @brief Creates an empty OctetStringBER object.
     *
     * @param type OctetStringBER type.
     */
    OctetStringBER(const uint8_t type = Type::OctetString) :
            BER(type) {
        _value = nullptr;
    }

    /**
     * @brief Allocates the char array.
     */
    void allocate() {
        free(_value);
        _value = static_cast<char*>(malloc(_length + 1));
        _value[_length] = 0;
    }
};

/**
 * @class NullBER
 * @brief BER object to handle null.
 *
 * - Type is TYPE_NULL.
 * - Length is fixed, 0 byte, short form encoding.
 * - Size is fixed, 2 bytes.
 * - No value.
 *
 * Example
 *
 * | Null | Encoding |
 * |:-----|:---------|
 * |      | 05 00    |
 */
class NullBER: public BER {
public:
    /**
     * @brief Creates a NullBER object.
     *
     * @param type NullBER type.
     */
    NullBER(const uint8_t type = Type::Null) :
            BER(type) {
    }
};

/**
 * @class ObjectIdentifierBER
 * @brief BER object to handle OID.
 *
 * - Type is TYPE_OBJECTIDENTIFIER.
 * - Length is variable.
 * - Size is variable.
 *
 * Example
 *
 * | Object Identifier            | Encoding                                     |
 * |:-----------------------------|:---------------------------------------------|
 * | 1.3.6.1.2.1.2.2.1.8.4096     | 06 0B 2B 06 01 02 01 02 02 01 08 A0 00       |
 * | 1.3.6.1.4.1.54858.81.1.1.1.0 | 06 0D 2B 06 01 04 01 83 AC 4A 51 01 01 01 00 |
 */
class ObjectIdentifierBER: public BER {
public:
    /**
     * @brief Creates an ObjectIdentifierBER object.
     *
     * Creates an ObjectIdentifierBER object from a pointer to a null-terminated
     * array of char.
     *
     * The constructor copies the value parameter into a string.
     *
     * @param value ObjectIdentifierBER char pointer value.
     */
    ObjectIdentifierBER(const char *value) :
            BER(Type::ObjectIdentifier) {
        setValue(value);
    }

#if SNMP_STREAM
    /**
     * @brief Encodes ObjectIdentifierBER to stream.
     *
     * Type and length are encoded by the inherited BER::encode() then
     * ObjectIdentifierBER value is encoded.
     *
     * @param stream Stream to write to.
     */
    virtual void encode(Stream &stream) {
        unsigned int index = 0;
        uint32_t subidentifier = 0;
        BER::encode(stream);
        char *token = const_cast<char*>(_value.c_str());
        while (token != NULL) {
            switch (index) {
            case 0:
                subidentifier = atoi(token);
                break;
            case 1:
                subidentifier = subidentifier * 40 + atoi(++token);
                stream.write(subidentifier);
                _size++;
                break;
                ;
            default: {
                subidentifier = atol(++token);
                uint32_t value = subidentifier;
                uint32_t length = 0;
                do {
                    value >>= 7;
                    length++;
                } while (value);
                Base::encode7bits(subidentifier, stream, length);
                _size += length;
            }
                break;
            }
            token = strchr(token, '.');
            index++;
        }
    }

    /**
     * @brief Decodes ObjectIdentifierBER from stream.
     *
     * Type and length are decoded by the inherited BER::decode() then
     * ObjectIdentifierBER value is decoded.
     *
     * @param stream Stream to read from.
     * @param flag Decoding flag.
     */
    virtual void decode(Stream &stream, const uint8_t flag = Flag::None) {
        unsigned int index = 0;
        uint32_t subidentifier = 0;
        BER::decode(stream, flag);
        unsigned int length = _length;
        _size += length;
        while (length) {
            if (index++) {
                subidentifier = 0;
                uint8_t byte;
                do {
                    length--;
                    byte = stream.read();
                    subidentifier <<= 7;
                    subidentifier += byte & 0x7F;
                } while (byte & 0x80);
                _value += "." + String(subidentifier);
            } else {
                length--;
                subidentifier = stream.read();
                _value = String(subidentifier / 40) + "." + String(subidentifier % 40);
            }
        }
    }
#else
    /**
     * @brief Encodes ObjectIdentifierBER to memory buffer.
     *
     * Type and length are encoded by the inherited BER::encode() then
     * ObjectIdentifierBER value is encoded.
     *
     * @param buffer Pointer to the buffer.
     * @return Next position to be written in buffer.
     */
    virtual uint8_t* encode(uint8_t *buffer) {
        unsigned int index = 0;
        uint32_t subidentifier = 0;
        uint8_t *pointer = BER::encode(buffer);
        char *token = const_cast<char*>(_value.c_str());
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
                subidentifier = atol(++token);
                uint32_t value = subidentifier;
                uint32_t length = 0;
                do {
                    value >>= 7;
                    length++;
                } while (value);
                Base::encode7bits(subidentifier, pointer, length);
                pointer += length;
            }
                break;
            }
            token = strchr(token, '.');
            index++;
        }
        return ++pointer;
    }

    /**
     * @brief Decodes ObjectIdentifierBER from memory buffer.
     *
     * Type and length are decoded by the inherited BER::decode() then
     * ObjectIdentifierBER value is decoded.
     *
     * @param buffer Pointer to the buffer.
     * @return Next position to be read in buffer.
     */
    virtual uint8_t* decode(uint8_t *buffer) {
        unsigned int index = 0;
        uint32_t subidentifier = 0;
        uint8_t *pointer = BER::decode(buffer);
        uint8_t *end = pointer + _length;
        do {
            if (index++) {
                subidentifier = 0;
                do {
                    subidentifier <<= 7;
                    subidentifier += *pointer & 0x7F;
                } while (*pointer++ & 0x80);
                _value += "." + String(subidentifier);
            } else {
                subidentifier = *pointer++;
                _value = String(subidentifier / 40) + "."
                        + String(subidentifier % 40);
            }
        } while (pointer < end);
        return pointer;
    }
#endif

    /**
     * @brief Gets the ObjectIdentifierBER value.
     *
     * @return ObjectIdentifierBER char array pointer value.
     */
    const char* getValue() const {
        return _value.c_str();
    }

    /**
     * @brief Set the ObjectIdentifierBER value.
     *
     * @note Length is updated.
     *
     * @param value ObjectIdentifierBER char pointer value.
     */
    void setValue(const char *value) {
        _value = value;
        unsigned int index = 0;
        uint32_t subidentifier = 0;
        char *token = const_cast<char*>(_value.c_str());
        while (token != NULL) {
            switch (index) {
            case 0:
                subidentifier = atoi(token);
                break;
            case 1:
                subidentifier = subidentifier * 40 + atoi(++token);
                _length++;
                break;
            default: {
                subidentifier = atol(++token);
                do {
                    subidentifier >>= 7;
                    _length++;
                } while (subidentifier);
            }
                break;
            }
            token = strchr(token, '.');
            index++;
        }
    }

private:
    /** OctetStringBER string value. */
    String _value;
};

/**
 * @class ArrayBER
 * @brief Base class for BER array of BERs.
 *
 * BERs are stored in an array or a vector depending on the definition of
 * SNMP_VECTORS.
 *
 * @tparam U Size of the array. Used only if SNMP_VECTORS is set to 0 or undefined.
 */
template<const uint8_t U>
class ArrayBER: public BER {
public:
    /**
     * @brief Creates an ArrayBER.
     *
     * @param type ArrayBER type.
     */
    ArrayBER(const uint8_t type) :
            BER(type) {
    }

    /**
     * @brief ArrayBER destructor.
     *
     * Delete all BERs of the array.
     */
    ~ArrayBER() {
#if SNMP_VECTOR
        for (auto ber : _bers) {
            delete ber;
        }
        _bers.clear();
#else
        for (uint8_t index = 0; index < _count; ++index) {
            delete _bers[index];
        }
#endif
    }

#if SNMP_STREAM
    /**
     * @brief Encodes ArrayBER to stream.
     *
     * Type and length are encoded by the inherited BER::encode() then each BER of
     * the array is encoded.
     *
     * @param stream Stream to write to.
     */
    virtual void encode(Stream &stream) {
        BER::encode(stream);
        for (uint8_t index = 0; index < _count; ++index) {
            _bers[index]->encode(stream);
        }
    }

    /**
     * @brief Decodes ArrayBER from stream.
     *
     * Type and length are decoded by the inherited BER::decode() then each BER of
     * the array is decoded.
     *
     * Decoding of BER from stream implies 3 steps.
     *
     * - Type of BER is read from stream.
     * - BER object is created from read type.
     * - BER is decoded from stream with flag set to Flag::Typed to skip type decoding
     *  already done.
     *
     * @param stream Stream to read from.
     * @param flag Decoding flag.
     */
    virtual void decode(Stream &stream, const uint8_t flag = Flag::None) {
        BER::decode(stream, flag);
        unsigned int length = _length;
        if (length) {
            Type type;
            _length = 0;
            do {
                type.decode(stream);
                BER *ber = create(type);
                if (ber) {
                    ber->decode(stream, Flag::Typed);
                    length -= ber->getSize();
                    add(ber);
                }
            } while (length);
        }
    }
#else
    /**
     * @brief Encodes ArrayBER to memory buffer.
     *
     * Type and length are encoded by the inherited BER::encode() then each BER of
     * the array is encoded.
     *
     * @param buffer Pointer to the buffer.
     * @return Next position to be written in buffer.
     */
    virtual uint8_t* encode(uint8_t *buffer) {
        uint8_t *pointer = BER::encode(buffer);
#if SNMP_VECTOR
        for (auto ber : _bers) {
            pointer = ber->encode(pointer);
        }
#else
        for (uint8_t index = 0; index < _count; ++index) {
            pointer = _bers[index]->encode(pointer);
        }
#endif
        return pointer;
    }

    /**
     * @brief Decodes ArrayBER from memory buffer.
     *
     * Type and length are decoded by the inherited BER::decode() then each BER of
     * the array is decoded.
     *
     * @param buffer Pointer to the buffer.
     * @return Next position to be read in buffer.
     */
    virtual uint8_t* decode(uint8_t *buffer) {
        uint8_t *pointer = BER::decode(buffer);
        if (_length) {
            uint8_t *end = pointer + _length;
            _length = 0;
            Type type;
            do {
                type.decode(pointer);
                BER *ber = create(type);
                if (ber) {
                    pointer = ber->decode(pointer);
                    add(ber);
                }
            } while (pointer < end);
        }
        return pointer;
    }
#endif

    /**
     * @brief Gets the size of the ArrayBER.
     *
     * The size of the ArrayBER is computed from:
     *
     * - Type size.
     * - Length size.
     * - Length.
     *
     * Length is the sum of the size of all the BERs in the array.
     *
     * @param refresh If true, computes size, if false returns already computed size.
     * @return BER size.
     */
    virtual const unsigned int getSize(const bool refresh = false) {
        if (refresh) {
            _length = 0;
            for (uint8_t index = 0; index < _count; ++index) {
                _length += _bers[index]->getSize(true);
            }
        }
        return BER::getSize();
    }

    /**
     * @brief Array subscript operator.
     *
     * Gets the BER at a given index in the array.
     *
     * @param index Index of the BER.
     * @return BER.
     */
    BER* operator [](const unsigned int index) {
        return _bers[index];
    }

    /**
     * @brief Gets the count of BERs in the array.
     *
     * @return Count of BERs.
     */
    const uint8_t count() const {
        return _count;
    }

protected:
    /**
     * @brief Adds a BER to the array.
     *
     * The length of the array is updated.
     *
     * @param ber BER to add.
     * @return BER given as parameter.
     */
    BER* add(BER *ber) {
#if SNMP_VECTOR
        _bers.push_back(ber);
        _length += ber->getSize();
        _count++;
#else
        if (_count < U) {
            _bers[_count++] = ber;
            _length += ber->getSize();
        }
#endif
        return ber;
    }

    /**
     * @brief Removes the last BER in the array.
     *
     * The length of the array is updated.
     */
    void remove() {
#if SNMP_VECTOR
        if (_bers.size()) {
            _length -= _bers.back()->getSize();
            _bers.pop_back();
            --_count;
        }
#else
        if (_count) {
            _length -= _bers[--_count]->getSize();
        }
#endif
    }

private:
    /** Count of BERs in the array. */
    uint8_t _count = 0;
#if SNMP_VECTOR
    /** Vector of BERs.*/
    std::vector<BER*> _bers;
#else
    /** Array of U BERs.*/
    BER *_bers[U];
#endif

    friend class Message;
    friend class VarBind;
    friend class VarBindList;
};

/**
 * @class SequenceBER
 * @brief BER object to handle sequence of BER objects.
 */
class SequenceBER: public ArrayBER<SNMP_CAPACITY> {
public:
    /**
     * @brief Creates a SequenceBER.
     *
     * @param type SequenceBER type.
     */
    SequenceBER(const uint8_t type = Type::Sequence) :
            ArrayBER(type) {
    }
};

/**
 * @class VarBind
 * @brief BER object to handle variable binding.
 *
 * A variable binding is a specialized array of BER.
 * It contains 2 BER objects.
 *
 * - An OID BER.
 * - A value BER of any type.
 */
class VarBind: public ArrayBER<2> {
public:
    /**
     * @brief Creates a VarBind.
     *
     * @param oid OID BER value.
     * @param value BER of any type. If not set a NullBER is created.
     */
    VarBind(const char *oid, BER *value = nullptr) :
            ArrayBER(Type::Sequence) {
        add(new ObjectIdentifierBER(oid));
        if (value) {
            add(value);
        } else {
            add(new NullBER);
        }
    }

    /**
     * @brief Gets variable binding name.
     *
     * This function is a shortcut to the OID value of the variable binding.
     *
     * @return OID BER value.
     */
    const char* getName() const {
        return static_cast<ObjectIdentifierBER*>(_bers[0])->getValue();
    }

    /**
     * @brief Gets variable binding value.
     *
     * This function is a shortcut to the value BER of the variable binding.
     *
     * @return Value BER.
     */
    BER* getValue() const {
        return _bers[1];
    }
};

/**
 * @class VarBindList
 * @brief BER object to handle a list of variable bindings.
 *
 * A variable binding list is a specialized array of BER.
 * It contains variable binding objects.
 */
class VarBindList: public SequenceBER {
public:
    /**
     * @brief Array subscript operator.
     *
     * Gets the variable binding at a given index in the list.
     *
     * @param index Index of the variable binding.
     * @return Variable binding.
     */
    VarBind* operator [](const uint32_t index) {
        return static_cast<VarBind*>(_bers[index]);
    }
};

/**
 * @class IPAddressBER
 * @brief BER object to handle IP address.
 *
 * - Type is TYPE_IPADDRESS.
 * - Length is fixed, 4 bytes, short form encoding.
 * - Size is fixed, 6 bytes.
 *
 * Example
 *
 * | IP address  | Encoding          |
 * |:------------|:------------------|
 * | 0.0.0.0     | 40 04 00 00 00 00 |
 * | 8.8.8.8     | 40 04 08 08 08 08 |
 * | 192.168.0.1 | 40 04 C0 A8 00 01 |
 */
class IPAddressBER: public OctetStringBER {
public:
    /**
     * @brief Creates an IPAddressBER object.
     *
     * IP address bytes are stored internally in a bytes array.
     *
     * @param value IPAddressBER IPAddress value.
     */
    IPAddressBER(IPAddress value) :
            OctetStringBER(Type::IPAddress) {
        _length = LENGTH;
        allocate();
        for (uint8_t index = 0; index < LENGTH; ++index) {
            _value[index] = value[index];
        }
    }

    /**
     * @brief Gets the IPAddressBER value.
     *
     * @return IPAddressBER bytes array value.
     */
    const uint8_t* getValue() {
        return reinterpret_cast<uint8_t*>(_value);
    }

private:
    /** Fixed length of the IPAddressBER. */
    static constexpr uint8_t LENGTH = 4;
};

/**
 * @class UIntegerBER
 * @brief Base class for unsigned integer BER.
 *
 * @tparam T Unsigned integer type.
 */
template<class T>
class UIntegerBER: public BER {
protected:
    /**
     * @brief Creates an UIntegerBER.
     *
     * @param value UIntegerBER unsigned integer value.
     * @param type UIntegerBER type.
     */
    UIntegerBER(const T value, const uint8_t type) :
            BER(type) {
        setValue(value);
    }

public:
#if SNMP_STREAM
    /**
     * @brief Encodes UIntegerBER to stream.
     *
     * The inherited BER::encodeNumeric<T>() template function encodes type, length
     * and value.
     *
     * @param stream Stream to write to.
     */
    virtual void encode(Stream &stream) {
        BER::encodeNumeric<T>(_value, stream);
    }

    /**
     * @brief Decodes UIntegerBER from stream.
     *
     * The inherited BER::decodeNumeric<T>() template function decodes type, length
     * and value.
     *
     * @param stream Stream to read from.
     * @param flag Decoding flag.
     */
    virtual void decode(Stream &stream, const uint8_t flag = Flag::None) {
        BER::decodeNumeric<T>(&_value, stream, flag);
    }
#else
    /**
     * @brief Encodes UIntegerBER to memory buffer.
     *
     * The inherited BER::encodeNumeric<T>() template function encodes type, length
     * and value.
     *
     * @param buffer Pointer to the buffer.
     * @return Next position to be written in buffer.
     */
    virtual uint8_t* encode(uint8_t *buffer) {
        return BER::encodeNumeric<T>(_value, buffer);
    }

    /**
     * @brief Decodes UIntegerBER from memory buffer.
     *
     * The inherited BER::decodeNumeric<T>() template function decodes type, length
     * and value.
     *
     * @param buffer Pointer to the buffer.
     * @return Next position to be read in buffer.
     */
    virtual uint8_t* decode(uint8_t *buffer) {
        return BER::decodeNumeric<T>(&_value, buffer);
    }
#endif

    /**
     * @brief Gets the UIntegerBER value.
     *
     * @return UIntegerBER unsigned integer value.
     */
    const T getValue() const {
        return _value;
    }

    /**
     * @brief Sets the UIntegerBER value.
     *
     * @note Length is updated.
     *
     * @param value UIntegerBER unsigned integer value.
     */
    void setValue(const T value) {
        _value = value;
        BER::setPositive<T>(value);
    }

private:
    /** UIntegerBER unsigned integer value. */
    T _value;
};

/**
 * @class Counter32BER
 * @brief BER object to handle 32-bit counter.
 *
 * - Type is TYPE_COUNTER32.
 * - Length is variable, from 1 to 5 byte(s), short form encoding.
 * - Size is variable, from 3 to 7 bytes.
 *
 * Example
 *
 * | Counter32  | Encoding             |
 * |-----------:|:---------------------|
 * |          0 | 41 01 00             |
 * | 4294967295 | 41 05 00 FF FF FF FF |
 */
class Counter32BER: public UIntegerBER<uint32_t> {
public:
    /**
     * @brief Creates a Counter32BER object.
     *
     * @param value Counter32BER integer value.
     */
    Counter32BER(const uint32_t value) :
            UIntegerBER(value, Type::Counter32) {
    }
};

/**
 * @class Counter64BER
 * @brief BER object to handle 64-bit counter.
 *
 * - Type is TYPE_COUNTER64.
 * - Length is variable, from 1 to 9 byte(s), short form encoding.
 * - Size is variable, from 3 to 11 bytes.
 *
 * Example
 *
 * | Counter64            | Encoding                         |
 * |---------------------:|:---------------------------------|
 * |                    0 | 46 01 00                         |
 * | 18446744073709551615 | 46 09 00 FF FF FF FF FF FF FF FF |
 */
class Counter64BER: public UIntegerBER<uint64_t> {
public:
    /**
     * @brief Creates a Counter64BER object.
     *
     * @param value Counter64BER integer value.
     */
    Counter64BER(const uint64_t value) :
            UIntegerBER(value, Type::Counter64) {
    }
};

/**
 * @class Gauge32BER
 * @brief BER object to handle 32-bit gauge.
 *
 * - Type is TYPE_GAUGE32.
 * - Length is variable, from 1 to 5 byte(s), short form encoding.
 * - Size is variable, from 3 to 7 bytes.
 *
 * Example
 *
 * | Gauge32    | Encoding             |
 * |-----------:|:---------------------|
 * |          0 | 41 01 00             |
 * | 4294967295 | 41 05 00 FF FF FF FF |
 */
class Gauge32BER: public UIntegerBER<uint32_t> {
public:
    /**
     * @brief Creates a Gauge32BER object.
     *
     * @param value Gauge32BER integer value.
     */
    Gauge32BER(const uint32_t value) :
            UIntegerBER(value, Type::Gauge32) {
    }
};

/**
 * @class TimeTicksBER
 * @brief BER object to handle time.
 *
 * Internally, time is a 32-bit value of hundredths of a second.
 *
 * - Type is TYPE_TIMETICKS.
 * - Length is variable, from 1 to 5 byte(s), short form encoding.
 * - Size is variable, from 3 to 7 bytes.
 *
 * Example
 *
 * | TimeTicks  | Encoding             |
 * |-----------:|:---------------------|
 * |          0 | 41 01 00             |
 * | 4294967295 | 41 05 00 FF FF FF FF |
 */
class TimeTicksBER: public UIntegerBER<uint32_t> {
public:
    /**
     * @brief Creates a TimeTicksBER object.
     *
     * @param value TimeTicksBER integer value.
     */
    TimeTicksBER(const uint32_t value) :
            UIntegerBER(value, Type::TimeTicks) {
    }
};

/**
 * @class OpaqueBER
 * @brief BER object to handle embedded BER object.
 *
 * - Type is TYPE_OPAQUE.
 * - Length is variable, short or long form encoding.
 * - Size is variable.
 *
 * Example
 *
 * | BER                  | Encoding                   |
 * |---------------------:|:---------------------------|
 * | 9F 78 04 C2 C7 FF FE | 44 07 9F 78 04 C2 C7 FF FE |
 *
 * @see [The Domestication of the Opaque Type for SNMP](https://datatracker.ietf.org/doc/html/draft-perkins-opaque-01)
 */

class OpaqueBER: public BER {
public:
    /**
     * @brief Creates an OpaqueBER.
     *
     * @param ber BER object.
     */
    OpaqueBER(BER *ber);

    /**
     * @brief OpaqueBER destructor.
     *
     * Releases embedded BER object.
     */
    virtual ~OpaqueBER() {
        delete _ber;
    }

#if SNMP_STREAM
    /**
     * @brief Encodes OpaqueBER to stream.
     *
     * Type and length are encoded by the inherited BER::encode() then the embedded
     * BER is encoded.
     *
     * @param stream Stream to write to.
     */
    virtual void encode(Stream &stream) {
        BER::encode(stream);
        _ber->encode(stream);
    }

    /**
     * @brief Decodes OpaqueBER from stream.
     *
     * Type and length are decoded by the inherited BER::decode() then the embedded
     * BER is decoded.
     *
     * Decoding of embedded BER from stream implies 3 steps.
     *
     * - Type of embedded BER is read from stream.
     * - Embedded BER object is created from read type.
     * - Embedded BER is decoded from stream with flag set to Flag::Typed to skip
     * type decoding already done.
     *
     * @param stream Stream to read from.
     * @param flag Decoding flag.
     */
    virtual void decode(Stream &stream, const uint8_t flag = Flag::None) {
        BER::decode(stream, flag);
        uint32_t length = _length;
        if (length) {
            Type type;
            do {
                type.decode(stream);
                _ber = create(type);
                if (_ber) {
                    _ber->decode(stream, Flag::Typed);
                    length -= _ber->getSize();
                }
            } while (length);
        }
    }
#else
    /**
     * @brief Encodes OpaqueBER to memory buffer.
     *
     * Type and length are encoded by the inherited BER::encode() then the embedded
     * BER is encoded.
     *
     * @param buffer Pointer to the buffer.
     * @return Next position to be written in buffer.
     */
    virtual uint8_t* encode(uint8_t *buffer) {
        uint8_t *pointer = BER::encode(buffer);
        return _ber->encode(pointer);
    }

    /**
     * @brief Decodes OpaqueBER from memory buffer.
     *
     * Type and length are decoded by the inherited BER::decode() then the embedded
     * BER is decoded.
     *
     * @param buffer Pointer to the buffer.
     * @return Next position to be read in buffer.
     */
    virtual uint8_t* decode(uint8_t *buffer) {
        uint8_t *pointer = BER::decode(buffer);
        uint8_t *end = pointer + _length;
        if (_length) {
            Type type;
            do {
                type.decode(pointer);
                _ber = create(type);
                if (_ber) {
                    pointer = _ber->decode(pointer);
                }
            } while (pointer < end);
        }
        return pointer;
    }
#endif

    /**
     * @brief Gets the size of the OpaqueBER.
     *
     * The size of the OpaqueBER is computed from:
     *
     * - Type size.
     * - Length size.
     * - Length.
     *
     * Length is the size of the embedded BER.
     *
     * @param refresh If true, computes size, if false returns already computed size.
     * @return BER size.
     */
    virtual const unsigned int getSize(const bool refresh = false) {
        if (refresh) {
            _length = _ber->getSize(true);
        }
        return BER::getSize();
    }

    /**
     * @brief Gets the embedded BER.
     *
     * @return BER.
     */
    BER* getBER() {
        return _ber;
    }

private:
    /** Embedded BER. */
    BER *_ber;
};

/**
 * @class FloatBER
 * @brief BER object to handle float.
 *
 * IEEE 754 single precision 32-bit is used to represent float.
 *
 * - Type is TYPE_FLOAT.
 * - Length is fixed, 4 bytes, short form encoding.
 * - Size is fixed, 6 bytes.
 *
 * Example
 *
 * | Float      | Encoding          |
 * |-----------:|:------------------|
 * |          0 | 48 04 00 00 00 00 |
 * |       +inf | 48 04 7F 80 00 00 |
 * |       -inf | 48 04 FF 80 00 00 |
 * |        NaN | 48 04 FF FF FF FF |
 *
 * @note FloatBER is never used alone but always embedded in an OpaqueBER as an
 * OpaqueFloatBER.
 */
class FloatBER: public BER {
public:
    /**
     * @brief Creates a FloatBER object.
     *
     * @param value FloatBER float value.
     * @param type FloatBER type.
     */
    FloatBER(const float value, const unsigned int type = Type::Float) :
            BER(type) {
        _length = LENGTH;
        setValue(value);
    }

#if SNMP_STREAM
    /**
     * @brief Encodes FloatBER to stream.
     *
     * The inherited BER::encodeNumeric<T>() template function encodes type, length
     * and value.
     *
     * @param stream Stream to write to.
     */
    virtual void encode(Stream &stream) {
        BER::encodeNumeric<uint32_t>(*(reinterpret_cast<uint32_t*>(&_value)), stream);
    }

    /**
     * @brief Decodes FloatBER from stream.
     *
     * The inherited BER::decodeNumeric<T>() template function decodes type, length
     * and value.
     *
     * @param stream Stream to read from.
     * @param flag Decoding flag.
     */
    virtual void decode(Stream &stream, const uint8_t flag = Flag::None) {
        BER::decodeNumeric<uint32_t>(reinterpret_cast<uint32_t*>(&_value), stream, flag);
    }
#else
    /**
     * @brief Encodes FloatBER to memory buffer.
     *
     * The inherited BER::encodeNumeric<T>() template function encodes type, length
     * and value.
     *
     * @param buffer Pointer to the buffer.
     * @return Next position to be written in buffer.
     */
    virtual uint8_t* encode(uint8_t *buffer) {
        return BER::encodeNumeric<uint32_t>(
                *(reinterpret_cast<uint32_t*>(&_value)), buffer);
    }

    /**
     * @brief Decodes FloatBER from memory buffer.
     *
     * The inherited BER::decodeNumeric<T>() template function decodes type, length
     * and value.
     *
     * @param buffer Pointer to the buffer.
     * @return Next position to be read in buffer.
     */
    virtual uint8_t* decode(uint8_t *buffer) {
        return BER::decodeNumeric<uint32_t>(
                reinterpret_cast<uint32_t*>(&_value), buffer);
    }
#endif

    /**
     * @brief Gets the FloatBER value.
     *
     * @return FloatBER float value.
     */
    const float getValue() const {
        return _value;
    }

    /**
     * @brief Sets the FloatBER value.
     *
     * @param value FloatBER float value.
     */
    void setValue(const float value) {
        _value = value;
    }

protected:
    /** Fixed length of the FloatBER. */
    static constexpr uint8_t LENGTH = 4;
    /** FloatBER float value. */
    float _value;
};

/**
 * @class NoSuchObjectBER
 * @brief BER object to handle noSuchObject exception.
 */
class NoSuchObjectBER: public NullBER {
public:
    /**
     * @brief Creates a NoSuchObjectBER object.
     */
    NoSuchObjectBER() :
            NullBER(Type::NoSuchObject) {
    }
};

/**
 * @class NoSuchInstanceBER
 * @brief BER object to handle noSuchInstance exception.
 */
class NoSuchInstanceBER: public NullBER {
public:
    /**
     * @brief Creates a NoSuchInstanceBER object.
     */
    NoSuchInstanceBER() :
            NullBER(Type::NoSuchInstance) {
    }
};

/**
 * @class EndOfMIBViewBER
 * @brief BER object to handle endOfMIBView exception.
 */
class EndOfMIBViewBER: public NullBER {
public:
    /**
     * @brief Creates a EndOfMIBViewBER object.
     */
    EndOfMIBViewBER() :
            NullBER(Type::EndOfMIBView) {
    }
};

/**
 * @class OpaqueFloatBER
 * @brief BER object to handle float.
 *
 * IEEE 754 single precision 32-bit is used to represent float.
 *
 * - Type is TYPE_OPAQUEFLOAT.
 * - Length is fixed, 4 bytes, short form encoding.
 * - Size is fixed, 7 bytes.
 *
 * Example
 *
 * | OpaqueFLoat | Encoding             |
 * |:------------|:---------------------|
 * | 25.589001   | 9f 78 04 41 CC B6 46 |
 *
 * @note OpaqueFloatBER is never used alone but always embedded in an OpaqueBER.
 *
 * @see [Support for Floating-Point in SNMP](https://datatracker.ietf.org/doc/html/draft-perkins-float-00)
 */
class OpaqueFloatBER: public FloatBER {
public:
    /**
     * @brief Creates an OpaqueFloatBER object.
     *
     * @param value OpaqueFloatBER float value.
     */
    OpaqueFloatBER(const float value) :
            FloatBER(value, Type::OpaqueFloat) {
    }
};

}  // namespace SNMP

#endif /* BER_H_ */
