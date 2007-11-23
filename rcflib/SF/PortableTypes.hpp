
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_SF_PORTABLETYPES_HPP
#define INCLUDE_SF_PORTABLETYPES_HPP

#include <boost/cstdint.hpp>
#include <boost/static_assert.hpp>

// TODO: do away with this stuff, use boost typedefs instead.
// Portability of C++ types is a non-RCF issue.

namespace SF {

    //***********************************************************************
    // Define some portable types (sizeof(...) will be same on all platforms)

    typedef bool                                Bool8;
    typedef char                                Char8;
    typedef unsigned char                       UChar8;
    typedef char                                Byte8;
    typedef float                               Float32;
    typedef double                              Double64;
    typedef boost::uint16_t                     Short16;
    typedef boost::int32_t                      Int32;
    typedef boost::uint32_t                     UInt32;

    BOOST_STATIC_ASSERT( sizeof(Bool8) == 1 );
    BOOST_STATIC_ASSERT( sizeof(Char8) == 1 );
    BOOST_STATIC_ASSERT( sizeof(UChar8) == 1 );
    BOOST_STATIC_ASSERT( sizeof(Byte8) == 1 );
    BOOST_STATIC_ASSERT( sizeof(Short16) == 2 );
    BOOST_STATIC_ASSERT( sizeof(Int32) == 4 );
    BOOST_STATIC_ASSERT( sizeof(UInt32) == 4 );
    BOOST_STATIC_ASSERT( sizeof(Float32) == 4 );
    BOOST_STATIC_ASSERT( sizeof(Double64) == 8 );

} // namespace SF

#endif // ! INCLUDE_SF_PORTABLETYPES_HPP
