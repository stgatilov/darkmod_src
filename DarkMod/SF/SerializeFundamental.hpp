
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_SF_SERIALIZEFUNDAMENTAL_HPP
#define INCLUDE_SF_SERIALIZEFUNDAMENTAL_HPP

#include <SF/Archive.hpp>
#include <SF/DataPtr.hpp>
#include <SF/I_Stream.hpp>
#include <SF/Serializer.hpp>
#include <SF/Tools.hpp>

namespace SF {

    // serialize fundamental types

    template<typename T>
    inline void serializeFundamental(SF::Archive &ar, T &t, const unsigned int, unsigned int count = 1)
    {
        typedef typename RCF::RemoveCv<T>::type U;
        BOOST_STATIC_ASSERT( RCF::IsFundamental<U>::value );
        U *pu = const_cast<U *>(&t);
        void *pvt = pu;

        if (ar.isRead())
        {
            I_Encoding &encoding = dynamic_cast<I_WithEncoding *>(ar.getStream())->getEncoding();
            DataPtr data;
            dynamic_cast<WithFormatRead *>(ar.getStream())->get(data);
            if (count > 1 && count != encoding.getCount(data,typeid(U)) )
            {
                // static array size mismatch
                RCF_THROW(RCF::Exception(RCF::SfError_DataFormat))(typeid(U).name())(count)(encoding.getCount(data,typeid(T)));
            }
            encoding.toObject(data, pvt, typeid(T), count );
        }
        else if (ar.isWrite())
        {
            I_Encoding &encoding = dynamic_cast<I_WithEncoding *>(ar.getStream())->getEncoding();
            DataPtr data;
            encoding.toData(data, pvt, typeid(U), count );
            dynamic_cast<WithFormatWrite *>(ar.getStream())->put(data);
        }
    }

} // namespace SF

#endif // ! INCLUDE_SF_SERIALIZEFUNDAMENTAL_HPP
