/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef _MISSION_INFO_DECL_H_
#define _MISSION_INFO_DECL_H_

#include "../../idlib/precompiled.h"
#include <boost/shared_ptr.hpp>

class CMissionInfoDecl
{
private:
	// The body text used for saving
	idStr _bodyText;

public:
	// Construct this declaration from the given token stream
	bool Parse(idLexer& src);

	/// Key/value data parsed from the mission info decl.
	idDict data;

	// Regenerates the declaration body using the given name as decl name
	void Update(const idStr& name);

	// Append the data to the given file
	void SaveToFile(idFile* file);

	static const char* const TYPE_NAME;
};
typedef boost::shared_ptr<CMissionInfoDecl> CMissionInfoDeclPtr;

#endif /* _MISSION_INFO_DECL_H_ */
