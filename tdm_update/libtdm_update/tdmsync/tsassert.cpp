#include "tsassert.h"
#include <stdarg.h>
#include <stdio.h>

namespace TdmSync {

std::string assertFailedMessage(const char *code, const char *file, int line) {
    char buff[256];
    sprintf(buff, "Assertion %s failed in %s on line %d", code, file, line);
    return buff;
}

std::string formatMessage(const char *format, ...) {
    char buff[16<<10];
    va_list args;
    va_start(args, format);
    vsnprintf(buff, sizeof(buff), format, args);
    va_end(args);
    return buff;
}

}
