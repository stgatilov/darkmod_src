/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef _MOD_INFO_DECL_H_
#define _MOD_INFO_DECL_H_

#include "../../idlib/precompiled.h"
#include <boost/shared_ptr.hpp>

class CModInfoDecl
{
private:
	// The body text used for saving
	idStr _bodyText;

public:
	// Construct this declaration from the given token stream
	bool Parse(idLexer& src);

	/// Key/value data parsed from the mod info decl.
	idDict data;

	// Regenerates the declaration body using the given name as decl name
	void Update(const idStr& name);

	// Append the data to the given file
	void SaveToFile(idFile* file);

	static const char* const TYPE_NAME;
};
typedef boost::shared_ptr<CModInfoDecl> CModInfoDeclPtr;

#endif /* _MOD_INFO_DECL_H_ */
