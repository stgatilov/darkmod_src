/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 866 $
 * $Date: 2007-03-23 22:25:02 +0100 (Fr, 23 Mär 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/
#ifndef SR_STIMRESPONSECOLLECTION__H
#define SR_STIMRESPONSECOLLECTION__H

#include "Stim.h"
#include "Response.h"

/******************************************************************************
* The Stim/Response system consists of a collection class, which handles the
* actuall stimulations and responses. Each entity can have exactly one such 
* collection, and all stim/responses are added to this collection. All 
* primary interactions with stims and responses are done via this collection.
******************************************************************************/

/**
 * CStimResponseCollection is the collection to handle all interactions within
 * stimulations and their responses. For each stim of a given type only one
 * stim/response may exist in a given collection.
 * Stim/responses should always be identified by their type instead of their pointer.
 * Handling for the objects is done via the collection class.
 */
class CStimResponseCollection {
public:
	CStimResponseCollection(void);
	~CStimResponseCollection(void);

	void			Save(idSaveGame *savefile) const;
	void			Restore(idRestoreGame *savefile);

	/**
	 * AddStim/Response creates a new stim of the given type and returns the pointer to
	 * the new object. If the stim already existed, it is not created again but the 
	 * pointer still is returned to the existing object.
	 * The returned pointer may be used for configuring or activating the stim, but it
	 * may NEVER be used to delete the object, and it should not be passed around
	 * extensively, because it may become invalid.
	 */
// TODO: Add additional parameters to AddStim: Magnitude, Interleave duration, duration
	CStim			*AddStim(idEntity *Owner, int Type, float Radius = 0.0f, bool Removable = true, bool Default = false);
	CResponse		*AddResponse(idEntity *Owner, int Type, bool Removable = true, bool Default = false);

	/**
	 * AddStim/Response with already configured objects. If the type already exists, the new object is not added 
	 * and the pointer to the existing object is returned, otherwise the added pointer is returned.
	 */
	CStim			*AddStim(CStim *);
	CResponse		*AddResponse(CResponse *);

	/**
	 * RemoveStim will remove the stim of the given type and the object is destroyed.
	 * Any pointer that still exists will become invalid after that.
	 * The number of remaining stims are returned.
	 */
	int				RemoveStim(int Type);
	int				RemoveResponse(int Type);
	int				RemoveStim(CStim *);
	int				RemoveResponse(CResponse *);

	/**
	* Returns true if the stim response collection has any stims or responses
	**/
	bool			HasStim( void );
	bool			HasResponse( void );

	/**
	 * AddEntityToList will add the given entity to the list exactly once. If the entity
	 * is already in the list, then nothing will happen and the entity stays in it.
	 */
	void			AddEntityToList(idList<void *> &List, void *);
 
	/**
	 * If the stim contains information for a timed event, this function parses the string
	 * and creates the appropriate timer structure.
	 *
	 * The timer is configured by several strings on the entity:
	 *
 	 * Key: sr_timer_time
	 * Value: Time in the format: HOURS:MINUTES:SECONDS:MILLISECONDS
	 *
	 * Key: sr_timer_reload
	 * Value: N
	 *
	 * N = 0-N for the number of times it should be reloaded.
	 * A value of -1 means that it is infinitely reloaded (until disabled).
	 *
	 * Key: sr_timer_type
	 * Value: { RELOAD | SINGLESHOT }
	 *
	 * Key: sr_timer_waitforstart
	 * Value: { 0 | 1 } - Set true if timer should wait for StartTimer to start
	 * Otherwise starts on spawn.
	 */
	void			CreateTimer(const idDict *args, CStim *Owner, int Counter);
	void			CreateTimer(CStim *Owner);

 	idList<CStim *>	&GetStimList(void) { return m_Stim; };
	idList<CResponse *>	&GetResponseList(void) { return m_Response; };

	CStimResponse	*GetStimResponse(int StimType, bool Stim);
	CStim			*GetStim(int StimType);
	CResponse		*GetResponse(int StimType);

	void			ParseSpawnArgsToStimResponse(const idDict *args, idEntity *Owner);
	bool			ParseSpawnArg(const idDict *args, idEntity *Owner, const char Class, int Counter);

	/*
	* This static method is used to allocate, on the heap, a stim of a given type.
	* Some stim types create descended classes with virtual overrides of some stim methods.
	* It is important to always uses this instead of allocating a CStim object yourself so
	* that the correct descended class is created.
	*
	* @param p_owner A pointer to the entity which owns this stim
	*
	* @param type The enumerated stim type value
	*/
	static			CStim *createStim(idEntity* p_Owner, StimType type);

	/*
	* This static method is used to allocate, on the heap, a response of a given type.
	* Some response types create descended classes with virtual overrides of some response methods.
	* It is important to always uses this instead of allocating a CResponse object yourself so
	* that the correct descended class is created.
	*
	* @param p_owner A pointer to the entity which owns this response
	*
	* @param type The enumerated stim type value for the response
	*/
	static			CResponse *createResponse (idEntity* p_owner, StimType type);

protected:
	idList<CStim *>		m_Stim;
	idList<CResponse *>	m_Response;
};

#endif /* SR_STIMRESPONSECOLLECTION__H */
