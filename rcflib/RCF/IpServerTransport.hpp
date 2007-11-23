
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_IPSERVERTRANSPORT_HPP
#define INCLUDE_RCF_IPSERVERTRANSPORT_HPP

#include <string>
#include <vector>

#include <RCF/ThreadLibrary.hpp>
#include <RCF/Tools.hpp>
#include <RCF/util/Platform/OS/BsdSockets.hpp>

namespace RCF {

    class I_IpServerTransport
    {
    public:
        I_IpServerTransport();
        void setNetworkInterface(const std::string &networkInterface);
        std::string getNetworkInterface() const;
        virtual int getPort() const = 0;
        void setAllowedClientIps(const std::vector<std::string> &allowedClientIps);
        std::vector<std::string> getAllowedClientIps() const;
        bool isClientIpAllowed(const std::string &ip) const;
        bool isClientAddrAllowed(const sockaddr_in &addr) const;

    private:
        std::string                 mNetworkInterface;
        std::vector<std::string>    mAllowedIps;
        std::vector<u_long>         mAllowedAddrs;
        mutable ReadWriteMutex      mReadWriteMutex;
    };

} // namespace RCF

#endif // ! INCLUDE_RCF_IPSERVERTRANSPORT_HPP
