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

	// Construct a xml::Document from the given filename (must be the full path).
	// Use the isValid() method to check if the load was successful.
	Document(const std::string& filename);
	
	// Copy constructor
	Document(const Document& other);

	// Destructor, frees the xmlDocPtr
	~Document();

	// Creates a new xml::Document object (allocates a new xmlDoc)
	static Document create();

	// Add a new toplevel node with the given name to this Document
	void addTopLevelNode(const std::string& name);

	// Returns the top level node (or an empty Node object if none exists)
	Node getTopLevelNode() const;

	// Merges the (top-level) nodes of the <other> document into this one.
	// The insertion point in this Document is specified by <importNode>.
	void importDocument(Document& other, Node& importNode);

	// Copies the given Nodes into this document (a top level node
	// must be created beforehand)
	void copyNodes(const NodeList& nodeList);
    
	// Returns TRUE if the document is ok and can be queried.
	bool isValid() const;

    // Evaluate the given XPath expression and return a NodeList of matching
    // nodes.
    NodeList findXPath(const std::string& path) const;
    
    // Saves the file to the disk via xmlSaveFormatFile
    void saveToFile(const std::string& filename) const;
};

} // namespace

#endif /* _XML_DOCUMENT_H_ */
