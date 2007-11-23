
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_SF_TOOLS_HPP
#define INCLUDE_SF_TOOLS_HPP

#include <RCF/Tools.hpp>

#include <boost/cstdint.hpp>

//#define RCF_ASSERT(x) RCF_ASSERT(x)
//#define SF_TRACE(x) RCF_TRACE(x)
//#define SF_THROW(E, msg) RCF_THROW(E(msg))

//****************************************************************************
// Helper macro to generate code for fundamental types
// TODO: add compiler-specific types (__int64 etc) ?
// TODO: replace all the types in the macro with portable boost types?


#define SF_FOR_EACH_FUNDAMENTAL_TYPE_(arg)  \
    arg(char)                               \
    arg(int)                                \
    arg(bool)                               \
    arg(float)                              \
    arg(double)                             \
    arg(short)                              \
    arg(long)                               \
    arg(unsigned short)                     \
    arg(unsigned char)                      \
    arg(unsigned int)                       \
    arg(unsigned long)                      \
    arg(long double)                        \
    //arg(wchar_t)
/*
#ifndef BOOST_NO_INT64_T

#define SF_FOR_EACH_FUNDAMENTAL_TYPE(arg)   \
    SF_FOR_EACH_FUNDAMENTAL_TYPE_(arg)      \
    arg(boost::int64_t)                     \
    arg(boost::uint64_t)

#else

#define SF_FOR_EACH_FUNDAMENTAL_TYPE(arg)   \
    SF_FOR_EACH_FUNDAMENTAL_TYPE_(arg)

#endif
*/
#if defined(_MSC_VER) || defined(__BORLANDC__)

#define SF_FOR_EACH_FUNDAMENTAL_TYPE(arg)   \
    SF_FOR_EACH_FUNDAMENTAL_TYPE_(arg)      \
    arg(__int64)                            \
    arg(unsigned __int64)

#else

#define SF_FOR_EACH_FUNDAMENTAL_TYPE(arg)   \
    SF_FOR_EACH_FUNDAMENTAL_TYPE_(arg)      \
    arg(long long)                          \
    arg(unsigned long long)

#endif

#endif // ! INCLUDE_SF_TOOLS_HPP

