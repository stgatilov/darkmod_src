
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_SF_STRING_HPP
#define INCLUDE_SF_STRING_HPP

#include <algorithm>
#include <string>

#include <SF/Archive.hpp>
#include <SF/SerializeDynamicArray.hpp>

#include <RCF/CurrentSerializationProtocol.hpp>

namespace SF {

    // std::basic_string
    template<typename C, typename T, typename A>
    inline void serialize(SF::Archive &ar, std::basic_string<C,T,A> &t, const unsigned int)
    {
        if (ar.isRead())
        {
            boost::uint32_t count = 0;
            ar & count;

            SF::IStream &is = dynamic_cast<SF::IStream &>(*ar.getStream());

            t.resize(0);

            std::size_t minSerializedLength = sizeof(C);
            bool verified = RCF::verifyAgainstArchiveSize(count*minSerializedLength);
            if (verified)
            {
                t.reserve(count);
            }

            boost::uint32_t charsRemaining = count;
            const boost::uint32_t BufferSize = 512;
            C buffer[BufferSize];
            while (charsRemaining)
            {
                boost::uint32_t charsToRead = RCF_MIN(BufferSize, charsRemaining);
                boost::uint32_t bytesToRead = charsToRead*sizeof(C);

                RCF_VERIFY(
                    is.read( (char *) buffer, bytesToRead) == bytesToRead,
                    RCF::Exception(RCF::RcfError_Deserialization))
                    (bytesToRead)(BufferSize)(count);

                t.append(buffer, charsToRead);
                charsRemaining -= charsToRead;
            }
        }
        else if (ar.isWrite())
        {
            boost::uint32_t count = static_cast<boost::uint32_t >(t.length());
            ar & count;
            dynamic_cast<SF::OStream *>(ar.getStream())->writeRaw(
                t.c_str(),
                count*sizeof(C));
        }
    }

} // namespace SF

#endif // ! INCLUDE_SF_STRING_HPP
