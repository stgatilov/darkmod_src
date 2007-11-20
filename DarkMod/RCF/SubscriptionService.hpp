
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_SUBSCRIPTIONSERVICE_HPP
#define INCLUDE_RCF_SUBSCRIPTIONSERVICE_HPP

#include <map>
#include <memory>
#include <string>
#include <utility>

#include <boost/shared_ptr.hpp>

#include <RCF/GetInterfaceName.hpp>
#include <RCF/ServerStub.hpp>
#include <RCF/Service.hpp>

namespace RCF {

    class ClientProgress;
    class RcfServer;
    class RcfSession;
    class I_ClientTransport;
    class I_ServerTransport;
    class I_Endpoint;
    class I_RcfClient;

    typedef boost::shared_ptr<ClientProgress>       ClientProgressPtr;
    typedef boost::shared_ptr<I_RcfClient>          RcfClientPtr;
    typedef std::auto_ptr<I_ClientTransport>        ClientTransportAutoPtr;
    typedef boost::shared_ptr<RcfSession>           RcfSessionPtr;
    typedef boost::weak_ptr<RcfSession>             RcfSessionWeakPtr;
    typedef boost::shared_ptr<I_ServerTransport>    ServerTransportPtr;

    class Subscription : boost::noncopyable
    {
    public:
        Subscription(
            ClientTransportAutoPtr clientTransportAutoPtr,
            RcfSessionWeakPtr rcfSessionWeakPtr);

        bool isConnected();
       
    private:
        friend class SubscriptionService;
        ClientTransportAutoPtr          mClientTransportAutoPtr;
        boost::weak_ptr<RcfSession>     mRcfSessionWeakPtr;
    };

    typedef boost::shared_ptr<Subscription> SubscriptionPtr;

    static const RCF::ClientProgressPtr gsClientProgressPtr;
   
    /// Service for implementing the subscribe part of publish/subscribe functionality.
    class SubscriptionService :
        public I_Service,
        boost::noncopyable
    {
    public:

        SubscriptionService();

        typedef boost::function0<void> OnDisconnect;

#if !defined(_MSC_VER) || _MSC_VER >= 1310

        template<typename Interface, typename Object>
        SubscriptionPtr beginSubscribe(
            Object &object,
            const I_Endpoint &publisherEndpoint,
            ClientProgressPtr clientProgressPtr = ClientProgressPtr(),
            const std::string &publisherName = "")
        {
            return beginSubscribe( (Interface*) 0, object, publisherEndpoint, clientProgressPtr, publisherName);
        }

        template<typename Interface, typename Object>
        SubscriptionPtr beginSubscribe(
            Object &object,
            const I_Endpoint &publisherEndpoint,
            OnDisconnect onDisconnect,
            ClientProgressPtr clientProgressPtr = ClientProgressPtr(),
            const std::string &publisherName = "")
        {
            return beginSubscribe( (Interface*) 0, object, publisherEndpoint, onDisconnect, clientProgressPtr, publisherName);
        }

        template<typename Interface, typename Object>
        SubscriptionPtr beginSubscribe(
            Object &object,
            ClientTransportAutoPtr clientTransportAutoPtr,
            ClientProgressPtr clientProgressPtr = ClientProgressPtr(),
            const std::string &publisherName = "")
        {
            return beginSubscribe( (Interface*) 0, object, clientTransportAutoPtr, clientProgressPtr, publisherName);
        }

        template<typename Interface, typename Object>
        SubscriptionPtr beginSubscribe(
            Object &object,
            ClientTransportAutoPtr clientTransportAutoPtr,
            OnDisconnect onDisconnect,
            ClientProgressPtr clientProgressPtr = ClientProgressPtr(),
            const std::string &publisherName = "")
        {
            return beginSubscribe( (Interface*) 0, object, clientTransportAutoPtr, onDisconnect, clientProgressPtr, publisherName);
        }

        template<typename Interface, typename Object>
        bool endSubscribe(Object &object)
        {
            return endSubscribe( (Interface *) 0, object);
        }

        template<typename Interface, typename Object>
        SubscriptionPtr getSubscriptionPtr(Object &object)
        {
            return getSubscriptionPtr( (Interface*) 0, object);
        }

#endif


        /// Begins a subscription to a remote publisher.
        /// \param Interface RCF interface of the subscription.
        /// \param Object Type of the subscriber object, must be compatible with Interface.
        /// \param object Subscriber object, that will receive published messages.
        /// \param publisherEndpoint Endpoint describing the server where the desired publishing service is located.
        /// \param publisherName Name of the publishing object to subscribe to.
        template<typename Interface, typename Object>
        SubscriptionPtr beginSubscribe(
            Interface *,
            Object &object,
            const I_Endpoint &publisherEndpoint,
            ClientProgressPtr clientProgressPtr,
            const std::string &publisherName_ = "")
        {
           
            return beginSubscribe(
                (Interface*) 0,
                object,
                publisherEndpoint,
                OnDisconnect(),
                clientProgressPtr,
                publisherName_);
        }

        template<typename Interface, typename Object>
        SubscriptionPtr beginSubscribe(
            Interface *,
            Object &object,
            const I_Endpoint &publisherEndpoint,
            OnDisconnect onDisconnect,
            ClientProgressPtr clientProgressPtr,
            const std::string &publisherName_ = "")
        {
            const std::string &publisherName = (publisherName_ == "") ?
                getInterfaceName((Interface *) NULL) :
                publisherName_;

            SubscriptionId whichSubscription = getSubscriptionId( (Interface*) 0, object);

            boost::shared_ptr< I_Deref<Object> > derefPtr(
                new DerefObj<Object>(object));

            RcfClientPtr rcfClientPtr(
                createServerStub((Interface *) 0, (Object *) 0, derefPtr));

            return beginSubscribeNamed(
                whichSubscription,
                rcfClientPtr,
                publisherEndpoint,
                onDisconnect,
                publisherName,
                clientProgressPtr);
        }

        /// Begins a subscription to a remote publisher.
        /// \param Interface RCF interface of the subscription.
        /// \param Object Type of the subscriber object, must be compatible with Interface.
        /// \param object Subscriber object, that will receive published messages.
        /// \param clientTransportAutoPtr Client transport to be used to access the desired publishing service.
        /// \param publisherName Name of the publishing object to subscribe to.
        template<typename Interface, typename Object>
        SubscriptionPtr beginSubscribe(
            Interface *,
            Object &object,
            ClientTransportAutoPtr clientTransportAutoPtr,
            ClientProgressPtr clientProgressPtr,
            const std::string &publisherName_ = "")
        {
           
            return beginSubscribe(
                (Interface*) 0,
                object,
                clientTransportAutoPtr,
                OnDisconnect(),
                clientProgressPtr,
                publisherName_);
               
        }
       

        template<typename Interface, typename Object>
        SubscriptionPtr beginSubscribe(
            Interface *,
            Object &object,
            ClientTransportAutoPtr clientTransportAutoPtr,
            OnDisconnect onDisconnect,
            ClientProgressPtr clientProgressPtr,
            const std::string &publisherName_ = "")
        {
            const std::string &publisherName = (publisherName_ == "") ?
                getInterfaceName((Interface *) NULL) :
                publisherName_;

            SubscriptionId whichSubscription = getSubscriptionId( (Interface*) 0, object);

            boost::shared_ptr< I_Deref<Object> > derefPtr(
                new DerefObj<Object>(object));

            RcfClientPtr rcfClientPtr(
                createServerStub((Interface *) 0, (Object *) 0, derefPtr));

            return beginSubscribeNamed(
                whichSubscription,
                rcfClientPtr,
                clientTransportAutoPtr,
                onDisconnect,
                publisherName,
                clientProgressPtr);
        }
       
        /// Ends a subscription.
        /// \param Interface RCF interface of the subscription.
        /// \param Object Type of the subscription object.
        /// \param object Reference to the subscription object, for which to end the subscription.
        template<typename Interface, typename Object>
        bool endSubscribe(Interface *, Object &object)
        {
            SubscriptionId whichSubscription = getSubscriptionId( (Interface*) 0, object);
            return endSubscribeNamed(whichSubscription);
        }

        /// Returns the status of the requested subscription.
        /// \param Interface RCF interface of the subscription.
        /// \param Object Type of the subscription object.
        /// \param object Reference to the subscription object, for which to obtain the status.
        /// \return SubscriptionStatus structure.
        //template<typename Interface, typename Object>
        //SubscriptionStatus getSubscriptionStatus(Object &object)
        //{
        //    SubscriptionId whichSubscription = getSubscriptionId<Interface>(object);
        //    return getSubscriptionStatusNamed(whichSubscription);
        //}

        template<typename Interface, typename Object>
        SubscriptionPtr getSubscriptionPtr(Interface *, Object &object)
        {
            SubscriptionId whichSubscription = getSubscriptionId( (Interface*) 0, object);
            ReadLock lock(mSubscriptionsMutex);
            return mSubscriptions[whichSubscription];
        }

        /// Returns the connected status of the given subscription
        /// \param Interface RCF interface of the subscription.
        /// \param Object Type of the subscription object.
        /// \param object Reference to the subscription object, for which to obtain the status.
        /// \return Connected status.
        //template<typename Interface, typename Object>
        //bool isConnected(Object &object)
        //{
        //    SubscriptionId whichSubscription = getSubscriptionId<Interface>(object);
        //    return isConnectedNamed(whichSubscription);
        //}

        void setRcfRuntimeVersion(int rcfRuntimeVersion);
        int getRcfRuntimeVersion();

    private:

        void onServiceAdded(RcfServer &server);
        void onServiceRemoved(RcfServer &server);
        void onServerClose(RcfServer &server);

        // ObjectId = (typename, address)
        typedef std::pair<std::string, void *> ObjectId;
        typedef std::pair<std::string, ObjectId> SubscriptionId;

        template<typename Object>
        ObjectId getObjectId(Object &object)
        {
            return std::make_pair( typeid(Object).name(), &object );
        }

        template<typename Interface, typename Object>
        SubscriptionId getSubscriptionId(Interface *, Object &object)
        {
            return std::make_pair(
                getInterfaceName((Interface *) NULL),
                getObjectId(object));
        }

        SubscriptionPtr beginSubscribeNamed(
            SubscriptionId whichSubscription,
            RcfClientPtr rcfClientPtr,
            ClientTransportAutoPtr publisherClientTransportPtr,
            OnDisconnect onDisconnect,
            const std::string &publisherName,
            ClientProgressPtr clientProgressPtr);

        SubscriptionPtr beginSubscribeNamed(
            SubscriptionId whichSubscription,
            RcfClientPtr rcfClientPtr,
            const I_Endpoint &publisherEndpoint,
            OnDisconnect onDisconnect,
            const std::string &publisherName,
            ClientProgressPtr clientProgressPtr);

        bool endSubscribeNamed(SubscriptionId whichSubscription);

        ServerTransportPtr                          mServerTransportPtr;
        ReadWriteMutex                              mSubscriptionsMutex;
        std::map<SubscriptionId, SubscriptionPtr >  mSubscriptions;

        int                                            mRcfRuntimeVersion;
    };

    typedef boost::shared_ptr<SubscriptionService> SubscriptionServicePtr;
/*
    template<typename Interface, typename Object>
    inline SubscriptionPtr SubscriptionService::beginSubscribe(
        Interface *,
        Object &object,
        const I_Endpoint &publisherEndpoint,
        const std::string &publisherName_,
        ClientProgressPtr clientProgressPtr)
    {
        return beginSubscribe<Interface>(
            object,
            publisherEndpoint,
            OnDisconnect(),
            publisherName_,
            clientProgressPtr);
    }

    template<typename Interface, typename Object>
    inline SubscriptionPtr SubscriptionService::beginSubscribe(
        Interface *,
        Object &object,
        ClientTransportAutoPtr clientTransportAutoPtr,
        const std::string &publisherName_,
        ClientProgressPtr clientProgressPtr)
    {
        return beginSubscribe<Interface>(
            object,
            clientTransportAutoPtr,
            OnDisconnect(),
            publisherName_,
            clientProgressPtr);
    }

    template<typename Interface, typename Object>
    inline SubscriptionPtr SubscriptionService::beginSubscribe(
        Interface *,
        Object &object,
        const I_Endpoint &publisherEndpoint,
        OnDisconnect onDisconnect,
        const std::string &publisherName_,
        ClientProgressPtr clientProgressPtr)
    {
        const std::string &publisherName = (publisherName_ == "") ?
            getInterfaceName((Interface *) NULL) :
        publisherName_;

        SubscriptionId whichSubscription = getSubscriptionId<Interface>(object);

        boost::shared_ptr< I_Deref<Object> > derefPtr(
            new DerefObj<Object>(object));

        RcfClientPtr rcfClientPtr(
            createServerStub((Interface *) 0, (Object *) 0, derefPtr));

        return beginSubscribeNamed(
            whichSubscription,
            rcfClientPtr,
            publisherEndpoint,
            onDisconnect,
            publisherName,
            clientProgressPtr);
    }

    template<typename Interface, typename Object>
    inline SubscriptionPtr SubscriptionService::beginSubscribe(
        Interface *,
        Object &object,
        ClientTransportAutoPtr clientTransportAutoPtr,
        OnDisconnect onDisconnect,
        const std::string &publisherName_,
        ClientProgressPtr clientProgressPtr)
    {
        const std::string &publisherName = (publisherName_ == "") ?
            getInterfaceName((Interface *) NULL) :
            publisherName_;

        SubscriptionId whichSubscription = getSubscriptionId<Interface>(object);

        boost::shared_ptr< I_Deref<Object> > derefPtr(
            new DerefObj<Object>(object) );

        RcfClientPtr rcfClientPtr(
            createServerStub((Interface *) 0, (Object *) 0, derefPtr) );

        return beginSubscribeNamed(
            whichSubscription,
            rcfClientPtr,
            clientTransportAutoPtr,
            onDisconnect,
            publisherName,
            clientProgressPtr);
    }
*/
} // namespace RCF

#endif // ! INCLUDE_RCF_SUBSCRIPTIONSERVICE_HPP
