#ifndef SNMPMESSAGE_H_
#define SNMPMESSAGE_H_

#include "BER.h"

#include <Stream.h>

namespace SNMP {

class Message {
public:
    class OID {
        friend class Message;

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
    };

    Message(uint8_t version = VERSION1, const char *community = nullptr, uint8_t type = 0);
    virtual ~Message();

    VarBind* add(const char *OID, BER *value = nullptr);

    unsigned int build(unsigned char *buffer);
    bool parse(unsigned char *buffer, uint32_t length);

    // Getters and setters
    uint8_t getVersion() const {
        return _version;
    }

    const char* getCommunity() const {
        return _community;
    }

    uint8_t getType() const {
        return _type;
    }

    int32_t getRequestID() const {
        return _requestID;
    }

    void setRequestID(int32_t requestId) {
        _requestID = requestId;
    }

    uint8_t getErrorStatus() const {
        return _errorStatus;
    }

    uint8_t getErrorIndex() const {
        return _errorIndex;
    }

    VarBindList* getVarBindList() const {
        return _varBindList;
    }

    // Set error, status and index
    bool setError(uint8_t errorStatus, uint8_t errorIndex) {
        if (_errorStatus == NO_ERROR) {
            _errorStatus = mapErrorStatus(errorStatus);
            _errorIndex = errorIndex;
            return true;
        }
        return false;
    }

    // Set enterprise OID
    bool setEnterprise(const char* enterprise) {
        if (_type == TYPE_TRAP) {
            _enterprise = enterprise;
            return true;
        }
        return false;
    }

    // Set agent address
    bool setAgentAddress(IPAddress agentAddr) {
        if (_type == TYPE_TRAP) {
            _agentAddr = agentAddr;
            return true;
        }
        return false;
    }

    // Set trap, generic and specific
    bool setTrap(uint8_t genericTrap, uint8_t specificTrap = 0) {
        if (_type == TYPE_TRAP) {
            _genericTrap = genericTrap;
            _specificTrap = specificTrap;
            return true;
        }
        return false;
    }

    // Set SNMP trap oid
    bool setSNMPTrapOID(const char* name) {
        if (((_type == TYPE_SNMPV2TRAP) || (_type == TYPE_INFORMREQUEST)) && _varBindList->count() == 0) {
            // Add sysUpTime.0, value will be set at build time
            add(OID::SYSUPTIME, new TimeTicksBER(0));
            // Add snmpTrapOID.0
            add(OID::SNMPTRAPOID, new ObjectIdentifierBER(name));
            return true;
        }
        return false;
    }

private:
    uint8_t mapErrorStatus(uint8_t errorStatus) {
        if (_version == VERSION1) {
            // RFC 2089 2.1 Mapping SNMPv2 error-status into SNMPv1 error-status
            switch (errorStatus) {
            case WRONG_VALUE:
            case WRONG_ENCODING:
            case WRONG_TYPE:
            case WRONG_LENGTH:
            case INCONSISTENT_VALUE:
                return BAD_VALUE;
            case NO_ACCESS:
            case NOT_WRITABLE:
            case NO_CREATION:
            case INCONSISTENT_NAME:
            case AUTHORIZATION_ERROR:
                return NO_SUCH_NAME;
            case RESOURCE_UNAVAILABLE:
            case COMMIT_FAILED:
            case UNDO_FAILED:
                return GEN_ERR;
            }
        }
        return errorStatus;
    }

    // Common
    uint8_t _version;
    const char *_community;
    uint8_t _type;
    SequenceBER *_message;
    VarBindList *_varBindList;
    // Generic
    int32_t _requestID;
    uint8_t _errorStatus = NO_ERROR;
    uint8_t _errorIndex = 0;
    // Trap
    const char* _enterprise = nullptr;
    IPAddress _agentAddr;
    uint8_t _genericTrap = COLD_START;
    uint8_t _specificTrap = 0;
    uint32_t _timeStamp = 0;
};

}

#endif /* SNMPMESSAGE_H_ */
