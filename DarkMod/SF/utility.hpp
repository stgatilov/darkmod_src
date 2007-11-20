
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_SF_UTILITY_HPP
#define INCLUDE_SF_UTILITY_HPP

#include <utility>

#include <SF/Archive.hpp>

namespace SF {

    // std::pair
    template<typename T, typename U>
    inline void serialize(Archive &ar, std::pair<T,U> &t, const unsigned int)
    {
        ar & t.first & t.second;
    }

} // namespace SF

#endif // ! INCLUDE_SF_UTILITY_HPP
