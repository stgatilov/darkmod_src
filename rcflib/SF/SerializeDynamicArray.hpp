
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_SF_SERIALIZEDYNAMICARRAY_HPP
#define INCLUDE_SF_SERIALIZEDYNAMICARRAY_HPP

#include <SF/SerializeFundamental.hpp>
#include <SF/SfNew.hpp>

namespace SF {

    // serialize C-style dynamic arrays

    template<typename T, typename N>
    class DynamicArray
    {
    public:
        DynamicArray( const T *&pt, const N &n ) : pt_( const_cast< T*&>(pt) ), n_( const_cast<N &>(n) ) {}
        DynamicArray(       T *&pt, const N &n ) : pt_( const_cast< T*&>(pt) ), n_( const_cast<N &>(n) ) {}
        DynamicArray(const DynamicArray &rhs) : pt_(rhs.pt_), n_(rhs.n_) {}
        T *&get() { return pt_; }
        N &length() { return n_; }
        T &operator[](unsigned int idx) { RCF_ASSERT( get() != NULL && 0 <= idx && idx < length() )(get())(idx)(length()); return *(get() + idx); }
    private:
        DynamicArray &operator=(const DynamicArray &rhs);    // Can't reassign reference members
        T *&pt_;
        N &n_;
    };

    template<typename T, typename N>
    inline void serializeFundamentalDynamicArray(
        Archive &ar,
        DynamicArray<T,N> &da,
        const unsigned int)
    {
        if (ar.isRead())
        {
            I_Encoding &e = dynamic_cast< I_WithEncoding * >(ar.getStream())->getEncoding();
            DataPtr data;
            bool bRet = dynamic_cast< WithFormatRead* >(ar.getStream())->get( data );
            if (bRet)
            {
                UInt32 nCount = e.getCount( data , typeid(T));
                da.get() = new T[ nCount ];
                da.length() = nCount;
                e.toObject(data, da.get(), typeid(T), nCount );
            }
        }
        else if (ar.isWrite())
        {
            if (da.length() > 0)
            {
                I_Encoding &e = dynamic_cast< I_WithEncoding * >(ar.getStream())->getEncoding();
                DataPtr data;
                e.toData(data, da.get(), typeid(T), da.length() );
                dynamic_cast< WithFormatWrite* >(ar.getStream())->put(data);
            }
        }
    }

    template<typename T, typename N>
    inline void serializeNonfundamentalDynamicArray(
        Archive &ar,
        DynamicArray<T,N> &da,
        const unsigned int)
    {
        if (ar.isRead())
        {
            UInt32 nCount;
            ar & nCount;
            da.get() = new T[nCount];
            da.length() = nCount;
            for (UInt32 i=0; i<da.length(); i++)
                ar & da[i];
        }
        else if (ar.isWrite())
        {
            ar & da.length();
            for (UInt32 i=0; i<da.length(); i++)
                ar & da[i];
        }
    }

    template<typename T, typename N>
    inline void serialize(Archive &ar, DynamicArray<T,N> &da, const unsigned int version)
    {
        const bool isFundamental = RCF::IsFundamental<T>::value;
        if (isFundamental)
        {
            serializeFundamentalDynamicArray(ar, da, version);
        }
        else
        {
            serializeNonfundamentalDynamicArray(ar, da, version);
        }
    }

    SF_NO_CTOR_T2( DynamicArray )

    template<typename T, typename N>
    inline DynamicArray<T,N> dynamicArray(const T *&pt, const N &size)
    {
        return DynamicArray<T,N>(pt, size);
    }

    template<typename T, typename N>
    inline DynamicArray<T,N> dynamicArray(      T *&pt, const N &size)
    {
        return DynamicArray<T,N>(pt, size);
    }

} // namespace SF

#endif // ! INCLUDE_SF_SERIALIZEDYNAMICARRAY_HPP
