/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/

#ifndef __AI_BASICMIND_H__
#define __AI_BASICMIND_H__

namespace ai
{

class BasicMind :
	public Mind
{
private:
	// The reference to the owning entity
	idEntityPtr<idAI> _owner;

	// The current alert state
	EAlertState _alertState;

public:
	BasicMind(idAI* owner);
	virtual ~BasicMind() {}

	virtual void Think();

	// Get the current alert state 
	virtual EAlertState GetAlertState() const;

	// Set the current alert state
	virtual void SetAlertState(EAlertState newState);

	virtual void Save(idSaveGame* savefile) const;
	virtual void Restore(idRestoreGame* savefile);
};

} // namespace ai

#endif /* __AI_BASICMIND_H__ */
