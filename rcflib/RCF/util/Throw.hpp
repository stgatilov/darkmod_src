
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_UTIL_THROW_HPP
#define INCLUDE_UTIL_THROW_HPP

#include <exception>
#include <memory>

#include <boost/current_function.hpp>

#include "Trace.hpp"

#include "VariableArgMacro.hpp"

namespace util {

    namespace detail {

#if defined(_MSC_VER) && _MSC_VER == 1200 && (defined(__SGI_STL_PORT) || defined(_STLPORT_VERSION))

        // for vc6 with stlport
        inline bool uncaught_exception()
        {
            return false;
        }

#else

        inline bool uncaught_exception()
        {
            return std::uncaught_exception();
        }

#endif

        class I_InvokeThrow
        {
        public:
            virtual ~I_InvokeThrow() {}
            virtual void invoke(const std::string &context) = 0;
        };

        template<typename E>
        class InvokeThrow : public I_InvokeThrow
        {
        public:
            InvokeThrow(const E &e) : mE(e)
            {}

            void invoke(const std::string &context)
            {
                const_cast<E &>(mE).setContext(context);
                throw mE;
            }

        private:
            const E &mE;
        };

        template<>
        class InvokeThrow<std::runtime_error> : public I_InvokeThrow
        {
        public:
            InvokeThrow(const std::runtime_error &e) : mE(e)
            {}

            void invoke(const std::string &context)
            {
                throw std::runtime_error( std::string(mE.what()) + ": " + context);
            }

        private:
            const std::runtime_error &mE;
        };

        template<typename T>
        const char *getTypeName(const T &t)
        {
            return typeid(t).name();
        }

    }

    class ThrowFunctor : public VariableArgMacroFunctor
    {
    public:
        // TODO: dummy ctor, for some reason
        ThrowFunctor() : mpTraceChannel(RCF_DEFAULT_INIT), mThrown(RCF_DEFAULT_INIT)
        {}

        template<typename E>
        ThrowFunctor(const E &e, util::TraceChannel &traceChannel) :
            VariableArgMacroFunctor(),
            mInvokeThrow(new detail::InvokeThrow<E>(e)),
            mpTraceChannel(&traceChannel),
            mThrown(RCF_DEFAULT_INIT)
        {}

        ~ThrowFunctor()
        {
            // dtor gets called repeatedly by borland, believe it or not
            if (!mThrown)
            {
                mThrown = true;

                std::string msg = header.str() + args.str();
                mpTraceChannel->getTraceTarget().trace(msg);
                if (!util::detail::uncaught_exception())
                {
                    mInvokeThrow->invoke(msg);
                }
            }
        }

    private:

        std::auto_ptr<detail::I_InvokeThrow> mInvokeThrow;
        util::TraceChannel *mpTraceChannel;
        bool mThrown;
    };


    #ifdef _MSC_VER
    #pragma warning( push )
    #pragma warning( disable : 4355 )  // warning C4355: 'this' : used in base member initializer list
    #endif

    // the const & cast in the macro is for the sake of gcc

    template<typename T>
    const T &identity(const T &t)
    {
        return t;
    }

#ifndef __BORLANDC__

    DECLARE_VARIABLE_ARG_MACRO( UTIL_THROW, ThrowFunctor );
    #define UTIL_THROW(e, channel)                                                                      \
        while (true)                                                                                    \
            (const util::VariableArgMacro<util::ThrowFunctor> &)                                        \
            util::VariableArgMacro<util::ThrowFunctor>(e, channel)                                      \
                .init(                                                                                  \
                "THROW : " + std::string(::util::detail::getTypeName(e)) + " : ",                       \
                    e.what(),                                                                           \
                    __FILE__,                                                                           \
                    __LINE__,                                                                           \
                    BOOST_CURRENT_FUNCTION )                                                            \
                .cast( (util::VariableArgMacro<util::ThrowFunctor> *) NULL)                             \
                .UTIL_THROW_A

    #define UTIL_THROW_A(x)               UTIL_THROW_OP(x, B)
    #define UTIL_THROW_B(x)               UTIL_THROW_OP(x, A)
    #define UTIL_THROW_OP(x, next)        UTIL_THROW_A.notify_((x), #x).UTIL_THROW_ ## next

    #ifdef _MSC_VER
    #pragma warning( pop )
    #endif

#else

    class BorlandThrowFunctor
    {
    public:
        template<typename E>
        BorlandThrowFunctor(const E &e, TraceChannel &traceChannel, const char *file, int line)
        {
            std::ostringstream ostr;
            ostr << file << "(" << line << "): THROW: " << typeid(e).name() << ": " << e.what();
            traceChannel.getTraceTarget().trace(ostr.str());
            throw e;
        }

        BorlandThrowFunctor &inst()
        {
            return *this;
        }

        template<typename T>
            BorlandThrowFunctor & operator()(const T &)
        {
            return *this;
        }
    };

    #define UTIL_THROW(e, channel) (::util::BorlandThrowFunctor(e, channel, __FILE__, __LINE__).inst())

#endif

    #define UTIL_VERIFY(cond, e, channel)       if (cond); else UTIL_THROW(e, channel)

} // namespace util

#endif // ! INCLUDE_UTIL_THROW_HPP
