
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_OPENSSLENCRYPTIONFILTER_HPP
#define INCLUDE_RCF_OPENSSLENCRYPTIONFILTER_HPP

#include <memory>
#include <string>

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include <RCF/AsyncFilter.hpp>

typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;

namespace RCF {

    /// Enumeration describing the role in a SSL conversation that an endpoint is playing.
    enum SslRole
    {
        SslServer,
        SslClient
    };

    class OpenSslEncryptionFilter;
    class OpenSslEncryptionFilterImpl;

    typedef boost::function1<bool, OpenSslEncryptionFilter&> VerifyFunctor;

    /// Filter implementing the SSL encryption protocol, through the OpenSSL library.
    class OpenSslEncryptionFilter : public Filter, boost::noncopyable
    {
    public:
        // TODO: should be private
        static const FilterDescription *spFilterDescription;

        static const FilterDescription & sGetFilterDescription();
        const FilterDescription & getFilterDescription() const;
        OpenSslEncryptionFilter(
            const std::string &certificateFile,
            const std::string &certificateFilePassword,
            const std::string &caCertificate = "",
            const std::string &ciphers = "",
            VerifyFunctor verifyFunctor = 0,
            SslRole sslRole = SslClient,
            unsigned int bioBufferSize = 2048);
        void reset();
        void read(const ByteBuffer &byteBuffer, std::size_t bytesRequested);
        void write(const std::vector<ByteBuffer> &byteBuffers);
        void onReadCompleted(const ByteBuffer &byteBuffer, int error);
        void onWriteCompleted(std::size_t bytesTransferred, int error);

        SSL *getSSL();
        SSL_CTX *getCTX();

    private:
        friend class OpenSslEncryptionFilterImpl;
        boost::shared_ptr<OpenSslEncryptionFilterImpl> mImplPtr;
    };

    /// Filter factory for OpenSslEncryptionFilter.
    class OpenSslEncryptionFilterFactory : public FilterFactory
    {
    public:
        OpenSslEncryptionFilterFactory(
            const std::string &certificateFile,
            const std::string &certificateFilePassword,
            const std::string &caCertificate = "",
            const std::string &ciphers = "",
            VerifyFunctor verifyFunctor = 0,
            bool serverRole = true);
        FilterPtr createFilter();
        const FilterDescription & getFilterDescription();

    private:
        std::string     mCertificateFile;
        std::string     mCertificateFilePassword;
        std::string     mCaCertificate;
        std::string     mCiphers;
        VerifyFunctor   mVerifyFunctor;
        SslRole         mRole;
    };


} // namespace RCF

#endif // ! INCLUDE_RCF_OPENSSLENCRYPTIONFILTER_HPP
