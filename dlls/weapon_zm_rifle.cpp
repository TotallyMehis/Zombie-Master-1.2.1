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


#ifdef CLIENT_DLL
	#include "c_hl2mp_player.h"
#else
	#include "hl2mp_player.h"
	#include "hl2_player.h"
#endif

#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "usermessages.h"

#ifdef CLIENT_DLL
#define CWeaponZMRifle C_WeaponZMRifle
#endif

extern ConVar sk_auto_reload_time;
//extern ConVar sk_plr_num_rifle_pellets;

class CWeaponZMRifle : public CBaseHL2MPCombatWeapon
{
public:
	DECLARE_CLASS( CWeaponZMRifle, CBaseHL2MPCombatWeapon );

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

private:
	CNetworkVar( bool,	m_bNeedPump );		// When emptied completely
	CNetworkVar( bool,	m_bDelayedFire1 );	// Fire primary when finished reloading
	CNetworkVar( bool,	m_bDelayedFire2 );	// Fire secondary when finished reloading
	CNetworkVar( bool,	m_bInZoom );		// QCK EDIT- tells the client if we're zoomed in, important


	CWeaponZMRifle( const CWeaponZMRifle & );
	void CheckZoomToggle( void );										// QCK EDIT
	void ToggleZoom( void );											// QCK EDIT

public:

	virtual const Vector& GetBulletSpread( void )
	{
		//static Vector cone = VECTOR_CONE_1DEGREES;
		static Vector cone = Vector( 0, 0, 0); //TGB: accuracy improvement, hopefully
		return cone;
	}

	virtual int				GetMinBurst() { return 1; }
	virtual int				GetMaxBurst() { return 3; }
	
	void					Drop( const Vector &vecVelocity );



	//TGB: override eye position to fix pickup problems
	//TGB: no longer truly necessary for the pickup stuff, but can't hurt either
	virtual Vector	EyePosition( void ) { return WorldSpaceCenter(); }

	bool StartReload( void );
	bool Reload( void );
	void FillClip( void );
	void FinishReload( void );
	void CheckHolsterReload( void );
	void Pump( void );
	bool Holster( CBaseCombatWeapon *pSwitchingTo = NULL );				// QCK EDIT
	bool Deploy( void );												// TGB zoomhack
	void ItemHolsterFrame( void ); 
	void ItemPostFrame( void );
	void ItemBusyFrame( void );											// QCK EDIT - used for a check
	void PrimaryAttack( void );
	void SecondaryAttack( void );										// QCK EDIT - secondary attack will take care of zooming for us
	void DryFire( void );
//#ifdef CLIENT_DLL
//	void ZoomBlurMessage( bool shouldBlur );
//#endif
	virtual float GetFireRate( void ) { return 0.75; };
	
	virtual int				GetWeaponTypeNumber( void ) { return 4; } //LAWYER:  In the main weapons group


#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();
#endif

	CWeaponZMRifle(void);

#ifdef CLIENT_DLL
	CBasePlayer *m_pPlayer; // QCK EDIT
#endif

};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponZMRifle, DT_WeaponZMRifle )

BEGIN_NETWORK_TABLE( CWeaponZMRifle, DT_WeaponZMRifle )
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bNeedPump ) ),
	RecvPropBool( RECVINFO( m_bDelayedFire1 ) ),
	RecvPropBool( RECVINFO( m_bDelayedFire2 ) ),
	RecvPropBool( RECVINFO( m_bInZoom ) ),	//QCK EDIT
#else
	SendPropBool( SENDINFO( m_bNeedPump ) ),
	SendPropBool( SENDINFO( m_bDelayedFire1 ) ),
	SendPropBool( SENDINFO( m_bDelayedFire2 ) ),
	SendPropBool( SENDINFO( m_bInZoom ) ),	//QCK EDIT
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponZMRifle )
	DEFINE_PRED_FIELD( m_bNeedPump, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bDelayedFire1, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bDelayedFire2, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bInZoom, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ), //QCK EDIT
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_zm_rifle, CWeaponZMRifle );
PRECACHE_WEAPON_REGISTER(weapon_zm_rifle);

#ifndef CLIENT_DLL
acttable_t	CWeaponZMRifle::m_acttable[] = 
{
	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_AR2,					false },
	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_AR2,					false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_AR2,			false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_AR2,			false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_AR2,	false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_AR2,		false },
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_AR2,					false },
	{ ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_AR2,				false },
};

IMPLEMENT_ACTTABLE(CWeaponZMRifle);

#endif


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponZMRifle::CWeaponZMRifle( void )
{
	m_bReloadsSingly = true;

	m_bNeedPump		= false;
	m_bDelayedFire1 = false;
	m_bDelayedFire2 = false;

	m_bInZoom		= false;

	m_fMinRange1		= 0.0;
	m_fMaxRange1		= 2000;
	m_fMinRange2		= 0.0;
	m_fMaxRange2		= 2000;
}


//-----------------------------------------------------------------------------
// Purpose: Override so only reload one shell at a time
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CWeaponZMRifle::StartReload( void )
{
	CBaseCombatCharacter *pOwner  = GetOwner();
	
	if ( pOwner == NULL )
		return false;

	if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
		return false;

	if (m_iClip1 >= GetMaxClip1())
		return false;

	if (m_bNeedPump)
		return false;
		
	//TGB: prevent even starting a reload
	if (pOwner->GetMoveType()==MOVETYPE_LADDER)
		return false;

	/* TGB: removed to fix desync when picking up ammo with an empty weapon, bug 0000381
	// If shotgun totally emptied then a pump animation is needed

	//NOTENOTE: This is kinda lame because the player doesn't get strong feedback on when the reload has finished,
	//			without the pump.  Technically, it's incorrect, but it's good for feedback...

	if (m_iClip1 <= 0)
	{
	m_bNeedPump = true;
	}*/

	int j = min(1, pOwner->GetAmmoCount(m_iPrimaryAmmoType));

	if (j <= 0)
		return false;

	SendWeaponAnim( ACT_RIFLE_RELOAD_START );

	// Make rifle shell visible
	SetBodygroup(1,0);

	const float delay = SequenceDuration()/* + GetFireRate()*/; //TGB: extra delay to make constant reload/fire/reload unfeasible
	pOwner->m_flNextAttack = gpGlobals->curtime + delay;
	m_flNextPrimaryAttack = gpGlobals->curtime + delay;

	m_bInReload = true;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Override so only reload one shell at a time
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CWeaponZMRifle::Reload( void )
{

#ifdef CLIENT_DLL
	if ( !HasPrimaryAmmo() )
	{
		//ZoomBlurMessage( false ); //qck: New method for zoom blur
		return false;
	}
#endif

	
	// Check that StartReload was called first
	if (!m_bInReload)
	{
		Warning("ERROR: Rifle Reload called incorrectly!\n");
	}

	CBaseCombatCharacter *pOwner  = GetOwner();
	
	if ( pOwner == NULL )
		return false;

	if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
		return false;

	if (m_iClip1 >= GetMaxClip1())
		return false;

	int j = min(1, pOwner->GetAmmoCount(m_iPrimaryAmmoType));

	if (j <= 0)
		return false;

	FillClip();
	// Play reload on different channel as otherwise steals channel away from fire sound
	WeaponSound(RELOAD);
	SendWeaponAnim( ACT_VM_RELOAD );

	const float delay = SequenceDuration()/* + GetFireRate()*/; //TGB: extra delay to make constant reload/fire/reload unfeasible
	pOwner->m_flNextAttack = gpGlobals->curtime + delay;
	m_flNextPrimaryAttack = gpGlobals->curtime + delay;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Play finish reload anim and fill clip
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CWeaponZMRifle::FinishReload( void )
{
	// Make rifle shell invisible
	SetBodygroup(1,1);

	CBaseCombatCharacter *pOwner  = GetOwner();
	
	if ( pOwner == NULL )
		return;

	m_bInReload = false;

	// Finish reload animation
	SendWeaponAnim( ACT_RIFLE_RELOAD_FINISH );

	const float delay = SequenceDuration()/* + GetFireRate()*/; //TGB: extra delay to make constant reload/fire/reload unfeasible
	pOwner->m_flNextAttack = gpGlobals->curtime + delay;
	m_flNextPrimaryAttack = gpGlobals->curtime + delay;
}

//-----------------------------------------------------------------------------
// Purpose: Play finish reload anim and fill clip
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CWeaponZMRifle::FillClip( void )
{
	CBaseCombatCharacter *pOwner  = GetOwner();
	
	if ( pOwner == NULL )
		return;

	// Add them to the clip
	if ( pOwner->GetAmmoCount( m_iPrimaryAmmoType ) > 0 )
	{
		if ( Clip1() < GetMaxClip1() )
		{
			m_iClip1++;
			pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Play weapon pump anim
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CWeaponZMRifle::Pump( void )
{
	CBaseCombatCharacter *pOwner  = GetOwner();

	if ( pOwner == NULL )
		return;

	//TGB: this looks meh because the shader doesn't turn off until after the levering
/*
#ifndef CLIENT_DLL
	//TGB: unzoom while levering
	if ( m_bInZoom )
	{
		CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
		if ( pPlayer->SetFOV( this, 0, 0.2f ) )
		{
			m_bInZoom = false;
		}
	}
#endif
*/

	m_bNeedPump = false;
	
	WeaponSound( SPECIAL1 );

	// Finish reload animation
	SendWeaponAnim( ACT_RIFLE_LEVER );

	pOwner->m_flNextAttack	= gpGlobals->curtime + SequenceDuration();
	m_flNextPrimaryAttack	= gpGlobals->curtime + SequenceDuration();
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CWeaponZMRifle::DryFire( void )
{
	WeaponSound(EMPTY);
	SendWeaponAnim( ACT_VM_DRYFIRE );
	
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();

}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CWeaponZMRifle::PrimaryAttack( void )
{
	if (m_bNeedPump)
		return;

	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if (pPlayer->GetMoveType()==MOVETYPE_LADDER)
		return;

	if (!pPlayer)
	{
		return;
	}

	// MUST call sound before removing a round from the clip of a CMachineGun
	WeaponSound(SINGLE);

	pPlayer->DoMuzzleFlash();

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );

	// Don't fire again until fire animation has completed
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
	m_iClip1 -= 1;

	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector	vecSrc		= pPlayer->Weapon_ShootPosition( );
	Vector	vecAiming	= pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );	

	FireBulletsInfo_t info( 1, vecSrc, vecAiming, GetBulletSpread(), MAX_TRACE_LENGTH, m_iPrimaryAmmoType );
	info.m_pAttacker = pPlayer;
	info.m_iDamage = GetPlayerDamage();

	// Fire the bullets, and force the first shot to be perfectly accuracy
	pPlayer->FireBullets( info );
	
	pPlayer->ViewPunch( QAngle( random->RandomFloat( -10, -4 ), random->RandomFloat( -2, 2 ), 0 ) );

	if (!m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0); 
	}

	if( m_iClip1 )
	{
		// pump so long as some rounds are left.
		m_bNeedPump = true;
	}
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// QCK EDIT- turn the secondary attack on
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void CWeaponZMRifle::SecondaryAttack( void )
{
	//NOTENOTE: The zooming is handled by the post/busy frames

	

}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// QCK EDIT- Allows us to toggle the zoom while reloading 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void CWeaponZMRifle::ItemBusyFrame( void )
{
	CheckZoomToggle();
}

//-----------------------------------------------------------------------------
// Purpose: Override so rifle can do mulitple reloads in a row
//-----------------------------------------------------------------------------
void CWeaponZMRifle::ItemPostFrame( void )
{
	// Allow zoom toggling -QCK EDIT
	CheckZoomToggle();
	
//#ifdef CLIENT_DLL
//	if( m_bInZoom )
//		ZoomBlurMessage( true );
//	else if( !m_bInZoom )
//		ZoomBlurMessage( false );
//#endif



	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if (!pOwner)
	{
		return;
	}

	if (m_bInReload)
	{
		// If I'm primary firing and have one round stop reloading and fire
		if ((pOwner->m_nButtons & IN_ATTACK ) && (m_iClip1 >=1))
		{
			m_bInReload		= false;
			//TGB: stop exploit + allow cancel reload w/o fire
			//m_bNeedPump		= false;
			//m_bDelayedFire1 = true;

			m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
		}
		// If I'm secondary firing and have one round stop reloading and fire
		//else if ((pOwner->m_nButtons & IN_ATTACK2 ) && (m_iClip1 >=2))
		//{
		//	m_bInReload		= false;
		//	m_bNeedPump		= false;
		//	m_bDelayedFire2 = true;
		//}
		// If I'm on a ladder and I'm trying to reload, stop
		else if (pOwner->GetMoveType()==MOVETYPE_LADDER)
		{
			m_bInReload		= false;
		}
		else if (m_flNextPrimaryAttack <= gpGlobals->curtime)
		{
			// If out of ammo end reload
			if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) <=0)
			{
				FinishReload();
				return;
			}
			// If clip not full reload again
			if (m_iClip1 < GetMaxClip1())
			{
				Reload();
				return;
			}
			// Clip full, stop reloading
			else
			{
				FinishReload();
				return;
			}
		}
	}
	else
	{			
		// Make rifle shell invisible
		SetBodygroup(1,1);
	}

	if ((m_bNeedPump) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
	{
		Pump();
		return;
	}
	
	if ( (m_bDelayedFire1 || pOwner->m_nButtons & IN_ATTACK) && m_flNextPrimaryAttack <= gpGlobals->curtime)
	{
		m_bDelayedFire1 = false;
		if ( (m_iClip1 <= 0 && UsesClipsForAmmo1()) || ( !UsesClipsForAmmo1() && !pOwner->GetAmmoCount(m_iPrimaryAmmoType) ) )
		{
			if (!pOwner->GetAmmoCount(m_iPrimaryAmmoType))
			{
				DryFire();
			}
			else
			{
				StartReload();
			}
		}
		// Fire underwater?
		else if (pOwner->GetWaterLevel() == 3 && m_bFiresUnderwater == false)
		{
			WeaponSound(EMPTY);
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
			return;
		}
		else
		{
			// If the firing button was just pressed, reset the firing time
			CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
			if ( pPlayer && pPlayer->m_afButtonPressed & IN_ATTACK )
			{
				 m_flNextPrimaryAttack = gpGlobals->curtime;
			}
			PrimaryAttack();
		}
	}

	if ( pOwner->m_nButtons & IN_RELOAD && UsesClipsForAmmo1() && !m_bInReload ) 
	{
		// reload when reload is pressed, or if no buttons are down and weapon is empty.
		StartReload();
	}
	else 
	{
		// no fire buttons down
		m_bFireOnEmpty = false;

		if ( !HasAnyAmmo() && m_flNextPrimaryAttack < gpGlobals->curtime ) 
		{
			/*TGB: no autoswitch
			// weapon isn't useable, switch.
			if ( !(GetWeaponFlags() & ITEM_FLAG_NOAUTOSWITCHEMPTY) && pOwner->SwitchToNextBestWeapon( this ) )
			{
				m_flNextPrimaryAttack = gpGlobals->curtime + 0.3;
				return;
			}
			*/
		}
		else
		{
			// weapon is useable. Reload if empty and weapon has waited as long as it has to after firing
			if ( m_iClip1 <= 0 && !(GetWeaponFlags() & ITEM_FLAG_NOAUTORELOAD) && m_flNextPrimaryAttack < gpGlobals->curtime )
			{
				if (StartReload())
				{
					// if we've successfully started to reload, we're done
					return;
				}
			}
		}

		WeaponIdle( );
		return;
	}

}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponZMRifle::ItemHolsterFrame( void )
{
	// Must be player held
	if ( GetOwner() && GetOwner()->IsPlayer() == false )
		return;

	// We can't be active
	if ( GetOwner()->GetActiveWeapon() == this )
		return;

	// If it's been longer than three seconds, reload
	if ( ( gpGlobals->curtime - m_flHolsterTime ) > sk_auto_reload_time.GetFloat() )
	{
		// Reset the timer
		m_flHolsterTime = gpGlobals->curtime;
	
		if ( GetOwner() == NULL )
			return;

		if ( m_iClip1 == GetMaxClip1() )
			return;

		// Just load the clip with no animations
		int ammoFill = min( (GetMaxClip1() - m_iClip1), GetOwner()->GetAmmoCount( GetPrimaryAmmoType() ) );
		
		GetOwner()->RemoveAmmo( ammoFill, GetPrimaryAmmoType() );
		m_iClip1 += ammoFill;
	}
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// QCK EDIT- turn off the zoom when the rifle is put away
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool CWeaponZMRifle::Holster( CBaseCombatWeapon *pSwitchingTo )
{

	if( m_bInZoom)
	{
		ToggleZoom();

#ifndef CLIENT_DLL
		CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
		if(pPlayer)
			pPlayer->SetScreenEffect(AllocPooledString("none"));
#endif

//#ifdef CLIENT_DLL
//		ZoomBlurMessage(false);
//#endif
	}


	return BaseClass::Holster( pSwitchingTo );
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// QCK EDIT- toggle the zoom on or off
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void CWeaponZMRifle::ToggleZoom( void )
{
	//Msg("Zoomed\n");
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

//#ifdef CLIENT_DLL
//		if(!m_bInZoom)
//		{
//			ZoomBlurMessage( true );
//		}
//		else
//		{
//			ZoomBlurMessage( false );
//		}
//#endif

#ifndef CLIENT_DLL
	
	
	if ( m_bInZoom )
	{
	
		if ( pPlayer->SetFOV( this, 0, 0.2f ) )
		{
			m_bInZoom = false;
			pPlayer->SetScreenEffect(AllocPooledString("none"));
		}
	}
	else
	{
	
		if ( pPlayer->SetFOV( this, 35, 0.1f ) )
		{
			m_bInZoom = true;
			pPlayer->SetScreenEffect(AllocPooledString("blur"));
		}
	}
#endif

}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// QCK EDIT- Check to see if we've pressed the ATTACK2 key and call ToggleZoom
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void CWeaponZMRifle::CheckZoomToggle( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( pPlayer->GetMoveType() == MOVETYPE_LADDER)
		return;
	
	if ( pPlayer->m_afButtonPressed & IN_ATTACK2 )
	{
		ToggleZoom();
	
	}
}

//-----------------------------------------------------------------------------
// Purpose: TGB: zoomhack
//-----------------------------------------------------------------------------
bool CWeaponZMRifle::Deploy( void )
{
	#ifndef CLIENT_DLL
	//fov hack to prevent zoom viewmodel weirding
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	pPlayer->SetDefaultFOV( pPlayer->GetDefaultFOV() ); //TGBHACKHACK
	#endif

	return BaseClass::Deploy();

}

//qckles: found a better way to do this


//#ifdef CLIENT_DLL
//void CWeaponZMRifle::ZoomBlurMessage( bool shouldBlur )
//{
//	engine->ClientCmd( VarArgs("shouldblur %i", shouldBlur)); 
//}
//#endif 
//
//#ifndef CLIENT_DLL

//qck: I did this in August originally, when I had almost no knowledge of the sdk. 
//I've cleaned it up to prevent some of the problems we were having before. I plan
//to look at this again later, since I can't recall if the client/server communication
//is actually needed or if it was just the product of my inexperience. In any case,
//this method works perfectly, and comes without a performance hit. 
//void CC_Blur_View( void )
//{
//	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );
//
//	CSingleUserRecipientFilter filter ( pPlayer ); // set recipient
//	filter.MakeReliable();  // reliable transmission
//	
//	int shouldBlur = atoi(engine->Cmd_Argv(1) );
//
//	Msg("Called\n");
//
//	UserMessageBegin( filter, "ZoomBlur" ); // create message 
//		WRITE_BYTE( shouldBlur ); // fill message
//	MessageEnd(); //send message
//}
//
//static ConCommand shouldblur("shouldblur", CC_Blur_View, "Controls the status of the rifle shader");

//#endif

void CWeaponZMRifle::Drop( const Vector &vecVelocity )
{
#ifndef CLIENT_DLL
	if(m_bInZoom)
		ToggleZoom();
	CBasePlayer* pPlayer = ToBasePlayer( GetOwner() );
	if(pPlayer)
	{
		pPlayer->SetScreenEffect(AllocPooledString("none"));
	}
#endif

	BaseClass::Drop( vecVelocity );
}

