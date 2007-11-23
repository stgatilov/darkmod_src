
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_SERVERSTUB_HPP
#define INCLUDE_RCF_SERVERSTUB_HPP

// TODO: move code to .cpp

#include <map>
#include <memory>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <RCF/GetInterfaceName.hpp>
#include <RCF/RcfClient.hpp>
#include <RCF/SerializationProtocol.hpp>
#include <RCF/Token.hpp>

// NB: occurrences of "interface" in this file have been replaced with "interface_", due to obscure bugs with Visual C++

namespace RCF {

    class Connection;
    class I_RcfClient;
    typedef boost::shared_ptr<I_RcfClient> RcfClientPtr;

    template<typename T>
    class I_Deref
    {
    public:
        virtual ~I_Deref() {}
        virtual T &deref() = 0;
    };

    template<typename T>
    class DerefPtr
    {
    public:
        typedef boost::shared_ptr< I_Deref<T> > type;
    };

    template<typename T>
    class DerefObj : public I_Deref<T>
    {
    public:
        DerefObj(T &t) :
            mT(t)
        {}

        T &deref()
        {
            return mT;
        }

    private:
        T &mT;
    };

    template<typename T>
    class DerefSharedPtr : public I_Deref<T>
    {
    public:
        DerefSharedPtr(boost::shared_ptr<T> tPtr) :
            mTPtr(tPtr)
        {}

        T &deref()
        {
            return *mTPtr;
        }

    private:
        boost::shared_ptr<T> mTPtr;
    };

    template<typename T>
    class DerefWeakPtr : public I_Deref<T>
    {
    public:
        DerefWeakPtr(boost::weak_ptr<T> tWeakPtr) :
            mTWeakPtr(tWeakPtr)
        {}

        T &deref()
        {
            boost::shared_ptr<T> tPtr(mTWeakPtr);
            if (tPtr.get())
            {
                return *tPtr;
            }
            RCF_THROW(Exception(RcfError_NoServerStub));
        }

    private:
        boost::weak_ptr<T> mTWeakPtr;
    };

    template<typename T>
    class DerefAutoPtr : public I_Deref<T>
    {
    public:
        DerefAutoPtr(const std::auto_ptr<T> &tAutoPtr) :
            mTAutoPtr( const_cast<std::auto_ptr<T> &>(tAutoPtr))
        {}

        T &deref()
        {
            return *mTAutoPtr;
        }

    private:
        std::auto_ptr<T> mTAutoPtr;
    };

    typedef boost::function3<
        void,
        int,
        SerializationProtocolIn &,
        SerializationProtocolOut &> InvokeFunctor;

    typedef std::map<std::string,  InvokeFunctor> InvokeFunctorMap;

    class StubAccess
    {
    public:

        template<typename InterfaceT, typename IdT, typename ImplementationT>
        void invoke(
            InterfaceT &interface_,
            const IdT &id,
            SerializationProtocolIn &in,
            SerializationProtocolOut &out,
            ImplementationT &t)
        {
            interface_.invoke(id, in, out, t);
        }

        template<typename InterfaceT, typename DerefPtrT>
        void registerInvokeFunctors(
            InterfaceT &interface_,
            InvokeFunctorMap &invokeFunctorMap,
            DerefPtrT derefPtr)
        {
            interface_.registerInvokeFunctors(invokeFunctorMap, derefPtr);
        }

        template<typename InheritT, typename InterfaceT, typename DerefPtrT>
        void registerParentInvokeFunctors(
            InheritT *,
            InterfaceT &interface_,
            InvokeFunctorMap &invokeFunctorMap,
            DerefPtrT derefPtr,
            boost::mpl::false_ *)
        {
            typedef typename GetInterface<InheritT>::type ParentInterfaceT;
            interface_.ParentInterfaceT::registerInvokeFunctors(
                invokeFunctorMap,
                derefPtr);
        }

        template<typename InheritT, typename InterfaceT, typename DerefPtrT>
        void registerParentInvokeFunctors(
            InheritT *,
            InterfaceT &,
            InvokeFunctorMap &,
            DerefPtrT,
            boost::mpl::true_*)
        {}

        template<typename InheritT, typename InterfaceT, typename DerefPtrT>
        void registerParentInvokeFunctors(
            InheritT *i,
            InterfaceT &interface_,
            InvokeFunctorMap &invokeFunctorMap,
            DerefPtrT derefPtr)
        {

#if defined(_MSC_VER) && _MSC_VER == 1200
#define RCF_TYPENAME
#else
#define RCF_TYPENAME typename
#endif

            typedef RCF_TYPENAME boost::is_same<
                InheritT,
                RCF_TYPENAME GetInterface<InheritT>::type >::type type;

#undef RCF_TYPENAME

            registerParentInvokeFunctors(
                i,
                interface_,
                invokeFunctorMap,
                derefPtr,
                (type *) NULL);
        }

        template<typename InheritT, typename InterfaceT>
        void setClientStubPtr(
            InheritT *,
            InterfaceT &interface_,
            boost::mpl::false_ *)
        {
            typedef typename InheritT::RcfClient ParentInterfaceT;
            interface_.ParentInterfaceT::setClientStubPtr(
                interface_.mClientStubPtr);
        }

        template<typename InheritT, typename InterfaceT>
        void setClientStubPtr(
            InheritT *,
            InterfaceT &,
            boost::mpl::true_ *)
        {}

        template<typename InheritT, typename InterfaceT>
        void setClientStubPtr(
            InheritT *i,
            InterfaceT &interface_)
        {
/*
#if defined(_MSC_VER) && _MSC_VER == 1200
#define RCF_TYPENAME
#else
#define RCF_TYPENAME typename
#endif
*/

            typedef BOOST_DEDUCED_TYPENAME boost::is_same<
                InheritT,
                BOOST_DEDUCED_TYPENAME GetInterface<InheritT>::type >::type type;

//#undef RCF_TYPENAME

            setClientStubPtr(i, interface_, (type *) NULL);
        }

#ifdef RCF_USE_SF_SERIALIZATION

        template<typename RcfClientT>
        void serialize(SF::Archive &ar, RcfClientT &rcfClient, const unsigned int)
        {
            if (ar.isWrite())
            {
                rcfClient.mClientStubPtr ?
                    ar & true & rcfClient.getClientStub() :
                    ar & false;
            }
            else //if (ar.isRead())
            {
                bool hasClientStub = false;
                ar & hasClientStub;
                if (hasClientStub)
                {
                    if (!rcfClient.mClientStubPtr)
                    {
                        typedef typename RcfClientT::Interface Interface;
                        std::string interfaceName = getInterfaceName( (Interface*) 0);
                        ClientStubPtr clientStubPtr(new ClientStub(interfaceName));
                        rcfClient.setClientStubPtr(clientStubPtr);
                    }
                    ar & rcfClient.getClientStub();
                }
                else
                {
                    rcfClient.setClientStubPtr( ClientStubPtr());
                }
            }
        }

#endif

        template<typename Archive, typename RcfClientT>
        void serialize(Archive &ar, RcfClientT &rcfClient, const unsigned RCF_PFTO_HACK int)
        {
            typedef typename Archive::is_saving IsSaving;
            const bool isSaving = IsSaving::value;

            if (isSaving)
            {
                bool hasClientStub = rcfClient.mClientStubPtr;
                ar & hasClientStub;
                if (hasClientStub)
                {
                    ar & rcfClient.getClientStub();
                }
            }
            else //if (ar.isRead())
            {
                bool hasClientStub = false;
                ar & hasClientStub;
                
                if (hasClientStub)
                {
                    if (!rcfClient.mClientStubPtr)
                    {
                        typedef typename RcfClientT::Interface Interface;
                        std::string interfaceName = getInterfaceName( (Interface*) 0);
                        ClientStubPtr clientStubPtr(new ClientStub(interfaceName));
                        rcfClient.setClientStubPtr(clientStubPtr);
                    }
                    ar & rcfClient.getClientStub();
                }
                else
                {
                    rcfClient.setClientStubPtr( ClientStubPtr());
                }
            }
        }

    };

    template<typename InterfaceT, typename ImplementationT>
    inline void invoke(
        InterfaceT &interface_,
        ImplementationT &t,
        int fnId,
        SerializationProtocolIn &in,
        SerializationProtocolOut &out)
    {
        switch (fnId) {
        case  0: StubAccess().invoke(interface_, boost::mpl::int_< 0>(), in, out, t); break;
        case  1: StubAccess().invoke(interface_, boost::mpl::int_< 1>(), in, out, t); break;
        case  2: StubAccess().invoke(interface_, boost::mpl::int_< 2>(), in, out, t); break;
        case  3: StubAccess().invoke(interface_, boost::mpl::int_< 3>(), in, out, t); break;
        case  4: StubAccess().invoke(interface_, boost::mpl::int_< 4>(), in, out, t); break;
        case  5: StubAccess().invoke(interface_, boost::mpl::int_< 5>(), in, out, t); break;
        case  6: StubAccess().invoke(interface_, boost::mpl::int_< 6>(), in, out, t); break;
        case  7: StubAccess().invoke(interface_, boost::mpl::int_< 7>(), in, out, t); break;
        case  8: StubAccess().invoke(interface_, boost::mpl::int_< 8>(), in, out, t); break;
        case  9: StubAccess().invoke(interface_, boost::mpl::int_< 9>(), in, out, t); break;
        case 10: StubAccess().invoke(interface_, boost::mpl::int_<10>(), in, out, t); break;
        case 11: StubAccess().invoke(interface_, boost::mpl::int_<11>(), in, out, t); break;
        case 12: StubAccess().invoke(interface_, boost::mpl::int_<12>(), in, out, t); break;
        case 13: StubAccess().invoke(interface_, boost::mpl::int_<13>(), in, out, t); break;
        case 14: StubAccess().invoke(interface_, boost::mpl::int_<14>(), in, out, t); break;
        case 15: StubAccess().invoke(interface_, boost::mpl::int_<15>(), in, out, t); break;
        case 16: StubAccess().invoke(interface_, boost::mpl::int_<16>(), in, out, t); break;
        case 17: StubAccess().invoke(interface_, boost::mpl::int_<17>(), in, out, t); break;
        case 18: StubAccess().invoke(interface_, boost::mpl::int_<18>(), in, out, t); break;
        case 19: StubAccess().invoke(interface_, boost::mpl::int_<19>(), in, out, t); break;
        case 20: StubAccess().invoke(interface_, boost::mpl::int_<20>(), in, out, t); break;
        case 21: StubAccess().invoke(interface_, boost::mpl::int_<21>(), in, out, t); break;
        case 22: StubAccess().invoke(interface_, boost::mpl::int_<22>(), in, out, t); break;
        case 23: StubAccess().invoke(interface_, boost::mpl::int_<23>(), in, out, t); break;
        case 24: StubAccess().invoke(interface_, boost::mpl::int_<24>(), in, out, t); break;
        case 25: StubAccess().invoke(interface_, boost::mpl::int_<25>(), in, out, t); break;
        case 26: StubAccess().invoke(interface_, boost::mpl::int_<26>(), in, out, t); break;
        case 27: StubAccess().invoke(interface_, boost::mpl::int_<27>(), in, out, t); break;
        case 28: StubAccess().invoke(interface_, boost::mpl::int_<28>(), in, out, t); break;
        case 29: StubAccess().invoke(interface_, boost::mpl::int_<29>(), in, out, t); break;
        case 30: StubAccess().invoke(interface_, boost::mpl::int_<30>(), in, out, t); break;
        case 31: StubAccess().invoke(interface_, boost::mpl::int_<31>(), in, out, t); break;
        case 32: StubAccess().invoke(interface_, boost::mpl::int_<32>(), in, out, t); break;
        case 33: StubAccess().invoke(interface_, boost::mpl::int_<33>(), in, out, t); break;
        case 34: StubAccess().invoke(interface_, boost::mpl::int_<34>(), in, out, t); break;
        case 35: StubAccess().invoke(interface_, boost::mpl::int_<35>(), in, out, t); break;
        default: RCF_THROW(Exception(RcfError_FnId))(fnId); // TODO: exception type
        }
    }

    template<typename InterfaceT, typename DerefPtrT>
    class InterfaceInvocator
    {
    public:
        InterfaceInvocator(InterfaceT &interface_, DerefPtrT derefPtr) :
            mInterface(interface_),
            mDerefPtr(derefPtr)
        {}

        void operator()(
            int fnId,
            SerializationProtocolIn &in,
            SerializationProtocolOut &out)
        {
            invoke<InterfaceT>(mInterface, mDerefPtr->deref(), fnId, in, out);
        }

    private:
        InterfaceT &    mInterface;
        DerefPtrT       mDerefPtr;
    };

    template<typename InterfaceT, typename DerefPtrT>
    void registerInvokeFunctors(
        InterfaceT &interface_,
        InvokeFunctorMap &invokeFunctorMap,
        DerefPtrT derefPtr)
    {
        // NB: same interface may occur more than once in the inheritance hierarchy of another interface, and in
        // that case, via overwriting, only one InterfaceInvocator is registered, so only the functions in one of the interfaces will ever be called.
        // But it doesn't matter, since even if an interface occurs several times in the inheritance hierarchy, each occurrence
        // of the interface will be bound to derefPtr in exactly the same way.

        typedef typename InterfaceT::Interface Interface;
        std::string interfaceName = ::RCF::getInterfaceName( (Interface *) NULL);

        invokeFunctorMap[ interfaceName ] =
            InterfaceInvocator<InterfaceT, DerefPtrT>(interface_, derefPtr);
    }

    class ServerStub;

    typedef boost::shared_ptr<ServerStub> ServerStubPtr;

    class ServerStub
    {
    public:

        template<typename InterfaceT, typename DerefPtrT>
        void registerInvokeFunctors(InterfaceT &interface_, DerefPtrT derefPtr)
        {
            StubAccess access;
            access.registerInvokeFunctors(
                interface_,
                mInvokeFunctorMap,
                derefPtr);
        }

        void invoke(
            const std::string &subInterface,
            int fnId,
            SerializationProtocolIn &in,
            SerializationProtocolOut &out);

        void merge(RcfClientPtr rcfClientPtr);

    private:
        // TODO: overhead for each server stub is bit on the heavy side
        InvokeFunctorMap                mInvokeFunctorMap;
        std::vector<RcfClientPtr>       mMergedStubs;
    };

    template<typename InterfaceT, typename ImplementationT, typename ImplementationPtrT>
    RcfClientPtr createServerStub(
        InterfaceT *,
        ImplementationT *,
        ImplementationPtrT px)
    {
        typedef typename InterfaceT::RcfClient RcfClient;
        return RcfClientPtr( new RcfClient(
            ServerStubPtr(new ServerStub()),
            px));
    }

} // namespace RCF

#include <RCF/ServerStub.inl>

#endif // ! INCLUDE_RCF_SERVERSTUB_HPP
