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

#include "ObjectiveCondition.h"

static bool init_version = FileVersionList("$Id$", init_version);

ObjectiveCondition::ObjectiveCondition() :
	_type(INVALID_TYPE),
	_value(-1),
	_srcMission(-1),
	_srcState(-1),
	_srcObj(-1),
	_targetObj(-1)
{}

ObjectiveCondition::ObjectiveCondition(const idDict& dict, int index)
{
	ParseFromSpawnargs(dict, index);
}

bool ObjectiveCondition::IsValid() const
{
	return _type != INVALID_TYPE && _value != -1 && _srcMission != -1 &&
		   _srcState != -1 && _srcObj != -1 && _targetObj;
}

bool ObjectiveCondition::Apply(CMissionData& missionData)
{
	// TODO

	return false;
}

void ObjectiveCondition::ParseFromSpawnargs(const idDict& dict, int index)
{
	/** 
		Schema:

		obj_condition_1_src_mission		<missionNumber> (0-based)
		obj_condition_1_src_obj			<objectiveNumber> (0-based)
		obj_condition_1_src_state		<objectiveState> 0|1|2
		obj_condition_1_target_obj		<objectiveNumber> (0-based)
		obj_condition_1_type			changestate|changevisibility|changemandatory
		obj_condition_1_value			changestate: 0|1|2
										changevisibility: 0|1
										changemandatory: 0|1
	*/

	idStr prefix = va("obj_condition_%d_", index);

	const char* type = dict.GetString(prefix + "type");

	if (idStr::Cmp(type, "changestate") == 0)
	{
		_type = CHANGE_STATE;
	}
	else if (idStr::Cmp(type, "changevisibility") != 0)
	{
		_type = CHANGE_VISIBILITY;
	}
	else if (idStr::Cmp(type, "changemandatory") != 0)
	{
		_type = CHANGE_MANDATORY;
	}
	else
	{
		_type = INVALID_TYPE;
	}

	// Parse the rest of the integer-based members
	_value =		dict.GetInt(prefix + "value", "-1");
	_srcMission =	dict.GetInt(prefix + "src_mission", "-1");
	_srcState =		dict.GetInt(prefix + "src_state", "-1");
	_srcObj =		dict.GetInt(prefix + "src_obj", "-1");
	_targetObj =	dict.GetInt(prefix + "target_obj", "-1");
}
