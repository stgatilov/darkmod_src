
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#include <RCF/IpServerTransport.hpp>

namespace RCF {

    I_IpServerTransport::I_IpServerTransport() :
    	mNetworkInterface("127.0.0.1"),
    	mReadWriteMutex(WriterPriority)
    {}

    void I_IpServerTransport::setNetworkInterface(
        const std::string &networkInterface)
    {
        WriteLock writeLock(mReadWriteMutex);
        mNetworkInterface = networkInterface;
    }

    std::string I_IpServerTransport::getNetworkInterface() const
    {
        ReadLock readLock(mReadWriteMutex);
        return mNetworkInterface;
    }

    void I_IpServerTransport::setAllowedClientIps(
        const std::vector<std::string> &allowedClientIps)
    {
        WriteLock writeLock(mReadWriteMutex);
        mAllowedIps.assign(allowedClientIps.begin(), allowedClientIps.end());

        // translate the ips into 32 bit values, network order
        mAllowedAddrs.clear();
        for (unsigned int i=0; i<mAllowedIps.size(); i++)
        {
            const std::string &ip = mAllowedIps[i];
            hostent *hostDesc = ::gethostbyname( ip.c_str() );
            if (hostDesc == NULL)
            {
                int err = Platform::OS::BsdSockets::GetLastError();
                std::string strErr = Platform::OS::GetErrorString(err);
                RCF_TRACE("gethostbyname() failed")(ip)(err)(strErr);
                mAllowedAddrs.push_back(INADDR_NONE);
            }
            else
            {
                in_addr addr = *((in_addr*) hostDesc->h_addr_list[0]);
                mAllowedAddrs.push_back(addr.s_addr);
            }
        }
    }

    std::vector<std::string> I_IpServerTransport::getAllowedClientIps() const
    {
        ReadLock readLock(mReadWriteMutex);
        return mAllowedIps;
    }

    bool I_IpServerTransport::isClientIpAllowed(const std::string &ip) const
    {
        ReadLock readLock(mReadWriteMutex);
        return
            mAllowedIps.empty() ||
            (std::find(mAllowedIps.begin(), mAllowedIps.end(), ip) !=
                mAllowedIps.end());
    }

    bool I_IpServerTransport::isClientAddrAllowed(const sockaddr_in &addr) const
    {
        ReadLock readLock(mReadWriteMutex);
        return
            mAllowedAddrs.empty() ||
            (std::find(
                mAllowedAddrs.begin(),
                mAllowedAddrs.end(),
                addr.sin_addr.s_addr) != mAllowedAddrs.end());
    }

} // namespace RCF
