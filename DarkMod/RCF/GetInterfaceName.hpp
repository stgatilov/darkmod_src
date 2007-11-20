
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_GETINTERFACENAME_HPP
#define INCLUDE_RCF_GETINTERFACENAME_HPP

#include <string>

namespace RCF {

    /// Returns the runtime name of the given RCF interface.
    template<typename Interface>
    inline std::string getInterfaceName(Interface * = 0)
    {
        return Interface::getInterfaceName();
    }

} // namespace RCF

#endif // ! INCLUDE_RCF_GETINTERFACENAME_HPP
