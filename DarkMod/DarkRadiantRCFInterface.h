#ifndef __RCF_SERVICE_DECL_H__
#define __RCF_SERVICE_DECL_H__

#include <string>
#include <RCF/Idl.hpp>
#include <SF/vector.hpp>

/**
 * greebo: This defines the service methods for DarkRadiant.
 *         Keep this in sync with the declaration in the DR codebase!
 */
RCF_BEGIN(D3ConsoleWriter, "D3ConsoleWriter")
    RCF_METHOD_V1(void, writeToConsole, const std::string&);
RCF_END(D3ConsoleWriter);

#endif /* __RCF_SERVICE_DECL_H__ */
