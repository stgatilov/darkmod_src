
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_STUBFACTORY_HPP
#define INCLUDE_RCF_STUBFACTORY_HPP

#include <boost/shared_ptr.hpp>

#include <RCF/RcfClient.hpp>
#include <RCF/Tools.hpp>

namespace RCF {

    class I_RcfClient;
    typedef boost::shared_ptr<I_RcfClient> RcfClientPtr;

    class I_StubFactory
    {
    public:
        virtual ~I_StubFactory()
        {}

        virtual RcfClientPtr makeServerStub() = 0;
    };

    template<typename T, typename I1>
    class StubFactory_1 : public I_StubFactory
    {
    public:
        RcfClientPtr makeServerStub()
        {
            boost::shared_ptr<T> tPtr( new T );

            boost::shared_ptr< RCF::I_Deref<T> > derefPtr(
                new RCF::DerefSharedPtr<T>(tPtr) );

            RcfClientPtr rcfClientPtr =
                createServerStub( (I1 *) NULL, (T *) NULL, derefPtr);

            return rcfClientPtr;
        }
    };

    template<typename T, typename I1, typename I2>
    class StubFactory_2 : public I_StubFactory
    {
    public:
        RcfClientPtr makeServerStub()
        {
            boost::shared_ptr<T> tPtr( new T );

            boost::shared_ptr< RCF::I_Deref<T> > derefPtr(
                new RCF::DerefSharedPtr<T>(tPtr) );

            RcfClientPtr rcfClientPtr =
                createServerStub( (I1 *) NULL, (T *) NULL, derefPtr);

            {
                RcfClientPtr mergeePtr = createServerStub(
                    (I2 *) 0,
                    (T *) 0,
                    derefPtr);

                rcfClientPtr->getServerStub().merge(mergeePtr);
            }

            return rcfClientPtr;
        }
    };

    template<typename T, typename I1, typename I2, typename I3>
    class StubFactory_3 : public I_StubFactory
    {
    public:
        RcfClientPtr makeServerStub()
        {
            boost::shared_ptr<T> tPtr( new T );

            boost::shared_ptr< RCF::I_Deref<T> > derefPtr(
                new RCF::DerefSharedPtr<T>(tPtr) );

            RcfClientPtr rcfClientPtr =
                createServerStub( (I1 *) NULL, (T *) NULL, derefPtr);

            {
                RcfClientPtr mergeePtr = createServerStub(
                    (I2 *) 0,
                    (T *) 0,
                    derefPtr);

                rcfClientPtr->getServerStub().merge(mergeePtr);
            }

            {
                RcfClientPtr mergeePtr = createServerStub(
                    (I3 *) 0,
                    (T *) 0,
                    derefPtr);

                rcfClientPtr->getServerStub().merge(mergeePtr);
            }

            return rcfClientPtr;
        }
    };

    template<typename T, typename I1, typename I2, typename I3, typename I4>
    class StubFactory_4 : public I_StubFactory
    {
    public:
        RcfClientPtr makeServerStub()
        {
            boost::shared_ptr<T> tPtr( new T );

            boost::shared_ptr< RCF::I_Deref<T> > derefPtr(
                new RCF::DerefSharedPtr<T>(tPtr) );

            RcfClientPtr rcfClientPtr =
                createServerStub( (I1 *) NULL, (T *) NULL, derefPtr);

            {
                RcfClientPtr mergeePtr = createServerStub(
                    (I2 *) 0,
                    (T *) 0,
                    derefPtr);

                rcfClientPtr->getServerStub().merge(mergeePtr);
            }

            {
                RcfClientPtr mergeePtr = createServerStub(
                    (I3 *) 0,
                    (T *) 0,
                    derefPtr);

                rcfClientPtr->getServerStub().merge(mergeePtr);
            }

            {
                RcfClientPtr mergeePtr = createServerStub(
                    (I4 *) 0,
                    (T *) 0,
                    derefPtr);

                rcfClientPtr->getServerStub().merge(mergeePtr);
            }

            return rcfClientPtr;
        }
    };

} // namespace RCF

#endif // ! INCLUDE_RCF_STUBFACTORY_HPP
