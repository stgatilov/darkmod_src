
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#include <RCF/Exception.hpp>

#include <RCF/Tools.hpp>

#include <RCF/util/Platform/OS/BsdSockets.hpp> // GetErrorString()

namespace RCF {

    std::string getErrorString(int rcfError)
    {
        if (rcfError >= RcfError_User)
        {
            return "non-RCF error";
        }

        switch (rcfError)
        {
        case RcfError_Ok                            :   return "no error";
        case RcfError_Unspecified                   :   return "unknown error";           
        case RcfError_ServerMessageLength           :   return "server side message length error";           
        case RcfError_ClientMessageLength           :   return "client side message length error";           
        case RcfError_Serialization                 :   return "data serialization error";           
        case RcfError_Deserialization               :   return "data deserialization error";           
        case RcfError_UserModeException             :   return "server side user exception";           
        case RcfError_UnknownEndpoint               :   return "unknown endpoint";           
        case RcfError_EndpointPassword              :   return "incorrect endpoint password";           
        case RcfError_EndpointDown                  :   return "endpoint unavailable";           
        case RcfError_EndpointRetry                 :   return "endpoint temporarily unavailable (try again)";           
        case RcfError_ClientConnectTimeout          :   return "client connect timed out";           
        case RcfError_PeerDisconnect                :   return "unexpected peer disconnection";           
        case RcfError_ClientCancel                  :   return "remote call cancelled by client";           
        case RcfError_StubAssignment                :   return "incompatible stub assignment";           
        case RcfError_PayloadFilterMismatch         :   return "message filter mismatch";           
        case RcfError_OpenSslFilterInit             :   return "failed to initialize OpenSSL filter";           
        case RcfError_OpenSslLoadCert               :   return "failed to load OpenSSL certificate file";           
        case RcfError_UnknownPublisher              :   return "unknown publisher name";           
        case RcfError_UnknownFilter                 :   return "unknown filter type";           
        case RcfError_NoServerStub                  :   return "server stub not found";           
        case RcfError_Sspi                          :   return "SSPI filter error";           
        case RcfError_SspiAuthFail                  :   return "SSPI authorization failed";           
        case RcfError_SspiInit                      :   return "failed to initialize SSPI filter";           
        case RcfError_UnknownSubscriber             :   return "unknown subscriber";           
        case RcfError_ClientReadTimeout             :   return "client read timed out";           
        case RcfError_ClientReadFail                :   return "client read failed";           
        case RcfError_ClientWriteTimeout            :   return "client write timed out";           
        case RcfError_ClientWriteFail               :   return "client write failed";           
        case RcfError_ClientConnectFail             :   return "client connect failed";           
        case RcfError_Filter                        :   return "filter error";           
        case RcfError_Socket                        :   return "socket error";           
        case RcfError_FnId                          :   return "invalid function id";           
        case RcfError_UnknownInterface              :   return "unknown object interface";           
        case RcfError_NoEndpoint                    :   return "no endpoint";           
        case RcfError_TransportCreation             :   return "failed to create transport";           
        case RcfError_FilterCount                   :   return "invalid number of filters";           
        case RcfError_FilterMessage                 :   return "failed to filter message";           
        case RcfError_UnfilterMessage               :   return "failed to unfilter message";           
        case RcfError_SspiCredentials               :   return "SSPI credentials failure";           
        case RcfError_SspiEncrypt                   :   return "SSPI encryption failure";           
        case RcfError_SspiDecrypt                   :   return "SSPI decryption failure";           
        case RcfError_SspiImpersonation             :   return "SSPI impersonation failure";           
        case RcfError_NotConnected                  :   return "send attempted without connecting";           
        case RcfError_SocketClose                   :   return "failure to close socket";           
        case RcfError_SocketBind                    :   return "failed to bind socket to port (port already in use?)";
        case RcfError_ZlibDeflate                   :   return "zlib compression error";           
        case RcfError_ZlibInflate                   :   return "zlib decompression error";           
        case RcfError_Zlib                          :   return "zlib error";           
        case RcfError_UnknownSerializationProtocol  :   return "unknown serialization protocol";
        case RcfError_InvalidErrorMessage           :   return "invalid error message from server";
        case SfError_NoCtor                         :   return "construction not supported for this type";
        case SfError_RefMismatch                    :   return "can't deserialize a reference into a non-reference object";
        case SfError_DataFormat                     :   return "input data format error";
        case SfError_ReadFailure                    :   return "failed to read data from underlying stream";
        case SfError_WriteFailure                   :   return "failed to write read data to underlying stream";
        case SfError_BaseDerivedRegistration        :   return "base/derived pair not registered";
        case SfError_TypeRegistration               :   return "type not registered";
        case RcfError_BadException                  :   return "non std::exception-derived exception was thrown";
        case RcfError_Decoding                      :   return "decoding error";
        case RcfError_Encoding                      :   return "encoding error";
        case RcfError_TokenRequestFailed            :   return "no tokens available";
        case RcfError_ObjectFactoryNotFound         :   return "object factory not found";
        case RcfError_PortInUse                     :   return "port already in use";
        case RcfError_DynamicObjectNotFound         :   return "dynamic object not found";
        case RcfError_VersionMismatch               :   return "version mismatch";
        case RcfError_RepeatedRetries               :   return "too many client retries";
        case RcfError_SslCertVerification           :   return "SSL certificate verification failure";
        case RcfError_OutOfBoundsLength                :    return "out of bounds length field";
        default                                     :   return "no available error message";
        }
    }

    std::string getOsErrorString(int osError)
    {
        return Platform::OS::GetErrorString(osError);
    }

    std::string getSubSystemName(int subSystem)
    {
        switch (subSystem)
        {
        case RcfSubsystem_None                      :   return "No sub system";
        case RcfSubsystem_Os                        :   return "Operating system";
        case RcfSubsystem_Zlib                      :   return "Zlib";
        case RcfSubsystem_OpenSsl                   :   return "OpenSSL";
        case RcfSubsystem_Asio                      :   return "Boost.Asio";
        default                                     :   return "No available sub system name";
        }
    }

    void RemoteException::throwCopy() const
    {
        RCF_ASSERT(
            typeid(*this) == typeid(RemoteException))
            (typeid(*this));

        RemoteException remoteException(*this);
        RCF_THROW(remoteException);
    }

} // namespace RCF
