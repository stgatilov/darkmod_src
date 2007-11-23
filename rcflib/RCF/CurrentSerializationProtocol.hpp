
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_CURRENTSERIALIZATIONPROTOCOL_HPP
#define INCLUDE_RCF_CURRENTSERIALIZATIONPROTOCOL_HPP

#include <cstddef>

namespace RCF {

    class SerializationProtocolIn;
    class SerializationProtocolOut;

    SerializationProtocolIn *getCurrentSerializationProtocolIn();
    SerializationProtocolOut *getCurrentSerializationProtocolOut();

    bool verifyAgainstArchiveSize(std::size_t bytesRequested);

} // namespace RCF

#endif // ! INCLUDE_RCF_CURRENTSERIALIZATIONPROTOCOL_HPP
