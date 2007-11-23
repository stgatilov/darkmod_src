
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#include <RCF/IpAddress.hpp>

namespace RCF {

    IpAddress::IpAddress() :
        mIp(),
        mPort(RCF_DEFAULT_INIT)
    {
        memset(&mAddr, 0, sizeof(mAddr));
    }

    IpAddress::IpAddress(const sockaddr_in &addr) :
        mAddr(addr),
        mIp(),
        mPort(RCF_DEFAULT_INIT)
    {
    }

    std::string IpAddress::getIp() const
    {
        if (mIp == "")
        {
            mIp = inet_ntoa(mAddr.sin_addr);
        }
        return mIp;
    }

    int IpAddress::getPort() const
    {
        if (mPort == 0)
        {
            mPort = ntohs(mAddr.sin_port);
        }
        return mPort;
    }

    const sockaddr_in &IpAddress::getSockAddr() const
    {
        return mAddr;
    }

} // namespace RCF
