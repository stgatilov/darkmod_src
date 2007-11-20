
#include <RCF/Protocol/Protocol.hpp>
#include <RCF/SerializationProtocol.hpp>

namespace RCF {

#if defined(RCF_USE_BOOST_SERIALIZATION) && !defined(RCF_USE_SF_SERIALIZATION)

    const int DefaultSerializationProtocol = BsBinary;

#else

    const int DefaultSerializationProtocol = SfBinary;

#endif

} // namespace RCF
