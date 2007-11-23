
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_FILTER_HPP
#define INCLUDE_RCF_FILTER_HPP

#include <string>
#include <vector>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#include <RCF/ByteBuffer.hpp>

namespace RCF {

    // collect these constants here so we can avoid collisions
    static const int RCF_FILTER_UNKNOWN                         = 0;
    static const int RCF_FILTER_IDENTITY                        = 1;
    static const int RCF_FILTER_OPENSSL_ENCRYPTION              = 2;
    static const int RCF_FILTER_ZLIB_COMPRESSION_STATELESS      = 3;
    static const int RCF_FILTER_ZLIB_COMPRESSION_STATEFUL       = 4;
    static const int RCF_FILTER_SSPI_NTLM                       = 5;
    static const int RCF_FILTER_SSPI_KERBEROS                   = 6;
    static const int RCF_FILTER_SSPI_NEGOTIATE                  = 7;

    static const int RCF_FILTER_XOR                             = 101;

    /// Runtime description of a filter.
    class FilterDescription
    {
    public:
        FilterDescription(
            const std::string &filterName,
            int filterId,
            bool removable) :
                mFilterName(filterName),
                mFilterId(filterId),
                mRemovable(removable)
        {}

        const std::string &getName() const
        {
            return mFilterName;
        }

        int getId() const
        {
            return mFilterId;
        }

        bool getRemovable() const
        {
            return mRemovable;
        }

    private:
        const std::string mFilterName;
        const int mFilterId;
        const bool mRemovable;
    };

    class Filter;

    typedef boost::shared_ptr<Filter> FilterPtr;

    class Filter
    {
    public:

        Filter() :
            mpPreFilter(RCF_DEFAULT_INIT),
            mpPostFilter(RCF_DEFAULT_INIT)
        {}

        virtual ~Filter()
        {}

        virtual void reset()
        {}

        // TODO: for generality, should take a vector<ByteBuffer> &
        // (applicable if message arrives fragmented through the transport)
        // BTW, bytesRequested is meaningful if byteBuffer is empty
        virtual void read(
            const ByteBuffer &byteBuffer,
            std::size_t bytesRequested) = 0;
       
        virtual void write(const std::vector<ByteBuffer> &byteBuffers) = 0;

        // TODO: remove error parameter, use exceptions instead
        virtual void onReadCompleted(
            const ByteBuffer &byteBuffer,
            int error) = 0;

        virtual void onWriteCompleted(
            std::size_t bytesTransferred,
            int error) = 0;

        virtual const FilterDescription &getFilterDescription() const = 0;

        void setPreFilter(Filter &preFilter)
        {
            mpPreFilter = &preFilter;
        }

        void setPostFilter(Filter &postFilter)
        {
            mpPostFilter = &postFilter;
        }

    protected:

        Filter &getPreFilter()
        {
            return *mpPreFilter;
        }

        Filter &getPostFilter()
        {
            return *mpPostFilter;
        }

        Filter *mpPreFilter;
        Filter *mpPostFilter;
    };

    class IdentityFilter : public Filter
    {
    public:

        void read(const ByteBuffer &byteBuffer, std::size_t bytesRequested)
        {
            getPostFilter().read(byteBuffer, bytesRequested);
        }

        void write(const std::vector<ByteBuffer> &byteBuffers)
        {
            getPostFilter().write(byteBuffers);
        }

        void onReadCompleted(const ByteBuffer &byteBuffer, int error)
        {
            getPreFilter().onReadCompleted(byteBuffer, error);
        }

        void onWriteCompleted(std::size_t bytesTransferred, int error)
        {
            getPreFilter().onWriteCompleted(bytesTransferred, error);
        }

        void reset();
       
        const FilterDescription &getFilterDescription() const;
        static const FilterDescription &sGetFilterDescription();
        static const FilterDescription *spFilterDescription;
    };

    inline void createNonReadOnlyByteBuffers(
        std::vector<ByteBuffer> &nonReadOnlyByteBuffers,
        const std::vector<ByteBuffer> &byteBuffers)
    {
        nonReadOnlyByteBuffers.resize(0);
        for (std::size_t i=0; i<byteBuffers.size(); ++i)
        {
            if (byteBuffers[i].getLength()  > 0)
            {
                if (byteBuffers[i].getReadOnly())
                {
                    boost::shared_ptr< std::vector<char> > spvc(
                        new std::vector<char>( byteBuffers[i].getLength()));

                    memcpy(
                        &(*spvc)[0],
                        byteBuffers[i].getPtr(),
                        byteBuffers[i].getLength() );

                    nonReadOnlyByteBuffers.push_back(
                        ByteBuffer(&(*spvc)[0], spvc->size(), spvc));
                }
                else
                {
                    nonReadOnlyByteBuffers.push_back(byteBuffers[i]);
                }
            }
        }
    }

    class XorFilter : public IdentityFilter
    {
    public:
        static const FilterDescription *spFilterDescription;
        static const FilterDescription &sGetFilterDescription();
        const FilterDescription &getFilterDescription() const;
       
        XorFilter() :
            mMask('U')
        {}
       
        void read(const ByteBuffer &byteBuffer, std::size_t bytesRequested)
        {
            getPostFilter().read(byteBuffer, bytesRequested);
        }

        void write(const std::vector<ByteBuffer> &byteBuffers)
        {
            // need to make copies of any readonly buffers before transforming
            // TODO: only do this if at least one byte buffer is non-readonly

            createNonReadOnlyByteBuffers(mByteBuffers, byteBuffers);

            // inplace transformation
            for (std::size_t i=0; i<mByteBuffers.size(); ++i)
            {
                for (std::size_t j=0; j<mByteBuffers[i].getLength() ; ++j)
                {
                    mByteBuffers[i].getPtr() [j] ^= mMask;
                }
            }

            getPostFilter().write(mByteBuffers);
            mByteBuffers.resize(0);
        }

        void onReadCompleted(const ByteBuffer &byteBuffer, int error)
        {
            RCF_UNUSED_VARIABLE(error);
            // TODO: error handling
            for (std::size_t i=0; i<byteBuffer.getLength() ; ++i)
            {
                byteBuffer.getPtr() [i] ^= mMask;
            }
            getPreFilter().onReadCompleted(byteBuffer, 0);
        }

        void onWriteCompleted(std::size_t bytesTransferred, int error)
        {
            RCF_UNUSED_VARIABLE(error);
            // TODO: error handling
            getPreFilter().onWriteCompleted(bytesTransferred, 0);
        }

    private:
        std::vector<ByteBuffer> mByteBuffers;
        char mMask;
    };
   
    typedef boost::shared_ptr<Filter>                       FilterPtr;
    typedef std::vector<FilterPtr>                          VectorFilter;
    typedef boost::shared_ptr< std::vector<FilterPtr> >     VectorFilterPtr;

    class FilterFactory
    {
    public:
        virtual ~FilterFactory()
        {}

        virtual FilterPtr createFilter() = 0;

        virtual const FilterDescription &getFilterDescription() = 0; // TODO: should be const
    };

    typedef boost::shared_ptr<FilterFactory> FilterFactoryPtr;

    class IdentityFilterFactory : public FilterFactory
    {
    public:
        FilterPtr createFilter();
        const FilterDescription &getFilterDescription();
    };

    class XorFilterFactory : public FilterFactory
    {
    public:
        FilterPtr createFilter();
        const FilterDescription & getFilterDescription();
    };

    struct FilterIdComparison
    {
        bool operator()(FilterPtr filterPtr, int filterId)
        {
            return filterPtr->getFilterDescription().getId() == filterId;
        }
    };

    void connectFilters(const std::vector<FilterPtr> &filters);

    bool filterData(
        const std::vector<ByteBuffer> &unfilteredData,
        std::vector<ByteBuffer> &filteredData,
        const std::vector<FilterPtr> &filters);

    bool unfilterData(
        const ByteBuffer &filteredByteBuffer,
        std::vector<ByteBuffer> &unfilteredByteBuffers,
        std::size_t unfilteredDataLen,
        const std::vector<FilterPtr> &filters);

    bool unfilterData(
        const ByteBuffer &filteredByteBuffer,
        ByteBuffer &unfilteredByteBuffer,
        std::size_t unfilteredDataLen,
        const std::vector<FilterPtr> &filters);

} // namespace RCF

#endif // ! INCLUDE_RCF_FILTER_HPP
