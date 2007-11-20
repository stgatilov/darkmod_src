
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_OBJECTFACTORYSERVICE_HPP
#define INCLUDE_RCF_OBJECTFACTORYSERVICE_HPP

#include <map>
#include <string>
#include <vector>

#include <boost/cstdint.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include <RCF/GetInterfaceName.hpp>
#include <RCF/Service.hpp>
#include <RCF/StubFactory.hpp>
#include <RCF/ThreadLibrary.hpp>
#include <RCF/Token.hpp>
#include <RCF/TypeTraits.hpp>

namespace RCF {

    class RcfServer;
    class StubEntry;
    class I_StubFactory;
    class I_RcfClient;
    class Token;

    typedef boost::shared_ptr<StubEntry> StubEntryPtr;
    typedef boost::shared_ptr<I_StubFactory> StubFactoryPtr;

    /// Service allowing remote clients to create objects on the server side,
    /// as opposed to just calling methods on pre-existing objects.
    class ObjectFactoryService :
        public I_Service,
        public I_StubEntryLookupProvider,
        boost::noncopyable
    {
    public:

        /// Constructor.
        /// \param numberOfTokens Maximum number of tokens, thereby also maximum number of created objects.
        /// \param objectTimeoutS Duration of time, in seconds, which must pass after a client invocation, before a created object may be deleted.
        ObjectFactoryService(
            unsigned int numberOfTokens,
            unsigned int objectTimeoutS,
            unsigned int cleanupIntervalS = 30,
            float cleanupThreshold = .67);

        /// Remotely accessible function, via I_ObjectFactory, allows a client to request the creation of an object.
        /// \param objectName Name of the type of the object to be created.
        /// \param token Token assigned to the created object, if the object was created.
        /// \return true if successful, otherwise false.
        //bool createObject(const std::string &objectName, Token &token);
        boost::int32_t createObject(const std::string &objectName, Token &token);
        boost::int32_t deleteObject(const Token &token);

        boost::int32_t createSessionObject(const std::string &objectName);
        boost::int32_t deleteSessionObject();

        /// Binds an object factory to a name.
        /// \param name Name by which clients will access the object factory.
        /// \return true if successful, otherwise false.

#if !defined(_MSC_VER) || _MSC_VER > 1200

        template<typename I1, typename ImplementationT>
        bool bind(const std::string &name_ = "")
        {
            return bind( (I1 *) NULL, (ImplementationT **) NULL, name_);
        }

        template<typename I1, typename I2, typename ImplementationT>
        bool bind(const std::string &name_ = "")
        {
            return bind( (I1 *) NULL, (I2 *) NULL, (ImplementationT **) NULL, name_);
        }

        template<typename I1, typename I2, typename I3, typename ImplementationT>
        bool bind(const std::string &name_ = "")
        {
            return bind( (I1 *) NULL, (I2 *) NULL, (I3 *) NULL, (ImplementationT **) NULL, name_);
        }

        template<typename I1, typename I2, typename I3, typename I4, typename ImplementationT>
        bool bind(const std::string &name_ = "")
        {
            return bind( (I1 *) NULL, (I2 *) NULL, (I3 *) NULL, (I4 *) NULL, (ImplementationT **) NULL, name_);
        }

#endif

        template<typename I1, typename ImplementationT>
        bool bind(I1 *, ImplementationT **, const std::string &name_ = "")
        {
            const std::string &name = (name_ == "") ?
                getInterfaceName((I1 *) NULL) :
                name_;

            StubFactoryPtr stubFactoryPtr(
                new RCF::StubFactory_1<ImplementationT, I1>());

            std::string desc; // TODO
            return insertStubFactory(name, desc, stubFactoryPtr);
        }

        template<typename I1, typename I2, typename ImplementationT>
        bool bind(I1 *, I2 *, ImplementationT **, const std::string &name_ = "")
        {
            const std::string &name = (name_ == "") ?
                getInterfaceName((I1 *) NULL) :
                name_;

            StubFactoryPtr stubFactoryPtr(
                new RCF::StubFactory_2<ImplementationT, I1, I2>());

            std::string desc; // TODO
            return insertStubFactory(name, desc, stubFactoryPtr);
        }

        template<typename I1, typename I2, typename I3, typename ImplementationT>
        bool bind(I1 *, I2 *, I3 *, ImplementationT **, const std::string &name_ = "")
        {
            const std::string &name = (name_ == "") ?
                getInterfaceName((I1 *) NULL) :
                name_;

            StubFactoryPtr stubFactoryPtr(
                new RCF::StubFactory_3<ImplementationT, I1, I2, I3>());

            std::string desc; // TODO
            return insertStubFactory(name, desc, stubFactoryPtr);
        }

        template<typename I1, typename I2, typename I3, typename I4, typename ImplementationT>
        bool bind(I1 *, I2 *, I3 *, I4 *, ImplementationT **, const std::string &name_ = "")
        {
            const std::string &name = (name_ == "") ?
                getInterfaceName((I1 *) NULL) :
                name_;

            StubFactoryPtr stubFactoryPtr(
                new RCF::StubFactory_4<ImplementationT, I1, I2, I3, I4>());

            std::string desc; // TODO
            return insertStubFactory(name, desc, stubFactoryPtr);
        }

    private:
        StubEntryPtr getStubEntryPtr(const Token &token);
        //void removeStubEntryPtr(const Token &token);
        void onServiceAdded(RcfServer &server);
        void onServiceRemoved(RcfServer &server);
        void onServerStart(RcfServer &);
        void onServerStop(RcfServer &);
        void stopCleanup();
        bool cycleCleanup(int timeoutMs, const volatile bool &stopFlag);
       
        bool insertStubFactory(
            const std::string &objectName,
            const std::string &desc,
            StubFactoryPtr stubFactoryPtr);

        bool removeStubFactory(const std::string &objectName);
        StubFactoryPtr getStubFactory(const std::string &objectName);
        void cleanupStubMap(unsigned int timeoutS);

        class TokenFactory : boost::noncopyable
        {
        public:
            TokenFactory(int tokenCount);
            bool requestToken(Token &token);
            void returnToken(const Token &token);
            const std::vector<Token> &getTokenSpace();
            std::size_t getAvailableTokenCount();

        private:
            std::vector<Token> mTokenSpace;
            std::vector<Token> mAvailableTokens;
            ReadWriteMutex mMutex;
        };

        typedef std::map<std::string, StubFactoryPtr> StubFactoryMap;
        // TODO: stub map should be a vector
        typedef std::map<Token, std::pair<MutexPtr, StubEntryPtr> > StubMap;

        // internally synchronized
        TokenFactory        mTokenFactory;

        unsigned int        mClientStubTimeoutS;
        Mutex               mCleanupThresholdMutex;
        Condition           mCleanupThresholdCondition;

        unsigned int        mCleanupIntervalS;
        float               mCleanupThreshold;

        ReadWriteMutex      mStubMapMutex;
        StubMap             mStubMap;

        ReadWriteMutex      mStubFactoryMapMutex;
        StubFactoryMap      mStubFactoryMap;

        volatile bool       mStopFlag;
    };

    typedef boost::shared_ptr<ObjectFactoryService> ObjectFactoryServicePtr;
/*
    void createRemoteObjectNamed(
        I_RcfClient &rcfClient,
        std::string objectName);

    template<typename Interface>
    inline void createRemoteObject(
        I_RcfClient &rcfClient,
        std::string objectName = "")
    {
        if (objectName == "")
        {
            objectName = getInterfaceName((Interface *) NULL);
        }
        createRemoteObjectNamed(rcfClient, objectName);
    }

    void createRemoteSessionObjectNamed(
        I_RcfClient &rcfClient,
        std::string objectName);

    template<typename Interface>
    inline void createRemoteSessionObject(
        I_RcfClient &rcfClient,
        std::string objectName = "")
    {
        if (objectName == "")
        {
            objectName = getInterfaceName((Interface *) NULL);
        }
        createRemoteSessionObjectNamed(rcfClient, objectName);
    }

    void deleteRemoteObject(
        I_RcfClient &rcfClient);

    void deleteRemoteSessionObject(
        I_RcfClient &rcfClient);
*/
} // namespace RCF

RCF_BROKEN_COMPILER_TYPE_TRAITS_SPECIALIZATION(RCF::Token)

#endif // ! INCLUDE_RCF_OBJECTFACTORYSERVICE_HPP
