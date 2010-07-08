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

#include "XmlNode.h"

#include <libxml/parser.h>

namespace xml
{

// Construct a Node from the given xmlNodePtr.

Node::Node(xmlNodePtr node):
    _xmlNode(node)
{}

// Return the actual node ptr
xmlNodePtr Node::GetNodePtr() const
{
	return _xmlNode;
}

// Return the name of a node 
const std::string Node::GetName() const
{
	if (_xmlNode) {
		return std::string( reinterpret_cast<const char*>(_xmlNode->name) );
	}
	else {
		return "";
	}
}

// Return a NodeList of all children of this node
NodeList Node::GetChildren() const
{
    NodeList retval;
    
    // Iterate throught the list of children, adding each child node
    // to the return list if it matches the requested name
    for (xmlNodePtr child = _xmlNode->children; child != NULL; child = child->next) {
        retval.push_back(child);
    }
    
    return retval;
}

// Creates a new child with no content (i.e. <name />)
Node Node::CreateChild(const std::string& name)
{
	xmlChar* nodeName = xmlCharStrdup(name.c_str());

	// Create a new child under the contained node
	xmlNodePtr newChild = xmlNewChild(_xmlNode,	NULL, nodeName, NULL);

	xmlFree(nodeName);
	
	// Create a new xml::Node out of this pointer and return it
	return Node(newChild);
}

// Return a NodeList of named children of this node

NodeList Node::GetNamedChildren(const std::string& name) const
{
    NodeList retval;
    
    // Iterate throught the list of children, adding each child node
    // to the return list if it matches the requested name
    for (xmlNodePtr child = _xmlNode->children; child != NULL; child = child->next) {
        if (xmlStrcmp(child->name, reinterpret_cast<const xmlChar*>(name.c_str())) == 0) {
            retval.push_back(child);
        }
    }
    
    return retval;
}

// Set the value of the given attribute
void Node::SetAttributeValue(const std::string& key, const std::string& value)
{
	xmlChar* k = xmlCharStrdup(key.c_str());
	xmlChar* v = xmlCharStrdup(value.c_str());

	xmlSetProp(_xmlNode, k, v);

	xmlFree(k);
	xmlFree(v);
}

// Return the value of a given attribute, or throw AttributeNotFoundException
// if the attribute does not exist.

std::string Node::GetAttributeValue(const std::string& key) const
{
    // Iterate through the chain of attributes to find the requested one.
    for (xmlAttrPtr attr = _xmlNode->properties; attr != NULL; attr = attr->next) {
        if (xmlStrcmp(attr->name, reinterpret_cast<const xmlChar*>(key.c_str())) == 0) {
            return reinterpret_cast<const char*>(attr->children->content);   
        }
    }

    // Not found, return an empty string
    return "";
}

// Return the textual content of a given node. This may be an empty string if there is no
// content available.

std::string Node::GetContent() const
{
	if (_xmlNode->children && _xmlNode->children->content) {
		return std::string(reinterpret_cast<const char*>(_xmlNode->children->content));
	}
	else {
		return "";
	}
}

void Node::AddText(const std::string& text)
{
	// Allocate a new text node
	xmlNodePtr whitespace = xmlNewText(
		reinterpret_cast<const xmlChar*>(text.c_str())
	);
	
	// Add the newly allocated text as sibling of this node
	xmlAddSibling(_xmlNode, whitespace);
}

void Node::Erase()
{
	// unlink the node from the list first, otherwise: crashes ahead!
	xmlUnlinkNode(_xmlNode);
	
	// All child nodes are freed recursively
	xmlFreeNode(_xmlNode);
}

} // namespace xml
