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

#include "cbase.h"
#include "player.h"
#include "ammodef.h"
#include "gamerules.h"
#include "grenade_molotov.h"
#include "weapon_brickbat.h"
#include "soundent.h"
#include "decals.h"
#include "fire.h"
#include "shake.h"
#include "ndebugoverlay.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "props.h"

//qck edit
#include "gib.h"

#define NUM_FIREBALLS 13 //qck: Picked for luck factor

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern short	g_sModelIndexFireball;

extern ConVar    sk_plr_dmg_molotov;
extern ConVar    sk_npc_dmg_molotov;
ConVar    sk_molotov_radius			( "sk_molotov_radius","0");

#define MOLOTOV_EXPLOSION_VOLUME	1024

#ifndef CLIENT_DLL
BEGIN_DATADESC( CGrenade_Molotov )

	DEFINE_FIELD( m_pFireTrail, FIELD_CLASSPTR ),

	// Function Pointers
	//TGBMERGENOTE: using _FUNCTION here errors out for mysterious reasons
	DEFINE_ENTITYFUNC ( MolotovTouch ),
	//DEFINE_FUNCTION( MolotovTouch ),
	//DEFINE_FUNCTION( MolotovThink ),
	DEFINE_THINKFUNC( MolotovThink ),

END_DATADESC()
#endif

LINK_ENTITY_TO_CLASS( grenade_molotov, CGrenade_Molotov );

void CGrenade_Molotov::Spawn( void )
{
	Precache( );

	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
	SetSolid( SOLID_BBOX ); 
	SetCollisionGroup( COLLISION_GROUP_PROJECTILE );
	RemoveEffects( EF_NOINTERP );

	//TGB: custom model
	SetModel( "models/weapons/molotov3rd_zm.mdl");

	//UTIL_SetSize(this, Vector( -6, -6, -2), Vector(6, 6, 2));
	
	//Expanded slightly due to Linux touchy crap
	SetTouch( &CGrenade_Molotov::MolotovTouch );
	SetThink( &CGrenade_Molotov::MolotovThink );

	SetNextThink( gpGlobals->curtime + 0.1f );

	m_flDamage		= 40;
	m_DmgRadius		= 128;

	m_takedamage	= DAMAGE_YES;
	m_iHealth		= 1;

	SetGravity( 1.0 );
	SetFriction( 0.8 );  // Give a little bounce so can flatten
	SetSequence( 1 );

	CFireTrail* fire = CFireTrail::CreateFireTrail();

	if ( fire )
	{
		fire->FollowEntity( this, "flame" );
		fire->SetLocalOrigin( vec3_origin );
		fire->SetMoveType( MOVETYPE_NONE );
		fire->SetLifetime( 20.0f ); 
		fire->m_StartSize = random->RandomFloat(1.0f, 2.0f);
		fire->m_EndSize = random->RandomFloat(3.5f, 5.0f);
	}
	
	/*
	//confusing nameage here
	m_pFireTrail = SmokeTrail::CreateSmokeTrail();

	if( m_pFireTrail )
	{
		m_pFireTrail->m_SpawnRate			= 48;
		m_pFireTrail->m_ParticleLifetime	= 1.0f;
		
		m_pFireTrail->m_StartColor.Init( 0.2f, 0.2f, 0.2f );
		m_pFireTrail->m_EndColor.Init( 0.0, 0.0, 0.0 );
		
		m_pFireTrail->m_StartSize	= 8;
		m_pFireTrail->m_EndSize		= 32;
		m_pFireTrail->m_SpawnRadius	= 4;
		m_pFireTrail->m_MinSpeed	= 8;
		m_pFireTrail->m_MaxSpeed	= 16;
		m_pFireTrail->m_Opacity		= 0.25f;

		m_pFireTrail->SetLifetime( 20.0f );
		m_pFireTrail->FollowEntity( this, "flame" );
	}
	*/
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CGrenade_Molotov::MolotovTouch( CBaseEntity *pOther )
{
	// Don't touch triggers (but DO hit weapons) //LAWYER: Shamelessly stolen from the RPG missile
	if ( pOther->IsSolidFlagSet(FSOLID_TRIGGER|FSOLID_VOLUME_CONTENTS) && pOther->GetCollisionGroup() != COLLISION_GROUP_WEAPON )
		return;

	Detonate();
}


//qck: The flaming chunks of flame and debris which go flying everywhere upon detonation
//I'm going off the assumption that our molotovs are mixed with tar, making them act more
//like napalm than anything. It's fun to watch it go all over. 

void CGrenade_Molotov::CreateFlyingChunk( const Vector &vecChunkPos, const QAngle &vecChunkAngles, const char *pszChunkName, bool bSmall )
{
	//qck: This basically creates debris, makes them invisible, sends them flying, and attachs a neat
	//fire particle effect. 
	CFlamingGib *pChunk = CREATE_ENTITY( CFlamingGib, "flaming_gib" );
	pChunk->Spawn( pszChunkName );
	pChunk->SetBloodColor( DONT_BLEED );
	
	//qck: This pulls the flames up and out of the ground a bit
	//Vector flameOrigin = vecChunkPos;
	//flameOrigin.z *= 1.5;

	pChunk->SetAbsOrigin( vecChunkPos );
	pChunk->SetAbsAngles( vecChunkAngles );
	pChunk->SetOwnerEntity( this );
	
	
	pChunk->m_lifeTime = 15.0f; //TGB: reduced, was 20

	//TGB: these gibs were never dying
	pChunk->SetNextThink( gpGlobals->curtime + pChunk->m_lifeTime );
	pChunk->SetThink ( &CGib::DieThink );

	//TGB: the fires were attempting to hurt the gibs, which is silly
	pChunk->m_takedamage = DAMAGE_NO;
	
	pChunk->SetCollisionGroup( COLLISION_GROUP_DEBRIS );
	pChunk->AddEffects( EF_NODRAW );
	
	// Set the velocity
	Vector vecVelocity;
	AngularImpulse angImpulse;

	QAngle angles;
	angles.x = random->RandomFloat( -70, 20 );
	angles.y = random->RandomFloat( 0, 360 );
	angles.z = 0.0f;
	AngleVectors( angles, &vecVelocity );
	
	//qck: Fire spawns all over the place.
	vecVelocity *= random->RandomFloat( 10, 20 );
	vecVelocity.z *= random->RandomFloat( 10, 20);
	vecVelocity.y *= random->RandomFloat( 5, 10 );
	vecVelocity.x *= random->RandomFloat( 5, 10 );

	angImpulse = RandomAngularImpulse( -90, 90 );

	pChunk->SetAbsVelocity( vecVelocity );
	//qck: This might be useful for sticky flames
	//pChunk->SetMoveType( MOVETYPE_NONE );

	//qck: SOLID_NONE allows the physobject to vanish after a short period of time (required), and allows it to stick to some surfaces.
	IPhysicsObject *pPhysicsObject = pChunk->VPhysicsInitNormal( SOLID_NONE	, pChunk->GetSolidFlags(), false );
		
	if ( pPhysicsObject )
	{
		pPhysicsObject->EnableMotion( true );
		pPhysicsObject->SetVelocity(&vecVelocity, &angImpulse );
	}
	
	
	//qck: Light the chunks up
	CFireTrail *pFireTrail = CFireTrail::CreateFireTrail();

	if ( pFireTrail == NULL )
		return;

	pFireTrail->FollowEntity( pChunk, "" );
	pFireTrail->SetParent( pChunk, 0 );
	pFireTrail->SetLocalOrigin( vec3_origin );
	pFireTrail->SetMoveType( MOVETYPE_NONE );
	pFireTrail->SetLifetime( pChunk->m_lifeTime - 1.0f);
	pFireTrail->m_StartSize = 15; //TGB: made the chunks smaller
	pFireTrail->m_EndSize = 6;

}
//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CGrenade_Molotov::Detonate( void ) 
{
	SetModelName( NULL_STRING );		//invisible
	AddSolidFlags( FSOLID_NOT_SOLID );	// intangible

	m_takedamage = DAMAGE_NO;
	
	//qck: I changed the third argument for Vector from -128 to 0. I'm not sure why it was at -128, but it
	//caused the grenade to explode at odd locations
	trace_t trace;
	//TGB: I'm not sure why we do a separate trace here if we only trace from our position to... our position
	//	better off using the trace that actually caused us to hit something
	//	this fixed problems with fire not appearing or appearing below displacements, etc.
	trace = BaseClass::GetTouchTrace();
	/*UTIL_TraceLine ( GetAbsOrigin(), GetAbsOrigin() + Vector ( 0, 0, 0 ),  MASK_SOLID_BRUSHONLY, 
		this, COLLISION_GROUP_NONE, &trace);*/

	// Pull out of the wall a bit
	if ( trace.fraction != 1.0 )
	{
		SetLocalOrigin( trace.endpos + (trace.plane.normal * (m_flDamage - 24) * 0.6) );
	}

	int contents = UTIL_PointContents ( GetAbsOrigin() );
	
	if ( (contents & MASK_WATER) )
	{
		UTIL_Remove( this );
		return;
	}

	EmitSound( "Grenade_Molotov.Detonate");
	EmitSound( "Grenade_Molotov.Detonate2");

	for( int x = 0; x < NUM_FIREBALLS; x++)
	{
		CreateFlyingChunk( GetAbsOrigin(), GetAbsAngles(), g_PropDataSystem.GetRandomChunkModel( "MetalChunks" ), false);
	}

	//TGB: reintroducing firestarting for a bit

// Start some fires
	int i;
	QAngle vecTraceAngles;
	Vector vecTraceDir;
	trace_t firetrace;

	for( i = 0 ; i < 5 ; i++ )
	{
		// build a little ray
		vecTraceAngles[PITCH]	= random->RandomFloat(45, 135);
		vecTraceAngles[YAW]		= random->RandomFloat(0, 360);
		vecTraceAngles[ROLL]	= 0.0f;

		AngleVectors( vecTraceAngles, &vecTraceDir );

		Vector vecStart, vecEnd;

		vecStart = GetAbsOrigin() + ( trace.plane.normal * 48 ); //tgb: was * 128
		//LAWYER:  Try to make fires more reliably apparent
		//TGB: still fails in some areas, lifted higher up
		vecStart.z = vecStart.z + 32;
		vecEnd = vecStart + vecTraceDir * 300;

		UTIL_TraceLine( vecStart, vecEnd, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &firetrace );

		Vector	ofsDir = ( firetrace.endpos - GetAbsOrigin() );
		float	offset = VectorNormalize( ofsDir );

		if ( offset > 128 )
			offset = 128;

		//Get our scale based on distance
		//float scale	 = 0.4f + ( 0.75f * ( 1.0f - ( offset / 128.0f ) ) );
		float growth = 0.8f + ( 0.75f * ( offset / 128.0f ) );
		float scale = 125.0f;

		if( firetrace.fraction != 1.0 )
		{
			//FireSystem_StartFireParented( firetrace.endpos, scale, growth, 20.0f, (SF_FIRE_START_ON|SF_FIRE_SMOKELESS), this->GetOwnerEntity(), FIRE_NATURAL, m_pDamageParent ); //LAWYER:  This is causing teamkills
			//TGB: just use the fire's owner as damage parent
			FireSystem_StartFire( firetrace.endpos, scale, growth, 20.0f, (SF_FIRE_START_ON|SF_FIRE_SMOKELESS), m_pDamageParent , FIRE_NATURAL);
		}
		else
		{
			//TGB: better to spawn a bit of fire *somewhere* than none at all
			//FireSystem_StartFireParented( trace.endpos + ( trace.plane.normal * 12 ), scale, growth, 10.0f, (SF_FIRE_START_ON), this->GetOwnerEntity(), FIRE_NATURAL, m_pDamageParent );
			FireSystem_StartFire( trace.endpos + ( trace.plane.normal * 12 ), scale, growth, 10.0f, (SF_FIRE_START_ON), m_pDamageParent, FIRE_NATURAL );
		}
	}
// End Start some fires
	
	//CPASFilter filter2( trace.endpos );

	//te->Explosion( filter2, 0.0,
	//	&trace.endpos, 
	//	g_sModelIndexFireball,
	//	2.0, 
	//	15,
	//	TE_EXPLFLAG_NOPARTICLES,
	//	m_DmgRadius,
	//	m_flDamage );

	CBaseEntity *pOwner = GetOwnerEntity();
	SetOwnerEntity( NULL ); // can't traceline attack owner if this is set

	UTIL_DecalTrace( &trace, "Scorch" );

	UTIL_ScreenShake( GetAbsOrigin(), 10.0, 60.0, 1.0, 100, SHAKE_START ); //LAWYER:  Not nearly explosive enough to cause a quake
	//qck: I made it shake the screen a small bit when you're really, really close to the explosion. It adds to the drama of the thing :p
	
	//CSoundEnt::InsertSound ( SOUND_DANGER, GetAbsOrigin(), BASEGRENADE_EXPLOSION_VOLUME, 3.0 ); 

	//TGB: check to make sure we don't deal damage if the parent has disconnected, likely to be a griefer attempting to get around teamkill checks
	if (m_pDamageParent != NULL && 
		m_pDamageParent->IsDisconnecting() == false && //these two are not mutually exclusive as you might think
		m_pDamageParent->IsConnected() == true)
	{
		//qck: Anything in the initial blast should take some serious damage
		RadiusDamage( CTakeDamageInfo( this, m_pDamageParent, m_flDamage, DMG_BURN ), GetAbsOrigin(), m_DmgRadius, CLASS_NONE, NULL );

		pOwner = NULL;
	}
	//if our parent is gone, we don't deal the instant damage, but we CAN safely start the fires
	

	//LAWYER:  Some crazy burn effects, ported from the BurnZombie
	//TGB: trace check to avoid burn-through-walls, pre-post-back-ported from burnzombie
	CBaseEntity *pObject = NULL;
	const Vector vecSource = GetAbsOrigin();
	Vector vecSpot;
	while ( ( pObject = gEntList.FindEntityInSphere( pObject, this->GetAbsOrigin(), m_DmgRadius ) ) != NULL )
	{
		vecSpot = pObject->BodyTarget( vecSource, false );
		UTIL_TraceLine( vecSource, vecSpot, MASK_SOLID, NULL, COLLISION_GROUP_NONE, &trace );

		if ( trace.fraction == 1.0)
		{
			//TGB: instead of igniting players, it ignites YOU, the thrower
			if ((pObject == pOwner || pObject->IsNPC()) && pObject != this)
			{
				//LAWYER:  It's alive.  Give it a bit of burnination!
				CBaseCombatCharacter *pOther = dynamic_cast<CBaseCombatCharacter *>(pObject);
				if (pOther && pOther->GetWaterLevel() < 3)
				{
					//DevMsg("Igniting %s\n", pOther->GetClassName());
					pOther->Ignite(100.0f);
				}
			}
			else
			{
				CBreakableProp *pProp = dynamic_cast<CBreakableProp *>(pObject);
				if (pProp)
				{
					pProp->Ignite(100.0f,false);
				}
			}
		}
	}


	AddEffects( EF_NODRAW );
	SetAbsVelocity( vec3_origin );
	SetNextThink( gpGlobals->curtime + 0.2 );

	if ( m_pFireTrail )
	{
		UTIL_Remove( m_pFireTrail );
	}

	UTIL_Remove(this);
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CGrenade_Molotov::MolotovThink( void )
{
	//LAWYER:  Check for waterness, as it should remove firebombs
	int contents = UTIL_PointContents ( GetAbsOrigin() );
	
	if ( (contents & MASK_WATER) )
	{
		UTIL_Remove( this );
		return;
	}

	// See if I can lose my owner (has dropper moved out of way?)
	// Want do this so owner can throw the brickbat
	if (GetOwnerEntity())
	{
		trace_t tr;
		Vector	vUpABit = GetAbsOrigin();
		vUpABit.z += 5.0;

		CBaseEntity* saveOwner	= GetOwnerEntity();
		SetOwnerEntity( NULL );
		UTIL_TraceEntity( this, GetAbsOrigin(), vUpABit, MASK_SOLID, &tr );
		if ( tr.startsolid || tr.fraction != 1.0 )
		{
			SetOwnerEntity( saveOwner );
		}
	}
	SetNextThink( gpGlobals->curtime + 0.1f );
}

void CGrenade_Molotov::Precache( void )
{
	BaseClass::Precache();

	PrecacheModel("models/weapons/molotov3rd_zm.mdl");

	UTIL_PrecacheOther("_firesmoke");

	PrecacheScriptSound( "Grenade_Molotov.Detonate" );
}

void CGrenade_Molotov::SetVelocity( const Vector &velocity, const AngularImpulse &angVelocity )
{
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if ( pPhysicsObject )
	{
		pPhysicsObject->AddVelocity( &velocity, &angVelocity );
	}
}
