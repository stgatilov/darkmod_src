
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_BYTEBUFFER_HPP
#define INCLUDE_RCF_BYTEBUFFER_HPP

#include <strstream>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <RCF/Tools.hpp>
#include <RCF/TypeTraits.hpp>

#include <RCF/ThreadLocalData.hpp>
#include <RCF/util/Platform/OS/BsdSockets.hpp> // WSABUF

namespace RCF {

    // ByteBuffer class for facilitating zero-copy transmission and reception

    class ByteBuffer
    {
    public:

        ByteBuffer();

        ByteBuffer(std::size_t pvlen);

        ByteBuffer(
            const boost::shared_ptr<std::vector<char> > &spvc,
            bool readOnly = false);

        ByteBuffer(
            char *pv,
            std::size_t pvlen,
            bool readOnly = false);

        ByteBuffer(
            char *pv,
            std::size_t pvlen,
            std::size_t leftMargin,
            bool readOnly = false);

        ByteBuffer(
            char *pv,
            std::size_t pvlen,
            const boost::shared_ptr<std::ostrstream> &spos,
            bool readOnly = false);

        ByteBuffer(
            char *pv,
            std::size_t pvlen,
            std::size_t leftMargin,
            const boost::shared_ptr<std::ostrstream> &spos,
            bool readOnly = false);

        ByteBuffer(
            char *pv,
            std::size_t pvlen,
            const boost::shared_ptr<std::vector<char> > &spvc,
            bool readOnly = false);

        ByteBuffer(
            char *pv,
            std::size_t pvlen,
            std::size_t leftMargin,
            const boost::shared_ptr<std::vector<char> > &spvc,
            bool readOnly = false);

        ByteBuffer(
            const ByteBuffer &byteBuffer,
            std::size_t offset = 0,
            std::size_t len = -1);

        char *getPtr() const;
        std::size_t getLength() const;
        std::size_t getLeftMargin() const;
        bool getReadOnly() const;
        bool isEmpty() const;
        void expandIntoLeftMargin(std::size_t len);
        std::string string() const;
        ByteBuffer release();
        void clear();

        operator bool()
        {
            return getLength() != 0;
        }

        bool operator !()
        {
            return getLength() == 0;
        }

    private:
        // sentries
        boost::shared_ptr< std::vector<char> >    mSpvc;
        boost::shared_ptr< std::ostrstream >    mSpos;

        char *                                    mPv;
        std::size_t                                mPvlen;
        std::size_t                                mLeftMargin;
        bool                                    mReadOnly;

    };

    inline bool operator==(const ByteBuffer &lhs, const ByteBuffer &rhs)
    {
        return
            lhs.getPtr() == rhs.getPtr() &&
            lhs.getLength() == rhs.getLength();
    }

    std::size_t lengthByteBuffers(const std::vector<ByteBuffer> &byteBuffers);

    template<typename Functor>
    inline void forEachByteBuffer(
        const Functor &functor,
        const std::vector<ByteBuffer> &byteBuffers,
        std::size_t offset,
        std::size_t length = -1)
    {
        std::size_t pos0        = 0;
        std::size_t pos1        = 0;
        std::size_t remaining   = length;

        for (std::size_t i=0; i<byteBuffers.size(); ++i)
        {
            pos1 = pos0 + byteBuffers[i].getLength() ;

            if (pos1 <= offset)
            {
                pos0 = pos1;
            }
            else if (pos0 <= offset && offset < pos1)
            {
                std::size_t len = RCF_MIN(pos1-offset, remaining);

                ByteBuffer byteBuffer(
                    byteBuffers[i],
                    offset-pos0,
                    len);

                functor(byteBuffer);
                pos0 = pos1;
                remaining -= len;
            }
            else if (remaining > 0)
            {
                std::size_t len = RCF_MIN(pos1-pos0, remaining);

                ByteBuffer byteBuffer(
                    byteBuffers[i],
                    0,
                    len);

                functor(byteBuffer);
                pos1 = pos0;
                remaining -= len;
            }
        }
    }

    void sliceByteBuffers(
        std::vector<ByteBuffer> &slicedBuffers,
        const std::vector<ByteBuffer> &byteBuffers,
        std::size_t offset,
        std::size_t length = -1);

    void copyByteBuffers(
        const std::vector<ByteBuffer> &byteBuffers,
        char *pch);

    void copyByteBuffers(
        const std::vector<ByteBuffer> &byteBuffers,
        ByteBuffer &byteBuffer);

    // Thread local caching

    template<typename T>
    struct CacheType
    {
        typedef
            std::vector<
                std::pair<
                    boost::shared_ptr< bool>,
                    boost::shared_ptr< T > > > Val;
    };

#ifndef BOOST_WINDOWS
    typedef iovec WSABUF;
#endif

    class ObjectCache
    {
    public:

        typedef CacheType< std::vector<RCF::ByteBuffer> >::Val  VectorByteBufferCache;
        typedef CacheType< std::vector<int> >::Val              VectorIntCache;
        typedef CacheType< std::vector<WSABUF> >::Val           VectorWsabufCache;

        VectorByteBufferCache &getCache(VectorByteBufferCache *)
        {
            return mVectorByteBufferCache;
        }

        VectorIntCache &getCache(VectorIntCache *)
        {
            return mVectorIntCache;
        }

        VectorWsabufCache &getCache(VectorWsabufCache *)
        {
            return mVectorWsabufCache;
        }

        void clear()
        {
            mVectorByteBufferCache.clear();
            mVectorIntCache.clear();
            mVectorWsabufCache.clear();
        }

    private:
        VectorByteBufferCache   mVectorByteBufferCache;
        VectorIntCache          mVectorIntCache;
        VectorWsabufCache       mVectorWsabufCache;
    };

    //typedef boost::shared_ptr<ObjectCache> ObjectCachePtr;
    // TODO: in its own header
    //class ThreadLocalData;
    //typedef boost::shared_ptr<ThreadLocalData> ThreadLocalDataPtr;
    //ThreadLocalDataPtr getThreadLocalDataPtr();

    //ObjectCache &getThreadLocalObjectCache();

    template<typename T>
    class ThreadLocalCached
    {
    public:

        typedef typename CacheType<T>::Val TCache;

        ThreadLocalCached()
        {
            //ObjectCachePtr objectCachePtr = getThreadLocalObjectCachePtr();
            ObjectCache &objectCache = getThreadLocalObjectCache();

            TCache &tCache = objectCache.getCache( (TCache *) NULL);
            for (std::size_t i=0; i<tCache.size(); ++i)
            {
                if (*tCache[i].first == false)
                {
                    mMarkPtr = tCache[i].first;
                    mtPtr = tCache[i].second;
                    break;
                }
            }
            if (!mtPtr)
            {
                typedef typename TCache::value_type ValueType;
                tCache.push_back( ValueType());
                tCache.back().first.reset( new bool());
                tCache.back().second.reset( new T());
                mMarkPtr = tCache.back().first;
                mtPtr = tCache.back().second;
            }
            *mMarkPtr = true;
        }

        ~ThreadLocalCached()
        {
            *mMarkPtr = false;
            mtPtr->resize(0);
        }

        T &get()
        {
            return *mtPtr;
        }

    private:

        boost::shared_ptr< bool> mMarkPtr;
        boost::shared_ptr< T > mtPtr;

    };

} // namespace RCF

RCF_BROKEN_COMPILER_TYPE_TRAITS_SPECIALIZATION(RCF::ByteBuffer)

#endif // ! INCLUDE_RCF_BYTEBUFFER_HPP
