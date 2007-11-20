
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_MARSHAL_HPP
#define INCLUDE_RCF_MARSHAL_HPP

#include <RCF/ClientStub.hpp>
#include <RCF/CurrentSerializationProtocol.hpp>
#include <RCF/SerializationProtocol.hpp>
#include <RCF/MethodInvocation.hpp>
#include <RCF/RcfSession.hpp>
#include <RCF/Tools.hpp>
#include <RCF/TypeTraits.hpp>
#include <RCF/util/Meta.hpp>

#include <SF/Tools.hpp> // FOR_EACH_PRIMITIVE_TYPE

#include <boost/mpl/and.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/not.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits.hpp>

#ifdef RCF_USE_BOOST_SERIALIZATION
#include <boost/serialization/split_free.hpp>
#endif

// TODO: move code into Marshal.inl

namespace RCF {

    // Boost.Serialization treats pointers of primitive types differently than other types,
    // hence the following hack, which unfortunately overrides all the serialization protocols,
    // not just Boost.Serialization.

#define RCF_DEFINE_PRIMITIVE_POINTER_SERIALIZATION(type)                \
    inline void serializeImpl(                                          \
        SerializationProtocolOut &out,                                  \
        const type *pt,                                                 \
        long int)                                                       \
    {                                                                   \
        serialize(out, *pt);                                            \
    }                                                                   \
                                                                        \
    inline void serializeImpl(                                          \
        SerializationProtocolOut &out,                                  \
        type *const pt,                                                 \
        long int)                                                       \
    {                                                                   \
        serialize(out, *pt);                                            \
    }                                                                   \
                                                                        \
    inline void deserializeImpl(                                        \
        SerializationProtocolIn &in,                                    \
        type *&pt,                                                      \
        long int)                                                       \
    {                                                                   \
        RCF_ASSERT(pt==NULL);                                           \
        pt = new type();                                                \
        deserialize(in, *pt);                                           \
    }

    SF_FOR_EACH_FUNDAMENTAL_TYPE( RCF_DEFINE_PRIMITIVE_POINTER_SERIALIZATION )

#define RCF_DEFINE_PRIMITIVE_POINTER_SERIALIZATION_T3(type)                \
    template<typename T1, typename T2, typename T3>                        \
    inline void serializeImpl(                                          \
        SerializationProtocolOut &out,                                  \
        const type<T1,T2,T3> *pt,                                       \
        int)                                                            \
    {                                                                   \
        serialize(out, *pt);                                            \
    }                                                                   \
                                                                        \
    template<typename T1, typename T2, typename T3>                        \
    inline void serializeImpl(                                          \
        SerializationProtocolOut &out,                                  \
        type<T1,T2,T3> *const pt,                                       \
        int)                                                            \
    {                                                                   \
        serialize(out, *pt);                                            \
    }                                                                   \
                                                                        \
    template<typename T1, typename T2, typename T3>                        \
    inline void deserializeImpl(                                        \
        SerializationProtocolIn &in,                                    \
        type<T1,T2,T3> *&pt,                                            \
        int)                                                            \
    {                                                                   \
        RCF_ASSERT(pt==NULL);                                           \
        pt = new type<T1,T2,T3>();                                      \
        deserialize(in, *pt);                                           \
    }

#if defined(__MWERKS__) || (defined(_MSC_VER) && _MSC_VER < 1310)
    // ambiguity issues with CW
    RCF_DEFINE_PRIMITIVE_POINTER_SERIALIZATION(std::string)
    RCF_DEFINE_PRIMITIVE_POINTER_SERIALIZATION(std::wstring)
#else
    RCF_DEFINE_PRIMITIVE_POINTER_SERIALIZATION_T3(std::basic_string)
#endif

#undef RCF_DEFINE_PRIMITIVE_POINTER_SERIALIZATION

#undef RCF_DEFINE_PRIMITIVE_POINTER_SERIALIZATION_T3

    // Boost.Serialization handles smart pointers very clumsily, so we do those ourselves

    //#define RCF_SERIALIZE_REFCOUNTSMARTPTR(RefCountSmartPtr)

#define RefCountSmartPtr boost::shared_ptr

    template<typename T>
    inline void serializeImpl(
        SerializationProtocolOut &out,
        const RefCountSmartPtr<T> *spt,
        int)
    {
        serialize(out, *spt);
    }

    template<typename T>
    inline void serializeImpl(
        SerializationProtocolOut &out,
        RefCountSmartPtr<T> *const spt,
        int)
    {
        serialize(out, *spt);
    }

    template<typename T>
    inline void deserializeImpl(
        SerializationProtocolIn &in,
        RefCountSmartPtr<T> *&spt,
        int)
    {
        spt = new RefCountSmartPtr<T>();
        deserialize(in, *spt);
    }

    template<typename T>
    inline void serializeImpl(
        SerializationProtocolOut &out,
        const RefCountSmartPtr<T> &spt,
        int)
    {
        serialize(out, spt.get());
    }

    template<typename T>
    inline void deserializeImpl(
        SerializationProtocolIn &in,
        RefCountSmartPtr<T> &spt,
        int)
    {
        T *pt = NULL;
        deserialize(in, pt);

        RefCountSmartPtr<T> *pspt =
            in.mPointerContext.get( (RefCountSmartPtr<T> **) NULL, pt);

        if (pspt == NULL)
        {
            spt = RefCountSmartPtr<T>(pt);

            in.mPointerContext.set(
                &spt,
                pt);
        }
        else
        {
            spt = *pspt;
        }
    }

#undef RefCountSmartPtr

    inline void serializeImpl(
        SerializationProtocolOut &out,
        const ByteBuffer &byteBuffer,
        long int)
    {
        int len = static_cast<int>(byteBuffer.getLength());
        serialize(out, len);
        out.insert(byteBuffer);
    }

    inline void deserializeImpl(
        SerializationProtocolIn &in,
        ByteBuffer &byteBuffer,
        long int)
    {
        int len = 0;
        deserialize(in, len);
        in.extractSlice(byteBuffer, len);
    }

#ifdef RCF_USE_SF_SERIALIZATION

    inline void serialize(SF::Archive &ar, ByteBuffer &byteBuffer, unsigned int)
    {
        // TODO: some subtle issues here, determining which marshalling buffer to access

        SerializationProtocolIn *pIn = NULL;
        SerializationProtocolOut *pOut = NULL;

        ClientStubPtr clientStubPtr = RCF::getCurrentClientStubPtr();
        RcfSessionPtr rcfSessionPtr = RCF::getCurrentRcfSessionPtr();
        if (clientStubPtr)
        {
            pIn = &clientStubPtr->mIn;
            pOut = &clientStubPtr->mOut;
        }
        else if (rcfSessionPtr)
        {
            pIn = &rcfSessionPtr->mIn;
            pOut = &rcfSessionPtr->mOut;
        }

        if (ar.isRead())
        {
            boost::uint32_t len = 0;
            ar & len;

            byteBuffer.clear();

            SerializationProtocolIn *pIn = getCurrentSerializationProtocolIn();

            if (pIn && len)
            {
                pIn->extractSlice(byteBuffer, len);
            }
            else if (len)
            {
                byteBuffer = ByteBuffer(len);

                SF::IStream &is = dynamic_cast<SF::IStream &>(*ar.getStream());

                boost::uint32_t bytesToRead = len;
                boost::uint32_t bytesRead = is.read( (SF::Byte8 *) byteBuffer.getPtr(), bytesToRead);

                RCF_VERIFY(
                    bytesRead == bytesToRead,
                    RCF::Exception(RCF::RcfError_Deserialization))
                    (bytesToRead)(bytesRead);
            }
        }
        else if (ar.isWrite())
        {
            boost::uint32_t len = static_cast<boost::uint32_t>(byteBuffer.getLength());
            ar & len;

            SerializationProtocolOut *pOut = getCurrentSerializationProtocolOut();

            if (pOut && len)
            {
                pOut->insert(byteBuffer);
            }
            else if (len)
            {
                boost::uint32_t bytesToWrite = len;
                dynamic_cast<SF::OStream *>(ar.getStream())->writeRaw(
                    (SF::Byte8 *) byteBuffer.getPtr(),
                    bytesToWrite);
            }
        }
    }

#endif // RCF_USE_SF_SERIALIZATION

#ifdef RCF_USE_BOOST_SERIALIZATION
} // namespace RCF

namespace boost { namespace serialization {

    template<class Archive>
    void save(Archive & ar, const RCF::ByteBuffer &byteBuffer, unsigned int)
    {

        RCF::SerializationProtocolIn *pIn = NULL;
        RCF::SerializationProtocolOut *pOut = NULL;

        RCF::ClientStubPtr clientStubPtr = RCF::getCurrentClientStubPtr();
        RCF::RcfSessionPtr rcfSessionPtr = RCF::getCurrentRcfSessionPtr();
        if (clientStubPtr)
        {
            pIn = &clientStubPtr->mIn;
            pOut = &clientStubPtr->mOut;
        }
        else if (rcfSessionPtr)
        {
            pIn = &rcfSessionPtr->mIn;
            pOut = &rcfSessionPtr->mOut;
        }

        //if (archive.isRead())
        //{
        //    boost::uint32_t len = 0;
        //    archive & len;
        //    pIn->extractSlice(byteBuffer, len);
        //}
        //else if (archive.isWrite())
        {
            boost::uint32_t len = byteBuffer.getLength();
            ar & len;
            pOut->insert(byteBuffer);
        }

    }

    template<class Archive>
    void load(Archive &ar, RCF::ByteBuffer &byteBuffer, unsigned int)
    {

        RCF::SerializationProtocolIn *pIn = NULL;
        RCF::SerializationProtocolOut *pOut = NULL;

        RCF::ClientStubPtr clientStubPtr = RCF::getCurrentClientStubPtr();
        RCF::RcfSessionPtr rcfSessionPtr = RCF::getCurrentRcfSessionPtr();
        if (clientStubPtr)
        {
            pIn = &clientStubPtr->mIn;
            pOut = &clientStubPtr->mOut;
        }
        else if (rcfSessionPtr)
        {
            pIn = &rcfSessionPtr->mIn;
            pOut = &rcfSessionPtr->mOut;
        }

        //if (archive.isRead())
        {
            boost::uint32_t len = 0;
            ar & len;
            pIn->extractSlice(byteBuffer, len);
        }
        //else if (archive.isWrite())
        //{
        //    boost::uint32_t len = byteBuffer.getLength();
        //    archive & len;
        //    pOut->insert(byteBuffer);
        //}

    }
}} // namespace boost namespace serialization

    BOOST_SERIALIZATION_SPLIT_FREE(RCF::ByteBuffer);

namespace RCF {
#endif // RCF_USE_BOOST_SERIALIZATION

    struct Void {};

    namespace IDL {

        class InHeader
        {
        public:
            InHeader(
                ClientStub &clientStub,
                bool oneway,
                const std::string subInterface,
                int fnId)
            {

                RCF4_TRACE("encoding request")
                    (clientStub.getTargetToken())
                    (clientStub.getTargetName())
                    (subInterface)
                    (fnId);

                clientStub.setTries(0);

                clientStub.mRequest.init(
                    clientStub.getTargetToken(),
                    clientStub.getTargetName(),
                    subInterface,
                    fnId,
                    clientStub.getSerializationProtocol(),
                    oneway,
                    false,
                    clientStub.getRcfRuntimeVersion(),
                    false);

                clientStub.mOut.reset(
                    clientStub.getSerializationProtocol(),
                    32,
                    clientStub.mRequest.encodeRequestHeader());
            }
        };

        // TODO: with vc6, mT may be uninitialized if primitive. Probably harmless.
        template<typename T>
        class InParameter_Value
        {
        public:
            BOOST_MPL_ASSERT(( boost::mpl::not_< IsPointer<T> > ));
            BOOST_MPL_ASSERT(( boost::mpl::not_< IsReference<T> > ));

            InParameter_Value(const T &t, SerializationProtocolOut &out) :
                mT()
            {
                serialize(out, t);
            }

            InParameter_Value(SerializationProtocolIn &in) :
                mT()
            {
                deserialize(in, mT);
            }

            const T &get()
            {
                return mT;
            }

        private:
            T mT;
        };

        template<typename T>
        class InParameter_Enum
        {
        public:
            BOOST_MPL_ASSERT(( boost::mpl::not_< IsPointer<T> > ));
            BOOST_MPL_ASSERT(( boost::mpl::not_< IsReference<T> > ));

            // initializer list oddity, due to vc71 breakage on enums
            InParameter_Enum(const T &t, SerializationProtocolOut &out) :
                mT( T())
            {
                serialize(out, t);
            }

            // initializer list oddity, due to vc71 breakage on enums
            InParameter_Enum(SerializationProtocolIn &in) :
                mT( T())
            {
                deserialize(in, mT);
            }

            const T &get()
            {
                return mT;
            }

        private:
            T mT;
        };

        template<typename PtrT>
        class InParameter_Ptr
        {
        public:
            typedef typename RemovePointer<PtrT>::type T;
            typedef typename RemoveCv<T>::type U;
            BOOST_MPL_ASSERT(( IsPointer<PtrT> ));
            BOOST_MPL_ASSERT(( boost::mpl::not_< IsPointer<T> > ));

            InParameter_Ptr(T *pt, SerializationProtocolOut &out)
            {
                serialize(out, pt);
            }

            InParameter_Ptr(SerializationProtocolIn &in)
            {
                U *pu = NULL;
                deserialize(in, pu);

                boost::shared_ptr<U> *ppu =
                    in.mPointerContext.get( (boost::shared_ptr<U> **) NULL, pu);

                if (ppu == NULL)
                {
                    mUPtr = boost::shared_ptr<U>(pu);

                    in.mPointerContext.set(
                        &mUPtr,
                        pu);

                }
                else
                {
                    mUPtr = *ppu;
                }
            }

            T *get()
            {
                return mUPtr.get();
            }

        private:
            boost::shared_ptr<U> mUPtr;
        };

        template<typename RefT>
        class InParameter_Ref
        {
        public:
            typedef typename RemoveReference<RefT>::type T;
            typedef typename RemoveCv<T>::type U;
            BOOST_MPL_ASSERT(( IsReference<RefT> ));
            BOOST_MPL_ASSERT(( boost::mpl::not_< IsPointer<T> > ));

            InParameter_Ref(const U &u, SerializationProtocolOut &out)
            {
                serialize(out, &u);
            }

            InParameter_Ref(SerializationProtocolIn &in)
            {
                U *pu = NULL;
                deserialize(in, pu);

                boost::shared_ptr<U> *ppu =
                    in.mPointerContext.get( (boost::shared_ptr<U>**) NULL, pu);

                if (ppu == NULL)
                {
                    mUPtr = boost::shared_ptr<U>(pu);

                    // TODO: this is only safe if lifetime of in <= lifetime of this!
                    in.mPointerContext.set(
                        &mUPtr,
                        pu);
                }
                else
                {
                    mUPtr = *ppu;
                }
            }

            T &get()
            {
                return *mUPtr;
            }
        private:
            boost::shared_ptr<U> mUPtr;
        };

        template<typename T>
        class OutParameter_Value
        {
        public:
            BOOST_MPL_ASSERT(( boost::mpl::not_< IsPointer<T> > ));
            BOOST_MPL_ASSERT(( boost::mpl::not_< IsReference<T> > ));

            OutParameter_Value(const T &, SerializationProtocolIn &, bool oneway, bool retry)
            {
                RCF_UNUSED_VARIABLE(oneway);
                RCF_UNUSED_VARIABLE(retry);
            }

            template<typename V>
            OutParameter_Value(InParameter_Value< V > &, SerializationProtocolOut &)
            {}

            template<typename V>
            OutParameter_Value(InParameter_Enum< V > &, SerializationProtocolOut &)
            {}
        };

        template<typename T>
        class OutParameter_Enum
        {
        public:
            BOOST_MPL_ASSERT(( boost::mpl::not_< boost::is_pointer<T> > ));
            BOOST_MPL_ASSERT(( boost::mpl::not_< boost::is_reference<T> > ));

            OutParameter_Enum(const T &, SerializationProtocolIn &, bool oneway, bool retry)
            {
                RCF_UNUSED_VARIABLE(oneway);
                RCF_UNUSED_VARIABLE(retry);
            }

            template<typename V>
            OutParameter_Enum(InParameter_Enum< V > &, SerializationProtocolOut &)
            {}
        };

        template<typename PtrT>
        class OutParameter_Ptr
        {
        public:
            typedef typename RemovePointer<PtrT>::type T;
            // TODO: is this innocuous?
#ifndef __BORLANDC__
            BOOST_MPL_ASSERT(( IsPointer<PtrT> ));
#endif
            BOOST_MPL_ASSERT(( boost::mpl::not_< IsPointer<T> > ));

            OutParameter_Ptr(const T *t, SerializationProtocolIn &in, bool oneway, bool retry)
            {
                RCF_UNUSED_VARIABLE(t);
                RCF_UNUSED_VARIABLE(in);
                RCF_UNUSED_VARIABLE(oneway);
                RCF_UNUSED_VARIABLE(retry);
            }

            template<typename V>
            OutParameter_Ptr(InParameter_Ptr< V > &v, SerializationProtocolOut &out)
            {
                RCF_UNUSED_VARIABLE(v);
                RCF_UNUSED_VARIABLE(out);
            }
        };

        template<typename CRefT>
        class OutParameter_CRef
        {
        public:
            typedef typename RemoveReference<CRefT>::type CT;
            typedef typename RemoveCv<CT>::type T;
            BOOST_MPL_ASSERT(( IsReference<CRefT> ));
            // TODO: is this innocuous?
#ifndef __BORLANDC__
            BOOST_MPL_ASSERT(( IsConst<CT> ));
#endif
            BOOST_MPL_ASSERT(( boost::mpl::not_< IsPointer<T> > ));

            OutParameter_CRef(const T &t, SerializationProtocolIn &in, bool oneway, bool retry)
            {
                RCF_UNUSED_VARIABLE(t);
                RCF_UNUSED_VARIABLE(in);
                RCF_UNUSED_VARIABLE(oneway);
                RCF_UNUSED_VARIABLE(retry);
            }

            template<typename V>
            OutParameter_CRef(InParameter_Ref< V > &v, SerializationProtocolOut &out)
            {
                RCF_UNUSED_VARIABLE(v);
                RCF_UNUSED_VARIABLE(out);
            }
        };

        template<typename RefT>
        class OutParameter_Ref
        {
        public:
            typedef typename RemoveReference<RefT>::type T;
            BOOST_MPL_ASSERT(( IsReference<RefT> ));
            BOOST_MPL_ASSERT(( boost::mpl::not_< IsConst<RefT> > ));
            BOOST_MPL_ASSERT(( boost::mpl::not_< IsPointer<T> > ));

            OutParameter_Ref(T &t, SerializationProtocolIn &in, bool oneway, bool retry)
            {
                if (!oneway && !retry)
                {
                    deserialize(in, t);
                }
            }

            template<typename V>
            OutParameter_Ref( InParameter_Ref< V > &v, SerializationProtocolOut &out)
            {
                serialize(out, v.get());
            }
        };

        template<typename T = Void>
        class OutReturnValue
        {
        public:
            // RCF interfaces cannot return naked pointers
            BOOST_MPL_ASSERT(( boost::mpl::not_< IsPointer<T> > ));
            // RCF interfaces cannot return references
            BOOST_MPL_ASSERT(( boost::mpl::not_< IsReference<T> > ));

            OutReturnValue(SerializationProtocolOut &out, const T &t)
            {
                serialize(out, t);
            }
        };

        template<>
        class OutReturnValue<Void>
        {
        public:
            OutReturnValue(SerializationProtocolOut &out, int t)
            {
                RCF_UNUSED_VARIABLE(out);
                RCF_UNUSED_VARIABLE(t);
            }
        };

        template<typename T = Void>
        class InReturnValue_Value
        {
        public:
            // RCF interfaces cannot return naked pointers
            BOOST_MPL_ASSERT(( boost::mpl::not_< IsPointer<T> > ));

            // RCF interfaces cannot return references
            BOOST_MPL_ASSERT(( boost::mpl::not_< IsReference<T> > ));

            InReturnValue_Value(
                ClientStub &clientStub,
                SerializationProtocolIn &in,
                SerializationProtocolOut &out,
                bool oneway,
                bool &retry);

            T &get();

        private:
            T mT;
        };

        template<>
        class InReturnValue_Value<Void>
        {
        public:
            InReturnValue_Value(
                ClientStub &clientStub,
                SerializationProtocolIn &in,
                SerializationProtocolOut &out,
                bool oneway,
                bool &retry);

            Void get()
            {
                return Void();
            }
        };

        template<typename T>
        class InReturnValue_Enum
        {
        public:
            // RCF interfaces cannot return naked pointers
            BOOST_MPL_ASSERT(( boost::mpl::not_< boost::is_pointer<T> > ));

            // RCF interfaces cannot return references
            BOOST_MPL_ASSERT(( boost::mpl::not_< boost::is_reference<T> > ));

            InReturnValue_Enum(
                ClientStub &clientStub,
                SerializationProtocolIn &in,
                SerializationProtocolOut &out,
                bool oneway,
                bool &retry);

            T &get();

        private:
            T mT;
        };

        template<typename T>
        struct InParameter
        {
            typedef typename
            boost::mpl::if_<
                boost::is_pointer<T>,
                InParameter_Ptr<T>,
                typename boost::mpl::if_<
                    boost::is_reference<T>,
                    InParameter_Ref<T>,
                    typename boost::mpl::if_<
                        boost::is_enum<T>,
                        InParameter_Enum<T>,
                        InParameter_Value<T>
                    >::type
                >::type
            >::type type;
        };

        template<typename T>
        struct InReturnValue
        {
            typedef typename
            boost::mpl::if_<
                boost::is_enum<T>,
                InReturnValue_Enum<T>,
                InReturnValue_Value<T>
            >::type type;
        };

#ifdef __BORLANDC__

        template<typename T>
        struct OutParameter
        {
            typedef OutParameter_Value<T> type;
        };

        template<typename T>
        struct OutParameter<T *>
        {
            typedef OutParameter_Ptr<T> type;
        };

        template<typename T>
        struct OutParameter<const T &>
        {
            typedef OutParameter_CRef<const T &> type;
        };

        template<typename T>
        struct OutParameter<T &>
        {
            typedef OutParameter_Ref<T &> type;
        };

#else

        template<typename T>
        struct is_const_reference
        {
            typedef typename
                boost::mpl::and_<
                boost::is_reference<T>,
                boost::is_const< typename boost::remove_reference<T>::type >
                >::type type;

            enum { value = type::value };
        };

        template<typename T>
        struct OutParameter
        {
            typedef typename
            boost::mpl::if_<
                boost::is_pointer<T>,
                OutParameter_Ptr<T>,
                typename boost::mpl::if_<
                    is_const_reference<T>,
                    OutParameter_CRef<T>,
                    typename boost::mpl::if_<
                        boost::is_reference<T>,
                        OutParameter_Ref<T>,
                        typename boost::mpl::if_<
                            boost::is_enum<T>,
                            OutParameter_Enum<T>,
                            OutParameter_Value<T>
                        >::type
                    >::type
                >::type
            >::type type;
        };

#endif

    } // namespace IDL

    // NB: using this instead of scope_guard,because Borland C++ is not triggering the scope_guard at all.
    // At least with this class, it's triggered in debug builds (but not release apparently).
    class ConnectionResetGuard
    {
    public:
        ConnectionResetGuard(ClientStub &clientStub) :
            mClientStub(clientStub),
            mDismissed(RCF_DEFAULT_INIT)
        {}

        void dismiss()
        {
            mDismissed = true;
        }

        ~ConnectionResetGuard()
        {
            RCF_DTOR_BEGIN
            if (!mDismissed)
            {
                mClientStub.disconnect();
            }
            RCF_DTOR_END
        }

    private:
        ClientStub &mClientStub;
        bool mDismissed;
    };

    // ClientMarshal

#ifdef __BORLANDC__
#define RCF_TYPENAME
#else
#define RCF_TYPENAME typename
#endif

    template<typename R>
    class ClientMarshal_R0
    {
    public:
        R operator()(ClientStub &clientStub, RemoteCallSemantics rcs, const std::string &subInterface, int fnId) const
        {
            bool oneway = (Oneway == rcs);
            ConnectionResetGuard connectionResetGuard(clientStub);
            try {
                while (true) {
                    RCF_VERIFY(clientStub.getTries() < 2, Exception(RcfError_RepeatedRetries));
                    bool retry = false;
                    RCF::IDL::InHeader(clientStub, oneway, subInterface, fnId);
                    RCF_TYPENAME RCF::IDL::InReturnValue< R >::type ret(clientStub, clientStub.mIn, clientStub.mOut, oneway, retry);
                    connectionResetGuard.dismiss();
                    clientStub.mIn.clearByteBuffer();
                    if (!retry) return ret.get();
                }
            } catch (const RCF::RemoteException &) { connectionResetGuard.dismiss(); throw; }
        }
    };

    template<typename R, typename A1>
    class ClientMarshal_R1
    {
    public:
        R operator()(ClientStub &clientStub, RemoteCallSemantics rcs, const std::string &subInterface, int fnId, A1 a1) const
        {
            bool oneway = (Oneway == rcs);
            ConnectionResetGuard connectionResetGuard(clientStub);
            try {
                while (true) {
                    RCF_VERIFY(clientStub.getTries() < 2, Exception(RcfError_RepeatedRetries));
                    bool retry = false;
                    RCF::IDL::InHeader(clientStub, oneway, subInterface, fnId);
                    RCF_TYPENAME RCF::IDL::InParameter< A1 >::type(a1, clientStub.mOut);
                    RCF_TYPENAME RCF::IDL::InReturnValue< R >::type ret(clientStub, clientStub.mIn, clientStub.mOut, oneway, retry);
                    RCF_TYPENAME RCF::IDL::OutParameter< A1 >::type(a1, clientStub.mIn, oneway, retry);
                    connectionResetGuard.dismiss();
                    clientStub.mIn.clearByteBuffer();
                    if (!retry) return ret.get();
                }
            } catch (const RCF::RemoteException &) { connectionResetGuard.dismiss(); throw; }
        }
    };

    template<typename R, typename A1, typename A2>
    class ClientMarshal_R2
    {
    public:
        R operator()(ClientStub &clientStub, RemoteCallSemantics rcs, const std::string &subInterface, int fnId, A1 a1, A2 a2) const
        {
            bool oneway = (Oneway == rcs);
            ConnectionResetGuard connectionResetGuard(clientStub);
            try {
                while (true) {
                    RCF_VERIFY(clientStub.getTries() < 2, Exception(RcfError_RepeatedRetries));
                    bool retry = false;
                    RCF::IDL::InHeader(clientStub, oneway, subInterface, fnId);
                    RCF_TYPENAME RCF::IDL::InParameter< A1 >::type(a1, clientStub.mOut);
                    RCF_TYPENAME RCF::IDL::InParameter< A2 >::type(a2, clientStub.mOut);
                    RCF_TYPENAME RCF::IDL::InReturnValue< R >::type ret(clientStub, clientStub.mIn, clientStub.mOut, oneway, retry);
                    RCF_TYPENAME RCF::IDL::OutParameter< A1 >::type(a1, clientStub.mIn, oneway, retry);
                    RCF_TYPENAME RCF::IDL::OutParameter< A2 >::type(a2, clientStub.mIn, oneway, retry);
                    connectionResetGuard.dismiss();
                    clientStub.mIn.clearByteBuffer();
                    if (!retry) return ret.get();
                }
            } catch (const RCF::RemoteException &) { connectionResetGuard.dismiss(); throw; }
        }
    };

    template<typename R, typename A1, typename A2, typename A3>
    class ClientMarshal_R3
    {
    public:
        R operator()(ClientStub &clientStub, RemoteCallSemantics rcs, const std::string &subInterface, int fnId, A1 a1, A2 a2, A3 a3) const
        {
            bool oneway = (Oneway == rcs);
            ConnectionResetGuard connectionResetGuard(clientStub);
            try {
                while (true) {
                    RCF_VERIFY(clientStub.getTries() < 2, Exception(RcfError_RepeatedRetries));
                    bool retry = false;
                    RCF::IDL::InHeader(clientStub, oneway, subInterface, fnId);
                    RCF_TYPENAME RCF::IDL::InParameter< A1 >::type(a1, clientStub.mOut);
                    RCF_TYPENAME RCF::IDL::InParameter< A2 >::type(a2, clientStub.mOut);
                    RCF_TYPENAME RCF::IDL::InParameter< A3 >::type(a3, clientStub.mOut);
                    RCF_TYPENAME RCF::IDL::InReturnValue< R >::type ret(clientStub, clientStub.mIn, clientStub.mOut, oneway, retry);
                    RCF_TYPENAME RCF::IDL::OutParameter< A1 >::type(a1, clientStub.mIn, oneway, retry);
                    RCF_TYPENAME RCF::IDL::OutParameter< A2 >::type(a2, clientStub.mIn, oneway, retry);
                    RCF_TYPENAME RCF::IDL::OutParameter< A3 >::type(a3, clientStub.mIn, oneway, retry);
                    connectionResetGuard.dismiss();
                    clientStub.mIn.clearByteBuffer();
                    if (!retry) return ret.get();
                }
            } catch (const RCF::RemoteException &) { connectionResetGuard.dismiss(); throw; }
        }
    };

    template<typename R, typename A1, typename A2, typename A3, typename A4>
    class ClientMarshal_R4
    {
    public:
        R operator()(ClientStub &clientStub, RemoteCallSemantics rcs, const std::string &subInterface, int fnId, A1 a1, A2 a2, A3 a3, A4 a4) const
        {
            bool oneway = (Oneway == rcs);
            ConnectionResetGuard connectionResetGuard(clientStub);
            try {
                while (true) {
                    RCF_VERIFY(clientStub.getTries() < 2, Exception(RcfError_RepeatedRetries));
                    bool retry = false;
                    RCF::IDL::InHeader(clientStub, oneway, subInterface, fnId);
                    RCF_TYPENAME RCF::IDL::InParameter< A1 >::type(a1, clientStub.mOut);
                    RCF_TYPENAME RCF::IDL::InParameter< A2 >::type(a2, clientStub.mOut);
                    RCF_TYPENAME RCF::IDL::InParameter< A3 >::type(a3, clientStub.mOut);
                    RCF_TYPENAME RCF::IDL::InParameter< A4 >::type(a4, clientStub.mOut);
                    RCF_TYPENAME RCF::IDL::InReturnValue< R >::type ret(clientStub, clientStub.mIn, clientStub.mOut, oneway, retry);
                    RCF_TYPENAME RCF::IDL::OutParameter< A1 >::type(a1, clientStub.mIn, oneway, retry);
                    RCF_TYPENAME RCF::IDL::OutParameter< A2 >::type(a2, clientStub.mIn, oneway, retry);
                    RCF_TYPENAME RCF::IDL::OutParameter< A3 >::type(a3, clientStub.mIn, oneway, retry);
                    RCF_TYPENAME RCF::IDL::OutParameter< A4 >::type(a4, clientStub.mIn, oneway, retry);
                    connectionResetGuard.dismiss();
                    clientStub.mIn.clearByteBuffer();
                    if (!retry) return ret.get();
                }
            } catch (const RCF::RemoteException &) { connectionResetGuard.dismiss(); throw; }
        }
    };

    template<typename R, typename A1, typename A2, typename A3, typename A4, typename A5>
    class ClientMarshal_R5
    {
    public:
        R operator()(ClientStub &clientStub, RemoteCallSemantics rcs, const std::string &subInterface, int fnId, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) const
        {
            bool oneway = (Oneway == rcs);
            ConnectionResetGuard connectionResetGuard(clientStub);
            try {
                while (true) {
                    RCF_VERIFY(clientStub.getTries() < 2, Exception(RcfError_RepeatedRetries));
                    bool retry = false;
                    RCF::IDL::InHeader(clientStub, oneway, subInterface, fnId);
                    RCF_TYPENAME RCF::IDL::InParameter< A1 >::type(a1, clientStub.mOut);
                    RCF_TYPENAME RCF::IDL::InParameter< A2 >::type(a2, clientStub.mOut);
                    RCF_TYPENAME RCF::IDL::InParameter< A3 >::type(a3, clientStub.mOut);
                    RCF_TYPENAME RCF::IDL::InParameter< A4 >::type(a4, clientStub.mOut);
                    RCF_TYPENAME RCF::IDL::InParameter< A5 >::type(a5, clientStub.mOut);
                    RCF_TYPENAME RCF::IDL::InReturnValue< R >::type ret(clientStub, clientStub.mIn, clientStub.mOut, oneway, retry);
                    RCF_TYPENAME RCF::IDL::OutParameter< A1 >::type(a1, clientStub.mIn, oneway, retry);
                    RCF_TYPENAME RCF::IDL::OutParameter< A2 >::type(a2, clientStub.mIn, oneway, retry);
                    RCF_TYPENAME RCF::IDL::OutParameter< A3 >::type(a3, clientStub.mIn, oneway, retry);
                    RCF_TYPENAME RCF::IDL::OutParameter< A4 >::type(a4, clientStub.mIn, oneway, retry);
                    RCF_TYPENAME RCF::IDL::OutParameter< A5 >::type(a5, clientStub.mIn, oneway, retry);
                    connectionResetGuard.dismiss();
                    clientStub.mIn.clearByteBuffer();
                    if (!retry) return ret.get();
                }
            } catch (const RCF::RemoteException &) { connectionResetGuard.dismiss(); throw; }
        }
    };

    template<typename R, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
    class ClientMarshal_R6
    {
    public:
        R operator()(ClientStub &clientStub, RemoteCallSemantics rcs, const std::string &subInterface, int fnId, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) const
        {
            bool oneway = (Oneway == rcs);
            ConnectionResetGuard connectionResetGuard(clientStub);
            try {
                while (true) {
                    RCF_VERIFY(clientStub.getTries() < 2, Exception(RcfError_RepeatedRetries));
                    bool retry = false;
                    RCF::IDL::InHeader(clientStub, oneway, subInterface, fnId);
                    RCF_TYPENAME RCF::IDL::InParameter< A1 >::type(a1, clientStub.mOut);
                    RCF_TYPENAME RCF::IDL::InParameter< A2 >::type(a2, clientStub.mOut);
                    RCF_TYPENAME RCF::IDL::InParameter< A3 >::type(a3, clientStub.mOut);
                    RCF_TYPENAME RCF::IDL::InParameter< A4 >::type(a4, clientStub.mOut);
                    RCF_TYPENAME RCF::IDL::InParameter< A5 >::type(a5, clientStub.mOut);
                    RCF_TYPENAME RCF::IDL::InParameter< A6 >::type(a6, clientStub.mOut);
                    RCF_TYPENAME RCF::IDL::InReturnValue< R >::type ret(clientStub, clientStub.mIn, clientStub.mOut, oneway, retry);
                    RCF_TYPENAME RCF::IDL::OutParameter< A1 >::type(a1, clientStub.mIn, oneway, retry);
                    RCF_TYPENAME RCF::IDL::OutParameter< A2 >::type(a2, clientStub.mIn, oneway, retry);
                    RCF_TYPENAME RCF::IDL::OutParameter< A3 >::type(a3, clientStub.mIn, oneway, retry);
                    RCF_TYPENAME RCF::IDL::OutParameter< A4 >::type(a4, clientStub.mIn, oneway, retry);
                    RCF_TYPENAME RCF::IDL::OutParameter< A5 >::type(a5, clientStub.mIn, oneway, retry);
                    RCF_TYPENAME RCF::IDL::OutParameter< A6 >::type(a6, clientStub.mIn, oneway, retry);
                    connectionResetGuard.dismiss();
                    clientStub.mIn.clearByteBuffer();
                    if (!retry) return ret.get();
                }
            } catch (const RCF::RemoteException &) { connectionResetGuard.dismiss(); throw; }
        }
    };

    template<typename R, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
    class ClientMarshal_R7
    {
    public:
        R operator()(ClientStub &clientStub, RemoteCallSemantics rcs, const std::string &subInterface, int fnId, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) const
        {
            bool oneway = (Oneway == rcs);
            ConnectionResetGuard connectionResetGuard(clientStub);
            try {
                while (true) {
                    RCF_VERIFY(clientStub.getTries() < 2, Exception(RcfError_RepeatedRetries));
                    bool retry = false;
                    RCF::IDL::InHeader(clientStub, oneway, subInterface, fnId);
                    RCF_TYPENAME RCF::IDL::InParameter< A1 >::type(a1, clientStub.mOut);
                    RCF_TYPENAME RCF::IDL::InParameter< A2 >::type(a2, clientStub.mOut);
                    RCF_TYPENAME RCF::IDL::InParameter< A3 >::type(a3, clientStub.mOut);
                    RCF_TYPENAME RCF::IDL::InParameter< A4 >::type(a4, clientStub.mOut);
                    RCF_TYPENAME RCF::IDL::InParameter< A5 >::type(a5, clientStub.mOut);
                    RCF_TYPENAME RCF::IDL::InParameter< A6 >::type(a6, clientStub.mOut);
                    RCF_TYPENAME RCF::IDL::InParameter< A7 >::type(a7, clientStub.mOut);
                    RCF_TYPENAME RCF::IDL::InReturnValue< R >::type ret(clientStub, clientStub.mIn, clientStub.mOut, oneway, retry);
                    RCF_TYPENAME RCF::IDL::OutParameter< A1 >::type(a1, clientStub.mIn, oneway, retry);
                    RCF_TYPENAME RCF::IDL::OutParameter< A2 >::type(a2, clientStub.mIn, oneway, retry);
                    RCF_TYPENAME RCF::IDL::OutParameter< A3 >::type(a3, clientStub.mIn, oneway, retry);
                    RCF_TYPENAME RCF::IDL::OutParameter< A4 >::type(a4, clientStub.mIn, oneway, retry);
                    RCF_TYPENAME RCF::IDL::OutParameter< A5 >::type(a5, clientStub.mIn, oneway, retry);
                    RCF_TYPENAME RCF::IDL::OutParameter< A6 >::type(a6, clientStub.mIn, oneway, retry);
                    RCF_TYPENAME RCF::IDL::OutParameter< A7 >::type(a7, clientStub.mIn, oneway, retry);
                    connectionResetGuard.dismiss();
                    clientStub.mIn.clearByteBuffer();
                    if (!retry) return ret.get();
                }
            } catch (const RCF::RemoteException &) { connectionResetGuard.dismiss(); throw; }
        }
    };

    template<typename R, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
    class ClientMarshal_R8
    {
    public:
        R operator()(ClientStub &clientStub, RemoteCallSemantics rcs, const std::string &subInterface, int fnId, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) const
        {
            bool oneway = (Oneway == rcs);
            ConnectionResetGuard connectionResetGuard(clientStub);
            try {
                while (true) {
                    RCF_VERIFY(clientStub.getTries() < 2, Exception(RcfError_RepeatedRetries));
                    bool retry = false;
                    RCF::IDL::InHeader(clientStub, oneway, subInterface, fnId);
                    RCF_TYPENAME RCF::IDL::InParameter< A1 >::type(a1, clientStub.mOut);
                    RCF_TYPENAME RCF::IDL::InParameter< A2 >::type(a2, clientStub.mOut);
                    RCF_TYPENAME RCF::IDL::InParameter< A3 >::type(a3, clientStub.mOut);
                    RCF_TYPENAME RCF::IDL::InParameter< A4 >::type(a4, clientStub.mOut);
                    RCF_TYPENAME RCF::IDL::InParameter< A5 >::type(a5, clientStub.mOut);
                    RCF_TYPENAME RCF::IDL::InParameter< A6 >::type(a6, clientStub.mOut);
                    RCF_TYPENAME RCF::IDL::InParameter< A7 >::type(a7, clientStub.mOut);
                    RCF_TYPENAME RCF::IDL::InParameter< A8 >::type(a8, clientStub.mOut);
                    RCF_TYPENAME RCF::IDL::InReturnValue< R >::type ret(clientStub, clientStub.mIn, clientStub.mOut, oneway, retry);
                    RCF_TYPENAME RCF::IDL::OutParameter< A1 >::type(a1, clientStub.mIn, oneway, retry);
                    RCF_TYPENAME RCF::IDL::OutParameter< A2 >::type(a2, clientStub.mIn, oneway, retry);
                    RCF_TYPENAME RCF::IDL::OutParameter< A3 >::type(a3, clientStub.mIn, oneway, retry);
                    RCF_TYPENAME RCF::IDL::OutParameter< A4 >::type(a4, clientStub.mIn, oneway, retry);
                    RCF_TYPENAME RCF::IDL::OutParameter< A5 >::type(a5, clientStub.mIn, oneway, retry);
                    RCF_TYPENAME RCF::IDL::OutParameter< A6 >::type(a6, clientStub.mIn, oneway, retry);
                    RCF_TYPENAME RCF::IDL::OutParameter< A7 >::type(a7, clientStub.mIn, oneway, retry);
                    RCF_TYPENAME RCF::IDL::OutParameter< A8 >::type(a8, clientStub.mIn, oneway, retry);
                    connectionResetGuard.dismiss();
                    clientStub.mIn.clearByteBuffer();
                    return ret.get();
                }
            } catch (const RCF::RemoteException &) { connectionResetGuard.dismiss(); throw; }
        }
    };

#undef RCF_TYPENAME

} // namespace RCF

#include <RCF/Marshal.inl>

#endif // ! INCLUDE_RCF_MARSHAL_HPP
