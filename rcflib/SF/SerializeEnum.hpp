
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_SF_SERIALIZEENUM_HPP
#define INCLUDE_SF_SERIALIZEENUM_HPP

#include <SF/Archive.hpp>
#include <SF/Serializer.hpp>

#define SF_SERIALIZE_ENUM(type)                                                        \
    inline void serialize(::SF::Archive &ar, type &t, const unsigned int version)   \
    {                                                                                \
        serializeEnum(ar, t, version);                                                \
    }

#endif // ! INCLUDE_SF_SERIALIZEENUM_HPP
