
#include "../idlib/precompiled.h"
#pragma hdrstop
#include "Game_local.h"

#ifdef MOD_WATERPHYSICS

// We do these splashes if the mass of the colliding object is less than these values.
// Anything large than MEDIUM_SPLASH does a large splash. (get it?)
const int SMALL_SPLASH		= 25;
const int MEDIUM_SPLASH		= 1000;

/*
===============================================================================

	idLiquid

===============================================================================
*/

CLASS_DECLARATION( idEntity, idLiquid )
	EVENT( EV_Touch,			idLiquid::Event_Touch )
END_CLASS

idLiquid::idLiquid( void ) {
}

/*
================
idLiquid::Save
================
*/
void idLiquid::Save( idSaveGame *savefile ) const {

	int i;

   	savefile->WriteStaticObject( this->physicsObj );
	
	savefile->WriteString(smokeName.c_str());
	savefile->WriteString(soundName.c_str());

	for( i = 0; i < 3; i++ )
		savefile->WriteParticle(this->splash[i]);
	savefile->WriteParticle(this->waves);
}

/*
================
idLiquid::Restore
================
*/
void idLiquid::Restore( idRestoreGame *savefile ) {
	
	int i;

	savefile->ReadStaticObject( this->physicsObj );
	RestorePhysics( &this->physicsObj );

	savefile->ReadString(this->smokeName);
	savefile->ReadString(this->soundName);

	for( i = 0; i < 3; i++ )
		savefile->ReadParticle(this->splash[i]);
	savefile->ReadParticle(this->waves);
}

/*
================
idLiquid::Spawn
================
*/
void idLiquid::Spawn() {
/*
	model = dynamic_cast<idRenderModelLiquid *>( renderEntity.hModel );
	if ( !model ) {
		gameLocal.Error( "Entity '%s' must have liquid model", name.c_str() );
	}
	model->Reset();
*/
	float liquidDensity;
	float liquidViscosity;
	float liquidFriction;
	idVec3 minSplash;
	idVec3 minWave;
	idStr temp;
	const char *splashName;

	common->Printf("idLiquid:%s) Spawned\n",this->GetName() );

	// getters
	spawnArgs.GetFloat("density","0.01043f",liquidDensity);
	spawnArgs.GetFloat("viscosity","3.0f",liquidViscosity);
	spawnArgs.GetFloat("friction","3.0f",liquidFriction);
	spawnArgs.GetString("liquid_name","water",temp);
	spawnArgs.GetVector("minSplashVelocity","100 100 100",minSplash);
	spawnArgs.GetVector("minWaveVelocity","60 60 60",minWave);

	// setters
	this->smokeName = "smoke_";
	this->smokeName.Append(temp);

	this->soundName = "snd_";
	this->soundName.Append(temp);

	splashName = spawnArgs.GetString("smoke_small","water_splash_tiny");
	this->splash[0] = static_cast<const idDeclParticle *>(declManager->FindType(DECL_PARTICLE,splashName));

	splashName = spawnArgs.GetString("smoke_medium","water_splash");
	this->splash[1] = static_cast<const idDeclParticle *>(declManager->FindType(DECL_PARTICLE,splashName));

	splashName = spawnArgs.GetString("smoke_large","water_splash_large");
	this->splash[2] = static_cast<const idDeclParticle *>(declManager->FindType(DECL_PARTICLE,splashName));

	splashName = spawnArgs.GetString("smoke_waves","water_waves");
	this->waves = static_cast<const idDeclParticle *>(declManager->FindType(DECL_PARTICLE,splashName));

	// setup physics
	this->physicsObj.SetSelf(this);
	this->physicsObj.SetClipModel( new idClipModel(this->GetPhysics()->GetClipModel()), liquidDensity );
	this->physicsObj.SetOrigin(this->GetPhysics()->GetOrigin());
	this->physicsObj.SetAxis(this->GetPhysics()->GetAxis());	
	this->physicsObj.SetGravity( gameLocal.GetGravity() );
	this->physicsObj.SetContents( CONTENTS_WATER | CONTENTS_TRIGGER );

	this->physicsObj.SetDensity(liquidDensity);
	this->physicsObj.SetViscosity(liquidViscosity);
	this->physicsObj.SetMinSplashVelocity(minSplash);
	this->physicsObj.SetMinWaveVelocity(minWave);

	this->SetPhysics( &this->physicsObj );

	BecomeActive( TH_THINK );
}

/*
================
idLiquid::Event_Touch

	This is mainly used for actors who touch the liquid, it spawns a splash
	near they're feet if they're moving fast enough.
================
*/
void idLiquid::Event_Touch( idEntity *other, trace_t *trace ) {
// FIXME: for QuakeCon
/*
	idVec3 pos;

	pos = other->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin();
	model->IntersectBounds( other->GetPhysics()->GetBounds().Translate( pos ), -10.0f );
*/

	idPhysics_Liquid *liquid;
	idPhysics_Actor *phys;

	if( !other->GetPhysics()->IsType(idPhysics_Actor::Type) )
		return;

	phys = static_cast<idPhysics_Actor *>(other->GetPhysics());
	if( phys->GetWaterLevel() != WATERLEVEL_FEET )
		return;

	impactInfo_t info;
	other->GetImpactInfo(this,trace->c.id,trace->c.point,&info);
	liquid = &this->physicsObj;

	trace->c.point = info.position + other->GetPhysics()->GetOrigin();
	trace->c.entityNum = other->entityNumber;

	// stop actors from constantly splashing when they're in the water
	// (this is such a bad thing to do!!!)
	// TODO: Fixme...
	//		1) Probably the best way to fix this is put a wait timer inside the actor and have this
	//		function set/reset that timer for when the actor should spawn particles at it's feet.
	//		2) Actors don't spawn particles at their feet, it's usually at the origin, for some
	//		reason info.position is (null), needs a fix so that splash position is correct
	if(	gameLocal.random.RandomFloat() > 0.5f )
		return;

	this->Collide(*trace,info.velocity);
}

/*
================
idLiquid::Collide
	Spawns a splash particle and attaches a sound to the colliding entity.
================
*/
bool idLiquid::Collide( const trace_t &collision, const idVec3 &velocity ) {
	idEntity *e = gameLocal.entities[collision.c.entityNum];
	idPhysics_Liquid *phys = static_cast<idPhysics_Liquid *>( this->GetPhysics() );
	const idDeclParticle *splash;
	const char *sName;
	float eMass;
	idVec3 splashSpot;
	float velSquare = velocity.LengthSqr();

	eMass = e->GetPhysics()->GetMass();
	splashSpot = collision.c.point;
		
	if( velSquare > phys->GetMinSplashVelocity().LengthSqr() ) {
		// pick which splash particle to spawn
		// first we check the entity, if it's not defined we use
		// one defined for this liquid.
		sName = e->spawnArgs.GetString(this->smokeName.c_str());
		if( *sName != '\0' ) {
			// load entity particle
			splash = static_cast<const idDeclParticle *>(declManager->FindType(DECL_PARTICLE,sName));
		}
		else {
			// load a liquid particle based on the mass of the splashing entity
			if( eMass < SMALL_SPLASH )
				splash = this->splash[0];
			else if( eMass < MEDIUM_SPLASH )
				splash = this->splash[1];
			else
				splash = this->splash[2];
		}
	
		// only play the sound for a splash
		e->StartSound( this->soundName.c_str(), SND_CHANNEL_ANY, 0, false, NULL);
	}
	else if( velSquare > phys->GetMinWaveVelocity().LengthSqr() ) {
		splash = this->waves;
	}
	else {
		// the object is moving to slow so we abort
		return true;
	}

	// spawn the particle
	gameLocal.smokeParticles->EmitSmoke( splash, gameLocal.time, gameLocal.random.RandomFloat(), splashSpot, collision.endAxis );
	return true;
}

#endif