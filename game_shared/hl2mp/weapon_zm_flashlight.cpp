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
// Purpose:		Flashlight - a new favorite
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
#include "flashlighteffect.h"
#include "model_types.h"
#include "beamdraw.h"
#include "fx_line.h"
#include "view.h"
#include "iviewrender_beams.h"

#include "dlight.h" 
#include "iefx.h"

extern void FormatViewModelAttachment( int iAttachment, Vector &vOrigin, QAngle &angle ); // qck edit
extern void FormatViewModelAttachment( Vector &vOrigin, bool bInverse ); // qck edit
ConVar r_FlashlightIsOn( "r_FlashlightIsOn", "false", FCVAR_NONE, "Is the flashlight on?");

#else
#include "hl2mp_player.h"
#include "ai_basenpc.h"
#endif


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	CROWBAR_RANGE	50.0f
#define	CROWBAR_REFIRE	0.6f

#define FLASHLIGHT_DISTANCE		100 //qck edit


#ifdef CLIENT_DLL
#define CWeaponZMFlashlight C_WeaponZMFlashlight
#endif

//-----------------------------------------------------------------------------
// CWeaponZMFlashlight
//-----------------------------------------------------------------------------

class CWeaponZMFlashlight : public CBaseHL2MPBludgeonWeapon
{
public:

	DECLARE_CLASS( CWeaponZMFlashlight, CBaseHL2MPBludgeonWeapon );

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

#ifndef CLIENT_DLL

	int UpdateTransmitState()
	{
		return SetTransmitState(FL_EDICT_ALWAYS);
	}

#endif

#ifndef CLIENT_DLL

	DECLARE_ACTTABLE();

#endif

	 CWeaponZMFlashlight();
	~CWeaponZMFlashlight();


	float		GetRange( void )		{	return	CROWBAR_RANGE;	}
	float		GetFireRate( void )		{	return	CROWBAR_REFIRE;	}

	void		AddViewKick( void );
	float		GetDamageForActivity( Activity hitActivity );
	void		PrimaryAttack( void );
	void		SecondaryAttack( void )	{	return;	}


	void		ItemPostFrame (void);
	void		ItemHolsterFrame (void);
	void		Drop( const Vector &vecVelocity );

	bool		Deploy ( void );
	bool		Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	void		GetWeaponAttachment( int attachmentId, Vector &outVector, Vector *dir /*= NULL*/ );
	//TGB: uncommented for testing
	void		DrawEffects();


	void Precache();

	


#ifdef CLIENT_DLL

	CFlashlightEffect *m_pFlashlightBeam;

	void			UpdateFlashlight();	
	void			OnDataChanged( DataUpdateType_t updateType );   


	virtual void	Simulate();				


	bool			m_bFlashlightDestroy;
	bool			m_bFlashlightCreate;

	//TGB: bool for check on whether to draw a third person beam
	bool			m_bThirdPersonLight;

#endif

	// Animation event
#ifndef CLIENT_DLL

	virtual void	Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	void			HandleAnimEventMeleeHit( animevent_t *pEvent, CBaseCombatCharacter *pOperator );

	int				WeaponMeleeAttack1Condition( float flDot, float flDist );

	//TGB: Anim handling for fixed melee timing
	virtual void	HandleAnimEvent( animevent_t *pEvent );

	CNetworkVar( bool, m_bFlashlightDestroy);
	CNetworkVar( bool, m_bFlashlightCreate);
#endif

	
	CWeaponZMFlashlight( const CWeaponZMFlashlight & );


private:

	CNetworkVar( bool, m_bSwinging );
	CNetworkVar( bool, m_bMeleeHit );

	float m_flResetAttack;
};




//-----------------------------------------------------------------------------
// CWeaponZMFlashlight
//-----------------------------------------------------------------------------

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponZMFlashlight, DT_WeaponZMFlashlight )

BEGIN_NETWORK_TABLE( CWeaponZMFlashlight, DT_WeaponZMFlashlight )
#ifdef CLIENT_DLL

RecvPropBool( RECVINFO( m_bMeleeHit ) ),
RecvPropBool( RECVINFO( m_bSwinging ) ),
RecvPropBool( RECVINFO( m_bFlashlightCreate ) ),
RecvPropBool( RECVINFO( m_bFlashlightDestroy ) ),

#else

SendPropBool( SENDINFO( m_bMeleeHit ) ),
SendPropBool( SENDINFO( m_bSwinging ) ),
SendPropBool( SENDINFO( m_bFlashlightCreate )),
SendPropBool( SENDINFO( m_bFlashlightDestroy)),

#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponZMFlashlight )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_zm_flashlight, CWeaponZMFlashlight );
PRECACHE_WEAPON_REGISTER( weapon_zm_flashlight );

#ifndef CLIENT_DLL


acttable_t	CWeaponZMFlashlight::m_acttable[] = 
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

IMPLEMENT_ACTTABLE(CWeaponZMFlashlight);


#endif

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CWeaponZMFlashlight::CWeaponZMFlashlight( void )
{
	m_bMeleeHit = false;
	m_bSwinging = false;
	m_bFlashlightCreate = false;
	m_bFlashlightDestroy = false;
	m_flResetAttack = 0;
#ifdef CLIENT_DLL
	m_bThirdPersonLight = false;
#endif
}


CWeaponZMFlashlight::~CWeaponZMFlashlight( void )
{
#ifdef CLIENT_DLL
	m_bThirdPersonLight = false;
#endif
}

#define LIGHT_GLOW_SPRITE "sprites/light_glow03.vmt"

void CWeaponZMFlashlight::Precache()
{
	PrecacheMaterial( LIGHT_GLOW_SPRITE );

	BaseClass::Precache();

}


//-----------------------------------------------------------------------------
// Purpose: Get the damage amount for the animation we're doing
// Input  : hitActivity - currently played activity
// Output : Damage amount
//-----------------------------------------------------------------------------
float CWeaponZMFlashlight::GetDamageForActivity( Activity hitActivity )
{
	return 10.0f; //LAWYER:  It's a pants weapon.
}

//-----------------------------------------------------------------------------
// Purpose: Add in a view kick for this weapon
//-----------------------------------------------------------------------------
void CWeaponZMFlashlight::AddViewKick( void )
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


#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Animation event handlers
//-----------------------------------------------------------------------------
void CWeaponZMFlashlight::HandleAnimEventMeleeHit( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
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

		// Fake a trace impact, so the effects work out like a player's crowbaw
		trace_t traceHit;
		UTIL_TraceLine( pOperator->Weapon_ShootPosition(), pHurt->GetAbsOrigin(), MASK_SHOT_HULL, pOperator, COLLISION_GROUP_NONE, &traceHit ); //LAWYER:  Try using this for Zombie blood decalling
		ImpactEffect( traceHit );
	}
	else
	{
		WeaponSound( SINGLE );
	}
}


//-----------------------------------------------------------------------------
// Animation event
//-----------------------------------------------------------------------------
void CWeaponZMFlashlight::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
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
//ConVar sk_crowbar_lead_time( "sk_crowbar_lead_time", "0.9" );

int CWeaponZMFlashlight::WeaponMeleeAttack1Condition( float flDot, float flDist )
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
void CWeaponZMFlashlight::Drop( const Vector &vecVelocity )
{
/*#ifndef CLIENT_DLL
	UTIL_Remove( this );
#endif*/
#ifdef CLIENT_DLL	
	if (m_pFlashlightBeam != NULL)
	{
		// Turned off the flashlight; delete it.
		m_pFlashlightBeam->TurnOff();

		delete m_pFlashlightBeam;
		m_pFlashlightBeam = NULL;
		Msg("Deleted inside holster\n");
		r_FlashlightIsOn.SetValue( false );
	}

	m_bThirdPersonLight = false;

#endif

	BaseClass::Drop( vecVelocity );
}

//-----------------------------------------------------------------------------
// Purpose: Draw weapon and start flashlighting
//-----------------------------------------------------------------------------
bool CWeaponZMFlashlight::Deploy( void )
{
	m_bSwinging = false; //init swinging check bool

#ifdef CLIENT_DLL

	// Turned on the headlight; create it.
	m_pFlashlightBeam = new CFlashlightEffect();
	Msg("Created inside deploy\n");

	m_pFlashlightBeam->TurnOn();
	r_FlashlightIsOn.SetValue( true );

	m_bThirdPersonLight = true;

#endif

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: Holster weapon and remove flashlight effect
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponZMFlashlight::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	m_bMeleeHit = false;

#ifdef CLIENT_DLL
	
	if (m_pFlashlightBeam != NULL)
	{
		// Turned off the flashlight; delete it.
		m_pFlashlightBeam->TurnOff();

		delete m_pFlashlightBeam;
		m_pFlashlightBeam = NULL;
		Msg("Deleted inside holster\n");
		r_FlashlightIsOn.SetValue( false );
	}

	m_bThirdPersonLight = false;

#endif

	return BaseClass::Holster( pSwitchingTo );
}


//-----------------------------------------------------------------------------
// Purpose: Update flashlight
//-----------------------------------------------------------------------------
void CWeaponZMFlashlight::ItemPostFrame()
{

#ifdef CLIENT_DLL

	//occasionally this triggers even though we're not swinging, especially when the LMOUSE is held instead of pressed.
	//seems to happen less when refire delay is higher
	if ( m_bMeleeHit ) 
	{
		//Msg("MeleeHit clientside processing...\n");
		//do the trace stuff here so we can pass it to the Hit() function
		trace_t traceHit;

		// Try a ray
		C_BasePlayer *pOwner = C_BasePlayer::GetLocalPlayer();
		if ( !pOwner )
			return;

		Vector swingStart = pOwner->Weapon_ShootPosition( );
		Vector forward;

		pOwner->EyeVectors( &forward, NULL, NULL );

		Vector swingEnd = swingStart + forward * GetRange();
		UTIL_TraceLine( swingStart, swingEnd, MASK_SHOT_HULL, pOwner, COLLISION_GROUP_NONE, &traceHit );
		// Apply an impact effect
		ImpactEffect( traceHit );
		//Msg("Setting MeleeHit to false\n");
		m_bMeleeHit = false;
	}
			
#endif

#ifndef CLIENT_DLL
	if ( m_flResetAttack > 0 && m_flResetAttack < gpGlobals->curtime && m_bSwinging )
	{
		//Msg("Not swinging nor hitting shit now\n");
		m_bMeleeHit = false;
		m_bSwinging = false;
	}
#endif
	
	BaseClass::ItemPostFrame();
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Receive AE_ZM_MELEEHIT and handle it
//-----------------------------------------------------------------------------
void CWeaponZMFlashlight::HandleAnimEvent( animevent_t *pEvent )
{
	if( pEvent->event == AE_ZM_MELEEHIT) 
	{
		//melee hit event thrown by model detected
		//ought to put this in a seperate function sometime		

		//do the trace stuff here so we can pass it to the Hit() function
		trace_t traceHit;

		// Try a ray
		CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
		if ( !pOwner )
			return;

		Vector swingStart = pOwner->Weapon_ShootPosition( );
		Vector forward;

		pOwner->EyeVectors( &forward, NULL, NULL );

		Vector swingEnd = swingStart + forward * GetRange();
		UTIL_TraceLine( swingStart, swingEnd, MASK_SHOT_HULL, pOwner, COLLISION_GROUP_NONE, &traceHit );

		//do we need this? I added it but it doesn't seem to do much.

		// Like bullets, bludgeon traces have to trace against triggers.
		CTakeDamageInfo triggerInfo( GetOwner(), GetOwner(), GetDamageForActivity( ACT_VM_HITCENTER ), DMG_CLUB );
		TraceAttackToTriggers( triggerInfo, traceHit.startpos, traceHit.endpos, vec3_origin );


		//do the damage etc
		BaseClass::Hit( traceHit, ACT_VM_HITCENTER);

		//TGB: Decals are now handled clientside through this networked var. Not ideal but it works.
		//Msg("Setting MeleeHit to true\n");
		m_bMeleeHit = true;
		
		return;
	}

	if( pEvent->event == AE_ZM_BEAMOFF)
	{
		{
			m_bFlashlightDestroy = true;
			m_bFlashlightCreate = false;
			return;
		}
	}

	if( pEvent->event == AE_ZM_BEAMON)
	{
		{
			m_bFlashlightCreate = true;
			m_bFlashlightDestroy = false;
			return;
		}
	}
	
	BaseClass::HandleAnimEvent( pEvent );
	
}
#endif


//-----------------------------------------------------------------------------
// Purpose: Deal with primary attack animations
//-----------------------------------------------------------------------------
void CWeaponZMFlashlight::PrimaryAttack()
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if (pPlayer->GetMoveType()==MOVETYPE_LADDER)
		return;

	//TGB: I'm bringing the flashlight back to basics for now
	//that means just shining a light, no smashing, and therefore no problem with beams needing to turn on/off at anim events

	/*
	//are we already swinging?
	if (!m_bSwinging)
	{
		//well now we are!
		m_bSwinging = true;
		// Send the anims
		SendWeaponAnim( ACT_VM_HITCENTER );
		//set the time we will be able to attack again
		m_flResetAttack = gpGlobals->curtime + SequenceDuration() + 0.3f;
		WeaponSound( SINGLE );

		CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
		if ( !pOwner )
			return;
		pOwner->SetAnimation( PLAYER_ATTACK1 );
	
	}
	*/

	//TGB: turning bug into feature: on/off button
	//the problem of the flashlight not shining is caused by r_FlashlightIsOn being 0 where it should be 1
	//so we make the player hit primary to turn his flashlight on or off and in doing so reset r_FlashlightIsOn
	
#ifdef CLIENT_DLL
	if ( m_flResetAttack > gpGlobals->curtime )
		return;

	//it's off
	if (r_FlashlightIsOn.GetBool() == false)
	{
		if (m_pFlashlightBeam == NULL)
		{
			// Turned on the headlight; create it.
			m_pFlashlightBeam = new CFlashlightEffect();
			DevMsg("Created inside prifire\n");
		}
		
		DevMsg("Turning ON flashlight in prifire\n");
		m_pFlashlightBeam->TurnOn();
		r_FlashlightIsOn.SetValue( true );
		m_flResetAttack = gpGlobals->curtime + 2.0f;

		m_bThirdPersonLight = true;

	}
	//it's on
	else
	{
		if (m_pFlashlightBeam != NULL)
		{
			// Turned off the flashlight; delete it.
			m_pFlashlightBeam->TurnOff();

			delete m_pFlashlightBeam;
			m_pFlashlightBeam = NULL;
			Msg("Deleted inside holster\n");
		}

		DevMsg("Turning OFF flashlight in prifire\n");
		r_FlashlightIsOn.SetValue( false );
		m_flResetAttack = gpGlobals->curtime + 2.0f;

		m_bThirdPersonLight = false;

	}

#endif

}

void CWeaponZMFlashlight::ItemHolsterFrame()
{
#ifdef CLIENT_DLL	
	if (m_pFlashlightBeam != NULL)
	{
		// Turned off the flashlight; delete it.
		m_pFlashlightBeam->TurnOff();

		delete m_pFlashlightBeam;
		m_pFlashlightBeam = NULL;
		Msg("Deleted inside holster\n");
		r_FlashlightIsOn.SetValue( false );
	}
	m_bThirdPersonLight = false;

#endif
	BaseClass::ItemHolsterFrame();

}

void CWeaponZMFlashlight::GetWeaponAttachment( int attachmentId, Vector &outVector, Vector *dir /*= NULL*/ )
{
#ifdef CLIENT_DLL
	QAngle	angles;
		CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
 
		if ( pOwner != NULL )
		{
			pOwner->GetViewModel()->GetAttachment( attachmentId, outVector, angles );
			::FormatViewModelAttachment( outVector, true );
		}
	if ( dir != NULL )
	{
		AngleVectors( angles, dir, NULL, NULL );
	}
#endif
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Update the flashlight with new angles and vectors for drawing
//-----------------------------------------------------------------------------
void CWeaponZMFlashlight::UpdateFlashlight( void )
{	
	if(r_FlashlightIsOn.GetBool())
	{
		// Setup the attachment
		QAngle angles;
		Vector beamOrigin, vecForward, vecRight, vecUp;

		int attachment = LookupAttachment( "light" );

		// Format for first-person
		if ( IsCarriedByLocalPlayer() )
		{
			CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

			if ( pOwner != NULL )
			{
				pOwner->GetViewModel()->GetAttachment( attachment, beamOrigin, angles );
				FormatViewModelAttachment( attachment, beamOrigin, angles );
				AngleVectors( angles, &vecUp, &vecRight, &vecForward ); 
			}
		}
		//Otherwise, for the third.
		//else 
		//	DrawEffects();

		// Update the light with the new position and direction.		
		m_pFlashlightBeam->UpdateLight( beamOrigin, vecForward, vecRight, vecUp, FLASHLIGHT_DISTANCE);	
	}
	//else if (!IsCarriedByLocalPlayer())
	//	DrawEffects(); //TGB: always draw other people's lights for now
	//TGB: actually, there's stuff in c_hl2mp_player for this, it'd be better to use that
}
 
//-----------------------------------------------------------------------------
// Purpose: Update the flashlight every frame. This is the only non-lagged 
//			method for this task.
//-----------------------------------------------------------------------------
void CWeaponZMFlashlight::Simulate()
{
	if(m_pFlashlightBeam != NULL)
		UpdateFlashlight();
}

void CWeaponZMFlashlight::OnDataChanged( DataUpdateType_t updateType )
{
	if(updateType == DATA_UPDATE_DATATABLE_CHANGED)
	{
		if(m_pFlashlightBeam)
		{
			if( m_bFlashlightDestroy )
			{
				m_pFlashlightBeam->TurnOff();
				r_FlashlightIsOn.SetValue( false );

			}
			if( m_bFlashlightCreate )
			{
				m_pFlashlightBeam->TurnOn();
				r_FlashlightIsOn.SetValue( true );
			}
		}
	}
	
	BaseClass::OnDataChanged( updateType );

}
//TGB: uncommented for testing
void CWeaponZMFlashlight::DrawEffects()
{
	//DevMsg("Drawing dlight\n");
	Vector	vecAttachment, vecDir;
	QAngle vecAngle;

	const int attachment = LookupAttachment( "light" );

	//TGB: we're in third person, so no viewmodel stuff
	GetAttachment( attachment, vecAttachment, vecAngle );
	AngleVectors ( vecAngle, &vecDir );

	//GetWeaponAttachment( attachment, vecAttachment, &vecDir );
 
	trace_t tr;
	UTIL_TraceLine( vecAttachment, vecAttachment + ( vecDir * 1024 ), MASK_ALL, GetOwner(), COLLISION_GROUP_NONE, &tr );
 
	dlight_t *dl = effects->CL_AllocDlight( LIGHT_INDEX_TE_DYNAMIC );
	VectorCopy (tr.endpos-(vecDir*5), dl->origin);
	dl->decay = 50;

	dl->radius = 200;
	dl->color.r = 220;
	dl->color.g = 220;
	dl->color.b = 200;
	dl->die = gpGlobals->curtime + 0.15f; 
}

#endif