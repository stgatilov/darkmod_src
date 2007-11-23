
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_SF_OBINARYSTREAM_HPP
#define INCLUDE_SF_OBINARYSTREAM_HPP

#include <SF/Stream.hpp>

namespace SF {

    class OBinaryStream : public OStream, public WithEncodingBinaryPortable
    {
    public:
        OBinaryStream() : OStream()
        {}

        OBinaryStream(std::ostream &os) : OStream(os)
        {}
    };

}

#endif // ! INCLUDE_SF_OBINARYSTREAM_HPP
