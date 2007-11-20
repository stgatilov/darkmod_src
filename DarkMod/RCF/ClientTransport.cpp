
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#include <RCF/ClientTransport.hpp>

namespace RCF {

    I_ClientTransport::I_ClientTransport() :
        mMaxMessageLength(1000*10)
    {}

    bool I_ClientTransport::isConnected()
    {
        return true;
    }

    void I_ClientTransport::setMaxMessageLength(std::size_t maxMessageLength)
    {
        mMaxMessageLength = maxMessageLength;
    }

    std::size_t I_ClientTransport::getMaxMessageLength() const
    {
        return mMaxMessageLength;
    }

} // namespace RCF
