
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_SERVERTASK_HPP
#define INCLUDE_RCF_SERVERTASK_HPP

#include <utility>
#include <vector>

#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#include <RCF/ThreadLibrary.hpp>
#include <RCF/ThreadManager.hpp>
#include <RCF/Tools.hpp>

namespace RCF {

    class TaskEntry
    {
    public:
        TaskEntry(
            Task task,
            StopFunctor stopFunctor,
            const std::string &threadName = "",
            ThreadManagerPtr threadManagerPtr = ThreadManagerPtr()) :
                mTask(task),
                mStopFunctor(stopFunctor)
        {
            if (!threadManagerPtr)
            {
                threadManagerPtr.reset(new FixedThreadPool(1));
            }
            threadManagerPtr->setThreadName(threadName);
            setThreadManagerPtr(threadManagerPtr);
        }

        I_ThreadManager &getThreadManager()
        {
            return *getThreadManagerPtr();
        }

        ThreadManagerPtr getThreadManagerPtr()
        {
            return mThreadManagerPtr;
        }

        void setThreadManagerPtr(ThreadManagerPtr threadManagerPtr)
        {
            mThreadManagerPtr = threadManagerPtr;
            threadManagerPtr->setTask(mTask);
            threadManagerPtr->setStopFunctor(mStopFunctor);
        }

        Task getTask()
        {
            return mTask;
        }

        void start(const volatile bool &stopFlag)
        {
            getThreadManager().start(stopFlag);
        }

        void stop(bool wait = true)
        {
            getThreadManager().stop(wait);
        }

    private:
        Task                mTask;
        StopFunctor         mStopFunctor;
        ThreadManagerPtr    mThreadManagerPtr;
    };

    typedef std::vector<TaskEntry> TaskEntries;

} // namespace RCF

#endif // ! INCLUDE_RCF_SERVERTASK_HPP
