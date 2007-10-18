/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/

#ifndef __AI_STATE_H__
#define __AI_STATE_H__

#include <boost/shared_ptr.hpp>

namespace ai
{

class State
{
public:
	// This is called when the state is first attached to the AI's Mind.
	virtual void Init() = 0;
};
typedef boost::shared_ptr<State> StatePtr;

} // namespace ai

#endif /* __AI_STATE_H__ */
