
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#include <RCF/MethodInvocation.hpp>

#include <vector>

#include <boost/mpl/assert.hpp>

#include <RCF/CurrentSession.hpp>
#include <RCF/RcfServer.hpp>
#include <RCF/RcfSession.hpp>
#include <RCF/SerializationProtocol.hpp>
#include <RCF/Service.hpp>

namespace RCF {

    //*************************************
    // MethodInvocationRequest

    MethodInvocationRequest::MethodInvocationRequest() :
        mToken(),
        mSubInterface(),
        mFnId(RCF_DEFAULT_INIT),
        mSerializationProtocol(RCF_DEFAULT_INIT),
        mOneway(RCF_DEFAULT_INIT),
        mClose(RCF_DEFAULT_INIT),
        mService(),
        mRcfRuntimeVersion(RCF_DEFAULT_INIT),
        mIgnoreRcfRuntimeVersion(RCF_DEFAULT_INIT)
    {}

    void MethodInvocationRequest::init(
        const Token &token,
        const std::string &service,
        const std::string &subInterface,
        int fnId,
        int serializationProtocol,
        bool oneway,
        bool close,
        int rcfRuntimeVersion,
        bool ignoreRcfRuntimeVersion)
    {
        mToken = token;
        mService = service;
        mSubInterface = subInterface;
        mFnId = fnId;
        mSerializationProtocol = serializationProtocol;
        mOneway = oneway;
        mClose = close;
        mRcfRuntimeVersion = rcfRuntimeVersion;
        mIgnoreRcfRuntimeVersion = ignoreRcfRuntimeVersion;
    }

    Token MethodInvocationRequest::getToken() const
    {
        return mToken;
    }

    std::string MethodInvocationRequest::getSubInterface() const
    {
        return mSubInterface;
    }

    int MethodInvocationRequest::getFnId() const
    {
        return mFnId;
    }

    bool MethodInvocationRequest::getOneway() const
    {
        return mOneway;
    }

    bool MethodInvocationRequest::getClose() const
    {
        return mClose;
    }

    std::string MethodInvocationRequest::getService() const
    {
        return mService;
    }

    void MethodInvocationRequest::setService(const std::string &service)
    {
        mService = service;
    }

    bool MethodInvocationRequest::decodeRequest(
        const ByteBuffer &byteBufferIn,
        RcfSessionPtr rcfSessionPtr,
        RcfServer &rcfServer)
    {

        ByteBuffer headerByteBuffer;
        ByteBuffer unfilteredPayloadByteBuffer;

        decodeFiltered(
            byteBufferIn,
            headerByteBuffer,
            unfilteredPayloadByteBuffer,
            &rcfServer,
            rcfSessionPtr,
            std::vector<FilterPtr>());

        ByteBuffer b = headerByteBuffer ?
            headerByteBuffer :
            unfilteredPayloadByteBuffer;

        std::size_t pos = 0;
        {
            int tokenId = 0;
            int msgId = 0;
            int messageVersion = 0;
            int runtimeVersion = 1;
            bool ignoreRuntimeVersion = false;

            decodeInt(msgId, b, pos);
            RCF_VERIFY(msgId == Descriptor_Request, Exception(RcfError_Decoding))(msgId);
            decodeInt(messageVersion, b, pos);
            
            if (messageVersion > 2)
            {
                return false;
            }

            decodeString(mService, b, pos);
            decodeInt(tokenId, b, pos);
            decodeString(mSubInterface, b, pos);
            decodeInt(mFnId, b, pos);
            decodeInt(mSerializationProtocol, b, pos);
            decodeBool(mOneway, b, pos);
            decodeBool(mClose, b, pos);

            if (messageVersion >= 1)
            {
                decodeInt(runtimeVersion, b, pos);
                decodeBool(ignoreRuntimeVersion, b, pos);
            }

            mToken = Token(tokenId);

            if (mSubInterface.empty())
            {
                mSubInterface = mService;
            }

            if (runtimeVersion > rcfServer.getRcfRuntimeVersion())
            {
                return false;
            }
            else
            {
                rcfSessionPtr->setRcfRuntimeVersion(runtimeVersion);
                // TODO: set ignore version flag as well
            }
        }

        if (headerByteBuffer)
        {
            rcfSessionPtr->mIn.reset(
                unfilteredPayloadByteBuffer,
                mSerializationProtocol);
        }
        else
        {
            rcfSessionPtr->mIn.reset(
                ByteBuffer(unfilteredPayloadByteBuffer, pos),
                mSerializationProtocol);
        }

        rcfSessionPtr->mOut.reset(mSerializationProtocol);

        return true;
    }

    void MethodInvocationRequest::encodeResponse(
        const std::vector<ByteBuffer> &byteBuffersIn,
        std::vector<ByteBuffer> &byteBuffersOut,
        const std::vector<FilterPtr> &filters,
        bool isException)
    {
       
        // prepend response header
        std::size_t len = 1+1+1;
        RCF_ASSERT(!byteBuffersIn.empty());
        ByteBuffer &front = const_cast<ByteBuffer &>(byteBuffersIn.front());
        front.expandIntoLeftMargin(len);
        std::size_t pos = 0;
        BOOST_STATIC_ASSERT(0<=Descriptor_Response && Descriptor_Response<255);
        RCF::encodeInt(Descriptor_Response, front, pos);
        RCF::encodeInt(0, front, pos);
        RCF::encodeBool(isException, front, pos);
        RCF_ASSERT(pos == len);

        encodeFiltered(
            byteBuffersOut,
            byteBuffersIn,
            filters);
    }

    ByteBuffer MethodInvocationRequest::encodeRequestHeader()
    {
        RCF_ASSERT(!mVecPtr || mVecPtr.unique());
        if (!mVecPtr)
        {
            mVecPtr.reset(new std::vector<char>(50));
        }

        int runtimeVersion = getCurrentClientStubPtr()->getRcfRuntimeVersion();
        int requestHeaderVersion = (runtimeVersion < 2) ? 0 : 1;

        std::size_t pos = 0;
        RCF::encodeInt(Descriptor_Request, *mVecPtr, pos);
        RCF::encodeInt(requestHeaderVersion, *mVecPtr, pos);
        RCF::encodeString(mService, *mVecPtr, pos);
        RCF::encodeInt(mToken.getId(), *mVecPtr, pos);
        mSubInterface == mService ?
            RCF::encodeString("", *mVecPtr, pos) :
            RCF::encodeString(mSubInterface, *mVecPtr, pos);
        RCF::encodeInt(mFnId, *mVecPtr, pos);
        RCF::encodeInt(mSerializationProtocol, *mVecPtr, pos);
        RCF::encodeBool(mOneway, *mVecPtr, pos);
        RCF::encodeBool(mClose, *mVecPtr, pos);

        if (requestHeaderVersion >= 1)
        {
            RCF::encodeInt(mRcfRuntimeVersion, *mVecPtr, pos);
            RCF::encodeBool(mIgnoreRcfRuntimeVersion, *mVecPtr, pos);
        }

        mVecPtr->resize(pos);

        return ByteBuffer(mVecPtr);
    }

    void MethodInvocationRequest::encodeRequest(
        const std::vector<ByteBuffer> &byteBuffersIn,
        std::vector<ByteBuffer> &byteBuffersOut,
        const std::vector<FilterPtr> &filters)
    {
        encodeFiltered(
            byteBuffersOut,
            byteBuffersIn,
            filters);
    }

    void MethodInvocationRequest::decodeResponse(
        const ByteBuffer &byteBufferIn,
        ByteBuffer &byteBufferOut,
        MethodInvocationResponse &response,
        const std::vector<FilterPtr> &filters)
    {

        // decode response

        ByteBuffer headerByteBuffer;
        ByteBuffer unfilteredPayloadByteBuffer;

        decodeFiltered(
            byteBufferIn,
            headerByteBuffer,
            unfilteredPayloadByteBuffer,
            NULL,
            RcfSessionPtr(),
            filters);

        std::size_t pos = 0;
        {

            // decode response header
            ByteBuffer b = headerByteBuffer ?
                headerByteBuffer :
                unfilteredPayloadByteBuffer;

            int msgId = 0;
            decodeInt(msgId, b, pos);

            int ver = 0;
            decodeInt(ver, b, pos);
           
            if (msgId == Descriptor_Error)
            {
                RCF_VERIFY(ver == 0, Exception(RcfError_Decoding))(ver);

                int error = 0;
                decodeInt(error, b, pos);
                response.mError = true;
                response.mErrorCode = error;
                if (error == RcfError_VersionMismatch)
                {
                    decodeInt(response.mArg0, b, pos);
                }
            }
            else
            {
                RCF_VERIFY(msgId == Descriptor_Response, Exception(RcfError_Decoding))(msgId);
                RCF_VERIFY(ver == 0, Exception(RcfError_Decoding))(ver);

                decodeBool(response.mException, b, pos);
            }
        }

        byteBufferOut =
            headerByteBuffer ?
                unfilteredPayloadByteBuffer :
                ByteBuffer(unfilteredPayloadByteBuffer, pos);
    }

    StubEntryPtr MethodInvocationRequest::locateStubEntryPtr(
        RcfServer &rcfServer)
    {
        Token targetToken = getToken();
        std::string targetName = getService();
        StubEntryPtr stubEntryPtr;
        RcfSessionPtr rcfSessionPtr = getCurrentRcfSessionPtr();
        if (targetToken != Token())
        {
            if (!rcfServer.mStubEntryLookupProviders.empty())
            {
                stubEntryPtr = rcfServer.mStubEntryLookupProviders[0]
                ->getStubEntryPtr(targetToken);
            }
        }
        else if (!targetName.empty())
        {
            ReadLock readLock(rcfServer.mStubMapMutex);
            std::string servantName = getService();
            RcfServer::StubMap::iterator iter = rcfServer.mStubMap.find(servantName);
            if (iter != rcfServer.mStubMap.end())
            {
                stubEntryPtr = (*iter).second;
            }
        }
        else
        {
            stubEntryPtr = rcfSessionPtr->getDefaultStubEntryPtr();
        }

        return stubEntryPtr;
    }

   
    //*******************************************
    // MethodInvocationResponse

    MethodInvocationResponse::MethodInvocationResponse() :
        mException(RCF_DEFAULT_INIT),
        mError(RCF_DEFAULT_INIT),
        mErrorCode(RCF_DEFAULT_INIT),
        mArg0(RCF_DEFAULT_INIT)
    {}

    bool MethodInvocationResponse::isException() const
    {
        return mException;
    }

    bool MethodInvocationResponse::isError() const
    {
        return mError;
    }

    int MethodInvocationResponse::getError() const
    {
        return mErrorCode;
    }

    int MethodInvocationResponse::getArg0() const
    {
        return mArg0;
    }

    //*******************************************

    void MethodInvocationRequest::encodeFiltered(
        std::vector<ByteBuffer> &message,
        const std::vector<ByteBuffer> &unfilteredData,
        const std::vector<FilterPtr> &filters)
    {
        if (filters.empty())
        {
            // encode unfiltered

            message.resize(0);

            std::copy(
                unfilteredData.begin(),
                unfilteredData.end(),
                std::back_inserter(message));
        }
        else
        {
            // encode filtered

            ThreadLocalCached< std::vector<ByteBuffer> > tlcFilteredData;
            std::vector<ByteBuffer> &filteredData = tlcFilteredData.get();

            std::size_t unfilteredLen = lengthByteBuffers(unfilteredData);
            bool ok = filterData(unfilteredData, filteredData, filters);
            RCF_VERIFY(ok, Exception(RcfError_FilterMessage));

            message.resize(0);

            std::copy(
                filteredData.begin(),
                filteredData.end(),
                std::back_inserter(message));

            if (filteredData.empty())
            {
                RCF_ASSERT(lengthByteBuffers(unfilteredData) == 0);
                RCF_ASSERT(!unfilteredData.empty());
                RCF_ASSERT(unfilteredData.front().getLength() == 0);
                message.push_back(unfilteredData.front());
            }

            const std::size_t VecLen = (5+10)*4;
            char vec[VecLen];
            ByteBuffer byteBuffer(&vec[0], VecLen);

            std::size_t pos = 0;
            RCF::encodeInt(Descriptor_FilteredPayload, byteBuffer, pos);
            RCF::encodeInt(0, byteBuffer, pos);

            RCF_VERIFY(
                filters.size() <= 10,
                Exception(RcfError_FilterCount));

            RCF::encodeInt(static_cast<int>(filters.size()), byteBuffer, pos);
            for (std::size_t i=0; i<filters.size(); ++i)
            {
                int filterId = filters[i]->getFilterDescription().getId();
                RCF::encodeInt(filterId, byteBuffer, pos);
            }

            int len1 = 0; // unfiltered leading portion
            RCF::encodeInt(len1, byteBuffer, pos);

            int len2 = static_cast<int>(unfilteredLen);
            RCF::encodeInt(len2, byteBuffer, pos);

            RCF_ASSERT(pos <= VecLen);
            std::size_t headerLen = pos;

            RCF_ASSERT(
                !message.empty() &&
                message.front().getLeftMargin() >= headerLen)
                (message.front().getLeftMargin())(headerLen);

            ByteBuffer &front = message.front();
            front.expandIntoLeftMargin(headerLen);
            memcpy(front.getPtr(), &vec[0], headerLen);

        }
    }

    void MethodInvocationRequest::decodeFiltered(
        const ByteBuffer &encodeddByteBuffer,
        ByteBuffer &byteBufferClear,
        ByteBuffer &byteBufferUnfiltered,
        RcfServer *pRcfServer,
        RcfSessionPtr rcfSessionPtr,
        const std::vector<FilterPtr> &filters)
    {
        // TODO
        RCF_UNUSED_VARIABLE(byteBufferClear);

        ThreadLocalCached< std::vector<int> > tlcFilterIds;
        std::vector<int> &filterIds = tlcFilterIds.get();

        std::size_t pos1 = 0;
        std::size_t pos2 = 0;
        std::size_t unfilteredLen = 0;

        char * const pch = (char*) encodeddByteBuffer.getPtr() ;
        if (pch[0] == Descriptor_FilteredPayload)
        {
            std::size_t pos = 0;

            int descriptor = 0;
            decodeInt(descriptor, encodeddByteBuffer, pos);
            RCF_VERIFY(descriptor == Descriptor_FilteredPayload, Exception(RcfError_Decoding))(descriptor);

            int version = 0;
            decodeInt(version, encodeddByteBuffer, pos);
            RCF_VERIFY(version == 0, Exception(RcfError_Decoding))(version);

            int filterCount = 0;
            decodeInt(filterCount, encodeddByteBuffer, pos);
            RCF_VERIFY(0 < filterCount && filterCount <= 10, Exception(RcfError_Decoding))(filterCount);

            filterIds.resize(0);
            for (int i=0; i<filterCount; ++i)
            {
                int filterId = 0;
                decodeInt(filterId, encodeddByteBuffer, pos);
                filterIds.push_back(filterId);
            }

            int clearLen = 0;
            decodeInt(clearLen, encodeddByteBuffer, pos);
            RCF_VERIFY(0 <= clearLen, Exception(RcfError_Decoding))(clearLen);
           
            int unfilteredLen_ = 0;
            decodeInt(unfilteredLen_, encodeddByteBuffer, pos);
            RCF_VERIFY(0 <= unfilteredLen, Exception(RcfError_Decoding))(unfilteredLen);

            pos1 = pos;
            pos2 = pos1 + clearLen;
            unfilteredLen = unfilteredLen_;
        }

        RCF_ASSERT(0 == pos1 || (0 < pos1 && pos1 <= pos2));

        ByteBuffer byteBufferIn = encodeddByteBuffer;
        ByteBuffer &unfilteredPayloadByteBuffer = byteBufferUnfiltered;
        ByteBuffer payloadByteBuffer(byteBufferIn, pos2);

        if (pRcfServer)
        {
            if (filterIds.empty())
            {
                rcfSessionPtr->mFiltered = false;
                unfilteredPayloadByteBuffer = payloadByteBuffer;
            }
            else
            {
                rcfSessionPtr->mFiltered = true;
                std::vector<FilterPtr> &filters = rcfSessionPtr->mFilters;
                if (
                    filters.size() != filterIds.size() ||
                    !std::equal(
                        filters.begin(),
                        filters.end(),
                        filterIds.begin(),
                        FilterIdComparison()))
                {
                    filters.clear();
                    std::transform(
                        filterIds.begin(),
                        filterIds.end(),
                        std::back_inserter(filters),
                        boost::bind( &RcfServer::createFilter, pRcfServer, _1) );

                    if (
                        std::find_if(
                        filters.begin(),
                        filters.end(),
                        SharedPtrIsNull()) == filters.end())
                    {
                        connectFilters(filters);
                    }
                    else
                    {
                        // TODO: better not to throw exceptions here?
                        RCF_THROW(Exception(RcfError_UnknownFilter));
                    }
                }

                bool bRet = unfilterData(
                    payloadByteBuffer,
                    unfilteredPayloadByteBuffer,
                    unfilteredLen,
                    filters);

                RCF_ASSERT(bRet);
            }
        }
        else
        {
            if (filters.size() == filterIds.size() &&
                std::equal(
                filters.begin(),
                filters.end(),
                filterIds.begin(),
                FilterIdComparison()))
            {
                if (filters.empty())
                {
                    unfilteredPayloadByteBuffer = payloadByteBuffer;
                }
                else
                {
                    bool bRet = unfilterData(
                        payloadByteBuffer,
                        unfilteredPayloadByteBuffer,
                        unfilteredLen,
                        filters);

                    RCF_VERIFY(bRet, Exception(RcfError_UnfilterMessage));
                }
            }
            else
            {
                RCF_THROW(
                    Exception(RcfError_PayloadFilterMismatch))
                    (filterIds.size())(filters.size());
            }
        }
    }

    //*************************************

    void encodeBool(bool value, std::vector<char> &vec, std::size_t &pos)
    {
        RCF_ASSERT(pos <= vec.size());
        if (pos + 1 > vec.size())
        {
            vec.resize(vec.size()+1);
        }

        value ?
            vec[pos] = 1 :
            vec[pos] = 0;
        pos += 1;
    }

    void encodeInt(int value, std::vector<char> &vec, std::size_t &pos)
    {
        RCF_ASSERT(pos <= vec.size());
        if (pos + 5 > vec.size())
        {
            vec.resize(vec.size()+5);
        }

        if (0 <= value && value < 255)
        {
            RCF_ASSERT(pos+1<=vec.size());
            vec[pos] = static_cast<char>(value);
            pos += 1;
        }
        else
        {
            RCF_ASSERT(pos+1<=vec.size());
            vec[pos] = (unsigned char)(255);
            pos += 1;

            RCF_ASSERT(pos+4<=vec.size());
            BOOST_STATIC_ASSERT(sizeof(int) == 4);
            RCF::machineToNetworkOrder(&value, 4, 1);
            memcpy(&vec[pos], &value, 4);
            pos += 4;
        }
    }

    void encodeString(
        const std::string &value,
        std::vector<char> &vec,
        std::size_t &pos)
    {
        int len = static_cast<int>(value.length());
        RCF::encodeInt(len, vec, pos);

        RCF_ASSERT(pos <= vec.size());
        if (pos + len > vec.size())
        {
            vec.resize(vec.size()+len);
        }
        memcpy(&vec[pos], value.c_str(), len);
        pos += len;
    }

    // TODO: refactor
    void encodeBool(bool value, const ByteBuffer &byteBuffer, std::size_t &pos)
    {
        RCF_ASSERT(pos+1 <= byteBuffer.getLength());

        value ?
            byteBuffer.getPtr()[pos] = 1 :
            byteBuffer.getPtr()[pos] = 0;
        pos += 1;
    }

    void encodeInt(int value, const ByteBuffer &byteBuffer, std::size_t &pos)
    {
        if (0 <= value && value < 255)
        {
            RCF_ASSERT(pos+1<=byteBuffer.getLength());
            byteBuffer.getPtr()[pos] = static_cast<char>(value);
            pos += 1;
        }
        else
        {
            RCF_ASSERT(pos+1<=byteBuffer.getLength());
            byteBuffer.getPtr()[pos] = (unsigned char)(255);
            pos += 1;

            RCF_ASSERT(pos+4<=byteBuffer.getLength());
            BOOST_STATIC_ASSERT(sizeof(int) == 4);
            RCF::machineToNetworkOrder(&value, 4, 1);
            memcpy(&byteBuffer.getPtr()[pos], &value, 4);
            pos += 4;
        }
    }

    void decodeBool(bool &value, const ByteBuffer &byteBuffer, std::size_t &pos)
    {
        RCF_VERIFY(
            pos+1 <= byteBuffer.getLength(),
            RCF::Exception(RcfError_Decoding));

        char ch = byteBuffer.getPtr()[pos];
       
        RCF_VERIFY(
            ch == 0 || ch == 1,
            RCF::Exception(RcfError_Decoding));

        pos += 1;
        value = ch ? true : false;
    }

    void decodeInt(int &value, const ByteBuffer &byteBuffer, std::size_t &pos)
    {
        RCF_VERIFY(
            pos+1 <= byteBuffer.getLength(),
            RCF::Exception(RcfError_Decoding));

        unsigned char ch = byteBuffer.getPtr()[pos];
        pos += 1;

        if (ch < 255)
        {
            value = ch;
        }
        else
        {
            RCF_VERIFY(
                pos+4 <= byteBuffer.getLength(),
                RCF::Exception(RcfError_Decoding));

            BOOST_STATIC_ASSERT(sizeof(int) == 4);
            memcpy(&value, byteBuffer.getPtr()+pos, 4);
            RCF::networkToMachineOrder(&value, 4, 1);
            pos += 4;
        }
    }

    void decodeString(
        std::string &value,
        const ByteBuffer &byteBuffer,
        std::size_t &pos)
    {
        int len = 0;
        decodeInt(len, byteBuffer, pos);

        RCF_VERIFY(
            pos+len <= byteBuffer.getLength(),
            RCF::Exception(RcfError_Decoding));

        value.assign(byteBuffer.getPtr()+pos, len);
        pos += len;
    }

} // namespace RCF
