
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_SF_SHARED_PTR_HPP
#define INCLUDE_SF_SHARED_PTR_HPP

//#include <boost/shared_ptr.hpp>
namespace boost {
    template<class T>
    class shared_ptr;
}

#include <SF/SerializeSmartPtr.hpp>

namespace SF {

    // serialize boost::shared_ptr
    SF_SERIALIZE_REFCOUNTED_SMARTPTR( boost::shared_ptr );

} // namespace SF

#endif // ! INCLUDE_SF_SHARED_PTR_HPP
