
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#include <RCF/Tools.hpp>

#include <RCF/InitDeinit.hpp>

#include <RCF/util/Platform/OS/GetCurrentTime.hpp>

namespace RCF {

    util::TraceChannel *pTraceChannels[10];

    // current time in ms, modulo 65536 s (turns over every ~18.2 hrs)
    unsigned int getCurrentTimeMs()
    {
        return Platform::OS::getCurrentTimeMs();
    }

    // Generate a timeout value for the given ending time.
    // Returns zero if endTime <= current time <= endTime+10% of timer resolution,
    // otherwise returns a nonzero duration in ms.
    // Timer resolution as above (18.2 hrs).
    unsigned int generateTimeoutMs(unsigned int endTimeMs)
    {
        // 90% of the timer interval
        static const unsigned int maxTimeoutMs = (((unsigned int)-1)/10)*9;
        unsigned int currentTimeMs = getCurrentTimeMs();
        unsigned int timeoutMs = endTimeMs - currentTimeMs;
        return (timeoutMs < maxTimeoutMs) ? timeoutMs : 0;
    }

    void deinitRcfTraceChannels()
    {
        for (int i=0; i<10; ++i)
        {
            delete pTraceChannels[i];
            pTraceChannels[i] = NULL;
        }
    }

    void initRcfTraceChannels()
    {
        deinitRcfTraceChannels();
        for (int i=0; i<10; ++i)
        {
            std::string name = "RCF";
            if (i > 0)
            {
                name += '0' + static_cast<char>(i);
            }
            pTraceChannels[i] = new util::TraceChannel(name);
        }

#ifndef NDEBUG
        pTraceChannels[9]->setTraceTarget("ODS");
#endif

    }

    RCF_ON_INIT_DEINIT_NAMED(
        initRcfTraceChannels(),
        deinitRcfTraceChannels(),
        InitRcfTraceChannels);

    const int gRcfRuntimeVersion = 2;

} // namespace RCF
