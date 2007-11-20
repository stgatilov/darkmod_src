
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_UTIL_ASSERT_HPP
#define INCLUDE_UTIL_ASSERT_HPP

#include <cassert>
#include <exception>

#include <boost/current_function.hpp>

#include "Throw.hpp"

namespace util {

    class VarArgAssert
    {
    public:
        VarArgAssert()
        {
            assert(0);
        }

        template<typename T>
        VarArgAssert &operator()(const T &)
        {
            return *this;
        }
    };

} // namespace util

#define UTIL_ASSERT_DEBUG(cond, e, channel)                                 \
    if (cond) ;                                                             \
    else util::VarArgAssert()

#define UTIL_ASSERT_RELEASE(cond, e, channel)                               \
    if (cond) ;                                                             \
    else UTIL_THROW(e, channel)(cond)

#define UTIL_ASSERT_NULL(cond, E)                                           \
    DUMMY_VARIABLE_ARG_MACRO()

#ifdef NDEBUG
#define UTIL_ASSERT UTIL_ASSERT_RELEASE
#else
#define UTIL_ASSERT UTIL_ASSERT_DEBUG
#endif

#endif //! INCLUDE_UTIL_ASSERT_HPP
