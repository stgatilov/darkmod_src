
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_SERIALIZATIONPROTOCOL_HPP
#define INCLUDE_RCF_SERIALIZATIONPROTOCOL_HPP

#include <string>
#include <sstream> // TODO: remove
#include <strstream>

#include <RCF/ByteBuffer.hpp>

// Serialization protocols

#include <RCF/Protocol/Protocol.hpp>

// NB: Any code that uses the RCF_USE_SF_SERIALIZATION/RCF_USE_BOOST_SERIALIZATION macros,
// needs to include this file first.

#if !defined(RCF_USE_SF_SERIALIZATION) && !defined(RCF_USE_BOOST_SERIALIZATION)
#define RCF_USE_SF_SERIALIZATION
#endif

#if defined(RCF_USE_SF_SERIALIZATION)
#include <RCF/Protocol/SF.hpp>
#endif

#if defined(RCF_USE_BOOST_SERIALIZATION)
#include <RCF/Protocol/BoostSerialization.hpp>
#endif

namespace RCF {

    bool isSerializationProtocolSupported(int protocol);

    std::string getSerializationProtocolName(int protocol);

    class PointerContext
    {
    public:
        template<typename SmartPtr, typename T>
        SmartPtr get(SmartPtr *, T t)
        {
            // TODO: this is not kosher, need to use typeid().name()
            return static_cast<SmartPtr>(
                mPtrMap[&typeid(SmartPtr)][ static_cast<void *>(t) ] );
        }

        template<typename SmartPtr, typename T>
        void set(SmartPtr sp, T t)
        {
            mPtrMap[&typeid(SmartPtr)][ static_cast<void *>(t) ] =
                static_cast<void *>(sp);
        }

        void clear()
        {
            mPtrMap.clear();
        }

    private:
        std::map< const std::type_info *, std::map<void*, void*> > mPtrMap;
    };


    class Token;
    class MethodInvocationRequest;
    class MethodInvocationResponse;

    class SerializationProtocolIn
    {
    public:
        SerializationProtocolIn();
        ~SerializationProtocolIn();
        void setSerializationProtocol(int protocol);
        int getSerializationProtocol() const;
        void reset(const ByteBuffer &data, int protocol);
        void clearByteBuffer();
        void clear();

        template<typename T>
        void read(T &t)
        {
            try
            {
                switch (mProtocol)
                {
                case 1: mInProtocol1 >> t; break;
                case 2: mInProtocol2 >> t; break;
                case 3: mInProtocol3 >> t; break;
                case 4: mInProtocol4 >> t; break;
                default: RCF_ASSERT(0)(mProtocol);
                }
            }
            catch(const std::exception &e)
            {
                std::ostringstream os;
                os
                    << "deserialization error, object type: "
                    << typeid(t).name()
                    << ", error type: "
                    << typeid(e).name()
                    << ", error msg: "
                    << e.what();

                RCF_THROW(
                    RCF::SerializationException(
                        RcfError_Deserialization,
                        os.str()));
            }
        }

        void extractSlice(ByteBuffer &byteBuffer, std::size_t len);

        std::size_t getArchiveLength();
        std::size_t getRemainingArchiveLength();

        PointerContext mPointerContext;

    private:

        friend class ClientStub; // TODO
        friend class RcfSession; // TODO

        void bindProtocol();
        void unbindProtocol();

        int                                 mProtocol;
        ByteBuffer                          mByteBuffer;
        std::istrstream *                   mIsPtr;
        std::vector<char>                   mIstrVec;

        Protocol< boost::mpl::int_<1> >::In mInProtocol1;
        Protocol< boost::mpl::int_<2> >::In mInProtocol2;
        Protocol< boost::mpl::int_<3> >::In mInProtocol3;
        Protocol< boost::mpl::int_<4> >::In mInProtocol4;
    };

    class SerializationProtocolOut
    {
    public:
        SerializationProtocolOut();
        void setSerializationProtocol(int protocol);
        int getSerializationProtocol() const;
        void clear();
        void reset(
            int protocol,
            std::size_t margin = 32,
            ByteBuffer byteBuffer = ByteBuffer());

        template<typename T>
        void write(const T &t)
        {
            try
            {
                switch (mProtocol)
                {
                case 1: mOutProtocol1 << t; break;
                case 2: mOutProtocol2 << t; break;
                case 3: mOutProtocol3 << t; break;
                case 4: mOutProtocol4 << t; break;
                default: RCF_ASSERT(0)(mProtocol);
                }
            }
            catch(const std::exception &e)
            {
                std::ostringstream os;
                os
                    << "serialization error, object type: "
                    << typeid(t).name()
                    << ", error type: "
                    << typeid(e).name()
                    << ", error msg: "
                    << e.what();

                RCF_THROW(
                    RCF::SerializationException(
                        RcfError_Serialization,
                        os.str()));
            }
        }

        void insert(const ByteBuffer &byteBuffer);
        void extractByteBuffers();
        void extractByteBuffers(std::vector<ByteBuffer> &byteBuffers);

    private:

        friend class ClientStub; // TODO
        friend class RcfSession; // TODO

        void bindProtocol();
        void unbindProtocol();

        int                                                 mProtocol;
        std::size_t                                         mMargin;
        boost::shared_ptr<std::ostrstream>                  mOsPtr;
        std::vector<std::pair<std::size_t, ByteBuffer> >    mByteBuffers;

        // these need to be below mOsPtr, for good order of destruction
        Protocol< boost::mpl::int_<1> >::Out                mOutProtocol1;
        Protocol< boost::mpl::int_<2> >::Out                mOutProtocol2;
        Protocol< boost::mpl::int_<3> >::Out                mOutProtocol3;
        Protocol< boost::mpl::int_<4> >::Out                mOutProtocol4;
    };

    template<typename T>
    void serializeImpl(
        SerializationProtocolOut &out,
        const T &t,
        long int)
    {
        out.write(t);
    }

    template<typename T>
    void deserializeImpl(
        SerializationProtocolIn &in,
        T &t,
        long int)
    {
        in.read(t);
    }

    template<typename T>
    void serialize(
        SerializationProtocolOut &out,
        const T &t)
    {
        serializeImpl(out, t, 0);
    }

    template<typename T>
    void deserialize(
        SerializationProtocolIn &in,
        T &t)
    {
        deserializeImpl(in, t, 0);
    }


} // namespace RCF

#endif // ! INCLUDE_RCF_SERIALIZATIONPROTOCOL_HPP
