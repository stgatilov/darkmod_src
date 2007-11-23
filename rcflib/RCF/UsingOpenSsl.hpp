
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_USINGOPENSSL_HPP
#define INCLUDE_RCF_USINGOPENSSL_HPP

#include <string>

namespace RCF {

    // Calling ERR_print_errors_fp() crashes the whole app for some reason,
    // so call my_ERR_print_errors_fp() instead,
    // it does the exact same thing.
    int my_print_fp(const char *str, size_t len, void *fp);
    void my_ERR_print_errors_fp(FILE *fp);
    std::string getOpenSslErrors();
    void initOpenSsl();

} // namespace RCF

#endif // ! INCLUDE_RCF_USINGOPENSSL_HPP
