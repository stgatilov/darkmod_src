
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_SF_ONATIVEBINARYSTREAM_HPP
#define INCLUDE_SF_ONATIVEBINARYSTREAM_HPP

#include <SF/Stream.hpp>

namespace SF {

    class ONativeBinaryStream : public OStream, public WithEncodingBinaryNative
    {
    public:
        ONativeBinaryStream(std::ostream &os) : OStream(os)
        {}
    };


}

#endif // ! INCLUDE_SF_ONATIVEBINARYSTREAM_HPP
