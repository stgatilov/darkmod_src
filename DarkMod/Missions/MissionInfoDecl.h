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

class CMissionInfoDecl : 
	public idDecl
{
public:
	~CMissionInfoDecl();

	virtual size_t			Size( void ) const;
	virtual const char *	DefaultDefinition( void ) const;
	virtual void			FreeData( void );
	virtual bool			Parse( const char *text, const int textLength );

	/// Key/value data parsed from the mission info decl.
	idDict					data;

	// Regenerates the declaration body using the given name as decl name
	void					Update(const idStr& name);

	// Returns the declaration or NULL if no declaration has been registered so far
	static CMissionInfoDecl*	Find(const idStr& name);

	// Creates a new declaration with the given name. When written, the declaration
	// will be saved to the default filename as given in the corresponding CVAR.
	static CMissionInfoDecl*	Create(const idStr& name);

	// Convenience method, combining the above two. The declaration will be created
	// using Create() if not found after calling Find().
	static CMissionInfoDecl*	FindOrCreate(const idStr& name);

	static const char* const TYPE_NAME;
};

#endif /* _MISSION_INFO_DECL_H_ */
