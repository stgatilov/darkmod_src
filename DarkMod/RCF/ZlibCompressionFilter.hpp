
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_ZLIBCOMPRESSIONFILTER_HPP
#define INCLUDE_RCF_ZLIBCOMPRESSIONFILTER_HPP

#include <memory>
#include <vector>

#include <boost/noncopyable.hpp>

#include <RCF/AsyncFilter.hpp>

namespace RCF {

    static const int ZlibDefaultBufferSize = 4096;

    class ZlibCompressionReadFilter;
    class ZlibCompressionWriteFilter;

    class ZlibCompressionFilter : public Filter, boost::noncopyable
    {
    public:
        ZlibCompressionFilter(int bufferSize, bool stateful);
       
    private:
        void reset();

        void read(const ByteBuffer &byteBuffer, std::size_t bytesRequested);
        void write(const std::vector<ByteBuffer> &byteBuffers);
        void onReadCompleted(const ByteBuffer &byteBuffer, int error);
        void onWriteCompleted(std::size_t bytesTransferred, int error);

        enum IoState
        {
            Ready,
            Reading,
            Writing
        };

        // input state
        IoState mPreState;

        friend class ZlibCompressionReadFilter;
        friend class ZlibCompressionWriteFilter;

        boost::shared_ptr<ZlibCompressionReadFilter> mReadFilter;
        boost::shared_ptr<ZlibCompressionWriteFilter> mWriteFilter;
    };

    /// Filter implementing a stateless compression protocol, through the Zlib library.
    class ZlibStatelessCompressionFilter : public ZlibCompressionFilter
    {
    public:
        /// Constructor.
        /// \param bufferSize Internal buffer size, limiting how much data can be compressed/decompressed in a single operation.
        ZlibStatelessCompressionFilter(
            int bufferSize = ZlibDefaultBufferSize) :
                ZlibCompressionFilter(bufferSize, false)
        {}

        static const FilterDescription & sGetFilterDescription();
        const FilterDescription & getFilterDescription() const;

        // TODO: should be private
        static const FilterDescription *spFilterDescription;
    };

    /// Filter implementing a stateful compression protocol, through the Zlib library.
    class ZlibStatefulCompressionFilter : public ZlibCompressionFilter
    {
    public:
        /// Constructor.
        /// \param bufferSize Internal buffer size, limiting how much data can be compressed/decompressed in a single operation.
        ZlibStatefulCompressionFilter(
            int bufferSize = ZlibDefaultBufferSize) :
                ZlibCompressionFilter(bufferSize, true)
        {}

        static const FilterDescription & sGetFilterDescription();
        const FilterDescription & getFilterDescription() const;

        // TODO: should be private
        static const FilterDescription *spFilterDescription;
    };
   
    /// Filter factory for ZlibStatelessCompressionFilter.
    class ZlibStatelessCompressionFilterFactory : public FilterFactory
    {
    public:
        /// Constructor.
        /// \param bufferSize Internal buffer size, limiting how much data can be compressed/decompressed in a single operation.
        ZlibStatelessCompressionFilterFactory(
            int bufferSize = ZlibDefaultBufferSize);

        FilterPtr createFilter();
        const FilterDescription & getFilterDescription();

    private:
        int mBufferSize;
    };

    /// Filter factory for ZlibStatefulCompressionFilter.
    class ZlibStatefulCompressionFilterFactory : public FilterFactory
    {
    public:
        /// Constructor.
        /// \param bufferSize Internal buffer size, limiting how much data can be compressed/decompressed in a single operation.
        ZlibStatefulCompressionFilterFactory(
            int bufferSize = ZlibDefaultBufferSize);

        FilterPtr createFilter();
        const FilterDescription & getFilterDescription();

    private:
        int mBufferSize;
    };

} // namespace RCF

#endif // ! INCLUDE_RCF_ZLIBCOMPRESSIONFILTER_HPP
