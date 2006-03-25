/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
 * Revision 1.1  2006/03/25 08:13:46  gildoran
 * New update for declarations... Improved the documentation/etc for xdata decls, and added some basic code for tdm_matinfo decls.
 *
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

	/// The surface type of the material.
	idStr	surfaceType;
};

#endif /* __DARKMOD_DECLTDM_MATINFO_H__ */
