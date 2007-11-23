
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_SF_WSTRING_HPP
#define INCLUDE_SF_WSTRING_HPP

#include <string>

#include <RCF/Tools.hpp>

#include <SF/Archive.hpp>
#include <SF/string.hpp>

namespace SF {

    void serialize(SF::Archive &ar, std::wstring &ws, const unsigned int)
    {
        if (ar.isRead())
        {
            std::string s;
            ar & s;
            ws = util::stringToWstring(s);
        }
        else if (ar.isWrite())
        {
            std::string s;
            s = util::wstringToString(ws);
            ar & s;
        }
    }

} // namespace SF

#endif // ! INCLUDE_SF_WSTRING_HPP
