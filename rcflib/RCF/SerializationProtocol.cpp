
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#include <RCF/SerializationProtocol.hpp>

namespace RCF {

    bool isSerializationProtocolSupported(int protocol)
    {
        switch (protocol)
        {
        case 1: return Protocol< boost::mpl::int_<1> >::getName() != "";
        case 2: return Protocol< boost::mpl::int_<2> >::getName() != "";
        case 3: return Protocol< boost::mpl::int_<3> >::getName() != "";
        case 4: return Protocol< boost::mpl::int_<4> >::getName() != "";
        case 5: return Protocol< boost::mpl::int_<5> >::getName() != "";
        case 6: return Protocol< boost::mpl::int_<6> >::getName() != "";
        case 7: return Protocol< boost::mpl::int_<7> >::getName() != "";
        case 8: return Protocol< boost::mpl::int_<8> >::getName() != "";
        case 9: return Protocol< boost::mpl::int_<9> >::getName() != "";
        case 10: return Protocol< boost::mpl::int_<10> >::getName() != "";
        default: return false;
        }
    }

    std::string getSerializationProtocolName(int protocol)
    {
        switch (protocol)
        {
        case 1: return Protocol< boost::mpl::int_<1> >::getName();
        case 2: return Protocol< boost::mpl::int_<2> >::getName();
        case 3: return Protocol< boost::mpl::int_<3> >::getName();
        case 4: return Protocol< boost::mpl::int_<4> >::getName();
        case 5: return Protocol< boost::mpl::int_<5> >::getName();
        case 6: return Protocol< boost::mpl::int_<6> >::getName();
        case 7: return Protocol< boost::mpl::int_<7> >::getName();
        case 8: return Protocol< boost::mpl::int_<8> >::getName();
        case 9: return Protocol< boost::mpl::int_<9> >::getName();
        case 10: return Protocol< boost::mpl::int_<10> >::getName();
        default: return "";
        }
    }

    SerializationProtocolIn::SerializationProtocolIn() :
        mProtocol(DefaultSerializationProtocol),
        mIsPtr(RCF_DEFAULT_INIT),
        mIstrVec(sizeof(std::istrstream))
    {
    }

    SerializationProtocolIn::~SerializationProtocolIn()
    {
        RCF_DTOR_BEGIN
            if (mIsPtr)
            {
                // can't delete the stream if an archive is still bound to it
                unbindProtocol();

                // need to be elaborate here, for borland compiler
                typedef std::istrstream Is;
                Is &is = *mIsPtr;
                is.~Is();
                mIsPtr = NULL;
            }
        RCF_DTOR_END
    }

    void SerializationProtocolIn::setSerializationProtocol(int protocol)
    {
        mProtocol = protocol;
    }

    int SerializationProtocolIn::getSerializationProtocol() const
    {
        return mProtocol;
    }

    static const char chZero = 0;

    void SerializationProtocolIn::reset(const ByteBuffer &data, int protocol)
    {
        unbindProtocol();

        mByteBuffer = data;

        // create a new istrstream, without allocating any memory from the heap
        if (mIsPtr)
        {
            //mIsPtr->~istrstream();

            // need to be elaborate here, for borland compiler
            typedef std::istrstream Is;
            Is &is = *mIsPtr;
            is.~Is();
            mIsPtr = NULL;
        }

        if (mByteBuffer)
        {
            new (&mIstrVec[0]) std::istrstream(
                (char*) mByteBuffer.getPtr() ,
                static_cast<int>(mByteBuffer.getLength()));
        }
        else
        {
            new (&mIstrVec[0]) std::istrstream(
                &chZero);
        }

        mIsPtr = reinterpret_cast<std::istrstream *>(&mIstrVec[0]);

        setSerializationProtocol(protocol);
        bindProtocol();

        mPointerContext.clear();
    }

    void SerializationProtocolIn::bindProtocol()
    {
        RCF_ASSERT(mIsPtr);
        switch (mProtocol)
        {
        case 1: mInProtocol1.bind(*mIsPtr); break;
        case 2: mInProtocol2.bind(*mIsPtr); break;
        case 3: mInProtocol3.bind(*mIsPtr); break;
        case 4: mInProtocol4.bind(*mIsPtr); break;
        default: RCF_ASSERT(0)(mProtocol);
        }
    }

    void SerializationProtocolIn::unbindProtocol()
    {
        switch (mProtocol)
        {
        case 1: mInProtocol1.unbind(); break;
        case 2: mInProtocol2.unbind(); break;
        case 3: mInProtocol3.unbind(); break;
        case 4: mInProtocol4.unbind(); break;
        default: RCF_ASSERT(0)(mProtocol);
        }
    }

    void SerializationProtocolIn::extractSlice(
        ByteBuffer &byteBuffer,
        std::size_t len)
    {
        if (len == 0)
        {
            byteBuffer.clear();
        }
        else
        {
            std::size_t pos = mIsPtr->tellg();

            std::size_t newPos = mIsPtr->rdbuf()->pubseekoff(
                len,
                std::ios::cur,
                std::ios::in);


            RCF_VERIFY(
                newPos != -1,
                RCF::SerializationException(
                RcfError_Deserialization,
                "extractSlice()"))
                (pos)(len)(mByteBuffer.getLength() );

            byteBuffer = ByteBuffer(mByteBuffer, pos, len);
        }
    }

    void SerializationProtocolIn::clearByteBuffer()
    {
        mByteBuffer = ByteBuffer();
    }

    std::size_t SerializationProtocolIn::getArchiveLength()
    {
        return mByteBuffer.getLength();
    }

    std::size_t SerializationProtocolIn::getRemainingArchiveLength()
    {
        std::size_t pos = mIsPtr->tellg();
        std::size_t len = mByteBuffer.getLength();
        RCF_ASSERT(pos <= len);
        return len - pos;
    }

    void SerializationProtocolIn::clear()
    {
        unbindProtocol();
        mPointerContext.clear();
    }

    SerializationProtocolOut::SerializationProtocolOut() :
        mProtocol(DefaultSerializationProtocol),
        mMargin(RCF_DEFAULT_INIT)
    {}

    void SerializationProtocolOut::setSerializationProtocol(int protocol)
    {
        mProtocol = protocol;
    }

    int SerializationProtocolOut::getSerializationProtocol() const
    {
        return mProtocol;
    }

#if defined(_MSC_VER) && _MSC_VER <= 1200
#define for if (0) {} else for
#endif

    void SerializationProtocolOut::reset(
        int protocol,
        std::size_t margin,
        ByteBuffer byteBuffer)
    {

        unbindProtocol();
        if (!mOsPtr)
        {
            mOsPtr.reset( new std::ostrstream() );
        }
        else
        {
            // rewind the ostrstream and reuse it
            mOsPtr->clear(); // freezing may have set error state
            mOsPtr->rdbuf()->freeze(0);
            mOsPtr->rdbuf()->pubseekoff(0, std::ios::beg, std::ios::out);
        }

        // set up margin
        mMargin = margin;
        for (std::size_t i=0; i<mMargin; ++i)
        {
            (*mOsPtr) << '%';
        }

        // copy byte buffer contents
        for (std::size_t i=0; i<byteBuffer.getLength(); ++i)
        {
            (*mOsPtr) << byteBuffer.getPtr()[i];
        }

        if (protocol != 0)
        {
            setSerializationProtocol(protocol);
        }
        bindProtocol();
    }

#if defined(_MSC_VER) && _MSC_VER <= 1200
#undef for
#endif

    void SerializationProtocolOut::bindProtocol()
    {
        switch (mProtocol)
        {
        case 1: mOutProtocol1.bind(*mOsPtr); break;
        case 2: mOutProtocol2.bind(*mOsPtr); break;
        case 3: mOutProtocol3.bind(*mOsPtr); break;
        case 4: mOutProtocol4.bind(*mOsPtr); break;
        default: RCF_ASSERT(0)(mProtocol);
        }
    }

    void SerializationProtocolOut::unbindProtocol()
    {
        switch (mProtocol)
        {
        case 1: mOutProtocol1.unbind(); break;
        case 2: mOutProtocol2.unbind(); break;
        case 3: mOutProtocol3.unbind(); break;
        case 4: mOutProtocol4.unbind(); break;
        default: RCF_ASSERT(0)(mProtocol);
        }
    }

    void SerializationProtocolOut::insert(const ByteBuffer &byteBuffer)
    {
        std::size_t streamPos = mOsPtr->pcount();
        mByteBuffers.push_back( std::make_pair(streamPos, byteBuffer));
    }

    void SerializationProtocolOut::extractByteBuffers()
    {
        mByteBuffers.resize(0);
    }

    void SerializationProtocolOut::extractByteBuffers(
        std::vector<ByteBuffer> &byteBuffers)
    {
        byteBuffers.resize(0);
        mOsPtr->freeze();
        char *pch = mOsPtr->str();
        std::size_t offset = 0;
        std::size_t offsetPrev = 0;
        std::size_t len = mOsPtr->pcount();
        for (std::size_t i=0; i<mByteBuffers.size(); ++i)
        {
            offset = mByteBuffers[i].first;

            RCF_ASSERT(
                offsetPrev <= offset && offset <= len)
                (offsetPrev)(offset)(len);

            if (offset-offsetPrev > 0)
            {
                if (offsetPrev == 0)
                {
                    byteBuffers.push_back(
                        ByteBuffer(
                        pch+offsetPrev+mMargin,
                        offset-offsetPrev-mMargin,
                        mMargin,
                        mOsPtr));
                }
                else
                {
                    byteBuffers.push_back(
                        ByteBuffer(
                        pch+offsetPrev,
                        offset-offsetPrev,
                        mOsPtr));
                }

            }
            byteBuffers.push_back(mByteBuffers[i].second);
            offsetPrev = offset;
        }
        RCF_ASSERT(
            offsetPrev <= offset && offset <= len)
            (offsetPrev)(offset)(len);

        if (len-offset > 0)
        {
            if (offset == 0)
            {
                byteBuffers.push_back(
                    ByteBuffer(
                    pch+offset+mMargin,
                    len-offset-mMargin,
                    mMargin,
                    mOsPtr));
            }
            else
            {
                byteBuffers.push_back(
                    ByteBuffer(
                    pch+offset,
                    len-offset,
                    mOsPtr));
            }

        }
        mOsPtr->freeze(false);
        mByteBuffers.resize(0);
    }

    void SerializationProtocolOut::clear()
    {
        unbindProtocol();
    }

} // namespace RCF
