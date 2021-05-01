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
//	#include "grenade_ar2.h"
	#include "hl2mp_player.h"
//	#include "basegrenade_shared.h"
#endif

#include "weapon_hl2mpbase.h"
#include "weapon_hl2mpbase_machinegun.h"

#ifdef CLIENT_DLL
#define CWeaponZMMac10 C_WeaponZMMac10
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//#define SMG1_GRENADE_DAMAGE 100.0f
//#define SMG1_GRENADE_RADIUS 250.0f

class CWeaponZMMac10 : public CHL2MPMachineGun
{
public:
	DECLARE_CLASS( CWeaponZMMac10, CHL2MPMachineGun );

	CWeaponZMMac10();

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	
	void	Precache( void );
	void	AddViewKick( void );
	void	PrimaryAttack( void );

	int		GetMinBurst() { return 2; }
	int		GetMaxBurst() { return 5; }

	virtual void Equip( CBaseCombatCharacter *pOwner );
	bool	Reload( void );

	float	GetFireRate( void ) { return 0.066f; }	//TGB: was 0.050
	Activity	GetPrimaryAttackActivity( void );

	virtual const Vector& GetBulletSpread( void )
	{
		static const Vector cone = VECTOR_CONE_6DEGREES;
		return cone;
	}

	virtual int		GetWeaponTypeNumber( void ) { return 4; } //LAWYER:  In the main weapons group

	const WeaponProficiencyInfo_t *GetProficiencyValues();

#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();
#endif

protected:

	Vector	m_vecTossVelocity;
	float	m_flNextGrenadeCheck;
	
private:
	CWeaponZMMac10( const CWeaponZMMac10 & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponZMMac10, DT_WeaponZMMac10 )

BEGIN_NETWORK_TABLE( CWeaponZMMac10, DT_WeaponZMMac10 )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponZMMac10 )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_zm_mac10, CWeaponZMMac10 );
PRECACHE_WEAPON_REGISTER(weapon_zm_mac10);

#ifndef CLIENT_DLL
acttable_t	CWeaponZMMac10::m_acttable[] = 
{
	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_PISTOL,					false },
	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_PISTOL,						false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_PISTOL,				false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_PISTOL,				false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_SMG1,	false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_PISTOL,			false },
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_PISTOL,					false },
	{ ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_PISTOL,					false },
};

IMPLEMENT_ACTTABLE(CWeaponZMMac10);
#endif

//=========================================================
CWeaponZMMac10::CWeaponZMMac10( )
{
	m_fMinRange1		= 0;// No minimum range. 
	m_fMaxRange1		= 1400;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponZMMac10::Precache( void )
{
//#ifndef CLIENT_DLL
//	UTIL_PrecacheOther("grenade_ar2");
//#endif

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Give this weapon longer range when wielded by an ally NPC.
//-----------------------------------------------------------------------------
void CWeaponZMMac10::Equip( CBaseCombatCharacter *pOwner )
{
	m_fMaxRange1 = 1400;

	BaseClass::Equip( pOwner );
}
void CWeaponZMMac10::PrimaryAttack( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if ( pPlayer == NULL )
		return;

	if (pPlayer->GetMoveType()==MOVETYPE_LADDER)
		return;
	
	BaseClass::PrimaryAttack();

	
/*	#ifdef CLIENT_DLL
		QAngle	vangles;
		
		engine->GetViewAngles(vangles);
		vangles.x = vangles.x + .1;
		engine->SetViewAngles( vangles );
	#endif*/

}
//-----------------------------------------------------------------------------
// Purpose: 
// Output : Activity
//-----------------------------------------------------------------------------
Activity CWeaponZMMac10::GetPrimaryAttackActivity( void )
{
	if ( m_nShotsFired < 2 )
		return ACT_VM_PRIMARYATTACK;

	if ( m_nShotsFired < 3 )
		return ACT_VM_RECOIL1;
	
	if ( m_nShotsFired < 4 )
		return ACT_VM_RECOIL2;

	return ACT_VM_RECOIL3;
}



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CWeaponZMMac10::Reload( void )
{
	bool fRet;
	float fCacheTime = m_flNextSecondaryAttack;

	fRet = DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD );
	if ( fRet )
	{
		// Undo whatever the reload process has done to our secondary
		// attack timer. We allow you to interrupt reloading to fire
		// a grenade.
		m_flNextSecondaryAttack = GetOwner()->m_flNextAttack = fCacheTime;

		WeaponSound( RELOAD );
	}

	return fRet;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponZMMac10::AddViewKick( void )
{
	#define	EASY_DAMPEN			0.5f
	#define	MAX_VERTICAL_KICK	2.0f	//Degrees
	#define	SLIDE_LIMIT			1.0f	//Seconds
	
	//Get the view kick
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if ( pPlayer == NULL )
		return;

	DoMachineGunKick( pPlayer, EASY_DAMPEN, MAX_VERTICAL_KICK, m_fFireDuration, SLIDE_LIMIT );

	/*
	CBasePlayer *pPlayer  = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

	int iSeed = CBaseEntity::GetPredictionRandomSeed() & 255;
	RandomSeed( iSeed );
	
	QAngle	viewPunch;

	viewPunch.x = random->RandomFloat( -0.25f, -0.5f ); //TGB: more kick, was: -0.25f, 0.5f
	viewPunch.y = random->RandomFloat( -0.6f, 0.6f ); //was: -0.6, .6
	viewPunch.z = 0.0f;

	//Add it to the view punch
	pPlayer->ViewPunch( viewPunch );
*/
	
}


//-----------------------------------------------------------------------------
const WeaponProficiencyInfo_t *CWeaponZMMac10::GetProficiencyValues()
{
	static WeaponProficiencyInfo_t proficiencyTable[] =
	{
		{ 7.0,		0.75	},
		{ 5.00,		0.75	},
		{ 10.0/3.0, 0.75	},
		{ 5.0/3.0,	0.75	},
		{ 1.00,		1.0		},
	};

	COMPILE_TIME_ASSERT( ARRAYSIZE(proficiencyTable) == WEAPON_PROFICIENCY_PERFECT + 1);

	return proficiencyTable;
}
