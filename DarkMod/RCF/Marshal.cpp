
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#include <RCF/Marshal.hpp>

#include <algorithm>

#include <boost/function.hpp>
#include <boost/serialization/split_free.hpp>

#include <RCF/ClientProgress.hpp>
#include <RCF/SerializationProtocol.hpp>

#ifdef RCF_USE_SF_SERIALIZATION
#include <SF/memory.hpp>
#endif

namespace boost {
    namespace serialization {

        template<typename Archive, typename T>
        void serialize(Archive &ar, std::auto_ptr<T> &apt, const unsigned int version)
        {
            split_free(ar, apt, version);
        }

        template<typename Archive, typename T>
        void save(Archive &ar, const std::auto_ptr<T> &apt, const unsigned int)
        {
            ar & apt.get();
        }

        template<typename Archive, typename T>
        void load(Archive &ar, std::auto_ptr<T> &apt, const unsigned int)
        {
            T *pt = NULL;
            ar & pt;
            apt.reset(pt);
        }

    }
}

namespace RCF {

    namespace IDL {

        void doInReturnValue(
            ClientStub &clientStub,
            SerializationProtocolIn &in,
            SerializationProtocolOut &out,
            bool oneway,
            bool &retry)
        {

            RCF_ASSERT(!retry);

            unsigned int totalTimeoutMs = clientStub.getRemoteCallTimeoutMs();
            unsigned int startTimeMs = getCurrentTimeMs();
            unsigned int endTimeMs = startTimeMs + totalTimeoutMs;
            unsigned int timeoutMs = 0;

            clientStub.verifyTransport();
            I_ClientTransport &connection = clientStub.getTransport();
            const std::vector<FilterPtr> &filters = clientStub.getMessageFilters();

            WithProgressCallback *pWithCallbackProgress =
                dynamic_cast<WithProgressCallback *>(&connection);

            if (pWithCallbackProgress)
            {
                pWithCallbackProgress->setClientProgressPtr(
                    clientStub.getClientProgressPtr());
            }

            // TODO: make sure timeouts behave as expected, esp. when connect() is doing round trip filter setup calls
            clientStub.connect();

            ThreadLocalCached< std::vector<ByteBuffer> > tlcByteBuffers;
            std::vector<ByteBuffer> &byteBuffers = tlcByteBuffers.get();

            out.extractByteBuffers(byteBuffers);
            int protocol = out.getSerializationProtocol();

            ThreadLocalCached< std::vector<ByteBuffer> > tlcEncodedByteBuffers;
            std::vector<ByteBuffer> &encodedByteBuffers = tlcEncodedByteBuffers.get();

            RCF::MethodInvocationRequest &request = clientStub.mRequest;

            request.encodeRequest(
                byteBuffers,
                encodedByteBuffers,
                filters);

            timeoutMs = generateTimeoutMs(endTimeMs);
            int err = connection.send(encodedByteBuffers, timeoutMs);
            RCF_ASSERT(err == 1)(err);

            if (!oneway)
            {
                timeoutMs = generateTimeoutMs(endTimeMs);
                ByteBuffer encodedByteBuffer;
                int err = connection.receive(encodedByteBuffer, timeoutMs);

                RCF_ASSERT(err == 1)(err);

                ByteBuffer unfilteredByteBuffer;

                MethodInvocationResponse response;
                request.decodeResponse(
                    encodedByteBuffer,
                    unfilteredByteBuffer,
                    response,
                    filters);

                in.reset(
                    unfilteredByteBuffer,
                    protocol);

                if (response.isException())
                {
                    std::auto_ptr<RemoteException> remoteExceptionAutoPtr;
                    deserialize(in, remoteExceptionAutoPtr);
                    remoteExceptionAutoPtr->throwCopy();
                }
                else if (response.isError())
                {
                    int err = response.getError();
                    if (err == RcfError_VersionMismatch)
                    {
                        int version = response.getArg0();

                        RCF_VERIFY(
                            version < clientStub.getRcfRuntimeVersion(),
                            Exception(RcfError_Encoding));

                        if (clientStub.getAutoVersioning() && clientStub.getTries() == 0)
                        {
                            clientStub.setRcfRuntimeVersion(version);
                            clientStub.setTries(1);
                            retry = true;
                        }
                        else
                        {
                            RCF_THROW(VersioningException(version));
                        }
                    }
                    else
                    {
                        RCF_THROW(RemoteException(response.getError()))
                            (response.getError());
                    }
                }
            }
        }
    }

} // namespace RCF
