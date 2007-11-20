
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_SF_MEMORY_HPP
#define INCLUDE_SF_MEMORY_HPP

#include <memory>

#include <SF/SerializeSmartPtr.hpp>

namespace SF {

    // serialize std::auto_ptr
    SF_SERIALIZE_SIMPLE_SMARTPTR( std::auto_ptr );

} // namespace SF

#endif // ! INCLUDE_SF_MEMORY_HPP
