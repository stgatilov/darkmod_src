
//******************************************************************************
// RCF - Remote Ca  ll Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#include <RCF/MulticastClientTransport.hpp>

#include <RCF/Exception.hpp>
#include <RCF/Tools.hpp>

namespace RCF {

    ClientTransportAutoPtr MulticastClientTransport::clone() const
    {
        RCF_ASSERT(0);
        return ClientTransportAutoPtr();
    }

    EndpointPtr MulticastClientTransport::getEndpointPtr() const
    {
        RCF_ASSERT(0);
        return EndpointPtr();
    }

    void reswap(
        std::list<ClientTransportPtr> &to,
        std::list<ClientTransportPtr> &from,
        Mutex &mutex)
    {
        Lock lock(mutex);
        to.swap(from);
        if (!from.empty())
        {
            std::copy(
                from.begin(),
                from.end(),
                std::back_inserter(to));
        }
    }

    int MulticastClientTransport::send(
        const std::vector<ByteBuffer> &data,
        unsigned int timeoutMs)
    {
        // TODO: looks like we need to make a full copy of data, otherwise individual sub-transports
        // might transform data itself
        // ...

        ClientTransportPtrList clientTransports;
        {
            Lock lock(mMutex);
            clientTransports.swap(mClientTransports);
        }

        using namespace boost::multi_index::detail;
        scope_guard guard = make_guard(
            reswap,
            boost::ref(mClientTransports),
            boost::ref(clientTransports),
            boost::ref(mMutex));
        RCF_UNUSED_VARIABLE(guard);

        timeoutMs = 1000; // TODO: for now, allow each sub transport a max of 1s to send the data
        bool needToRemove = false;
        for (
            ClientTransportPtrList::iterator it = clientTransports.begin();
            it != clientTransports.end();
            ++it)
        {
            try
            {
                if ((*it)->isConnected())
                {
                    (*it)->send(data, timeoutMs);
                }
                else
                {
                    needToRemove = true;
                    (*it).reset();
                }
            }
            catch(const Exception &e)
            {
                RCF_TRACE("")(e);
                needToRemove = true;
                (*it).reset();
            }
        }

        if (needToRemove)
        {
            clientTransports.remove( ClientTransportPtr() );
        }       

        return 1;
    }

    int MulticastClientTransport::receive(
        ByteBuffer &byteBuffer,
        unsigned int timeoutMs)
    {
        RCF_UNUSED_VARIABLE(byteBuffer);
        RCF_UNUSED_VARIABLE(timeoutMs);
        RCF_ASSERT(0);
        return 1;
    }

    bool MulticastClientTransport::isConnected()
    {
        return true;
    }

    void MulticastClientTransport::connect(unsigned int timeoutMs)
    {
        RCF_UNUSED_VARIABLE(timeoutMs);
    }

    void MulticastClientTransport::disconnect(unsigned int timeoutMs)
    {
        RCF_UNUSED_VARIABLE(timeoutMs);
    }

    void MulticastClientTransport::addTransport(
        const ClientTransportPtr &clientTransportPtr)
    {
        Lock lock(mMutex);
        mClientTransports.push_back(clientTransportPtr);
    }

    void MulticastClientTransport::setTransportFilters(
        const std::vector<FilterPtr> &)
    {
        // not supported
    }

    void MulticastClientTransport::getTransportFilters(
        std::vector<FilterPtr> &)
    {
        // not supported
    }

} // namespace RCF
