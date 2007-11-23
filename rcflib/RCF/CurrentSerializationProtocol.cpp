
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#include <RCF/CurrentSerializationProtocol.hpp>

#include <RCF/ClientStub.hpp>
#include <RCF/RcfSession.hpp>
#include <RCF/ThreadLocalData.hpp>

namespace RCF {

    SerializationProtocolIn *getCurrentSerializationProtocolIn()
    {
        ClientStubPtr clientStubPtr = RCF::getCurrentClientStubPtr();
        RcfSessionPtr rcfSessionPtr = RCF::getCurrentRcfSessionPtr();
        if (clientStubPtr)
        {
            return &clientStubPtr->mIn;
        }
        else if (rcfSessionPtr)
        {
            return &rcfSessionPtr->mIn;
        }
        else
        {
            return NULL;
        }
    }

    SerializationProtocolOut *getCurrentSerializationProtocolOut()
    {
        ClientStubPtr clientStubPtr = RCF::getCurrentClientStubPtr();
        RcfSessionPtr rcfSessionPtr = RCF::getCurrentRcfSessionPtr();
        if (clientStubPtr)
        {
            return &clientStubPtr->mOut;
        }
        else if (rcfSessionPtr)
        {
            return &rcfSessionPtr->mOut;
        }
        else
        {
            return NULL;
        }
    }

    bool verifyAgainstArchiveSize(std::size_t bytesRequested)
    {

#ifdef RCF_MULTI_THREADED

        RCF::SerializationProtocolIn *pIn = RCF::getCurrentSerializationProtocolIn();

#else

        RCF::SerializationProtocolIn *pIn = NULL;

#endif

        if (pIn)
        {
            std::size_t remainingArchiveLength = pIn->getRemainingArchiveLength();
            if (bytesRequested > remainingArchiveLength)
            {
                RCF_THROW(
                    RCF::SerializationException(RCF::RcfError_OutOfBoundsLength))
                    (bytesRequested)(remainingArchiveLength);
            }
        }

        return pIn ? true : false;
    }

} // namespace RCF
