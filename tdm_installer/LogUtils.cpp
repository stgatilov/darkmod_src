#include "LogUtils.h"
#include "OsUtils.h"
#include <time.h>

#ifdef _MSC_VER
#define _WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

static int writeToDebugOutput = -1;
static FILE *logFile = nullptr;

LoggerTdm::LoggerTdm() {
	if (writeToDebugOutput < 0) {
		writeToDebugOutput = 0;
#ifdef _MSC_VER
		writeToDebugOutput = IsDebuggerPresent();
#endif
	}

	if (!logFile) {
		logFile = fopen("tdm_installer.log", "wt");
		if (!logFile)
			throw ErrorException("cannot open log file", lcCantOpenFile);
		time_t timestamp = time(0);
		fprintf(logFile,
			"This is log file of tdm_installer application.\n"
			"Created at %s\n",
			ctime(&timestamp)
		);
		fflush(logFile);
	}
}

void LoggerTdm::Message(LogCode code, Severity severity, const char *message) {
	std::string printedText = formatMessage("%s%s\n",
        severity == ZipSync::sevFatal ? "FATAL: " :
        severity == ZipSync::sevError ? "ERROR: " :
        severity == ZipSync::sevWarning ? "Warning: " : "",
        message
	);

	printf("%s", printedText.c_str());
	fflush(stdout);
#ifdef _MSC_VER
	if (writeToDebugOutput)
		OutputDebugStringA(printedText.c_str());
#endif
	if (logFile) {
		fprintf(logFile, "%s", printedText.c_str());
		fflush(logFile);
	}
}
