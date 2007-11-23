
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#include <RCF/UsingOpenSsl.hpp>

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/ssl.h>

#include <vector>

#include <boost/shared_ptr.hpp>

#include <RCF/InitDeinit.hpp>
#include <RCF/Tools.hpp>

namespace RCF {

    // Calling ERR_print_errors_fp() crashes the whole app for some reason,
    // so call my_ERR_print_errors_fp() instead,
    // it does the exact same thing.
    int my_print_fp(const char *str, size_t len, void *fp)
    {
        (void) len;
        return fprintf((FILE *)fp, "%s", str);
    }

    void my_ERR_print_errors_fp(FILE *fp)
    {
        ERR_print_errors_cb(my_print_fp, fp);
    }

    std::string getOpenSslErrors()
    {
        boost::shared_ptr<BIO> bio( BIO_new( BIO_s_mem() ), BIO_free );
        ERR_print_errors(bio.get());
        std::vector<char> buffer(256);
        unsigned int startPos = 0;
        unsigned int bytesRead = 0;
        while (true)
        {
            RCF_ASSERT(
                buffer.size() > startPos)
                (buffer.size())(startPos);
           
            int ret = BIO_read(
                bio.get(),
                &buffer[startPos],
                static_cast<int>(buffer.size()-startPos));

            if (ret > 0)
            {
                bytesRead += ret;
            }
            if (bytesRead < buffer.size())
            {
                break;
            }
            startPos = static_cast<unsigned int>(buffer.size());
            buffer.resize( 2*buffer.size() );
        }

        return std::string(&buffer[0], bytesRead);
    }

    void initOpenSsl()
    {
        SSL_library_init(); // always returns 1
        SSL_load_error_strings(); // no return value
#ifndef __BORLANDC__
        ERR_load_crypto_strings(); // no return value
#endif
        OpenSSL_add_all_algorithms(); // no return value
    }

    RCF_ON_INIT_NAMED( initOpenSsl();, InitOpenSsl)

} // namespace RCF
