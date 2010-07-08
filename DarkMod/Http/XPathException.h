#ifndef XPATHEXCEPTION_H_
#define XPATHEXCEPTION_H_

#include <stdexcept>
#include <string>

namespace xml
{

// Exception to indicate failure to process an XPath lookup.

class XPathException:
    public std::runtime_error
{
public:

    // Constructor. Must initialise the parent.
    XPathException(const std::string& what):
        std::runtime_error(what) {}
        
};

} // namespace

#endif /*XPATHEXCEPTION_H_*/
