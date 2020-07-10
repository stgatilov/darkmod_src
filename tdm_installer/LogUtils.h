#include "Logging.h"

//reuse logging system and asserts from ZipSync
//note: initially, all logging goes to console (which is hidden on Windows)
using ZipSync::g_logger;
using ZipSync::formatMessage;
using ZipSync::LogCode;
using ZipSync::Severity;
using ZipSync::lcAssertFailed;
using ZipSync::lcCantOpenFile;
using ZipSync::lcMinizipError;
using ZipSync::assertFailedMessage;
using ZipSync::ErrorException;

//after user decides on installation directory,
//we reset logging with this implementation (writes to a file)
class LoggerTdm : public ZipSync::Logger {
public:
	LoggerTdm();
    virtual void Message(LogCode code, Severity severity, const char *message) override;
};
