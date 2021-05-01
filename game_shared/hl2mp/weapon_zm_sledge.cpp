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
// Purpose:		Sledgehammer - a hard-hitting favorite
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
#include "eventlist.h"

#if defined( CLIENT_DLL )
#include "c_hl2mp_player.h"
#else
#include "hl2mp_player.h"
#include "ai_basenpc.h"
#include "ilagcompensationmanager.h"
#endif


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//will probably be tweaked often in balancing
//TGB: reduced a bit for the new animations
#define	SLEDGE_RANGE	60.0f
#define	SLEDGE_REFIRE	2.9f

#ifdef CLIENT_DLL
#define CWeaponZMSledge C_WeaponZMSledge
#endif

//-----------------------------------------------------------------------------
// CWeaponZMSledge
//-----------------------------------------------------------------------------

class CWeaponZMSledge : public CBaseHL2MPBludgeonWeapon
{
public:

	DECLARE_CLASS( CWeaponZMSledge, CBaseHL2MPBludgeonWeapon );

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

//#ifndef CLIENT_DLL
//
//	int UpdateTransmitState()
//	{
//		return SetTransmitState(FL_EDICT_ALWAYS);
//	}
//
//#endif

#ifndef CLIENT_DLL

	DECLARE_ACTTABLE();

#endif

	 CWeaponZMSledge();
	~CWeaponZMSledge();


	float		GetRange( void )		{	return	SLEDGE_RANGE;	}
	float		GetFireRate( void )		{	return	SLEDGE_REFIRE;	}

	void		AddViewKick( void );
	float		GetDamageForActivity( Activity hitActivity );
	void		PrimaryAttack( void );
	void		SecondaryAttack( void );


	void		ItemPostFrame (void);
	void		Drop( const Vector &vecVelocity );

	bool		Deploy ( void );
	bool		Holster( CBaseCombatWeapon *pSwitchingTo = NULL );

	virtual int				GetWeaponTypeNumber( void ) { return 1; } //LAWYER:  In the CC group

	// Animation event
	void			HandleAnimEventMeleeHit(CBaseCombatCharacter *pOperator );
#ifndef CLIENT_DLL

	virtual void	Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	int				WeaponMeleeAttack1Condition( float flDot, float flDist );

//	CNetworkVar( bool, m_bFlashlightDestroy);
//	CNetworkVar( bool, m_bFlashlightCreate);

#else
	bool	OnFireEvent( C_BaseViewModel *pViewModel, const Vector& origin, const QAngle& angles, int event, const char *options );

#endif

	
	CWeaponZMSledge( const CWeaponZMSledge & );


private:

	//CNetworkVar( bool, m_bSwinging );
	
	bool m_bSecondaryAttack;
	//float m_flResetAttack;
};




//-----------------------------------------------------------------------------
// CWeaponZMSledge
//-----------------------------------------------------------------------------

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponZMSledge, DT_WeaponZMSledge )

BEGIN_NETWORK_TABLE( CWeaponZMSledge, DT_WeaponZMSledge )
// #ifdef CLIENT_DLL
// 
// RecvPropBool( RECVINFO( m_bMeleeHit ) ),
// RecvPropBool( RECVINFO( m_bSwinging ) ),
// 
// #else
// 
// SendPropBool( SENDINFO( m_bMeleeHit ) ),
// SendPropBool( SENDINFO( m_bSwinging ) ),
// 
// #endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponZMSledge )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_zm_sledge, CWeaponZMSledge );
PRECACHE_WEAPON_REGISTER( weapon_zm_sledge );

#ifndef CLIENT_DLL

//TGB: changed into custom animations
acttable_t	CWeaponZMSledge::m_acttable[] = 
{
	{ ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_SLAM, true },
	{ ACT_HL2MP_IDLE,					ACT_ZM_IDLE_SLEDGE,					false },
	{ ACT_HL2MP_RUN,					ACT_ZM_RUN_SLEDGE,					false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_ZM_IDLE_CROUCH_SLEDGE,			false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_ZM_WALK_CROUCH_SLEDGE,			false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_ZM_GESTURE_RANGE_ATTACK_SLEDGE,	false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_MELEE,			false },
	{ ACT_HL2MP_JUMP,					ACT_ZM_JUMP_SLEDGE,					false },
};

IMPLEMENT_ACTTABLE(CWeaponZMSledge);


#endif

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CWeaponZMSledge::CWeaponZMSledge( void )
{
// 	m_bMeleeHit = false;
// 	m_bSwinging = false;
	m_bSecondaryAttack = false;
//	m_flResetAttack = 0;

}


CWeaponZMSledge::~CWeaponZMSledge( void )
{

}



//-----------------------------------------------------------------------------
// Purpose: Get the damage amount for the animation we're doing
// Input  : hitActivity - currently played activity
// Output : Damage amount
//-----------------------------------------------------------------------------
float CWeaponZMSledge::GetDamageForActivity( Activity hitActivity )
{
	//TGB: the damage mech here is something I just thought up, and probably not final

	//TGB: changed some values to fit better with new anims, primary should be weaker
	
	//base damage is 50, enough to instakill base zombie
	float damage = 50.0f;

	//apply random factor to base damage
	//we should now often hit for 0.8 to 0.99 % of the damage, which is not enough to instakill
	//sometimes we do the full 50 damage and instakill
	if (hitActivity == ACT_VM_HITCENTER && !m_bSecondaryAttack )
		damage *= random->RandomFloat( 0.55f, 1.1f );

	//in the alternate attack we have a higher chance of instakilling
	//in fact, we can deal a rather large amount of damage that will really send it flying 
	if (hitActivity == ACT_VM_HITCENTER && m_bSecondaryAttack )
	{
		damage *= random->RandomFloat( 0.99f, 2.5f );

	}

	//Msg("\nSledge damage: %f, in attack %i", damage, hitActivity);
	return damage;
}

//-----------------------------------------------------------------------------
// Purpose: Add in a view kick for this weapon
//-----------------------------------------------------------------------------
void CWeaponZMSledge::AddViewKick( void )
{
	CBasePlayer *pPlayer  = ToBasePlayer( GetOwner() );

	if ( pPlayer == NULL )
		return;

	QAngle punchAng;

	punchAng.x = random->RandomFloat( 1.0f, 2.0f );
	punchAng.y = random->RandomFloat( -2.0f, -1.0f );
	punchAng.z = 0.0f;

	pPlayer->ViewPunch( punchAng ); 
}



//-----------------------------------------------------------------------------
// Animation event handlers
//-----------------------------------------------------------------------------
void CWeaponZMSledge::HandleAnimEventMeleeHit( CBaseCombatCharacter *pOperator )
{
	//do the trace stuff here so we can pass it to the Hit() function
	trace_t traceHit;

	// Try a ray
	CBasePlayer *pOwner = ToBasePlayer(pOperator);
	if ( !pOwner )
		return;

	Vector swingStart = pOwner->Weapon_ShootPosition( );
	Vector forward;

	pOwner->EyeVectors( &forward, NULL, NULL );

	Vector swingEnd = swingStart + forward * GetRange();
#ifndef CLIENT_DLL
	CHL2MP_Player *pPlayer = ToHL2MPPlayer( GetPlayerOwner() );
	// Move other players back to history positions based on local player's lag
	lagcompensation->StartLagCompensation( pPlayer, pPlayer->GetCurrentCommand() );
#endif
	UTIL_TraceLine( swingStart, swingEnd, MASK_SHOT_HULL, pOwner, COLLISION_GROUP_NONE, &traceHit );
	Hit( traceHit, ACT_VM_HITCENTER);
#ifndef CLIENT_DLL
	// Move other players back to history positions based on local player's lag
	lagcompensation->FinishLagCompensation( pPlayer );
#endif

}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Attempt to lead the target (needed because citizens can't hit manhacks with the crowbar!)
//-----------------------------------------------------------------------------
//ConVar sk_crowbar_lead_time( "sk_crowbar_lead_time", "0.9" );

int CWeaponZMSledge::WeaponMeleeAttack1Condition( float flDot, float flDist )
{
	// Attempt to lead the target (needed because citizens can't hit manhacks with the crowbar!)
	CAI_BaseNPC *pNPC	= GetOwner()->MyNPCPointer();
	CBaseEntity *pEnemy = pNPC->GetEnemy();
	if (!pEnemy)
		return COND_NONE;

	Vector vecVelocity;
	vecVelocity = pEnemy->GetSmoothedVelocity( );

	// Project where the enemy will be in a little while
	float dt = 0.9;
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
void CWeaponZMSledge::Drop( const Vector &vecVelocity )
{
/*#ifndef CLIENT_DLL
	UTIL_Remove( this );
#endif*/
	BaseClass::Drop( vecVelocity );
}

//-----------------------------------------------------------------------------
// Purpose: Draw weapon and start flashlighting
//-----------------------------------------------------------------------------
bool CWeaponZMSledge::Deploy( void )
{
//	m_bSwinging = false; //init swinging check bool

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: Holster weapon and remove flashlight effect
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponZMSledge::Holster( CBaseCombatWeapon *pSwitchingTo )
{

	return BaseClass::Holster( pSwitchingTo );
}


//-----------------------------------------------------------------------------
// Purpose: Update flashlight
//-----------------------------------------------------------------------------
void CWeaponZMSledge::ItemPostFrame()
{

// #ifndef CLIENT_DLL
// 
// 	if ( m_flResetAttack > 0 && m_flResetAttack < gpGlobals->curtime && m_bSwinging )
// 	{
// 		DevMsg("Not swinging nor hitting shit now\n");
// 		//m_bMeleeHit = false;
// 		m_bSwinging = false;
// 		m_bSecondaryAttack = false;
// 	}
// #endif
	
	BaseClass::ItemPostFrame();
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Animation event
//-----------------------------------------------------------------------------
void CWeaponZMSledge::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
	case AE_ZM_MELEEHIT:
		HandleAnimEventMeleeHit( pOperator );
		break;

	default:
		BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
		break;
	}
}
#else
//--------------------------------------------------------------
// TGB: turns out we can see anim events on the client as well, d'oh, so lets use them
//--------------------------------------------------------------
bool CWeaponZMSledge::OnFireEvent( C_BaseViewModel *pViewModel, const Vector& origin, const QAngle& angles, int event, const char *options )
{
	if (event == AE_ZM_MELEEHIT)
	{
		//DevMsg("Heard MeleeHit on client!\n");
		HandleAnimEventMeleeHit(GetOwner());

		return true;
	}
	return false;
}
#endif


//-----------------------------------------------------------------------------
// Purpose: Deal with primary attack animations
//-----------------------------------------------------------------------------
void CWeaponZMSledge::PrimaryAttack()
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if (!pPlayer || pPlayer->GetMoveType()==MOVETYPE_LADDER)
		return;

	m_bSecondaryAttack = false;

	
	// Send the anims
	//DevMsg("\nDispatching sledge animation.");
	SendWeaponAnim( ACT_VM_HITCENTER );
	//set the time we will be able to attack again
	//m_flResetAttack = gpGlobals->curtime + SequenceDuration() + SLEDGE_REFIRE;
	//primary attack sound
	WeaponSound( SPECIAL1 );

	pPlayer->SetAnimation( PLAYER_ATTACK1 );
	

	// we can attack again soon
	m_flNextPrimaryAttack = gpGlobals->curtime + SLEDGE_REFIRE;
	m_flNextSecondaryAttack = gpGlobals->curtime + (SLEDGE_REFIRE * 1.2f);
}

//-----------------------------------------------------------------------------
// Purpose: Deal with secondary attack animations
//-----------------------------------------------------------------------------
void CWeaponZMSledge::SecondaryAttack()
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( !pOwner )
		return;
	//can't do this swing if we're crouching as it's physically impossible
	//TGB: UNDONE: new anims
//	if ( pOwner->GetFlags() & FL_DUCKING )
//		return;
	if (pOwner->GetMoveType()==MOVETYPE_LADDER)
		return;

	
	m_bSecondaryAttack = true;
	// Send the _SECONDARY_ anims
	SendWeaponAnim( ACT_VM_HITCENTER2 );
	//set the time we will be able to attack again
	//m_flResetAttack = gpGlobals->curtime + SequenceDuration() + (SLEDGE_REFIRE * 2.5);
	//secondary attack sound
	WeaponSound( SPECIAL1 );

	//no specific secondary anims yet for this
	pOwner->SetAnimation( PLAYER_ATTACK1 ); 
	

	//we can do primary prods quicker than a full swing
	m_flNextPrimaryAttack = gpGlobals->curtime + (SLEDGE_REFIRE * 1.7f);
	m_flNextSecondaryAttack = gpGlobals->curtime + (SLEDGE_REFIRE * 1.8f);
}
