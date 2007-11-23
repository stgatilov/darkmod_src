
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#include <RCF/Token.hpp>

#include <iostream>

namespace RCF {

    //*****************************************
    // Token

    Token::Token() :
        mId(RCF_DEFAULT_INIT)
    {}

    bool operator<(const Token &lhs, const Token &rhs)
    {
        return (lhs.getId() < rhs.getId());
    }

    bool operator==(const Token &lhs, const Token &rhs)
    {
        return lhs.getId() == rhs.getId();
    }

    bool operator!=(const Token &lhs, const Token &rhs)
    {
        return ! (lhs == rhs);
    }

    int Token::getId() const
    {
        return mId;
    }
   
    std::ostream &operator<<(std::ostream &os, const Token &token)
    {
        os << "( id = " << token.getId() << " )";
        return os;
    }

    Token::Token(int id) :
        mId(id)
    {}
   
} // namespace RCF
