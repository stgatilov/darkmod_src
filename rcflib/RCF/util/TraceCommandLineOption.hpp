
//******************************************************************************
// Copyright (c) 2003. All rights reserved.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_UTIL_TRACECOMMANDLINEOPTION_HPP
#define INCLUDE_UTIL_TRACECOMMANDLINEOPTION_HPP

#include "CommandLine.hpp"
#include "InitDeinit.hpp"
#include "Trace.hpp"

namespace util {

    // TraceCommandLineOption

    class TraceCommandLineOption : public CommandLineOption<std::string>
    {
    public:
        TraceCommandLineOption() :
            CommandLineOption<std::string>("trace", "", "-trace <name>,<target>")
        {}

        void on_notify_end()
        {
            // Go through values, making calls to util::Trace::setTraceChannel()
            const std::vector<std::string> &values = getValues();
            for (unsigned int i=0; i<values.size(); i++)
            {
                std::string traceChannelName;
                std::string traceTargetName;
                std::string value = values[i];
                util::Scan( value, "%1,%2" )(traceChannelName)(traceTargetName);
                TraceManager::getSingleton().makeTraceTarget(traceTargetName);
                if (traceChannelName == "*")
                {
                    std::vector<std::string> names = TraceManager::getSingleton().getTraceChannelNames();
                    for (unsigned int i=0; i<names.size(); i++)
                    {
                        TraceChannel &channel = TraceManager::getSingleton().getTraceChannel(names[i]);
                        channel.setTraceTarget(traceTargetName);
                    }
                }
                else if (TraceManager::getSingleton().existsTraceChannel(traceChannelName))
                {
                    TraceChannel &channel = TraceManager::getSingleton().getTraceChannel(traceChannelName);
                    channel.setTraceTarget(traceTargetName);
                }
                else
                {
                    std::cout
                        << "Unrecognized trace channel name: " << traceChannelName
                        << " (use -tracechannels option to list available trace channels).\n";
                }
            }
        }

    };


    // TraceChannelsCommandLineOption

    class TraceChannelsCommandLineOption : public CommandLineOption<bool>
    {
    public:

        TraceChannelsCommandLineOption() :
          CommandLineOption<bool>("tracechannels", false, "dump names and targets for all trace channels")
          {}

          void on_notify_end()
          {
              if (CommandLineOption<bool>::get() == true)
              {
                  std::vector<std::string> names = TraceManager::getSingleton().getTraceChannelNames();
                  for (unsigned int i=0; i<names.size(); i++)
                  {
                      TraceChannel &channel = TraceManager::getSingleton().getTraceChannel(names[i]);
                      std::string traceChannelName = channel.getName();
                      std::string traceTargetName = channel.getTraceTarget().getName();
                      std::cout << traceChannelName << " -> " << traceTargetName << "\n";
                  }
                  exit(0);
              }
          }

    };

    UTIL_ON_INIT_NAMED( TraceManager::getSingleton(); static TraceChannelsCommandLineOption traceChannelsCommandLineOption, TraceInitialize2 )
    UTIL_ON_INIT_NAMED( TraceManager::getSingleton(); static TraceCommandLineOption traceCommandLineOption, TraceInitialize3 )

} // namespace util

#endif // ! INCLUDE_UTIL_TRACECOMMANDLINEOPTION_HPP
