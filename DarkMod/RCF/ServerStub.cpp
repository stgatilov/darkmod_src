
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#include <RCF/ServerStub.hpp>

#include <iterator>

#include <RCF/RcfClient.hpp>

namespace RCF {

    void ServerStub::invoke(
        const std::string &subInterface,
        int fnId,
        SerializationProtocolIn &in,
        SerializationProtocolOut &out)
    {
        // no mutex here, since there is never anyone writing to mInvokeFunctorMap

        RCF_VERIFY(
            mInvokeFunctorMap.find(subInterface) != mInvokeFunctorMap.end(),
            Exception(RcfError_UnknownInterface))
            (subInterface)(fnId)(mInvokeFunctorMap.size())(mMergedStubs.size());

        mInvokeFunctorMap[subInterface](fnId, in, out);
    }

    void ServerStub::merge(RcfClientPtr rcfClientPtr)
    {
        InvokeFunctorMap &invokeFunctorMap =
            rcfClientPtr->getServerStub().mInvokeFunctorMap;

        std::copy(
            invokeFunctorMap.begin(),
            invokeFunctorMap.end(),
            std::insert_iterator<InvokeFunctorMap>(
                mInvokeFunctorMap,
                mInvokeFunctorMap.begin()));

        invokeFunctorMap.clear();

        mMergedStubs.push_back(rcfClientPtr);
    }

} // namespace RCF
