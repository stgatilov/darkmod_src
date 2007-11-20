
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_UTIL_PREPOSTCONDITION_HPP
#define INCLUDE_UTIL_PREPOSTCONDITION_HPP

// BCB problems as usual
#ifdef __BORLANDC__
#define DISABLE_PREPOSTCONDITIONS
#endif

#ifdef DISABLE_PREPOSTCONDITIONS
#undef DISABLE_PREPOSTCONDITIONS

//*******************************************************
// Noop prepostcondition implementation

// Provide no-op macros so that client code compiles, even if it doesn't do anything.

namespace util {
    namespace PrePostCondition {
        struct Dummy {
            template<typename T> const Dummy &operator()(const T &) const {
                return *this;
            }
        };
    } // namespace PrePostCondition
} // namespace util

#define UTIL_PRECONDITION(print, fail, condition)       if (false) util::PrePostCondition::Dummy()
#define UTIL_POSTCONDITION(print, fail, condition)      if (false) util::PrePostCondition::Dummy()
#define UTIL_BEGIN_INVARIANT(type)                      void util_PrePostCondition_dummy() {
#define UTIL_DEFINE_INVARIANT(condition)                if (false) util::PrePostCondition::Dummy()
#define UTIL_END_INVARIANT()                            ;}
#define UTIL_POST(arg)                                  arg
#define UTIL_PRE(arg)                                   arg

#else

//*******************************************************
// Proper prepostcondition implementation

#include <cassert>
#include <functional>
#include <sstream>
#include <utility>
#include <vector>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>

#include "Platform/Platform.hpp"
#include "VariableArgMacro.hpp"

// Define the class invariant

namespace util {

    namespace PrePostCondition {

        // Define variable arg macro for use in defining class invariant
        struct InvariantArgsFunctor : public Functor<>
        {
            InvariantArgsFunctor() : invariantArgs(dummy)
            {
                assert(0);
            }

            InvariantArgsFunctor(const std::vector<std::pair<std::string, std::string> > &invariantArgs, const std::string &condition)
                : invariantArgs(const_cast<std::vector<std::pair<std::string, std::string> > &>(invariantArgs)),
                  condition(condition)
            {}

            void deinit()
            {
                invariantArgs.push_back( std::make_pair( condition, this->args.str() ) );
            }

        private:
            std::vector<std::pair<std::string, std::string> > dummy;
            std::vector<std::pair<std::string, std::string> > &invariantArgs;
            std::string condition;
        };

        DECLARE_VARIABLE_ARG_MACRO( UTIL_INVARIANTARGSFUNCTOR, InvariantArgsFunctor );
        #define UTIL_INVARIANTARGSFUNCTOR(invariantArgs, condition)     util::PrePostCondition::VariableArgMacro<util::PrePostCondition::InvariantArgsFunctor>(invariantArgs, #condition).UTIL_INVARIANTARGSFUNCTOR_A
        #define UTIL_INVARIANTARGSFUNCTOR_A(x)                          UTIL_INVARIANTARGSFUNCTOR_OP(x, B)
        #define UTIL_INVARIANTARGSFUNCTOR_B(x)                          UTIL_INVARIANTARGSFUNCTOR_OP(x, A)
        #define UTIL_INVARIANTARGSFUNCTOR_OP(x, next)                   UTIL_INVARIANTARGSFUNCTOR_A.notify_((x), #x).UTIL_INVARIANTARGSFUNCTOR_ ## next

        namespace Invariant {

            static bool dummy1;
            static std::vector<std::pair<std::string, std::string> > dummy2;
            enum Action { Evaluate, GetArgs };
       
        }

    }

   
}

#define UTIL_BEGIN_INVARIANT(type)                                                                                                                                                                                  \
    boost::function<bool()> getInvariantFunctor() { return boost::lambda::bind( &type::getInvariant, this ); }                                                                                                      \
    boost::function<void(std::vector<std::pair<std::string, std::string> > &)> getInvariantArgsFunctor() { return boost::lambda::bind( &type::getInvariantArgs, this, boost::lambda::_1 ); }                        \
    bool getInvariant() { bool ret; invariant(util::PrePostCondition::Invariant::Evaluate, ret,util::PrePostCondition::Invariant::dummy2); return ret; }                                                            \
    void getInvariantArgs(std::vector<std::pair<std::string, std::string> > &invariantArgs) { invariant(util::PrePostCondition::Invariant::GetArgs, util::PrePostCondition::Invariant::dummy1, invariantArgs); }    \
    void invariant(util::PrePostCondition::Invariant::Action action, bool &ret, std::vector<std::pair<std::string, std::string> > &invariantArgs)                                                                   \
    {                                                                                                                                                                                                               \
        ret = true

#define UTIL_DEFINE_INVARIANT(invariant)                                                        \
        ;                                                                                       \
        if (action == util::PrePostCondition::Invariant::Evaluate)                              \
            ret = ret && (invariant) ;                                                          \
        if (action == util::PrePostCondition::Invariant::GetArgs)                               \
            if (!(invariant))                                                                   \
                UTIL_INVARIANTARGSFUNCTOR(invariantArgs, invariant)

#define UTIL_END_INVARIANT()                                                                    \
        ;                                                                                       \
    }

// Defer evaluation of an argument until the containing condition is about to be verified
#define UTIL_POST(arg) util::PrePostCondition::Post(*this, arg)
#define UTIL_PRE

// Define variable arg macros for pre- and post-conditions
namespace util{

    namespace PrePostCondition {

        inline std::string &replace(std::string &s, const std::string &strOld, const std::string &strNew)
        {
            std::string::size_type pos = 0;
            while ((pos = s.find(strOld)) != std::string::npos)
                s.replace(pos, pos + strOld.length(), strNew);
            return s;
        }

        inline std::string &replaceBoostLambdaVarWithPost(std::string &s)
        {
            return replace(s, "util::PrePostCondition::Post(*this, ", "POST(");
        }

        template<typename T>
        inline void writeArg(std::ostream &os, const T &t, const char *name)
        {
            std::string strName = name;
            os << replaceBoostLambdaVarWithPost( strName ) << "=" << t() << ", ";
        }

        template<typename X, typename T>
        inline
        boost::lambda::lambda_functor<
            boost::lambda::identity<
                const T &
            >
        >
        Post(X &, const T &t)
        {
            return boost::lambda::var(t);
        }

        template<typename X, typename T>
        inline
        boost::lambda::lambda_functor<
            boost::lambda::lambda_functor_base<
                boost::lambda::action<1, boost::lambda::function_action<1> >,
                typename boost::lambda::detail::bind_tuple_mapper<T (*const)()>::type
            >
        >
        Post( X &, T(*fun)() )
        {
            return boost::lambda::bind(fun);
        }

        template<typename X, typename T>
        inline
        boost::lambda::lambda_functor<
            boost::lambda::lambda_functor_base<
                boost::lambda::action<2, boost::lambda::function_action<2> >,
                typename boost::lambda::detail::bind_tuple_mapper<T (X::*const)(), X *const>::type
            >
        >
        Post( X &obj, T(X::*fun)() )
        {
            return boost::lambda::bind(fun, &obj);
        }

        template<typename X, typename T>
        inline
        boost::lambda::lambda_functor<
            boost::lambda::lambda_functor_base<
                boost::lambda::action<1, boost::lambda::function_action<1> >,
                typename boost::lambda::detail::bind_tuple_mapper<const boost::function<T ()> >::type
            >
        >
        Post( X &, T(__stdcall *fun)() )
        {
            boost::function<T()> f(fun);
            return boost::lambda::bind(f);
        }

        class Print_ODS
        {
        public:
            void operator()(const std::string &arg)
            {
                Platform::OS::OutputDebugString( arg.c_str() );
            }
        };

        class Fail_Null
        {
        public:
            void operator()()
            {}
        };

        class Fail_Assert
        {
        public:
            void operator()()
            {
                assert(0);
            }
        };

        class DbCFailure : public std::runtime_error
        {
        public:
            DbCFailure(const std::string &msg) : std::runtime_error(msg)
            {}
        };

        class Fail_Throw
        {
            void operator()()
            {
                throw DbCFailure("DbC failure");
            }
        };

        template<typename Print, typename Fail>
        class PrePostConditionFunctor
        {
        public:

            template<typename T> T &cast(T *)
            {
                return dynamic_cast<T &>(*this);
            }

            void deinit()
            {

                if (condition() == false)
                {
                    std::string msg =
                        getType() == PreCondition ?
                        "FAILURE: Precondition: " :
                        "FAILURE: Postcondition: ";

                    std::string strCondition = context.condition;
                    replaceBoostLambdaVarWithPost(strCondition);
                    formatHeader( msg + strCondition + " in " + context.function );
                    formatArgs();
                    print();
                    fail();
                }
                if (invariant() == false)
                {
                    std::string msg =
                        getType() == PreCondition ?
                        "FAILURE: Class invariant (on entry): " :
                        "FAILURE: Class invariant (on exit): ";

                    std::vector<std::pair<std::string, std::string> > invariantArgs;
                    this->invariantArgs( invariantArgs );
                    for (unsigned int i=0; i<invariantArgs.size(); i++)
                    {
                        formatHeader( msg + invariantArgs[i].first + " in " + context.function );
                        formatArgs( invariantArgs[i].second );
                        print();
                        fail();
                    }
                }
            }

            template<typename T> void notify(const T &t, const char *name)
            {
                notify( (boost::lambda::constant(0), t), name );
            }


            template<typename F> void notify(const boost::lambda::lambda_functor<F> &t, const char *name)
            {
                typedef typename boost::lambda::lambda_functor<F>::nullary_return_type T;

                boost::function<T()> f = t;
               
                boost::function< void(std::ostream &) > g =
                    boost::lambda::bind(
                        writeArg< boost::function<T()> >,
                        boost::lambda::_1,
                        f,
                        name
                    );

                this->argFunctors.push_back(g);
            }
           
            PrePostConditionFunctor &setConditionFunctor(boost::function<bool()> condition)
            {
                this->condition = condition;
                return *this;
            }

            PrePostConditionFunctor &setInvariantFunctor(boost::function<bool()> invariant)
            {
                this->invariant = invariant;
                return *this;
            }

            PrePostConditionFunctor &setInvariantArgsFunctor(boost::function<void(std::vector<std::pair<std::string, std::string> > &)> invariantArgs)
            {
                this->invariantArgs = invariantArgs;
                return *this;
            }

            PrePostConditionFunctor &setContext(const char *condition, const char *file, unsigned int line, const char *function)
            {
                Context context = { condition, file, line, function };
                this->context = context;
                return *this;
            }

            enum Type { PreCondition, PostCondition };
            virtual Type getType() = 0;

        private:

            std::string header;
            std::string args;

            boost::function<bool()> condition;
            boost::function<bool()> invariant;
            boost::function<void(std::vector<std::pair<std::string, std::string> > &)> invariantArgs;
            std::vector< boost::function<void(std::ostream &)> > argFunctors;
           
            struct Context {
                const char *condition;
                const char *file;
                unsigned int line;
                const char *function;
            };
            Context context;

            void formatHeader(const std::string &label)
            {
                struct timeb tp;
                ftime(&tp);
                int timestamp = static_cast<int>( (tp.time % 60) * 1000 + tp.millitm );
                int threadid = Platform::OS::GetCurrentThreadId();
                std::ostringstream ostr;
                ostr << context.file << "(" << context.line << "): "
                    << label
                    << ": Thread-id=" << threadid
                    << " : Timestamp(ms)=" << timestamp << ": ";
                   
                header = ostr.str();
            }

            void formatArgs()
            {
                std::ostringstream ostr;

                for (unsigned int i=0; i<argFunctors.size(); i++)
                    argFunctors[i](ostr);

                args = ostr.str();
            }

            void formatArgs(const std::string &args)
            {
                this->args = args;
            }

            void print()
            {
                Print()(header + args + "\n");
            }

            void fail()
            {
                if (!std::uncaught_exception())
                {
                    Fail()();
                }
            }

        };

        template<typename Print = Print_ODS, typename Fail = Fail_Null>
        class PreConditionFunctor : public PrePostConditionFunctor<Print, Fail>
        {
            Type getType() { return PreCondition; }
        };

        template<typename Print = Print_ODS, typename Fail = Fail_Null>
        class PostConditionFunctor : public PrePostConditionFunctor<Print, Fail>
        {
            Type getType() { return PostCondition; }
        };

        DECLARE_VARIABLE_ARG_MACRO_T2( UTIL_PRECONDITION, PreConditionFunctor );
        #define UTIL_PRECONDITION(print, fail, cond)                                                                                \
            util::PrePostCondition::VariableArgMacro<util::PrePostCondition::PreConditionFunctor<print, fail> >()                   \
            .setConditionFunctor(boost::lambda::constant(true) && cond)                                                             \
            .setInvariantFunctor(this->getInvariantFunctor())                                                                       \
            .setInvariantArgsFunctor(this->getInvariantArgsFunctor())                                                               \
            .setContext(#cond, __FILE__, __LINE__, BOOST_CURRENT_FUNCTION)                                                          \
            .cast( (util::PrePostCondition::VariableArgMacro<util::PrePostCondition::PreConditionFunctor<print, fail> > *) NULL )   \
            .UTIL_PRECONDITION_A
        #define UTIL_PRECONDITION_A(x)                  UTIL_PRECONDITION_OP(x, B)
        #define UTIL_PRECONDITION_B(x)                  UTIL_PRECONDITION_OP(x, A)
        #define UTIL_PRECONDITION_OP(x, next)           UTIL_PRECONDITION_A.notify_((x), #x).UTIL_PRECONDITION_ ## next

        DECLARE_VARIABLE_ARG_MACRO_T2( UTIL_POSTCONDITION, PostConditionFunctor );
        #define UTIL_POSTCONDITION(print, fail, cond) UTIL_POSTCONDITION_(print, fail, cond, __LINE__)
        #define UTIL_POSTCONDITION_(print, fail, cond, lineNo) UTIL_POSTCONDITION__(print, fail, cond, lineNo)
        #define UTIL_POSTCONDITION__(print, fail, cond, lineNo) UTIL_POSTCONDITION___(print, fail, cond, instance##lineNo)
        #define UTIL_POSTCONDITION___(print, fail, cond, instance)                                                                      \
            util::PrePostCondition::VariableArgMacro<util::PrePostCondition::PostConditionFunctor<print, fail> > instance; instance     \
            .setConditionFunctor(boost::lambda::constant(true) && cond)                                                                 \
            .setInvariantFunctor(this->getInvariantFunctor())                                                                           \
            .setInvariantArgsFunctor(this->getInvariantArgsFunctor())                                                                   \
            .setContext(#cond, __FILE__, __LINE__, BOOST_CURRENT_FUNCTION)                                                              \
            .cast( (util::PrePostCondition::VariableArgMacro<util::PrePostCondition::PostConditionFunctor<print, fail> > *) NULL )      \
            .UTIL_POSTCONDITION_A
        #define UTIL_POSTCONDITION_A(x)                 UTIL_POSTCONDITION_OP(x, B)
        #define UTIL_POSTCONDITION_B(x)                 UTIL_POSTCONDITION_OP(x, A)
        #define UTIL_POSTCONDITION_OP(x, next)          UTIL_POSTCONDITION_A.notify_((x), #x).UTIL_POSTCONDITION_ ## next

    } // namespace PrePostCondition

} // namespace util

#endif

#define PRECONDITION(condition)     UTIL_PRECONDITION( util::PrePostCondition::Print_ODS, util::PrePostCondition::Fail_Null, condition )
#define POSTCONDITION(condition)    UTIL_POSTCONDITION( util::PrePostCondition::Print_ODS, util::PrePostCondition::Fail_Null, condition )
#define BEGIN_INVARIANT             UTIL_BEGIN_INVARIANT
#define DEFINE_INVARIANT            UTIL_DEFINE_INVARIANT
#define END_INVARIANT               UTIL_END_INVARIANT
#define POST                        UTIL_POST
#define PRE                         UTIL_PRE

/*
//-----------------------------------------
// trivial usage example
#define PRECONDITION(condition)        UTIL_PRECONDITION( util::PrePostCondition::Print_ODS, util::PrePostCondition::Fail_Null, condition )
#define POSTCONDITION(condition)    UTIL_POSTCONDITION( util::PrePostCondition::Print_ODS, util::PrePostCondition::Fail_Null, condition )
#define BEGIN_INVARIANT                UTIL_BEGIN_INVARIANT
#define DEFINE_INVARIANT            UTIL_DEFINE_INVARIANT
#define END_INVARIANT                UTIL_END_INVARIANT
#define POST                        UTIL_POST
#define PRE                            UTIL_PRE

namespace util {

    namespace PrePostCondition {

        namespace example {

            class X {
            public:
                X(float a) : a(a)
                {
                    POSTCONDITION(1);
                }

                ~X()
                {
                    PRECONDITION(1);
                }

                void div(float b)
                {
                    PRECONDITION(b > 0)(b);
                    POSTCONDITION( POST(a) == a/b )(POST(a))(a)(b);
                    a = a/b;
                }

            private:
                float a;

                BEGIN_INVARIANT(X)
                    DEFINE_INVARIANT(a>0)(a)
                END_INVARIANT()

            };

        } // namespace example

    } // namespace PrePostCondition

} // namespace util

#undef PRECONDITION
#undef POSTCONDITION
#undef BEGIN_INVARIANT
#undef DEFINE_INVARIANT
#undef END_INVARIANT
#undef POST
#undef PRE

// end trivial usage example
//-----------------------------------------
*/

#endif //! INCLUDE_UTIL_PREPOSTCONDITION_HPP
