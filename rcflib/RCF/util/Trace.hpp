   
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_UTIL_TRACE_HPP
#define INCLUDE_UTIL_TRACE_HPP

#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>

#include <boost/current_function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include "InitDeinit.hpp"
#include "Scan.hpp"
#include "ThreadLibrary.hpp"
#include "Throw.hpp"
#include "UnusedVariable.hpp"
#include "VariableArgMacro.hpp"

#include "Platform/OS/OutputDebugString.hpp"

//*************************************************
// Trace utility

namespace util {

    // TraceTarget

    class TraceTarget : boost::noncopyable
    {
    public:
        virtual ~TraceTarget() {}
        virtual std::string getName() = 0;
        virtual bool isNull() = 0;
        virtual void trace(const std::string &msg) = 0;
    };
   
    class TraceTargetNull : public TraceTarget
    {
    public:
        std::string getName() { return "null"; }
        bool isNull() { return true; }
        void trace(const std::string &msg) { RCF_UNUSED_VARIABLE(msg); }
    };

    class TraceTargetOds : public TraceTarget
    {
    public:

        std::string getName()
        {
            return "ODS";
        }

        bool isNull()
        {
            return false;
        }
       
        void trace(const std::string &msg)
        {
            Lock lock(m); RCF_UNUSED_VARIABLE(lock);
            Platform::OS::OutputDebugString( msg.c_str() );
        }
    private:
        Mutex m;
    };

    class TraceTargetStdout : public TraceTarget
    {
    public:

        std::string getName()
        {
            return "stdout";
        }

        bool isNull()
        {
            return false;
        }

        void trace(const std::string &msg)
        {
            Lock lock(m); RCF_UNUSED_VARIABLE(lock);
            std::cout << msg;
            std::cout.flush();
        }
    private:
        Mutex m;
    };

    class TraceTargetStderr : public TraceTarget
    {
    public:

        std::string getName()
        {
            return "stderr";
        }

        bool isNull()
        {
            return false;
        }

        void trace(const std::string &msg)
        {
            Lock lock(m); RCF_UNUSED_VARIABLE(lock);
            std::cerr << msg;
            std::cerr.flush();
        }
    private:
        Mutex m;
    };

    class TraceTargetFile : public TraceTarget
    {
    public:
        TraceTargetFile( const std::string &filename ) :
            fout( new std::ofstream(filename.c_str())),
            filename(filename)
        {}

        bool isNull()
        {
            return false;
        }

        std::string getName()
        {
            return filename;
        }

        void trace(const std::string &msg)
        {
            Lock lock(m); RCF_UNUSED_VARIABLE(lock);
            *fout << msg;
            fout->flush();
        }

    private:
        Mutex m;
        std::auto_ptr<std::ofstream> fout;
        std::string filename;
    };

    // TraceChannel

    class TraceChannel
    {
    public:
        TraceChannel(const std::string &traceChannelName);
       
        void trace(const std::string &msg)
        {
            traceTarget->trace(msg);
        }

        void setTraceTarget(const std::string &traceTargetName);

        TraceTarget &getTraceTarget() { return *traceTarget; }

        std::string getName() const { return traceChannelName; }

    private:
        const std::string traceChannelName;
        boost::shared_ptr<TraceTarget> traceTarget;
    };


    // TraceManager

    class TraceManager : boost::noncopyable
    {
    private:
        TraceManager()
        {
            makeTraceTarget("", boost::shared_ptr<TraceTarget>( new TraceTargetOds ) );
            makeTraceTarget("ODS", boost::shared_ptr<TraceTarget>( new TraceTargetOds ) );
            makeTraceTarget("stdout", boost::shared_ptr<TraceTarget>( new TraceTargetStdout ) );
            makeTraceTarget("stderr", boost::shared_ptr<TraceTarget>( new TraceTargetStderr ) );
            makeTraceTarget("null", boost::shared_ptr<TraceTarget>( new TraceTargetNull ) );
        }

        static TraceManager *&singletonPtr()
        {
            static TraceManager *pTraceManager = NULL;
            return pTraceManager;
        }

    public:

        static TraceManager &getSingleton()
        {
            return *getSingletonPtr();
        }

        static TraceManager *getSingletonPtr()
        {
            if (singletonPtr() == NULL)
            {
                delete singletonPtr();
                singletonPtr() = new TraceManager;
            }
            return singletonPtr();
        }

        static void deleteSingletonPtr()
        {
            delete singletonPtr();
            singletonPtr() = NULL;
        }

    public:

        bool existsTraceChannel(const std::string &name)
        {
            return traceChannelMap.find(name) != traceChannelMap.end();
        }

        TraceChannel &getTraceChannel(const std::string &name)
        {
            TraceChannelMap::const_iterator it = traceChannelMap.find(name);
            if (it == traceChannelMap.end())
            {
                // TODO: put in a .cpp file!
                //UTIL_THROW(std::runtime_error("trace channel does not exist"))(name);
                throw std::runtime_error("trace channel does not exist");
            }
            TraceChannel *pTraceChannel = (*it).second;
            return *pTraceChannel;
        }

        std::vector<std::string> getTraceChannelNames()
        {
            std::vector<std::string> names;
            for (
                TraceChannelMap::const_iterator it = traceChannelMap.begin();
                it != traceChannelMap.end();
                it++)
                {
                    names.push_back( (*it).second->getName() );
                }
            return names;
        }

        boost::shared_ptr<TraceTarget> getTraceTarget(const std::string &traceTargetName)
        {
            if (traceTargetMap.find(traceTargetName) == traceTargetMap.end())
            {
                // TODO: put in a .cpp file!
                //UTIL_THROW(std::runtime_error("trace target does not exist"))(traceTargetName);
                throw std::runtime_error("trace channel does not exist");
            }
            return traceTargetMap[traceTargetName];
        }

        void makeTraceTarget(const std::string &traceTargetName)
        {
            makeTraceTarget(
                traceTargetName,
                boost::shared_ptr<TraceTarget>( new TraceTargetFile(traceTargetName) ));
        }

        void makeTraceTarget(const std::string &traceTargetName, boost::shared_ptr<TraceTarget> traceTarget)
        {
            if (traceTargetMap.find(traceTargetName) == traceTargetMap.end())
            {
                traceTargetMap[traceTargetName] = traceTarget;
            }
        }

        TraceChannel &getUtilTraceChannel()
        {
            if (NULL == utilTraceChannel.get())
            {
                utilTraceChannel.reset( new TraceChannel("util") );
            }
            return *utilTraceChannel;
        }

        friend class TraceChannel; // so the TraceChannel objects can register themselves

        void registerTraceChannel(TraceChannel &traceChannel)
        {
            std::string name = traceChannel.getName();
            traceChannelMap[name] = &traceChannel;
        }

        typedef std::map<std::string, TraceChannel *> TraceChannelMap;
        TraceChannelMap traceChannelMap;

        typedef std::map<std::string, boost::shared_ptr<TraceTarget> > TraceTargetMap;
        TraceTargetMap traceTargetMap;

        std::auto_ptr<TraceChannel> utilTraceChannel;

    };

    inline TraceChannel::TraceChannel(const std::string &traceChannelName) :
        traceChannelName(traceChannelName)
    {
        setTraceTarget("null");
        TraceManager::getSingleton().registerTraceChannel(*this);
    }

    inline void TraceChannel::setTraceTarget(const std::string &traceTargetName)
    {
        traceTarget = TraceManager::getSingleton().getTraceTarget(traceTargetName);
    }


   
    // TraceFunctor, for variable arg macro semantics

    class TraceFunctor : public VariableArgMacroFunctor
    {
    public:
        TraceFunctor &init_trace(TraceChannel *traceChannel)
        {
            this->traceChannel = traceChannel;
            return *this;
        }

        void deinit()
        {
            std::string msg = header.str() + args.str() + "\n";
            traceChannel->trace(msg);
        }

    private:
        TraceChannel *traceChannel;
    };

    #ifdef _MSC_VER
    #pragma warning( push )
    #pragma warning( disable : 4355 )  // warning C4355: 'this' : used in base member initializer list
    #endif

    DECLARE_VARIABLE_ARG_MACRO( UTIL_TRACE, TraceFunctor );
    #define UTIL_TRACE(msg, channel)                                                \
        if (!channel.getTraceTarget().isNull())                                     \
            (const util::VariableArgMacro<util::TraceFunctor> &)                    \
            util::VariableArgMacro<util::TraceFunctor>()                            \
                .init_trace(&channel)                                               \
                .init(                                                              \
                    "TRACE: ",                                                      \
                    msg,                                                            \
                    __FILE__,                                                       \
                    __LINE__,                                                       \
                    BOOST_CURRENT_FUNCTION )                                        \
                .cast( (util::VariableArgMacro<util::TraceFunctor> *) NULL )        \
                .UTIL_TRACE_A

    #define UTIL_TRACE_A(x)                         UTIL_TRACE_OP(x, B)
    #define UTIL_TRACE_B(x)                         UTIL_TRACE_OP(x, A)
    #define UTIL_TRACE_OP(x, next)                  UTIL_TRACE_A.notify_((x), #x).UTIL_TRACE_ ## next

    #ifdef _MSC_VER
    #pragma warning( pop )
    #endif

    #define UTIL_TRACE_DEFAULT(msg)                                                 \
        UTIL_TRACE(msg, ::TraceManager::getSingleton().getUtilTraceChannel())

    // auto initialization
    UTIL_ON_INIT_NAMED( TraceManager::getSingleton().getUtilTraceChannel(), TraceInitialize1 )
    UTIL_ON_DEINIT_NAMED( TraceManager::deleteSingletonPtr(), TraceDeinitialize1 )

}

#endif //! INCLUDE_UTIL_TRACE_HPP
