
#ifndef INCLUDE_RCF_TEST_TESTMINIMAL_HPP
#define INCLUDE_RCF_TEST_TESTMINIMAL_HPP

#if (__BORLANDC__ >= 0x560) && defined(_USE_OLD_RW_STL)

int failures = 0;

#define BOOST_CHECK(arg) if (arg); else ++failures;

namespace boost { static const int exit_success = 0; }

int test_main(int argc, char **argv);

int main(int argc, char **argv)
{
    return test_main(argc, argv) + failures;
}

extern "C" void tss_cleanup_implemented(void)
{}

#else

#include <boost/test/minimal.hpp>

#endif

#endif // ! INCLUDE_RCF_TEST_TESTMINIMAL_HPP
