
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_RCFSERVER_HPP
#define INCLUDE_RCF_RCFSERVER_HPP

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/weak_ptr.hpp>

#include <RCF/CheckRtti.hpp>
#include <RCF/GetInterfaceName.hpp>
#include <RCF/RcfClient.hpp>
#include <RCF/ServerStub.hpp>
#include <RCF/ServerTransport.hpp>
#include <RCF/ThreadLibrary.hpp>

namespace RCF {

    class I_ServerTransport;
    class StubEntry;
    class I_Service;
    class I_Session;
    class RcfSession;
    class I_Endpoint;
    class I_StubEntryLookupProvider;
    class I_FilterFactoryLookupProvider;
    class I_RcfClient;

    typedef boost::shared_ptr<I_ServerTransport>                ServerTransportPtr;
    typedef boost::shared_ptr<I_Proactor>                       ProactorPtr;
    typedef boost::shared_ptr<StubEntry>                        StubEntryPtr;
    typedef boost::shared_ptr<I_Service>                        ServicePtr;
    typedef boost::shared_ptr<RcfSession>                       RcfSessionPtr;
    typedef boost::shared_ptr<I_StubEntryLookupProvider>        StubEntryLookupProviderPtr;
    typedef boost::shared_ptr<I_FilterFactoryLookupProvider>    FilterFactoryLookupProviderPtr;
    typedef boost::shared_ptr<I_RcfClient>                      RcfClientPtr;

#if defined(_MSC_VER) && _MSC_VER < 1310

    template<typename T>
    struct BindDirect
    {
        typedef char (&yes_type)[1];
        typedef char (&no_type)[2];
        template<typename U> static yes_type dummyfunc(std::auto_ptr<U> *);
        template<typename U> static yes_type dummyfunc(boost::shared_ptr<U> *);
        template<typename U> static yes_type dummyfunc(boost::weak_ptr<U> *);
        static no_type dummyfunc(...);
        typedef boost::mpl::bool_< sizeof(no_type) == sizeof(dummyfunc( (T *) 0)) > type;
    };

#else

    template<typename T> struct BindDirect                            { typedef boost::mpl::true_ type; };
    template<typename T> struct BindDirect< std::auto_ptr<T> >        { typedef boost::mpl::false_ type; };
    template<typename T> struct BindDirect< boost::shared_ptr<T> >    { typedef boost::mpl::false_ type; };
    template<typename T> struct BindDirect< boost::weak_ptr<T> >    { typedef boost::mpl::false_ type; };

#endif


#define RCF_SERVER_BINDING_SECTION_1_1(i1, T, arg)                                          \
    template<typename i1, typename ImplementationT>                                         \
    bool bind_impl_1(i1*, T t, const std::string &name_, arg*)

#define RCF_SERVER_BINDING_SECTION_1_2(i1, i2, T, arg)                                      \
    template<typename i1, typename i2, typename ImplementationT>                            \
    bool bind_impl_2(i1*, i2 *, T t, const std::string &name_, arg*)

#define RCF_SERVER_BINDING_SECTION_1_3(i1, i2, i3, T, arg)                                  \
    template<typename i1, typename i2, typename i3, typename ImplementationT>               \
    bool bind_impl_3(i1*, i2*, i3*, T t, const std::string &name_, arg*)

#define RCF_SERVER_BINDING_SECTION_1_4(i1, i2, i3, i4, T, arg)                              \
    template<typename i1, typename i2, typename i3, typename i4, typename ImplementationT>  \
    bool bind_impl_4(i1*, i2*, i3*, i4*, T t, const std::string &name_, arg*)

#define RCF_SERVER_BINDING_SECTION_2(/*T, */DerefT)                                             \
    {                                                                                       \
        const std::string &name = (name_ == "") ?                                           \
            I1::getInterfaceName() :                                                        \
            name_;                                                                          \
                                                                                            \
        boost::shared_ptr< I_Deref<ImplementationT> > derefPtr(                             \
            new DerefT(t) );                                                                \
                                                                                            \
        RcfClientPtr rcfClientPtr = createServerStub(                                       \
            (I1 *) 0,                                                                       \
            (ImplementationT *) 0,                                                          \
            derefPtr);                                                                      \

#define RCF_SERVER_BINDING_SECTION_2_(/*T, */DerefT)                                            \
    {                                                                                       \
        const std::string &name = (name_ == "") ?                                           \
            I1::getInterfaceName() :                                                        \
            name_;                                                                          \
                                                                                            \
        boost::shared_ptr< I_Deref<ImplementationT> > derefPtr(                             \
            new DerefT(t) );                                                                \
                                                                                            \
        RcfClientPtr rcfClientPtr = createServerStub(                                       \
            (I1 *) 0,                                                                       \
            (ImplementationT *) 0,                                                          \
            derefPtr);                                                                      \

#define RCF_SERVER_BINDING_SECTION_3(I_n)                                                   \
        {                                                                                   \
            RcfClientPtr mergeePtr = createServerStub(                                      \
                (I_n *) 0,                                                                  \
                (ImplementationT *) 0,                                                      \
                derefPtr);                                                                  \
                                                                                            \
            rcfClientPtr->getServerStub().merge(mergeePtr);                                 \
    }

#define RCF_SERVER_BINDING_SECTION_4                                                        \
        return bindShared(name, rcfClientPtr);                                              \
    }                                                                                       \

    /// Server class, supporting pluggable transports and services.
    class RcfServer :
        public virtual I_SessionManager,
        boost::noncopyable
    {
    public:

        //*************************************
        // public interface

        /// Default constructor.
        RcfServer();

        /// Constructor.
        /// \param endpoint Represents endpoint location of the server. Determines which server transport will be created and used.
        RcfServer(const I_Endpoint &endpoint);

        /// Constructor.
        /// \param servicePtr Service to load, typically a server transport.
        RcfServer(const ServicePtr &servicePtr);

        /// Constructor.
        /// \param serverTransportPtr Server transport to load.
        RcfServer(const ServerTransportPtr &serverTransportPtr);

        /// Constructor.
        /// \param serverTransports Vector of services to load.
        RcfServer(std::vector<ServicePtr> services);

        /// Constructor.
        /// \param serverTransports Vector of server transport services to load.
        RcfServer(std::vector<ServerTransportPtr> serverTransports);

        /// Destructor.
        ~RcfServer();

        /// Starts the server, by starting all services, including transports.
        /// Synchronized: yes, idempotent: yes.
        /// \param spawnThreads Indicates whether any server threads should be spawned.
        void start(bool spawnThreads = true);

        typedef boost::function0<void> JoinFunctor;

        /// Adds a join functor to the server, to be called when stopping the server.
        /// \param joinFunctor Join functor.
        void addJoinFunctor(const JoinFunctor &joinFunctor);

        /// Starts the server, by taking over the current thread. All server tasks will be run sequentially in the calling thread.
        /// Synchronized: no, idempotent: no.
        void startInThisThread();

        /// Starts the server, by taking over the current thread. All server tasks will be run sequentially in the calling thread.
        /// Synchronized: no, idempotent: no.
        /// \param joinFunctor Functor to call in order to join this thread.
        void startInThisThread(const JoinFunctor &joinFunctor);

        /// Stops the server. Optionally waits until all server threads have terminated.
        /// Synchronized: yes, idempotent: yes
        /// \param wait True to wait for server threads to terminate, false to return immediately.
        void stop(bool wait = true);

        /// Cycles all services, including transports. May or may not result in the dispatching of a client request.
        /// \param timeoutMs Maximum waiting time, in milliseconds, for any blocking operations.
        /// \return boolean value indicating if the server is being stopped.
        bool cycle(int timeoutMs = 0);

        /// Cycles the server's thread-specific session queue.
        /// \param timeoutMs Maximum waiting time, in milliseconds, for any blocking operations.
        /// \param stopFlag Reference to boolean value that, when set, indicates that all server threads should terminate as soon as possible.
        void cycleSessions(int timeoutMs, const volatile bool &stopFlag);

        /// Opens all services, including transports.
        void open();

        /// Closes all services, including transports.
        void close();

        /// Returns a reference to the primary server transport.
        /// \return Reference to primary server transport.
        I_ServerTransport &getServerTransport();

        /// Returns a reference to the primary server transport, as a service.
        /// \return Reference to primary server transport.
        I_Service &getServerTransportService();

        /// Returns a shared pointer to the primary server transport.
        /// \return Shared pointer to primary server transport.
        ServerTransportPtr getServerTransportPtr();

        /// Adds a service to the server.
        /// \param servicePtr Service to be added. E.g. ...
        /// \return true if service added successfully, false otherwise (probably because the service has already been added).
        bool addService(const ServicePtr &servicePtr);

        /// Removes a service from the server.
        /// \param servicePtr Service to be removed.
        /// \return true.
        bool removeService(const ServicePtr &servicePtr);

        /// Adds a server transport service to the server.
        /// \param serverTransportPtr Server transport to be added. E.g. ...
        /// \return true if server transport added successfully, false otherwise (probably because the server transport has already been added).
        bool addServerTransport(const ServerTransportPtr &serverTransportPtr);

        /// Removes a server transport service from the server.
        /// \param serverTransportPtr Server transport to be removed.
        /// \return true.
        bool removeServerTransport(const ServerTransportPtr &serverTransportPtr);

        typedef boost::function1<void, RcfServer&> StartCallback;

        /// Sets the start callback, which is invoked by each worker thread when it starts.
        void setStartCallback(const StartCallback &startCallback);
        /*
        /// Sets the start callback, which is invoked by each worker thread when it starts.
        template<typename T> void setStartCallback(void (T::*pfn)(RcfServer &), T &t)
        {
            // this function is implemented here (rather than in RcfServer.inl), because of borland c++ idiosyncracies
            setStartCallback( boost::bind(pfn, &t, _1) );
        }
        */
    private:
        void invokeStartCallback();

    private:
        bool bindShared(const std::string &name, const RcfClientPtr &rcfClientPtr);

        //*************************************
        // async io transport interface

    private:
        SessionPtr createSession();
        void onReadCompleted(const SessionPtr &sessionPtr);
        void onWriteCompleted(const SessionPtr &sessionPtr);

        void handleSession(const RcfSessionPtr &sessionPtr);
        void serializeSessionExceptionResponse(const RcfSessionPtr &sessionPtr);
        void sendSessionResponse(const RcfSessionPtr &sessionPtr, bool isException);
        void closeSession(const RcfSessionPtr &sessionPtr);

        //*************************************
        // transports, queues and threads

    private:
        //SessionQueue &getSessionQueue(const RcfSession &session);
        //typedef ThreadSpecificPtr<SessionQueue>::Val ThreadSpecificSessionQueuePtr;
        //ThreadSpecificSessionQueuePtr mThreadSpecificSessionQueuePtr;
        // eventually other specialized session queues...

        volatile bool mServerThreadsStopFlag;
        Mutex mOpenedMutex;
        bool mOpened;

        Mutex mStartedMutex;
        bool mStarted;

    public:

        /// Returns a value indicating whether or not all server threads should terminate their activities.
        bool getStopFlag() const;

        //*************************************
        // stub management

    private:
        ReadWriteMutex mStubMapMutex;
        typedef std::map<std::string, StubEntryPtr> StubMap;
        StubMap mStubMap;

        //*************************************
        // service management

    private:
        ReadWriteMutex mServicesMutex;
        std::vector<ServicePtr> mServices;
        std::vector<StubEntryLookupProviderPtr> mStubEntryLookupProviders;
        std::vector<FilterFactoryLookupProviderPtr> mFilterFactoryLookupProviders;
        std::vector<ServerTransportPtr> mServerTransports;

        std::vector<JoinFunctor> mJoinFunctors;

        void startService(const ServicePtr &servicePtr) const;
        void stopService(const ServicePtr &servicePtr, bool wait = true) const;

        FilterPtr createFilter(int filterId);

    private:
        // start callback
        StartCallback mStartCallback;

        // start functionality
        //Mutex mStartMutex;
        Platform::Threads::condition mStartEvent;

        // stop functionality
        //Mutex mStopMutex;
        Platform::Threads::condition mStopEvent;

    public:

        /// Waits for the server to be stopped.
        void waitForStopEvent();

        /// Waits for the server to be started.
        void waitForStartEvent();

        // TODO: get rid of this hack
    private:
        friend class MethodInvocationRequest;

    public:
        int getRcfRuntimeVersion();
        void setRcfRuntimeVersion(int version);

    private:
        int mRcfRuntimeVersion;


    public:

        // binding infrastructure

        // following functions are defined inline for reasons of portability (vc6 in particular)

        // direct
        RCF_SERVER_BINDING_SECTION_1_1(I1, ImplementationT &, boost::mpl::true_)
        RCF_SERVER_BINDING_SECTION_2_(DerefObj<ImplementationT>)
        RCF_SERVER_BINDING_SECTION_4

        RCF_SERVER_BINDING_SECTION_1_2(I1, I2, ImplementationT &, boost::mpl::true_)
        RCF_SERVER_BINDING_SECTION_2_(DerefObj<ImplementationT>)
        RCF_SERVER_BINDING_SECTION_3(I2)
        RCF_SERVER_BINDING_SECTION_4

        RCF_SERVER_BINDING_SECTION_1_3(I1, I2, I3, ImplementationT &, boost::mpl::true_)
        RCF_SERVER_BINDING_SECTION_2_(DerefObj<ImplementationT>)
        RCF_SERVER_BINDING_SECTION_3(I2)
        RCF_SERVER_BINDING_SECTION_3(I3)
        RCF_SERVER_BINDING_SECTION_4

        RCF_SERVER_BINDING_SECTION_1_4(I1, I2, I3, I4, ImplementationT &, boost::mpl::true_)
        RCF_SERVER_BINDING_SECTION_2_(DerefObj<ImplementationT>)
        RCF_SERVER_BINDING_SECTION_3(I2)
        RCF_SERVER_BINDING_SECTION_3(I3)
        RCF_SERVER_BINDING_SECTION_3(I4)
        RCF_SERVER_BINDING_SECTION_4

        // indirect - shared_ptr
        RCF_SERVER_BINDING_SECTION_1_1(I1, boost::shared_ptr<ImplementationT>, boost::mpl::false_)
        RCF_SERVER_BINDING_SECTION_2(DerefSharedPtr<ImplementationT>)
        RCF_SERVER_BINDING_SECTION_4

        RCF_SERVER_BINDING_SECTION_1_2(I1, I2, boost::shared_ptr<ImplementationT>, boost::mpl::false_)
        RCF_SERVER_BINDING_SECTION_2(DerefSharedPtr<ImplementationT>)
        RCF_SERVER_BINDING_SECTION_3(I2)
        RCF_SERVER_BINDING_SECTION_4

        RCF_SERVER_BINDING_SECTION_1_3(I1, I2, I3, boost::shared_ptr<ImplementationT>, boost::mpl::false_)
        RCF_SERVER_BINDING_SECTION_2(DerefSharedPtr<ImplementationT>)
        RCF_SERVER_BINDING_SECTION_3(I2)
        RCF_SERVER_BINDING_SECTION_3(I3)
        RCF_SERVER_BINDING_SECTION_4

        RCF_SERVER_BINDING_SECTION_1_4(I1, I2, I3, I4, boost::shared_ptr<ImplementationT>, boost::mpl::false_)
        RCF_SERVER_BINDING_SECTION_2(DerefSharedPtr<ImplementationT>)
        RCF_SERVER_BINDING_SECTION_3(I2)
        RCF_SERVER_BINDING_SECTION_3(I3)
        RCF_SERVER_BINDING_SECTION_3(I4)
        RCF_SERVER_BINDING_SECTION_4

        // indirect - weak_ptr
        RCF_SERVER_BINDING_SECTION_1_1(I1, boost::weak_ptr<ImplementationT>, boost::mpl::false_)
        RCF_SERVER_BINDING_SECTION_2(DerefWeakPtr<ImplementationT>)
        RCF_SERVER_BINDING_SECTION_4

        RCF_SERVER_BINDING_SECTION_1_2(I1, I2, boost::weak_ptr<ImplementationT>, boost::mpl::false_)
        RCF_SERVER_BINDING_SECTION_2(DerefWeakPtr<ImplementationT>)
        RCF_SERVER_BINDING_SECTION_3(I2)
        RCF_SERVER_BINDING_SECTION_4

        RCF_SERVER_BINDING_SECTION_1_3(I1, I2, I3, boost::weak_ptr<ImplementationT>, boost::mpl::false_)
        RCF_SERVER_BINDING_SECTION_2(DerefWeakPtr<ImplementationT>)
        RCF_SERVER_BINDING_SECTION_3(I2)
        RCF_SERVER_BINDING_SECTION_3(I3)
        RCF_SERVER_BINDING_SECTION_4

        RCF_SERVER_BINDING_SECTION_1_4(I1, I2, I3, I4, boost::weak_ptr<ImplementationT>, boost::mpl::false_)
        RCF_SERVER_BINDING_SECTION_2(DerefWeakPtr<ImplementationT>)
        RCF_SERVER_BINDING_SECTION_3(I2)
        RCF_SERVER_BINDING_SECTION_3(I3)
        RCF_SERVER_BINDING_SECTION_3(I4)
        RCF_SERVER_BINDING_SECTION_4

        // indirect - auto_ptr
        RCF_SERVER_BINDING_SECTION_1_1(I1, const std::auto_ptr<ImplementationT> &, boost::mpl::false_)
        RCF_SERVER_BINDING_SECTION_2(DerefAutoPtr<ImplementationT>)
        RCF_SERVER_BINDING_SECTION_4

        RCF_SERVER_BINDING_SECTION_1_2(I1, I2, const std::auto_ptr<ImplementationT> &, boost::mpl::false_)
        RCF_SERVER_BINDING_SECTION_2(DerefAutoPtr<ImplementationT>)
        RCF_SERVER_BINDING_SECTION_3(I2)
        RCF_SERVER_BINDING_SECTION_4

        RCF_SERVER_BINDING_SECTION_1_3(I1, I2, I3, const std::auto_ptr<ImplementationT> &, boost::mpl::false_)
        RCF_SERVER_BINDING_SECTION_2(DerefAutoPtr<ImplementationT>)
        RCF_SERVER_BINDING_SECTION_3(I2)
        RCF_SERVER_BINDING_SECTION_3(I3)
        RCF_SERVER_BINDING_SECTION_4

        RCF_SERVER_BINDING_SECTION_1_4(I1, I2, I3, I4, const std::auto_ptr<ImplementationT> &, boost::mpl::false_)
        RCF_SERVER_BINDING_SECTION_2(DerefAutoPtr<ImplementationT>)
        RCF_SERVER_BINDING_SECTION_3(I2)
        RCF_SERVER_BINDING_SECTION_3(I3)
        RCF_SERVER_BINDING_SECTION_3(I4)
        RCF_SERVER_BINDING_SECTION_4

#if !defined(_MSC_VER) || _MSC_VER > 1200

        template<typename I1, typename ImplementationT>
        bool bind(ImplementationT &t, const std::string &name = "")
        {
            typedef typename BindDirect<ImplementationT>::type type;
            return bind_impl_1((I1 *) NULL, t, name, (type *) NULL);
        }

        template<typename I1, typename I2, typename ImplementationT>
        bool bind(ImplementationT &t, const std::string &name = "")
        {
            typedef typename BindDirect<ImplementationT>::type type;
            return bind_impl_2((I1 *) NULL, (I2 *) NULL, t, name, (type *) NULL);
        }

        template<typename I1, typename I2, typename I3, typename ImplementationT>
        bool bind(ImplementationT &t, const std::string &name = "")
        {
            typedef typename BindDirect<ImplementationT>::type type;
            return bind_impl_3((I1 *) NULL, (I2 *) NULL, (I3 *) NULL, t, name, (type *) NULL);
        }

        template<typename I1, typename I2, typename I3, typename I4, typename ImplementationT>
        bool bind(ImplementationT &t, const std::string &name = "")
        {
            typedef typename BindDirect<ImplementationT>::type type;
            return bind_impl_4((I1 *) NULL, (I2 *) NULL, (I3 *) NULL, (I4 *) NULL, t, name, (type *) NULL);
        }


        template<typename I1, typename ImplementationT>
        bool bind(const ImplementationT &t, const std::string &name = "")
        {
            typedef typename BindDirect<ImplementationT>::type type;
            return bind_impl_1((I1 *) NULL, t, name, (type *) NULL);
        }

        template<typename I1, typename I2, typename ImplementationT>
        bool bind(const ImplementationT &t, const std::string &name = "")
        {
            typedef typename BindDirect<ImplementationT>::type type;
            return bind_impl_2((I1 *) NULL, (I2 *) NULL, t, name, (type *) NULL);
        }

        template<typename I1, typename I2, typename I3, typename ImplementationT>
        bool bind(const ImplementationT &t, const std::string &name = "")
        {
            typedef typename BindDirect<ImplementationT>::type type;
            return bind_impl_3((I1 *) NULL, (I2 *) NULL, (I3 *) NULL, t, name, (type *) NULL);
        }

        template<typename I1, typename I2, typename I3, typename I4, typename ImplementationT>
        bool bind(const ImplementationT &t, const std::string &name = "")
        {
            typedef typename BindDirect<ImplementationT>::type type;
            return bind_impl_4((I1 *) NULL, (I2 *) NULL, (I3 *) NULL, (I4 *) NULL, t, name, (type *) NULL);
        }


        template<typename InterfaceT>
        bool unbind(const std::string &name = "")
        {
            return unbind((InterfaceT *) NULL, name);
        }

#endif

        template<typename I1, typename ImplementationT>
        bool bind(I1 *i1, ImplementationT &t, const std::string &name = "")
        {
            typedef typename BindDirect<ImplementationT>::type type;
            return bind_impl_1(i1, t, name, (type *) NULL);
        }

        template<typename I1, typename I2, typename ImplementationT>
        bool bind(I1 *i1, I2 *i2, ImplementationT &t, const std::string &name = "")
        {
            typedef typename BindDirect<ImplementationT>::type type;
            return bind_impl_2(i1, i2, t, name, (type *) NULL);
        }

        template<typename I1, typename I2, typename I3, typename ImplementationT>
        bool bind(I1 *i1, I2 *i2, I3 *i3, ImplementationT &t, const std::string &name = "")
        {
            typedef typename BindDirect<ImplementationT>::type type;
            return bind_impl_3(i1, i2, i3, t, name, (type *) NULL);
        }

        template<typename I1, typename I2, typename I3, typename I4, typename ImplementationT>
        bool bind(I1 *i1, I2 *i2, I3 *i3, I4 *i4, ImplementationT &t, const std::string &name = "")
        {
            typedef typename BindDirect<ImplementationT>::type type;
            return bind_impl_4(i1, i2, i3, i4, t, name, (type *) NULL);
        }

        template<typename InterfaceT>
        bool unbind(InterfaceT * , const std::string &name_ = "")
        {
            const std::string &name = (name_ == "") ?
                getInterfaceName((InterfaceT *) NULL) :
                name_;

            WriteLock writeLock(mStubMapMutex);
            mStubMap[name].reset();
            return true;
        }


    };

} // namespace RCF

#include <RCF/RcfServer.inl>

#endif // ! INCLUDE_RCF_RCFSERVER_HPP
