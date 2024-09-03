#include "BER.h"

/**
 * @namespace SNMP
 * @brief %SNMP library namespace.
 */
namespace SNMP {

/**
 * @brief Creates a BER of given type.
 *
 * @param type BER type.
 * @return Pointer to created BER or nullptr if the type is unknown.
 */
BER* BER::create(const Type &type) {
    switch (type) {
    case Type::Boolean:
        return new BooleanBER(false);
    case Type::Integer:
        return new IntegerBER(0);
    case Type::OctetString:
        return new OctetStringBER(nullptr, 0);
    case Type::Null:
    case Type::NoSuchObject:
    case Type::NoSuchInstance:
    case Type::EndOfMIBView:
        return new NullBER(type);
    case Type::ObjectIdentifier:
        return new ObjectIdentifierBER(nullptr);
    case Type::IPAddress:
        return new IPAddressBER(IPAddress());
    case Type::Counter32:
        return new Counter32BER(0);
    case Type::Gauge32:
        return new Gauge32BER(0);
    case Type::TimeTicks:
        return new TimeTicksBER(0);
    case Type::Opaque:
        return new OpaqueBER(nullptr);
    case Type::Counter64:
        return new Counter64BER(0);
    case Type::Float:
        return new FloatBER(0);
    case Type::OpaqueFloat:
        return new OpaqueFloatBER(0);
    case Type::Sequence:
    case Type::GetRequest:
    case Type::GetNextRequest:
    case Type::GetResponse:
    case Type::SetRequest:
    case Type::Trap:
    case Type::GetBulkRequest:
    case Type::InformRequest:
    case Type::SNMPv2Trap:
    case Type::Report:
        return new SequenceBER(type);
    }
    return nullptr;
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
OctetStringBER::OctetStringBER(const char *value, const uint32_t length) :
        OctetStringBER() {
    setValue(value, length);
}

/**
 * @brief
 *
 * @param ber
 */
OpaqueBER::OpaqueBER(BER *ber) :
        BER(Type::Opaque) {
    _ber = ber;
    if (ber) {
        _length = _ber->getSize();
    }
}

}  // namespace SNMP
