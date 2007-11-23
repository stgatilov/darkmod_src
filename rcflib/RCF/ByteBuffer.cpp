
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#include <RCF/ByteBuffer.hpp>

#include <RCF/InitDeinit.hpp>
#include <RCF/ThreadLibrary.hpp>

namespace RCF {


    ByteBuffer::ByteBuffer() :
        mPv(RCF_DEFAULT_INIT),
        mPvlen(RCF_DEFAULT_INIT),
        mLeftMargin(RCF_DEFAULT_INIT),
        mReadOnly(RCF_DEFAULT_INIT)
    {}

    ByteBuffer::ByteBuffer(std::size_t pvlen) :
    	mSpvc(new std::vector<char>(pvlen)),
        mPv( mSpvc->empty() ? NULL : &mSpvc->front()),
        mPvlen(pvlen),
        mLeftMargin(RCF_DEFAULT_INIT),
        mReadOnly(RCF_DEFAULT_INIT)
    {}

    ByteBuffer::ByteBuffer(
        const boost::shared_ptr<std::vector<char> > &spvc,
        bool readOnly) :
        	mSpvc(spvc),
        	mSpos(),
            mPv( spvc->empty() ? NULL : const_cast<char*>(&spvc->front())),
            mPvlen(spvc->size()),
            mLeftMargin(RCF_DEFAULT_INIT),
            mReadOnly(readOnly)
    {}

    ByteBuffer::ByteBuffer(
        char *pv,
        std::size_t pvlen,
        bool readOnly) :
            mPv(pv),
            mPvlen(pvlen),
            mLeftMargin(RCF_DEFAULT_INIT),
            mReadOnly(readOnly)
    {}

    ByteBuffer::ByteBuffer(
        char *pv,
        std::size_t pvlen,
        std::size_t leftMargin,
        bool readOnly) :
            mPv(pv),
            mPvlen(pvlen),
            mLeftMargin(leftMargin),
            mReadOnly(readOnly)
    {}

    ByteBuffer::ByteBuffer(
        char *pv,
        std::size_t pvlen,
        const boost::shared_ptr<std::ostrstream> &spos,
        bool readOnly) :
        	mSpos(spos),
            mPv(pv),
            mPvlen(pvlen),
            mLeftMargin(RCF_DEFAULT_INIT),
            mReadOnly(readOnly)
    {}

    ByteBuffer::ByteBuffer(
        char *pv,
        std::size_t pvlen,
        std::size_t leftMargin,
        const boost::shared_ptr<std::ostrstream> &spos,
        bool readOnly) :
        	 mSpos(spos),
            mPv(pv),
            mPvlen(pvlen),
            mLeftMargin(leftMargin),
            mReadOnly(readOnly)
    {}

    ByteBuffer::ByteBuffer(
        char *pv,
        std::size_t pvlen,
        const boost::shared_ptr<std::vector<char> > &spvc,
        bool readOnly) :
        	mSpvc(spvc),
            mPv(pv),
            mPvlen(pvlen),
            mLeftMargin(RCF_DEFAULT_INIT),
            mReadOnly(readOnly)           
    {}

    ByteBuffer::ByteBuffer(
        char *pv,
        std::size_t pvlen,
        std::size_t leftMargin,
        const boost::shared_ptr<std::vector<char> > &spvc,
        bool readOnly) :
        	 mSpvc(spvc),
            mPv(pv),
            mPvlen(pvlen),
            mLeftMargin(leftMargin),
            mReadOnly(readOnly)
    {}

    ByteBuffer::ByteBuffer(
        const ByteBuffer &byteBuffer,
        std::size_t offset,
        std::size_t len) :
        	mSpvc(byteBuffer.mSpvc),
        	mSpos(byteBuffer.mSpos),
            mPv(byteBuffer.mPv + offset),
            mPvlen( len == -1 ? byteBuffer.mPvlen-offset : len),
            mLeftMargin( offset ? 0 : byteBuffer.mLeftMargin),
            mReadOnly(byteBuffer.mReadOnly)
    {
        RCF_ASSERT(offset <= byteBuffer.mPvlen)(offset)(byteBuffer.mPvlen);

        RCF_ASSERT(
            len == -1 || offset+len <= byteBuffer.mPvlen)
            (offset)(len)(byteBuffer.mPvlen);
    }

    char *ByteBuffer::getPtr() const
    {
        return mPv;
    }

    std::size_t ByteBuffer::getLength() const
    {
        return mPvlen;
    }

    std::size_t ByteBuffer::getLeftMargin() const
    {
        return mLeftMargin;
    }

    bool ByteBuffer::getReadOnly() const
    {
        return mReadOnly;
    }

    bool ByteBuffer::isEmpty() const
    {
        return getLength() == 0;
    }

    void ByteBuffer::expandIntoLeftMargin(std::size_t len)
    {
        RCF_ASSERT(len <= mLeftMargin)(len)(mLeftMargin);
        mPv -= len;
        mPvlen += len;
        mLeftMargin -= len;
    }

    std::string ByteBuffer::string() const
    {
        return std::string(getPtr(), getLength());
    }

    ByteBuffer ByteBuffer::release()
    {
        ByteBuffer byteBuffer(*this);
        *this = ByteBuffer();
        return byteBuffer;
    }

    void ByteBuffer::clear()
    {
        *this = ByteBuffer();
    }

    std::size_t lengthByteBuffers(const std::vector<ByteBuffer> &byteBuffers)
    {
        std::size_t length = 0;
        for (std::size_t i=0; i<byteBuffers.size(); ++i)
        {
            length += byteBuffers[i].getLength() ;
        }
        return length;
    }

    class ByteBufferPushBackFunctor
    {
    public:
        ByteBufferPushBackFunctor(std::vector<ByteBuffer> &byteBuffers) :
            mpByteBuffers(&byteBuffers)
        {}

        void operator()(const ByteBuffer &byteBuffer) const
        {
            mpByteBuffers->push_back(byteBuffer);
        }

    private:
        std::vector<ByteBuffer> *mpByteBuffers;
    };

    void sliceByteBuffers(
        std::vector<ByteBuffer> &slicedBuffers,
        const std::vector<ByteBuffer> &byteBuffers,
        std::size_t offset,
        std::size_t length)
    {
        slicedBuffers.resize(0);

        forEachByteBuffer(
            ByteBufferPushBackFunctor(slicedBuffers),
            byteBuffers, offset, length);
    }   

    void copyByteBuffers(
        const std::vector<ByteBuffer> &byteBuffers,
        char *pch)
    {
        for (std::size_t i=0; i<byteBuffers.size(); ++i)
        {
            memcpy(pch, byteBuffers[i].getPtr() , byteBuffers[i].getLength() );
            pch += byteBuffers[i].getLength() ;
        }
    }

    void copyByteBuffers(
        const std::vector<ByteBuffer> &byteBuffers,
        ByteBuffer &byteBuffer)
    {
        boost::shared_ptr<std::vector<char> > vecPtr(
            new std::vector<char>(lengthByteBuffers(byteBuffers)));

        copyByteBuffers(byteBuffers, &(*vecPtr)[0]);
        byteBuffer = ByteBuffer(
            &(*vecPtr)[0],
            (*vecPtr).size(),
            vecPtr);
    }

} // namespace RCF
