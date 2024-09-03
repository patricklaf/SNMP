#ifndef SNMPMESSAGE_H_
#define SNMPMESSAGE_H_

#include "BER.h"

/**
 * @namespace SNMP
 * @brief %SNMP library namespace.
 */
namespace SNMP {

/**
 * @class PDU
 * @brief Helper class to handle PDU.
 */

class PDU {
private:
    /**
     * @struct Generic
     * @brief Helper struct to handle generic PDU.
     *
     * Defines all needed to create a generic PDU.
     *
     * All PDUs are generic except Trap PDU.
     */
    struct Generic {
        /**
         * @struct Bulk
         * @brief  Helper struct to handle GetBulkRequest PDU.
         */
        struct Bulk {
            /** Number of OIDs treated as getRequest. */
            uint8_t _nonRepeaters;
            /** Number of get next operations for each additional OIDs. */
            uint8_t _maxRepetitions;
        };

        /**
         * @brief Initializes to default values.
         *
         * - Initializes Error struct.
         * - Request identifier is set to random value.
         */
        Generic() :
                _error() {
            _requestID = rand();
        }

        /**
         * Request identifier of a message.
         * Response, if exists, must have the same request identifier.
         */
        uint32_t _requestID;

        /**
         * @brief Structures used by generic PDUs.
         *
         * All generic PDUs have an Error struct except GetBulkRequest PDU which have
         * a Bulk struct instead.
         *
         * The union helps to save memory.
         */
        union {
            /** Error struct for generic PDU. */
            Error _error;
            /** Bulk struct for GetBulkRequest PDU. */
            Bulk _bulk;
        };
    };
public:
    /**
     * @brief Creates a PDU object.
     */
    PDU() :
            _generic() {
    }

    /**
     * @brief PDU destructor.
     */
    ~PDU() {
    }

    union {
        /** Generic struct for generic PDU. */
        Generic _generic;
        /** Trap struct for trap PDU. */
        Trap _trap;
    };
};

/**
 * @class Message
 * @brief SNMP message object.
 *
 * @warning Inherited PDU makes extensive use of unions to save memory. Therefore,
 * some members of the class are valid and relevant only if message type is
 * compatible.
 *
 * Example
 *
 * ```cpp
 * // Creates a Trap message
 * Messsage message(Version::V1, "public", Type::Trap);
 * message.setTrap(Trap::ColdStart);    // Good
 * message.setNonRepeaters(2);          // Bad, undefined behavior!
 * ```
 */
class Message: public SequenceBER, private PDU {
public:
    /**
     * @class OID
     * @brief Helper class to defines some useful SNMP OIDs.
     */
    class OID {
    public:
        static constexpr char *COLDSTART = "1.3.6.1.6.3.1.1.5.1";
        static constexpr char *WARMSTART = "1.3.6.1.6.3.1.1.5.2";
        static constexpr char *LINKDOWN = "1.3.6.1.6.3.1.1.5.3";
        static constexpr char *LINKUP = "1.3.6.1.6.3.1.1.5.4";
        static constexpr char *AUTHENTICATIONFAILURE = "1.3.6.1.6.3.1.1.5.5";

    private:
        static constexpr char *SYSUPTIME = "1.3.6.1.2.1.1.3.0";
        static constexpr char *SNMPTRAPOID = "1.3.6.1.6.3.1.1.4.1.0";
        static constexpr char *SNMPTRAPENTERPRISE = "1.3.6.1.6.3.1.1.4.3.0";

        friend class Message;
    };

    /**
     * @brief Creates a Message.
     *
     * @param version %SNMP version.
     * @param community %SNMP community.
     * @param type PDU BER type.
     */
    Message(const uint8_t version = Version::V1, const char *community = nullptr,
            const uint8_t type = Type::GetRequest) :
            SequenceBER(), PDU() {
        _version = version;
        _community = community;
        _type = type;
        _varBindList = new VarBindList();
    }

    /**
     * @brief Message destructor
     *
     * Releases variable bindings.
     */
    virtual ~Message() {
        delete _varBindList;
    }

    /**
     * @brief Gets the size of the message.
     *
     * Builds the message and call inherited ArrayBER::getSize() to ensure the size
     * is valid.
     *
     * @param refresh Refresh parameter for inherited ArrayBER::getSize().
     * @return Message size.
     */
    virtual const unsigned int getSize(const bool refresh = false) {
        build();
        return ArrayBER::getSize(refresh);
    }

    /**
     * @brief Adds a VarBind.
     *
     * Creates and adds a VarBind to the message variable bindings list.
     *
     * @param oid %OID BER value.
     * @param value BER of any type.
     * @return VarBind added.
     */
    VarBind* add(const char *oid, BER *value = nullptr) {
        return static_cast<VarBind*>(_varBindList->add(new VarBind(oid, value)));
    }

    /**
     * @brief Gets the version.
     *
     * @return %SNMP version.
     */
    const uint8_t getVersion() const {
        return _version;
    }

    /**
     * @brief Gets the community.
     *
     * @return %SNMP community.
     */
    const char* getCommunity() const {
        return _community;
    }

    /**
     * @brief Gets the type.
     *
     * @return PDU type.
     */
    const uint8_t getType() const {
        return _type;
    }

    /**
     * @brief Gets the request identifier.
     *
     * @warning Valid only if the PDU is generic.
     *
     * @return Request identifier.
     */
    const int32_t getRequestID() const {
        return _generic._requestID;
    }

    /**
     * @brief Sets the request identifier.
     *
     * @warning Valid only if the PDU is generic.
     *
     * @param requestId Request identifier.
     */
    void setRequestID(const int32_t requestId) {
        _generic._requestID = requestId;
    }

    /**
     * @brief Gets the error status.
     *
     * @warning Valid only if the PDU is generic, except for the GetBulkRequest PDU.
     *
     * @return Error status.
     */
    const uint8_t getErrorStatus() const {
        return _generic._error._status;
    }

    /**
     * @brief Gets the error index.
     *
     * @warning Valid only if the PDU is generic, except for the GetBulkRequest PDU.
     *
     * @return Error index.
     */
    const uint8_t getErrorIndex() const {
        return _generic._error._index;
    }

    /**
     * @brief Sets error status and error index.
     *
     * @warning Valid only if the PDU is generic, except for the GetBulkRequest PDU.
     *
     * @param status Error status.
     * @param index Error index.
     */
    void setError(const uint8_t status, const uint8_t index) {
        _generic._error._status = mapErrorStatus(status);
        _generic._error._index = index;
    }

    /**
     * @brief Gets the non repeaters.
     *
     * @warning Valid only for GetBulkRequest PDU.
     *
     * @return Number of OIDs treated as getRequest.
     */
    const uint8_t getNonRepeaters() const {
        return _generic._bulk._nonRepeaters;
    }

    /**
     * @brief Sets the non repeaters.
     *
     * @warning Valid only for GetBulkRequest PDU.
     *
     * @param nonRepeaters Number of OIDs treated as getRequest.
     */
    void setNonRepeaters(const uint8_t nonRepeaters) {
        _generic._bulk._nonRepeaters = nonRepeaters;
    }

    /**
     * @brief Gets the maximum repetition.
     *
     * @warning Valid only for GetBulkRequest PDU.
     *
     * @return Number of get next operations for each additional OIDs.
     */
    const uint8_t getMaxRepetition() const {
        return _generic._bulk._maxRepetitions;
    }

    /**
     * @brief Sets the maximum repetition.
     *
     * @warning Valid only for GetBulkRequest PDU.
     *
     * @param maxRepetitions Number of get next operations for each additional OIDs.
     */
    void setMaxRepetitions(const uint8_t maxRepetitions) {
        _generic._bulk._maxRepetitions = maxRepetitions;
    }

    /**
     * @brief Sets the enterprise.
     *
     * @warning Valid only for Trap PDU.
     *
     * @param enterprise Enterprise OID.
     */
    void setEnterprise(const char *enterprise) {
        _trap._enterprise = enterprise;
    }

    /**
     * @brief Sets the agent address.
     *
     * @warning Valid only for Trap PDU.
     *
     * @param agentAddr Network address of the agent.
     */
    void setAgentAddress(IPAddress agentAddr) {
        _trap._agentAddr = agentAddr;
    }

    /**
     * @brief Sets the generic and specific traps.
     *
     * @warning Valid only for Trap PDU.
     *
     * @param generic Generic trap code.
     * @param specific Specific trap code.
     */
    void setTrap(const uint8_t generic, const uint8_t specific = 0) {
        _trap._genericTrap = generic;
        _trap._specificTrap = specific;
    }

    /**
     * @brief Sets the SNMPTRAPOID variable binding.

     * @warning Valid only for InformRequest or SNMPv2Trap PDU.
     *
     * - Adds mandatory variable binding *sysUpTime.0*. Value will be set at build
     * time.
     *- Adds variable binding *snmpTrapOID.0* with value name.
     *
     * @param name snmpTrapOID.0 value.
     */
    void setSNMPTrapOID(const char *name) {
//        add(OID::SYSUPTIME, new TimeTicksBER(0));
        add(OID::SYSUPTIME, new TimeTicksBER(millis() / 10));
        add(OID::SNMPTRAPOID, new ObjectIdentifierBER(name));
    }

    /**
     * @brief Gets the variable bindings list.
     *
     * @return Variable bindings list.
     */
    VarBindList* getVarBindList() const {
        return _varBindList;
    }

private:
    /**
     * @brief Builds the message.
     *
     * If required, updates value of *sysUpTime.0* variable binding.
     */
    void build() {
        SequenceBER *pdu = new SequenceBER(_type);
        switch (_type) {
        case Type::Trap:
            pdu->add(new ObjectIdentifierBER(_trap._enterprise));
            pdu->add(new IPAddressBER(_trap._agentAddr));
            pdu->add(new IntegerBER(_trap._genericTrap));
            pdu->add(new IntegerBER(_trap._specificTrap));
            pdu->add(new TimeTicksBER(millis() / 10));
            break;
        case Type::GetBulkRequest:
            pdu->add(new IntegerBER(_generic._requestID));
            pdu->add(new IntegerBER(_generic._bulk._nonRepeaters));
            pdu->add(new IntegerBER(_generic._bulk._maxRepetitions));
            break;
        default:
            pdu->add(new IntegerBER(_generic._requestID));
            pdu->add(new IntegerBER(_generic._error._status));
            pdu->add(new IntegerBER(_generic._error._index));
//            if ((_type == Type::SNMPv2Trap) || (_type == Type::InformRequest)) {
//                // TODO Is it really useful?
//                static_cast<TimeTicksBER*>((*_varBindList)[0]->getValue())->setValue(
//                        millis() / 10);
//                // Refresh size
//                _varBindList->getSize(true);
//            }
            break;
        }
        pdu->add(_varBindList);
        ArrayBER::add(new IntegerBER(_version));
        ArrayBER::add(new OctetStringBER(_community, strlen(_community)));
        ArrayBER::add(pdu);
        _varBindList = nullptr;
    }

    /**
     * @brief Parses the message.
     */
    void parse() {
        _version = static_cast<IntegerBER*>(operator [](0))->getValue();
        _community = static_cast<OctetStringBER*>(operator [](1))->getValue();
        SequenceBER *pdu = static_cast<SequenceBER*>(operator [](2));
        _type = pdu->getType();
        switch (_type) {
        case Type::Trap:
            _trap._enterprise = static_cast<ObjectIdentifierBER*>((*pdu)[0])->getValue();
            _trap._agentAddr = static_cast<IPAddressBER*>((*pdu)[1])->getValue();
            _trap._genericTrap = static_cast<IntegerBER*>((*pdu)[2])->getValue();
            _trap._specificTrap = static_cast<IntegerBER*>((*pdu)[3])->getValue();
            _trap._timeStamp = static_cast<TimeTicksBER*>((*pdu)[4])->getValue();
            break;
        case Type::GetBulkRequest:
            _generic._requestID = static_cast<IntegerBER*>((*pdu)[0])->getValue();
            _generic._bulk._nonRepeaters = static_cast<IntegerBER*>((*pdu)[1])->getValue();
            _generic._bulk._maxRepetitions = static_cast<IntegerBER*>((*pdu)[2])->getValue();
            break;
        default:
            _generic._requestID = static_cast<IntegerBER*>((*pdu)[0])->getValue();
            _generic._error._status = static_cast<IntegerBER*>((*pdu)[1])->getValue();
            _generic._error._index = static_cast<IntegerBER*>((*pdu)[2])->getValue();
            break;
        }
        // Branch variable bind list
        delete _varBindList;
        _varBindList = static_cast<VarBindList*>((*pdu)[_type == Type::Trap ? 5 : 3]);
        pdu->remove();
    }
#if SNMP_STREAM
    /**
     * @brief Builds the message to stream.
     *
     * - Builds the message.
     * - Encodes the message to stream.
     *
     * @param stream Stream to write to.
     */
    void build(Stream &stream) {
        build();
        encode(stream);
    }

    /**
     * @brief Parses the message from stream.
     *
     * - Decodes the message from stream.
     * - Parses the message.
     *
     * @param stream Stream to read from.
     */
    void parse(Stream &stream) {
        decode(stream);
        parse();
    }
#else
    /**
     * @brief Builds the message to buffer.
     *
     * - Encodes the message to buffer.
     *
     * @note The message is already built to compute size needed to allocate the
     * build buffer.
     *
     * @param buffer Pointer to the buffer.
     */
    void build(uint8_t *buffer) {
        encode(buffer);
    }

    /**
     * @brief Parses the message from buffer.
     *
     * - Decodes the message from buffer.
     * - Parses the message.
     *
     * @param buffer Pointer to the buffer.
     */
    void parse(uint8_t *buffer) {
        decode(buffer);
        parse();
    }
#endif

    /**
     * @brief Maps error status.
     *
     * Converts an error status from %SNMP version 2 to %SNMP version 1.
     *
     * @see [V2ToV1 Mapping SNMPv2 onto SNMPv1 within a bi-lingual SNMP agent](https://datatracker.ietf.org/doc/html/rfc2089/)
     *
     * @param status %SNMP version 2 error status.
     * @return %SNMP version 1 error status.
     */
    const uint8_t mapErrorStatus(const uint8_t status) const {
        if (_version == Version::V1) {
            // RFC 2089 2.1 Mapping SNMPv2 error-status into SNMPv1 error-status
            switch (status) {
            case Error::WrongValue:
            case Error::WrongEncoding:
            case Error::WrongType:
            case Error::WrongLength:
            case Error::InconsistentValue:
                return Error::BadValue;
            case Error::NoAccess:
            case Error::NotWritable:
            case Error::NoCreation:
            case Error::InconsistentName:
            case Error::AuthorizationError:
                return Error::NoSuchName;
            case Error::ResourceUnavailable:
            case Error::CommitFailed:
            case Error::UndoFailed:
                return Error::GenErr;
            }
        }
        return status;
    }

    /** %SNMP version. @see Version. */
    uint8_t _version;
    /** %SNMP community. */
    const char *_community;
    /** PDU BER type. @see Type. */
    uint8_t _type;
    /** Variable bindings list. */
    VarBindList *_varBindList;

    friend class SNMP;
};

} // namespace SNMP

#endif /* SNMPMESSAGE_H_ */
