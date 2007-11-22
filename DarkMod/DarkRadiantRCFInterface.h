#ifndef __RCF_SERVICE_DECL_H__
#define __RCF_SERVICE_DECL_H__

#include <string>
#include <RCF/Idl.hpp>
#include <SF/vector.hpp>

/**
 * greebo: This defines the service methods for DarkRadiant.
 *         Keep this in sync with the declaration in the DR codebase!
 */
RCF_BEGIN(D3DarkModInterface, "DarkMod")
    RCF_METHOD_V1(void, writeToConsole, const std::string&);
	RCF_METHOD_V1(void, executeConsoleCommand, const std::string&);
	RCF_METHOD_V1(void, startConsoleBuffering, int);
	RCF_METHOD_V1(void, endConsoleBuffering, int);
RCF_END(D3DarkModInterface);

/**
 * greebo: This defines DarkRadiant's RCF Interface, for use by Doom 3
 */
RCF_BEGIN(DarkRadiantRCFService, "DarkRadiant")
	RCF_METHOD_V1(void, writeToConsole, const std::string&);
	RCF_METHOD_V0(void, signalCommandDone);
RCF_END(DarkRadiantRCFService);

#endif /* __RCF_SERVICE_DECL_H__ */
