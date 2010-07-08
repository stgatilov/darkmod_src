/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);

#include "XmlDocument.h"
#include "XPathException.h"

#include <libxml/parser.h>
#include <libxml/xpath.h>

#include <iostream>

namespace xml
{

// Construct a wrapper around the provided xmlDocPtr.
Document::Document(xmlDocPtr doc):
    _xmlDoc(doc)
{}

Document Document::CreateFromFile(const std::string& filename)
{
	return xml::Document(xmlParseFile(filename.c_str()));
}

Document Document::CreateFromString(const std::string& str)
{
	return xml::Document(xmlParseMemory(str.c_str(), str.length()));
}

Document::~Document()
{
	if (_xmlDoc != NULL)
	{
		// Free the xml document memory
		xmlFreeDoc(_xmlDoc);
	}
}

Document Document::Create()
{
	xmlChar* versionStr = xmlCharStrdup("1.0");

	// Create a new xmlDocPtr and return the object
	xmlDocPtr doc = xmlNewDoc(versionStr);

	xmlFree(versionStr);

	return Document(doc);
}

void Document::AddTopLevelNode(const std::string& name) {
	if (!IsValid()) {
		return; // is not Valid, place an assertion here?
	}

	xmlChar* nameStr = xmlCharStrdup(name.c_str());
	xmlChar* emptyStr = xmlCharStrdup("");

	xmlNodePtr root = xmlNewDocNode(_xmlDoc, NULL, nameStr, emptyStr);
	xmlNodePtr oldRoot = xmlDocSetRootElement(_xmlDoc, root);

	if (oldRoot != NULL) 
    {
		// Old root element, remove it
		xmlUnlinkNode(oldRoot);
		xmlFreeNode(oldRoot);
	}
	
	xmlFree(nameStr);
	xmlFree(emptyStr);
}

Node Document::GetTopLevelNode() const {
	if (!IsValid()) {
		// Invalid Document, return a NULL node
		return Node(NULL);
	}

	return Node(_xmlDoc->children);
}

void Document::ImportDocument(Document& other, Node& importNode) {
	// Locate the top-level node(s) of the other document
	xml::NodeList topLevelNodes = other.FindXPath("/*");
	
	xmlNodePtr targetNode = importNode.GetNodePtr();

	if (targetNode->children == NULL || targetNode->name == NULL) {
		// invalid importnode
		return;
	}

	// Add each of the imported nodes to the target importNode
	for (std::size_t i = 0; i < topLevelNodes.size(); i++) {
		xmlAddPrevSibling(targetNode->children, 
						  topLevelNodes[i].GetNodePtr());
	}
}

void Document::CopyNodes(const NodeList& nodeList) {
	if (!IsValid() || _xmlDoc->children == NULL) {
		return; // is not Valid, place an assertion here?
	}

	// Copy the child nodes one by one
	for (std::size_t i = 0; i < nodeList.size(); i++) {
		// Copy the node
		xmlNodePtr node = xmlCopyNode(nodeList[i].GetNodePtr(), 1);
		// Add this node to the top level node of this document
		xmlAddChild(xmlDocGetRootElement(_xmlDoc), node);
	}
}

bool Document::IsValid() const {
	return _xmlDoc != NULL;
}

// Evaluate an XPath expression and return matching Nodes.
NodeList Document::FindXPath(const std::string& path) const
{
    // Set up the XPath context
    xmlXPathContextPtr context = xmlXPathNewContext(_xmlDoc);

    if (context == NULL) {
        std::cerr << "ERROR: xml::findPath() failed to create XPath context "
                  << "when searching for " << path << std::endl;
        throw XPathException("Failed to create XPath context");
    }
    
    // Evaluate the expression  
    const xmlChar* xpath = reinterpret_cast<const xmlChar*>(path.c_str());
    xmlXPathObjectPtr result = xmlXPathEvalExpression(xpath, context);
    xmlXPathFreeContext(context);

    if (result == NULL) {
        std::cerr << "ERROR: xml::findPath() failed to evaluate expression "
                  << path << std::endl;
        throw XPathException("Failed to evaluate XPath expression");
    }
    
    // Construct the return vector. This may be empty if the provided XPath
    // expression does not identify any nodes.
    NodeList retval;
    xmlNodeSetPtr nodeset = result->nodesetval;
	if (nodeset != NULL) {
	    for (int i = 0; i < nodeset->nodeNr; i++) {
	        retval.push_back(Node(nodeset->nodeTab[i]));
	    }
	}

    xmlXPathFreeObject(result);
    return retval;
}

// Saves the file to the disk via xmlSaveFormatFile
void Document::SaveToFile(const std::string& filename) const {
	xmlSaveFormatFile(filename.c_str(), _xmlDoc, 1);
}

}
