/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef __DARKMOD_DECLTDM_MATINFO_H__
#define __DARKMOD_DECLTDM_MATINFO_H__

/// A TDM-specific material information declaration class.
/** Currently, the only info it contains is surfacetype.
 */
class tdmDeclTDM_MatInfo : public idDecl
{
public:
	tdmDeclTDM_MatInfo();
	~tdmDeclTDM_MatInfo();

	virtual size_t			Size( void ) const;
	virtual const char *	DefaultDefinition( void ) const;
	virtual void			FreeData( void );
	virtual bool			Parse( const char *text, const int textLength );

	/// Used to cache the TDM_MatInfos for all the materials applied to surfaces of a map.
	static void precacheMap( idMapFile *map );
	static void precacheModel( idRenderModel *model );

	/// The surface type of the material.
	idStr	surfaceType;
};

#endif /* __DARKMOD_DECLTDM_MATINFO_H__ */
