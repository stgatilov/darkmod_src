
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_SF_ITEXTSTREAM_HPP
#define INCLUDE_SF_ITEXTSTREAM_HPP

#include <SF/Stream.hpp>

namespace SF {

    class ITextStream : public IStream, public WithEncodingText
    {
    public:
        ITextStream() : IStream()
        {}

        ITextStream(std::istream &is) : IStream(is)
        {}
    };

}

#endif // ! INCLUDE_SF_ITEXTSTREAM_HPP
