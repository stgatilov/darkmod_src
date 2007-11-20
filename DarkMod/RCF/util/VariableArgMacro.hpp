
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_UTIL_VARIABLEARGMACRO_HPP
#define INCLUDE_UTIL_VARIABLEARGMACRO_HPP

#include <sstream>
#include <typeinfo>

#include <boost/noncopyable.hpp>

#include "Platform/OS/GetCurrentTime.hpp"
#include "Platform/OS/ThreadId.hpp"

namespace util {

    class VariableArgMacroFunctor : boost::noncopyable
    {
    public:
        virtual ~VariableArgMacroFunctor()
        {}

        VariableArgMacroFunctor &init(
            const std::string &label,
            const std::string &msg,
            const char *file,
            int line,
            const char *func)
        {
            unsigned int timestamp = Platform::OS::getCurrentTimeMs();
            int threadid = Platform::OS::GetCurrentThreadId();
           
            header
                << file << "(" << line << "): "
                << func << ": "
                << ": Thread-id=" << threadid
                << " : Timestamp(ms)=" << timestamp << ": "
                << label << msg << ": " ;

            return *this;
        }

        void deinit()
        {
            args << std::endl;
        }

        template<typename T>
        void notify(const T &t, const char *name)
        {
            args << name << "=" << t << ", ";
        }

        void notify(size_t t, const char *name)
        {
            args << name << "=" << static_cast<unsigned int>(t) << ", ";
        }

#if defined(_MSC_VER) && _MSC_VER <= 1200
        void notify(__int64 t, const char *name)
        {
            //args << name << "=" << t << ", ";
        }
#endif

        // TODO: fix this properly
#ifdef _UNICODE
        void notify(const wchar_t *, const char *) { /*TODO*/ }
        void notify(const std::wstring &, const char *) { /*TODO*/ }
#endif

        template<typename T> T &cast()
        {
            return dynamic_cast<T &>(*this);
        }

        template<typename T> T &cast(T *)
        {
            return dynamic_cast<T &>(*this);
        }

        std::ostringstream header;
        std::ostringstream args;
    };

    class DummyVariableArgMacroObject
    {
    public:
        template<typename T>
        DummyVariableArgMacroObject &operator()(const T &)
        {
            return *this;
        }
    };

    #define DUMMY_VARIABLE_ARG_MACRO() \
        if (false) ::util::DummyVariableArgMacroObject()

} // namespace util


#define DECLARE_VARIABLE_ARG_MACRO(macro_name, functor)                                             \
    DECLARE_VARIABLE_ARG_MACRO_(macro_name, macro_name##_A, macro_name##_B, functor)

#define DECLARE_VARIABLE_ARG_MACRO_( macro_name, macro_name_A, macro_name_B, functor)               \
    template<typename Functor>                                                                      \
    class VariableArgMacro;                                                                         \
                                                                                                    \
    template<> class                                                                                \
    VariableArgMacro< functor > : public functor                                                    \
    {                                                                                               \
    public:                                                                                         \
        VariableArgMacro() :                                                                        \
            macro_name_A(*this), macro_name_B(*this)                                                \
        {}                                                                                          \
        template<typename T1>                                                                       \
        VariableArgMacro(const T1 &t1) :                                                            \
            functor(t1), macro_name_A(*this), macro_name_B(*this)                                   \
        {}                                                                                          \
        template<typename T1, typename T2>                                                          \
        VariableArgMacro(const T1 &t1, const T2 &t2) :                                              \
            functor(t1,t2), macro_name_A(*this), macro_name_B(*this)                                \
        {}                                                                                          \
        template<typename T1, typename T2>                                                          \
        VariableArgMacro(const T1 &t1, T2 &t2) :                                              \
            functor(t1,t2), macro_name_A(*this), macro_name_B(*this)                                \
        {}                                                                                          \
        ~VariableArgMacro()                                                                         \
        {                                                                                           \
            functor::deinit();                                                                      \
        }                                                                                           \
        VariableArgMacro &macro_name_A;                                                             \
        VariableArgMacro &macro_name_B;                                                             \
        template<typename T>                                                                        \
        VariableArgMacro< functor > &notify_(const T &t, const char *name)                          \
        {                                                                                           \
            notify(t,name); return *this;                                                           \
        }                                                                                           \
        VariableArgMacro< functor > &notify_(size_t t, const char *name)                            \
        {                                                                                           \
            functor::notify(t,name); return *this;                                                  \
        }                                                                                           \
    }

#define DECLARE_VARIABLE_ARG_MACRO_T1( macro_name, functor )                                        \
    DECLARE_VARIABLE_ARG_MACRO_T1_(macro_name, macro_name##_A, macro_name##_B, functor)

#define DECLARE_VARIABLE_ARG_MACRO_T1_( macro_name, macro_name_A, macro_name_B, functor)            \
    template<typename Functor>                                                                      \
    class VariableArgMacro;                                                                         \
                                                                                                    \
    template<typename U>                                                                            \
    class VariableArgMacro< functor<U> > : public functor<U>                                        \
    {                                                                                               \
    public:                                                                                         \
        VariableArgMacro() :                                                                        \
            macro_name_A(*this), macro_name_B(*this)                                                \
        {}                                                                                          \
        template<typename T1>                                                                       \
        VariableArgMacro(const T1 &t1) :                                                            \
            functor<U>(t1), macro_name_A(*this), macro_name_B(*this)                                \
        {}                                                                                          \
        template<typename T1, typename T2>                                                          \
        VariableArgMacro(const T1 &t1, const T2 &t2) :                                              \
            functor<U>(t1,t2), macro_name_A(*this), macro_name_B(*this)                             \
        {}                                                                                          \
        ~VariableArgMacro()                                                                         \
        {                                                                                           \
            functor<U>::deinit();                                                                   \
        }                                                                                           \
        VariableArgMacro &macro_name_A;                                                             \
        VariableArgMacro &macro_name_B;                                                             \
        template<typename T>                                                                        \
        VariableArgMacro< functor<U> > &notify_(const T &t, const char *name)                       \
        {                                                                                           \
            notify(t,name);                                                                         \
            return *this;                                                                           \
        }                                                                                           \
        VariableArgMacro< functor<U> > &notify_(size_t t, const char *name)                         \
        {                                                                                           \
            functor<U>::notify(t,name);                                                             \
            return *this;                                                                           \
        }                                                                                           \
    }

#define DECLARE_VARIABLE_ARG_MACRO_T2( macro_name, functor )                                        \
    DECLARE_VARIABLE_ARG_MACRO_T2_(macro_name, macro_name##_A, macro_name##_B, functor)

#define DECLARE_VARIABLE_ARG_MACRO_T2_( macro_name, macro_name_A, macro_name_B, functor)            \
    template<typename Functor>                                                                      \
    class VariableArgMacro;                                                                         \
                                                                                                    \
    template<typename U, typename V>                                                                \
    class VariableArgMacro< functor<U,V> > : public functor<U,V>                                    \
    {                                                                                               \
    public:                                                                                         \
        VariableArgMacro() :                                                                        \
            macro_name_A(*this), macro_name_B(*this)                                                \
        {}                                                                                          \
        template<typename T1>                                                                       \
        VariableArgMacro(const T1 &t1) :                                                            \
            functor<U,V>(t1), macro_name_A(*this), macro_name_B(*this)                              \
        {}                                                                                          \
        template<typename T1, typename T2>                                                          \
        VariableArgMacro(const T1 &t1, const T2 &t2) :                                              \
            functor<U,V>(t1,t2), macro_name_A(*this), macro_name_B(*this)                           \
        {}                                                                                          \
        ~VariableArgMacro()                                                                         \
        {                                                                                           \
            functor<U>::deinit();                                                                   \
        }                                                                                           \
        VariableArgMacro &macro_name_A;                                                             \
        VariableArgMacro &macro_name_B;                                                             \
        template<typename T>                                                                        \
        VariableArgMacro< functor<U,V> > &notify_(const T &t, const char *name)                     \
        {                                                                                           \
            notify(t,name);                                                                         \
            return *this;                                                                           \
        }                                                                                           \
        VariableArgMacro< functor<U,V> > &notify_(size_t t, const char *name)                       \
        {                                                                                           \
            functor<U,V>::notify(t,name);                                                           \
            return *this;                                                                           \
        }                                                                                           \
    }

#endif // ! INCLUDE_UTIL_VARIABLEARGMACRO_HPP
