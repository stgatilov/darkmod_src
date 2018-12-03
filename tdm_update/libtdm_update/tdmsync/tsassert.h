#ifndef _TDM_SYNC_ASSERT_H_923469
#define _TDM_SYNC_ASSERT_H_923469

#include <string>

namespace TdmSync {
std::string assertFailedMessage(const char *code, const char *file, int line);
std::string formatMessage(const char *format, ...);
#define TdmSyncAssert(cond) if (!(cond)) throw BaseError(assertFailedMessage(#cond, __FILE__, __LINE__));
#define TdmSyncAssertF(cond, ...) if (!(cond)) throw BaseError(formatMessage(__VA_ARGS__));
}

#endif
