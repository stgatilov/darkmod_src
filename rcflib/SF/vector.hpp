
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_SF_VECTOR_HPP
#define INCLUDE_SF_VECTOR_HPP

#include <vector>

#include <boost/mpl/assert.hpp>

#include <SF/Serializer.hpp>
#include <SF/SerializeStl.hpp>
#include <SF/Tools.hpp>

#include <RCF/CurrentSerializationProtocol.hpp>

namespace SF {

    // std::vector

    template<typename T, typename A>
    inline void serializeVector(
        SF::Archive &ar,
        std::vector<T,A> &t,
        const unsigned int version,
        boost::mpl::false_ *)
    {
        serializeStlContainer<PushBackSemantics>(ar, t, version);
    }

    template<typename T, typename A>
    inline void serializeVector(
        SF::Archive &ar,
        std::vector<T,A> &t,
        const unsigned int version,
        boost::mpl::true_ *)
    {
        serializeVectorFast(ar, t, version);
    }

    template<typename T, typename A>
    inline void serialize(
        SF::Archive &ar,
        std::vector<T,A> &t,
        const unsigned int version)
    {
        typedef typename RCF::IsFundamental<T>::type type;
        serializeVector(ar, t, version, (type *) 0);
    }

    template<typename T, typename A>
    inline void serializeVectorFast(
        SF::Archive &ar,
        std::vector<T,A> &t,
        const unsigned int)
    {
        if (ar.isRead())
        {
            boost::uint32_t count = 0;
            ar & count;

            if (count)
            {
                SF::IStream &is = dynamic_cast<SF::IStream &>(*ar.getStream());

                t.resize(0);

                std::size_t minSerializedLength = sizeof(T);
                bool verified = RCF::verifyAgainstArchiveSize(count*minSerializedLength);
                if (verified)
                {
                    t.reserve(count);
                }

                boost::uint32_t elementsRemaining = count;
                const boost::uint32_t BufferSize = 512;
                while (elementsRemaining)
                {
                    boost::uint32_t elementsRead = count - elementsRemaining;
                    boost::uint32_t elementsToRead = RCF_MIN(BufferSize, elementsRemaining);
                    boost::uint32_t bytesToRead = elementsToRead*sizeof(T);
                    t.resize( t.size() + elementsToRead);

                    RCF_VERIFY(
                        is.read( (char*) &t[elementsRead] , bytesToRead) == bytesToRead,
                        RCF::Exception(RCF::RcfError_Deserialization))
                        (elementsToRead)(BufferSize)(count);

                    elementsRemaining -= elementsToRead;
                }
            }
        }
        else if (ar.isWrite())
        {
            boost::uint32_t count = static_cast<boost::uint32_t>(t.size());
            ar & count;
            if (count)
            {
                boost::uint32_t bytesToWrite = count * sizeof(T);
                dynamic_cast<SF::OStream *>(ar.getStream())->writeRaw(
                    (SF::Byte8 *) &t[0],
                    bytesToWrite);
            }
        }
    }

} // namespace SF

#endif // ! INCLUDE_SF_VECTOR_HPP
