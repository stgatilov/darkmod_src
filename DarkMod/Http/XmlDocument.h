#ifndef _XML_DOCUMENT_H_
#define _XML_DOCUMENT_H_

#include "XmlNode.h"

typedef struct _xmlDoc xmlDoc;
typedef xmlDoc *xmlDocPtr;

#include <string>

namespace xml
{
    
/* Document
 * 
 * This is a wrapper class for an xmlDocPtr. It provides a function to
 * evaluate an XPath expression on the document and return the set of 
 * matching Nodes.
 *
 * The contained xmlDocPtr is automatically released on destruction 
 * of this object.
 */
class Document
{
private:

    // Contained xmlDocPtr.
    xmlDocPtr _xmlDoc;
    
public:
    // Construct a Document using the provided xmlDocPtr.
	Document(xmlDocPtr doc);

	// Copy constructor
	Document(const Document& other);

	// Construct a xml::Document from the given filename (must be the full path).
	// Use the isValid() method to check if the load was successful.
	static Document CreateFromFile(const std::string& filename);

	// Named constructor. Construct a xml::Document from the given string in memory.
	// Use the isValid() method to check if the load was successful.
	static Document CreateFromString(const std::string& str);

	// Creates a new xml::Document object (allocates a new xmlDoc)
	static Document Create();

	// Destructor, frees the xmlDocPtr
	~Document();

	// Add a new toplevel node with the given name to this Document
	void AddTopLevelNode(const std::string& name);

	// Returns the top level node (or an empty Node object if none exists)
	Node GetTopLevelNode() const;

	// Merges the (top-level) nodes of the <other> document into this one.
	// The insertion point in this Document is specified by <importNode>.
	void ImportDocument(Document& other, Node& importNode);

	// Copies the given Nodes into this document (a top level node
	// must be created beforehand)
	void CopyNodes(const NodeList& nodeList);
    
	// Returns TRUE if the document is ok and can be queried.
	bool IsValid() const;

    // Evaluate the given XPath expression and return a NodeList of matching
    // nodes.
    NodeList FindXPath(const std::string& path) const;
    
    // Saves the file to the disk via xmlSaveFormatFile
    void SaveToFile(const std::string& filename) const;
};

} // namespace

#endif /* _XML_DOCUMENT_H_ */
