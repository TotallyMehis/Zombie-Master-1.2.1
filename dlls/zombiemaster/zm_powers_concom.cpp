//=============================================================================
// Copyright (c) Zombie Master Development Team. All rights reserved.
// The use and distribution terms for this software are covered by the MIT
// License (http://opensource.org/licenses/mit-license.php) which
// can be found in the file LICENSE.TXT at the root of this distribution. By
// using this software in any fashion, you are agreeing to be bound by the
// terms of this license. You must not remove this notice, or any other, from
// this software.
//
// Note that due to the number of files included in the SDK, it is not feasible
// to include this notice in all of them. All original files or files 
// containing large modifications should contain this notice. If in doubt,
// assume the above notice applies, and refer to the included LICENSE.TXT text.
//=============================================================================
//
// Purpose: Console commands for the Zombie Master's strange powers
//
//=============================================================================//

#include "cbase.h"
#include "player.h"
#include "Sprite.h"

//for physexplode power
#include "physobj.h"
#include "IEffects.h"

//For SpotCreate
#include "ai_basenpc.h"
#include "game.h"
#include "zombiemaster_specific.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#define ZM_PHYSEXP_DAMAGE	"17500"		//needs to be pretty high or heavier objects won't budge
#define ZM_PHYSEXP_RADIUS	"222"	

//#define ZM_PHYSEXP_COST		400		//pretty expensive, but for something that can blow a barricade apart...
//#define ZM_SPOTCREATE_COST	100

//TGB: these get replicated to the client
ConVar zm_physexp_cost( "zm_physexp_cost", "400", FCVAR_REPLICATED, "Explosion cost" );
ConVar zm_spotcreate_cost( "zm_spotcreate_cost", "100", FCVAR_REPLICATED, "Spotcreate cost" );

ConVar zm_physexp_forcedrop_radius( "zm_physexp_forcedrop_radius", "128", FCVAR_NOTIFY, "Radius in which players are forced to drop what they carry so that the physexp can affect the objects." );

#define ZM_PHYSEXP_DELAY	7.4f

//TGB: COMMENTED, going to do it all differently


//#define	ZM_PHYSEXP_SPRITE "effects/zm_refract.vmt"
//#define	ZM_PHYSEXP_SPRITE "effects/zm_ring.vmt"
/*
//gah, just make a seperate model ent class
class CDelayedPhysExp_Effects : public CBaseAnimating
{
	DECLARE_CLASS( CDelayedPhysExp_Effects, CBaseAnimating );
	DECLARE_SERVERCLASS();

	void	Spawn () { 
			Precache();
			SetSolid( SOLID_NONE );
			SetModel( "models/manipulatable.mdl" );
			UTIL_SetSize( this, -Vector(2,2,2), Vector(2,2,2) );
		}

	void	Precache()	{
			DevMsg("*** PHYSEXP_EFFECTS PRECACHING\n");
			PrecacheMaterial( ZM_PHYSEXP_SPRITE );
			PrecacheMaterial("models/red2"); //test
			PrecacheModel( "models/manipulatable.mdl" );
			BaseClass::Precache();
		}
};

IMPLEMENT_SERVERCLASS_ST(CDelayedPhysExp_Effects, DT_DelayedPhysExp_Effects)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( env_physexp_effects, CDelayedPhysExp_Effects );
*/
//------------------------------------------------------------------------------
// Purpose: TGB: A CPhysExplosion that explodes after a set delay
//				Move to physobj.h/.cpp if we're going to do this in more places
//------------------------------------------------------------------------------
class CDelayedPhysExplosion : public CPhysExplosion
{
public:
	//DECLARE_CLASS( CDelayedPhysExplosion, CPhysExplosion );
	DECLARE_CLASS( CDelayedPhysExplosion, CPhysExplosion );

	~CDelayedPhysExplosion();

	void	Spawn ( void );
	void	Precache ( void );
//	void	Explode( CBaseEntity *pActivator );

	void	DelayedExplode ( float delay );
	void	DelayThink ();
	void	CreateDelayEffects (float delay);

	

	DECLARE_DATADESC();
private:

//	EHANDLE		m_hDelayEffect;
//	float		m_damage;
//	float		m_radius;
//	string_t	m_targetEntityName;
	CBaseEntity	*m_pSparker;
};


BEGIN_DATADESC( CDelayedPhysExplosion )
	//DEFINE_KEYFIELD( m_damage, FIELD_FLOAT, "magnitude" ),
	//DEFINE_KEYFIELD( m_radius, FIELD_FLOAT, "radius" ),
	DEFINE_THINKFUNC( DelayThink ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( env_delayed_physexplosion, CDelayedPhysExplosion );

CDelayedPhysExplosion::~CDelayedPhysExplosion()
{
//	CBaseEntity *m_pDelayEffect = m_hDelayEffect;
//	UTIL_Remove(m_pDelayEffect);

	UTIL_Remove(m_pSparker);
}

void CDelayedPhysExplosion::Spawn()
{
	Precache();

}


void CDelayedPhysExplosion::Precache()
{
	PrecacheScriptSound( "ZMPower.PhysExplode_Buildup" );
	PrecacheScriptSound( "ZMPower.PhysExplode_Boom" );

	BaseClass::Precache();
}

void CDelayedPhysExplosion::DelayedExplode( float delay )
{
	DevMsg("CDelayedPhysExplosion: initiating delaythink\n");

	CreateDelayEffects( delay );

//	if (m_hDelayEffect)
//		DevMsg("Delayeffect created succesfully\n");

	SetThink( &CDelayedPhysExplosion::DelayThink );
	SetNextThink( gpGlobals->curtime + delay );	
}

//--------------------------------------------------------------
// Stop sparking, make players drop things, and do our explosion
//--------------------------------------------------------------
void CDelayedPhysExplosion::DelayThink( )
{
	DevMsg("CDelayedPhysExplosion: think triggered, exploding at %f\n", gpGlobals->curtime);
	SetThink(NULL);

	//TGB: more sparklies
	g_pEffects->Sparks(GetAbsOrigin(), 10, 5);

	//TGB: woomp sound
	CPASAttenuationFilter filter( this, 1.0f);
	filter.MakeReliable();

	EmitSound_t ep;
	ep.m_pSoundName = "ZMPower.PhysExplode_Boom";
	ep.m_pOrigin = &GetAbsOrigin();

	EmitSound( filter, entindex(), ep );

	//make players in range drop their stuff, radius is cvar'd
	CBaseEntity *ent = NULL;
	for ( CEntitySphereQuery sphere( GetAbsOrigin(), zm_physexp_forcedrop_radius.GetFloat(), FL_CLIENT ); (ent = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity() )
	{
		CBasePlayer *pPlayer = ToBasePlayer( ent ); //will be null if not a player

		if (pPlayer != NULL)
			pPlayer->ForceDropOfCarriedPhysObjects(NULL);
					
	}

	//actual physics explosion
	Explode(NULL, this);
	
	//remove delay effects
	UTIL_Remove(m_pSparker);

	//another run for good measure
	g_pEffects->Sparks(GetAbsOrigin(), 15, 3);

	//TGB: clean ourselves up, else we stay around til round end
	UTIL_Remove(this);
}

void CDelayedPhysExplosion::CreateDelayEffects( float delay )
{
	//sound
	CPASAttenuationFilter filter( this, 0.6f);
	filter.MakeReliable();

	EmitSound_t ep;
	
	ep.m_pSoundName = "ZMPower.PhysExplode_Buildup";
	ep.m_pOrigin = &GetAbsOrigin();

	EmitSound( filter, entindex(), ep );

	//TGB: we want a particle effect instead
	g_pEffects->Sparks(this->GetAbsOrigin(), 1, 5);

	CBaseEntity *m_pSparker  = (CBaseEntity *)CreateEntityByName("env_spark");
	if (m_pSparker)
	{
		//set flags
		//copied over, see envspark.cpp
		const int SF_SPARK_START_ON			= 64;
		const int SF_SPARK_GLOW				= 128;
		const int SF_SPARK_SILENT			= 256;
		m_pSparker->AddSpawnFlags( SF_SPARK_START_ON );
		m_pSparker->AddSpawnFlags( SF_SPARK_GLOW );
		m_pSparker->AddSpawnFlags( SF_SPARK_SILENT );

		m_pSparker->KeyValue( "MaxDelay" , 0.1f );
		m_pSparker->KeyValue( "Magnitude" , 2 );
		m_pSparker->KeyValue( "TrailLength" , 1.5 );

		//modify delay to account for delayed dying of sparker
		delay -= 2.2f;
		m_pSparker->KeyValue( "DeathTime" , (gpGlobals->curtime + delay) );
		//DevMsg( "Sparker deathtime = %f\n", (gpGlobals->curtime + delay) );

		DispatchSpawn(m_pSparker);
		m_pSparker->Teleport( &GetAbsOrigin(), NULL, NULL );
	}
	//visual
//	m_pDelayEffect = CSprite::SpriteCreate( ZM_PHYSEXP_SPRITE, GetLocalOrigin(), FALSE );
//	m_pDelayEffect->SetTransparency( kRenderTransAddFrameBlend, 255, 255, 255, 0, kRenderFxNone );
	//CPhysExplosion *exp_ent = (CPhysExplosion *)CreateEntityByName("env_physexplosion");

	//CDelayedPhysExp_Effects *m_pDelayEffect  = (CDelayedPhysExp_Effects *)CreateEntityByName("env_physexp_effects");
	//m_hDelayEffect = m_pDelayEffect;
	////DevMsg("Attempted to create exp_ent\n");
	//if (m_pDelayEffect)
	//{
	//	DispatchSpawn(m_pDelayEffect);
	//	// Now attempt to drop into the world
	//	m_pDelayEffect->Teleport( &GetAbsOrigin(), NULL, NULL );
	//	m_pDelayEffect->Activate();
	//}
}

//------------------------------------------------------------------------------
// Purpose: TGB: Create a physics explosion at a chosen location
//------------------------------------------------------------------------------
#define SF_PHYSEXPLOSION_NODAMAGE			0x0001
#define SF_PHYSEXPLOSION_DISORIENT_PLAYER	0x0010
void ZM_Power_PhysExplode( void )
{
	Vector location = Vector( atof(engine->Cmd_Argv(1)), atof(engine->Cmd_Argv(2)), atof(engine->Cmd_Argv(3)) );
	if (location.IsValid() == false)
	{
		Warning("Invalid location for physexplode\n");
		return;
	}

	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );   //find a pointer to the player that the client controls
	if ( !pPlayer || (pPlayer && pPlayer->IsZM() == false) )
		return;
	
	if ( pPlayer->m_iZombiePool.Get() < zm_physexp_cost.GetInt() )
		return;

	//deduct sauce
	pPlayer->m_iZombiePool.GetForModify() -= zm_physexp_cost.GetInt();

	DevMsg("PhysExplode loc: %f %f %f\n", location.x, location.y, location.z);

	//TGB: I'm employing a method as seen in npc_create here

	//CPhysExplosion *exp_ent = (CPhysExplosion *)CreateEntityByName("env_physexplosion");
	CDelayedPhysExplosion *exp_ent = (CDelayedPhysExplosion *)CreateEntityByName("env_delayed_physexplosion");

	//DevMsg("Attempted to create exp_ent\n");
	if (exp_ent)
	{
		exp_ent->KeyValue( "magnitude", ZM_PHYSEXP_DAMAGE );
		exp_ent->KeyValue( "radius", ZM_PHYSEXP_RADIUS );
		//don't do damage, or wooden boxes and such will explode immediately
		exp_ent->AddSpawnFlags( SF_PHYSEXPLOSION_NODAMAGE );

		//disorienting could help balance
		exp_ent->AddSpawnFlags( SF_PHYSEXPLOSION_DISORIENT_PLAYER );

		//DevMsg("Dispatching exp_ent spawn\n");
		DispatchSpawn(exp_ent);

		// Now attempt to drop into the world
		exp_ent->Teleport( &location, NULL, NULL );

		//DevMsg("Exploding exp_ent (with delay)\n");
		exp_ent->Activate();
		//exp_ent->Explode( pPlayer );
		exp_ent->DelayedExplode( ZM_PHYSEXP_DELAY );


		ClientPrint(pPlayer, HUD_PRINTTALK, "Explosion created.");
	}
}

static ConCommand zm_power_physexplode("zm_power_physexplode", ZM_Power_PhysExplode, "Creates a physics explosion at a chosen location" );


//------------------------------------------------------------------------------
// Purpose: Lawyer:  Summon a shamblie in an out-of-the-way place
//------------------------------------------------------------------------------
void ZM_Power_SpotCreate( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );   //find a pointer to the player that the client controls
	if ( !pPlayer || (pPlayer && pPlayer->IsZM() == false) )
		return;

	Vector location = Vector( atof(engine->Cmd_Argv(1)), atof(engine->Cmd_Argv(2)), atof(engine->Cmd_Argv(3)) );

	if (location.IsValid() == false)
	{
		Warning("Invalid location for spot creation\n");
		return;
	}

	//TGB: BUGBUG: lifting up the location means the roof needs to be 72+50 units above or spawn will be denied
	//		instead, maybe try a few traces at 15-unit height decrements before denying

	//location.z += 50; //push up a bit

	//location.z += 15;

	//trace down to find the floor, in an attempt to prevent us spawning under displacements etc
	trace_t tr_floor;
	const Vector delta = Vector(0, 0, 25);
	Vector lifted_location = location + delta;
	Vector sunken_location = location - delta;
	UTIL_TraceHull(lifted_location, sunken_location, 
		NAI_Hull::Mins(HULL_HUMAN),
		NAI_Hull::Maxs(HULL_HUMAN),	
		MASK_NPCSOLID, NULL, COLLISION_GROUP_NONE, &tr_floor);

	if ( tr_floor.fraction == 1.0f )
	{
		DevWarning("Spotcreate: floor not found!\n");
		ClientPrint( pPlayer, HUD_PRINTCENTER, "The zombie does not fit in that location!\n" );
		return;
	}

	//whatever we hit should be our spawning location
	location = tr_floor.endpos;


	if ( pPlayer->m_iZombiePool.Get() < zm_spotcreate_cost.GetInt() )
		return;

	/*
	if ( zm_zombiemax.GetInt() <= pPlayer->m_iZombiePopCount )
	{
		//find the ZM so we can talk to him
		CBasePlayer *zmplayer = CBasePlayer::GetZM();
		if (zmplayer)
			ClientPrint( zmplayer, HUD_PRINTCENTER, "Maximum number of zombies reached!\n" );
		return;
	}*/

	CBasePlayer *pEntity = NULL;
	trace_t		tr;
	Vector		vecSpot;
	Vector		vecHeadTarget = location;

	vecHeadTarget.z += 64;
	//LAWYER:  Check if it's a valid spawnpoint first
	//LAWYER: Check for block brushes first D:

//	CTriggerBlockSpotCreate *pSelector = NULL;

	for ( int i = 0; i < gEntList.m_ZombieSpotCreateBlocker.Count(); i++)
	{

		CTriggerBlockSpotCreate *pSelector = dynamic_cast< CTriggerBlockSpotCreate * >(gEntList.m_ZombieSpotCreateBlocker[i]);
		if (pSelector)
		{
			if (pSelector->m_bActive && pSelector->CollisionProp())
			{
				//TGB: heh, turns out IsPointInBounds does all the work for us, without origin hassle

				/*
				Vector vecMins = pSelector->CollisionProp()->OBBMins();
				Vector vecMaxs = pSelector->CollisionProp()->OBBMaxs();
				Warning("Point at %f,%f,%f\n", location.x, location.y, location.z);
				Warning("Mins at %f,%f,%f\n", vecMins.x, vecMins.y, vecMins.z);
				Warning("Maxs at %f,%f,%f\n", vecMaxs.x, vecMaxs.y, vecMaxs.z);
				
				//Cycle through all of the blocker brushes
				if (vecMins.x <= location.x && vecMins.y <= location.y && vecMins.z <= location.z && vecMaxs.x >= location.x && vecMaxs.y >= location.y && vecMaxs.z >= location.z)
				*/
				if (pSelector->CollisionProp()->IsPointInBounds(location))
				{
					//CBasePlayer *zmplayer = CBasePlayer::GetZM();
					ClientPrint( pPlayer, HUD_PRINTTALK, "No hidden zombie may be created there\n" );
					return;
				}
			}
		}
	}


	//For each Player on the server
	// Check that the thing can 'see' this entity.
	for (int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pEntity = UTIL_PlayerByIndex( i );
		if (pEntity)
		{
			DevMsg("Spotspawn found player %i\n", i);
			vecSpot = pEntity->BodyTarget( location, false );
			UTIL_TraceLine( location, vecSpot, MASK_OPAQUE, NULL, COLLISION_GROUP_NONE, &tr ); //test at feet level

			bool visible = false;
			if ( tr.fraction == 1.0 && pEntity->GetTeamNumber() == 2)
			{
				DevMsg("Failed SpotCreate, %i is visible at feet\n", i);
				visible = true; //We've hit a Human!
			}

			UTIL_TraceLine( vecHeadTarget, vecSpot, MASK_OPAQUE, NULL, COLLISION_GROUP_NONE, &tr ); //Test at eye level

			if ( tr.fraction == 1.0 && pEntity->GetTeamNumber() == 2)
			{
				DevMsg("Failed SpotCreate, %i is visible at head\n", i);
				visible = true; //We've hit a Human!
			}

			if (visible) //one of the traces hit
			{
				//tell the ZM
				//CBasePlayer *zmplayer = CBasePlayer::GetZM();
				ClientPrint( pPlayer, HUD_PRINTCENTER, "One of the survivors can see this location!\n" );
				return;
			}

		}
	}

	//TGB: this check may be obsolete now that we do a tracehull to find the spawn location in the first place
	//LAWYER:  From Monstermaker, this checks for bad spawnpoints.
	trace_t trCheckShape;
	UTIL_TraceHull( location, 
		location + Vector( 0, 0, 1 ),
		NAI_Hull::Mins(HULL_HUMAN),
		NAI_Hull::Maxs(HULL_HUMAN),
		MASK_NPCSOLID,
		NULL,
		COLLISION_GROUP_NONE,
		&trCheckShape );

	if( trCheckShape.fraction != 1.0 )
	{
		//Warning("Doesn't fit there!\n");
		ClientPrint( pPlayer, HUD_PRINTCENTER, "The zombie does not fit in that location!\n" );
		return;
	}

	//Warning("SpotCreate Successful\n");

	DevMsg("SpotCreate loc: %f %f %f\n", location.x, location.y, location.z);

	//TGB: spawn stuff moved into spawnzombie func

	CNPC_BaseZombie *pZombie = CZombieSpawn::SpawnZombie("npc_zombie", location, pPlayer->GetAbsAngles());

	//DevMsg("Attempted to create exp_ent\n");
	if (pZombie)
	{
		//TGB: only deduct sauce if something spawned properly
		//deduct sauce
		pPlayer->m_iZombiePool.GetForModify() -= zm_spotcreate_cost.GetInt();

		ClientPrint(pPlayer, HUD_PRINTTALK, "Hidden zombie spawned.");
	}
}

static ConCommand zm_power_spotcreate("zm_power_spotcreate", ZM_Power_SpotCreate, "Creates a Shambler at target location, if it is unseen to players" );
