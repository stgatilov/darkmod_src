
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_SERVICE_HPP
#define INCLUDE_RCF_SERVICE_HPP

#include <boost/shared_ptr.hpp>

#include <RCF/ServerTask.hpp>
#include <RCF/ThreadLibrary.hpp>

namespace RCF {

    class I_Service;
    class RcfServer;
    class StubEntry;
    class Token;
    typedef boost::shared_ptr<I_Service> ServicePtr;
    typedef boost::shared_ptr<StubEntry> StubEntryPtr;

    /// Base class for RcfServer plug-in services.
    class I_Service
    {
    public:
        /// Constructor.
        I_Service();

        // Virtual destructor.
        virtual ~I_Service()
        {}
       
        /// Invoked when the service is added to a server.
        /// \param server RcfServer object to which the service is being added.
        virtual void onServiceAdded(RcfServer &server) = 0;

        /// Invoked when the service is removed from a server.
        /// \param server RcfServer object from which the service is being removed.
        virtual void onServiceRemoved(RcfServer &server) = 0;

        /// Invoked when the server is opened.
        /// \param server RcfServer object to which the service has been added.
        virtual void onServerOpen(RcfServer &server);

        /// Invoked when the server is closed.
        /// \param server RcfServer object to which the service has been added.
        virtual void onServerClose(RcfServer &server);

        /// Invoked when the server is started, before any server transport threads are spawned.
        /// \param server RcfServer object to which the service has been added.
        virtual void onServerStart(RcfServer &server);

        /// Invoked when the server is stopped, after all server transport threads have been stopped.
        /// \param server RcfServer object to which the service has been added.
        virtual void onServerStop(RcfServer &server);

        // TODO: this needs to be public so users can install custom thread managers, right?
    //protected:
        ReadWriteMutex &getTaskEntriesMutex();
        TaskEntries &getTaskEntries();

    private:
        ReadWriteMutex  mTaskEntriesMutex;
        TaskEntries     mTaskEntries;
    };


    class I_StubEntryLookupProvider;
    typedef boost::shared_ptr<I_StubEntryLookupProvider> StubEntryLookupProviderPtr;

    class I_StubEntryLookupProvider
    {
    public:
        virtual ~I_StubEntryLookupProvider()
        {}

        virtual StubEntryPtr getStubEntryPtr(const Token &token) = 0;
    };


    class I_FilterFactoryLookupProvider;
    typedef boost::shared_ptr<I_FilterFactoryLookupProvider> FilterFactoryLookupProviderPtr;
    class FilterFactory;
    typedef boost::shared_ptr<FilterFactory> FilterFactoryPtr;

    class I_FilterFactoryLookupProvider
    {
    public:
        virtual ~I_FilterFactoryLookupProvider()
        {}

        virtual FilterFactoryPtr getFilterFactoryPtr(int filterId) = 0;
    };

} // namespace RCF

#endif // ! INCLUDE_RCF_SERVICE_HPP
