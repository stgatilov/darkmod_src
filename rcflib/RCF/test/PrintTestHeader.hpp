
#ifndef INCLUDE_RCF_TEST_PRINTTESTHEADER
#define INCLUDE_RCF_TEST_PRINTTESTHEADER

#include <stdlib.h>
#include <iostream>

inline void printTestHeader(const char *file)
{
    std::cout << "\n*********************\n";
    std::cout << file << std::endl;
    time_t now = time(NULL);
    std::cout << "Time now: " << std::string(ctime(&now));
    std::cout << "*********************\n\n";
}

#endif // ! INCLUDE_RCF_TEST_PRINTTESTHEADER