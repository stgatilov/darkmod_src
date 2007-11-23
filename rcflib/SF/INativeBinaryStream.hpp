
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_SF_INATIVEBINARYSTREAM_HPP
#define INCLUDE_SF_INATIVEBINARYSTREAM_HPP

#include <SF/Stream.hpp>

namespace SF {

    class INativeBinaryStream : public IStream, public WithEncodingBinaryNative
    {
    public:
        INativeBinaryStream(std::istream &is) : IStream(is)
        {}
    };

}

#endif // ! INCLUDE_SF_INATIVEBINARYSTREAM_HPP
