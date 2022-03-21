#ifndef SNMPMESSAGE_H_
#define SNMPMESSAGE_H_

#include "BER.h"

#include <Stream.h>

namespace SNMP {

class Message {
public:
	Message();
	Message(uint8_t version, const char *community, uint8_t type);
	virtual ~Message();

	void initialize();

	BER* add(BER* ber);

	unsigned int build(char* buffer);
	bool parse(char *buffer, unsigned int length);

	uint8_t getVersion() const {
		return _version;
	}

	const char* getCommunity() const {
		return _community;
	}

	uint8_t getType() const {
		return _type;
	}

	void setRequestID(uint32_t requestId) {
		_requestID = requestId;
	}

	uint32_t getRequestID() const {
		return _requestID;
	}

	uint8_t getErrorStatus() const {
		return _errorStatus;
	}

	uint8_t getErrorIndex() const {
		return _errorIndex;
	}

	SequenceBER* getVariables() const {
		return _variables;
	}

protected:
	uint8_t _version;
	const char *_community;
	uint8_t _type;
	uint32_t _requestID;
	uint8_t _errorStatus;
	uint8_t _errorIndex;
	SequenceBER* _message;
	SequenceBER* _variables;
};

}

#endif /* SNMPMESSAGE_H_ */
