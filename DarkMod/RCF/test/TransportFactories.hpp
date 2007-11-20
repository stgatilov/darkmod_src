
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_TEST_TRANSPORTFACTORIES_HPP
#define INCLUDE_RCF_TEST_TRANSPORTFACTORIES_HPP

#include <iostream>
#include <typeinfo>
#include <utility>
#include <vector>

#include <boost/config.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/xtime.hpp>

#include <RCF/ClientStub.hpp>
#include <RCF/InitDeinit.hpp>

#ifdef RCF_USE_BOOST_ASIO
#include <RCF/TcpAsioServerTransport.hpp>
#endif

#ifdef BOOST_WINDOWS
#include <RCF/TcpIocpServerTransport.hpp>
#endif

#include <RCF/TcpClientTransport.hpp>
#include <RCF/TcpServerTransport.hpp>
#include <RCF/UdpClientTransport.hpp>
#include <RCF/UdpServerTransport.hpp>

// these headers should be in special test header
#include <RCF/test/PrintTestHeader.hpp>
#include <RCF/util/TraceCommandLineOption.hpp>
#include <RCF/test/SpCollector.hpp>
#include <RCF/util/Platform/OS/Sleep.hpp>

#if defined(_MSC_VER) && _MSC_VER <= 1200
#define for if (0) {} else for
#endif

#include <RCF/ObjectFactoryService.hpp>
template<typename Interface>
inline bool tryCreateRemoteObject(
    RCF::I_RcfClient &rcfClient,
    std::string objectName = "")
{
    try
    {
        rcfClient.getClientStub().createRemoteObject(objectName);
        return true;
    }
    catch (const RCF::Exception &e)
    {
        RCF_TRACE("")(e);
        return false;
    }
}

#include <RCF/util/AutoRun.hpp>
#include <RCF/util/PortNumbers.hpp>

namespace Platform {
    namespace OS {

        inline void SleepMs(std::size_t msec)
        {
            boost::xtime xt = {0};
            // 2007-05-29, Test_ClientTimeout hung on the following line, for unknown reasons
            boost::xtime_get(&xt, boost::TIME_UTC);
            xt.nsec += static_cast<boost::xtime::xtime_nsec_t>(msec*1000000);
            boost::thread::sleep(xt);
        }

    }
}

namespace RCF {

    typedef boost::shared_ptr<ClientTransportAutoPtr> ClientTransportAutoPtrPtr;

    typedef std::pair<ServerTransportPtr, ClientTransportAutoPtrPtr> TransportPair;

    class I_TransportFactory
    {
    public:
        virtual ~I_TransportFactory() {}
        // TODO: split into createServerTransport()/createClientTransport()
        virtual TransportPair createTransports() = 0;
        virtual TransportPair createNonListeningTransports() = 0;
        virtual bool isConnectionOriented() = 0;
    };

    typedef boost::shared_ptr<I_TransportFactory> TransportFactoryPtr;

    typedef std::vector<TransportFactoryPtr> TransportFactories;

    static TransportFactories &getTransportFactories()
    {
        static TransportFactories transportFactories;
        return transportFactories;
    }

    static TransportFactories &getIpTransportFactories()
    {
        static TransportFactories ipTransportFactories;
        return ipTransportFactories;
    }


    inline void writeTransportTypes(
        std::ostream &os,
        I_ServerTransport &serverTransport,
        I_ClientTransport &clientTransport)
    {
        os
            << "Server transport: " << typeid(serverTransport).name() << ", "
            << "Client transport: " << typeid(clientTransport).name()
            << std::endl;
    }

    //**************************************************
    // transport factories

    class TcpTransportFactory : public I_TransportFactory
    {
        TransportPair createTransports()
        {
            std::string ip = util::PortNumbers::getSingleton().getIp();
            int port = util::PortNumbers::getSingleton().getNext();
            return std::make_pair(
                ServerTransportPtr( new TcpServerTransport(port) ),
                ClientTransportAutoPtrPtr(
                    new ClientTransportAutoPtr(
                        new TcpClientTransport(ip, port) )));
        }
        TransportPair createNonListeningTransports()
        {
            return std::make_pair(
                ServerTransportPtr( new TcpServerTransport(0) ),
                ClientTransportAutoPtrPtr());
        }
        bool isConnectionOriented()
        {
            return true;
        }
    };

#ifdef BOOST_WINDOWS

    class TcpIocpTransportFactory : public I_TransportFactory
    {
        TransportPair createTransports()
        {
            std::string ip = util::PortNumbers::getSingleton().getIp();
            int port = util::PortNumbers::getSingleton().getNext();
            return std::make_pair(
                ServerTransportPtr( new TcpIocpServerTransport(ip, port) ),
                ClientTransportAutoPtrPtr(
                    new ClientTransportAutoPtr(
                        new TcpClientTransport(ip, port) )));
        }
        TransportPair createNonListeningTransports()
        {
            return std::make_pair(
                ServerTransportPtr( new TcpIocpServerTransport(0) ),
                ClientTransportAutoPtrPtr());
        }
        bool isConnectionOriented()
        {
            return true;
        }
    };

    AUTO_RUN(
        getTransportFactories().push_back(
            TransportFactoryPtr( new TcpIocpTransportFactory())))

    AUTO_RUN(
        getIpTransportFactories().push_back(
            TransportFactoryPtr( new TcpIocpTransportFactory())))

#endif

#ifdef RCF_USE_BOOST_ASIO

    class TcpAsioTransportFactory : public I_TransportFactory
    {
        TransportPair createTransports()
        {
            std::string ip = util::PortNumbers::getSingleton().getIp();
            int port = util::PortNumbers::getSingleton().getNext();
            return std::make_pair(
                ServerTransportPtr( new TcpAsioServerTransport(port) ),
                ClientTransportAutoPtrPtr(
                    new ClientTransportAutoPtr(
                        new TcpClientTransport(ip, port))));
        }

        TransportPair createNonListeningTransports()
        {
            return std::make_pair(
                ServerTransportPtr( new TcpAsioServerTransport(0) ),
                ClientTransportAutoPtrPtr());
        }

        bool isConnectionOriented()
        {
            return true;
        }

    };

    AUTO_RUN(
        getTransportFactories().push_back(
            TransportFactoryPtr( new TcpAsioTransportFactory())))

    AUTO_RUN(
        getIpTransportFactories().push_back(
            TransportFactoryPtr( new TcpAsioTransportFactory())))

#endif

    class UdpTransportFactory : public I_TransportFactory
    {
        TransportPair createTransports()
        {
            std::string ip = util::PortNumbers::getSingleton().getIp();
            int port = util::PortNumbers::getSingleton().getNext();
            return std::make_pair(
                ServerTransportPtr( new UdpServerTransport(port) ),
                ClientTransportAutoPtrPtr(
                    new ClientTransportAutoPtr(
                        new UdpClientTransport(ip, port) )));
        }

        TransportPair createNonListeningTransports()
        {
            return std::make_pair(
                ServerTransportPtr( new UdpServerTransport(0) ),
                ClientTransportAutoPtrPtr());
        }

        bool isConnectionOriented()
        {
            return false;
        }
    };

#ifndef RCF_TEST_NO_UDP
    AUTO_RUN(
        getTransportFactories().push_back(
            TransportFactoryPtr( new UdpTransportFactory())))

    AUTO_RUN(
        getIpTransportFactories().push_back(
            TransportFactoryPtr( new UdpTransportFactory())))
#endif

} // namespace RCF

#endif // ! INCLUDE_RCF_TEST_TRANSPORTFACTORIES_HPP
