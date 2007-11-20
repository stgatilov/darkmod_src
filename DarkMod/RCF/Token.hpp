
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_TOKEN_HPP
#define INCLUDE_RCF_TOKEN_HPP

#include <iosfwd>

namespace RCF {

    class Connection;

    class Token
    {
    public:
        Token();
        Token(int id);
        int getId() const;
        friend bool operator<(const Token &lhs, const Token &rhs);
        friend bool operator==(const Token &lhs, const Token &rhs);
        friend bool operator!=(const Token &lhs, const Token &rhs);
       
        template<typename Archive> void serialize(Archive &ar, const unsigned int)
        {
            ar & mId;
        }
       
        friend std::ostream &operator<<(std::ostream &os, const Token &token);

    private:
        int mId;
    };

} // namespace RCF

#endif // ! INCLUDE_RCF_TOKEN_HPP
