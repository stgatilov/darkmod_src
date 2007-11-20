
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_IDL_HPP
#define INCLUDE_RCF_IDL_HPP

#include <boost/mpl/bool.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/int.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_same.hpp>

#include <RCF/ClientStub.hpp>
#include <RCF/ClientTransport.hpp>
#include <RCF/Endpoint.hpp>
#include <RCF/Exception.hpp>
#include <RCF/GetInterfaceName.hpp>
#include <RCF/Marshal.hpp>
#include <RCF/RcfClient.hpp>
#include <RCF/ServerStub.hpp>
#include <RCF/util/Meta.hpp>

// RCF_BEGIN
/// Alias for RCF_BEGIN_INHERITED_0
#define RCF_BEGIN(InterfaceT, Name) RCF_BEGIN_I0(InterfaceT, Name)

// RCF_BEGIN_INHERITED
/// Alias for RCF_BEGIN_INHERITED_1
//#define RCF_BEGIN_INHERITED(InterfaceT, Name, InheritT) RCF_BEGIN_INHERITED_1(InterfaceT, Name, InheritT)

// RCF_BEGIN_INHERITED_0
/// Marks the beginning of the definition of a RCF interface.
/// \param InterfaceT Interface name (symbolic).
/// \param Name Interface name (string). If "", then the name will be auto generated from the symbolic name.
#define RCF_BEGIN_I0(InterfaceT, Name)                                                                  \
    RCF_BEGIN_IMPL_PRELUDE(InterfaceT, Name)                                                            \
    RCF_BEGIN_IMPL_INHERITED_0(InterfaceT, Name)                                                        \
    RCF_BEGIN_IMPL_POSTLUDE(InterfaceT, Name)

// RCF_BEGIN_INHERITED_1
/// Marks the beginning of the definition of a inherited RCF interface.
/// \param InterfaceT Interface name (symbolic).
/// \param Name Interface name (string). If "", then the name will be auto generated from the symbolic name.
/// \param InheritT Interface to derive from. May be a generic C++ class, for instance an abstract base class, or a RCF interface.
#define RCF_BEGIN_I1(InterfaceT, Name, InheritT1)                                                       \
    RCF_BEGIN_IMPL_PRELUDE(InterfaceT, Name)                                                            \
    RCF_BEGIN_IMPL_INHERITED_1(InterfaceT, Name, InheritT1)                                             \
    RCF_BEGIN_IMPL_POSTLUDE(InterfaceT, Name)

// RCF_BEGIN_INHERITED_2
/// Marks the beginning of the definition of a inherited RCF interface.
/// \param InterfaceT Interface name (symbolic).
/// \param Name Interface name (string). If "", then the name will be auto generated from the symbolic name.
/// \param InheritT1 First interface to derive from. May be a generic C++ class, for instance an abstract base class, or a RCF interface.
/// \param InheritT2 Second interface to derive from. May be a generic C++ class, for instance an abstract base class, or a RCF interface.
#define RCF_BEGIN_I2(InterfaceT, Name, InheritT1, InheritT2)                                            \
    RCF_BEGIN_IMPL_PRELUDE(InterfaceT, Name)                                                            \
    RCF_BEGIN_IMPL_INHERITED_2(InterfaceT, Name, InheritT1, InheritT2)                                  \
    RCF_BEGIN_IMPL_POSTLUDE(InterfaceT, Name)

// RCF_BEGIN_IMPL_PRELUDE
#define RCF_BEGIN_IMPL_PRELUDE(InterfaceT, Name)                                                        \
                                                                                                        \
    template<typename T>                                                                                \
    class RcfClient;                                                                                    \
                                                                                                        \
    class InterfaceT                                                                                    \
    {                                                                                                   \
    public:                                                                                             \
        typedef RcfClient<InterfaceT> RcfClient;                                                        \
        static std::string getInterfaceName()                                                           \
        {                                                                                               \
            return std::string(Name) == "" ? #InterfaceT : Name;                                        \
        }                                                                                               \
    };

// RCF_BEGIN_IMPL_INHERITED_0
#define RCF_BEGIN_IMPL_INHERITED_0(InterfaceT, Name)                                                    \
    template<>                                                                                          \
    class RcfClient< InterfaceT > :                                                                     \
        public virtual ::RCF::I_RcfClient                                                               \
    {                                                                                                   \
    private:                                                                                            \
        template<typename DerefPtrT>                                                                    \
        void registerInvokeFunctors(::RCF::InvokeFunctorMap &invokeFunctorMap, DerefPtrT derefPtr)      \
        {                                                                                               \
            ::RCF::registerInvokeFunctors(*this, invokeFunctorMap, derefPtr);                           \
        }                                                                                               \
        void setClientStubPtr(::RCF::ClientStubPtr clientStubPtr)                                       \
        {                                                                                               \
            mClientStubPtr = clientStubPtr;                                                             \
        }

// RCF_BEGIN_IMPL_INHERITED_1
#define RCF_BEGIN_IMPL_INHERITED_1(InterfaceT, Name, InheritT1)                                         \
    template<>                                                                                          \
    class RcfClient< InterfaceT > :                                                                     \
        public virtual ::RCF::I_RcfClient,                                                              \
        public virtual ::RCF::GetInterface<InheritT1>::type                                             \
    {                                                                                                   \
    private:                                                                                            \
        template<typename DerefPtrT>                                                                    \
        void registerInvokeFunctors(::RCF::InvokeFunctorMap &invokeFunctorMap, DerefPtrT derefPtr)      \
        {                                                                                               \
            ::RCF::registerInvokeFunctors(*this, invokeFunctorMap, derefPtr);                           \
            ::RCF::StubAccess().registerParentInvokeFunctors( (InheritT1 *) NULL, *this, invokeFunctorMap, derefPtr); \
        }                                                                                               \
        void setClientStubPtr(::RCF::ClientStubPtr clientStubPtr)                                       \
        {                                                                                               \
            mClientStubPtr = clientStubPtr;                                                             \
            ::RCF::StubAccess().setClientStubPtr( (InheritT1*) 0, *this);                               \
        }

// RCF_BEGIN_IMPL_INHERITED_2
#define RCF_BEGIN_IMPL_INHERITED_2(InterfaceT, Name, InheritT1, InheritT2)                              \
    template<>                                                                                          \
    class RcfClient< InterfaceT > :                                                                     \
        public virtual ::RCF::I_RcfClient,                                                              \
        public virtual ::RCF::GetInterface<InheritT1>::type,                                            \
        public virtual ::RCF::GetInterface<InheritT2>::type                                             \
    {                                                                                                   \
    private:                                                                                            \
        template<typename DerefPtrT>                                                                    \
        void registerInvokeFunctors(::RCF::InvokeFunctorMap &invokeFunctorMap, DerefPtrT derefPtr)      \
        {                                                                                               \
            ::RCF::registerInvokeFunctors(*this, invokeFunctorMap, derefPtr);                           \
            ::RCF::StubAccess().registerParentInvokeFunctors( (InheritT1 *) NULL, *this, invokeFunctorMap, derefPtr);   \
            ::RCF::StubAccess().registerParentInvokeFunctors( (InheritT2 *) NULL, *this, invokeFunctorMap, derefPtr);   \
        }                                                                                               \
        void setClientStubPtr(::RCF::ClientStubPtr clientStubPtr)                                       \
        {                                                                                               \
            mClientStubPtr = clientStubPtr;                                                             \
            ::RCF::StubAccess().setClientStubPtr( (InheritT1*) 0, *this);                               \
            ::RCF::StubAccess().setClientStubPtr( (InheritT2*) 0, *this);                               \
        }

// RCF_BEGIN_IMPL_POSTLUDE
#define RCF_BEGIN_IMPL_POSTLUDE(InterfaceT, Name)                                                       \
    public:                                                                                             \
                                                                                                        \
        RcfClient()                                                                                     \
        {}                                                                                              \
                                                                                                        \
        template<typename DerefPtrT>                                                                    \
        RcfClient(::RCF::ServerStubPtr serverStubPtr, DerefPtrT derefPtr)                               \
        {                                                                                               \
            serverStubPtr->registerInvokeFunctors(*this, derefPtr);                                     \
            mServerStubPtr = serverStubPtr;                                                             \
        }                                                                                               \
                                                                                                        \
        RcfClient(                                                                                      \
            const ::RCF::I_Endpoint &endpoint,                                                          \
            const std::string &targetName_ = "")                                                        \
        {                                                                                               \
            const std::string &interfaceName = ::RCF::getInterfaceName( (InterfaceT *) NULL);           \
            const std::string &targetName = (targetName_ == "") ? interfaceName : targetName_;          \
            ::RCF::ClientStubPtr clientStubPtr( new ::RCF::ClientStub(interfaceName, targetName) );     \
            clientStubPtr->setEndpoint(endpoint);                                                       \
            setClientStubPtr(clientStubPtr);                                                            \
        }                                                                                               \
                                                                                                        \
        RcfClient(                                                                                      \
            ::RCF::ClientTransportAutoPtr clientTransportAutoPtr,                                       \
            const std::string &targetName_ = "")                                                        \
        {                                                                                               \
            const std::string &interfaceName = ::RCF::getInterfaceName( (InterfaceT *) NULL);           \
            const std::string &targetName = (targetName_ == "") ? interfaceName : targetName_;          \
            ::RCF::ClientStubPtr clientStubPtr( new ::RCF::ClientStub(interfaceName, targetName) );     \
            clientStubPtr->setTransport(clientTransportAutoPtr);                                        \
            setClientStubPtr(clientStubPtr);                                                            \
        }                                                                                               \
                                                                                                        \
        RcfClient(                                                                                      \
            const ::RCF::ClientStub &clientStub,                                                        \
            const std::string &targetName_ = "")                                                        \
        {                                                                                               \
            const std::string &interfaceName = ::RCF::getInterfaceName( (InterfaceT *) NULL);           \
            const std::string &targetName = (targetName_ == "") ? interfaceName : targetName_;          \
            ::RCF::ClientStubPtr clientStubPtr( new ::RCF::ClientStub(clientStub) );                    \
            clientStubPtr->setTargetName(targetName);                                                   \
            setClientStubPtr(clientStubPtr);                                                            \
        }                                                                                               \
                                                                                                        \
        template<typename T>                                                                            \
        RcfClient(                                                                                      \
            const RcfClient<T> &rhs)                                                                    \
        {                                                                                               \
            ::RCF::ClientStubPtr clientStubPtr( new ::RCF::ClientStub(rhs.getClientStub()));            \
            setClientStubPtr(clientStubPtr);                                                            \
        }                                                                                               \
                                                                                                        \
        template<typename T>                                                                            \
        RcfClient &operator=(const RcfClient<T> &rhs)                                                   \
        {                                                                                               \
            if (&rhs != this)                                                                           \
            {                                                                                           \
                ::RCF::ClientStubPtr clientStubPtr( new ::RCF::ClientStub(rhs.getClientStub()));        \
                setClientStubPtr(clientStubPtr);                                                        \
            }                                                                                           \
            return *this;                                                                               \
        }                                                                                               \
                                                                                                        \
    public:                                                                                             \
        ::RCF::ClientStub &getClientStub()                                                              \
        {                                                                                               \
            return *mClientStubPtr;                                                                     \
        }                                                                                               \
                                                                                                        \
        const ::RCF::ClientStub &getClientStub() const                                                  \
        {                                                                                               \
            return *mClientStubPtr;                                                                     \
        }                                                                                               \
                                                                                                        \
    private:                                                                                            \
        ::RCF::ServerStub &getServerStub()                                                              \
        {                                                                                               \
            return *mServerStubPtr;                                                                     \
        }                                                                                               \
                                                                                                        \
    public:                                                                                             \
        template<typename Archive>                                                                      \
        void serialize(Archive &ar, const unsigned int ver)                                             \
        {                                                                                               \
            ::RCF::StubAccess().serialize(ar, *this, ver);                                              \
        }                                                                                               \
                                                                                                        \
    private:                                                                                            \
                                                                                                        \
        template<typename T, typename U>                                                                \
        void invoke(                                                                                    \
            const T &,                                                                                  \
            ::RCF::SerializationProtocolIn &,                                                           \
            ::RCF::SerializationProtocolOut &, const U &)                                               \
        {                                                                                               \
            RCF_THROW(::RCF::Exception(::RCF::RcfError_FnId))(typeid(T));                               \
        }                                                                                               \
                                                                                                        \
        ::RCF::ClientStubPtr mClientStubPtr;                                                            \
        ::RCF::ServerStubPtr mServerStubPtr;                                                            \
        friend class ::RCF::StubAccess;                                                                 \
        typedef RcfClient< InterfaceT > ThisT;                                                          \
        typedef ::RCF::Dummy<ThisT> DummyThisT;                                                         \
        friend RCF::default_ RCF_make_next_dispatch_id_func(DummyThisT *, ThisT *,...);                 \
    public:                                                                                             \
        typedef InterfaceT Interface;


// RCF_END
/// Marks the end of the definition of a RCF interface.
/// \param InterfaceT Symbolic name of the interface

// TODO: we don't even need InterfaceT parameter here...
#define RCF_END( InterfaceT )                                                                           \
    };

// RCF_METHOD_R0
/// Defines a member function of an interface, having non-void return type and no arguments.
/// \param R Return type.
/// \param func Function name.
#define RCF_METHOD_R0(R,func)                        RCF_METHOD_R0_(R,func, RCF_MAKE_UNIQUE_ID(func, R0))
#define RCF_METHOD_R0_(R, func, id)                                                                     \
        public:                                                                                         \
            RCF_MAKE_NEXT_DISPATCH_ID(id);                                                              \
            R func()                                                                                    \
            {                                                                                           \
                return func(getClientStub().getDefaultCallingSemantics());                              \
            }                                                                                           \
            R func(RCF::RemoteCallSemantics rcs)                                                        \
            {                                                                                           \
                ::RCF::CurrentClientStubPtrSentry sentry(mClientStubPtr);                               \
                return ::RCF::ClientMarshal_R0< R >()(                                                  \
                    getClientStub(),                                                                    \
                    rcs,                                                                                \
                    ::RCF::getInterfaceName( (Interface *) NULL),                                       \
                    id::value);                                                                         \
            }                                                                                           \
                                                                                                        \
        private:                                                                                        \
            template<typename T>                                                                        \
            void invoke(                                                                                \
                const id &,                                                                             \
                ::RCF::SerializationProtocolIn &in,                                                     \
                ::RCF::SerializationProtocolOut &out,                                                   \
                T &t)                                                                                   \
            {                                                                                           \
                RCF_UNUSED_VARIABLE(in);                                                                \
                ::RCF::IDL::OutReturnValue< R >(out, t.func());                                         \
            }



// RCF_METHOD_R1
/// Defines a member function of an interface, having non-void return type and one argument.
/// \param R Return type.
/// \param func Function name.
/// \param A1 Type of first argument.
#define RCF_METHOD_R1(R,func,A1)                        RCF_METHOD_R1_(R,func,A1, RCF_MAKE_UNIQUE_ID(func, R1))
#define RCF_METHOD_R1_(R,func,A1,id)                                                                    \
        public:                                                                                         \
            RCF_MAKE_NEXT_DISPATCH_ID(id);                                                              \
            R func(A1 a1)                                                                               \
            {                                                                                           \
                return func(getClientStub().getDefaultCallingSemantics(), a1);                          \
            }                                                                                           \
            R func(::RCF::RemoteCallSemantics rcs, A1 a1)                                               \
            {                                                                                           \
                ::RCF::CurrentClientStubPtrSentry sentry(mClientStubPtr);                               \
                return ::RCF::ClientMarshal_R1<R,A1 >()(                                                \
                    getClientStub(),                                                                    \
                    rcs,                                                                                \
                    ::RCF::getInterfaceName( (Interface *) NULL),                                       \
                    id::value,                                                                          \
                    a1);                                                                                \
            }                                                                                           \
                                                                                                        \
        private:                                                                                        \
            template<typename T>                                                                        \
            void invoke(                                                                                \
                const id &,                                                                             \
                ::RCF::SerializationProtocolIn &in,                                                     \
                ::RCF::SerializationProtocolOut &out,                                                   \
                T &t)                                                                                   \
            {                                                                                           \
                ::RCF::IDL::InParameter< A1 >::type arg1(in);                                           \
                ::RCF::IDL::OutReturnValue< R >(out, t.func(arg1.get()));                               \
                ::RCF::IDL::OutParameter< A1 >::type(arg1, out);                                        \
            }

// RCF_METHOD_R2
/// Defines a member function of an interface, having non-void return type and two arguments.
/// \param R Return type.
/// \param func Function name.
/// \param A1 Type of first argument.
/// \param A2 Type of second argument.
#define RCF_METHOD_R2(R,func,A1,A2)                        RCF_METHOD_R2_(R,func,A1,A2, RCF_MAKE_UNIQUE_ID(func, R2))
#define RCF_METHOD_R2_(R,func,A1,A2,id)                                                                 \
        public:                                                                                         \
            RCF_MAKE_NEXT_DISPATCH_ID(id);                                                              \
            R func(A1 a1, A2 a2)                                                                        \
            {                                                                                           \
                return func(getClientStub().getDefaultCallingSemantics(), a1, a2);                      \
            }                                                                                           \
            R func(::RCF::RemoteCallSemantics rcs, A1 a1, A2 a2)                                        \
            {                                                                                           \
                ::RCF::CurrentClientStubPtrSentry sentry(mClientStubPtr);                               \
                return ::RCF::ClientMarshal_R2< R,A1,A2 >()(                                            \
                    getClientStub(),                                                                    \
                    rcs,                                                                                \
                    ::RCF::getInterfaceName( (Interface *) NULL),                                       \
                    id::value,                                                                          \
                    a1, a2);                                                                            \
            }                                                                                           \
                                                                                                        \
        private:                                                                                        \
            template<typename T>                                                                        \
            void invoke(                                                                                \
                const id &,                                                                             \
                ::RCF::SerializationProtocolIn &in,                                                     \
                ::RCF::SerializationProtocolOut &out,                                                   \
                T &t)                                                                                   \
            {                                                                                           \
                ::RCF::IDL::InParameter< A1 >::type arg1(in);                                           \
                ::RCF::IDL::InParameter< A2 >::type arg2(in);                                           \
                ::RCF::IDL::OutReturnValue< R >(out, t.func(arg1.get(), arg2.get()));                   \
                ::RCF::IDL::OutParameter< A1 >::type(arg1, out);                                        \
                ::RCF::IDL::OutParameter< A2 >::type(arg2, out);                                        \
            }


// RCF_METHOD_R3
/// Defines a member function of an interface, having non-void return type and three arguments.
/// \param R Return type.
/// \param func Function name.
/// \param A1 Type of first argument.
/// \param A2 Type of second argument.
/// \param A3 Type of third argument.
#define RCF_METHOD_R3(R,func,A1,A2,A3)                        RCF_METHOD_R3_(R,func,A1,A2,A3, RCF_MAKE_UNIQUE_ID(func, R3))
#define RCF_METHOD_R3_(R,func,A1,A2,A3,id)                                                              \
        public:                                                                                         \
            RCF_MAKE_NEXT_DISPATCH_ID(id);                                                              \
            R func(A1 a1, A2 a2, A3 a3)                                                                 \
            {                                                                                           \
                return func(getClientStub().getDefaultCallingSemantics(), a1, a2, a3);                  \
            }                                                                                           \
            R func(::RCF::RemoteCallSemantics rcs, A1 a1, A2 a2, A3 a3)                                 \
            {                                                                                           \
                ::RCF::CurrentClientStubPtrSentry sentry(mClientStubPtr);                               \
                return ::RCF::ClientMarshal_R3< R,A1,A2,A3 >()(                                         \
                    getClientStub(),                                                                    \
                    rcs,                                                                                \
                    ::RCF::getInterfaceName( (Interface *) NULL),                                       \
                    id::value,                                                                          \
                    a1, a2, a3);                                                                        \
            }                                                                                           \
                                                                                                        \
        private:                                                                                        \
            template<typename T>                                                                        \
            void invoke(                                                                                \
                const id &,                                                                             \
                ::RCF::SerializationProtocolIn &in,                                                     \
                ::RCF::SerializationProtocolOut &out,                                                   \
                T &t)                                                                                   \
            {                                                                                           \
                ::RCF::IDL::InParameter< A1 >::type arg1(in);                                           \
                ::RCF::IDL::InParameter< A2 >::type arg2(in);                                           \
                ::RCF::IDL::InParameter< A3 >::type arg3(in);                                           \
                ::RCF::IDL::OutReturnValue< R >(out, t.func(arg1.get(), arg2.get(), arg3.get()));       \
                ::RCF::IDL::OutParameter< A1 >::type(arg1, out);                                        \
                ::RCF::IDL::OutParameter< A2 >::type(arg2, out);                                        \
                ::RCF::IDL::OutParameter< A3 >::type(arg3, out);                                        \
            }

// RCF_METHOD_R4
/// Defines a member function of an interface, having non-void return type and four arguments.
/// \param R Return type.
/// \param func Function name.
/// \param A1 Type of first argument.
/// \param A2 Type of second argument.
/// \param A3 Type of third argument.
/// \param A4 Type of fourth argument.
#define RCF_METHOD_R4(R,func,A1,A2,A3,A4)                        RCF_METHOD_R4_(R,func,A1,A2,A3,A4, RCF_MAKE_UNIQUE_ID(func, R4))
#define RCF_METHOD_R4_(R,func,A1,A2,A3,A4,id)                                                           \
        public:                                                                                         \
            RCF_MAKE_NEXT_DISPATCH_ID(id);                                                              \
            R func(A1 a1, A2 a2, A3 a3, A4 a4)                                                          \
            {                                                                                           \
                return func(getClientStub().getDefaultCallingSemantics(), a1, a2, a3, a4);              \
            }                                                                                           \
            R func(::RCF::RemoteCallSemantics rcs, A1 a1, A2 a2, A3 a3, A4 a4 )                         \
            {                                                                                           \
                ::RCF::CurrentClientStubPtrSentry sentry(mClientStubPtr);                               \
                return ::RCF::ClientMarshal_R4< R,A1,A2,A3,A4 >()(                                      \
                    getClientStub(),                                                                    \
                    rcs,                                                                                \
                    ::RCF::getInterfaceName( (Interface *) NULL),                                       \
                    id::value,                                                                          \
                    a1, a2, a3, a4);                                                                    \
            }                                                                                           \
                                                                                                        \
        private:                                                                                        \
            template<typename T>                                                                        \
            void invoke(                                                                                \
                const id &,                                                                             \
                ::RCF::SerializationProtocolIn &in,                                                     \
                ::RCF::SerializationProtocolOut &out,                                                   \
                T &t)                                                                                   \
            {                                                                                           \
                ::RCF::IDL::InParameter< A1 >::type arg1(in);                                           \
                ::RCF::IDL::InParameter< A2 >::type arg2(in);                                           \
                ::RCF::IDL::InParameter< A3 >::type arg3(in);                                           \
                ::RCF::IDL::InParameter< A4 >::type arg4(in);                                                   \
                ::RCF::IDL::OutReturnValue< R >(out, t.func(arg1.get(), arg2.get(), arg3.get(), arg4.get()));   \
                ::RCF::IDL::OutParameter< A1 >::type(arg1, out);                                                \
                ::RCF::IDL::OutParameter< A2 >::type(arg2, out);                                        \
                ::RCF::IDL::OutParameter< A3 >::type(arg3, out);                                        \
                ::RCF::IDL::OutParameter< A4 >::type(arg4, out);                                        \
            }

// RCF_METHOD_R5
/// Defines a member function of an interface, having non-void return type and five arguments.
/// \param R Return type.
/// \param func Function name.
/// \param A1 Type of first argument.
/// \param A2 Type of second argument.
/// \param A3 Type of third argument.
/// \param A4 Type of fourth argument.
/// \param A5 Type of fifth argument.
#define RCF_METHOD_R5(R,func,A1,A2,A3,A4,A5)                        RCF_METHOD_R5_(R,func,A1,A2,A3,A4,A5, RCF_MAKE_UNIQUE_ID(func, R5))
#define RCF_METHOD_R5_(R,func,A1,A2,A3,A4,A5, id)                                                       \
        public:                                                                                         \
            RCF_MAKE_NEXT_DISPATCH_ID(id);                                                              \
            R func(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5)                                                   \
            {                                                                                           \
                return func(getClientStub().getDefaultCallingSemantics(), a1, a2, a3, a4, a5);          \
            }                                                                                           \
            R func(::RCF::RemoteCallSemantics rcs, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5)                   \
            {                                                                                           \
                ::RCF::CurrentClientStubPtrSentry sentry(mClientStubPtr);                               \
                return ::RCF::ClientMarshal_R5< R,A1,A2,A3,A4,A5 >()(                                   \
                    getClientStub(),                                                                    \
                    rcs,                                                                                \
                    ::RCF::getInterfaceName( (Interface *) NULL),                                       \
                    id::value,                                                                          \
                    a1, a2, a3, a4, a5);                                                                \
            }                                                                                           \
                                                                                                        \
        private:                                                                                        \
            template<typename T>                                                                        \
            void invoke(                                                                                \
                const id &,                                                                             \
                ::RCF::SerializationProtocolIn &in,                                                     \
                ::RCF::SerializationProtocolOut &out,                                                   \
                T &t)                                                                                   \
            {                                                                                           \
                ::RCF::IDL::InParameter< A1 >::type arg1(in);                                           \
                ::RCF::IDL::InParameter< A2 >::type arg2(in);                                           \
                ::RCF::IDL::InParameter< A3 >::type arg3(in);                                           \
                ::RCF::IDL::InParameter< A4 >::type arg4(in);                                           \
                ::RCF::IDL::InParameter< A5 >::type arg5(in);                                                               \
                ::RCF::IDL::OutReturnValue< R >(out, t.func(arg1.get(), arg2.get(), arg3.get(), arg4.get(), arg5.get()));    \
                ::RCF::IDL::OutParameter< A1 >::type(arg1, out);                                                            \
                ::RCF::IDL::OutParameter< A2 >::type(arg2, out);                                        \
                ::RCF::IDL::OutParameter< A3 >::type(arg3, out);                                        \
                ::RCF::IDL::OutParameter< A4 >::type(arg4, out);                                        \
                ::RCF::IDL::OutParameter< A5 >::type(arg5, out);                                        \
            }

// RCF_METHOD_R6
/// Defines a member function of an interface, having non-void return type and six arguments.
/// \param R Return type.
/// \param func Function name.
/// \param A1 Type of first argument.
/// \param A2 Type of second argument.
/// \param A3 Type of third argument.
/// \param A4 Type of fourth argument.
/// \param A5 Type of fifth argument.
/// \param A6 Type of sixth argument.
#define RCF_METHOD_R6(R,func,A1,A2,A3,A4,A5,A6)                        RCF_METHOD_R6_(R,func,A1,A2,A3,A4,A5,A6, RCF_MAKE_UNIQUE_ID(func, R6))
#define RCF_METHOD_R6_(R,func,A1,A2,A3,A4,A5,A6, id)                                                    \
        public:                                                                                         \
            RCF_MAKE_NEXT_DISPATCH_ID(id);                                                              \
            R func(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6)                                            \
            {                                                                                           \
                return func(getClientStub().getDefaultCallingSemantics(), a1, a2, a3, a4, a5, a6);      \
            }                                                                                           \
            R func(::RCF::RemoteCallSemantics rcs, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6)            \
            {                                                                                           \
                ::RCF::CurrentClientStubPtrSentry sentry(mClientStubPtr);                               \
                return ::RCF::ClientMarshal_R6< R,A1,A2,A3,A4,A5,A6 >()(                                \
                    getClientStub(),                                                                    \
                    rcs,                                                                                \
                    ::RCF::getInterfaceName( (Interface *) NULL),                                       \
                    id::value,                                                                          \
                    a1, a2, a3, a4, a5, a6);                                                            \
            }                                                                                           \
                                                                                                        \
        private:                                                                                        \
            template<typename T>                                                                        \
            void invoke(                                                                                \
                const id &,                                                                             \
                ::RCF::SerializationProtocolIn &in,                                                     \
                ::RCF::SerializationProtocolOut &out,                                                   \
                T &t)                                                                                   \
            {                                                                                           \
                ::RCF::IDL::InParameter< A1 >::type arg1(in);                                           \
                ::RCF::IDL::InParameter< A2 >::type arg2(in);                                           \
                ::RCF::IDL::InParameter< A3 >::type arg3(in);                                           \
                ::RCF::IDL::InParameter< A4 >::type arg4(in);                                           \
                ::RCF::IDL::InParameter< A5 >::type arg5(in);                                           \
                ::RCF::IDL::InParameter< A6 >::type arg6(in);                                                                           \
                ::RCF::IDL::OutReturnValue< R >(out, t.func(arg1.get(), arg2.get(), arg3.get(), arg4.get(), arg5.get(), arg6.get()));   \
                ::RCF::IDL::OutParameter< A1 >::type(arg1, out);                                                                        \
                ::RCF::IDL::OutParameter< A2 >::type(arg2, out);                                        \
                ::RCF::IDL::OutParameter< A3 >::type(arg3, out);                                        \
                ::RCF::IDL::OutParameter< A4 >::type(arg4, out);                                        \
                ::RCF::IDL::OutParameter< A5 >::type(arg5, out);                                        \
                ::RCF::IDL::OutParameter< A6 >::type(arg6, out);                                        \
            }

// RCF_METHOD_R7
/// Defines a member function of an interface, having non-void return type and seven arguments.
/// \param R Return type.
/// \param func Function name.
/// \param A1 Type of first argument.
/// \param A2 Type of second argument.
/// \param A3 Type of third argument.
/// \param A4 Type of fourth argument.
/// \param A5 Type of fifth argument.
/// \param A6 Type of fifth argument.
/// \param A7 Type of fifth argument.
#define RCF_METHOD_R7(R,func,A1,A2,A3,A4,A5,A6,A7)                        RCF_METHOD_R7_(R,func,A1,A2,A3,A4,A5,A6,A7, RCF_MAKE_UNIQUE_ID(func, R7))
#define RCF_METHOD_R7_(R,func,A1,A2,A3,A4,A5,A6,A7, id)                                                 \
        public:                                                                                         \
            RCF_MAKE_NEXT_DISPATCH_ID(id);                                                              \
            R func(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7)                                     \
            {                                                                                           \
                return func(getClientStub().getDefaultCallingSemantics(), a1, a2, a3, a4, a5, a6, a7);  \
            }                                                                                           \
            R func(::RCF::RemoteCallSemantics rcs, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7)     \
            {                                                                                           \
                ::RCF::CurrentClientStubPtrSentry sentry(mClientStubPtr);                               \
                return ::RCF::ClientMarshal_R7< R,A1,A2,A3,A4,A5,A6,A7 >()(                             \
                    getClientStub(),                                                                    \
                    rcs,                                                                                \
                    ::RCF::getInterfaceName( (Interface *) NULL),                                       \
                    id::value,                                                                          \
                    a1, a2, a3, a4, a5, a6, a7);                                                        \
            }                                                                                           \
                                                                                                        \
        private:                                                                                        \
            template<typename T>                                                                        \
            void invoke(                                                                                \
                const id &,                                                                             \
                ::RCF::SerializationProtocolIn &in,                                                     \
                ::RCF::SerializationProtocolOut &out,                                                   \
                T &t)                                                                                   \
            {                                                                                           \
                ::RCF::IDL::InParameter< A1 >::type arg1(in);                                           \
                ::RCF::IDL::InParameter< A2 >::type arg2(in);                                           \
                ::RCF::IDL::InParameter< A3 >::type arg3(in);                                           \
                ::RCF::IDL::InParameter< A4 >::type arg4(in);                                           \
                ::RCF::IDL::InParameter< A5 >::type arg5(in);                                           \
                ::RCF::IDL::InParameter< A6 >::type arg6(in);                                           \
                ::RCF::IDL::InParameter< A7 >::type arg7(in);                                                                                       \
                ::RCF::IDL::OutReturnValue< R >(out, t.func(arg1.get(), arg2.get(), arg3.get(), arg4.get(), arg5.get(), arg6.get(), arg7.get()));   \
                ::RCF::IDL::OutParameter< A1 >::type(arg1, out);                                                                                    \
                ::RCF::IDL::OutParameter< A2 >::type(arg2, out);                                        \
                ::RCF::IDL::OutParameter< A3 >::type(arg3, out);                                        \
                ::RCF::IDL::OutParameter< A4 >::type(arg4, out);                                        \
                ::RCF::IDL::OutParameter< A5 >::type(arg5, out);                                        \
                ::RCF::IDL::OutParameter< A6 >::type(arg6, out);                                        \
                ::RCF::IDL::OutParameter< A7 >::type(arg7, out);                                        \
            }

// RCF_METHOD_R8
/// Defines a member function of an interface, having non-void return type and eight arguments.
/// \param R Return type.
/// \param func Function name.
/// \param A1 Type of first argument.
/// \param A2 Type of second argument.
/// \param A3 Type of third argument.
/// \param A4 Type of fourth argument.
/// \param A5 Type of fifth argument.
/// \param A6 Type of sixth argument.
/// \param A7 Type of seventh argument.
/// \param A8 Type of eighth argument.
#define RCF_METHOD_R8(R,func,A1,A2,A3,A4,A5,A6,A7,A8)                        RCF_METHOD_R8_(R,func,A1,A2,A3,A4,A5,A6,A7,A8, RCF_MAKE_UNIQUE_ID(func, R8))
#define RCF_METHOD_R8_(R,func,A1,A2,A3,A4,A5,A6,A7,A8, id)                                              \
        public:                                                                                         \
            RCF_MAKE_NEXT_DISPATCH_ID(id);                                                              \
            R func(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8)                              \
            {                                                                                               \
                return func(getClientStub().getDefaultCallingSemantics(), a1, a2, a3, a4, a5, a6, a7, a8);  \
            }                                                                                               \
            R func(::RCF::RemoteCallSemantics rcs, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8)  \
            {                                                                                               \
                ::RCF::CurrentClientStubPtrSentry sentry(mClientStubPtr);                               \
                return ::RCF::ClientMarshal_R8< R,A1,A2,A3,A4,A5,A6,A7,A8 >()(                          \
                    getClientStub(),                                                                    \
                    rcs,                                                                                \
                    ::RCF::getInterfaceName( (Interface *) NULL),                                       \
                    id::value,                                                                          \
                    a1, a2, a3, a4, a5, a6, a7, a8);                                                    \
            }                                                                                           \
                                                                                                        \
        private:                                                                                        \
            template<typename T>                                                                        \
            void invoke(                                                                                \
                const id &,                                                                             \
                ::RCF::SerializationProtocolIn &in,                                                     \
                ::RCF::SerializationProtocolOut &out,                                                   \
                T &t)                                                                                   \
            {                                                                                           \
                ::RCF::IDL::InParameter< A1 >::type arg1(in);                                           \
                ::RCF::IDL::InParameter< A2 >::type arg2(in);                                           \
                ::RCF::IDL::InParameter< A3 >::type arg3(in);                                           \
                ::RCF::IDL::InParameter< A4 >::type arg4(in);                                           \
                ::RCF::IDL::InParameter< A5 >::type arg5(in);                                           \
                ::RCF::IDL::InParameter< A6 >::type arg6(in);                                           \
                ::RCF::IDL::InParameter< A7 >::type arg7(in);                                           \
                ::RCF::IDL::InParameter< A8 >::type arg8(in);                                                                                                   \
                ::RCF::IDL::OutReturnValue< R >(out, t.func(arg1.get(), arg2.get(), arg3.get(), arg4.get(), arg5.get(), arg6.get(), arg7.get(), arg8.get()));   \
                ::RCF::IDL::OutParameter< A1 >::type(arg1, out);                                                                                                \
                ::RCF::IDL::OutParameter< A2 >::type(arg2, out);                                        \
                ::RCF::IDL::OutParameter< A3 >::type(arg3, out);                                        \
                ::RCF::IDL::OutParameter< A4 >::type(arg4, out);                                        \
                ::RCF::IDL::OutParameter< A5 >::type(arg5, out);                                        \
                ::RCF::IDL::OutParameter< A6 >::type(arg6, out);                                        \
                ::RCF::IDL::OutParameter< A7 >::type(arg7, out);                                        \
                ::RCF::IDL::OutParameter< A8 >::type(arg8, out);                                        \
            }

// RCF_METHOD_V0
/// Defines a member function of an interface, having void return type and no arguments.
/// \param R Return type - must be void.
/// \param func Function name.
#define RCF_METHOD_V0(R,func)                        RCF_METHOD_V0_(R,func, RCF_MAKE_UNIQUE_ID(func, V0))
#define RCF_METHOD_V0_(R,func, id)                                                                      \
        public:                                                                                         \
            BOOST_STATIC_ASSERT(( boost::is_same<R, void>::value ));                                    \
            RCF_MAKE_NEXT_DISPATCH_ID(id);                                                              \
            void func()                                                                                 \
            {                                                                                           \
                func(getClientStub().getDefaultCallingSemantics());                                     \
            }                                                                                           \
            void func(::RCF::RemoteCallSemantics rcs)                                                   \
            {                                                                                           \
                ::RCF::CurrentClientStubPtrSentry sentry(mClientStubPtr);                               \
                ::RCF::ClientMarshal_R0< RCF::Void>()(                                                  \
                    getClientStub(),                                                                    \
                    rcs,                                                                                \
                    ::RCF::getInterfaceName( (Interface *) NULL),                                       \
                    id::value);                                                                         \
            }                                                                                           \
                                                                                                        \
        private:                                                                                        \
            template<typename T>                                                                        \
            void invoke(                                                                                \
                const id &,                                                                             \
                ::RCF::SerializationProtocolIn &in,                                                     \
                ::RCF::SerializationProtocolOut &out,                                                   \
                T &t)                                                                                   \
            {                                                                                           \
                RCF_UNUSED_VARIABLE(in);                                                                \
                ::RCF::IDL::OutReturnValue<>(out, (t.func(), 0));                                       \
            }

// RCF_METHOD_V1
/// Defines a member function of an interface, having void return type and one argument.
/// \param R Return type - must be void
/// \param func Function name.
/// \param A1 Type of first argument.
#define RCF_METHOD_V1(R,func,A1)                        RCF_METHOD_V1_(R,func,A1, RCF_MAKE_UNIQUE_ID(func, V1))
#define RCF_METHOD_V1_(R,func,A1, id)                                                                   \
        public:                                                                                         \
            BOOST_STATIC_ASSERT(( boost::is_same<R, void>::value ));                                    \
            RCF_MAKE_NEXT_DISPATCH_ID(id);                                                              \
            void func(A1 a1)                                                                            \
            {                                                                                           \
                func(getClientStub().getDefaultCallingSemantics(), a1);                                 \
            }                                                                                           \
            void func(::RCF::RemoteCallSemantics rcs, A1 a1)                                            \
            {                                                                                           \
                ::RCF::CurrentClientStubPtrSentry sentry(mClientStubPtr);                               \
                ::RCF::ClientMarshal_R1< RCF::Void, A1 >()(                                             \
                    getClientStub(),                                                                    \
                    rcs,                                                                                \
                    ::RCF::getInterfaceName( (Interface *) NULL),                                       \
                    id::value,                                                                          \
                    a1);                                                                                \
            }                                                                                           \
                                                                                                        \
        private:                                                                                        \
            template<typename T>                                                                        \
            void invoke(                                                                                \
                const id &,                                                                             \
                ::RCF::SerializationProtocolIn &in,                                                     \
                ::RCF::SerializationProtocolOut &out,                                                   \
                T &t)                                                                                   \
            {                                                                                           \
                ::RCF::IDL::InParameter< A1 >::type arg1(in);                                           \
                ::RCF::IDL::OutReturnValue<>(out, (t.func(arg1.get()), 0));                             \
                ::RCF::IDL::OutParameter< A1 >::type(arg1, out);                                        \
            }


// RCF_METHOD_V2
/// Defines a member function of an interface, having void return type and two arguments.
/// \param R Return type - must be void.
/// \param func Function name.
/// \param A1 Type of first argument.
/// \param A2 Type of second argument.
#define RCF_METHOD_V2(R,func,A1,A2)                        RCF_METHOD_V2_(R,func,A1,A2, RCF_MAKE_UNIQUE_ID(func, V2))
#define RCF_METHOD_V2_(R,func,A1,A2, id)                                                                \
        public:                                                                                         \
            BOOST_STATIC_ASSERT(( boost::is_same<R, void>::value ));                                    \
            RCF_MAKE_NEXT_DISPATCH_ID(id);                                                              \
            void func(A1 a1, A2 a2)                                                                     \
            {                                                                                           \
                func(getClientStub().getDefaultCallingSemantics(), a1, a2);                             \
            }                                                                                           \
            void func(::RCF::RemoteCallSemantics rcs, A1 a1, A2 a2)                                     \
            {                                                                                           \
                ::RCF::CurrentClientStubPtrSentry sentry(mClientStubPtr);                               \
                ::RCF::ClientMarshal_R2< RCF::Void, A1, A2 >()(                                         \
                    getClientStub(),                                                                    \
                    rcs,                                                                                \
                    ::RCF::getInterfaceName( (Interface *) NULL),                                       \
                    id::value,                                                                          \
                    a1, a2);                                                                            \
            }                                                                                           \
                                                                                                        \
        private:                                                                                        \
            template<typename T>                                                                        \
            void invoke(                                                                                \
                const id &,                                                                             \
                ::RCF::SerializationProtocolIn &in,                                                     \
                ::RCF::SerializationProtocolOut &out,                                                   \
                T &t)                                                                                   \
            {                                                                                           \
                ::RCF::IDL::InParameter< A1 >::type arg1(in);                                           \
                ::RCF::IDL::InParameter< A2 >::type arg2(in);                                           \
                ::RCF::IDL::OutReturnValue<>(out, (t.func(arg1.get(), arg2.get()), 0));                 \
                ::RCF::IDL::OutParameter< A1 >::type(arg1, out);                                        \
                ::RCF::IDL::OutParameter< A2 >::type(arg2, out);                                        \
            }


// RCF_METHOD_V3
/// Defines a member function of an interface, having void return type and three arguments.
/// \param R Return type - must be void.
/// \param func Function name.
/// \param A1 Type of first argument.
/// \param A2 Type of second argument.
/// \param A3 Type of third argument.
#define RCF_METHOD_V3(R,func,A1,A2,A3)                        RCF_METHOD_V3_(R,func,A1,A2,A3, RCF_MAKE_UNIQUE_ID(func, V3))
#define RCF_METHOD_V3_(R,func,A1,A2,A3, id)                                                             \
        public:                                                                                         \
            BOOST_STATIC_ASSERT(( boost::is_same<R, void>::value ));                                    \
            RCF_MAKE_NEXT_DISPATCH_ID(id);                                                              \
            void func(A1 a1, A2 a2, A3 a3)                                                              \
            {                                                                                           \
                func(getClientStub().getDefaultCallingSemantics(), a1, a2, a3);                         \
            }                                                                                           \
            void func(::RCF::RemoteCallSemantics rcs, A1 a1, A2 a2, A3 a3)                              \
            {                                                                                           \
                ::RCF::CurrentClientStubPtrSentry sentry(mClientStubPtr);                               \
                ::RCF::ClientMarshal_R3< RCF::Void, A1, A2, A3 >()(                                     \
                    getClientStub(),                                                                    \
                    rcs,                                                                                \
                    ::RCF::getInterfaceName( (Interface *) NULL),                                       \
                    id::value,                                                                          \
                    a1, a2, a3);                                                                        \
            }                                                                                           \
                                                                                                        \
        private:                                                                                        \
            template<typename T>                                                                        \
            void invoke(                                                                                \
                const id &,                                                                             \
                ::RCF::SerializationProtocolIn &in,                                                     \
                ::RCF::SerializationProtocolOut &out,                                                   \
                T &t)                                                                                   \
            {                                                                                           \
                ::RCF::IDL::InParameter< A1 >::type arg1(in);                                           \
                ::RCF::IDL::InParameter< A2 >::type arg2(in);                                           \
                ::RCF::IDL::InParameter< A3 >::type arg3(in);                                           \
                ::RCF::IDL::OutReturnValue<>(out, (t.func(arg1.get(), arg2.get(), arg3.get()), 0));     \
                ::RCF::IDL::OutParameter< A1 >::type(arg1, out);                                        \
                ::RCF::IDL::OutParameter< A2 >::type(arg2, out);                                        \
                ::RCF::IDL::OutParameter< A3 >::type(arg3, out);                                        \
            }

// RCF_METHOD_V4
/// Defines a member function of an interface, having void return type and four arguments.
/// \param R Return type - must be void.
/// \param func Function name.
/// \param A1 Type of first argument.
/// \param A2 Type of second argument.
/// \param A3 Type of third argument.
/// \param A4 Type of fourth argument.
#define RCF_METHOD_V4(R,func,A1,A2,A3,A4)                        RCF_METHOD_V4_(R,func,A1,A2,A3,A4, RCF_MAKE_UNIQUE_ID(func, V4))
#define RCF_METHOD_V4_(R,func,A1,A2,A3,A4, id)                                                          \
        public:                                                                                         \
            BOOST_STATIC_ASSERT(( boost::is_same<R, void>::value ));                                    \
            RCF_MAKE_NEXT_DISPATCH_ID(id);                                                              \
            void func(A1 a1, A2 a2, A3 a3, A4 a4)                                                       \
            {                                                                                           \
                func(getClientStub().getDefaultCallingSemantics(), a1, a2, a3, a4);                     \
            }                                                                                           \
            void func(::RCF::RemoteCallSemantics rcs, A1 a1, A2 a2, A3 a3, A4 a4)                       \
            {                                                                                           \
                ::RCF::CurrentClientStubPtrSentry sentry(mClientStubPtr);                               \
                ::RCF::ClientMarshal_R4< RCF::Void, A1, A2, A3, A4 >()(                                 \
                    getClientStub(),                                                                    \
                    rcs,                                                                                \
                    ::RCF::getInterfaceName( (Interface *) NULL),                                       \
                    id::value,                                                                          \
                    a1, a2, a3, a4);                                                                    \
            }                                                                                           \
                                                                                                        \
        private:                                                                                        \
            template<typename T>                                                                        \
            void invoke(                                                                                \
                const id &,                                                                             \
                ::RCF::SerializationProtocolIn &in,                                                     \
                ::RCF::SerializationProtocolOut &out,                                                   \
                T &t)                                                                                   \
            {                                                                                           \
                ::RCF::IDL::InParameter< A1 >::type arg1(in);                                           \
                ::RCF::IDL::InParameter< A2 >::type arg2(in);                                           \
                ::RCF::IDL::InParameter< A3 >::type arg3(in);                                           \
                ::RCF::IDL::InParameter< A4 >::type arg4(in);                                                       \
                ::RCF::IDL::OutReturnValue<>(out, (t.func(arg1.get(), arg2.get(), arg3.get(), arg4.get()), 0));     \
                ::RCF::IDL::OutParameter< A1 >::type(arg1, out);                                                    \
                ::RCF::IDL::OutParameter< A2 >::type(arg2, out);                                        \
                ::RCF::IDL::OutParameter< A3 >::type(arg3, out);                                        \
                ::RCF::IDL::OutParameter< A4 >::type(arg4, out);                                        \
            }

// RCF_METHOD_V5
/// Defines a member function of an interface, having void return type and five arguments.
/// \param R Return type - must be void.
/// \param func Function name.
/// \param A1 Type of first argument.
/// \param A2 Type of second argument.
/// \param A3 Type of third argument.
/// \param A4 Type of fourth argument.
/// \param A5 Type of fifth argument.
#define RCF_METHOD_V5(R,func,A1,A2,A3,A4,A5)                        RCF_METHOD_V5_(R,func,A1,A2,A3,A4,A5, RCF_MAKE_UNIQUE_ID(func, V5))
#define RCF_METHOD_V5_(R,func,A1,A2,A3,A4,A5, id)                                                       \
        public:                                                                                         \
            BOOST_STATIC_ASSERT(( boost::is_same<R, void>::value ));                                    \
            RCF_MAKE_NEXT_DISPATCH_ID(id);                                                              \
            void func(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5)                                                \
            {                                                                                           \
                func(getClientStub().getDefaultCallingSemantics(), a1, a2, a3, a4, a5);                 \
            }                                                                                           \
            void func(::RCF::RemoteCallSemantics rcs, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5)                \
            {                                                                                           \
                ::RCF::CurrentClientStubPtrSentry sentry(mClientStubPtr);                               \
                ::RCF::ClientMarshal_R5< RCF::Void, A1, A2, A3, A4, A5 >()(                             \
                    getClientStub(),                                                                    \
                    rcs,                                                                                \
                    ::RCF::getInterfaceName( (Interface *) NULL),                                       \
                    id::value,                                                                          \
                    a1, a2, a3, a4, a5);                                                                \
            }                                                                                           \
                                                                                                        \
        private:                                                                                        \
            template<typename T>                                                                        \
            void invoke(                                                                                \
                const id &,                                                                             \
                ::RCF::SerializationProtocolIn &in,                                                     \
                ::RCF::SerializationProtocolOut &out,                                                   \
                T &t)                                                                                   \
            {                                                                                           \
                ::RCF::IDL::InParameter< A1 >::type arg1(in);                                           \
                ::RCF::IDL::InParameter< A2 >::type arg2(in);                                           \
                ::RCF::IDL::InParameter< A3 >::type arg3(in);                                           \
                ::RCF::IDL::InParameter< A4 >::type arg4(in);                                           \
                ::RCF::IDL::InParameter< A5 >::type arg5(in);                                                                   \
                ::RCF::IDL::OutReturnValue<>(out, (t.func(arg1.get(), arg2.get(), arg3.get(), arg4.get(), arg5.get()), 0));     \
                ::RCF::IDL::OutParameter< A1 >::type(arg1, out);                                                                \
                ::RCF::IDL::OutParameter< A2 >::type(arg2, out);                                        \
                ::RCF::IDL::OutParameter< A3 >::type(arg3, out);                                        \
                ::RCF::IDL::OutParameter< A4 >::type(arg4, out);                                        \
                ::RCF::IDL::OutParameter< A5 >::type(arg5, out);                                        \
            }

// RCF_METHOD_V6
/// Defines a member function of an interface, having void return type and six arguments.
/// \param R Return type - must be void.
/// \param func Function name.
/// \param A1 Type of first argument.
/// \param A2 Type of second argument.
/// \param A3 Type of third argument.
/// \param A4 Type of fourth argument.
/// \param A5 Type of fifth argument.
/// \param A6 Type of sixth argument.
#define RCF_METHOD_V6(R,func,A1,A2,A3,A4,A5,A6)                        RCF_METHOD_V6_(R,func,A1,A2,A3,A4,A5,A6, RCF_MAKE_UNIQUE_ID(func, V6))
#define RCF_METHOD_V6_(R,func,A1,A2,A3,A4,A5,A6, id)                                                    \
        public:                                                                                         \
            BOOST_STATIC_ASSERT(( boost::is_same<R, void>::value ));                                    \
            RCF_MAKE_NEXT_DISPATCH_ID(id);                                                              \
            void func(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6)                                         \
            {                                                                                           \
                func(getClientStub().getDefaultCallingSemantics(), a1, a2, a3, a4, a5, a6);             \
            }                                                                                           \
            void func(::RCF::RemoteCallSemantics rcs, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6)         \
            {                                                                                           \
                ::RCF::CurrentClientStubPtrSentry sentry(mClientStubPtr);                               \
                ::RCF::ClientMarshal_R6< RCF::Void, A1, A2, A3, A4, A5, A6 >()(                         \
                    getClientStub(),                                                                    \
                    rcs,                                                                                \
                    ::RCF::getInterfaceName( (Interface *) NULL),                                       \
                    id::value,                                                                          \
                    a1, a2, a3, a4, a5, a6);                                                            \
            }                                                                                           \
                                                                                                        \
        private:                                                                                        \
            template<typename T>                                                                        \
            void invoke(                                                                                \
                const id &,                                                                             \
                ::RCF::SerializationProtocolIn &in,                                                     \
                ::RCF::SerializationProtocolOut &out,                                                   \
                T &t)                                                                                   \
            {                                                                                           \
                ::RCF::IDL::InParameter< A1 >::type arg1(in);                                           \
                ::RCF::IDL::InParameter< A2 >::type arg2(in);                                           \
                ::RCF::IDL::InParameter< A3 >::type arg3(in);                                           \
                ::RCF::IDL::InParameter< A4 >::type arg4(in);                                           \
                ::RCF::IDL::InParameter< A5 >::type arg5(in);                                           \
                ::RCF::IDL::InParameter< A6 >::type arg6(in);                                                                               \
                ::RCF::IDL::OutReturnValue<>(out, (t.func(arg1.get(), arg2.get(), arg3.get(), arg4.get(), arg5.get(), arg6.get()), 0));     \
                ::RCF::IDL::OutParameter< A1 >::type(arg1, out);                                                                            \
                ::RCF::IDL::OutParameter< A2 >::type(arg2, out);                                        \
                ::RCF::IDL::OutParameter< A3 >::type(arg3, out);                                        \
                ::RCF::IDL::OutParameter< A4 >::type(arg4, out);                                        \
                ::RCF::IDL::OutParameter< A5 >::type(arg5, out);                                        \
                ::RCF::IDL::OutParameter< A6 >::type(arg6, out);                                        \
            }

// RCF_METHOD_V7
/// Defines a member function of an interface, having void return type and seven arguments.
/// \param R Return type - must be void.
/// \param func Function name.
/// \param A1 Type of first argument.
/// \param A2 Type of second argument.
/// \param A3 Type of third argument.
/// \param A4 Type of fourth argument.
/// \param A5 Type of fifth argument.
/// \param A6 Type of sixth argument.
/// \param A7 Type of seventh argument.
#define RCF_METHOD_V7(R,func,A1,A2,A3,A4,A5,A6,A7)                        RCF_METHOD_V7_(R,func,A1,A2,A3,A4,A5,A6,A7, RCF_MAKE_UNIQUE_ID(func, V7))
#define RCF_METHOD_V7_(R,func,A1,A2,A3,A4,A5,A6,A7, id)                                                 \
        public:                                                                                         \
            BOOST_STATIC_ASSERT(( boost::is_same<R, void>::value ));                                    \
            RCF_MAKE_NEXT_DISPATCH_ID(id);                                                              \
            void func(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7)                                  \
            {                                                                                           \
                func(getClientStub().getDefaultCallingSemantics(), a1, a2, a3, a4, a5, a6, a7);         \
            }                                                                                           \
            void func(::RCF::RemoteCallSemantics rcs, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7)  \
            {                                                                                           \
                ::RCF::CurrentClientStubPtrSentry sentry(mClientStubPtr);                               \
                ::RCF::ClientMarshal_R7< RCF::Void, A1, A2, A3, A4, A5, A6, A7 >()(                     \
                    getClientStub(),                                                                    \
                    rcs,                                                                                \
                    ::RCF::getInterfaceName( (Interface *) NULL),                                       \
                    id::value,                                                                          \
                    a1, a2, a3, a4, a5, a6, a7);                                                        \
            }                                                                                           \
                                                                                                        \
        private:                                                                                        \
            template<typename T>                                                                        \
            void invoke(                                                                                \
                const id &,                                                                             \
                ::RCF::SerializationProtocolIn &in,                                                     \
                ::RCF::SerializationProtocolOut &out,                                                   \
                T &t)                                                                                   \
            {                                                                                           \
                ::RCF::IDL::InParameter< A1 >::type arg1(in);                                           \
                ::RCF::IDL::InParameter< A2 >::type arg2(in);                                           \
                ::RCF::IDL::InParameter< A3 >::type arg3(in);                                           \
                ::RCF::IDL::InParameter< A4 >::type arg4(in);                                           \
                ::RCF::IDL::InParameter< A5 >::type arg5(in);                                           \
                ::RCF::IDL::InParameter< A6 >::type arg6(in);                                           \
                ::RCF::IDL::InParameter< A7 >::type arg7(in);                                                                                           \
                ::RCF::IDL::OutReturnValue<>(out, (t.func(arg1.get(), arg2.get(), arg3.get(), arg4.get(), arg5.get(), arg6.get(), arg7.get()), 0));     \
                ::RCF::IDL::OutParameter< A1 >::type(arg1, out);                                                                                        \
                ::RCF::IDL::OutParameter< A2 >::type(arg2, out);                                        \
                ::RCF::IDL::OutParameter< A3 >::type(arg3, out);                                        \
                ::RCF::IDL::OutParameter< A4 >::type(arg4, out);                                        \
                ::RCF::IDL::OutParameter< A5 >::type(arg5, out);                                        \
                ::RCF::IDL::OutParameter< A6 >::type(arg6, out);                                        \
                ::RCF::IDL::OutParameter< A7 >::type(arg7, out);                                        \
            }

// RCF_METHOD_V8
/// Defines a member function of an interface, having void return type and eight arguments.
/// \param R Return type - must be void.
/// \param func Function name.
/// \param A1 Type of first argument.
/// \param A2 Type of second argument.
/// \param A3 Type of third argument.
/// \param A4 Type of fourth argument.
/// \param A5 Type of fifth argument.
/// \param A6 Type of sixth argument.
/// \param A7 Type of seventh argument.
/// \param A8 Type of eighth argument.
#define RCF_METHOD_V8(R,func,A1,A2,A3,A4,A5,A6,A7,A8)                        RCF_METHOD_V8_(R,func,A1,A2,A3,A4,A5,A6,A7,A8, RCF_MAKE_UNIQUE_ID(func, V8))
#define RCF_METHOD_V8_(R,func,A1,A2,A3,A4,A5,A6,A7,A8, id)                                              \
        public:                                                                                         \
            BOOST_STATIC_ASSERT(( boost::is_same<R, void>::value ));                                    \
            RCF_MAKE_NEXT_DISPATCH_ID(id);                                                              \
            void func(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8)                           \
            {                                                                                           \
                func(getClientStub().getDefaultCallingSemantics(), a1, a2, a3, a4, a5, a6, a7, a8);     \
            }                                                                                                   \
            void func(::RCF::RemoteCallSemantics rcs, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8)   \
            {                                                                                                   \
                ::RCF::CurrentClientStubPtrSentry sentry(mClientStubPtr);                               \
                ::RCF::ClientMarshal_R8< RCF::Void, A1, A2, A3, A4, A5, A6, A7, A8 >()(                 \
                    getClientStub(),                                                                    \
                    rcs,                                                                                \
                    ::RCF::getInterfaceName( (Interface *) NULL),                                       \
                    id::value,                                                                          \
                    a1, a2, a3, a4, a5, a6, a7, a8);                                                    \
            }                                                                                           \
                                                                                                        \
        private:                                                                                        \
            template<typename T>                                                                        \
            void invoke(                                                                                \
                const id &,                                                                             \
                ::RCF::SerializationProtocolIn &in,                                                     \
                ::RCF::SerializationProtocolOut &out,                                                   \
                T &t)                                                                                   \
            {                                                                                           \
                ::RCF::IDL::InParameter< A1 >::type arg1(in);                                           \
                ::RCF::IDL::InParameter< A2 >::type arg2(in);                                           \
                ::RCF::IDL::InParameter< A3 >::type arg3(in);                                           \
                ::RCF::IDL::InParameter< A4 >::type arg4(in);                                           \
                ::RCF::IDL::InParameter< A5 >::type arg5(in);                                           \
                ::RCF::IDL::InParameter< A6 >::type arg6(in);                                           \
                ::RCF::IDL::InParameter< A7 >::type arg7(in);                                           \
                ::RCF::IDL::InParameter< A8 >::type arg8(in);                                                                                                       \
                ::RCF::IDL::OutReturnValue<>(out, (t.func(arg1.get(), arg2.get(), arg3.get(), arg4.get(), arg5.get(), arg6.get(), arg7.get(), arg8.get()), 0));     \
                ::RCF::IDL::OutParameter< A1 >::type(arg1, out);                                                                                                    \
                ::RCF::IDL::OutParameter< A2 >::type(arg2, out);                                        \
                ::RCF::IDL::OutParameter< A3 >::type(arg3, out);                                        \
                ::RCF::IDL::OutParameter< A4 >::type(arg4, out);                                        \
                ::RCF::IDL::OutParameter< A5 >::type(arg5, out);                                        \
                ::RCF::IDL::OutParameter< A6 >::type(arg6, out);                                        \
                ::RCF::IDL::OutParameter< A7 >::type(arg7, out);                                        \
                ::RCF::IDL::OutParameter< A8 >::type(arg8, out);                                        \
            }


// RCF_MAKE_UNIQUE_ID

BOOST_STATIC_ASSERT( sizeof(RCF::defined_) != sizeof(RCF::default_));

#define RCF_MAKE_UNIQUE_ID(func, sig)                       RCF_MAKE_UNIQUE_ID_(func, sig, __LINE__)
#define RCF_MAKE_UNIQUE_ID_(func, sig, __LINE__)            RCF_MAKE_UNIQUE_ID__(func, sig, __LINE__)
#define RCF_MAKE_UNIQUE_ID__(func, sig, Line)               rcf_unique_id_##func##_##sig##_##Line

#define RCF_MAKE_NEXT_DISPATCH_ID(next_dispatch_id)                                                                                                                            \
    typedef                                                                                                                                                                    \
    boost::mpl::if_< boost::mpl::bool_< (sizeof(RCF_make_next_dispatch_id_func((DummyThisT *) 0, (ThisT *) 0, (boost::mpl::int_< 0> *) 0)) == sizeof(RCF::defined_)) >,        \
    boost::mpl::if_< boost::mpl::bool_< (sizeof(RCF_make_next_dispatch_id_func((DummyThisT *) 0, (ThisT *) 0, (boost::mpl::int_< 1> *) 0)) == sizeof(RCF::defined_)) >,        \
    boost::mpl::if_< boost::mpl::bool_< (sizeof(RCF_make_next_dispatch_id_func((DummyThisT *) 0, (ThisT *) 0, (boost::mpl::int_< 2> *) 0)) == sizeof(RCF::defined_)) >,        \
    boost::mpl::if_< boost::mpl::bool_< (sizeof(RCF_make_next_dispatch_id_func((DummyThisT *) 0, (ThisT *) 0, (boost::mpl::int_< 3> *) 0)) == sizeof(RCF::defined_)) >,        \
    boost::mpl::if_< boost::mpl::bool_< (sizeof(RCF_make_next_dispatch_id_func((DummyThisT *) 0, (ThisT *) 0, (boost::mpl::int_< 4> *) 0)) == sizeof(RCF::defined_)) >,        \
    boost::mpl::if_< boost::mpl::bool_< (sizeof(RCF_make_next_dispatch_id_func((DummyThisT *) 0, (ThisT *) 0, (boost::mpl::int_< 5> *) 0)) == sizeof(RCF::defined_)) >,        \
    boost::mpl::if_< boost::mpl::bool_< (sizeof(RCF_make_next_dispatch_id_func((DummyThisT *) 0, (ThisT *) 0, (boost::mpl::int_< 6> *) 0)) == sizeof(RCF::defined_)) >,        \
    boost::mpl::if_< boost::mpl::bool_< (sizeof(RCF_make_next_dispatch_id_func((DummyThisT *) 0, (ThisT *) 0, (boost::mpl::int_< 7> *) 0)) == sizeof(RCF::defined_)) >,        \
    boost::mpl::if_< boost::mpl::bool_< (sizeof(RCF_make_next_dispatch_id_func((DummyThisT *) 0, (ThisT *) 0, (boost::mpl::int_< 8> *) 0)) == sizeof(RCF::defined_)) >,        \
    boost::mpl::if_< boost::mpl::bool_< (sizeof(RCF_make_next_dispatch_id_func((DummyThisT *) 0, (ThisT *) 0, (boost::mpl::int_< 9> *) 0)) == sizeof(RCF::defined_)) >,        \
    boost::mpl::if_< boost::mpl::bool_< (sizeof(RCF_make_next_dispatch_id_func((DummyThisT *) 0, (ThisT *) 0, (boost::mpl::int_<10> *) 0)) == sizeof(RCF::defined_)) >,        \
    boost::mpl::if_< boost::mpl::bool_< (sizeof(RCF_make_next_dispatch_id_func((DummyThisT *) 0, (ThisT *) 0, (boost::mpl::int_<11> *) 0)) == sizeof(RCF::defined_)) >,        \
    boost::mpl::if_< boost::mpl::bool_< (sizeof(RCF_make_next_dispatch_id_func((DummyThisT *) 0, (ThisT *) 0, (boost::mpl::int_<12> *) 0)) == sizeof(RCF::defined_)) >,        \
    boost::mpl::if_< boost::mpl::bool_< (sizeof(RCF_make_next_dispatch_id_func((DummyThisT *) 0, (ThisT *) 0, (boost::mpl::int_<13> *) 0)) == sizeof(RCF::defined_)) >,        \
    boost::mpl::if_< boost::mpl::bool_< (sizeof(RCF_make_next_dispatch_id_func((DummyThisT *) 0, (ThisT *) 0, (boost::mpl::int_<14> *) 0)) == sizeof(RCF::defined_)) >,        \
    boost::mpl::if_< boost::mpl::bool_< (sizeof(RCF_make_next_dispatch_id_func((DummyThisT *) 0, (ThisT *) 0, (boost::mpl::int_<15> *) 0)) == sizeof(RCF::defined_)) >,        \
    boost::mpl::if_< boost::mpl::bool_< (sizeof(RCF_make_next_dispatch_id_func((DummyThisT *) 0, (ThisT *) 0, (boost::mpl::int_<16> *) 0)) == sizeof(RCF::defined_)) >,        \
    boost::mpl::if_< boost::mpl::bool_< (sizeof(RCF_make_next_dispatch_id_func((DummyThisT *) 0, (ThisT *) 0, (boost::mpl::int_<17> *) 0)) == sizeof(RCF::defined_)) >,        \
    boost::mpl::if_< boost::mpl::bool_< (sizeof(RCF_make_next_dispatch_id_func((DummyThisT *) 0, (ThisT *) 0, (boost::mpl::int_<18> *) 0)) == sizeof(RCF::defined_)) >,        \
    boost::mpl::if_< boost::mpl::bool_< (sizeof(RCF_make_next_dispatch_id_func((DummyThisT *) 0, (ThisT *) 0, (boost::mpl::int_<19> *) 0)) == sizeof(RCF::defined_)) >,        \
    boost::mpl::if_< boost::mpl::bool_< (sizeof(RCF_make_next_dispatch_id_func((DummyThisT *) 0, (ThisT *) 0, (boost::mpl::int_<20> *) 0)) == sizeof(RCF::defined_)) >,        \
    boost::mpl::if_< boost::mpl::bool_< (sizeof(RCF_make_next_dispatch_id_func((DummyThisT *) 0, (ThisT *) 0, (boost::mpl::int_<21> *) 0)) == sizeof(RCF::defined_)) >,        \
    boost::mpl::if_< boost::mpl::bool_< (sizeof(RCF_make_next_dispatch_id_func((DummyThisT *) 0, (ThisT *) 0, (boost::mpl::int_<22> *) 0)) == sizeof(RCF::defined_)) >,        \
    boost::mpl::if_< boost::mpl::bool_< (sizeof(RCF_make_next_dispatch_id_func((DummyThisT *) 0, (ThisT *) 0, (boost::mpl::int_<23> *) 0)) == sizeof(RCF::defined_)) >,        \
    boost::mpl::if_< boost::mpl::bool_< (sizeof(RCF_make_next_dispatch_id_func((DummyThisT *) 0, (ThisT *) 0, (boost::mpl::int_<24> *) 0)) == sizeof(RCF::defined_)) >,        \
    boost::mpl::if_< boost::mpl::bool_< (sizeof(RCF_make_next_dispatch_id_func((DummyThisT *) 0, (ThisT *) 0, (boost::mpl::int_<25> *) 0)) == sizeof(RCF::defined_)) >,        \
    boost::mpl::if_< boost::mpl::bool_< (sizeof(RCF_make_next_dispatch_id_func((DummyThisT *) 0, (ThisT *) 0, (boost::mpl::int_<26> *) 0)) == sizeof(RCF::defined_)) >,        \
    boost::mpl::if_< boost::mpl::bool_< (sizeof(RCF_make_next_dispatch_id_func((DummyThisT *) 0, (ThisT *) 0, (boost::mpl::int_<27> *) 0)) == sizeof(RCF::defined_)) >,        \
    boost::mpl::if_< boost::mpl::bool_< (sizeof(RCF_make_next_dispatch_id_func((DummyThisT *) 0, (ThisT *) 0, (boost::mpl::int_<28> *) 0)) == sizeof(RCF::defined_)) >,        \
    boost::mpl::if_< boost::mpl::bool_< (sizeof(RCF_make_next_dispatch_id_func((DummyThisT *) 0, (ThisT *) 0, (boost::mpl::int_<29> *) 0)) == sizeof(RCF::defined_)) >,        \
    boost::mpl::if_< boost::mpl::bool_< (sizeof(RCF_make_next_dispatch_id_func((DummyThisT *) 0, (ThisT *) 0, (boost::mpl::int_<30> *) 0)) == sizeof(RCF::defined_)) >,        \
    boost::mpl::if_< boost::mpl::bool_< (sizeof(RCF_make_next_dispatch_id_func((DummyThisT *) 0, (ThisT *) 0, (boost::mpl::int_<31> *) 0)) == sizeof(RCF::defined_)) >,        \
    boost::mpl::if_< boost::mpl::bool_< (sizeof(RCF_make_next_dispatch_id_func((DummyThisT *) 0, (ThisT *) 0, (boost::mpl::int_<32> *) 0)) == sizeof(RCF::defined_)) >,        \
    boost::mpl::if_< boost::mpl::bool_< (sizeof(RCF_make_next_dispatch_id_func((DummyThisT *) 0, (ThisT *) 0, (boost::mpl::int_<33> *) 0)) == sizeof(RCF::defined_)) >,        \
    boost::mpl::if_< boost::mpl::bool_< (sizeof(RCF_make_next_dispatch_id_func((DummyThisT *) 0, (ThisT *) 0, (boost::mpl::int_<34> *) 0)) == sizeof(RCF::defined_)) >,        \
    boost::mpl::int_<35>,                           \
    boost::mpl::int_<34> >::type,                   \
    boost::mpl::int_<33> >::type,                   \
    boost::mpl::int_<32> >::type,                   \
    boost::mpl::int_<31> >::type,                   \
    boost::mpl::int_<30> >::type,                   \
    boost::mpl::int_<29> >::type,                   \
    boost::mpl::int_<28> >::type,                   \
    boost::mpl::int_<27> >::type,                   \
    boost::mpl::int_<26> >::type,                   \
    boost::mpl::int_<25> >::type,                   \
    boost::mpl::int_<24> >::type,                   \
    boost::mpl::int_<23> >::type,                   \
    boost::mpl::int_<22> >::type,                   \
    boost::mpl::int_<21> >::type,                   \
    boost::mpl::int_<20> >::type,                   \
    boost::mpl::int_<19> >::type,                   \
    boost::mpl::int_<18> >::type,                   \
    boost::mpl::int_<17> >::type,                   \
    boost::mpl::int_<16> >::type,                   \
    boost::mpl::int_<15> >::type,                   \
    boost::mpl::int_<14> >::type,                   \
    boost::mpl::int_<13> >::type,                   \
    boost::mpl::int_<12> >::type,                   \
    boost::mpl::int_<11> >::type,                   \
    boost::mpl::int_<10> >::type,                   \
    boost::mpl::int_< 9> >::type,                   \
    boost::mpl::int_< 8> >::type,                   \
    boost::mpl::int_< 7> >::type,                   \
    boost::mpl::int_< 6> >::type,                   \
    boost::mpl::int_< 5> >::type,                   \
    boost::mpl::int_< 4> >::type,                   \
    boost::mpl::int_< 3> >::type,                   \
    boost::mpl::int_< 2> >::type,                   \
    boost::mpl::int_< 1> >::type,                   \
    boost::mpl::int_< 0> >::type next_dispatch_id;  \
    friend RCF::defined_ RCF_make_next_dispatch_id_func(DummyThisT *, ThisT *, next_dispatch_id *)

#endif // ! INCLUDE_RCF_IDL_HPP
