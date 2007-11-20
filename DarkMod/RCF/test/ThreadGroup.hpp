
#ifndef INCLUDE_RCF_TEST_THREADGROUP_HPP
#define INCLUDE_RCF_TEST_THREADGROUP_HPP

#include <vector>

#include <boost/shared_ptr.hpp>

#include <RCF/ThreadLibrary.hpp>

typedef RCF::Thread Thread;
typedef boost::shared_ptr<Thread> ThreadPtr;
typedef std::vector<ThreadPtr> ThreadGroup;

inline void joinThreadGroup(const ThreadGroup &threadGroup)
{
    for (unsigned int i=0; i<threadGroup.size(); ++i)
    {
        threadGroup[i]->join();
    }
}

#endif // ! INCLUDE_RCF_TEST_THREADGROUP_HPP
