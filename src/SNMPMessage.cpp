#include "SNMPMessage.h"

namespace SNMP {

Message::Message(uint8_t version, const char *community, uint8_t type) {
    _version = version;
    _community = community;
    _type = type;
    _requestID = rand();
    _message = new SequenceBER();
    _varBindList = new VarBindList();
}

Message::~Message() {
    delete _varBindList;
    delete _message;
}

VarBind* Message::add(const char *oid, BER *value) {
    return static_cast<VarBind*>(_varBindList->add(new VarBind(oid, value)));
}

unsigned int Message::build(char *buffer) {
    SequenceBER *data = new SequenceBER(_type);
    if (_type == TYPE_TRAP) {
        data->add(new ObjectIdentifierBER(_enterprise));
        data->add(new IPAddressBER(_agentAddr));
        data->add(new IntegerBER(_genericTrap));
        data->add(new IntegerBER(_specificTrap));
        data->add(new TimeTicksBER(millis() / 10));
     } else {
        data->add(new IntegerBER(_requestID));
        data->add(new IntegerBER(_errorStatus));
        data->add(new IntegerBER(_errorIndex));
        if ((_type == TYPE_SNMPV2TRAP) || (_type == TYPE_INFORMREQUEST)) {
            // Set value of sysUpTime.0 variable binding
            static_cast<TimeTicksBER*>((*_varBindList)[0]->getValue())->setValue(millis() / 10);
            // Force size adjust
            _varBindList->getSize(true);
        }
    }
    data->add(_varBindList);
    _message->add(new IntegerBER(_version));
    _message->add(new OctetStringBER(_community, strlen(_community)));
    _message->add(data);
    _varBindList = nullptr;
    return _message->encode(buffer);
}

bool Message::parse(char *buffer, unsigned int length) {
    if (_message->decode(buffer)) {
        _version = static_cast<IntegerBER*>((*_message)[0])->getValue();
        _community = static_cast<OctetStringBER*>((*_message)[1])->getValue();
        SequenceBER *data = static_cast<SequenceBER*>((*_message)[2]);
        _type = data->getType();
        if (_type == TYPE_TRAP) {
            _enterprise = static_cast<ObjectIdentifierBER*>((*data)[0])->getValue();
            _agentAddr = static_cast<IPAddressBER*>((*data)[1])->getValue();
            _genericTrap = static_cast<IntegerBER*>((*data)[2])->getValue();
            _specificTrap = static_cast<IntegerBER*>((*data)[3])->getValue();
            _timeStamp = static_cast<TimeTicksBER*>((*data)[4])->getValue();
        } else {
            _requestID = static_cast<IntegerBER*>((*data)[0])->getValue();
            _errorStatus = static_cast<IntegerBER*>((*data)[1])->getValue();
            _errorIndex = static_cast<IntegerBER*>((*data)[2])->getValue();
        }
        // Branch variable bind list
        delete _varBindList;
        _varBindList = static_cast<VarBindList*>((*data)[_type == TYPE_TRAP ? 5 : 3]);
        data->remove();
        return true;
    }
    return false;
}

}
