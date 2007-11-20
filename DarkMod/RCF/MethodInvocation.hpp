
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_METHODINVOCATION_HPP
#define INCLUDE_RCF_METHODINVOCATION_HPP

#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <RCF/ByteBuffer.hpp>
#include <RCF/AsyncFilter.hpp>
#include <RCF/Token.hpp>

namespace RCF {

    class RcfServer;
    class StubEntry;
    typedef boost::shared_ptr<StubEntry> StubEntryPtr;
    class RcfSession;
    typedef boost::shared_ptr<RcfSession> RcfSessionPtr;
    class SerializationProtocolIn;
    class SerializationProtocolOut;

    class MethodInvocationResponse;

    // message types
    static const int Descriptor_Error               = 0;
    static const int Descriptor_Request             = 1;
    static const int Descriptor_Response            = 2;
    static const int Descriptor_FilteredPayload     = 3;

    // encoding and decoding
    void encodeBool(bool value, std::vector<char> &vec, std::size_t &pos);
    void encodeInt(int value, std::vector<char> &vec, std::size_t &pos);
    void encodeString(const std::string &value, std::vector<char> &vec, std::size_t &pos);

    void encodeBool(bool value, const ByteBuffer &byteBuffer, std::size_t &pos);
    void encodeInt(int value, const ByteBuffer &byteBuffer, std::size_t &pos);
    void encodeString(const std::string &value, const ByteBuffer &byteBuffer, std::size_t &pos);

    void decodeBool(bool &value, const ByteBuffer &byteBuffer, std::size_t &pos);
    void decodeInt(int &value, const ByteBuffer &byteBuffer, std::size_t &pos);
    void decodeString(std::string &value, const ByteBuffer &byteBuffer, std::size_t &pos);

    class MethodInvocationRequest
    {
    public:
        MethodInvocationRequest();

        void init(
            const Token &token,
            const std::string &service,
            const std::string &subInterface,
            int fnId,
            int serializationProtocol,
            bool oneway,
            bool close,
            int rcfRuntimeVersion,
            bool ignoreRcfRuntimeVersion);

        Token getToken() const;
        std::string getSubInterface() const;
        int getFnId() const;
        bool getOneway() const;
        bool getClose() const;
        std::string getService() const;
        void setService(const std::string &service);

        ByteBuffer encodeRequestHeader();

        void encodeRequest(
            const std::vector<ByteBuffer> &byteBuffersIn,
            std::vector<ByteBuffer> &byteBuffersOut,
            const std::vector<FilterPtr> &filters);

        bool decodeRequest(
            const ByteBuffer &byteBufferIn,
            RcfSessionPtr rcfSessionPtr,
            RcfServer &rcfServer);

        void encodeResponse(
            const std::vector<ByteBuffer> &byteBuffersIn,
            std::vector<ByteBuffer> &byteBuffersOut,
            const std::vector<FilterPtr> &filters,
            bool isException);

        void decodeResponse(
            const ByteBuffer &byteBufferIn,
            ByteBuffer &byteBufferOut,
            MethodInvocationResponse &response,
            const std::vector<FilterPtr> &filters);

        StubEntryPtr locateStubEntryPtr(RcfServer &rcfServer);

    private:

        void decodeFiltered(
            const ByteBuffer &encodedByteBuffer,
            ByteBuffer &byteBufferClear,
            ByteBuffer &byteBufferUnfiltered,
            RcfServer *pRcfServer,
            RcfSessionPtr rcfSessionPtr,
            const std::vector<FilterPtr> &filters);

        void encodeFiltered(
            std::vector<ByteBuffer> &byteBuffersIn,
            const std::vector<ByteBuffer> &byteBuffersOut,
            const std::vector<FilterPtr> &filters);

        Token           mToken;
        std::string     mSubInterface;
        int             mFnId;
        int             mSerializationProtocol;
        bool            mOneway;
        bool            mClose;
        std::string     mService;
        int                mRcfRuntimeVersion;
        bool            mIgnoreRcfRuntimeVersion;

        boost::shared_ptr<std::vector<char> > mVecPtr;
    };

    class MethodInvocationResponse
    {
    public:
        MethodInvocationResponse();

        bool isException() const;
        bool isError() const;
        int getError() const;
        int getArg0() const;

    private:
        friend class MethodInvocationRequest;

        bool            mException;
        bool            mError;
        int             mErrorCode;
        int                mArg0;
    };

} // namespace RCF

#endif // ! INCLUDE_RCF_METHODINVOCATION_HPP
