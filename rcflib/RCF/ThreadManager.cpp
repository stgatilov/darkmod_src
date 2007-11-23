
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#include <RCF/ThreadManager.hpp>

//#include <boost/lambda/bind.hpp>

#include <RCF/InitDeinit.hpp>
#include <RCF/ThreadLocalData.hpp>

namespace RCF {

    // I_ThreadManager

    void I_ThreadManager::setThreadName(const std::string &threadName)
    {
        Lock lock(mInitDeinitMutex);
        mThreadName = threadName;
    }

    std::string I_ThreadManager::getThreadName()
    {
        Lock lock(mInitDeinitMutex);
        return mThreadName;
    }

#ifdef _MSC_VER

    typedef struct tagTHREADNAME_INFO
    {
        DWORD dwType; // must be 0x1000
        LPCSTR szName; // pointer to name (in user addr space)
        DWORD dwThreadID; // thread ID (-1=caller thread)
        DWORD dwFlags; // reserved for future use, must be zero
    } THREADNAME_INFO;

    void SetThreadName( DWORD dwThreadID, LPCSTR szThreadName)
    {
        THREADNAME_INFO info;
        info.dwType = 0x1000;
        info.szName = szThreadName;
        info.dwThreadID = dwThreadID;
        info.dwFlags = 0;

        __try
        {
            RaiseException( 0x406D1388, 0, sizeof(info)/sizeof(DWORD), (ULONG_PTR*)&info );
        }
        __except(EXCEPTION_CONTINUE_EXECUTION)
        {
        }
    }

    void I_ThreadManager::setMyThreadName()
    {
        std::string threadName = getThreadName();
        if (!threadName.empty())
        {
            SetThreadName( DWORD(-1), threadName.c_str());
        }
    }

#else

    void I_ThreadManager::setMyThreadName()
    {}

#endif

    void I_ThreadManager::onInit()
    {
        std::vector<ThreadInitFunctor> initFunctors;
        {
            Lock lock(mInitDeinitMutex);
            std::copy(
                mThreadInitFunctors.begin(), 
                mThreadInitFunctors.end(), 
                std::back_inserter(initFunctors));
        }

        std::for_each(
            initFunctors.begin(), 
            initFunctors.end(), 
            boost::bind(&ThreadInitFunctor::operator(), _1));
    }

    void I_ThreadManager::onDeinit()
    {
        std::vector<ThreadDeinitFunctor> deinitFunctors;
        {
            Lock lock(mInitDeinitMutex);
            std::copy(
                mThreadDeinitFunctors.begin(), 
                mThreadDeinitFunctors.end(), 
                std::back_inserter(deinitFunctors));
        }

        std::for_each(
            deinitFunctors.begin(), 
            deinitFunctors.end(), 
            boost::bind(&ThreadDeinitFunctor::operator(), _1));
    }

    void I_ThreadManager::addThreadInitFunctor(ThreadInitFunctor threadInitFunctor)
    {
        Lock lock(mInitDeinitMutex);
        mThreadInitFunctors.push_back(threadInitFunctor);
    }

    void I_ThreadManager::addThreadDeinitFunctor(ThreadDeinitFunctor threadDeinitFunctor)
    {
        Lock lock(mInitDeinitMutex);
        mThreadDeinitFunctors.push_back(threadDeinitFunctor);
    }

    // FixedThreadPool

    FixedThreadPool::FixedThreadPool(std::size_t threadCount) :
    	mStarted(RCF_DEFAULT_INIT),
        mThreadCount(threadCount)
    {
        RCF1_TRACE("");
    }

    void FixedThreadPool::repeatTask(
        const ThreadInfoPtr &threadInfoPtr,
        const Task &task,
        int timeoutMs,
        const volatile bool &stopFlag)
    {

        RCF1_TRACE("");
        setThreadInfoPtr(threadInfoPtr);

        setMyThreadName();

        onInit();

        bool taskFlag = false;
        while (!stopFlag && !taskFlag)
        {
            try
            {
                while (!stopFlag && !taskFlag)
                {
                    taskFlag = task(timeoutMs, stopFlag, false);
                }
            }
            catch(const std::exception &e)
            {
                RCF1_TRACE("worker thread: exception")(e);
            }
            catch(...)
            {
                RCF1_TRACE("worker thread: non std::exception derived exception");
            }
        }

        onDeinit();

        RCF1_TRACE("")(stopFlag);
    }

    // not synchronized
    void FixedThreadPool::start(const volatile bool &stopFlag)
    {
        RCF1_TRACE("");
        if (!mStarted)
        {
            RCF_ASSERT(mThreads.empty())(mThreads.size());
            mThreads.clear();
            mThreads.resize(mThreadCount);
            for (std::size_t i=0; i<mThreads.size(); ++i)
            {
                ThreadInfoPtr threadInfoPtr( new ThreadInfo( shared_from_this()));

                ThreadPtr threadPtr(new Thread(
                    boost::bind(
                    &FixedThreadPool::repeatTask,
                    this,
                    threadInfoPtr,
                    mTask,
                    1000,
                    boost::ref(stopFlag))));

                mThreads[i] = threadPtr;
            }
            mStarted = true;
        }
    }

    // not synchronized
    void FixedThreadPool::stop(bool wait)
    {
        RCF1_TRACE("")(wait);
        if (mStarted)
        {
            for (std::size_t i=0; i<mThreads.size(); ++i)
            {
                if (mStopFunctor)
                {
                    mStopFunctor();
                }
                if (wait)
                {
                    mThreads[i]->join();
                    mThreads[i].reset();
                }
            }
            mThreads.clear();
            if (wait)
            {
                mStarted = false;
            }
        }
    }

    void FixedThreadPool::setTask(const Task &task)
    {
        RCF_ASSERT(!mStarted);
        mTask = task;
    }

    void FixedThreadPool::setStopFunctor(const StopFunctor &stopFunctor)
    {
        RCF_ASSERT(!mStarted);
        mStopFunctor = stopFunctor;
    }

    // DynamicThreadPool

    DynamicThreadPool::DynamicThreadPool(
        std::size_t threadTargetCount,
        std::size_t threadMaxCount) :
            mStarted(RCF_DEFAULT_INIT),
            mThreadTargetCount(threadTargetCount),
            mThreadMaxCount(threadMaxCount),
            mParkedCount(RCF_DEFAULT_INIT),
            mUnparkCount(RCF_DEFAULT_INIT),
            mStopFlag(RCF_DEFAULT_INIT),
            mpUserStopFlag(RCF_DEFAULT_INIT)
    {
        RCF1_TRACE("");
        RCF_ASSERT(
            mThreadTargetCount < mThreadMaxCount)
            (mThreadTargetCount)(mThreadMaxCount);
    }

    std::size_t DynamicThreadPool::getParkedCount()
    {
        Lock lock(mParkMutex);
        return mParkedCount;
    }

    bool DynamicThreadPool::launchThread(const volatile bool &userStopFlag)
    {
        RCF1_TRACE("")(mParkedCount)(mUnparkCount)(mStopFlag)(userStopFlag);

        RCF_ASSERT(
            mThreads.size() <= mThreadMaxCount)
            (mThreads.size())(mThreadMaxCount);

       // if (static_cast<int>(mThreads.size()) == mThreadMaxCount)
        if (mThreads.size() == mThreadMaxCount)
        {
            return false; // out of threads
        }
        else
        {
            ThreadInfoPtr threadInfoPtr( new ThreadInfo(shared_from_this()));

            ThreadPtr threadPtr( new Thread(
                boost::bind(
                    &DynamicThreadPool::repeatTask,
                    this,
                    threadInfoPtr,
                    mTask,
                    1000,
                    boost::ref(userStopFlag))));

            mThreads.push_back(threadPtr);

            return true;
        }
    }

    void DynamicThreadPool::notifyBusy()
    {
        RCF1_TRACE("")(mParkedCount)(mUnparkCount)(mStopFlag);

        if (!mStopFlag && !getThreadInfoPtr()->mBusy)
        {
            getThreadInfoPtr()->mBusy = true;

            Lock lock(mParkMutex);
            if (mParkedCount == 0)
            {
                !launchThread(*mpUserStopFlag) ? ++mUnparkCount :0;
            }
            else
            {
                mUnparkCondition.notify_one();
                --mParkedCount;
            }
        }
    }

    void DynamicThreadPool::notifyReady()
    {
        RCF1_TRACE("")(mParkedCount)(mUnparkCount)(mStopFlag);

        if (!mStopFlag && getThreadInfoPtr()->mBusy)
        {
            {
                Lock lock(mParkMutex);
                if (mUnparkCount > 0)
                {
                    --mUnparkCount;
                }
                else
                {
                    ++mParkedCount;
                    mUnparkCondition.wait(lock);
                }
            }
           
            getThreadInfoPtr()->mBusy = false;
        }
    }

    void DynamicThreadPool::repeatTask(
        const RCF::ThreadInfoPtr &threadInfoPtr,
        const Task &task,
        int timeoutMs,
        const volatile bool &stopFlag)
    {
        RCF1_TRACE("")(mParkedCount)(mUnparkCount)(mStopFlag);
        setThreadInfoPtr(threadInfoPtr);

        setMyThreadName();

        onInit();

        bool taskFlag = false;
        while (!stopFlag && !taskFlag)
        {
            try
            {
                while (!stopFlag && !taskFlag)
                {
                    taskFlag = task(timeoutMs, stopFlag, false);
                    notifyReady();
                }
            }
            catch(const std::exception &e)
            {
                RCF1_TRACE("worker thread: exception")(e);
            }
            catch(...)
            {
                RCF1_TRACE("worker thread: non std:: derived exception");
            }           
        }

        onDeinit();

        RCF_TRACE("")(stopFlag);
    }

    // not synchronized
    void DynamicThreadPool::start(const volatile bool &stopFlag)
    {
        RCF1_TRACE("")(mParkedCount)(mUnparkCount)(mStopFlag);

        if (!mStarted)
        {
            RCF_ASSERT(mThreads.empty())(mThreads.size());
            mThreads.clear();
            mParkedCount = 0;
            mUnparkCount = 0;
            mStopFlag = false;
            mpUserStopFlag = &stopFlag;

            for (std::size_t i=0; i<mThreadTargetCount; ++i)
            {
                bool ok = launchThread(stopFlag);
                RCF_ASSERT(ok);
            }

            mStarted = true;
        }
    }

    // not synchronized
    void DynamicThreadPool::stop(bool wait)
    {
        RCF1_TRACE("")(mParkedCount)(mUnparkCount)(mStopFlag)(wait);

        if (mStarted)
        {
            {
                Lock lock(mParkMutex);
                mStopFlag = true;
                mUnparkCondition.notify_all();
                mParkedCount = 0;
            }

            for (std::size_t i=0; i<mThreads.size(); ++i)
            {
                if (mStopFunctor)
                {
                    mStopFunctor();
                }
                if (wait)
                {
                    mThreads[i]->join();
                }
            }
            mThreads.clear();
            if (wait)
            {
                mStarted = false;
            }
        }
    }

    void DynamicThreadPool::setTask(const Task &task)
    {
        RCF_ASSERT(!mStarted);
        mTask = task;
    }

    void DynamicThreadPool::setStopFunctor(const StopFunctor &stopFunctor)
    {
        RCF_ASSERT(!mStarted);
        mStopFunctor = stopFunctor;
    }

} // namespace RCF




