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
// Purpose:		Crowbar - an old favorite
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "gamerules.h"
#include "ammodef.h"
#include "mathlib.h"
#include "in_buttons.h"
#include "weapon_hl2mpbasebasebludgeon.h"
#include "vstdlib/random.h"
#include "npcevent.h"

#if defined( CLIENT_DLL )
	#include "c_hl2mp_player.h"
#else
	#include "hl2mp_player.h"
	#include "ai_basenpc.h"
#endif


//TGBNOTES
//  EXCLUDE EITHER THIS OR WEAPON_ZM_IMPROVISED
//  The basics of this is all working, but the rendering and bonemerging are less than ideal. The crowbar seems to be placed
//  in the actual world instead of just the viewmodel space. This creates clipping when walking into a wall for example.
//  It is also rendered as being behind the viewmodel.
//  Somehow it would need to be attached differently and/or rendered differently so that it plays nicer with the viewmodel.

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//should be dynamic at one point
#define	IMP_RANGE	55.0f
#define	IMP_REFIRE	1.0f


#ifdef CLIENT_DLL
#define CWeaponZMImprovised C_WeaponZMImprovised
#endif

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// CImprovisedModel
//-----------------------------------------------------------------------------
class CImprovisedModel : public C_BaseAnimating
{
public:
	//DECLARE_NETWORKCLASS();
	DECLARE_CLASS( CImprovisedModel, C_BaseAnimating );

	~CImprovisedModel ();

	void	Spawn();
	void	Precache( void );
	void	Attach( CBaseEntity *pOwner ); 

private:
};

LINK_ENTITY_TO_CLASS( improvised_weapon_model, CImprovisedModel );

CImprovisedModel::~CImprovisedModel()
{
	//Release();
}

void CImprovisedModel::Precache ( void )
{
	//UNUSED
	PrecacheModel( "models/weapons/imp_crowbar_zm.mdl" );
	//PrecacheModel( "models/weapons/w_crowbar.mdl" );
}

void CImprovisedModel::Spawn( void )
{

	DevMsg("Creating improvised weapon model\n");
	Precache();
	//SetSolid( SOLID_NONE );

	//SetModel ( "models/weapons/imp_crowbar_zm.mdl" );

	//SetModel ( "models/weapons/w_crowbar.mdl" );
	
}

void CImprovisedModel::Attach( CBaseEntity *pOwner )
{
	DevMsg("Attaching improvised weapon model\n");
	FollowEntity( pOwner, true );
	
	string_t name = GetModelName();
	DevMsg("ImpModel set to %s\n", name );

	//SetOwnerEntity( pOwner );
}
#endif //client_dll

//-----------------------------------------------------------------------------
// CWeaponZMImprovised
//-----------------------------------------------------------------------------

class CWeaponZMImprovised : public CBaseHL2MPBludgeonWeapon
{
public:
	DECLARE_CLASS( CWeaponZMImprovised, CBaseHL2MPBludgeonWeapon );

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();
#endif

	CWeaponZMImprovised();
	~CWeaponZMImprovised();

	float		GetRange( void )		{	return	IMP_RANGE;	}
	float		GetFireRate( void )		{	return	IMP_REFIRE;	}

	void		AddViewKick( void );
	float		GetDamageForActivity( Activity hitActivity );
	void		SecondaryAttack( void )	{	return;	}

	void		Drop( const Vector &vecVelocity );

	bool		Deploy ( void );
	bool		Holster( CBaseCombatWeapon *pSwitchingTo = NULL );

	void		Precache ();
	
	virtual int				GetWeaponTypeNumber( void ) { return 1; } //LAWYER:  In the CC group

	// Animation event
#ifndef CLIENT_DLL
	virtual void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	void HandleAnimEventMeleeHit( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	int WeaponMeleeAttack1Condition( float flDot, float flDist );
#endif

	CWeaponZMImprovised( const CWeaponZMImprovised & );

private:
#ifdef CLIENT_DLL
	CImprovisedModel *pWeapon;
#endif
		
};

//-----------------------------------------------------------------------------
// CWeaponZMImprovised
//-----------------------------------------------------------------------------

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponZMImprovised, DT_WeaponZMImprovised )

BEGIN_NETWORK_TABLE( CWeaponZMImprovised, DT_WeaponZMImprovised )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponZMImprovised )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_zm_improvised, CWeaponZMImprovised );
PRECACHE_WEAPON_REGISTER( weapon_zm_improvised );

#ifndef CLIENT_DLL

acttable_t	CWeaponZMImprovised::m_acttable[] = 
{
	{ ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_SLAM, true },
	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_MELEE,					false },
	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_MELEE,					false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_MELEE,			false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_MELEE,			false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_MELEE,	false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_MELEE,			false },
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_MELEE,					false },
};

IMPLEMENT_ACTTABLE(CWeaponZMImprovised);

#endif

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CWeaponZMImprovised::CWeaponZMImprovised(  )
{

}

CWeaponZMImprovised::~CWeaponZMImprovised(  )
{
#ifdef CLIENT_DLL
	if (pWeapon)
		pWeapon->Release();
#endif
}

void CWeaponZMImprovised::Precache()
{
	//precache to-be-attached model
	PrecacheModel( "models/Weapons/imp_crowbar_zm.mdl" );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Get the damage amount for the animation we're doing
// Input  : hitActivity - currently played activity
// Output : Damage amount
//-----------------------------------------------------------------------------
float CWeaponZMImprovised::GetDamageForActivity( Activity hitActivity )
{
	return 10.0f; //LAWYER:  It's a pants weapon.
}

//-----------------------------------------------------------------------------
// Purpose: Add in a view kick for this weapon
//-----------------------------------------------------------------------------
void CWeaponZMImprovised::AddViewKick( void )
{
	CBasePlayer *pPlayer  = ToBasePlayer( GetOwner() );

	if ( pPlayer == NULL )
		return;

	QAngle punchAng;

	punchAng.x = random->RandomFloat( 1.0f, 2.0f );
	punchAng.y = random->RandomFloat( -2.0f, -1.0f );
	
	pPlayer->ViewPunch( punchAng ); 
}


#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Animation event handlers
//-----------------------------------------------------------------------------
void CWeaponZMImprovised::HandleAnimEventMeleeHit( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	// Trace up or down based on where the enemy is...
	// But only if we're basically facing that direction
	Vector vecDirection;
	AngleVectors( GetAbsAngles(), &vecDirection );

	Vector vecEnd;
	VectorMA( pOperator->Weapon_ShootPosition(), 50, vecDirection, vecEnd );
	CBaseEntity *pHurt = pOperator->CheckTraceHullAttack( pOperator->Weapon_ShootPosition(), vecEnd, 
		Vector(-16,-16,-16), Vector(36,36,36), GetDamageForActivity( GetActivity() ), DMG_CLUB, 0.75 );
	
	// did I hit someone?
	if ( pHurt )
	{
		// play sound
		WeaponSound( MELEE_HIT );

		// Fake a trace impact, so the effects work out like a player's crowbar
		trace_t traceHit;
		UTIL_TraceLine( pOperator->Weapon_ShootPosition(), pHurt->GetAbsOrigin(), MASK_SHOT_HULL, pOperator, COLLISION_GROUP_NONE, &traceHit ); //LAWYER:  Try using this for Zombie blood decalling
		ImpactEffect( traceHit );
	}
	else
	{
		WeaponSound( MELEE_MISS );
	}
}


//-----------------------------------------------------------------------------
// Animation event
//-----------------------------------------------------------------------------
void CWeaponZMImprovised::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
	case EVENT_WEAPON_MELEE_HIT:
		HandleAnimEventMeleeHit( pEvent, pOperator );
		break;

	default:
		BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
		break;
	}
}

//-----------------------------------------------------------------------------
// Attempt to lead the target (needed because citizens can't hit manhacks with the crowbar!)
//-----------------------------------------------------------------------------
ConVar sk_crowbar_lead_time( "sk_crowbar_lead_time", "0.9" );

int CWeaponZMImprovised::WeaponMeleeAttack1Condition( float flDot, float flDist )
{
	// Attempt to lead the target (needed because citizens can't hit manhacks with the crowbar!)
	CAI_BaseNPC *pNPC	= GetOwner()->MyNPCPointer();
	CBaseEntity *pEnemy = pNPC->GetEnemy();
	if (!pEnemy)
		return COND_NONE;

	Vector vecVelocity;
	vecVelocity = pEnemy->GetSmoothedVelocity( );

	// Project where the enemy will be in a little while
	float dt = sk_crowbar_lead_time.GetFloat();
	dt += random->RandomFloat( -0.3f, 0.2f );
	if ( dt < 0.0f )
		dt = 0.0f;

	Vector vecExtrapolatedPos;
	VectorMA( pEnemy->WorldSpaceCenter(), dt, vecVelocity, vecExtrapolatedPos );

	Vector vecDelta;
	VectorSubtract( vecExtrapolatedPos, pNPC->WorldSpaceCenter(), vecDelta );

	if ( fabs( vecDelta.z ) > 70 )
	{
		return COND_TOO_FAR_TO_ATTACK;
	}

	Vector vecForward = pNPC->BodyDirection2D( );
	vecDelta.z = 0.0f;
	float flExtrapolatedDist = Vector2DNormalize( vecDelta.AsVector2D() );
	if ((flDist > 64) && (flExtrapolatedDist > 64))
	{
		return COND_TOO_FAR_TO_ATTACK;
	}

	float flExtrapolatedDot = DotProduct2D( vecDelta.AsVector2D(), vecForward.AsVector2D() );
	if ((flDot < 0.7) && (flExtrapolatedDot < 0.7))
	{
		return COND_NOT_FACING_ATTACK;
	}

	return COND_CAN_MELEE_ATTACK1;
}

#endif



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponZMImprovised::Drop( const Vector &vecVelocity )
{
//#ifndef CLIENT_DLL
//	if (pWeapon)
//		UTIL_Remove( pWeapon );
//#endif

#ifdef CLIENT_DLL
	if (pWeapon)
		pWeapon->Release();
#endif

	BaseClass::Drop( vecVelocity );
}

//-----------------------------------------------------------------------------
// Purpose: TGB: zoomhack
//-----------------------------------------------------------------------------
bool CWeaponZMImprovised::Deploy( void )
{

	CHL2MP_Player *pPlayer = ToHL2MPPlayer( GetOwner() );
	if (pPlayer)
	{
#ifndef CLIENT_DLL
		pPlayer->CreateViewModel( 1 );
#endif

		CBaseViewModel *pVM_new = pPlayer->GetViewModel( 1 );

		if (pVM_new)
		{
			pVM_new->SetWeaponModel( "models/Weapons/imp_crowbar_zm.mdl", NULL);

			CBaseViewModel *pVM_hands = pPlayer->GetViewModel();
			if (pVM_hands)
			{
				DevMsg("Attaching improvised weapon vmodel\n");
				pVM_new->FollowEntity( pVM_hands, true );
			}
			else
				Warning("Look ma, no hands\n");
		}
		else
		{
#ifndef CLIENT_DLL
			Warning("Viewmodel creation failed on server!\n");
#else
			Warning("Viewmodel creation failed on client!\n");
#endif
		}
	} //pPlayer



	/*
#ifdef CLIENT_DLL
	//CBasePlayer *pPlayer  = ToBasePlayer( GetOwner() );
	CHL2MP_Player *pPlayer = ToHL2MPPlayer( GetOwner() );
	if ( pPlayer )
	{
		CBaseViewModel *pVM = pPlayer->GetViewModel();
		if ( pVM )
		{
			//create weapon model
			
			pWeapon = new CImprovisedModel;
            if (pWeapon)
			{
				if ( pWeapon->InitializeAsClientEntity( "models/Weapons/imp_crowbar_zm.mdl", RENDER_GROUP_OPAQUE_ENTITY ) == false )
				{
					pWeapon->Release();
					Warning("CImprovisedModel init failed!\n");
					return BaseClass::Deploy();
				}

				//pWeapon->Spawn();
				pWeapon->Attach( pVM );
			
				if ( pWeapon->IsEffectActive( EF_BONEMERGE ) )
					DevMsg("Imp bonemerge correct\n");
				else
					Warning("Imp bonemerge failed\n");

				DevMsg("ImpModel is indeed set to %s\n", GetModelName() );
			}
			else
				Warning("CImprovisedModel creation failed!\n");
		}
	}
#endif
	*/

	return BaseClass::Deploy();

}

bool CWeaponZMImprovised::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	//destroy weapon model
//#ifndef CLIENT_DLL
//	if (pWeapon)
//		UTIL_Remove( pWeapon );
//#endif
#ifdef CLIENT_DLL
	if (pWeapon)
		pWeapon->Release();
#endif

	return BaseClass::Holster(pSwitchingTo);
}

