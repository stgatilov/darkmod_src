/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
 * Revision 1.3  2006/12/13 18:54:05  gildoran
 * Added SDK function names to error messages to help with debugging.
 * Fixed a bug that allowed creation of duplicate handles.
 * Added OVERLAYS_INVALID_HANDLE to be used instead of OVERLAYS_MIN_HANDLE - 1
 *
 * Revision 1.2  2006/09/18 18:56:50  gildoran
 * Added getNextOverlay, and code to automatically set an overlay as interactive if the GUI is.
 *
 * Revision 1.1  2006/09/18 13:38:13  gildoran
 * Added the first version of a unified interface for GUIs.
 *
 *
 ***************************************************************************/

#ifndef __DARKMOD_OVERLAYSYS_H__
#define __DARKMOD_OVERLAYSYS_H__

const int OVERLAYS_MIN_HANDLE = 1;
const int OVERLAYS_INVALID_HANDLE = 0;

struct SOverlay;

/// Container class to keep track of multiple GUIs.
/**	An overlay system is used to keep track of an arbitrary number of GUIs.
 *	The overlay system consists of zero or more 'overlays'. An overlay
 *	contains a GUI and some bookkeeping information. Each overlay has a
 *	'layer' which is used to sort the order the overlays are drawn in.
 *	Overlays in higher layers are drawn on top of overlays in lower layers.
 *	Multiple overlays may exist in the same layer, but their drawing order
 *	with respect to eachother is undefined.
 *	
 *	An overlay may be opaque. If/when the overlay system is rendered directly
 *	to screen, this overlay is assumed to have no transparent sections, and
 *	nothing underneath it will be rendered. This has no effect on GUIs that
 *	are in the game-world.
 *	
 *	An overlay may be interactive. If the overlay system is asked which GUI
 *	interactivity should be routed to, it will return the GUI in the highest
 *	interactive overlay. This has no effect on GUIs that are in the
 *	game-world.
 *	
 *	An overlay may be external, meaning that its GUI points to a GUI defined
 *	externally. Note that although external overlays are saved and restored,
 *	their GUI pointer isn't, and needs to be set again.
 *	
 *	I'm assuming the overlay system will only be used to contain a few
 *	overlays at a time, so I've opted for simpler data structures and easier
 *	code, rather than trying to make the most efficient system possible.
 *	However, certain easy optimizatons have been made; although accessing a
 *	different overlay from last time is O(N) with respect to the number of
 *	overlays contained, repeatedly accessing the same overlay is very fast.
 *	Similarly, results relating to which layers are opaque/interactive are
 *	cached.
 */
class COverlaySys
{
  public:

	COverlaySys();
	~COverlaySys();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	/// Draws the contained GUIs to the screen, in order.
	void					drawOverlays();
	/// Returns true if any of the GUIs are opaque.
	bool					isOpaque();
	/// Returns the interactive GUI.
	idUserInterface*		findInteractive();
	/// Used for iterating through the overlays in drawing-order.
	/**	Very efficient if used properly:
	 *	    int h = OVERLAYS_INVALID_HANDLE;
	 *	    while ( (h = o.getNextOverlay(h) ) != OVERLAYS_INVALID_HANDLE )
	 *	        o.doSomethingWith( h );
	 *	It loses efficiency if a handle is accessed other than the one returned.
	 */
	int						getNextOverlay( int handle );

	/// Create a new overlay, returning a handle for that overlay.
	int						createOverlay( int layer, int handle = OVERLAYS_INVALID_HANDLE );
	/// Destroy an overlay.
	void					destroyOverlay( int handle );
	/// Returns whether or not an overlay exists.
	bool					exists( int handle );
	/// Sets the overlay's GUI to an external GUI.
	void					setGui( int handle, idUserInterface* gui );
	/// Sets the overlay's GUI to an internal, unique one.
	bool					setGui( int handle, const char* file );
	/// Return an overlay's GUI.
    idUserInterface*		getGui( int handle );
	/// Change an overlay's layer.
	void					setLayer( int handle, int layer );
	/// Return an overlay's layer.
	int						getLayer( int handle );
	/// Return whether or not an overlay is external.
	bool					isExternal( int handle );
	/// Change whether or not an overlay is considered opaque.
	void					setOpaque( int handle, bool isOpaque );
	/// Return whether or not an overlay is considered opaque.
	bool					isOpaque( int handle );
	/// Change whether or not an overlay is considered interactive.
	void					setInteractive( int handle, bool isInteractive );
	/// Return whether or not an overlay is considered interactive.
	bool					isInteractive( int handle );

  private:

	/// Returns the overlay associated with a handle.
	SOverlay*				findOverlay( int handle, bool updateCache = true );
	/// Returns the highest opaque overlay.
	idLinkList<SOverlay>*	findOpaque();

	/// The list of overlays.
	idLinkList<SOverlay>	m_overlays;

	/// The last handle accessed.
	int						m_lastUsedHandle;
	/// The overlay of the last handle accessed.
	SOverlay*				m_lastUsedOverlay;

	/// Whether or not the highest opaque overlay needs to be recalculated.
	bool					m_updateOpaque;
	/// The cached value of the highest opaque overlay.
	idLinkList<SOverlay>*	m_highestOpaque;

	/// Whether or not the interactive overlay needs to be recalculated.
	bool					m_updateInteractive;
	/// The cached value of the interactive overlay.
	idUserInterface*		m_highestInteractive;

	/// This is the next handle to try out when creating a new overlay.
	int						m_nextHandle;
};

struct SOverlay
{
	idLinkList<SOverlay>	m_node;
	idUserInterface*		m_gui;
	int						m_handle;
	int						m_layer;
	bool					m_external;
	bool					m_opaque;
	bool					m_interactive;
};

#endif /* __DARKMOD_OVERLAYSYS_H__ */
