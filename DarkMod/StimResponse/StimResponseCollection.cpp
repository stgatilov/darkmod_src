/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 866 $
 * $Date: 2007-03-23 22:25:02 +0100 (Fr, 23 M�r 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/
#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: StimResponseCollection.cpp 870 2007-03-27 14:21:59Z greebo $", init_version);

#include "StimResponseCollection.h"
#include "../AIComm_StimResponse.h"

CStimResponseCollection::CStimResponseCollection(void)
{
}

CStimResponseCollection::~CStimResponseCollection(void)
{
}

CStim* CStimResponseCollection::createStim(idEntity* p_owner, StimType type)
{
	CStim* pRet;

	if (type == ST_COMMUNICATION)
	{
		DM_LOG(LC_STIM_RESPONSE, LT_DEBUG).LogString ("Creating CAIComm_Stim\r");
		pRet = new CAIComm_Stim (p_owner, type);
	}
	else
	{
		DM_LOG(LC_STIM_RESPONSE, LT_DEBUG).LogString ("Creating CStim\r");
		pRet = new CStim(p_owner, type);
	}

	return pRet;
}

CResponse* CStimResponseCollection::createResponse(idEntity* p_owner, StimType type)
{
	CResponse* pRet;

	if (type == ST_COMMUNICATION)
	{
		DM_LOG(LC_STIM_RESPONSE, LT_DEBUG).LogString ("Creating CAIComm_Response\r");
		pRet = new CAIComm_Response (p_owner, type);
	}
	else
	{
		DM_LOG(LC_STIM_RESPONSE, LT_DEBUG).LogString ("Creating CResponse\r");
		pRet = new CResponse(p_owner, type);
	}

	// Optimization: Set contents to include CONTENTS_RESPONSE
	p_owner->GetPhysics()->SetContents( p_owner->GetPhysics()->GetContents() | CONTENTS_RESPONSE );

	return pRet;
}

CStim *CStimResponseCollection::AddStim(idEntity *Owner, int Type, float fRadius, bool bRemovable, bool bDefault)
{
	CStim *pRet = NULL;
	int i, n;

	n = m_Stim.Num();
	for(i = 0; i < n; i++)
	{
		if(m_Stim[i]->m_StimTypeId == Type)
		{
			DM_LOG(LC_STIM_RESPONSE, LT_DEBUG).LogString ("Stim of that type is already in collection, returning it");
			pRet = m_Stim[i];
			break;
		}
	}

	if(pRet == NULL)
	{
		// Create either type specific descended class, or the default base class
		pRet = createStim(Owner, (StimType) Type);
		m_Stim.Append(pRet);
	}

	if(pRet != NULL)
	{
		pRet->m_Default = bDefault;
		pRet->m_Removable = bRemovable;
		pRet->m_Radius = fRadius;
		pRet->m_State = SS_DISABLED;

		AddEntityToList((idList<void *>	&)gameLocal.m_StimEntity, Owner); 
	}

	return pRet;
}

CResponse *CStimResponseCollection::AddResponse(idEntity *Owner, int Type, bool bRemovable, bool bDefault)
{
	CResponse *pRet = NULL;
	int i, n;

	n = m_Response.Num();
	for(i = 0; i < n; i++)
	{
		if(m_Response[i]->m_StimTypeId == Type)
		{
			DM_LOG(LC_STIM_RESPONSE, LT_DEBUG).LogString ("Response of that type is already in collection, returning it");
			pRet = m_Response[i];
			break;
		}
	}

	if(pRet == NULL)
	{
		pRet = createResponse (Owner, (StimType) Type);
		m_Response.Append(pRet);
	}

	if(pRet != NULL)
	{
		pRet->m_Default = bDefault;
		pRet->m_Removable = bRemovable;

		AddEntityToList((idList<void *>	&)gameLocal.m_RespEntity, Owner); 
	}

	// Optimization: Update clip contents to include contents_response

	return pRet;
}

CStim *CStimResponseCollection::AddStim(CStim *s)
{
	CStim *pRet = NULL;
	int i, n;

	if(s == NULL)
		goto Quit;

	n = m_Stim.Num();
	for(i = 0; i < n; i++)
	{
		if(m_Stim[i]->m_StimTypeId == s->m_StimTypeId)
		{
			pRet = m_Stim[i];
			break;
		}
	}

	if(pRet == NULL)
	{
		pRet = s;
		m_Stim.Append(pRet);

		AddEntityToList((idList<void *>	&)gameLocal.m_StimEntity, s->m_Owner); 
	}

Quit:
	return pRet;
}

CResponse *CStimResponseCollection::AddResponse(CResponse *r)
{
	CResponse *pRet = NULL;
	int i, n;

	if(r == NULL)
		goto Quit;

	n = m_Response.Num();
	for(i = 0; i < n; i++)
	{
		if(m_Response[i]->m_StimTypeId == r->m_StimTypeId)
		{
			pRet = m_Response[i];
			break;
		}
	}

	if(pRet == NULL)
	{
		pRet = r;
		m_Response.Append(pRet);

		AddEntityToList((idList<void *>	&)gameLocal.m_RespEntity, r->m_Owner);
	}

Quit:
	return pRet;
}


int CStimResponseCollection::RemoveStim(int Type)
{
	CStim *pRet = NULL;
	int i, n;

	n = m_Stim.Num();
	for(i = 0; i < n; i++)
	{
		if(m_Stim[i]->m_StimTypeId == Type)
		{
			pRet = m_Stim[i];
			if(pRet->m_Removable == true)
			{
				m_Stim.RemoveIndex(i);
				delete pRet;
			}
			break;
		}
	}

	return m_Stim.Num();
}

int CStimResponseCollection::RemoveResponse(int Type)
{
	idEntity *owner = NULL;
	CResponse *pRet = NULL;
	int i, n;

	n = m_Response.Num();
	for(i = 0; i < n; i++)
	{
		if(m_Response[i]->m_StimTypeId == Type)
		{
			pRet = m_Response[i];
			if(pRet->m_Removable == true)
			{
				owner = pRet->m_Owner;
				m_Response.RemoveIndex(i);
				delete pRet;
			}
			break;
		}
	}

	// Remove the CONTENTS_RESPONSE flag if no more responses
	if( m_Response.Num() <= 0 && owner != NULL )
		owner->GetPhysics()->SetContents( owner->GetPhysics()->GetContents() & ~CONTENTS_RESPONSE );

	return m_Response.Num();
}

int CStimResponseCollection::RemoveStim(CStim *s)
{
	CStim *pRet = NULL;
	int i, n;

	n = m_Stim.Num();
	for(i = 0; i < n; i++)
	{
		if(m_Stim[i] == s)
		{
			pRet = m_Stim[i];
			if(pRet->m_Removable == true)
			{
				m_Stim.RemoveIndex(i);
				delete pRet;
			}
			break;
		}
	}

	return m_Stim.Num();
}

int CStimResponseCollection::RemoveResponse(CResponse *r)
{
	CResponse *pRet = NULL;
	int i, n;

	n = m_Response.Num();
	for(i = 0; i < n; i++)
	{
		if(m_Response[i] == r)
		{
			pRet = m_Response[i];
			if(pRet->m_Removable == true)
			{
				m_Response.RemoveIndex(i);
				delete pRet;
			}
			break;
		}
	}

	return m_Response.Num();
}


void CStimResponseCollection::AddEntityToList(idList<void *> &oList, void *e)
{
	bool add = true;
	int i, n;

	n = oList.Num();
	for(i = 0; i < n; i++)
	{
		if(oList[i] == e)
		{
			add = false;
			break;
		}
	}

	if(add == true)
		oList.Append(e);
}

CStimResponse *CStimResponseCollection::GetStimResponse(int StimType, bool Stim)
{
	CStimResponse *rc = NULL, *sr;
	int i, n;
	idList<CStimResponse *> *oList = (Stim == true) ? (idList<CStimResponse *> *)&m_Stim : (idList<CStimResponse *> *)&m_Response;

	n = oList->Num();
	for(i = 0; i < n; i++)
	{
		sr = oList->operator[](i);
		if(sr->m_StimTypeId == StimType)
		{
			rc = sr;
			break;
		}
	}

	return rc;
}

CStim *CStimResponseCollection::GetStim(int StimType)
{
	return (CStim *)GetStimResponse(StimType, true);
}

CResponse *CStimResponseCollection::GetResponse(int StimType)
{
	return (CResponse *)GetStimResponse(StimType, false);
}

bool CStimResponseCollection::ParseSpawnArg(const idDict *args, idEntity *Owner, const char sr_class, int Counter)
{
	bool rc = false;
	idStr str;
	idStr name;
	CStim *stim = NULL;
	CResponse *resp = NULL;
	CStimResponse *sr = NULL;
	float Radius = 0.0f;
	StimState state( SS_DISABLED );
	StimType typeOfStim;
	
	// Check if the entity contains either a stim or a response.
	if(sr_class != 'S' && sr_class != 'R')
	{
		DM_LOG(LC_STIM_RESPONSE, LT_ERROR)LOGSTRING("Invalid sr_class value [%s]\r", str.c_str());
		goto Quit;
	}

	// Get the id of the stim/response type so we know what sub-class to create
	sprintf(name, "sr_type_%u", Counter);
	args->GetString(name, "-1", str);

	// This is invalid as an entity definition
	if(str == "-1")
	{
		sr = NULL;
		goto Quit;
	}

	// If the first character is alphanumeric, we check if it 
	// is a known id and convert it.
	/* StimType */ typeOfStim = ST_DEFAULT;

	if((str[0] >= 'a' && str[0] <= 'z')
		|| (str[0] >= 'A' && str[0] <= 'Z'))
	{
		// Try to recognise the string as known Stim type
		typeOfStim = CStimResponse::getStimType(str);
		
		// If the string hasn't been found, we have id == ST_DEFAULT.
		if (typeOfStim == ST_DEFAULT)
		{
			DM_LOG(LC_STIM_RESPONSE, LT_ERROR)LOGSTRING("Invalid sr_type id [%s]\r", str.c_str());
			sr = NULL;
			goto Quit;
		}
	}
	else if(str[0] >= '0' && str[0] <= '9') // Is it numeric?
	{	
		typeOfStim = (StimType) atol(str.c_str());
	}
	else		// neither a character nor a number, thus it is invalid.
	{
		DM_LOG(LC_STIM_RESPONSE, LT_ERROR)LOGSTRING("Invalid sr_type id [%s]\r", str.c_str());
		sr = NULL;
		goto Quit;
	}


	if(sr_class == 'S')
	{
		stim = createStim (Owner, typeOfStim);
		sr = stim;
	}
	else if (sr_class == 'R')
	{
		resp = createResponse (Owner, typeOfStim);
		sr = resp;
	}

	// Set stim response type
	sr->m_StimTypeId = typeOfStim;

	// Set stim response name string
	sr->m_StimTypeName = str;

	// Read stim response state from the def file
	sprintf(name, "sr_state_%u", Counter);
	args->GetInt(name, "1", (int &)state);
	
	if( args->GetBool(name, "1") )
		sr->EnableSR(true);
	else
		sr->EnableSR(false);

	sprintf(name, "sr_chance_%u", Counter);
	sr->m_Chance = args->GetFloat(name, "1.0");

	// A stim also may have a radius
	if(sr_class == 'S')
	{
		sprintf(name, "sr_radius_%u", Counter);
		args->GetFloat(name, "0.0", Radius);
		stim->m_Radius = Radius;

		sprintf(name, "sr_falloffexponent_%u", Counter);
		stim->m_FallOffExponent = args->GetFloat(name, "0");

		sprintf(name, "sr_use_bounds_%u", Counter);
		stim->m_bUseEntBounds = args->GetBool(name, "0");

		sprintf(name, "sr_collision_%u", Counter);
		stim->m_bCollisionBased = args->GetBool(name, "0");

		sprintf(name, "sr_velocity_%u", Counter);
		stim->m_Velocity = args->GetVector(name, "0 0 0");

		sprintf(name, "sr_bounds_mins_%u", Counter);
		stim->m_Bounds[0] = args->GetVector(name, "0 0 0");

		sprintf(name, "sr_bounds_maxs_%u", Counter);
		stim->m_Bounds[1] = args->GetVector(name, "0 0 0");

		// set up time interleaving so the stim isn't fired every frame
		sprintf(name, "sr_time_interval_%u", Counter);
		stim->m_TimeInterleave = args->GetInt(name, "0");

		// userfriendly stim duration time
		sprintf(name, "sr_duration_%u", Counter);
		stim->m_Duration = args->GetInt(name, "0");

		sprintf(name, "sr_magnitude_%u", Counter);
		stim->m_Magnitude = args->GetFloat(name, "1.0");

		sprintf(name, "sr_max_fire_count_%u", Counter);
		stim->m_MaxFireCount = args->GetFloat(name, "-1");

		// Check if we have a timer on this stim.
		CreateTimer(args, stim, Counter);
	}
	else	// this is only for responses
	{
		sprintf(name, "sr_chance_timeout_%u", Counter);
		sr->m_ChanceTimer = args->GetFloat(name, "-1");

		sprintf(name, "sr_random_effects_%u", Counter);
		resp->m_NumRandomEffects = args->GetFloat(name, "0");

		// Get the name of the script function for processing the response
		name = "sr_script_" + str;
		args->GetString(name, "", str);
		resp->m_ScriptFunction = str;

		// Try to identify the ResponseEffect spawnargs
		int effectIdx = 1;
		while (effectIdx > 0) {
			// Try to find a string like "sr_effect_2_1"
			sprintf(name, "sr_effect_%u_%u", Counter, effectIdx);
			args->GetString(name, "", str);

			if (str == "")
			{
				// Set the index to negative values to end the loop
				effectIdx = -1;
			}
			else {
				// Assemble the postfix of this effect for later key/value lookup
				// This is passed to the effect script eventually
				idStr effectPostfix;
				sprintf(effectPostfix, "%u_%u", Counter, effectIdx);

				DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Adding response effect\r");
				resp->addResponseEffect(str, effectPostfix, args);
				effectIdx++;
			}
		}
	}

	rc = true;

Quit:
	if(sr != NULL)
	{
		if(stim != NULL)
		{
			DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Stim %08lX added to collection for %08lX\r", stim, Owner);
			AddStim(stim);
			stim->m_State = state;
		}

		if(resp != NULL)
		{
			DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Response %08lX added to collection for %08lX\r", resp, Owner);
			AddResponse(resp);
		}
	}

	return rc;
}

void CStimResponseCollection::ParseSpawnArgsToStimResponse(const idDict *args, idEntity *Owner)
{
	idStr str;
	idStr name;
	int i;
	char sr_class;

	if(Owner == NULL)
	{
		DM_LOG(LC_STIM_RESPONSE, LT_ERROR)LOGSTRING("Owner set to NULL is not allowed!\r");
		goto Quit;
	}

	i = 1;
	while(i != 0)
	{
		sprintf(name, "sr_class_%u", i);
		DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Looking for %s\r", name.c_str());
		if(!args->GetString(name, "X", str))
			goto Quit;

		sr_class = str[0];
		if(ParseSpawnArg(args, Owner, sr_class, i) == false)
			goto Quit;

		i++;
	}

Quit:
	return;
}


void CStimResponseCollection::CreateTimer(const idDict *args, CStim *stim, int Counter)
{
	idStr str;
	int n;
	
	CStimResponseTimer* timer = stim->GetTimer();

	args->GetInt( va("sr_timer_reload_%u",Counter) , "-1", n);
	timer->m_Reload = n;

	args->GetString( va("sr_timer_type_%u",Counter), "", str);
	if(str.Cmp("RELOAD") == 0) {
		timer->m_Type = CStimResponseTimer::SRTT_RELOAD;
	}
	else {
		timer->m_Type = CStimResponseTimer::SRTT_SINGLESHOT;
	}

	args->GetString( va("sr_timer_time_%u",Counter), "0:0:0:0", str );
    TimerValue val;
	val = CStimResponseTimer::ParseTimeString( str );
	
	// if timer is actually set
	if( val.Time.Hour || val.Time.Minute || val.Time.Second || val.Time.Millisecond )
	{
		// TODO: Return a bool here so that the outer function knows not to add this to m_Stim in the collection?

		stim->AddTimerToGame();
		timer->SetTimer(val.Time.Hour, val.Time.Minute, val.Time.Second, val.Time.Millisecond);
		
		// timer starts on map startup by default, otherwise wait for start
		if( !(args->GetBool( va("sr_timer_waitforstart_%u",Counter), "0" )) ) {
			timer->Start(static_cast<unsigned long>(sys->GetClockTicks()));
		}
	}
}

bool CStimResponseCollection::HasStim( void )
{
	return (m_Stim.Num() > 0);
}

bool CStimResponseCollection::HasResponse( void )
{
	return (m_Response.Num() > 0);
}
