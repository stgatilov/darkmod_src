
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_SF_SET_HPP
#define INCLUDE_SF_SET_HPP

#include <set>

#include <SF/SerializeStl.hpp>

namespace SF {

    // std::set
    template<typename K, typename T, typename A>
    inline void serialize(SF::Archive &ar, std::set<K,T,A> &t, const unsigned int version)
    {
        serializeStlContainer<InsertSemantics>(ar, t, version);
    }

    // std::multiset
    template<typename K, typename T, typename A>
        inline void serialize(SF::Archive &ar, std::multiset<K,T,A> &t, const unsigned int version)
    {
        serializeStlContainer<InsertSemantics>(ar, t, version);
    }

} // namespace SF

#endif // ! INCLUDE_SF_SET_HPP
