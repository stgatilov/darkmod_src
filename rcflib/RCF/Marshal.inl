
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_MARSHAL_INL_
#define INCLUDE_RCF_MARSHAL_INL_

#include <RCF/ClientTransport.hpp>
#include <RCF/Marshal.hpp>
#include <RCF/Exception.hpp>
#include <RCF/Tools.hpp>

namespace RCF {

    namespace IDL {

        void doInReturnValue(
            ClientStub &clientStub,
            SerializationProtocolIn &in,
            SerializationProtocolOut &out,
            bool oneway,
            bool &retry);

        template<typename T>
        InReturnValue_Value<T>::InReturnValue_Value(
            ClientStub &clientStub,
            SerializationProtocolIn &in,
            SerializationProtocolOut &out,
            bool oneway,
            bool &retry) :
                mT()
        {
            doInReturnValue(clientStub, in, out, oneway, retry);
            if (!oneway && !retry)
            {
                deserialize(in, mT);
            }
        }

        template<typename T>
        T &InReturnValue_Value<T>::get()
        {
            return mT;
        }

        template<typename T>
        InReturnValue_Enum<T>::InReturnValue_Enum(
            ClientStub &clientStub,
            SerializationProtocolIn &in,
            SerializationProtocolOut &out,
            bool oneway,
            bool &retry) :
                mT( T()) // vc 7.1 idiosyncrasy as I recall
        {
            doInReturnValue(clientStub, in, out, oneway, retry);
            if (!oneway && !retry)
            {
                deserialize(in, mT);
            }
        }

        template<typename T>
        T &InReturnValue_Enum<T>::get()
        {
            return mT;
        }

        inline InReturnValue_Value<Void>::InReturnValue_Value(
            ClientStub &clientStub,
            SerializationProtocolIn &in,
            SerializationProtocolOut &out,
            bool oneway,
            bool &retry)
        {
            doInReturnValue(clientStub, in, out, oneway, retry);
        }

    } // namespace IDL

} // namespace RCF

#endif // ! INCLUDE_RCF_MARSHAL_INL_
