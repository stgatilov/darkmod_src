
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#include <RCF/ZlibCompressionFilter.hpp>

#include "zlib.h"

#include <RCF/InitDeinit.hpp>
#include <RCF/Tools.hpp>

namespace RCF {

    class ZlibCompressionReadFilter
    {
    public:
        ZlibCompressionReadFilter(
            ZlibCompressionFilter &filter,
            int bufferSize);

        ~ZlibCompressionReadFilter();
        void reset();
        void read(const ByteBuffer &byteBuffer, std::size_t bytesRequested);
        void onReadCompleted(const ByteBuffer &byteBuffer, int error);

    private:
        void resetDecompressionState();
        bool decompress();

        ZlibCompressionFilter &     filter;
        z_stream                    d_stream_;
        std::size_t                 bytesRequested;
        ByteBuffer                  preBuffer;
        ByteBuffer                  postBuffer;
        int                         zerr_;
        bool                        decompressionStateInited;

        ByteBuffer                  origBuffer;
        boost::shared_ptr<std::vector<char> > mVecPtr;
    };

    class ZlibCompressionWriteFilter
    {
    public:
        ZlibCompressionWriteFilter(
            ZlibCompressionFilter &filter,
            int bufferSize,
            bool stateful);

        ~ZlibCompressionWriteFilter();
        void reset();
        void write(const std::vector<ByteBuffer> &byteBuffers);
        void onWriteCompleted(std::size_t bytesTransferred, int error);

    private:
        void resetCompressionState();
        void compress();

        ZlibCompressionFilter &     filter;
        z_stream                    c_stream_;
        std::size_t                 totalBytesIn;
        std::size_t                 totalBytesOut;
        int                         zerr_;
        bool                        compressionStateInited;
        const bool                  stateful;

        std::vector<ByteBuffer>     postBuffers;
        std::vector<ByteBuffer>     preBuffers;

        boost::shared_ptr<std::vector<char> > mVecPtr;
    };

    ZlibCompressionReadFilter::ZlibCompressionReadFilter(
        ZlibCompressionFilter &filter,
        int bufferSize) :
            filter(filter),
            d_stream_(),
            bytesRequested(RCF_DEFAULT_INIT),
            zerr_(Z_OK),
            decompressionStateInited(RCF_DEFAULT_INIT)
    {

        memset(&d_stream_, 0, sizeof(d_stream_));

        // TODO: buffer size
        RCF_UNUSED_VARIABLE(bufferSize);
        resetDecompressionState();
    }

    ZlibCompressionReadFilter::~ZlibCompressionReadFilter()
    {
        RCF_DTOR_BEGIN
            if (decompressionStateInited)
            {
                zerr_ = inflateEnd(&d_stream_);
                RCF_VERIFY(
                    zerr_ == Z_OK,
                    FilterException(
                        RcfError_Zlib, zerr_, RcfSubsystem_Zlib,
                        "inflateEnd() failed"))(zerr_);
                decompressionStateInited = false;
            }
        RCF_DTOR_END
    }

    void ZlibCompressionReadFilter::reset()
    {
        resetDecompressionState();
    }

    void ZlibCompressionReadFilter::resetDecompressionState()
    {
        if (decompressionStateInited)
        {
            zerr_ = inflateEnd(&d_stream_);

            RCF_VERIFY(
                zerr_ == Z_OK,
                FilterException(
                    RcfError_Zlib,
                    zerr_,
                    RcfSubsystem_Zlib,
                    "inflateEnd() failed"))
                (zerr_);

            decompressionStateInited = false;
        }
        d_stream_.zalloc = NULL;
        d_stream_.zfree = NULL;
        d_stream_.opaque = NULL;
        zerr_ = inflateInit(&d_stream_);
        RCF_VERIFY(
            zerr_ == Z_OK,
            FilterException(
                RcfError_Zlib, zerr_, RcfSubsystem_Zlib,
                "inflateInit() failed"))(zerr_);
        decompressionStateInited = true;
    }

    // TODO: do the right thing with the byteBuffer parameter
    void ZlibCompressionReadFilter::read(
        const ByteBuffer &byteBuffer,
        std::size_t bytesRequested)
    {
        origBuffer = ByteBuffer(
            byteBuffer,
            0,
            RCF_MIN(byteBuffer.getLength(), bytesRequested));

        if (postBuffer.getLength() > 0)
        {
            onReadCompleted(postBuffer, 0);
        }
        else
        {
            postBuffer = ByteBuffer();

            this->bytesRequested = bytesRequested;
            // TODO:  a dedicated buffer for reading, so we can read a lot at a time
            // TODO: buffer size
            std::size_t bytesToRead = 4096;
            filter.getPostFilter().read(ByteBuffer(), bytesToRead);
        }
    }

    void ZlibCompressionReadFilter::onReadCompleted(
        const ByteBuffer &byteBuffer,
        int error)
    {
        RCF_VERIFY(
            error == 0,
            FilterException(RcfError_Filter))
            (error)(byteBuffer.getLength());

        if (origBuffer.getLength() > 0)
        {
            preBuffer = origBuffer;
        }
        else
        {
            if (mVecPtr.get() == NULL || !mVecPtr.unique())
            {
                mVecPtr.reset( new std::vector<char>());
            }
            mVecPtr->resize(bytesRequested);
            preBuffer = ByteBuffer(mVecPtr);
            origBuffer = preBuffer;
        }
        postBuffer = byteBuffer;
        if (decompress())
        {
            origBuffer.clear();
            filter.getPreFilter().onReadCompleted(preBuffer.release(), 0);
        }
        else
        {
            preBuffer.clear();
            read(origBuffer, bytesRequested);
        }
    }

    bool ZlibCompressionReadFilter::decompress()
    {
        d_stream_.next_in = (Bytef*) postBuffer.getPtr();
        d_stream_.avail_in = static_cast<uInt>(postBuffer.getLength());
        d_stream_.next_out = (Bytef*) preBuffer.getPtr();
        d_stream_.avail_out = static_cast<uInt>(preBuffer.getLength());
        zerr_ = inflate(&d_stream_, Z_SYNC_FLUSH);
        RCF_VERIFY(
            zerr_ == Z_OK || zerr_ == Z_STREAM_END || zerr_ == Z_BUF_ERROR,
            FilterException(
                RcfError_ZlibInflate, zerr_, RcfSubsystem_Zlib,
                "inflate() failed"))
            (zerr_)(preBuffer.getLength())(postBuffer.getLength());
        if (zerr_ == Z_STREAM_END)
        {
            resetDecompressionState();
        }

        preBuffer = ByteBuffer(
            preBuffer,
            0,
            preBuffer.getLength() - d_stream_.avail_out);

        postBuffer = ByteBuffer(
            postBuffer,
            postBuffer.getLength() - d_stream_.avail_in);

        if (postBuffer.getLength() == 0)
        {
            postBuffer.clear();
        }

        return preBuffer.getLength() > 0;
    }

    ZlibCompressionWriteFilter::ZlibCompressionWriteFilter(
        ZlibCompressionFilter &filter,
        int bufferSize,
        bool stateful) :
            filter(filter),
            c_stream_(),
            totalBytesIn(RCF_DEFAULT_INIT),
            totalBytesOut(RCF_DEFAULT_INIT),
            zerr_(Z_OK),
            compressionStateInited(RCF_DEFAULT_INIT),
            stateful(stateful)
    {
        memset(&c_stream_, 0, sizeof(c_stream_));

        // TODO: buffer size
        RCF_UNUSED_VARIABLE(bufferSize);
    }

    ZlibCompressionWriteFilter::~ZlibCompressionWriteFilter()
    {
        RCF_DTOR_BEGIN
            if (compressionStateInited)
            {
                zerr_ = deflateEnd(&c_stream_);
                RCF_VERIFY(
                    zerr_ == Z_OK || zerr_ == Z_DATA_ERROR,
                    FilterException(
                        RcfError_Zlib, zerr_, RcfSubsystem_Zlib,
                        "deflateEnd() failed"))(zerr_)(stateful);
                compressionStateInited = false;
            }
        RCF_DTOR_END
    }

    void ZlibCompressionWriteFilter::reset()
    {
        resetCompressionState();
    }

    void ZlibCompressionWriteFilter::resetCompressionState()
    {
        if (compressionStateInited)
        {
            zerr_ = deflateEnd(&c_stream_);
            RCF_VERIFY(
                zerr_ == Z_OK || zerr_ == Z_DATA_ERROR,
                FilterException(
                    RcfError_Zlib, zerr_, RcfSubsystem_Zlib,
                    "deflateEnd() failed"))(zerr_);
            compressionStateInited = false;
        }
        c_stream_.zalloc = NULL;
        c_stream_.zfree = NULL;
        c_stream_.opaque = NULL;
        zerr_ = deflateInit(&c_stream_, Z_DEFAULT_COMPRESSION);
        RCF_VERIFY(
            zerr_ == Z_OK,
            FilterException(
                RcfError_Zlib, zerr_, RcfSubsystem_Zlib,
                "deflateInit() failed"))(zerr_);
        compressionStateInited = true;
    }

    void ZlibCompressionWriteFilter::write(
        const std::vector<ByteBuffer> &byteBuffers)
    {
        if (stateful == false || compressionStateInited == false)
        {
            resetCompressionState();
        }
        preBuffers.resize(0);

        std::copy(
            byteBuffers.begin(),
            byteBuffers.end(),
            std::back_inserter(preBuffers));

        compress();
        filter.getPostFilter().write(postBuffers);
    }

    void ZlibCompressionWriteFilter::onWriteCompleted(
        std::size_t bytesTransferred,
        int error)
    {
        // 1. if partial buffer was written -> write remaining part of buffer
        // 2. if whole buffer was written -> check if any more compression or writing is needed
        // 3. if no more compression or writing needed, notify previous filter of completion

        RCF_VERIFY(
            error == 0,
            FilterException(RcfError_Filter))
            (error)(bytesTransferred);

        RCF_ASSERT(
            bytesTransferred <= lengthByteBuffers(postBuffers))
            (bytesTransferred)(lengthByteBuffers(postBuffers));

        if (bytesTransferred < lengthByteBuffers(postBuffers))
        {
            // TODO: optimize
            std::vector<ByteBuffer> slicedBuffers;
            sliceByteBuffers(slicedBuffers, postBuffers, bytesTransferred);
            postBuffers = slicedBuffers;
            filter.getPostFilter().write(postBuffers);
        }
        else
        {
            preBuffers.resize(0);
            postBuffers.resize(0);
            filter.getPreFilter().onWriteCompleted(totalBytesIn, 0);
        }
    }

    void ZlibCompressionWriteFilter::compress()
    {
        postBuffers.resize(0);

        // TODO: buffer size
        std::size_t bufferSize = 2*(lengthByteBuffers(preBuffers)+7+7);
        std::size_t leftMargin = preBuffers.front().getLeftMargin();

        if (mVecPtr.get() == NULL || !mVecPtr.unique())
        {
            mVecPtr.reset( new std::vector<char>());
        }
        mVecPtr->resize(leftMargin + bufferSize);

        if (leftMargin > 0)
        {
            postBuffers.push_back(
                ByteBuffer(
                    &mVecPtr->front()+leftMargin,
                    mVecPtr->size()-leftMargin,
                    leftMargin,
                    mVecPtr));
        }
        else
        {
            postBuffers.push_back(
                ByteBuffer(
                    &mVecPtr->front(),
                    mVecPtr->size(),
                    mVecPtr));
        }

        ByteBuffer &outBuffer = postBuffers.back();
        std::size_t outPos = 0;
        std::size_t outRemaining = outBuffer.getLength() - outPos;
        totalBytesIn = 0;
        totalBytesOut = 0;

        for (std::size_t i=0; i<preBuffers.size(); ++i)
        {
            RCF_ASSERT(
                outPos < outBuffer.getLength())
                (outPos)(outBuffer.getLength());

            ByteBuffer &inBuffer = preBuffers[i];
            c_stream_.next_in = (Bytef*) inBuffer.getPtr();
            c_stream_.avail_in = static_cast<uInt>(inBuffer.getLength());
            c_stream_.next_out = (Bytef*) &outBuffer.getPtr()[outPos];
            c_stream_.avail_out = static_cast<uInt>(outRemaining);

            zerr_ = (i<preBuffers.size()-1) ?
                deflate(&c_stream_, Z_NO_FLUSH) :
                deflate(&c_stream_, Z_SYNC_FLUSH);

            RCF_VERIFY(
                zerr_ == Z_OK || zerr_ == Z_BUF_ERROR,
                FilterException(
                    0, zerr_, RcfSubsystem_Zlib,
                    "deflate() failed"))
                (zerr_)(inBuffer.getLength())(outBuffer.getLength());

            RCF_ASSERT(c_stream_.avail_out >= 0)(c_stream_.avail_out);

            std::size_t bytesIn = inBuffer.getLength() - c_stream_.avail_in;
            std::size_t bytesOut = outRemaining - c_stream_.avail_out;
            totalBytesIn += bytesIn;
            totalBytesOut += bytesOut;
            outPos += bytesOut;
            outRemaining -= bytesOut;
        }

        if (!stateful)
        {
            c_stream_.next_in = NULL;
            c_stream_.avail_in = 0;
            c_stream_.next_out = (Bytef*) &outBuffer.getPtr()[outPos];
            c_stream_.avail_out = static_cast<uInt>(outRemaining);

            zerr_ = deflate(&c_stream_, Z_FINISH);
            RCF_VERIFY(
                zerr_ == Z_BUF_ERROR || zerr_ == Z_STREAM_END,
                FilterException(
                    0, zerr_, RcfSubsystem_Zlib,
                    "deflate() failed"))
                    (zerr_)(outPos)(outRemaining);

            RCF_ASSERT(c_stream_.avail_out > 0)(c_stream_.avail_out);

            std::size_t bytesOut = outRemaining - c_stream_.avail_out;
            totalBytesOut += bytesOut;
            outPos += bytesOut;
            outRemaining -= bytesOut;
        }

        preBuffers.resize(0);
        outBuffer = ByteBuffer(outBuffer, 0, totalBytesOut);
    }

    const FilterDescription & ZlibStatelessCompressionFilter::sGetFilterDescription()
    {
        return *spFilterDescription;
    }

    const FilterDescription & ZlibStatefulCompressionFilter::sGetFilterDescription()
    {
        return *spFilterDescription;
    }

    const FilterDescription *ZlibStatelessCompressionFilter::spFilterDescription = NULL;
    const FilterDescription *ZlibStatefulCompressionFilter::spFilterDescription = NULL;

    static void initZlibCompressionFilterDescriptions()
    {
        RCF_ASSERT(!ZlibStatelessCompressionFilter::spFilterDescription);
        RCF_ASSERT(!ZlibStatefulCompressionFilter::spFilterDescription);

        ZlibStatelessCompressionFilter::spFilterDescription =
            new FilterDescription(
                "Zlib stateless compression filter",
                RCF_FILTER_ZLIB_COMPRESSION_STATELESS,
                false);

        ZlibStatefulCompressionFilter::spFilterDescription =
            new FilterDescription(
                "Zlib stateful compression filter",
                RCF_FILTER_ZLIB_COMPRESSION_STATEFUL,
                false);
    }

    static void deinitZlibCompressionFilterDescriptions()
    {
        delete ZlibStatelessCompressionFilter::spFilterDescription;
        ZlibStatelessCompressionFilter::spFilterDescription = NULL;

        delete ZlibStatefulCompressionFilter::spFilterDescription;
        ZlibStatefulCompressionFilter::spFilterDescription = NULL;
    }

    RCF_ON_INIT_DEINIT(
        initZlibCompressionFilterDescriptions(),
        deinitZlibCompressionFilterDescriptions())

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4355 )  // warning C4355: 'this' : used in base member initializer list
#endif

    ZlibCompressionFilter::ZlibCompressionFilter(int bufferSize, bool stateful) :
        mPreState(Ready),
        mReadFilter( new ZlibCompressionReadFilter(*this, bufferSize) ),
        mWriteFilter( new ZlibCompressionWriteFilter(*this, bufferSize, stateful) )
    {}

#ifdef _MSC_VER
#pragma warning( pop )
#endif

    const FilterDescription & ZlibStatelessCompressionFilter::getFilterDescription() const
    {
        return sGetFilterDescription();
    }

    const FilterDescription & ZlibStatefulCompressionFilter::getFilterDescription() const
    {
        return sGetFilterDescription();
    }

    void ZlibCompressionFilter::reset()
    {
        mPreState = Ready;
        mReadFilter->reset();
        mWriteFilter->reset();
    }

    void ZlibCompressionFilter::read(
        const ByteBuffer &byteBuffer,
        std::size_t bytesRequested)
    {
        mPreState = Reading;
        mReadFilter->read(byteBuffer, bytesRequested);
    }

    void ZlibCompressionFilter::write(
        const std::vector<ByteBuffer> &byteBuffers)
    {
        mPreState = Writing;
        mWriteFilter->write(byteBuffers);
    }

    void ZlibCompressionFilter::onReadCompleted(
        const ByteBuffer &byteBuffer,
        int error)
    {
        mReadFilter->onReadCompleted(byteBuffer, error);
    }

    void ZlibCompressionFilter::onWriteCompleted(
        std::size_t bytesTransferred,
        int error)
    {
        mWriteFilter->onWriteCompleted(bytesTransferred, error);
    }

    ZlibStatelessCompressionFilterFactory::ZlibStatelessCompressionFilterFactory(
        int bufferSize) :
            mBufferSize(bufferSize)
    {}

    FilterPtr ZlibStatelessCompressionFilterFactory::createFilter()
    {
        return FilterPtr( new ZlibStatelessCompressionFilter(mBufferSize));
    }

    const FilterDescription & ZlibStatelessCompressionFilterFactory::getFilterDescription()
    {
        return ZlibStatelessCompressionFilter::sGetFilterDescription();
    }

    ZlibStatefulCompressionFilterFactory::ZlibStatefulCompressionFilterFactory(
        int bufferSize) :
            mBufferSize(bufferSize)
    {}

    FilterPtr ZlibStatefulCompressionFilterFactory::createFilter()
    {
        return FilterPtr( new ZlibStatefulCompressionFilter(mBufferSize));
    }

    const FilterDescription & ZlibStatefulCompressionFilterFactory::getFilterDescription()
    {
        return ZlibStatefulCompressionFilter::sGetFilterDescription();
    }

} // namespace RCF
