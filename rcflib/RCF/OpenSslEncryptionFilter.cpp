
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#include <RCF/OpenSslEncryptionFilter.hpp>

#include <openssl/bio.h>
#include <openssl/crypto.h>
#include <openssl/engine.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

#include <RCF/InitDeinit.hpp>
#include <RCF/Tools.hpp>
#include <RCF/UsingOpenSsl.hpp>

namespace RCF {

    class OpenSslEncryptionFilterImpl
    {
    public:
        OpenSslEncryptionFilterImpl(
            OpenSslEncryptionFilter &openSslEncryptionFilter,
            SslRole sslRole,
            const std::string &certificateFile,
            const std::string &certificateFilePassword,
            const std::string &caCertificate,
            const std::string &ciphers,
            VerifyFunctor verifyFunctor,
            unsigned int bioBufferSize);

        void reset();
        void read(const ByteBuffer &byteBuffer, std::size_t bytesRequested);
        void write(const std::vector<ByteBuffer> &byteBuffers);
        void onReadWriteCompleted(std::size_t bytesTransferred, int error);

        SSL *getSSL();
        SSL_CTX *getCTX();

    private:
        void init();
        bool loadCertificate(
            boost::shared_ptr<SSL_CTX> ctx,
            const std::string &file,
            const std::string &password);

        bool loadCaCertificate(
            boost::shared_ptr<SSL_CTX> ctx,
            const std::string &file);

        void readWrite();
        void transferData();
        void onDataTransferred(std::size_t bytesTransferred);
        void retryReadWrite();

        std::size_t pos;
        std::size_t readRequested;
        ByteBuffer preByteBuffer;
        std::vector<ByteBuffer> mByteBuffers;
        boost::shared_ptr< std::vector<char> > mVecPtr;


        enum IoState
        {
            Ready,
            Reading,
            Writing
        };

        // server or client
        SslRole sslRole;

        // certificate
        std::string certificateFile;
        std::string certificateFilePassword;

        // ca certificate
        std::string caCertificate;

        // ciphers
        std::string ciphers;

        // input state
        IoState preState;

        // output state
        IoState postState;

        // retry state
        bool retry;
        bool handshakeOk;

        char *preBuffer;
        char *postBuffer;
        std::size_t postBufferLen;
        std::size_t postBufferRequested;

        // verification callback
        VerifyFunctor mVerifyFunctor;

        int err;

        // OpenSSL members
        // NB: using shared_ptr instead of auto_ptr, since we need custom deleters
        boost::shared_ptr<SSL_CTX> ssl_ctx;
        boost::shared_ptr<SSL> ssl;
        boost::shared_ptr<BIO> bio;
        boost::shared_ptr<BIO> io_bio;
        boost::shared_ptr<BIO> ssl_bio;

        unsigned int bioBufferSize;

        OpenSslEncryptionFilter &openSslEncryptionFilter;
    };

    const FilterDescription & OpenSslEncryptionFilter::sGetFilterDescription()
    {
        return *spFilterDescription;
    }

    const FilterDescription *OpenSslEncryptionFilter::spFilterDescription = NULL;

    static void initOpenSslEncryptionFilterDescription()
    {
        RCF_ASSERT(!OpenSslEncryptionFilter::spFilterDescription);
        OpenSslEncryptionFilter::spFilterDescription =
            new FilterDescription(
                "OpenSSL encryption filter",
                RCF_FILTER_OPENSSL_ENCRYPTION,
                true);
    }

    static void deinitOpenSslEncryptionFilterDescription()
    {
        delete OpenSslEncryptionFilter::spFilterDescription;
        OpenSslEncryptionFilter::spFilterDescription = NULL;
    }

    RCF_ON_INIT_DEINIT(
        initOpenSslEncryptionFilterDescription(),
        deinitOpenSslEncryptionFilterDescription())

#ifdef _MSC_VER
#pragma warning( push )
// warning C4355: 'this' : used in base member initializer list
#pragma warning( disable : 4355 ) 
#endif

    OpenSslEncryptionFilter::OpenSslEncryptionFilter(
        const std::string &certificateFile,
        const std::string &certificateFilePassword,
        const std::string &caCertificate,
        const std::string &ciphers,
        VerifyFunctor verifyFunctor,
        SslRole sslRole,
        unsigned int bioBufferSize) :
            mImplPtr( new OpenSslEncryptionFilterImpl(
                *this,
                sslRole,
                certificateFile,
                certificateFilePassword,
                caCertificate,
                ciphers,
                verifyFunctor,
                bioBufferSize) )
    {}

#ifdef _MSC_VER
#pragma warning( pop )
#endif

    void OpenSslEncryptionFilter::reset()
    {
        mImplPtr->reset();
    }

    void OpenSslEncryptionFilter::read(
        const ByteBuffer &byteBuffer,
        std::size_t bytesRequested)
    {
        mImplPtr->read(byteBuffer, bytesRequested);
    }

    void OpenSslEncryptionFilter::write(
        const std::vector<ByteBuffer> &byteBuffers)
    {
        mImplPtr->write(byteBuffers);
    }

    void OpenSslEncryptionFilter::onReadCompleted(
        const ByteBuffer &byteBuffer,
        int error)
    {
        mImplPtr->onReadWriteCompleted(byteBuffer.getLength(), error);
    }

    void OpenSslEncryptionFilter::onWriteCompleted(
        std::size_t bytesTransferred,
        int error)
    {
        mImplPtr->onReadWriteCompleted(bytesTransferred, error);
    }

    SSL *OpenSslEncryptionFilter::getSSL()
    {
        return mImplPtr->getSSL();
    }

    SSL_CTX *OpenSslEncryptionFilter::getCTX()
    {
        return mImplPtr->getCTX();
    }

    SSL *OpenSslEncryptionFilterImpl::getSSL()
    {
        return ssl.get();
    }

    SSL_CTX *OpenSslEncryptionFilterImpl::getCTX()
    {
        return ssl_ctx.get();
    }

    const FilterDescription & OpenSslEncryptionFilter::getFilterDescription() const
    {
        return sGetFilterDescription();
    }

    OpenSslEncryptionFilterImpl::OpenSslEncryptionFilterImpl(
        OpenSslEncryptionFilter &openSslEncryptionFilter,
        SslRole sslRole,
        const std::string &certificateFile,
        const std::string &certificateFilePassword,
        const std::string &caCertificate,
        const std::string &ciphers,
        VerifyFunctor verifyFunctor,
        unsigned int bioBufferSize) :
            sslRole(sslRole),
            certificateFile(certificateFile),
            certificateFilePassword(certificateFilePassword),
            caCertificate(caCertificate),
            ciphers(ciphers),
            preBuffer(RCF_DEFAULT_INIT),
            postBuffer(RCF_DEFAULT_INIT),
            postBufferLen(RCF_DEFAULT_INIT),
            postBufferRequested(RCF_DEFAULT_INIT),
            preState(Ready),
            postState(Ready),
            bioBufferSize(bioBufferSize),
            retry(RCF_DEFAULT_INIT),
            handshakeOk(RCF_DEFAULT_INIT),
            mVerifyFunctor(verifyFunctor),
            err(RCF_DEFAULT_INIT),
            openSslEncryptionFilter(openSslEncryptionFilter)
    {
        // TODO: don't hold on to the password, use it immediately
        init();
    }

    void OpenSslEncryptionFilterImpl::read(
        const ByteBuffer &byteBuffer,
        std::size_t bytesRequested)
    {
        RCF_ASSERT(preState == Ready)(preState);
        preState = Reading;
        if (byteBuffer.getLength() == 0)
        {
            if (mVecPtr.get() == NULL && !mVecPtr.unique())
            {
                mVecPtr.reset(new std::vector<char>(bytesRequested));
            }
            mVecPtr->resize(bytesRequested);
            preByteBuffer = ByteBuffer(mVecPtr);
        }
        else
        {
            preByteBuffer = byteBuffer;
        }
        readRequested = bytesRequested;
        readWrite();

    }

    void OpenSslEncryptionFilterImpl::write(
        const std::vector<ByteBuffer> &byteBuffers)
    {
        RCF_ASSERT(preState == Ready)(preState);
        preState = Writing;
        preByteBuffer = byteBuffers.front();
        readWrite();
    }

    void OpenSslEncryptionFilterImpl::onReadWriteCompleted(
        std::size_t bytesTransferred,
        int error)
    {
        RCF_VERIFY(error == 0, FilterException(RcfError_Filter));

        // complete the data transfer
        onDataTransferred(bytesTransferred);

        if (retry)
        {
            retryReadWrite();
        }
        else
        {
            if (preState == Writing && BIO_ctrl_pending(io_bio.get()) > 0)
            {
                transferData();
            }
            else
            {
                IoState state = preState;
                preState = Ready;
                if (state == Reading)
                {
                    ByteBuffer byteBuffer(preByteBuffer.release(), 0, pos);
                    openSslEncryptionFilter.mpPreFilter->onReadCompleted(byteBuffer, 0);
                }
                else
                {
                    preByteBuffer.clear();
                    openSslEncryptionFilter.mpPreFilter->onWriteCompleted(pos, 0);
                }
            }
        }
    }


    void OpenSslEncryptionFilterImpl::readWrite()
    {
        // set input parameters
        retry = true;
        err = 0;
        pos = 0;
        retryReadWrite();
    }

    void OpenSslEncryptionFilterImpl::retryReadWrite()
    {
        RCF_ASSERT(preState == Reading || preState == Writing)(preState);

        int sslState = SSL_get_state(ssl.get());
        if (!handshakeOk && sslState == SSL_ST_OK)
        {
            handshakeOk = true;
            {
                if (!caCertificate.empty())
                {
                    // verify the peers certificate against our CA's
                    bool verifyOk = (SSL_get_verify_result(ssl.get()) == X509_V_OK);
                    boost::shared_ptr<X509> peerCert(
                        SSL_get_peer_certificate(ssl.get()),
                        X509_free);
                    if (!peerCert || !verifyOk)
                    {
                        if (!mVerifyFunctor || !mVerifyFunctor(openSslEncryptionFilter))
                        {
                            RCF_THROW(Exception(RcfError_SslCertVerification));
                        }
                    }
                }
            }
        }
        else if (handshakeOk && sslState != SSL_ST_OK)
        {
            handshakeOk = false;
        }
        

        int bioRet = (preState == Reading) ?
            BIO_read(ssl_bio.get(), preByteBuffer.getPtr(), static_cast<int>(readRequested)) :
            BIO_write(ssl_bio.get(), preByteBuffer.getPtr(), static_cast<int>(preByteBuffer.getLength()));


        RCF_ASSERT(
            -1 <= bioRet && bioRet <= static_cast<int>(preByteBuffer.getLength()))
            (bioRet)(preByteBuffer.getLength());

        if (bioRet == -1 && BIO_should_retry(ssl_bio))
        {
            // initiate io requests on underlying filters
            retry = true;
            transferData();
        }
        else if (0 < bioRet && bioRet <= static_cast<int>(preByteBuffer.getLength()))
        {
            retry = false;
            pos += bioRet;
            if (preState == Writing)
            {
                // TODO: maybe this is not always true
                RCF_ASSERT(BIO_ctrl_pending(io_bio.get()) > 0);
                transferData();
            }
            else
            {
                preState = Ready;
                //openSslEncryptionFilter.mpPreFilter->onWriteCompleted(pos, 0);
                ByteBuffer byteBuffer(preByteBuffer.release(), 0, pos);
                openSslEncryptionFilter.mpPreFilter->onReadCompleted(byteBuffer, 0);
            }
        }
        else
        {
            err = -1;
        }
    }

    void OpenSslEncryptionFilterImpl::transferData()
    {
        if (BIO_ctrl_pending(io_bio.get()) == 0)
        {
            // move data from network to io_bio
            postState = Reading;
            postBufferRequested =
                static_cast<int>(BIO_ctrl_get_read_request(io_bio.get()));

            postBufferLen = BIO_nwrite0(io_bio.get(), &postBuffer);
           
            RCF_ASSERT(
                postBufferRequested <= postBufferLen)
                (postBufferRequested)(postBufferLen);

            // NB: completion routine will call BIO_nwrite(io_bio, len)
            openSslEncryptionFilter.mpPostFilter->read(
                ByteBuffer(postBuffer, postBufferLen),
                postBufferRequested);
        }
        else
        {
            // move data from io_bio to network
            postState = Writing;
            postBufferRequested = static_cast<int>(BIO_ctrl_pending(io_bio.get()));
            postBufferLen = BIO_nread0(io_bio.get(), &postBuffer);
            // NB: completion routine will call BIO_nread(io_bio, postBufferLen)
            mByteBuffers.resize(0);
            mByteBuffers.push_back( ByteBuffer(postBuffer, postBufferLen));
            openSslEncryptionFilter.mpPostFilter->write(mByteBuffers);
            mByteBuffers.resize(0);
        }
    }

    void OpenSslEncryptionFilterImpl::onDataTransferred(std::size_t bytesTransferred)
    {
        // complete a data transfer, in the direction that was requested

        // TODO: assert that, on read, data was actually transferred into postBuffer
        // and not somewhere else

        RCF_ASSERT(bytesTransferred > 0);
        RCF_ASSERT(
            (postState == Reading && bytesTransferred <= postBufferRequested) ||
            (postState == Writing && bytesTransferred <= postBufferLen))
            (postState)(bytesTransferred)(postBufferRequested)(postBufferLen);

        if (postState == Reading)
        {
            // return value not documented
            BIO_nwrite(
                io_bio.get(),
                &postBuffer,
                static_cast<int>(bytesTransferred));

            postBuffer = 0;
            postBufferLen = 0;
            postState = Ready;
        }
        else if (postState == Writing)
        {
            // return value not documented
            BIO_nread(
                io_bio.get(),
                &postBuffer,
                static_cast<int>(bytesTransferred));

            postBuffer = 0;
            postBufferLen = 0;
            postState = Ready;
        }
    }

    void OpenSslEncryptionFilterImpl::reset()
    {
        init();
    }

    void OpenSslEncryptionFilterImpl::init()
    {
        // TODO: sort out any OpenSSL-dependent order of destruction issues

        ssl_bio = boost::shared_ptr<BIO>(
            BIO_new(BIO_f_ssl()),
            BIO_free);

        ssl_ctx = boost::shared_ptr<SSL_CTX>(
            SSL_CTX_new(SSLv23_method()),
            SSL_CTX_free);

        RCF_ASSERT(sslRole == SslServer || sslRole == SslClient)(sslRole);
        
        if (!certificateFile.empty())
        {
            loadCertificate(ssl_ctx, certificateFile, certificateFilePassword);
        }

        if (!caCertificate.empty())
        {
            loadCaCertificate(ssl_ctx, caCertificate);
        }

        ssl = boost::shared_ptr<SSL>(
            SSL_new(ssl_ctx.get()),
            SSL_free);

        if (sslRole == SslServer && !caCertificate.empty())
        {
            SSL_set_verify(ssl.get(), SSL_VERIFY_PEER, NULL);
        }

        BIO *bio_temp = NULL;
        BIO *io_bio_temp = NULL;
        BIO_new_bio_pair(&bio_temp, bioBufferSize, &io_bio_temp, bioBufferSize);
        bio = boost::shared_ptr<BIO>(
            bio_temp,
            BIO_free);
        io_bio = boost::shared_ptr<BIO>(
            io_bio_temp,
            BIO_free);

        RCF_ASSERT(sslRole == SslServer || sslRole == SslClient)(sslRole);
        sslRole == SslServer ?
            SSL_set_accept_state(ssl.get()) :
            SSL_set_connect_state(ssl.get());

        SSL_set_bio(ssl.get(), bio.get(), bio.get());
        BIO_set_ssl(ssl_bio.get(), ssl.get(), BIO_NOCLOSE);

        if (
            ssl_ctx.get() == NULL ||
            ssl.get() == NULL ||
            bio.get() == NULL ||
            io_bio.get() == NULL)
        {
            RCF_THROW(Exception(RcfError_OpenSslFilterInit))
                (getOpenSslErrors())(certificateFile);
        }

    }

    bool OpenSslEncryptionFilterImpl::loadCertificate(
        boost::shared_ptr<SSL_CTX> ctx,
        const std::string &file,
        const std::string &password)
    {
        RCF_ASSERT(ctx.get());
        if (1 == SSL_CTX_use_certificate_chain_file(ctx.get(), file.c_str()))
        {
            boost::shared_ptr<BIO> bio(
                BIO_new( BIO_s_file() ),
                BIO_free );
            if (bio.get())
            {
                if (1 == BIO_read_filename(bio.get(), file.c_str()))
                {
                    boost::shared_ptr<EVP_PKEY> evp(
                        PEM_read_bio_PrivateKey(
                            bio.get(),
                            NULL,
                            NULL,
                            (void *) password.c_str()),
                        EVP_PKEY_free);
                    if (evp.get())
                    {
                        if (1 == SSL_CTX_use_PrivateKey(ctx.get(), evp.get()))
                        {
                            return true;
                        }
                    }
                }
            }
        }
        RCF_THROW(
            Exception(RcfError_OpenSslLoadCert))
            (getOpenSslErrors())(file)(password);
        return false;
    }

    bool OpenSslEncryptionFilterImpl::loadCaCertificate(
        boost::shared_ptr<SSL_CTX> ctx,
        const std::string &file)
    {
        RCF_ASSERT(ctx.get());

        if (SSL_CTX_load_verify_locations(ctx.get(), file.c_str(), NULL) != 1)
        {
            RCF_THROW(
                Exception(RcfError_OpenSslLoadCert))
                (getOpenSslErrors())(file);
        }
        return true;
    }

    OpenSslEncryptionFilterFactory::OpenSslEncryptionFilterFactory(
        const std::string &certificateFile,
        const std::string &certificateFilePassword,
        const std::string &caCertificate,
        const std::string &ciphers,
        VerifyFunctor verifyFunctor,
        bool serverRole) :
            mCertificateFile(certificateFile),
            mCertificateFilePassword(certificateFilePassword),
            mCaCertificate(caCertificate),
            mCiphers(ciphers),
            mVerifyFunctor(verifyFunctor),
            mRole(serverRole ? SslServer : SslClient)
    {}

    FilterPtr OpenSslEncryptionFilterFactory::createFilter()
    {
        return FilterPtr( new OpenSslEncryptionFilter(
            mCertificateFile,
            mCertificateFilePassword,
            mCaCertificate,
            mCiphers,
            mVerifyFunctor,
            mRole));
    }

    const FilterDescription & OpenSslEncryptionFilterFactory::getFilterDescription()
    {
        return OpenSslEncryptionFilter::sGetFilterDescription();
    }

} // namespace RCF
