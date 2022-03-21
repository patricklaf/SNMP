#include "SNMPMessage.h"

namespace SNMP {

Message::Message() :
        _community(NULL), _version(VERSION1) {
    initialize();
}

Message::Message(uint8_t version, const char *community, uint8_t type) {
    _version = version;
    _community = community;
    _type = type;
    initialize();
}

Message::~Message() {
    delete _variables;
    delete _message;
}

void Message::initialize() {
    // TODO implement better generator
    _requestID = rand();
    _errorStatus = 0;
    _errorIndex = 0;
    _message = new SequenceBER();
    _variables = new SequenceBER();
}

BER* Message::add(BER *ber) {
    _variables->add(ber);
    return ber;
}

unsigned int Message::build(char *buffer) {
    SequenceBER *data = new SequenceBER(_type);
    IntegerBER *requestID = new IntegerBER(_requestID);
    IntegerBER *errorStatus = new IntegerBER(0);
    IntegerBER *errorIndex = new IntegerBER(0);
    data->add(requestID);
    data->add(errorStatus);
    data->add(errorIndex);
    data->add(_variables);
    IntegerBER *version = new IntegerBER(_version);
    OctetStringBER *community = new OctetStringBER(_community, strlen(_community));
    _message->add(version);
    _message->add(community);
    _message->add(data);
    _variables = nullptr;
    return _message->encode(buffer);
}

bool Message::parse(char *buffer, unsigned int length) {
    if (_message->decode(buffer)) {
        _version = ((IntegerBER*) (*_message)[0])->getValue();
        _community = ((OctetStringBER*) (*_message)[1])->getValue();
        SequenceBER *data = ((SequenceBER*) (*_message)[2]);
        _type = data->getType();
        switch (_type) {
        case TYPE_GETREQUEST:
        case TYPE_GETNEXTREQUEST:
        case TYPE_GETRESPONSE:
        case TYPE_SETREQUEST:
            _requestID = ((IntegerBER*) (*data)[0])->getValue();
            _errorStatus = ((IntegerBER*) (*data)[1])->getValue();
            _errorIndex = ((IntegerBER*) (*data)[2])->getValue();
            // Branch variables
            delete _variables;
            _variables = (SequenceBER*) (*data)[3];
            data->remove();
            return true;
        default:
            // Unknown type, return false
            break;
        }
    }
    return false;
}

}
