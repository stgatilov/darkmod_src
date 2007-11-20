
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_EXCEPTION_HPP
#define INCLUDE_RCF_EXCEPTION_HPP

#include <sstream>
#include <stdexcept>
#include <string>

#include <RCF/util/DefaultInit.hpp>
#include <RCF/TypeTraits.hpp>

namespace RCF {

    // RCF error codes
    // range 0-1000 reserved for RCF, remaining range can be used independently of RCF

    static const int RcfError_Ok                                =  0;
    static const int RcfError_Unspecified                       =  1;
    static const int RcfError_ServerMessageLength               =  2;
    static const int RcfError_ClientMessageLength               =  3;
    static const int RcfError_Serialization                     =  4;
    static const int RcfError_Deserialization                   =  5;
    static const int RcfError_UserModeException                 =  6;
    static const int RcfError_UnknownEndpoint                   =  8;
    static const int RcfError_EndpointPassword                  =  9;
    static const int RcfError_EndpointDown                      = 10;
    static const int RcfError_EndpointRetry                     = 11;
    static const int RcfError_ClientConnectTimeout              = 16;
    static const int RcfError_PeerDisconnect                    = 17;
    static const int RcfError_ClientCancel                      = 18;
    static const int RcfError_StubAssignment                    = 19;
    static const int RcfError_PayloadFilterMismatch             = 20;
    static const int RcfError_OpenSslFilterInit                 = 21;
    static const int RcfError_OpenSslLoadCert                   = 22;
    static const int RcfError_UnknownPublisher                  = 23;
    static const int RcfError_UnknownFilter                     = 24;
    static const int RcfError_NoServerStub                      = 25;
    static const int RcfError_Sspi                              = 26;
    static const int RcfError_SspiAuthFail                      = 27;
    static const int RcfError_SspiInit                          = 28;
    static const int RcfError_UnknownSubscriber                 = 29;
    static const int RcfError_ClientReadTimeout                 = 30;
    static const int RcfError_ClientReadFail                    = 31;
    static const int RcfError_ClientWriteTimeout                = 32;
    static const int RcfError_ClientWriteFail                   = 33;
    static const int RcfError_ClientConnectFail                 = 34;
    static const int RcfError_Filter                            = 35;
    static const int RcfError_Socket                            = 36;
    static const int RcfError_FnId                              = 37;
    static const int RcfError_UnknownInterface                  = 38;
    static const int RcfError_NoEndpoint                        = 39;
    static const int RcfError_TransportCreation                 = 40;
    static const int RcfError_FilterCount                       = 41;
    static const int RcfError_FilterMessage                     = 42;
    static const int RcfError_UnfilterMessage                   = 43;
    static const int RcfError_SspiCredentials                   = 44;
    static const int RcfError_SspiEncrypt                       = 45;
    static const int RcfError_SspiDecrypt                       = 46;
    static const int RcfError_SspiImpersonation                 = 47;
    static const int RcfError_NotConnected                      = 48;
    static const int RcfError_SocketClose                       = 49;
    static const int RcfError_ZlibDeflate                       = 50;
    static const int RcfError_ZlibInflate                       = 51;
    static const int RcfError_Zlib                              = 52;
    static const int RcfError_UnknownSerializationProtocol      = 53;
    static const int RcfError_InvalidErrorMessage               = 54;
    static const int SfError_NoCtor                             = 55;
    static const int SfError_RefMismatch                        = 56;
    static const int SfError_DataFormat                         = 57;
    static const int SfError_ReadFailure                        = 58;
    static const int SfError_WriteFailure                       = 59;
    static const int SfError_BaseDerivedRegistration            = 60;
    static const int SfError_TypeRegistration                   = 61;
    static const int RcfError_BadException                      = 62;
    static const int RcfError_SocketBind                        = 63;
    static const int RcfError_Decoding                          = 64;
    static const int RcfError_Encoding                          = 65;
    static const int RcfError_TokenRequestFailed                = 66;
    static const int RcfError_ObjectFactoryNotFound             = 67;
    static const int RcfError_PortInUse                         = 68;
    static const int RcfError_DynamicObjectNotFound             = 69;
    static const int RcfError_VersionMismatch                   = 70;
    static const int RcfError_RepeatedRetries                   = 71;
    static const int RcfError_SslCertVerification               = 72;
    static const int RcfError_OutOfBoundsLength                 = 73;
    static const int RcfError_User                              = 1001;

    // RCF subsystem identifiers
    static const int RcfSubsystem_None                          = 0;
    static const int RcfSubsystem_Os                            = 1;
    static const int RcfSubsystem_Zlib                          = 2;
    static const int RcfSubsystem_OpenSsl                       = 3;
    static const int RcfSubsystem_Asio                          = 4;

    std::string getErrorString(int rcfError);
    std::string getSubSystemName(int subSystem);
    std::string getOsErrorString(int osError);

    /// Base class of all exceptions thrown by RCF.
    class Exception : public std::runtime_error
    {
    public:
        Exception() :
            std::runtime_error(""),
            mWhat(),
            mError(RCF_DEFAULT_INIT),
            mSubSystemError(RCF_DEFAULT_INIT),
            mSubSystem(RCF_DEFAULT_INIT)
        {}

        Exception(const std::string &what, const std::string &context = "") :
            std::runtime_error(""),
            mWhat(what),
            mContext(context),
            mError(RcfError_Unspecified),
            mSubSystemError(RCF_DEFAULT_INIT),
            mSubSystem(RCF_DEFAULT_INIT)
        {}

        Exception(
            int error,
            const std::string &what = "",
            const std::string &context = "") :
                std::runtime_error(""),
                mWhat(what),
                mContext(context),
                mError(error),
                mSubSystemError(RCF_DEFAULT_INIT),
                mSubSystem(RCF_DEFAULT_INIT)
        {}

        Exception(
            int error,
            int subSystemError,
            int subSystem,
            const std::string &what = "",
            const std::string &context = "") :
                std::runtime_error(""),
                mWhat(what),
                mContext(context),
                mError(error),
                mSubSystemError(subSystemError),
                mSubSystem(subSystem)
        {}

        ~Exception() throw()
        {}

        const char *what() const throw()
        {
            mTranslatedWhat = translate();
            return mTranslatedWhat.c_str();
        }

        int getError() const
        {
            return mError;
        }

        int getSubSystemError() const
        {
            return mSubSystemError;
        }

        int getSubSystem() const
        {
            return mSubSystem;
        }

        void setContext(const std::string &context)
        {
            mContext = context;
        }

        std::string getContext() const
        {
            return mContext;
        }

        void setWhat(const std::string &what)
        {
            mWhat = what;
        }

        std::string getWhat() const
        {
            return mWhat;
        }

    protected:

        std::string translate() const
        {
            //return getErrorString(mError) + " (" + mWhat + ")";

            std::string osErr;
            // TODO: retrieve error messages from other subsystems as well
            if (mSubSystem == RcfSubsystem_Os)
            {
                osErr = getOsErrorString(mSubSystemError);
            }

            std::ostringstream os;
            os
                << "[" << mError << ": " << getErrorString(mError) << "]"
                << "[" << mSubSystem << ": " << getSubSystemName(mSubSystem) << "]"
                << "[" << mSubSystemError << ": " << osErr << "]"
                << "[What: " << mWhat << "]"
                << "[Context: " << mContext << "]";
            return os.str();
        }

        // protected to make serialization of RemoteException simpler
    protected:

        std::string mWhat;
        std::string mContext;
        int mError;
        int mSubSystemError;
        int mSubSystem;

        mutable std::string mTranslatedWhat;
    };

    class RemoteException : public Exception
    {
    public:
        RemoteException()
        {}

        RemoteException(
            int remoteError,
            const std::string &remoteWhat = "",
            const std::string &remoteContext = "",
            const std::string &remoteExceptionType = "") :
                Exception(
                    remoteError,
                    remoteWhat,
                    remoteContext),
                    mRemoteExceptionType(remoteExceptionType)
        {}

        RemoteException(
            int remoteError,
            int remoteSubSystemError,
            int remoteSubSystem,
            const std::string &remoteWhat = "",
            const std::string &remoteContext = "",
            const std::string &remoteExceptionType = "") :
                Exception(
                    remoteError,
                    remoteSubSystemError,
                    remoteSubSystem,
                    remoteWhat,
                    remoteContext),
                    mRemoteExceptionType(remoteExceptionType)
        {}

        ~RemoteException() throw()
        {}

        const char *what() const throw()
        {
            mTranslatedWhat = "[Remote]" + translate();
            return mTranslatedWhat.c_str();
        }

        std::string getRemoteExceptionType() const
        {
            return mRemoteExceptionType;
        }

        template<typename Archive>
        void serialize(Archive &ar, const unsigned int)
        {
            ar
                & mWhat
                & mContext
                & mError
                & mSubSystemError
                & mSubSystem
                & mRemoteExceptionType;
        }

        virtual void throwCopy() const;

    private:
        std::string mRemoteExceptionType;
    };

#define RCF_DEFINE_EXCEPTION(E, PE)                             \
    class E : public PE                                         \
    {                                                           \
    public:                                                     \
        E(                                                      \
            const std::string &what = "") :                     \
                PE(RcfError_Unspecified, what)                  \
        {}                                                      \
        E(                                                      \
            int error,                                          \
            const std::string &what = "") :                     \
                PE(error, what)                                 \
        {}                                                      \
        E(                                                      \
            int error,                                          \
            int subSystemError,                                 \
            int subSystem,                                      \
            const std::string &what = "") :                     \
                PE(error, subSystemError, subSystem, what)      \
        {}                                                      \
        ~E() throw()                                            \
        {}                                                      \
    };

    RCF_DEFINE_EXCEPTION(SerializationException,        Exception)
    RCF_DEFINE_EXCEPTION(AssertionFailureException,     Exception)
    RCF_DEFINE_EXCEPTION(FilterException,               Exception)

    class VersioningException : public RemoteException
    {
    public:
        VersioningException(int version) :
            RemoteException(RcfError_VersionMismatch),
            mVersion(version)
        {}

        ~VersioningException() throw()
        {}

        int getVersion() const
        {
            return mVersion;
        }

    private:
        int mVersion;
    };

#undef RCF_DEFINE_EXCEPTION

} // namespace RCF

// TODO: this should be in RcfServer.cpp, I think
#include <memory>
#include <boost/type_traits.hpp>
RCF_BROKEN_COMPILER_TYPE_TRAITS_SPECIALIZATION( RCF::RemoteException )
RCF_BROKEN_COMPILER_TYPE_TRAITS_SPECIALIZATION( std::auto_ptr<RCF::RemoteException> )

#endif // ! INCLUDE_RCF_EXCEPTION_HPP
