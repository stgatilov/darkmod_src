
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_SF_LIST_HPP
#define INCLUDE_SF_LIST_HPP

#include <list>

#include <SF/SerializeStl.hpp>

namespace SF {

    // std::list
    template<typename T, typename A>
    inline void serialize(SF::Archive &ar, std::list<T,A> &t, const unsigned int version)
    {
        serializeStlContainer<PushBackSemantics>(ar, t, version);
    }

} // namespace SF

#endif // ! INCLUDE_SF_LIST_HPP
