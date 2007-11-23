
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#include <RCF/UsingBsdSockets.hpp>

#include <iostream>
#include <string>

#include <boost/config.hpp>

#include <RCF/InitDeinit.hpp>
#include <RCF/Tools.hpp>
#include <RCF/util/Platform/OS/BsdSockets.hpp>

#ifdef BOOST_WINDOWS

namespace RCF {

    inline void initWinsock()
    {
        WORD wVersion = MAKEWORD( 1, 0 );
        WSADATA wsaData;
        int ret = WSAStartup(wVersion, &wsaData);
        int err = Platform::OS::BsdSockets::GetLastError();
        RCF_VERIFY(ret == 0, Exception(0, err, RcfSubsystem_Os, "WSAStartup()") );
    }

    inline void deinitWinsock()
    {
        WSACleanup();
    }

    RCF_ON_INIT_DEINIT( initWinsock(), deinitWinsock() )

} // namespace RCF

#endif

