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
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"
#include "eventlist.h"

#ifdef CLIENT_DLL
	#include "c_hl2mp_player.h"
	#include "c_user_message_register.h"
#else
	#include "hl2mp_player.h"
#endif

#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "usermessages.h"


#ifdef CLIENT_DLL
#define CWeaponZMRevolver C_WeaponZMRevolver
#endif

//-----------------------------------------------------------------------------
// CWeaponZMRevolver
//-----------------------------------------------------------------------------

class CWeaponZMRevolver : public CBaseHL2MPCombatWeapon
{
	DECLARE_CLASS( CWeaponZMRevolver, CBaseHL2MPCombatWeapon );
public:

	CWeaponZMRevolver( void );

	void	PrimaryAttack( void );
	void	SecondaryAttack( void );

	//void	ItemPreFrame( void ) {FakeShootAction(); BaseClass::ItemPreFrame();}
	//void	ItemPostFrame( void );
	//void	ItemBusyFrame( void ){FakeShootAction(); BaseClass::ItemBusyFrame();}
	//void	WeaponIdle( void ){FakeShootAction(); BaseClass::WeaponIdle();}

	bool	Holster( CBaseCombatWeapon *pSwitchingTo );


	void	Shoot( bool accurate ); //TGB: used by fakeshootaction and handleanimevent

	//void	FakeShootAction( void );
	//CNetworkVar( bool, m_bFakeFireTrace );

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
#ifndef CLIENT_DLL
	void	Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	DECLARE_ACTTABLE();
#else
	bool	OnFireEvent( C_BaseViewModel *pViewModel, const Vector& origin, const QAngle& angles, int event, const char *options );
#endif


	virtual int				GetWeaponTypeNumber( void ) { return 2; } //LAWYER:  In the Pistols group

private:
	bool	m_bAccurateFire;

	CWeaponZMRevolver( const CWeaponZMRevolver & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponZMRevolver, DT_WeaponZMRevolver )

BEGIN_NETWORK_TABLE( CWeaponZMRevolver, DT_WeaponZMRevolver )
/*
#ifdef CLIENT_DLL

RecvPropBool( RECVINFO( m_bFakeFireTrace ) ),


#else

SendPropBool( SENDINFO( m_bFakeFireTrace ) ),


#endif
*/
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponZMRevolver )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_zm_revolver, CWeaponZMRevolver );
PRECACHE_WEAPON_REGISTER( weapon_zm_revolver );


#ifndef CLIENT_DLL
acttable_t CWeaponZMRevolver::m_acttable[] = 
{
	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_PISTOL,					false },
	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_PISTOL,					false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_PISTOL,			false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_PISTOL,			false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_PISTOL,	false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_PISTOL,		false },
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_PISTOL,					false },
	{ ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_PISTOL,				false },
};



IMPLEMENT_ACTTABLE( CWeaponZMRevolver );

#endif

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponZMRevolver::CWeaponZMRevolver( void )
{
	//m_bFakeFireTrace = false;
	m_bReloadsSingly	= false;
	m_bFiresUnderwater	= false;
	m_bAccurateFire = false;

}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponZMRevolver::PrimaryAttack( void )
{
	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if ( !pPlayer )
	{
		return;
	}

	// No attacking while on a ladder
	if( pPlayer->GetMoveType() == MOVETYPE_LADDER)
		return;

	if ( m_iClip1 <= 0 )
	{
		if ( !m_bFireOnEmpty )
		{
			Reload();
		}
		else
		{
			WeaponSound( EMPTY );
			m_flNextPrimaryAttack = gpGlobals->curtime + 1.0f; //was: 0.15
			m_flNextSecondaryAttack = gpGlobals->curtime + 1.0f;
		}

		return;
	}
	m_bAccurateFire = true;
	SendWeaponAnim( ACT_VM_PRIMARYATTACK );
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_flNextPrimaryAttack = gpGlobals->curtime + 1.0f;
	m_flNextSecondaryAttack = gpGlobals->curtime + 1.0f;

}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponZMRevolver::SecondaryAttack( void )
{
	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if ( !pPlayer )
	{
		return;
	}


	// No attacking while on a ladder
	if( pPlayer->GetMoveType() == MOVETYPE_LADDER)
		return;

	if ( m_iClip1 <= 0 )
	{
		if ( !m_bFireOnEmpty )
		{
			Reload();
		}
		else
		{
			//TGB: 0000436 revolver empty sound can be spammed at an SMG firerate, curtime term was missing
			//	however, even with the time fixed it's not necessary to play this here
			//WeaponSound( EMPTY );
			m_flNextPrimaryAttack = gpGlobals->curtime + 1.0f; //was: 0.15
			m_flNextSecondaryAttack = gpGlobals->curtime + 1.0f;
		}

		return;
	}
/*
	WeaponSound( SINGLE );
	pPlayer->DoMuzzleFlash();
*/
	m_bAccurateFire = false;
	SendWeaponAnim( ACT_VM_SECONDARYATTACK );
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.4f;
	m_flNextSecondaryAttack = gpGlobals->curtime + 0.3f;
/*
	m_iClip1--;

	Vector vecSrc		= pPlayer->Weapon_ShootPosition();
	Vector vecAiming	= pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );	

	FireBulletsInfo_t info( 1, vecSrc, vecAiming, vec3_origin, MAX_TRACE_LENGTH, m_iPrimaryAmmoType );
	info.m_pAttacker = pPlayer;

	// Fire the bullets, and force the first shot to be perfectly accuracy
	pPlayer->FireBullets( info );

	//Disorient the player
	QAngle angles = pPlayer->GetLocalAngles();

	angles.x += random->RandomInt( -1, 1 );
	angles.y += random->RandomInt( -1, 1 );
	angles.z = 0;

#ifndef CLIENT_DLL
	pPlayer->SnapEyeAngles( angles );
#endif

	pPlayer->ViewPunch( QAngle( -8, random->RandomFloat( -2, 2 ), 0 ) );

	if ( !m_iClip1 && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 ); 
	}*/
}

//--------------------------------------------------------------
// TGB: moved code from HandleAnimEvent and FakeShootAction into this, easier than duplicating changes all the time
//--------------------------------------------------------------
void CWeaponZMRevolver::Shoot(bool accurate)
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if ( !pPlayer )
		return;

	//LAWYER: Revolver stuff is defined by the animation event
	//WeaponSound( SINGLE );
	pPlayer->DoMuzzleFlash();

	m_iClip1--;

	//TGB: replaced cone difference with eyeangle punch

	FireBulletsInfo_t info;
	info.m_pAttacker = pPlayer;

	//TGB bloodfix? decals working again now anyway
	info.m_iDamage = GetPlayerDamage();

	info.m_iShots = 1;
	info.m_vecSrc = pPlayer->Weapon_ShootPosition();
	info.m_vecDirShooting = pPlayer->GetAutoaimVector( AUTOAIM_2DEGREES );
	info.m_vecSpread = VECTOR_CONE_1DEGREES;
	info.m_flDistance = MAX_TRACE_LENGTH;
	info.m_iAmmoType = m_iPrimaryAmmoType;
	info.m_iTracerFreq = 1; //always trace, don't fire that many bullets

	// Fire the bullets
	pPlayer->FireBullets( info );

	//TGB: this is really a rather nice effect, but because of the hacky nature of the client's shooting action it gets screwy
	
#ifndef CLIENT_DLL
	if (accurate == false)
	{
		//Disorient the player
		QAngle angles = pPlayer->GetLocalAngles();

		angles.x += random->RandomInt( -2, 2 );
		angles.y += random->RandomInt( 2, 4 );
		angles.z = 0;
		pPlayer->SnapEyeAngles( angles );
	}
#endif
//
	if (accurate)
		pPlayer->ViewPunch( QAngle( -8, random->RandomFloat( -1, 1 ), 0 ) );
	else
		pPlayer->ViewPunch( QAngle( random->RandomFloat( -8, 2 ), random->RandomFloat( -2, 2 ), 0 ) );


	if ( !m_iClip1 && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 ); 
	}
}

//-----------------------------------------------------------------------------
// Purpose: Actually fire a bullet
//-----------------------------------------------------------------------------
#ifndef CLIENT_DLL
void CWeaponZMRevolver::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	
	if( pEvent->event == AE_ZM_REVOLVERSHOOT ) 
	{
		Shoot(m_bAccurateFire);

		//DevMsg("Revolver server event caught\n");

		//third person sound
		WeaponSound( SINGLE );
		//third person anim
		//UNDONE: works, but will fire revolvershoot animation events, which can't be caught
		/*CBasePlayer *pPlayer = ToBasePlayer(pOperator);
		if (pPlayer)
		{
			pPlayer->SetAnimation( PLAYER_ATTACK1 );
		}*/

		return;
	}

	BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
	
}
#else
//--------------------------------------------------------------
// TGB: turns out we can see anim events on the client as well, d'oh, so lets use them
//--------------------------------------------------------------
bool CWeaponZMRevolver::OnFireEvent( C_BaseViewModel *pViewModel, const Vector& origin, const QAngle& angles, int event, const char *options )
{
	if (event == AE_ZM_REVOLVERSHOOT)
	{
		//DevMsg("Heard MeleeHit on client!\n");
		Shoot(m_bAccurateFire);

		return true;
	}
	return false;
}
#endif

/*TGB: handled with usermessage now to call Shoot at the right moment
//-----------------------------------------------------------------------------
// Purpose: Fake the trace
//-----------------------------------------------------------------------------
void CWeaponZMRevolver::FakeShootAction()
{

#ifdef CLIENT_DLL

	if ( m_bFakeFireTrace ) 
	{
		//Shoot(m_bAccurateFire);
		m_bFakeFireTrace = false;
	}
		
#endif

#ifndef CLIENT_DLL

	if (m_bFakeFireTrace)
	{
		m_bFakeFireTrace = false;
	}
#endif

}
*/
//-----------------------------------------------------------------------------
// Purpose: Holster weapon
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponZMRevolver::Holster( CBaseCombatWeapon *pSwitchingTo )
{
//	m_bFakeFireTrace = false;

	return BaseClass::Holster( pSwitchingTo );
}


//TGB: let's try using a usermessage for the revolver client effect notification instead of unreliable networked vars
//the handy thing about this message is that we know we only receive it once after the server fires the revolver and sends the msg
//this is really pretty easy to do by the way: http://developer.valvesoftware.com/wiki/Networking_Events_%26_Messages#User_Messages
/*TGB UNDONE: found a way to nab anim events on the client
#ifdef CLIENT_DLL
void __MsgFunc_RevolverShot( bf_read &msg )
{
	int accurate = msg.ReadOneBit();
	DevMsg("UserMessage RevolverShot called. Bit Value: %i\n", accurate);

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if (pPlayer)
	{
		CWeaponZMRevolver *revolver = dynamic_cast<CWeaponZMRevolver*>(pPlayer->GetActiveWeapon());
		if (revolver)
		{
			DevMsg("Revolvershot: shooting...\n");
			if (accurate == 1)
				revolver->Shoot(true);
			else
				revolver->Shoot(false);
		}
	}
	else
	{
		DevMsg("RevolverShot: no player found\n");
	}
}

USER_MESSAGE_REGISTER( RevolverShot );

#endif
*/