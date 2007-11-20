
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_TOOLS_HPP
#define INCLUDE_RCF_TOOLS_HPP

// Various utilities

#include <stdlib.h>
#include <time.h>

#include <deque>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <typeinfo>
#include <vector>

#include <boost/bind.hpp>
#include <boost/current_function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits.hpp>

#include <RCF/Exception.hpp>
#include <RCF/ThreadLibrary.hpp>
#include <RCF/util/Meta.hpp>
#include <RCF/util/UnusedVariable.hpp>
#include <RCF/util/Platform/OS/BsdSockets.hpp> // GetErrorString()

// Assertion mechanism
#include <RCF/util/Assert.hpp>
#define RCF_ASSERT(x) UTIL_ASSERT(x, RCF::AssertionFailureException(), (*::RCF::pTraceChannels[9]))

// Verification mechanism
#include <RCF/util/Throw.hpp>
#define RCF_VERIFY(cond, e) UTIL_VERIFY(cond, e, (*::RCF::pTraceChannels[9]))

// Trace mechanism
#include <RCF/util/Trace.hpp>
//#define RCF_TRACE(x) DUMMY_VARIABLE_ARG_MACRO()
#define RCF_TRACE(x)  UTIL_TRACE(x, (*::RCF::pTraceChannels[0]))        // for RcfServer layer
#define RCF1_TRACE(x) UTIL_TRACE(x, (*::RCF::pTraceChannels[1]))        // for thread manager layer
#define RCF2_TRACE(x) UTIL_TRACE(x, (*::RCF::pTraceChannels[2]))        // for server transport layer
#define RCF3_TRACE(x) UTIL_TRACE(x, (*::RCF::pTraceChannels[3]))        // for client transport layer
#define RCF4_TRACE(x) UTIL_TRACE(x, (*::RCF::pTraceChannels[4]))        // for client layer
#define RCF5_TRACE(x) UTIL_TRACE(x, (*::RCF::pTraceChannels[5]))        // for init/deinit
#define RCF6_TRACE(x) UTIL_TRACE(x, (*::RCF::pTraceChannels[6]))
#define RCF7_TRACE(x) UTIL_TRACE(x, (*::RCF::pTraceChannels[7]))
#define RCF8_TRACE(x) UTIL_TRACE(x, (*::RCF::pTraceChannels[8]))
#define RCF9_TRACE(x) UTIL_TRACE(x, (*::RCF::pTraceChannels[9]))        // for unexpected stuff (throws, asserts, suppressed dtor errors etc.)

namespace RCF {
    extern util::TraceChannel *pTraceChannels[10];
}

// Throw mechanism
#include <RCF/util/Throw.hpp>
#define RCF_THROW(e)          UTIL_THROW(e, (*::RCF::pTraceChannels[9]))

// Scope guard mechanism
#include <boost/multi_index/detail/scope_guard.hpp>

// assorted tracing conveniences
#ifndef __BORLANDC__
namespace std {
#endif

    // Trace std::vector
    template<typename T>
    std::ostream &operator<<(std::ostream &os, const std::vector<T> &v)
    {
        os << "(";
        std::copy(v.begin(), v.end(), std::ostream_iterator<T>(os, ", "));
        os << ")";
        return os;
    }

    // Trace std::deque
    template<typename T>
    std::ostream &operator<<(std::ostream &os, const std::deque<T> &d)
    {
        os << "(";
        std::copy(d.begin(), d.end(), std::ostream_iterator<T>(os, ", "));
        os << ")";
        return os;
    }


    // Trace type_info
    inline std::ostream &operator<<(std::ostream &os, const std::type_info &ti)
    {
        return os << ti.name();
    }

    // Trace exception
    inline std::ostream &operator<<(std::ostream &os, const std::exception &e)
    {
        return os
            << "Type: " << typeid(e).name()
            << ", What: " << e.what();
    }

    // Trace exception
    inline std::ostream &operator<<(std::ostream &os, const RCF::Exception &e)
    {
        os << "Type: " << typeid(e).name();
        os << ", What: " << e.what();

        os << ", RCF:" << e.getError();
        os << ": " << RCF::getErrorString(e.getError());

        if (e.getSubSystem() == RCF::RcfSubsystem_Os)
        {
            os << ", OS:" << e.getSubSystemError();
            os << ": " << RCF::getOsErrorString(e.getSubSystemError());
        }
        else if (e.getSubSystem() == RCF::RcfSubsystem_Asio)
        {
            os << ", Asio:" << e.getSubSystemError();
        }
        else if (e.getSubSystem() == RCF::RcfSubsystem_Zlib)
        {
            os << ", Zlib:" << e.getSubSystemError();
        }
        else if (e.getSubSystem() == RCF::RcfSubsystem_OpenSsl)
        {
            os << ", OpenSSSL:" << e.getSubSystemError();
        }

        return os;
    }

#ifndef __BORLANDC__
} // namespace std
#endif

namespace RCF {

    // Time in ms since ca 1970, modulo 65536 s (turns over every ~18.2 hrs).
    unsigned int getCurrentTimeMs();

    // Generate a timeout value for the given ending time.
    // Returns zero if endTime <= current time <= endTime+10%of timer resolution, otherwise returns a nonzero duration in ms.
    // Timer resolution as above (18.2 hrs).
    unsigned int generateTimeoutMs(unsigned int endTimeMs);

} // namespace RCF

// narrow/wide string utilities
#include <RCF/util/Tchar.hpp>
namespace RCF {

    typedef util::tstring tstring;

} // namespace RCF


namespace RCF {

    // null deleter, for use with for shared_ptr
    class NullDeleter
    {
    public:
        template<typename T>
            void operator()(T T)
        {}
    };

    class SharedPtrIsNull
    {
    public:
        template<typename T>
        bool operator()(boost::shared_ptr<T> spt) const
        {
            return spt.get() == NULL;
        }
    };

} // namespace RCF

// VC workaround, in case platform headers have defined the min and max macros

#if !defined(_MSC_VER) || (defined(_MSC_VER) && _MSC_VER == 1200) || defined(__SGI_STL_PORT) || defined(_STLPORT_VERSION)
template<typename T>
T _cpp_min(T t1, T t2)
{
    return (t1 <= t2) ? t1 : t2;
}
template<typename T>
T _cpp_max(T t1, T t2)
{
    return (t1 <= t2) ? t2 : t1;
}
#endif

#if defined(min)
#define RCF_MIN _cpp_min
#define RCF_MAX _cpp_max
#else
#define RCF_MIN std::min
#define RCF_MAX std::max
#endif

#include <RCF/util/DefaultInit.hpp>

namespace RCF {

    inline std::string toString(const std::exception &e)
    {
        std::ostringstream os;

        const RCF::Exception *pE = dynamic_cast<const RCF::Exception *>(&e);
        if (pE)
        {
            int err = pE->getError();
            std::string errMsg = RCF::getErrorString(err);
            os << "[RCF:" << err << "] " << errMsg << std::endl;
            if (pE->getSubSystem() == RCF::RcfSubsystem_Os)
            {
                err = pE->getSubSystemError();
                errMsg = Platform::OS::GetErrorString(err);
                os << "[OS:" << err << "] " << errMsg << std::endl;
            }
            os << "[Context] " << pE->getContext() << std::endl;
        }

        os << "[What] " << e.what() << std::endl;
        os << "[Exception type] " << typeid(e).name() << std::endl;
        return os.str();
    }

} // namespace RCF

// destructor try/catch blocks
#define RCF_DTOR_BEGIN                      \
    try {

#define RCF_DTOR_END                        \
    }                                       \
    catch (const std::exception &e)         \
    {                                       \
    if (!util::detail::uncaught_exception()) \
        {                                   \
            throw;                          \
        }                                   \
        else                                \
        {                                   \
            RCF9_TRACE(RCF::toString(e));   \
        }                                   \
    }

// vc6 issues

#if defined(_MSC_VER) && _MSC_VER == 1200

typedef unsigned long ULONG_PTR;

namespace std {

    inline std::ostream &operator<<(std::ostream &os, __int64)
    {
        // TODO
        RCF_ASSERT(0);
        return os;
    }

    inline std::ostream &operator<<(std::ostream &os, unsigned __int64)
    {
        // TODO
        RCF_ASSERT(0);
        return os;
    }

    inline std::istream &operator>>(std::istream &os, __int64 &)
    {
        // TODO
        RCF_ASSERT(0);
        return os;
    }

    inline std::istream &operator>>(std::istream &os, unsigned __int64 &)
    {
        // TODO
        RCF_ASSERT(0);
        return os;
    }

}

#endif

#if (__BORLANDC__ >= 0x560) && defined(_USE_OLD_RW_STL)
#include <boost/thread.hpp>
#include <boost/thread/tss.hpp>
#include <libs/thread/src/thread.cpp>
#include <libs/thread/src/tss.cpp>

static void DummyFuncToGenerateTemplateSpecializations(void)
{
    // http://lists.boost.org/boost-users/2005/06/12412.php

    // This forces generation of
    // boost::thread::thread(boost::function0<void>&)

    boost::function0<void> A = NULL;
    boost::thread B(A);

    // This forces generation of
    // boost::detail::tss::init(boost::function1<void, void *,
    // std::allocator<boost::function_base> > *)
    // but has the consequence of requiring the deliberately undefined function
    // tss_cleanup_implemented

    boost::function1<void, void*>* C = NULL;
    boost::detail::tss D(C);
}

#endif

#if defined(_MSC_VER) && _MSC_VER >= 1310
// need this for 64 bit builds with asio 0.3.7 (boost 1.33.1)
#include <boost/mpl/equal.hpp>
#include <boost/utility/enable_if.hpp>
namespace boost {
    template<typename T>
    inline std::size_t hash_value(
        T t,
        typename boost::enable_if< boost::mpl::equal<T, std::size_t> >::type * = 0)
    {
        return t;
    }
}
#endif

#if defined(_MSC_VER) && _MSC_VER < 1310
#define RCF_PFTO_HACK long
#else
#define RCF_PFTO_HACK
#endif

#define RCF_VERSION 903

// TODO: find a better place for this
namespace RCF {
    // legacy - version number 1
    // 2007-04-26 - version number 2
    extern const int gRcfRuntimeVersion;
} // namespace RCF

#if defined(RCF_USE_BOOST_THREADS) && !defined(RCF_MULTI_THREADED)
#error RCF_MULTI_THREADED must be defined if RCF_USE_BOOST_THREADS is defined
#endif

#ifndef RCF_MULTI_THREADED
#define RCF_SINGLE_THREADED
#endif

#endif // ! INCLUDE_RCF_TOOLS_HPP
