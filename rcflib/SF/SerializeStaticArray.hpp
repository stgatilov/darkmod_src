
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_SF_SERIALIZESTATICARRAY_HPP
#define INCLUDE_SF_SERIALIZESTATICARRAY_HPP

#include <SF/Archive.hpp>

namespace SF {

    // serialize C-style static arrays

    template<typename T, unsigned int N>
    inline void serializeFundamentalStaticArray(Archive &ar, T (*pt)[N], const unsigned int version)
    {
        serializeFundamental(ar, (*pt)[0], version, N);
    }

    template<typename T, unsigned int N>
    inline void serializeNonfundamentalStaticArray(Archive &ar, T (*pt)[N], const unsigned int)
    {
        for (unsigned int i=0; i<N; i++)
            ar & (*pt)[i];
    }


    template<bool IsFundamental>
    class SerializeStaticArray;

    template<>
    class SerializeStaticArray<true>
    {
    public:
        template<typename Archive, typename T, unsigned int N>
        void operator()(Archive &ar, T (*pt)[N], const unsigned int version)
        {
            serializeFundamentalStaticArray(ar, pt, version);
        }
    };

    template<>
    class SerializeStaticArray<false>
    {
    public:
        template<typename Archive, typename T, unsigned int N>
        void operator()(Archive &ar, T (*pt)[N], const unsigned int version)
        {
            serializeNonfundamentalStaticArray(ar, pt, version);
        }
    };


    template<typename Archive, typename T, unsigned int N>
    inline void preserialize(Archive &ar, T (*pt)[N], const unsigned int version)
    {
        static const bool IsFundamental = RCF::IsFundamental<T>::value;
        SerializeStaticArray<IsFundamental>()(ar, pt, version);
    }

} // namespace SF

#endif // ! INCLUDE_SF_SERIALIZESTATICARRAY_HPP
