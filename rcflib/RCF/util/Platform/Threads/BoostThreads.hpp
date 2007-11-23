
#ifndef INCLUDE_UTIL_PLATFORM_THREADS_BOOSTTHREADS_HPP
#define INCLUDE_UTIL_PLATFORM_THREADS_BOOSTTHREADS_HPP

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4275 ) // warning C4275: non dll-interface class 'std::runtime_error' used as base for dll-interface class 'boost::thread_resource_error'
#pragma warning( disable : 4251 ) // warning C4251: 'boost::thread_group::m_threads' : class 'std::list<_Ty>' needs to have dll-interface to be used by clients of class 'boost::thread_group'
#endif

#include <boost/noncopyable.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/tss.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/xtime.hpp>

#if defined(RCF_USE_BOOST_READ_WRITE_MUTEX)
#include <boost/thread/read_write_mutex.hpp>
#endif

#ifdef _MSC_VER
#pragma warning( pop )
#endif

#include <RCF/util/DefaultInit.hpp>
#include <RCF/util/UnusedVariable.hpp>

namespace Platform {

    namespace Threads {

        typedef boost::thread thread;
        typedef boost::thread_group thread_group;

        template<typename T>
        struct thread_specific_ptr
        {
            typedef boost::thread_specific_ptr<T> Val;
        };

        typedef boost::mutex                    mutex;
        typedef boost::try_mutex                try_mutex;
        typedef boost::recursive_mutex          recursive_mutex;
        typedef boost::recursive_try_mutex      recursive_try_mutex;

        class condition : boost::noncopyable
        {
        public:
            template<typename T> void  wait(const T &t) { mCondition.wait(t); }
            template<typename T> bool timed_wait(const T &t, int timeoutMs)
            {
                boost::xtime xt;
                boost::xtime_get(&xt, boost::TIME_UTC);
                xt.sec += timeoutMs / 1000;
                xt.nsec += 1000*(timeoutMs - 1000*(timeoutMs / 1000));
                return mCondition.timed_wait(t, xt);
            }
            void notify_one() { mCondition.notify_one(); }
            void notify_all() { mCondition.notify_all(); }
        private:
            boost::condition mCondition;
        };

#if defined(RCF_USE_BOOST_READ_WRITE_MUTEX)

        // use the read/write mutex from boost.threads
        // NB: v 1.32 and 1.33.0 have it, 1.33.1 doesn't (removed due to unresolved bugs...)
       
        enum read_write_scheduling_policy
        {
            writer_priority = boost::read_write_scheduling_policy::writer_priority,
            reader_priority = boost::read_write_scheduling_policy::reader_priority,
            alternating_many_reads = boost::read_write_scheduling_policy::alternating_many_reads,
            alternating_single_read = boost::read_write_scheduling_policy::alternating_single_read
        };

        class read_write_mutex : public boost::read_write_mutex
        {
        public:
            read_write_mutex(read_write_scheduling_policy policy) :
                boost::read_write_mutex(boost::read_write_scheduling_policy::read_write_scheduling_policy_enum(policy))
            {}
        };
       
#else

        // simple drop in replacement for boost::read_write_mutex, until there's a final version in boost.threads

        enum read_write_scheduling_policy
        {
            writer_priority
            , reader_priority
            , alternating_many_reads
            , alternating_single_read
        };

        class read_write_mutex;

        namespace detail {

            class scoped_read_lock : boost::noncopyable
            {
            public:
                scoped_read_lock(read_write_mutex &rwm);
                ~scoped_read_lock();
                void lock();
                void unlock();

            private:
                typedef recursive_mutex::scoped_lock    scoped_lock;
                read_write_mutex &                      rwm;
                bool                                    locked;
            };

            class scoped_write_lock : boost::noncopyable
            {
            public:
                scoped_write_lock(read_write_mutex &rwm);
                ~scoped_write_lock();
                void lock();
                void unlock();

            private:
                typedef recursive_mutex::scoped_lock    scoped_lock;
                read_write_mutex &                      rwm;
                scoped_lock                             readLock;
                scoped_lock                             writeLock;
                bool                                    locked;
            };

        } // namespace detail

        class read_write_mutex : boost::noncopyable
        {
        public:
            read_write_mutex(read_write_scheduling_policy rwsp) :
                readerCount(RCF_DEFAULT_INIT)
            {
                RCF_UNUSED_VARIABLE(rwsp);
            }

        private:
           
            typedef recursive_mutex::scoped_lock    scoped_lock;

            void waitOnReadUnlock(scoped_lock &lock)
            {
                readUnlockEvent.wait(lock);
            }

            void notifyReadUnlock()
            {
                readUnlockEvent.notify_all();
            }
           
            recursive_mutex                         readMutex;
            recursive_mutex                         writeMutex;
            condition                               readUnlockEvent;
            int                                     readerCount;

        public:

            typedef detail::scoped_read_lock        scoped_read_lock;
            typedef detail::scoped_write_lock       scoped_write_lock;

            friend class detail::scoped_read_lock;
            friend class detail::scoped_write_lock;

        };

        namespace detail {

            inline scoped_read_lock::scoped_read_lock(read_write_mutex &rwm) :
                rwm(rwm),
                locked(RCF_DEFAULT_INIT)
            {
                lock();
            }

            inline scoped_read_lock::~scoped_read_lock()
            {
                unlock();
            }

            inline void scoped_read_lock::lock()
            {
                if (!locked)
                {
                    {
                        scoped_lock lock( rwm.readMutex );
                        ++rwm.readerCount;
                    }
                    locked = true;
                }
            }

            inline void scoped_read_lock::unlock()
            {
                if (locked)
                {
                    {
                        scoped_lock lock( rwm.readMutex );
                        --rwm.readerCount;
                        rwm.notifyReadUnlock();
                    }
                    locked = false;
                }
            }

            inline scoped_write_lock::scoped_write_lock(read_write_mutex &rwm) :
                rwm(rwm),
                readLock(rwm.readMutex, false),
                writeLock(rwm.writeMutex, false),
                locked(RCF_DEFAULT_INIT)
            {
                lock();
            }

            inline scoped_write_lock::~scoped_write_lock()
            {
                unlock();
            }

            inline void scoped_write_lock::lock()
            {
                if (!locked)
                {
                    readLock.lock();
                    while (rwm.readerCount > 0)
                    {
                        rwm.waitOnReadUnlock(readLock);
                    }
                    writeLock.lock();
                    locked = true;
                }
            }

            inline void scoped_write_lock::unlock()
            {
                if (locked)
                {
                    writeLock.unlock();
                    readLock.unlock();
                    locked = false;
                }
            }

        } // namespace detail

#endif // RCF_USE_BOOST_READ_WRITE_MUTEX

    } // namespace Threads

} // namespace Platform

#endif // ! INCLUDE_UTIL_PLATFORM_THREADS_BOOSTTHREADS_HPP
