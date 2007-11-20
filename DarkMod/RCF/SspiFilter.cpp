
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#include <RCF/SspiFilter.hpp>

#include <boost/multi_index/detail/scope_guard.hpp>

#include <RCF/CurrentSession.hpp>
#include <RCF/InitDeinit.hpp>
#include <RCF/Tools.hpp>

// TODO: unit tests w/ and w/o unicode
#include <tchar.h>

#ifdef _UNICODE

#define INIT_SEC_INTERFACE_NAME       "InitSecurityInterfaceW"
typedef unsigned short UTCHAR;

#else

#define INIT_SEC_INTERFACE_NAME       "InitSecurityInterfaceA"
typedef unsigned char UTCHAR;

#endif

// spelling mistake in mingw headers!
#if defined(__MINGW32__) && __GNUC__ == 3 && __GNUC_MINOR__ <= 2
#define cbMaxSignature cbMaxSIgnature
#endif

// missing stuff in mingw headers
#ifdef __MINGW32__
#ifndef SEC_WINNT_AUTH_IDENTITY_VERSION
#define SEC_WINNT_AUTH_IDENTITY_VERSION 0x200

typedef struct _SEC_WINNT_AUTH_IDENTITY_EXW {
    unsigned long Version;
    unsigned long Length;
    unsigned short SEC_FAR *User;
    unsigned long UserLength;
    unsigned short SEC_FAR *Domain;
    unsigned long DomainLength;
    unsigned short SEC_FAR *Password;
    unsigned long PasswordLength;
    unsigned long Flags;
    unsigned short SEC_FAR * PackageList;
    unsigned long PackageListLength;
} SEC_WINNT_AUTH_IDENTITY_EXW, *PSEC_WINNT_AUTH_IDENTITY_EXW;

// end_ntifs

typedef struct _SEC_WINNT_AUTH_IDENTITY_EXA {
    unsigned long Version;
    unsigned long Length;
    unsigned char SEC_FAR *User;
    unsigned long UserLength;
    unsigned char SEC_FAR *Domain;
    unsigned long DomainLength;
    unsigned char SEC_FAR *Password;
    unsigned long PasswordLength;
    unsigned long Flags;
    unsigned char SEC_FAR * PackageList;
    unsigned long PackageListLength;
} SEC_WINNT_AUTH_IDENTITY_EXA, *PSEC_WINNT_AUTH_IDENTITY_EXA;

#ifdef UNICODE
#define SEC_WINNT_AUTH_IDENTITY_EX  SEC_WINNT_AUTH_IDENTITY_EXW    // ntifs
#define PSEC_WINNT_AUTH_IDENTITY_EX PSEC_WINNT_AUTH_IDENTITY_EXW   // ntifs
#else
#define SEC_WINNT_AUTH_IDENTITY_EX  SEC_WINNT_AUTH_IDENTITY_EXA
#endif

// begin_ntifs
#endif // SEC_WINNT_AUTH_IDENTITY_VERSION      

#ifdef __cplusplus
extern "C" {
#endif

    typedef enum
    {
        NameUnknown = 0,
        NameFullyQualifiedDN = 1,
        NameSamCompatible = 2,
        NameDisplay = 3,
        NameUniqueId = 6,
        NameCanonical = 7,
        NameUserPrincipal = 8,
        NameCanonicalEx = 9,
        NameServicePrincipal = 10,
        NameDnsDomain = 12
    } EXTENDED_NAME_FORMAT, * PEXTENDED_NAME_FORMAT ;

#ifdef __cplusplus
}
#endif

#endif // __MINGW__

namespace RCF {

    PSecurityFunctionTable getSecurityFunctionTable();

#ifdef UNICODE
    //LPCTSTR GetUserNameExName = _T("GetUserNameExW");
    LPCSTR GetUserNameExName = "GetUserNameExW";
#else
    LPCSTR GetUserNameExName = "GetUserNameExA";
#endif

    typedef BOOLEAN (WINAPI *PfnGetUserNameEx)(EXTENDED_NAME_FORMAT, LPTSTR, PULONG);
    HMODULE hModuleSecur32 = 0;
    PfnGetUserNameEx pfnGetUserNameEx = NULL;
   

    tstring getMyUserName()
    {
        std::vector<TCHAR> vec;
        DWORD len = 0;
        BOOL ret = GetUserName(NULL, &len);
        BOOL err = 0;
        vec.resize(len);
        ret = GetUserName(&vec[0], &len);
        err = GetLastError();
        RCF_VERIFY(
            ret,
            Exception(
                RcfError_Sspi,
                err,
                RcfSubsystem_Os,
                "GetUserName() failed"));
        return tstring(&vec[0]);
    }

    tstring getMyDomain()
    {
        if (pfnGetUserNameEx)
        {
            ULONG count = 0;
            pfnGetUserNameEx(NameSamCompatible, NULL, &count);
            std::vector<TCHAR> vec(count);
            BOOLEAN ok = pfnGetUserNameEx(NameSamCompatible, &vec[0], &count);
            DWORD dwErr = GetLastError();

            RCF_VERIFY(
                ok,
                Exception(
                RcfError_SspiCredentials,
                dwErr,
                RcfSubsystem_Os,
                "GetUserNameEx() failed"))(dwErr);

            tstring domainAndUser(&vec[0]);
            tstring domain = domainAndUser.substr(
                0,
                domainAndUser.find('\\'));
            return domain;
        }
        else
        {
            // GetUserNameEx() is not available on older Windows versions, so
            // here's the alternative.

            // This code may fail if we are impersonating another user, and our
            // Windows privileges aren't appropriately enabled. OpenThreadToken()
            // fails with "Access denied".

            using namespace boost::multi_index::detail;

            // obtain current token
            HANDLE hToken;
            BOOL ok = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, &hToken);
            DWORD dwErr1 = GetLastError();
            DWORD dwErr2 = 0;
            if (!ok)
            {
                ok = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken);
                dwErr2 = GetLastError();
            }

            RCF_VERIFY(
                ok,
                Exception(
                    RcfError_SspiCredentials,
                    dwErr2,
                    RcfSubsystem_Os,
                    "OpenProcessToken() failed"))(dwErr1)(dwErr2);

            scope_guard guard = make_guard(&CloseHandle, hToken);
            RCF_UNUSED_VARIABLE(guard);

            PTOKEN_USER ptiUser     = NULL;
            DWORD       cbti        = 0;

            // find the length of the token information buffer
            GetTokenInformation(hToken, TokenUser, NULL, 0, &cbti);

            // allocate buffer for token information
            std::vector<char> vec(cbti);
            ptiUser = (PTOKEN_USER) &vec[0];

            // obtain token information
            GetTokenInformation(hToken, TokenUser, ptiUser, cbti, &cbti);

            // extract domain and username
            TCHAR    szDomain[256];           
            DWORD    szDomainLen = sizeof(szDomain)/sizeof(szDomain[0]);

            TCHAR    szUsername[256];           
            DWORD    szUsernameLen = sizeof(szUsername)/sizeof(szUsername[0]);

            SID_NAME_USE snu;

            ok = LookupAccountSid(
                NULL, ptiUser->User.Sid,
                szUsername, &szUsernameLen,
                szDomain, &szDomainLen,
                &snu);
            DWORD err = GetLastError();
            RCF_VERIFY(
                ok,
                Exception(
                    RcfError_SspiCredentials,
                    err,
                    RcfSubsystem_Os,
                    "LookupAccountSid() failed"));

            return szDomain;
        }
    }

    tstring getMyMachineName()
    {
        const int BufferSize = MAX_COMPUTERNAME_LENGTH + 1;
        TCHAR buffer[BufferSize];
        DWORD dwSize = sizeof(buffer)/sizeof(buffer[0]);
        BOOL ok = GetComputerName(buffer, &dwSize);
        RCF_ASSERT(ok);
        return tstring(&buffer[0]);
    }

    SspiFilterBase::SspiFilterBase(
        const tstring &packageName,
        const tstring &packageList,
        bool server) :
            mPackageName(packageName),
            mPackageList(packageList),
            mQop(None),
            mContextRequirements(RCF_DEFAULT_INIT),
            mServer(server),
            mPreState(Ready),
            mBytesRequestedOrig(RCF_DEFAULT_INIT),
            mWriteBuffer(RCF_DEFAULT_INIT),
            mWriteBufferPos(RCF_DEFAULT_INIT),
            mWriteBufferLen(RCF_DEFAULT_INIT),
            mReadBuffer(RCF_DEFAULT_INIT),
            mReadBufferPos(RCF_DEFAULT_INIT),
            mReadBufferLen(RCF_DEFAULT_INIT),
            mPostState(Ready),
            mHaveContext(RCF_DEFAULT_INIT),
            mHaveCredentials(RCF_DEFAULT_INIT),
            mImplicitCredentials(true),
            mContext(),
            mCredentials(),
            mTarget(),
            mContextState(AuthContinue),
            mEvent(ReadIssued),
            mLimitRecursion(!server)
    {

#if defined(_MSC_VER) && _MSC_VER < 1310
        memset(&mContext, 0, sizeof(mContext));
        memset(&mCredentials, 0, sizeof(mCredentials));
#endif

        mPkgInfo.Name = NULL;
        mPkgInfo.Comment = NULL;

        init();
    }

    SspiFilterBase::SspiFilterBase(
        const tstring &target,
        QualityOfProtection qop,
        ULONG contextRequirements,
        const tstring &packageName,
        const tstring &packageList,
        bool server) :
            mPackageName(packageName),
            mPackageList(packageList),
            mQop(qop),
            mContextRequirements(contextRequirements),
            mServer(server),
            mPreState(Ready),
            mBytesRequestedOrig(RCF_DEFAULT_INIT),
            mWriteBuffer(RCF_DEFAULT_INIT),
            mWriteBufferPos(RCF_DEFAULT_INIT),
            mWriteBufferLen(RCF_DEFAULT_INIT),
            mReadBuffer(RCF_DEFAULT_INIT),
            mReadBufferPos(RCF_DEFAULT_INIT),
            mReadBufferLen(RCF_DEFAULT_INIT),
            mPostState(Ready),
            mHaveContext(RCF_DEFAULT_INIT),
            mHaveCredentials(RCF_DEFAULT_INIT),
            mImplicitCredentials(true),
            mContext(),
            mCredentials(),
            mTarget(target),
            mContextState(AuthContinue),
            mEvent(ReadIssued),
            mLimitRecursion(!server)
    {

#if defined(_MSC_VER) && _MSC_VER < 1310
        memset(&mContext, 0, sizeof(mContext));
        memset(&mCredentials, 0, sizeof(mCredentials));
#endif

        mPkgInfo.Name = NULL;
        mPkgInfo.Comment = NULL;

        init();
    }

    // client mode ctor, accessible to the public
    SspiFilterBase::SspiFilterBase(
        const tstring &userName,
        const tstring &password,
        const tstring &domain,
        const tstring &target,
        QualityOfProtection qop,
        ULONG contextRequirements,
        const tstring &packageName,
        const tstring &packageList,
        bool server) :
            mPackageName(packageName),
            mPackageList(packageList),
            mQop(qop),
            mContextRequirements(contextRequirements),
            mServer(server),
            mPreState(Ready),
            mBytesRequestedOrig(RCF_DEFAULT_INIT),
            mWriteBuffer(RCF_DEFAULT_INIT),
            mWriteBufferPos(RCF_DEFAULT_INIT),
            mWriteBufferLen(RCF_DEFAULT_INIT),
            mReadBuffer(RCF_DEFAULT_INIT),
            mReadBufferPos(RCF_DEFAULT_INIT),
            mReadBufferLen(RCF_DEFAULT_INIT),
            mPostState(Ready),
            mHaveContext(RCF_DEFAULT_INIT),
            mHaveCredentials(RCF_DEFAULT_INIT),
            mImplicitCredentials(RCF_DEFAULT_INIT),
            mContext(),
            mCredentials(),
            mTarget(target),
            mContextState(AuthContinue),
            mEvent(ReadIssued),
            mLimitRecursion(!server)
    {

#if defined(_MSC_VER) && _MSC_VER < 1310
        memset(&mContext, 0, sizeof(mContext));
        memset(&mCredentials, 0, sizeof(mCredentials));
#endif

        mPkgInfo.Name = NULL;
        mPkgInfo.Comment = NULL;

        acquireCredentials(userName, password, domain);
        init();
    }

    SspiFilterBase::~SspiFilterBase()
    {
        RCF_DTOR_BEGIN
            deinit();
            freeCredentials();
        RCF_DTOR_END
    }

#if defined(_MSC_VER) && _MSC_VER == 1200
#define FreeCredentialsHandle FreeCredentialHandle
#endif

    void SspiFilterBase::freeCredentials()
    {
        if (mHaveCredentials)
        {
            SECURITY_STATUS status = 0;
            status = getSecurityFunctionTable()->FreeCredentialsHandle(&mCredentials);
            RCF_VERIFY(
                status == SEC_E_OK || status == SEC_E_INVALID_HANDLE,
                FilterException(
                    RcfError_Sspi,
                    status,
                    RcfSubsystem_Os,
                    "FreeCredentialsHandle() failed"));
        }

        if (mPkgInfo.Name)
        {
            delete [] mPkgInfo.Name;
        }

        if (mPkgInfo.Comment)
        {
            delete [] mPkgInfo.Comment;
        }

    }

#if defined(_MSC_VER) && _MSC_VER == 1200
#undef FreeCredentialsHandle
#endif

    void SspiFilterBase::reset()
    {
        init();
    }

    void SspiFilterBase::deinit()
    {
        if (mHaveContext)
        {
            SECURITY_STATUS status = 0;       
            status = getSecurityFunctionTable()->DeleteSecurityContext(&mContext);
            RCF_VERIFY(
                status == SEC_E_OK || status == SEC_E_INVALID_HANDLE,
                FilterException(
                    RcfError_Sspi,
                    status,
                    RcfSubsystem_Os,
                    "DeleteSecurityContext() failed"));
            mHaveContext = false;
        }
    }

    void SspiFilterBase::init()
    {
        deinit();

        mPreState = Ready;
        mPostState = Ready;
        mContextState = AuthContinue;
        mEvent = ReadIssued;

        resizeReadBuffer(0);
        resizeWriteBuffer(0);
    }

    void SspiFilterBase::read(
        const ByteBuffer &byteBuffer,
        std::size_t bytesRequested)
    {
        mReadByteBufferOrig = byteBuffer;
        mBytesRequestedOrig = bytesRequested;
        mPreState = Reading;
        handleEvent(ReadIssued);
    }

    void SspiFilterBase::write(const std::vector<ByteBuffer> &byteBuffers)
    {
        // TODO: write many buffers in one go
        mWriteByteBufferOrig = byteBuffers.front();
        mPreState = Writing;
        handleEvent(WriteIssued);
    }

    void SspiFilterBase::onReadCompleted_(
        const ByteBuffer &byteBuffer,
        int error)
    {
        RCF_ASSERT(!error)(error);

        RCF_ASSERT(
            mReadBuffer + mReadBufferPos == byteBuffer.getPtr())
            (mReadBuffer)(mReadBufferPos)(byteBuffer.getPtr());

        mReadBufferPos += byteBuffer.getLength();

        RCF_ASSERT(
            mReadBufferPos <= mReadBufferLen)
            (mReadBufferPos)(mReadBufferLen);

        // TODO: this is not so cool
        const_cast<ByteBuffer &>(byteBuffer).clear();
        handleEvent(ReadCompleted);
    }

    // Recursion limiter can only be used on synchronous filter stacks, and
    // avoids excessive recursion when reading or writing data in small pieces.
    // On asynchronous filter stacks, it would introduce a race condition by setting
    // filter state _after_ invoking downstream async read/write operations.
    void SspiFilterBase::onReadCompleted(
        const ByteBuffer &byteBuffer,
        int error)
    {
        if (mLimitRecursion)
        {
            applyRecursionLimiter(
                mRecursionStateRead,
                &SspiFilterBase::onReadCompleted_,
                byteBuffer,
                error);
        }
        else
        {
            onReadCompleted_(byteBuffer, error);
        }
    }

    void SspiFilterBase::onWriteCompleted_(
        std::size_t bytesTransferred,
        int error)
    {
        RCF_ASSERT(!error);

        mByteBuffers.resize(0);
        mWriteBufferPos += bytesTransferred;
       
        RCF_ASSERT(
            mWriteBufferPos <= mWriteBufferLen)
            (mWriteBufferPos)(mWriteBufferLen);

        handleEvent(WriteCompleted);
    }

    void SspiFilterBase::onWriteCompleted(
        std::size_t bytesTransferred,
        int error)
    {
        if (mLimitRecursion)
        {
            applyRecursionLimiter(
                mRecursionStateWrite,
                &SspiFilterBase::onWriteCompleted_,
                bytesTransferred,
                error);
        }
        else
        {
            onWriteCompleted_(bytesTransferred, error);
        }
    }

    void SspiFilterBase::handleEvent(Event event)
    {
        RCF_ASSERT(
            event == ReadIssued || event == WriteIssued ||
            event == ReadCompleted || event == WriteCompleted)(event);

        mEvent = event;
        if ((mEvent == ReadIssued || mEvent == WriteIssued) || completeBlock())
        {
            if (mContextState != AuthOkAck)
            {
                handleHandshakeEvent();
            }
            else
            {
                switch (mEvent)
                {
                case ReadIssued:

                    if (0 < mReadBufferPos && mReadBufferPos < mReadBufferLen)
                    {
                        // read from currently decrypted  block
                        std::size_t bytesAvail = mReadBufferLen - mReadBufferPos;

                        std::size_t bytesToRead =
                            RCF_MIN(bytesAvail, mBytesRequestedOrig);

                        ByteBuffer byteBuffer;
                        if (mReadByteBufferOrig.getLength() > 0)
                        {
                            memcpy(
                                mReadByteBufferOrig.getPtr(),
                                mReadBuffer+mReadBufferPos,
                                bytesToRead);

                            byteBuffer = ByteBuffer(
                                mReadByteBufferOrig,
                                0,
                                bytesToRead);
                        }
                        else
                        {
                            byteBuffer = ByteBuffer(
                                mReadByteBuffer,
                                mReadBufferPos,
                                bytesToRead);
                        }
                       
                        mReadBufferPos += bytesToRead;
                        mReadByteBufferOrig = ByteBuffer();
                        mpPreFilter->onReadCompleted(byteBuffer, 0);
                    }
                    else
                    {
                        // read in a new block
                        resizeReadBuffer(4);
                        readBuffer();
                    }
                    break;

                case WriteIssued:

                    encryptWriteBuffer();
                    writeBuffer();
                    break;

                case ReadCompleted:

                    decryptReadBuffer();
                    handleEvent(ReadIssued);
                    break;

                case WriteCompleted:

                    {
                        std::size_t bytesTransferred =
                            mWriteByteBufferOrig.getLength();

                        mWriteByteBufferOrig = ByteBuffer();
                        mpPreFilter->onWriteCompleted(bytesTransferred, 0);
                    }
                   
                    break;

                default:
                    RCF_ASSERT(0);
                }
            }
        }
    }

    void SspiFilterBase::readBuffer()
    {
        RCF_ASSERT(
            0 <= mReadBufferPos && mReadBufferPos <= mReadBufferLen)
            (mReadBufferPos)(mReadBufferLen);

        mPostState = Reading;
        mTempByteBuffer = ByteBuffer(mReadByteBuffer, mReadBufferPos);
        mpPostFilter->read(mTempByteBuffer, mReadBufferLen-mReadBufferPos);
    }

    void SspiFilterBase::writeBuffer()
    {
        RCF_ASSERT(
            0 <= mWriteBufferPos && mWriteBufferPos <= mWriteBufferLen)
            (mWriteBufferPos)(mWriteBufferLen);

        mPostState = Writing;
       
        mByteBuffers.resize(0);
        mByteBuffers.push_back( ByteBuffer(mWriteByteBuffer, mWriteBufferPos));
        mpPostFilter->write(mByteBuffers);
    }

    bool SspiFilterBase::completeReadBlock()
    {
        RCF_ASSERT(
            0 <= mReadBufferPos && mReadBufferPos <= mReadBufferLen )
            (mReadBufferPos)(mReadBufferLen);

        if (mReadBufferPos == mReadBufferLen && mReadBufferLen == 4)
        {
            // read the 4 byte length field, now read the rest of the block
            BOOST_STATIC_ASSERT( sizeof(unsigned int) == 4 );
            BOOST_STATIC_ASSERT( sizeof(DWORD) == 4 );

            // TODO: sanity check on len
            unsigned int len = * (unsigned int *) mReadBuffer;
            bool integrity = (len & (1<<30)) ? true : false;
            bool encryption = (len & (1<<31)) ? true : false;
            len = len & ~(1<<30);
            len = len & ~(1<<31);
            * (unsigned int *) mReadBuffer = len;
            // TODO: literal
            RCF_VERIFY(
                !(integrity && encryption),
                FilterException(RcfError_Sspi, "both integrity and encryption requested"));
            if (mServer)
            {
                if (integrity)
                {
                    mQop = Integrity;
                }
                else if (encryption)
                {
                    mQop = Encryption;
                }
                else
                {
                    mQop = None;
                }
            }

            resizeReadBuffer(4+len);
            mReadBufferPos = 4;
            readBuffer();
            return false;
        }

        return (mReadBufferPos < mReadBufferLen) ?
            readBuffer(), false :
            true;
    }

    bool SspiFilterBase::completeWriteBlock()
    {
        RCF_ASSERT(
            0 <= mWriteBufferPos && mWriteBufferPos <= mWriteBufferLen )
            (mWriteBufferPos)(mWriteBufferLen);

        return (mWriteBufferPos < mWriteBufferLen) ?
            writeBuffer(), false :
            true;
    }

    bool SspiFilterBase::completeBlock()
    {
        // check to see if a whole block was read or written
        // if not, issue another read or write
        RCF_ASSERT(
            mPostState == Reading || mPostState == Writing )
            (mPostState);

        return
            mPostState == Reading ?
                completeReadBlock() :
                completeWriteBlock();
    }

    void SspiFilterBase::resizeReadBuffer(std::size_t newSize)
    {
        // TODO: optimize up and down sizing of vector

        mTempByteBuffer.clear();
        mReadByteBuffer.clear();
        if (!mReadBufferVectorPtr || !mReadBufferVectorPtr.unique())
        {
            boost::shared_ptr<std::vector<char> > vecPtr(mReadBufferVectorPtr);
            (vecPtr.get() && !vecPtr->empty()) ?
                mReadBufferVectorPtr.reset(new std::vector<char>(*vecPtr)) :
                mReadBufferVectorPtr.reset(new std::vector<char>());
        }

        std::size_t newSize_ = newSize == 0 ? 1 : newSize;
        mReadBufferVectorPtr->resize(newSize_);
        mReadByteBuffer = ByteBuffer(mReadBufferVectorPtr);
        mReadBuffer = mReadByteBuffer.getPtr();
        mReadBufferPos = 0;
        mReadBufferLen = mReadByteBuffer.getLength();
        mReadBufferLen = (mReadBufferLen == 1) ? 0 : mReadBufferLen;

        RCF_ASSERT(mReadBufferLen == newSize)(mReadBufferLen)(newSize);
    }

    void SspiFilterBase::resizeWriteBuffer(std::size_t newSize)
    {
        // TODO: optimize up and down sizing of vector

        mWriteByteBuffer.clear();
        if (!mWriteBufferVectorPtr || !mWriteBufferVectorPtr.unique())
        {
            boost::shared_ptr<std::vector<char> > vecPtr(mWriteBufferVectorPtr);
            (vecPtr.get() && !vecPtr->empty()) ?
                mWriteBufferVectorPtr.reset(new std::vector<char>(*vecPtr)) :
                mWriteBufferVectorPtr.reset(new std::vector<char>());
        }

        std::size_t newSize_ = newSize == 0 ? 1 : newSize;
        mWriteBufferVectorPtr->resize(newSize_);
        mWriteByteBuffer = ByteBuffer(mWriteBufferVectorPtr);
        mWriteBuffer = mWriteByteBuffer.getPtr();
        mWriteBufferPos = 0;
        mWriteBufferLen = mWriteByteBuffer.getLength();
        mWriteBufferLen = mWriteBufferLen == 1 ? 0 : mWriteBufferLen;
        RCF_ASSERT(mWriteBufferLen == newSize)(mWriteBufferLen)(newSize);
    }

    void SspiFilterBase::encryptWriteBuffer()
    {
        // encrypt the pre buffer to the write buffer

        RCF_ASSERT(mContextState == AuthOkAck)(mContextState);

        if (mQop == Integrity)
        {
            SecPkgContext_Sizes sizes;
            getSecurityFunctionTable()->QueryContextAttributes(
                &mContext,
                SECPKG_ATTR_SIZES,
                &sizes);

            DWORD cbPacketLength    = 4;
            DWORD cbMsgLength       = 4;
            DWORD cbMsg             = static_cast<DWORD>(mWriteByteBufferOrig.getLength());
            DWORD cbSignature       = sizes.cbMaxSignature;
            DWORD cbPacket            = cbMsgLength + cbMsg + cbSignature;

            resizeWriteBuffer(cbPacketLength + cbPacket);
            memcpy(mWriteBuffer+cbPacketLength, &cbMsg, cbMsgLength);
            memcpy(
                mWriteBuffer+cbPacketLength+cbMsgLength,
                mWriteByteBufferOrig.getPtr(),
                mWriteByteBufferOrig.getLength());

            char *pMsg              = &mWriteBuffer[4];
            SecBuffer rgsb[2]       = {0,0};
            rgsb[0].cbBuffer        = cbMsg;
            rgsb[0].BufferType      = SECBUFFER_DATA;
            rgsb[0].pvBuffer        = pMsg + cbMsgLength;
            rgsb[1].cbBuffer        = cbSignature;
            rgsb[1].BufferType      = SECBUFFER_TOKEN;
            rgsb[1].pvBuffer        = pMsg + cbMsgLength + cbMsg;
            SecBufferDesc sbd       = {0};
            sbd.ulVersion           = SECBUFFER_VERSION;
            sbd.cBuffers            = sizeof(rgsb)/sizeof(*rgsb);
            sbd.pBuffers            = rgsb;

            SECURITY_STATUS status = getSecurityFunctionTable()->MakeSignature(
                &mContext,
                0,
                &sbd,
                0);

            RCF_VERIFY(
                status == SEC_E_OK,
                FilterException(
                    RcfError_SspiEncrypt,
                    status,
                    RcfSubsystem_Os,
                    "MakeSignature() failed"))(status);

            cbSignature                = rgsb[1].cbBuffer;
            cbPacket                = cbMsgLength + cbMsg + cbSignature;
            resizeWriteBuffer(cbPacketLength + cbPacket);
            DWORD encodedLength        = cbPacket;
            RCF_ASSERT(encodedLength < (1<<30))(encodedLength);
            encodedLength            = encodedLength | (1<<30);
            * (DWORD*) mWriteBuffer = encodedLength;
        }
        else if (mQop == Encryption)
        {
            SecPkgContext_Sizes sizes;
            getSecurityFunctionTable()->QueryContextAttributes(
                &mContext,
                SECPKG_ATTR_SIZES,
                &sizes);

            DWORD cbPacketLength    = 4;
            DWORD cbMsgLength       = 4;
            DWORD cbMsg             = static_cast<DWORD>(mWriteByteBufferOrig.getLength());
            DWORD cbTrailer         = sizes.cbSecurityTrailer;
            DWORD cbPacket            = cbMsgLength + cbMsg + cbTrailer;

            resizeWriteBuffer(cbPacketLength + cbPacket);
            memcpy(mWriteBuffer+cbPacketLength, &cbMsg, cbMsgLength);
            memcpy(
                mWriteBuffer+cbPacketLength+cbMsgLength,
                mWriteByteBufferOrig.getPtr(),
                mWriteByteBufferOrig.getLength());

            BYTE *pEncryptedMsg     =((BYTE *) mWriteBuffer) + 4;
            SecBuffer rgsb[2]       = {0,0};
            rgsb[0].cbBuffer        = cbMsg;
            rgsb[0].BufferType      = SECBUFFER_DATA;
            rgsb[0].pvBuffer        = pEncryptedMsg + cbMsgLength;
            rgsb[1].cbBuffer        = cbTrailer;
            rgsb[1].BufferType      = SECBUFFER_TOKEN;
            rgsb[1].pvBuffer        = pEncryptedMsg + cbMsgLength + cbMsg;
            SecBufferDesc sbd       = {0};
            sbd.ulVersion           = SECBUFFER_VERSION;
            sbd.cBuffers            = sizeof(rgsb)/sizeof(*rgsb);
            sbd.pBuffers            = rgsb;

            SECURITY_STATUS status = getSecurityFunctionTable()->EncryptMessage(
                &mContext,
                0,
                &sbd,
                0);

            RCF_VERIFY(
                status == SEC_E_OK,
                FilterException(
                    RcfError_SspiEncrypt,
                    status,
                    RcfSubsystem_Os,
                    "EncryptMessage() failed"))(status);

            cbTrailer                = rgsb[1].cbBuffer;
            cbPacket                = cbMsgLength + cbMsg + cbTrailer;
            resizeWriteBuffer(cbPacketLength + cbPacket);
            DWORD encodedLength        = cbPacket;
            RCF_ASSERT(encodedLength < (1<<30))(encodedLength);
            encodedLength            = encodedLength | (1<<31);
            * (DWORD*) mWriteBuffer = encodedLength;
        }
        else
        {
            RCF_ASSERT(mQop == None)(mQop);
            RCF_ASSERT(
                mWriteByteBufferOrig.getLength() < (1<<31))
                (mWriteByteBufferOrig.getLength());

            resizeWriteBuffer(mWriteByteBufferOrig.getLength()+4);
            memcpy(
                mWriteBuffer+4,
                mWriteByteBufferOrig.getPtr(),
                mWriteByteBufferOrig.getLength());

            DWORD dw = static_cast<DWORD>(mWriteByteBufferOrig.getLength());
            *(DWORD*) mWriteBuffer = dw;
        }
    }

    void SspiFilterBase::decryptReadBuffer()
    {
        // decrypt read buffer in place

        RCF_ASSERT(mContextState == AuthOkAck)(mContextState);

        if (mQop == Integrity)
        {
            BYTE *pMsg              = ((BYTE *) mReadBuffer) + 4;
            DWORD cbPacketLength    = 4;
            DWORD cbPacket          = *(DWORD*) mReadBuffer;
            DWORD cbMsgLength       = 4;
            DWORD cbMsg             = *(DWORD*) pMsg;
            DWORD cbSignature       = cbPacket - cbMsgLength - cbMsg;
            SecBuffer rgsb[2]       = {0,0};
            rgsb[0].cbBuffer        = cbMsg;
            rgsb[0].BufferType      = SECBUFFER_DATA;
            rgsb[0].pvBuffer        = pMsg + cbMsgLength;
            rgsb[1].cbBuffer        = cbSignature;
            rgsb[1].BufferType      = SECBUFFER_TOKEN;
            rgsb[1].pvBuffer        = pMsg + cbMsgLength + cbMsg;
            SecBufferDesc sbd       = {0};
            sbd.ulVersion           = SECBUFFER_VERSION;
            sbd.cBuffers            = sizeof(rgsb)/sizeof(*rgsb);
            sbd.pBuffers            = rgsb;
            ULONG qop               = 0;
            SECURITY_STATUS status  = getSecurityFunctionTable()->VerifySignature(
                &mContext,
                &sbd,
                0,
                &qop);

            RCF_VERIFY(
                status == SEC_E_OK,
                FilterException(
                    RcfError_SspiDecrypt,
                    status,
                    RcfSubsystem_Os,
                    "VerifySignature() failed"))(status);

            resizeReadBuffer(cbPacketLength + cbMsgLength + cbMsg);
            mReadBufferPos          = cbPacketLength + cbMsgLength;
        }
        else if (mQop == Encryption)
        {
            BYTE *pMsg              = ((BYTE *) mReadBuffer) + 4;
            DWORD cbPacketLength    = 4;
            DWORD cbPacket          = *(DWORD*)mReadBuffer;
            DWORD cbMsgLength       = 4;
            DWORD cbMsg             = *(DWORD*) pMsg;
            DWORD cbTrailer         = (cbPacket - cbMsgLength) - cbMsg;
            SecBuffer rgsb[2]       = {0,0};
            rgsb[0].cbBuffer        = cbMsg;
            rgsb[0].BufferType      = SECBUFFER_DATA;
            rgsb[0].pvBuffer        = pMsg + cbMsgLength;
            rgsb[1].cbBuffer        = cbTrailer;
            rgsb[1].BufferType      = SECBUFFER_TOKEN;
            rgsb[1].pvBuffer        = pMsg + cbMsgLength + cbMsg;
            SecBufferDesc sbd       = {0};
            sbd.ulVersion           = SECBUFFER_VERSION;
            sbd.cBuffers            = sizeof(rgsb)/sizeof(*rgsb);
            sbd.pBuffers            = rgsb;
            ULONG qop               = 0;

            SECURITY_STATUS status  = getSecurityFunctionTable()->DecryptMessage(
                &mContext,
                &sbd,
                0,
                &qop);

            RCF_VERIFY(
                status == SEC_E_OK,
                FilterException(
                    RcfError_SspiDecrypt,
                    status,
                    RcfSubsystem_Os,
                    "DecryptMessage() failed"))(status);

            resizeReadBuffer(cbPacketLength + cbMsgLength + cbMsg);
            mReadBufferPos          = cbPacketLength + cbMsgLength;
        }
        else
        {
            RCF_ASSERT(mQop == None)(mQop);
            mReadBufferPos = 4;
        }
    }

    void SspiFilterBase::resumeUserIo()
    {
        RCF_ASSERT( mPreState == Reading || mPreState == Writing )(mPreState);
        handleEvent( mPreState == Reading ? ReadIssued : WriteIssued );
    }

    SspiImpersonator::SspiImpersonator(SspiFilterBasePtr sspiFilterPtr) :
        mSspiFilterPtr(sspiFilterPtr)
    {
    }

    SspiImpersonator::~SspiImpersonator()
    {
        RCF_DTOR_BEGIN
            revertToSelf();
        RCF_DTOR_END
    }

    bool SspiImpersonator::impersonate()
    {
        if (mSspiFilterPtr)
        {
            RCF_ASSERT(
                mSspiFilterPtr->mContextState == SspiFilterBase::AuthOkAck )
                (mSspiFilterPtr->mContextState);

            SECURITY_STATUS status = mSspiFilterPtr->getSecurityFunctionTable()
                ->ImpersonateSecurityContext(&mSspiFilterPtr->mContext);

            RCF_VERIFY(
                status == SEC_E_OK,
                FilterException(
                    RcfError_SspiImpersonation, status, RcfSubsystem_Os,
                    "ImpersonateSecurityContext() failed"))(status);

            return true;
        }
        else
        {
            return false;
        }
    }

    void SspiImpersonator::revertToSelf() const
    {
        if (mSspiFilterPtr)
        {
            RCF_ASSERT( mSspiFilterPtr->mContextState == SspiFilterBase::AuthOkAck );
            SECURITY_STATUS status = mSspiFilterPtr->getSecurityFunctionTable()
                ->RevertSecurityContext(&mSspiFilterPtr->mContext);

            RCF_VERIFY(
                status == SEC_E_OK,
                FilterException(
                    RcfError_SspiImpersonation, status, RcfSubsystem_Os,
                    "RevertSecurityContext() failed"));
        }
    }
   
    bool SspiServerFilter::doHandshake()
    {
        // use the block in the read buffer to proceed through the handshake procedure

        // lazy acquiring of implicit credentials
        if (mImplicitCredentials && !mHaveCredentials)
        {
            acquireCredentials();
        }

        DWORD cbPacket          = mPkgInfo.cbMaxToken;
        DWORD cbPacketLength    = 4;

        std::vector<char> vec(cbPacketLength + cbPacket);

        BYTE *pPacket           = (BYTE*) &vec[0];
        SecBuffer ob            = {0};
        ob.BufferType           = SECBUFFER_TOKEN;
        ob.cbBuffer             = cbPacket;
        ob.pvBuffer             = pPacket+cbPacketLength;
        SecBufferDesc obd       = {0};
        obd.cBuffers            = 1;
        obd.ulVersion           = SECBUFFER_VERSION;
        obd.pBuffers            = &ob;

        RCF_ASSERT(
            mReadBufferLen == 0 || mReadBufferLen > 4)
            (mReadBufferLen);

        RCF_ASSERT(
            !mServer || (mServer && mReadBufferLen > 4))
            (mServer)(mReadBufferLen);

        SecBufferDesc ibd       = {0};
        SecBuffer ib            = {0};
        if (mReadBufferLen > 4)
        {
            ib.BufferType       = SECBUFFER_TOKEN;
            ib.cbBuffer         = *(DWORD *)mReadBuffer;
            ib.pvBuffer         = mReadBuffer+cbPacketLength;
            ibd.cBuffers        = 1;
            ibd.ulVersion       = SECBUFFER_VERSION;
            ibd.pBuffers        = &ib;
        }

        DWORD   CtxtAttr        = 0;
        TimeStamp Expiration    = {0};
        SECURITY_STATUS status  = getSecurityFunctionTable()->AcceptSecurityContext(
            &mCredentials,
            mHaveContext ? &mContext : NULL,
            &ibd,
            mContextRequirements,
            SECURITY_NATIVE_DREP,
            &mContext,
            &obd,
            &CtxtAttr,
            &Expiration);

        switch (status)
        {
        case SEC_E_OK:
        case SEC_I_CONTINUE_NEEDED:
        case SEC_I_COMPLETE_NEEDED:
        case SEC_I_COMPLETE_AND_CONTINUE:
        case SEC_E_INCOMPLETE_MESSAGE:
            mHaveContext = true;
            break;
        default:
            break;
        }

        cbPacket = ob.cbBuffer;

        // We only support NTLM, Kerberos and Negotiate SSP's, so there's never
        // a need to call CompleteAuthToken()
        RCF_ASSERT(
            status != SEC_I_COMPLETE_AND_CONTINUE &&
            status != SEC_I_COMPLETE_NEEDED)
            (status);

        if (status == SEC_I_CONTINUE_NEEDED)
        {
            // authorization ok so far, copy outbound data to write buffer
            mContextState = AuthContinue;
            *(DWORD *) pPacket = cbPacket;
            resizeWriteBuffer(cbPacketLength + cbPacket);
            memcpy(mWriteBuffer, pPacket, cbPacketLength + cbPacket);
        }
        else if (status == SEC_E_OK)
        {
            // authorization ok, send a special block of our own to notify client
            mContextState = AuthOk;
            if (cbPacket > 0)
            {
                *(DWORD *) pPacket = cbPacket;
                resizeWriteBuffer(cbPacketLength + cbPacket);
                memcpy(mWriteBuffer, pPacket, cbPacketLength + cbPacket);
            }
            else
            {
                resizeWriteBuffer(4+4+4);
                *(DWORD*) mWriteBuffer = 8;
                *(DWORD*) (mWriteBuffer+4) = RcfError_Ok;
                *(DWORD*) (mWriteBuffer+8) = 0;
            }
        }
        else
        {
            // authorization failed, send a special block of our own to notify client
            mContextState = AuthFailed;
            resizeWriteBuffer(4+4+4);
            *(DWORD*) mWriteBuffer = 8;
            *(DWORD*) (mWriteBuffer+4) = RcfError_SspiAuthFail;
            *(DWORD*) (mWriteBuffer+8) = status;
        }

        return true;
    }

    void SspiServerFilter::handleHandshakeEvent()
    {
        // take another step through the handshake process

        switch (mEvent)
        {
        case ReadIssued:
        case WriteIssued:

            // read first block from client
            RCF_ASSERT(mEvent == ReadIssued)(mEvent);
            resizeReadBuffer(4);
            readBuffer();
            break;

        case ReadCompleted:
           
            // process inbound block and write outbound block
            doHandshake();
            writeBuffer();
            break;

        case WriteCompleted:

            switch (mContextState)
            {
            case AuthOk:
                mContextState = AuthOkAck;
                resumeUserIo();
                break;

            case AuthFailed:
                RCF_THROW(FilterException(RcfError_SspiAuthFail));
                break;

            default:
                resizeReadBuffer(4);
                readBuffer();
            }
            break;
        default:
            RCF_ASSERT(0);
        }
    }

    bool SspiClientFilter::doHandshake()
    {
        // use the block in the read buffer to proceed through the handshake procedure

        // lazy acquiring of implicit credentials
        if (mImplicitCredentials && !mHaveCredentials)
        {
            acquireCredentials();
        }

        if (mContextState == AuthOk)
        {
            if (mReadBufferLen == 12)
            {
                DWORD rcfErr = *(DWORD*) &mReadBuffer[4];
                DWORD osErr = *(DWORD*) &mReadBuffer[8];
                if (rcfErr == RcfError_Ok)
                {
                    mContextState = AuthOkAck;
                    resumeUserIo();
                    return false;
                }
                else
                {
                    RCF_THROW(RemoteException(rcfErr, osErr, RcfSubsystem_Os));
                }
            }
            else
            {
                RCF_THROW(Exception(RcfError_SspiAuthFail));
            }
        }
       
        DWORD cbPacketLength    = 4;
        DWORD cbPacket          = mPkgInfo.cbMaxToken;
        std::vector<char> vec(cbPacket + cbPacketLength);

        BYTE *pPacket           = (BYTE*) &vec[0];
        SecBuffer ob            = {0};
        ob.BufferType           = SECBUFFER_TOKEN;
        ob.cbBuffer             = cbPacket;
        ob.pvBuffer             = pPacket + cbPacketLength;
        SecBufferDesc obd       = {0};
        obd.cBuffers            = 1;
        obd.ulVersion           = SECBUFFER_VERSION;
        obd.pBuffers            = &ob;

        RCF_ASSERT(
            mReadBufferLen == 0 || mReadBufferLen > 4)
            (mReadBufferLen);

        RCF_ASSERT(
            !mServer || (mServer && mReadBufferLen > 4))
            (mServer)(mReadBufferLen);

        SecBuffer ib            = {0};
        SecBufferDesc ibd       = {0};

        if (mReadBufferLen > 4)
        {
            ib.BufferType       = SECBUFFER_TOKEN;
            ib.cbBuffer         = *(DWORD *) mReadBuffer;
            ib.pvBuffer         = mReadBuffer + cbPacketLength;
            ibd.cBuffers        = 1;
            ibd.ulVersion       = SECBUFFER_VERSION;
            ibd.pBuffers        = &ib;
        }

        const TCHAR *target = mTarget.empty() ? _T("") : mTarget.c_str();

        DWORD CtxtAttr          = 0;
        TimeStamp Expiration    = {0};
        ULONG CtxtReq =  mContextRequirements;

        SECURITY_STATUS status  = getSecurityFunctionTable()->InitializeSecurityContext(
            &mCredentials,
            mHaveContext ? &mContext : NULL,
            (TCHAR *) target,
            CtxtReq,
            0,
            SECURITY_NATIVE_DREP,
            (mHaveContext && mReadBufferLen > 4) ? &ibd : NULL,
            0,
            &mContext,
            &obd,
            &CtxtAttr,
            &Expiration);

        switch (status)
        {
        case SEC_E_OK:
        case SEC_I_CONTINUE_NEEDED:
        case SEC_I_COMPLETE_NEEDED:
        case SEC_I_COMPLETE_AND_CONTINUE:
        case SEC_E_INCOMPLETE_MESSAGE:
        case SEC_I_INCOMPLETE_CREDENTIALS:
            mHaveContext = true;
            break;
        default:
            break;
        }

        RCF_ASSERT(
            status != SEC_I_COMPLETE_NEEDED &&
            status != SEC_I_COMPLETE_AND_CONTINUE)
            (status);
       
        cbPacket                = ob.cbBuffer;
        if (cbPacket > 0)
        {
            *(DWORD *)pPacket   = cbPacket;
            mContextState       =
                (status == SEC_E_OK) ?
                    AuthOk :
                    (status == SEC_I_CONTINUE_NEEDED) ?
                        AuthContinue :
                        AuthFailed;

            RCF_VERIFY(
                mContextState != AuthFailed,
                Exception(
                    RcfError_SspiAuthFail,
                    status,
                    RcfSubsystem_Os,
                    "InitializeSecurityContext() failed"))(status);

            resizeWriteBuffer(cbPacketLength + cbPacket);
            memcpy(mWriteBuffer, pPacket, cbPacketLength + cbPacket);
            return true;
        }
        else
        {
            mContextState = AuthOkAck;
            resumeUserIo();
            return false;
        }
    }

    void SspiClientFilter::handleHandshakeEvent()
    {
        // take another step through the handshake process

        switch (mEvent)
        {
        case ReadIssued:
        case WriteIssued:
           
            // create first block to send to server
            //resizeReadBuffer(0);
            doHandshake();
            writeBuffer();
            break;

        case ReadCompleted:

            // process a block, and send any emitted output block
            if (doHandshake())
            {
                writeBuffer();
            }
            break;

        case WriteCompleted:

            // issue a read for the next block from the server
            resizeReadBuffer(4);
            readBuffer();
            break;

        default:
            RCF_ASSERT(0);
        }
    }

    // TODO: rename
    void SspiFilterBase::acquireCredentials(
        const tstring &userName,
        const tstring &password,
        const tstring &domain)
    {
        // acquire credentials, implicitly (currently logged on user),
        // or explicitly (supply username and password)

        RCF_ASSERT(!mHaveCredentials);

        // TODO: whats with copying pPackage here?

        // setup security package
        SecPkgInfo *pPackage = NULL;
       
        SECURITY_STATUS status = getSecurityFunctionTable()->QuerySecurityPackageInfo(
            (TCHAR*) mPackageName.c_str(),
            &pPackage);

        if ( status != SEC_E_OK )
        {
            RCF_THROW(
                FilterException(
                    RcfError_Sspi, status, RcfSubsystem_Os,
                    "QuerySecurityPackageInfo() failed"))
                (mPackageName.c_str())(status);
        }

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4996 )  // warning C4996: '...' was declared deprecated
#endif

        TCHAR *pName = new TCHAR[ _tcslen(pPackage->Name) + 1 ];
        _tcscpy(pName, pPackage->Name);

        TCHAR *pComment = new TCHAR[ _tcslen(pPackage->Comment) + 1 ];
        _tcscpy(pComment, pPackage->Comment);

#ifdef _MSC_VER
#pragma warning( pop )
#endif

        memcpy ( (void*)&mPkgInfo, (void*)pPackage, sizeof(SecPkgInfo) );
        mPkgInfo.Name = pName;
        mPkgInfo.Comment = pComment;

        getSecurityFunctionTable()->FreeContextBuffer( (void*) pPackage );

        TimeStamp Expiration                    = {0};

#if defined(_MSC_VER) && _MSC_VER == 1200
        SEC_WINNT_AUTH_IDENTITY identity     = {0};
#else
        SEC_WINNT_AUTH_IDENTITY_EX identity     = {0};
#endif

        UTCHAR *pDomain = reinterpret_cast<UTCHAR*>(const_cast<TCHAR*>(domain.c_str()));
        unsigned long pDomainLen = static_cast<unsigned long>(domain.length());

        UTCHAR *pUsername = reinterpret_cast<UTCHAR*>(const_cast<TCHAR*>(userName.c_str()));
        unsigned long pUsernameLen = static_cast<unsigned long>(userName.length());

        UTCHAR *pPassword = reinterpret_cast<UTCHAR*>(const_cast<TCHAR*>(password.c_str()));
        unsigned long pPasswordLen = static_cast<unsigned long>(password.length());

        UTCHAR *pPackages = reinterpret_cast<UTCHAR*>(const_cast<TCHAR*>(mPackageList.c_str()));
        unsigned long pPackagesLen = static_cast<unsigned long>(mPackageList.length());

        if (!userName.empty())
        {
            if (!domain.empty())
            {
                identity.Domain                    = pDomain;
                identity.DomainLength            = pDomainLen;
            }
            if (!userName.empty())
            {
                identity.User                    = pUsername;
                identity.UserLength                = pUsernameLen;
            }
            if (!password.empty())
            {
                identity.Password                = pPassword;
                identity.PasswordLength            = pPasswordLen;
            }
        }

#ifdef _UNICODE
        identity.Flags                            = SEC_WINNT_AUTH_IDENTITY_UNICODE;
#else
        identity.Flags                            = SEC_WINNT_AUTH_IDENTITY_ANSI;
#endif

#if defined(_MSC_VER) && _MSC_VER == 1200
        void *pIdentity = &identity;
#else
        identity.Version                        = SEC_WINNT_AUTH_IDENTITY_VERSION;
        identity.Length                            = sizeof(identity);
        if (!mPackageList.empty())
        {
            identity.PackageList                = pPackages;
            identity.PackageListLength            = pPackagesLen;
        }
        SEC_WINNT_AUTH_IDENTITY_EX *pIdentity = &identity;
#endif

       
        status = getSecurityFunctionTable()->AcquireCredentialsHandle(
            NULL,
            mPkgInfo.Name,
            mServer ? SECPKG_CRED_INBOUND : SECPKG_CRED_OUTBOUND ,
            NULL,
            pIdentity,
            NULL, NULL,
            &mCredentials,
            &Expiration);

        if (status != SEC_E_OK)
        {
            RCF_THROW(
                FilterException(
                    RcfError_Sspi, status, RcfSubsystem_Os,
                    "AcquireCredentialsHandle() failed"))
                (mPkgInfo.Name)(userName)(password)(domain)(status);
        }

        mHaveCredentials = true;

    }

    SspiServerFilter::SspiServerFilter(
        const tstring &packageName,
        const tstring &packageList) :
    SspiFilterBase(packageName, packageList, true)
    {}

    SspiNtlmServerFilter::SspiNtlmServerFilter() :
    SspiServerFilter(_T("NTLM"), _T(""))
    {}

    const FilterDescription &SspiNtlmServerFilter::getFilterDescription() const
    {
        return SspiNtlmServerFilter::sGetFilterDescription();
    }

    const FilterDescription &SspiNtlmServerFilter::sGetFilterDescription()
    {
        return *spFilterDescription;
    }

    SspiKerberosServerFilter::SspiKerberosServerFilter() :
    SspiServerFilter(_T("Kerberos"), _T(""))
    {}

    const FilterDescription &SspiKerberosServerFilter::getFilterDescription() const
    {
        return SspiKerberosServerFilter::sGetFilterDescription();
    }

    const FilterDescription &SspiKerberosServerFilter::sGetFilterDescription()
    {
        return *spFilterDescription;
    }

    SspiNegotiateServerFilter::SspiNegotiateServerFilter(const tstring &packageList) :
    SspiServerFilter(_T("Negotiate"), packageList)
    {}

    const FilterDescription &SspiNegotiateServerFilter::getFilterDescription() const
    {
        return SspiNegotiateServerFilter::sGetFilterDescription();
    }

    const FilterDescription &SspiNegotiateServerFilter::sGetFilterDescription()
    {
        return *spFilterDescription;
    }

    FilterPtr SspiNtlmFilterFactory::createFilter()
    {
        return FilterPtr( new SspiNtlmServerFilter() );
    }
    const FilterDescription &SspiNtlmFilterFactory::getFilterDescription()
    {
        return SspiNtlmServerFilter::sGetFilterDescription();
    }

    FilterPtr SspiKerberosFilterFactory::createFilter()
    {
        return FilterPtr( new SspiKerberosServerFilter() );
    }
    const FilterDescription &SspiKerberosFilterFactory::getFilterDescription()
    {
        return SspiKerberosServerFilter::sGetFilterDescription();
    }

    SspiNegotiateFilterFactory::SspiNegotiateFilterFactory(
        const tstring &packageList) :
            mPackageList(packageList)
    {}

    FilterPtr SspiNegotiateFilterFactory::createFilter()
    {
        return FilterPtr( new SspiNegotiateServerFilter(mPackageList) );
    }
    const FilterDescription &SspiNegotiateFilterFactory::getFilterDescription()
    {
        return SspiNegotiateServerFilter::sGetFilterDescription();
    }

    HINSTANCE               ghProvider          = NULL;      // provider dll's instance
    PSecurityFunctionTable  gpSecurityInterface = NULL;      // security interface table

    PSecurityFunctionTable SspiFilterBase::getSecurityFunctionTable() const
    {
        return gpSecurityInterface;
    }

    void SspiInitialize()
    {
        // load the provider dll
        ghProvider = LoadLibrary ( _T("security.dll") );
        if (ghProvider == NULL)
        {
            int err = GetLastError();
            RCF_THROW(
                FilterException(
                    RcfError_SspiInit,
                    err,
                    RcfSubsystem_Os,
                    "LoadLibrary(\"security.dll\") failed"));
        }

        INIT_SECURITY_INTERFACE InitSecurityInterface;

        InitSecurityInterface = reinterpret_cast<INIT_SECURITY_INTERFACE> (
            GetProcAddress(ghProvider, INIT_SEC_INTERFACE_NAME));

        if (InitSecurityInterface == NULL)
        {
            int err = GetLastError();
            RCF_THROW(
                FilterException(RcfError_SspiInit, err, RcfSubsystem_Os,
                "GetProcAddress() failed to retrieve address of InitSecurityInterface())"));
        }

        gpSecurityInterface = InitSecurityInterface();
        if (gpSecurityInterface == NULL)
        {
            int err = GetLastError();
            RCF_THROW(
                FilterException(RcfError_SspiInit, err, RcfSubsystem_Os,
                "InitSecurityInterface() failed"));
        }

        // try to load the GetUserNameEx() function, if we can
        hModuleSecur32 = LoadLibrary( _T("secur32.dll"));
        if (hModuleSecur32)
        {
            pfnGetUserNameEx = (PfnGetUserNameEx) GetProcAddress(hModuleSecur32, GetUserNameExName);
        }

    }

    void SspiUninitialize()
    {
        FreeLibrary (ghProvider);
        ghProvider = NULL;
        gpSecurityInterface = NULL;

        if (hModuleSecur32)   
        {
            FreeLibrary(hModuleSecur32);
            hModuleSecur32 = 0;
            pfnGetUserNameEx = NULL;
        }
    }

    RCF_ON_INIT_DEINIT( SspiInitialize(), SspiUninitialize() )

    const FilterDescription *SspiNtlmServerFilter::spFilterDescription = NULL;
    const FilterDescription *SspiKerberosServerFilter::spFilterDescription = NULL;
    const FilterDescription *SspiNegotiateServerFilter::spFilterDescription = NULL;

    static void initSspiFilterDescriptions()
    {
        RCF_ASSERT(!SspiNtlmServerFilter::spFilterDescription);
        RCF_ASSERT(!SspiKerberosServerFilter::spFilterDescription);
        RCF_ASSERT(!SspiNegotiateServerFilter::spFilterDescription);

        SspiNtlmServerFilter::spFilterDescription =
            new FilterDescription(
                "sspi ntlm filter",
                RCF_FILTER_SSPI_NTLM,
                true);
       
        SspiKerberosServerFilter::spFilterDescription =
            new FilterDescription(
                "sspi kerberos filter",
                RCF_FILTER_SSPI_KERBEROS,
                true);
       
        SspiNegotiateServerFilter::spFilterDescription =
            new FilterDescription(
                "sspi negotiate filter",
                RCF_FILTER_SSPI_NEGOTIATE,
                true);
    }

    static void deinitSspiFilterDescriptions()
    {
        delete SspiNtlmServerFilter::spFilterDescription;
        SspiNtlmServerFilter::spFilterDescription = NULL;

        delete SspiKerberosServerFilter::spFilterDescription;
        SspiKerberosServerFilter::spFilterDescription = NULL;

        delete SspiNegotiateServerFilter::spFilterDescription;
        SspiNegotiateServerFilter::spFilterDescription = NULL;
    }

    RCF_ON_INIT_DEINIT(
        initSspiFilterDescriptions(),
        deinitSspiFilterDescriptions())

}
