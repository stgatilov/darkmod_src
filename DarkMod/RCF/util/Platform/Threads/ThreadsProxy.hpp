
#ifndef INCLUDE_UTIL_PLATFORM_BOOST_THREADSPROXY_HPP
#define INCLUDE_UTIL_PLATFORM_BOOST_THREADSPROXY_HPP

// Single-threaded proxy for boost threads

#include <memory>

namespace Platform {

    namespace Threads {

        class thread {
        public:
            template<typename T> thread(      T &t) { /*t();*/ }
            template<typename T> thread(const T &t) { /*const_cast<T &>(t)();*/ }
            void join() {}
        };

        class thread_group {
        public:
            template<typename T> thread *create_thread(const T &t) { const_cast<T &>(t)(); return NULL; }
            void add_thread(thread *) {}
            void remove_thread(thread *) {}
            void join_all() {}
        };

        template<typename T>
        struct thread_specific_ptr {
            typedef std::auto_ptr<T> Val;
        };

        struct lock_t {
            template<typename T1> lock_t(const T1 &) {}
            template<typename T1, typename T2> lock_t(const T1 &, const T2 &t2) {}
            bool locked() { return true; }
            void lock() {}
            void unlock() {}
        };

        struct mutex                    { typedef lock_t scoped_lock; };
        struct recursive_mutex          { typedef lock_t scoped_lock; };
        struct recursive_try_mutex      { typedef lock_t scoped_lock; typedef lock_t scoped_try_lock; };
        struct try_mutex                { typedef lock_t scoped_lock; typedef lock_t scoped_try_lock; };

        enum read_write_scheduling_policy {
            writer_priority,
            reader_priority,
            alternating_many_reads,
            alternating_single_read
        };
       
        struct read_write_mutex {
            read_write_mutex(read_write_scheduling_policy) {}
            typedef lock_t scoped_read_lock;
            typedef lock_t scoped_write_lock;
            typedef lock_t scoped_read_write_lock;
        };

        struct condition {
            condition() {}
            template<typename T> void  wait(const T &) {}
            template<typename T> bool timed_wait(const T &t, int timeoutMs) { return true; }
            void notify_one() {}
            void notify_all() {}
        };

    } // namespace Threads

} // namespace Platform

#endif // ! INCLUDE_UTIL_PLATFORM_BOOST_THREADSPROXY_HPP
