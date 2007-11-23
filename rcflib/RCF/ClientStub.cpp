
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#include <RCF/ClientStub.hpp>

#include <boost/bind.hpp>

#include <RCF/ClientTransport.hpp>
#include <RCF/InitDeinit.hpp>
#include <RCF/SerializationProtocol.hpp>
#include <RCF/ServerInterfaces.hpp>

namespace RCF {

    //****************************************
    // ClientStub

    static unsigned int gClientRemoteCallTimeoutMs = 1000*10; // 10s default timeout
    static unsigned int gClientConnectTimeoutMs = 1000*2; // 2s default timeout

    void setDefaultConnectTimeoutMs(unsigned int connectTimeoutMs)
    {
        gClientConnectTimeoutMs = connectTimeoutMs;
    }

    unsigned int getDefaultConnectTimeoutMs()
    {
        return gClientConnectTimeoutMs;
    }

    void setDefaultRemoteCallTimeoutMs(unsigned int remoteCallTimeoutMs)
    {
        gClientRemoteCallTimeoutMs = remoteCallTimeoutMs;
    }

    unsigned int getDefaultRemoteCallTimeoutMs()
    {
        return gClientRemoteCallTimeoutMs;
    }

    void ClientStub::setAutoReconnect(bool autoReconnect)
    {
        mAutoReconnect = autoReconnect;
    }

    bool ClientStub::getAutoReconnect() const
    {
        return mAutoReconnect;
    }

    void ClientStub::setClientProgressPtr(ClientProgressPtr ClientProgressPtr)
    {
        mClientProgressPtr = ClientProgressPtr;
    }

    ClientProgressPtr ClientStub::getClientProgressPtr() const
    {
        return mClientProgressPtr;
    }

    ClientStub::ClientStub(const std::string &interfaceName) :
        mToken(),
        mDefaultCallingSemantics(Twoway),
        mProtocol(DefaultSerializationProtocol),
        mEndpointName(),
        mObjectName(),
        mInterfaceName(interfaceName),
        mRemoteCallTimeoutMs(gClientRemoteCallTimeoutMs),
        mConnectTimeoutMs(gClientConnectTimeoutMs),
        mAutoReconnect(true),
        mConnected(RCF_DEFAULT_INIT),
        mTries(RCF_DEFAULT_INIT),
        mAutoVersioning(true),
        mRcfRuntimeVersion(gRcfRuntimeVersion)
    {
    }

    ClientStub::ClientStub(const std::string &interfaceName, const std::string &objectName) :
        mToken(),
        mDefaultCallingSemantics(Twoway),
        mProtocol(DefaultSerializationProtocol),
        mEndpointName(),
        mObjectName(objectName),
        mInterfaceName(interfaceName),
        mRemoteCallTimeoutMs(gClientRemoteCallTimeoutMs),
        mConnectTimeoutMs(gClientConnectTimeoutMs),
        mAutoReconnect(true),
        mConnected(RCF_DEFAULT_INIT),
        mTries(RCF_DEFAULT_INIT),
        mAutoVersioning(true),
        mRcfRuntimeVersion(gRcfRuntimeVersion)
    {
    }

    ClientStub::ClientStub(const ClientStub &rhs) :
        mToken(rhs.mToken),
        mDefaultCallingSemantics(rhs.mDefaultCallingSemantics),
        mProtocol(rhs.mProtocol),
        mEndpointName(rhs.mEndpointName),
        mObjectName(rhs.mObjectName),
        mInterfaceName(rhs.mInterfaceName),
        mRemoteCallTimeoutMs(rhs.mRemoteCallTimeoutMs),
        mConnectTimeoutMs(rhs.mConnectTimeoutMs),
        mAutoReconnect(rhs.mAutoReconnect),
        mConnected(RCF_DEFAULT_INIT),
        mTries(RCF_DEFAULT_INIT),
        mAutoVersioning(rhs.mAutoVersioning),
        mRcfRuntimeVersion(rhs.mRcfRuntimeVersion)
    {
        setEndpoint( rhs.getEndpoint() );
    }

    ClientStub &ClientStub::operator=( const ClientStub &rhs )
    {
        if (&rhs != this)
        {
            if (mInterfaceName == rhs.mInterfaceName)
            {
                mToken = rhs.mToken;
                mDefaultCallingSemantics = rhs.mDefaultCallingSemantics;
                mProtocol = rhs.mProtocol;
                mEndpointName = rhs.mEndpointName;
                mObjectName = rhs.mObjectName;
                mRemoteCallTimeoutMs = rhs.mRemoteCallTimeoutMs;
                mConnectTimeoutMs = rhs.mConnectTimeoutMs;
                mAutoReconnect = rhs.mAutoReconnect;
                mConnected = false;
                mAutoVersioning = rhs.mAutoVersioning;
                mRcfRuntimeVersion = rhs.mRcfRuntimeVersion;
                setEndpoint( rhs.getEndpoint());
            }
            else
            {
                RCF_THROW(
                    Exception(RcfError_StubAssignment))
                    (mInterfaceName)(rhs.mInterfaceName);
            }
        }
        return *this;
    }

    Token ClientStub::getTargetToken() const
    {
        return mToken;
    }

    void ClientStub::setTargetToken(Token token)
    {
        mToken = token;
    }

    std::string ClientStub::getTargetName() const
    {
        return mObjectName;
    }

    void ClientStub::setTargetName(const std::string &objectName)
    {
        mObjectName = objectName;
    }

    RemoteCallSemantics ClientStub::getDefaultCallingSemantics() const
    {
        return mDefaultCallingSemantics;
    }

    void ClientStub::setDefaultCallingSemantics(
        RemoteCallSemantics defaultCallingSemantics)
    {
        mDefaultCallingSemantics = defaultCallingSemantics;
    }

    void ClientStub::setSerializationProtocol(int protocol)
    {
        mProtocol = protocol;
    }

    int ClientStub::getSerializationProtocol() const
    {
        return mProtocol;
    }

#ifdef RCF_USE_SF_SERIALIZATION

    void ClientStub::enableSfSerializationPointerTracking(bool enable)
    {
        mOut.mOutProtocol1.setCustomizationCallback(
            boost::bind(enableSfPointerTracking_1, _1, enable) );

        //mOut.mOutProtocol2.setCustomizationCallback(
        //    boost::bind(enableSfPointerTracking_2, _1, enable) );
    }

#else

    void ClientStub::enableSfSerializationPointerTracking(bool enable)
    {}

#endif

    void ClientStub::setEndpoint(const I_Endpoint &endpoint)
    {
        mEndpoint = endpoint.clone();
    }

    void ClientStub::setEndpoint(EndpointPtr endpointPtr)
    {
        mEndpoint = endpointPtr;
    }

    EndpointPtr ClientStub::getEndpoint() const
    {
        return mEndpoint;
    }

    void ClientStub::setTransport(std::auto_ptr<I_ClientTransport> transport)
    {
        mTransport = transport;
        mConnected = mTransport.get() && mTransport->isConnected();
    }

    std::auto_ptr<I_ClientTransport> ClientStub::releaseTransport()
    {
        verifyTransport();
        return mTransport;
    }

    I_ClientTransport& ClientStub::getTransport()
    {
        verifyTransport();
        return *mTransport;
    }

    void ClientStub::createTransport()
    {
        RCF_VERIFY(mEndpoint.get(), Exception(RcfError_NoEndpoint));
        mTransport.reset( mEndpoint->createClientTransport().release() );
        RCF_VERIFY(mTransport.get(), Exception(RcfError_TransportCreation));
    }

    void ClientStub::verifyTransport()
    {
        if (!mTransport.get())
        {
            createTransport();
        }
    }

    void ClientStub::connect()
    {
        verifyTransport();
        if (!mConnected || (mConnected && mAutoReconnect && !mTransport->isConnected()))
        {
            mTransport->disconnect(mConnectTimeoutMs);
            mTransport->connect(mConnectTimeoutMs);

            std::vector<FilterPtr> filters;
            mTransport->getTransportFilters(filters);

            std::for_each(
                filters.begin(),
                filters.end(),
                boost::bind(&Filter::reset, _1));

            mTransport->setTransportFilters(std::vector<FilterPtr>());
            if (!filters.empty())
            {
                requestTransportFilters(filters);
            }
            mConnected = true;
        }
    }

    void ClientStub::disconnect()
    {
        if (mTransport.get())
        {
            mTransport->disconnect(mConnectTimeoutMs);
            mConnected = false;
        }
    }

    bool ClientStub::isConnected()
    {
        return mTransport.get() && mTransport->isConnected();
    }

    void ClientStub::setMessageFilters(const std::vector<FilterPtr> &filters)
    {
        mMessageFilters.assign(filters.begin(), filters.end());
        RCF::connectFilters(mMessageFilters);
    }

    void ClientStub::setMessageFilters(FilterPtr filterPtr)
    {
        std::vector<FilterPtr> filters;
        filters.push_back(filterPtr);
        setMessageFilters(filters);
    }

    const std::vector<FilterPtr> &ClientStub::getMessageFilters()
    {
        return mMessageFilters;
    }

    void ClientStub::setRemoteCallTimeoutMs(unsigned int remoteCallTimeoutMs)
    {
        mRemoteCallTimeoutMs = remoteCallTimeoutMs;
    }

    unsigned int ClientStub::getRemoteCallTimeoutMs() const
    {
        return mRemoteCallTimeoutMs;
    }

    void ClientStub::setConnectTimeoutMs(unsigned int connectTimeoutMs)
    {
        mConnectTimeoutMs = connectTimeoutMs;
    }

    unsigned int ClientStub::getConnectTimeoutMs() const
    {
        return mConnectTimeoutMs;
    }

    void pushBackFilterId(std::vector<boost::int32_t> &filterIds, FilterPtr filterPtr)
    {
        filterIds.push_back( filterPtr->getFilterDescription().getId());
    }

    namespace {

        class RestoreClientTransportGuard
        {
        public:

            RestoreClientTransportGuard(ClientStub &client, ClientStub &clientTemp) :
                mClient(client),
                mClientTemp(clientTemp)
            {}

            ~RestoreClientTransportGuard()
            {
                RCF_DTOR_BEGIN
                mClient.setTransport(mClientTemp.releaseTransport());
                RCF_DTOR_END
            }

        private:
            ClientStub &mClient;
            ClientStub &mClientTemp;
        };

    }

    // TODO: merge common code with queryTransportFilters()
    void ClientStub::requestTransportFilters(const std::vector<FilterPtr> &filters)
    {
        // TODO: currently, the current message filter sequence is not being used,
        // when making the filter request call to the server.

        using namespace boost::multi_index::detail; // for scope_guard

        std::vector<boost::int32_t> filterIds;
        std::for_each(filters.begin(), filters.end(),
            boost::bind(pushBackFilterId, boost::ref(filterIds), _1));

        if (!isConnected())
        {
            connect();
        }
        RCF::RcfClient<RCF::I_RequestTransportFilters> client(*this);
        client.getClientStub().setTransport( releaseTransport());
        client.getClientStub().setTargetToken( Token());

        RestoreClientTransportGuard guard(*this, client.getClientStub());
        RCF_UNUSED_VARIABLE(guard);

        client.getClientStub().setRemoteCallTimeoutMs( getRemoteCallTimeoutMs() );
        int ret = client.requestTransportFilters(RCF::Twoway, filterIds);
        RCF_VERIFY(ret == RcfError_Ok, RemoteException(RcfError_UnknownFilter))(filterIds);
        client.getClientStub().getTransport().setTransportFilters(filters);
    }

    void ClientStub::requestTransportFilters(FilterPtr filterPtr)
    {
        std::vector<FilterPtr> filters;
        if (filterPtr.get())
        {
            filters.push_back(filterPtr);
        }
        requestTransportFilters(filters);
    }

    void ClientStub::clearTransportFilters()
    {
        disconnect();
        if (mTransport.get())
        {
            mTransport->setTransportFilters( std::vector<FilterPtr>());
        }
    }

    bool ClientStub::queryForTransportFilters(const std::vector<FilterPtr> &filters)
    {
        using namespace boost::multi_index::detail; // for scope_guard

        std::vector<boost::int32_t> filterIds;
        std::for_each(filters.begin(), filters.end(),
            boost::bind(pushBackFilterId, boost::ref(filterIds), _1));

        if (!isConnected())
        {
            connect();
        }
        RCF::RcfClient<RCF::I_RequestTransportFilters> client(*this);
        client.getClientStub().setTransport( releaseTransport());
        client.getClientStub().setTargetToken( Token());

        RestoreClientTransportGuard guard(*this, client.getClientStub());
        RCF_UNUSED_VARIABLE(guard);

        client.getClientStub().setRemoteCallTimeoutMs( getRemoteCallTimeoutMs() );
        int ret = client.queryForTransportFilters(RCF::Twoway, filterIds);
        return ret == RcfError_Ok;
    }

    bool ClientStub::queryForTransportFilters(FilterPtr filterPtr)
    {
        std::vector<FilterPtr> filters;
        if (filterPtr.get())
        {
            filters.push_back(filterPtr);
        }
        return queryForTransportFilters(filters);
    }

    void ClientStub::setAutoVersioning(bool autoVersioning)
    {
        mAutoVersioning = autoVersioning;
    }

    bool ClientStub::getAutoVersioning() const
    {
        return mAutoVersioning;
    }

    void ClientStub::setRcfRuntimeVersion(int version)
    {
        mRcfRuntimeVersion = version;
    }

    int ClientStub::getRcfRuntimeVersion() const
    {
        return mRcfRuntimeVersion;
    }

    void ClientStub::setTries(std::size_t tries)
    {
        mTries = tries;
    }

    std::size_t ClientStub::getTries() const
    {
        return mTries;
    }

} // namespace RCF
