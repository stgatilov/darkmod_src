

//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_THREADMANAGER_HPP
#define INCLUDE_RCF_THREADMANAGER_HPP

#include <vector>

#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#include <RCF/ThreadLibrary.hpp>
#include <RCF/Tools.hpp>

namespace RCF {

    typedef boost::function3<bool, int, const volatile bool &, bool>    Task;
    typedef boost::function0<void>                                      StopFunctor;

    class I_ThreadManager
    {
    public:
        virtual ~I_ThreadManager()
        {}

        // TODO: should start/stop be accessible by anyone other than RcfServer?
        virtual void start(const volatile bool &stopFlag) = 0;
        virtual void stop(bool wait = true) = 0;
        virtual void notifyBusy() {}

        typedef boost::function0<void> ThreadInitFunctor;
        void addThreadInitFunctor(ThreadInitFunctor threadInitFunctor);

        typedef boost::function0<void> ThreadDeinitFunctor;
        void addThreadDeinitFunctor(ThreadDeinitFunctor threadDeinitFunctor);

        void setThreadName(const std::string &threadName);
        std::string getThreadName();

    protected:
        
        Mutex mInitDeinitMutex;
        std::vector<ThreadInitFunctor> mThreadInitFunctors;
        std::vector<ThreadDeinitFunctor> mThreadDeinitFunctors;

        void onInit();
        void onDeinit();

        std::string mThreadName;

        void setMyThreadName();

    private:
        friend class TaskEntry;

    private:
        virtual void notifyReady() {}
        virtual void setTask(const Task &task)  = 0;
        virtual void setStopFunctor(const StopFunctor &stopFunctor) = 0;
    };

    typedef boost::shared_ptr<I_ThreadManager> ThreadManagerPtr;

    typedef unsigned int ThreadId;

    class ThreadInfo
    {
    public:

        ThreadInfo(ThreadManagerPtr threadManagerPtr) :
            mThreadManagerPtr(threadManagerPtr),
            mBusy(RCF_DEFAULT_INIT)
        {}

        virtual ~ThreadInfo()
        {}

        ThreadManagerPtr mThreadManagerPtr;

        bool mBusy;
    };

    typedef boost::shared_ptr<ThreadInfo> ThreadInfoPtr;

    // Thread manager implementations

    // FixedThreadPool

    class FixedThreadPool :
        public I_ThreadManager,
        public boost::enable_shared_from_this<FixedThreadPool>
    {
    public:

        FixedThreadPool(std::size_t threadCount = 1);

    private:

        // start()/stop() are not synchronized
        void start(const volatile bool &stopFlag);
        void stop(bool wait);

        void setTask(const Task &task);
        void setStopFunctor(const StopFunctor &stopFunctor);
        void repeatTask(
            const ThreadInfoPtr &threadInfoPtr,
            const Task &task,
            int timeoutMs,
            const volatile bool &stopFlag);

        Task mTask;
        StopFunctor mStopFunctor;
        bool mStarted;
        std::size_t mThreadCount;
        std::vector<ThreadPtr> mThreads;
    };

    typedef boost::shared_ptr<FixedThreadPool> FixedThreadPoolPtr;

    // DynamicThreadPool

    class DynamicThreadPool :
        public I_ThreadManager,
        public boost::enable_shared_from_this<DynamicThreadPool>
    {
    public:
        DynamicThreadPool(
            std::size_t threadTargetCount = 1,
            std::size_t threadMaxCount = 10);

        std::size_t getParkedCount();
       
    private:

        bool launchThread(const volatile bool &userStopFlag);
        void notifyBusy();
        void notifyReady();
        void repeatTask(
            const RCF::ThreadInfoPtr &threadInfoPtr,
            const Task &task,
            int timeoutMs,
            const volatile bool &stopFlag);
       
        // start()/stop() not synchronized
        void start(const volatile bool &stopFlag);
        void stop(bool wait);
       
        void setTask(const Task &task);
        void setStopFunctor(const StopFunctor &stopFunctor);
       
    private:

        bool mStarted;
        std::size_t mThreadTargetCount;
        std::size_t mThreadMaxCount;
        std::size_t mParkedCount;
        std::size_t mUnparkCount;

        Task mTask;
        StopFunctor mStopFunctor;

        Mutex mParkMutex;
        Condition mUnparkCondition;

        volatile bool mStopFlag;
        const volatile bool *mpUserStopFlag;

        std::vector<ThreadPtr> mThreads;
    };

    typedef boost::shared_ptr<DynamicThreadPool> DynamicThreadPoolPtr;

} // namespace RCF

#endif // ! INCLUDE_RCF_THREADMANAGER_HPP
