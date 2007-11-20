
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#include <RCF/AsyncFilter.hpp>

#include <RCF/ByteBuffer.hpp>
#include <RCF/InitDeinit.hpp>
#include <RCF/Tools.hpp>

namespace RCF {

    // IdentityFilter

    void IdentityFilter::reset()
    {}

    const FilterDescription & IdentityFilter::getFilterDescription() const
    {
        return sGetFilterDescription();
    }

    const FilterDescription & IdentityFilter::sGetFilterDescription()
    {
        return *spFilterDescription;
    }

    const FilterDescription *IdentityFilter::spFilterDescription = NULL;

    RCF_ON_INIT_DEINIT(
        IdentityFilter::spFilterDescription = new FilterDescription("identity filter", RCF_FILTER_IDENTITY, true),
        delete IdentityFilter::spFilterDescription; IdentityFilter::spFilterDescription = NULL )

    // IdentityFilterFactory

    FilterPtr IdentityFilterFactory::createFilter()
    {
        return FilterPtr( new IdentityFilter() );
    }

    const FilterDescription & IdentityFilterFactory::getFilterDescription()
    {
        return IdentityFilter::sGetFilterDescription();
    }

    // XorFilter

    const FilterDescription & XorFilter::getFilterDescription() const
    {
        return sGetFilterDescription();
    }

    const FilterDescription & XorFilter::sGetFilterDescription()
    {
        return *spFilterDescription;
    }

    const FilterDescription *XorFilter::spFilterDescription = NULL;

    RCF_ON_INIT_DEINIT(
        XorFilter::spFilterDescription = new FilterDescription("Xor filter", RCF_FILTER_XOR, true); ,
        delete XorFilter::spFilterDescription; XorFilter::spFilterDescription = NULL;)


    // XorFilterFactory

    FilterPtr XorFilterFactory::createFilter()
    {
        return FilterPtr( new XorFilter() );
    }

    const FilterDescription & XorFilterFactory::getFilterDescription()
    {
        return XorFilter::sGetFilterDescription();
    }

    class ReadProxy : public IdentityFilter
    {
    public:
        ReadProxy() :
            mInByteBufferPos(RCF_DEFAULT_INIT),
            mBytesTransferred(RCF_DEFAULT_INIT)
        {}

        std::size_t getOutBytesTransferred() const
        {
            return mBytesTransferred;
        }

        ByteBuffer getOutByteBuffer()
        {
            ByteBuffer byteBuffer = mOutByteBuffer;
            mOutByteBuffer = ByteBuffer();
            return byteBuffer;
        }

        void setInByteBuffer(const ByteBuffer &byteBuffer)
        {
            mInByteBuffer = byteBuffer;
        }

        void clear()
        {
            mInByteBuffer = ByteBuffer();
            mOutByteBuffer = ByteBuffer();
            mInByteBufferPos = 0;
            mBytesTransferred = 0;
        }

    private:

        void read(const ByteBuffer &byteBuffer, std::size_t bytesRequested)
        {
            RCF_ASSERT(byteBuffer.isEmpty())(byteBuffer.getLength());

            RCF_ASSERT(
                0 <= mInByteBufferPos && mInByteBufferPos < mInByteBuffer.getLength())
                (mInByteBufferPos)(mInByteBuffer.getLength());

            std::size_t bytesRemaining = mInByteBuffer.getLength() - mInByteBufferPos;
            std::size_t bytesToRead = RCF_MIN(bytesRemaining, bytesRequested);
            ByteBuffer myByteBuffer(mInByteBuffer, mInByteBufferPos, bytesToRead);
            mInByteBufferPos += bytesToRead;
            getPreFilter().onReadCompleted(myByteBuffer, 0);
        }

        void onReadCompleted(const ByteBuffer &byteBuffer, int error)
        {
            RCF_UNUSED_VARIABLE(error);
            mOutByteBuffer = byteBuffer;
            mBytesTransferred = byteBuffer.getLength();
        }

    private:

        std::size_t mInByteBufferPos;
        std::size_t mBytesTransferred;

        ByteBuffer mInByteBuffer;
        ByteBuffer mOutByteBuffer;
    };

    class WriteProxy : public IdentityFilter, boost::noncopyable
    {
    public:
        WriteProxy() :
            mBytesTransferred(RCF_DEFAULT_INIT),
            mError(RCF_DEFAULT_INIT),
            mTlcByteBuffers(),
            mByteBuffers(mTlcByteBuffers.get())
        {
        }

        const std::vector<ByteBuffer> &getByteBuffers() const
        {
            return mByteBuffers;
        }

        void clear()
        {
            mByteBuffers.resize(0);
            mBytesTransferred = 0;
            mError = 0;
        }

        std::size_t getBytesTransferred() const
        {
            return mBytesTransferred;
        }

        int getError() const
        {
            return mError;
        }

    private:

        void write(const std::vector<ByteBuffer> &byteBuffers)
        {
            std::copy(
                byteBuffers.begin(),
                byteBuffers.end(),
                std::back_inserter(mByteBuffers));

            std::size_t bytesTransferred = lengthByteBuffers(byteBuffers);
            getPreFilter().onWriteCompleted(bytesTransferred, 0);
        }

        void onWriteCompleted(std::size_t bytesTransferred, int error)
        {
            mBytesTransferred = bytesTransferred;
            mError = error;
        }

    private:
        std::size_t mBytesTransferred;
        int mError;

        RCF::ThreadLocalCached< std::vector<RCF::ByteBuffer> > mTlcByteBuffers;
        std::vector<RCF::ByteBuffer> &mByteBuffers;
    };

    //typedef boost::shared_ptr<WriteProxy> WriteProxyPtr;
    //typedef boost::shared_ptr<ReadProxy> ReadProxyPtr;

    bool filterData(
        const std::vector<ByteBuffer> &unfilteredData,
        std::vector<ByteBuffer> &filteredData,
        const std::vector<FilterPtr> &filters)
    {
        //int error                           = 0;
        std::size_t bytesTransferred        = 0;
        std::size_t bytesTransferredTotal   = 0;

        WriteProxy writeProxy;
        writeProxy.setPreFilter(*filters.back());
        filters.back()->setPostFilter(writeProxy);
        filters.front()->setPreFilter(writeProxy);

        std::size_t unfilteredDataLen = lengthByteBuffers(unfilteredData);
        while (bytesTransferredTotal < unfilteredDataLen)
        {

            ThreadLocalCached< std::vector<ByteBuffer> > tlcSlicedByteBuffers;
            std::vector<ByteBuffer> &slicedByteBuffers = tlcSlicedByteBuffers.get();
            sliceByteBuffers(slicedByteBuffers, unfilteredData, bytesTransferredTotal);
            filters.front()->write(slicedByteBuffers);

            // TODO: error handling
            bytesTransferred = writeProxy.getBytesTransferred();
            bytesTransferredTotal += bytesTransferred;
        }
        RCF_ASSERT(
            bytesTransferredTotal == unfilteredDataLen)
            (bytesTransferredTotal)(unfilteredDataLen);

        filteredData.resize(0);

        std::copy(
            writeProxy.getByteBuffers().begin(),
            writeProxy.getByteBuffers().end(),
            std::back_inserter(filteredData));

        return bytesTransferredTotal == unfilteredDataLen;
    }

    bool unfilterData(
        const ByteBuffer &filteredByteBuffer,
        std::vector<ByteBuffer> &unfilteredByteBuffers,
        std::size_t unfilteredDataLen,
        const std::vector<FilterPtr> &filters)
    {
        int error                           = 0;
        std::size_t bytesTransferred        = 0;
        std::size_t bytesTransferredTotal   = 0;

        ByteBuffer byteBuffer;
        unfilteredByteBuffers.resize(0);

        ReadProxy readProxy;
        readProxy.setInByteBuffer(filteredByteBuffer);
        readProxy.setPreFilter(*filters.back());
        filters.back()->setPostFilter(readProxy);
        filters.front()->setPreFilter(readProxy);

        while (!error && bytesTransferredTotal < unfilteredDataLen)
        {
            filters.front()->read(ByteBuffer(), unfilteredDataLen - bytesTransferredTotal);
            bytesTransferred = readProxy.getOutBytesTransferred();
            byteBuffer = readProxy.getOutByteBuffer();
            // TODO: error handling
            bytesTransferredTotal += (error) ? 0 : bytesTransferred;
            unfilteredByteBuffers.push_back(byteBuffer);
        }
        return bytesTransferredTotal == unfilteredDataLen;
    }

    bool unfilterData(
        const ByteBuffer &filteredByteBuffer,
        ByteBuffer &unfilteredByteBuffer,
        std::size_t unfilteredDataLen,
        const std::vector<FilterPtr> &filters)
    {
        ThreadLocalCached< std::vector<ByteBuffer> > tlcUnfilteredByteBuffers;
        std::vector<ByteBuffer> &unfilteredByteBuffers = tlcUnfilteredByteBuffers.get();

        bool ret = unfilterData(
            filteredByteBuffer,
            unfilteredByteBuffers,
            unfilteredDataLen,
            filters);

        if (unfilteredByteBuffers.empty())
        {
            unfilteredByteBuffer = ByteBuffer();
        }
        else if (unfilteredByteBuffers.size() == 1)
        {
            unfilteredByteBuffer = unfilteredByteBuffers[0];
        }
        else
        {
            // TODO: maybe check for adjacent buffers, in which case one should not need to make a copy
            copyByteBuffers(unfilteredByteBuffers, unfilteredByteBuffer);
        }
        return ret;
    }

    void connectFilters(const std::vector<FilterPtr> &filters)
    {
        for (std::size_t i=0; i<filters.size(); ++i)
        {
            if (i>0)
            {
                filters[i]->setPreFilter(*filters[i-1]);
            }
            if (i<filters.size()-1)
            {
                filters[i]->setPostFilter(*filters[i+1]);
            }
        }
    }

} // namespace RCF
