
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#include <RCF/ServerTransport.hpp>

namespace RCF {

    I_ServerTransport::I_ServerTransport() :
        mMaxMessageLength(10*1000)
    {}

    void I_ServerTransport::setMaxMessageLength(std::size_t maxMessageLength)
    {
        mMaxMessageLength = maxMessageLength;
    }

    std::size_t I_ServerTransport::getMaxMessageLength() const
    {
        return mMaxMessageLength;
    }

} // namespace RCF
