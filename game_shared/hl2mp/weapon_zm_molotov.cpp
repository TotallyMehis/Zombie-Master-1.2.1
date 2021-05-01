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
#include "eventlist.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
	#include "c_hl2mp_player.h"
	#include "c_te_effect_dispatch.h"
	#include "c_sprite.h"
	#include "model_types.h"
	#include "ClientEffectPrecacheSystem.h"
	extern void FormatViewModelAttachment( Vector &vOrigin, bool bInverse );
#else
	#include "hl2mp_player.h"
	#include "te_effect_dispatch.h"
	#include "grenade_molotov.h"
	#include "Sprite.h"
#endif

#include "weapon_ar2.h"
#include "effect_dispatch_data.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"



// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define GRENADE_TIMER	2.5f //Seconds

#define GRENADE_PAUSED_NO			0
#define GRENADE_PAUSED_PRIMARY		1
#define GRENADE_PAUSED_SECONDARY	2

#define GRENADE_RADIUS	4.0f // inches

#define GRENADE_DAMAGE_RADIUS 250.0f

#define BG_VISIBLE 1
#define BG_INVISIBLE 0

#define MOLOTOV_FIRE_SPRITE "sprites/fire_vm_grey.vmt"

#ifdef CLIENT_DLL
#define CWeaponMolotov C_WeaponMolotov
#endif

//-----------------------------------------------------------------------------
// Fragmentation grenades
//-----------------------------------------------------------------------------
class CWeaponMolotov: public CBaseHL2MPCombatWeapon
{
	DECLARE_CLASS( CWeaponMolotov, CBaseHL2MPCombatWeapon );
public:

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CWeaponMolotov();

	~CWeaponMolotov() {
#ifndef CLIENT_DLL
		if (pLighterFlame)
			UTIL_Remove(pLighterFlame);

		if (pClothFlame)
			UTIL_Remove(pClothFlame);
#endif
	}

	void	Precache( void );
	void	PrimaryAttack( void );
	void	SecondaryAttack( void );
	void	DecrementAmmo( CBaseCombatCharacter *pOwner );
	void	ItemPostFrame( void );
	void	Spawn ( void );

	bool	Deploy( void );
	bool	Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	
	bool	Reload( void );
	void	CheckForZeroAmmo( CBaseCombatCharacter *pOperator );


#ifndef CLIENT_DLL
	void	Equip(  CBaseCombatCharacter *pOwner );
	void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	virtual void	HandleAnimEvent( animevent_t *pEvent );
	void RemoveFlames();
#else
	void	ViewModelDrawn( C_BaseViewModel *pBaseViewModel );

	void	DrawJiggeringSprite(Vector vecAttach); //Animate the flame


	//virtual RenderGroup_t	GetRenderGroup( void ) {	return RENDER_GROUP_TWOPASS;	}
	//virtual bool			IsTransparent( void ) { return true; }
	//virtual bool			IsTranslucent( void ) { return true; }
	//virtual RenderGroup_t	GetRenderGroup( void ) { return RENDER_GROUP_TRANSLUCENT_ENTITY; }
	
	//TGB: use advanced method instead of just EyePosition for doing our player-weapon pickup bump visibility trace
// 	Vector GetBumpTracePos() {
// 		IPhysicsObject *physobj = VPhysicsGetObject();
// 		Vector worldPos;
// 		if (physobj)
// 			physobj->LocalToWorld( &worldPos, physobj->GetMassCenterLocalSpace() );
// 		else
// 			worldPos = EyePosition();
// 
// 		return worldPos;
// 	}

#endif
	float	m_fNextJiggerTime;
	int		m_iLastJiggerX;
	int		m_iLastJiggerY;

	void	ThrowGrenade( CBasePlayer *pPlayer );
	bool	IsPrimed( bool ) { return ( m_AttackPaused != 0 );	}
	
	virtual int				GetWeaponTypeNumber( void ) { return 8; } //LAWYER:  In the equipment group

	//TGB: can't whip out a molotov if we have none in our pockets (though we should never have an empty molotov weapon)
	bool	CanDeploy( void) { 
		CBaseCombatCharacter *pOwner = GetOwner();
		if (pOwner)
			return (pOwner->GetAmmoCount(GetPrimaryAmmoType()) > 0);
		else
			return false;
	}


private:

	void	RollGrenade( CBasePlayer *pPlayer );
	void	LobGrenade( CBasePlayer *pPlayer );
	// check a throw from vecSrc.  If not valid, move the position back along the line to vecEye
	void	CheckThrowPosition( CBasePlayer *pPlayer, const Vector &vecEye, Vector &vecSrc );

	CNetworkVar( bool,	m_bRedraw );	//Draw the weapon again after throwing a grenade
	
	CNetworkVar( int,	m_AttackPaused );
	CNetworkVar( bool,	m_fDrawbackFinished );
	CNetworkVar( bool,	m_bLighterFlame );
	CNetworkVar( bool,	m_bClothFlame );


#ifndef CLIENT_DLL
	CSprite *pLighterFlame;
	CSprite *pClothFlame;
#else
	CMaterialReference	m_hFlameMaterial;
	CMaterialReference	m_hClothFlameMaterial;
#endif
	/*
#ifdef CLIENT_DLL
	C_FireMolotov		*molotovFlame;
#endif
	*/

	CWeaponMolotov( const CWeaponMolotov & );
	int					m_iExplosionStep;			//LAWYER:  The step through the ignite animation
	bool				m_bIgnited; //we're succesfully igniting during/after this anim


#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();
#endif
};

#ifndef CLIENT_DLL

acttable_t	CWeaponMolotov::m_acttable[] = 
{
	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_GRENADE,					false },
	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_GRENADE,					false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_GRENADE,			false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_GRENADE,			false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_GRENADE,	false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_GRENADE,		false },
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_GRENADE,					false },
};

IMPLEMENT_ACTTABLE(CWeaponMolotov);

#endif

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponMolotov, DT_WeaponMolotov )

BEGIN_NETWORK_TABLE( CWeaponMolotov, DT_WeaponMolotov )

#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bRedraw ) ),
	RecvPropBool( RECVINFO( m_fDrawbackFinished ) ),
	RecvPropInt( RECVINFO( m_AttackPaused ) ),
	RecvPropBool( RECVINFO( m_bLighterFlame ) ),
	RecvPropBool( RECVINFO( m_bClothFlame ) ),
#else
	SendPropBool( SENDINFO( m_bRedraw ) ),
	SendPropBool( SENDINFO( m_fDrawbackFinished ) ),
	SendPropInt( SENDINFO( m_AttackPaused ) ),
	SendPropBool( SENDINFO( m_bLighterFlame ) ),
	SendPropBool( SENDINFO( m_bClothFlame ) ),
#endif
	
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponMolotov )
	DEFINE_PRED_FIELD( m_bRedraw, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_fDrawbackFinished, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_AttackPaused, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_zm_molotov, CWeaponMolotov );
PRECACHE_WEAPON_REGISTER(weapon_zm_molotov);

CWeaponMolotov::CWeaponMolotov( void )
{
#ifndef CLIENT_DLL
	pLighterFlame = NULL;
	pClothFlame = NULL;
#endif
/*#ifdef CLIENT_DLL
	molotovFlame = new C_FireMolotov();
#endif*/

}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMolotov::Precache( void )
{
	BaseClass::Precache();

#ifndef CLIENT_DLL
	//UTIL_PrecacheOther( "npc_zm_molotov" ); //TGB: this said npc_zm_molotov, probably a mistake
	UTIL_PrecacheOther( "grenade_molotov" ); //TGB: figured out that npc_grenade was the frag nade's ent class, so corrected this to grenade_molotov (which is the bottle's ent class, see grenade_molotov.cpp)
#endif

	PrecacheModel( MOLOTOV_FIRE_SPRITE );

	PrecacheScriptSound( "WeaponFrag.Throw" );
	PrecacheScriptSound( "WeaponFrag.Roll" );
	m_bRedraw = false;
	m_iExplosionStep = 0;
}

// TGB: we need a spawn here because the molotov always gets ammo on spawn, contrary to normal weps
void CWeaponMolotov::Spawn( void )
{
	//get default ammo
	SetPrimaryAmmoCount( GetDefaultClip1() );
	
	m_iClip1 = WEAPON_NOCLIP;

	//DevMsg("bLF to false @spawn\n");
	m_bLighterFlame = false;
	m_bClothFlame = false;

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if (pOwner)
	{
		CBaseViewModel *pVM = pOwner->GetViewModel();
		if (pVM)
		{
			pVM->SetBodygroup(1, BG_INVISIBLE);
		}
	}

//	DevMsg("Mtov spwn rprt: BG0: %i, BG1: %i ", pVM->GetBodygroup(0), pVM->GetBodygroup(1));



	//DevMsg("changed to BG0: %i, BG1: %i\n", GetBodygroup(0), GetBodygroup(1));

	

	/* we don't use clip2
	SetSecondaryAmmoCount( GetDefaultClip2() );
	m_iClip2 = WEAPON_NOCLIP;
	*/
	BaseClass::Spawn();
	//LAWYER:  Molotov specific stuff
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Receive AE_ZM_LIGHTERFLAME and handle it
//-----------------------------------------------------------------------------
void CWeaponMolotov::HandleAnimEvent( animevent_t *pEvent )
{
	//DevMsg("BG: %i state %i\n", FindBodygroupByName("flame"), GetBodygroup(FindBodygroupByName("flame")));

	if( pEvent->event == AE_ZM_LIGHTERFLAME) 
	{

		//turning it on/off doesn't seem to have any effect
	
		//DevMsg("BG state is %i, bLF is %i, onreq: 0,1 offreq: 1,0 \n", GetBodygroup(1), m_bLighterFlame);
		
		//screw the lighter flame bool, just switch it around and see if it does anything
		//if (/*m_bLighterFlame &&*/ m_nSkin == BG_INVISIBLE)
		/*{
			DevMsg("Attempting to turn on flame\n");
			m_nSkin = BG_VISIBLE;
			//SetBodygroup(1, BG_VISIBLE); //turn on flame
			m_bLighterFlame = false;
		}*/

		//else if (/*!m_bLighterFlame &&*/ m_nSkin == BG_VISIBLE)
		/*{
			DevMsg("Attempting to turn OFF flame\n");
			m_nSkin = INVISIBLE;
			//SetBodygroup(1,BG_INVISIBLE);
		}*/
		DevMsg("RECEIVED LIGHTEREVENT\n");
		
		m_fNextJiggerTime = gpGlobals->curtime; //Initialise the flame for every throw
		m_iLastJiggerX = 2;
		m_iLastJiggerY = 4;
		

		CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
		if (pOwner)
		{
			CBaseViewModel *pVM = pOwner->GetViewModel();
			if (pVM)
			{
				if ( m_bLighterFlame /*&& pVM->GetBodygroup(1) == BG_INVISIBLE*/ )
				{
					DevMsg("Attempting to turn on flame\n");
					//THIS WORKS AW YEAH
					pVM->SetBodygroup(1, BG_VISIBLE);
					DevMsg("Flame goes off next event\n");
					m_bLighterFlame = false;
				}
				else if ( !m_bLighterFlame/* && pVM->GetBodygroup(1) == BG_VISIBLE*/ )
				{
					DevMsg("Attempting to turn OFF flame\n");
					pVM->SetBodygroup(1, BG_INVISIBLE);
				}
			}
		}
	}
	else if (pEvent->event == AE_ZM_CLOTHFLAME)
	{
		//DevMsg("Recvd AE_ZM_CLOTHFLAME\n");
		m_bClothFlame = true;

		//DevMsg("bLF to false @clothflame\n");
		//m_bLighterFlame = false; //when cloth is on, we can turn the lighter off soon
	}
	

	BaseClass::HandleAnimEvent( pEvent );
	
}
#endif

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: handle npc/third person events
// Input  : *pEvent - 
//			*pOperator - 
//-----------------------------------------------------------------------------
void CWeaponMolotov::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	bool fThrewGrenade = false;

	switch( pEvent->event )
	{
		case EVENT_WEAPON_SEQUENCE_FINISHED:
			//LAWYER:  Cycle through the different animations here
			if (m_iExplosionStep < 4 && m_iExplosionStep > 0)
			{
				float fRandomness = random->RandomFloat(0,100);

				m_iExplosionStep++;
				DevMsg("ExplosionStep %i\n", m_iExplosionStep);

				if (m_bIgnited)
				{
					DevMsg("IGNITED! bLF should be TRUE here, is: %i\n", (int)m_bLighterFlame );
					//m_bLighterFlame = false;
					SendWeaponAnim( ACT_VM_PRIMARYATTACK_4 );
					m_iExplosionStep = 4;
					return;
				}

				//if rand is smaller than 100 - our health, this light attempt works
				//this method made no sense and was borked, it should be easier to light if health is high
				if ( fRandomness < min(pOwner->GetHealth(),100) )
				{
					DevMsg("NEXT ANIM = SUCCESS! bLF to TRUE\n");
					m_bLighterFlame = true;
					m_bIgnited = true;
					
				}
				else if (m_iExplosionStep >= 3)
				{
					m_bLighterFlame = true;
					m_bIgnited = true; //always works on our last anim
				}

				switch (m_iExplosionStep)
				{
				case 2:
					SendWeaponAnim( ACT_VM_PRIMARYATTACK_2 ); break;
				case 3:
					SendWeaponAnim( ACT_VM_PRIMARYATTACK_3 ); break;
				case 4:
					m_bIgnited = true; break; //failsafe
				default:
					DevMsg("ERROR: Reached molotov switch default!");
					m_bIgnited = true;
				}

				/*
				
				if (fRandomness > 75 && m_iExplosionStep != 3)
				{
					m_iExplosionStep++;
					if (m_iExplosionStep == 2)
						{
							SendWeaponAnim( ACT_VM_PRIMARYATTACK_2 );
							Warning( "ZOMJ!  Primaryattack2!\n");
						}
					else if (m_iExplosionStep == 3)
						{
							SendWeaponAnim( ACT_VM_PRIMARYATTACK_3 );
							Warning( "OHNOES!  Primaryattack3!\n");
						}
				}
				else
				{
					m_bLighterFlame = true;
					//TGB: if we send the lighting anim, step should be 4 so that we don't send others afterwards
					m_iExplosionStep = 4;
					SendWeaponAnim( ACT_VM_PRIMARYATTACK_4 );
					Warning("It finally lit! Primaryattack4\n");
					m_bClothFlame = true;
				}
				*/
			}
			else
			{
				m_fDrawbackFinished = true;
			}
			break;

		case EVENT_WEAPON_THROW:
		case EVENT_WEAPON_THROW2:
		case EVENT_WEAPON_THROW3:
			Msg("Throwing molotov event received, throwfunction called!\n");
			ThrowGrenade( pOwner );
			DecrementAmmo( pOwner );
			fThrewGrenade = true;
			CheckForZeroAmmo(pOperator);
			break;
/*
		case EVENT_WEAPON_THROW2:
			RollGrenade( pOwner );
			DecrementAmmo( pOwner );
			fThrewGrenade = true;
			CheckForZeroAmmo(pOperator);
			break;

		case EVENT_WEAPON_THROW3:
			LobGrenade( pOwner );
			DecrementAmmo( pOwner );
			fThrewGrenade = true;
			CheckForZeroAmmo(pOperator);
			break;
*/
		default:
			BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
			break;
	}

#define RETHROW_DELAY	0.5
	if( fThrewGrenade )
	{
		m_flNextPrimaryAttack	= gpGlobals->curtime + RETHROW_DELAY;
		m_flNextSecondaryAttack	= gpGlobals->curtime + RETHROW_DELAY;
		m_flTimeWeaponIdle = FLT_MAX; //NOTE: This is set once the animation has finished up!
	}
}

#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponMolotov::Deploy( void )
{
	m_bRedraw = false;
	m_fDrawbackFinished = false;

	//flame off
	m_bClothFlame = false;
	m_bLighterFlame = false;
	//SetBodygroup(1,BG_INVISIBLE);

/*#ifdef CLIENT_DLL
	if (IsCarriedByLocalPlayer())
		molotovFlame->StartClientOnly();
#endif*/

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponMolotov::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	m_bRedraw = false;
	m_fDrawbackFinished = false;

	//TGB: flame was staying on after holstering
	m_bClothFlame = false;
	m_bLighterFlame = false;
	

	//flame off
	//SetBodygroup(1,BG_INVISIBLE);
/*#ifdef CLIENT_DLL
	if (IsCarriedByLocalPlayer())
		molotovFlame->RemoveClientOnly();
#endif*/

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponMolotov::Reload( void )
{
#ifndef CLIENT_DLL
	RemoveFlames();
#endif
	//flame off
	//SetBodygroup(1,BG_INVISIBLE);

	if ( !HasPrimaryAmmo() )
		return false;

	if ( ( m_bRedraw ) && ( m_flNextPrimaryAttack <= gpGlobals->curtime ) && ( m_flNextSecondaryAttack <= gpGlobals->curtime ) )
	{
		//Redraw the weapon
		SendWeaponAnim( ACT_VM_DRAW );

		m_bClothFlame = false;

		//Update our times
		m_flNextPrimaryAttack	= gpGlobals->curtime + SequenceDuration();
		m_flNextSecondaryAttack	= gpGlobals->curtime + SequenceDuration();
		m_flTimeWeaponIdle = gpGlobals->curtime + SequenceDuration();

		//Mark this as done
		m_bRedraw = false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMolotov::SecondaryAttack( void )
{/*
	if ( m_bRedraw )
		return;

	if ( !HasPrimaryAmmo() )
		return;

	CBaseCombatCharacter *pOwner  = GetOwner();

	if ( pOwner == NULL )
		return;

	CBasePlayer *pPlayer = ToBasePlayer( pOwner );
	
	if ( pPlayer == NULL )
		return;

	// Note that this is a secondary attack and prepare the grenade attack to pause.
	m_AttackPaused = GRENADE_PAUSED_SECONDARY;
	SendWeaponAnim( ACT_VM_PULLBACK_LOW );

	// Don't let weapon idle interfere in the middle of a throw!
	m_flTimeWeaponIdle = FLT_MAX;
	m_flNextSecondaryAttack	= FLT_MAX;

	// If I'm now out of ammo, switch away
	if ( !HasPrimaryAmmo() )
	{
		pPlayer->SwitchToNextBestWeapon( this );
	}*/
	return; //LAWYER:  Rolling a Molotov would be a rather silly idea
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMolotov::PrimaryAttack( void )
{
	if ( m_bRedraw )
		return;

	CBaseCombatCharacter *pOwner  = GetOwner();
	
	if ( pOwner == NULL )
	{ 
		return;
	}

	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if (pPlayer->GetMoveType()==MOVETYPE_LADDER)
		return;

	if ( !pPlayer )
		return;

	if (pOwner->GetWaterLevel() == 3) //LAWYER: No underwaterness
		return;

	// Note that this is a primary attack and prepare the grenade attack to pause.
	
	m_AttackPaused = GRENADE_PAUSED_PRIMARY;

	//TGB: first lighting attempt a success?
	const int iRandomness = random->RandomInt(0,100);
	if ( iRandomness < min(pPlayer->GetHealth(),75) )
	{
		DevMsg("Rand is %i and healthrating is %i\n", iRandomness, min(pPlayer->GetHealth(),75));
		DevMsg("NEXT ANIM = SUCCESS! bLF to TRUE\n");
		m_bLighterFlame = true;
		m_bIgnited = true;
		
	}
	else
	{
		DevMsg("No success, bLF to false\n");
		m_bLighterFlame = false;
		m_bIgnited = false;
	}

	SendWeaponAnim( ACT_VM_PRIMARYATTACK_1 );
	

	m_iExplosionStep = 1;

	// Put both of these off indefinitely. We do not know how long
	// the player will hold the grenade.
	m_flTimeWeaponIdle = FLT_MAX;
	m_flNextPrimaryAttack = FLT_MAX;

	// If I'm now out of ammo, switch away
	if ( !HasPrimaryAmmo() )
	{
		pPlayer->SwitchToNextBestWeapon( this );
	}

#ifndef CLIENT_DLL

	if ( pPlayer == NULL )
		return;

	
	if (!pLighterFlame)
	{
		pLighterFlame = CSprite::SpriteCreate( MOLOTOV_FIRE_SPRITE, GetAbsOrigin(), false );

		if ( pLighterFlame )
		{
			int attachment = LookupAttachment( "clothflame" );
			pLighterFlame->SetAttachment( pPlayer->GetViewModel(), attachment );
			pLighterFlame->SetTransparency( kRenderTransAlpha, 255, 255, 255, 0, kRenderFxStrobeSlow );
			//pFlame->SetBrightness( 128 );
			pLighterFlame->SetScale( 0.01f );
			//pFlame->SetGlowProxySize( 4.0f );
			//pLighterFlame->AnimateThink();
			//pFlame->AnimateUntilDead();
		}
	}else if (!pLighterFlame->IsOn())
		pLighterFlame->TurnOn();
	

#endif
}
#ifndef CLIENT_DLL
void CWeaponMolotov::RemoveFlames()
{
	
	if ( pLighterFlame )
	{
		pLighterFlame->TurnOff();
	}
	/*
	if ( pClothFlame )
	{
		pClothFlame->TurnOff();
	}
	*/
	
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOwner - 
//-----------------------------------------------------------------------------
void CWeaponMolotov::DecrementAmmo( CBaseCombatCharacter *pOwner )
{
	pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMolotov::ItemPostFrame( void )
{
		//update flames
#ifndef CLIENT_DLL
	//DevMsg("Simulating.. ");
	/*
	if ( pLighterFlame )
	{
		//DevMsg("Attempting to update lighterflame.. ");

		int maxFrames = modelinfo->GetModelFrameCount(pLighterFlame->GetModel());
		if (pLighterFlame->m_flFrame + 1 > maxFrames)
		{
			pLighterFlame->Animate(0);
			//DevMsg("maxFrames reached! ");
		}
		else
			pLighterFlame->Animate(pLighterFlame->m_flFrame + 1);
	}
	*/
	
	if ( pClothFlame )
	{
		//DevMsg("Attempting to update clothflame.. ");
		int maxFrames = modelinfo->GetModelFrameCount(pClothFlame->GetModel());
		if (pClothFlame->m_flFrame + 1 > maxFrames)
		{
			pClothFlame->Animate(0);
			//DevMsg("maxFrames reached! ");
		}
		else
			pClothFlame->Animate(pClothFlame->m_flFrame + 1);
	}
	//DevMsg("\n");
#endif

	if( m_fDrawbackFinished )
	{
		CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

		if (pOwner)
		{
			switch( m_AttackPaused )
			{
			case GRENADE_PAUSED_PRIMARY:
			case GRENADE_PAUSED_SECONDARY:
				if( !(pOwner->m_nButtons & IN_ATTACK) && m_iExplosionStep >= 4) //LAWYER:  Make sure its ignited first
				{
					Msg("ItemPostFrame sends throw anim!\n");
					SendWeaponAnim( ACT_VM_THROW );
					m_fDrawbackFinished = false;
				}
				break;
			//TGB: does this ever occur?
/*			case GRENADE_PAUSED_SECONDARY:
				if( !(pOwner->m_nButtons & IN_ATTACK2) )
				{
					//See if we're ducking
					if ( pOwner->m_nButtons & IN_DUCK )
					{
						//Send the weapon animation
						SendWeaponAnim( ACT_VM_SECONDARYATTACK );
					}
					else
					{
						Msg("ItemPostFrame initiates haulback anim!\n");
						//Send the weapon animation
						SendWeaponAnim( ACT_VM_HAULBACK );
					}

					m_fDrawbackFinished = false;
				}
				break;
*/ //LAWYER:  Safeguard against something that shouldn't really happen
			default:
				break;
			}
		}
	}

	BaseClass::ItemPostFrame();

	if ( m_bRedraw )
	{
		if ( IsViewModelSequenceFinished() )
		{
			Reload();
		}
	}
}

	// check a throw from vecSrc.  If not valid, move the position back along the line to vecEye
void CWeaponMolotov::CheckThrowPosition( CBasePlayer *pPlayer, const Vector &vecEye, Vector &vecSrc )
{
	trace_t tr;

	UTIL_TraceHull( vecEye, vecSrc, -Vector(GRENADE_RADIUS+2,GRENADE_RADIUS+2,GRENADE_RADIUS+2), Vector(GRENADE_RADIUS+2,GRENADE_RADIUS+2,GRENADE_RADIUS+2), 
		pPlayer->PhysicsSolidMaskForEntity(), pPlayer, pPlayer->GetCollisionGroup(), &tr );
	
	if ( tr.DidHit() )
	{
		vecSrc = tr.endpos;
	}
}

void DropPrimedFragGrenade( CHL2MP_Player *pPlayer, CBaseCombatWeapon *pGrenade )
{
	CWeaponMolotov *pWeaponFrag = dynamic_cast<CWeaponMolotov*>( pGrenade );

	if ( pWeaponFrag )
	{
		pWeaponFrag->ThrowGrenade( pPlayer );
		pWeaponFrag->DecrementAmmo( pPlayer );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CWeaponMolotov::ThrowGrenade( CBasePlayer *pPlayer )
{
#ifndef CLIENT_DLL
	if (!pPlayer) return;

	Vector	vecEye = pPlayer->EyePosition();
	Vector	vForward, vRight;

	pPlayer->EyeVectors( &vForward, &vRight, NULL );
	Vector vecSrc = vecEye + vForward * 18.0f + vRight * 10.0f; //was: 7 and 25; was: 10 and 25; was 18 and 8
	CheckThrowPosition( pPlayer, vecEye, vecSrc );
//	vForward[0] += 0.1f;
	vForward[2] += 0.1f;

	Vector vecThrow;
	pPlayer->GetVelocity( &vecThrow, NULL );
	vecThrow += vForward * 1200;

//	CBaseGrenade *pGrenade = Fraggrenade_Create( vecSrc, vec3_angle, vecThrow, AngularImpulse(600,random->RandomInt(-1200,1200),0), pPlayer, GRENADE_TIMER );

	// Don't set the owner here, or the player can't interact with grenades he's thrown
	CGrenade_Molotov *pGrenade = (CGrenade_Molotov *)CBaseEntity::Create( "grenade_molotov", vecSrc, vec3_angle, pPlayer );
	
	//pGrenade->SetTimer( GRENADE_TIMER, GRENADE_TIMER - 1.5f );
	pGrenade->SetVelocity( vecThrow, AngularImpulse(600,random->RandomInt(-1200,1200),0) );
	pGrenade->SetThrower( ToBaseCombatCharacter( pPlayer ) );
	pGrenade->m_takedamage = DAMAGE_EVENTS_ONLY;
	pGrenade->ApplyAbsVelocityImpulse(vecThrow);
	pGrenade->SetLocalAngularVelocity(
	QAngle(
			0,
			0,
			random->RandomFloat ( -100, -500 )
			)
									);
	pGrenade->m_pDamageParent = pPlayer; //LAWYER:  For damage parenting/anti-TK

	if ( pGrenade )
	{
		if ( pPlayer && pPlayer->m_lifeState != LIFE_ALIVE )
		{
			pPlayer->GetVelocity( &vecThrow, NULL );

			IPhysicsObject *pPhysicsObject = pGrenade->VPhysicsGetObject();
			if ( pPhysicsObject )
			{
				pPhysicsObject->SetVelocity( &vecThrow, NULL );
			}
		}
		//pGrenade->SetOwnerEntity(this->GetOwnerEntity());
//		pGrenade->SetDamage( GetHL2MPWpnData().m_iPlayerDamage );
//		pGrenade->SetDamageRadius( GRENADE_DAMAGE_RADIUS );
	}
#endif

	m_bRedraw = true;

	WeaponSound( SINGLE );
	
	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );
	m_iExplosionStep = 0;

	//TGB: turn off lighterflame always
	CBaseViewModel *pVM = pPlayer->GetViewModel();
	if (pVM)
	{
		pVM->SetBodygroup(1, BG_INVISIBLE);
	}
	m_bLighterFlame = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CWeaponMolotov::LobGrenade( CBasePlayer *pPlayer )
{
#ifndef CLIENT_DLL
	Vector	vecEye = pPlayer->EyePosition();
	Vector	vForward, vRight;

	pPlayer->EyeVectors( &vForward, &vRight, NULL );
	Vector vecSrc = vecEye + vForward * 18.0f + vRight * 8.0f + Vector( 0, 0, -8 );
	CheckThrowPosition( pPlayer, vecEye, vecSrc );
	
	Vector vecThrow;
	pPlayer->GetVelocity( &vecThrow, NULL );
	vecThrow += vForward * 350 + Vector( 0, 0, 50 );
	//CBaseGrenade *pGrenade = Fraggrenade_Create( vecSrc, vec3_angle, vecThrow, AngularImpulse(200,random->RandomInt(-600,600),0), pPlayer, GRENADE_TIMER );
	// Don't set the owner here, or the player can't interact with grenades he's thrown
	CGrenade_Molotov *pGrenade = (CGrenade_Molotov *)CBaseEntity::Create( "grenade_molotov", vecSrc, vec3_angle, pPlayer );
	
	//pGrenade->SetTimer( GRENADE_TIMER, GRENADE_TIMER - 1.5f );
	pGrenade->SetVelocity( vecThrow, AngularImpulse(200,random->RandomInt(-600,600),0) );
	pGrenade->SetThrower( ToBaseCombatCharacter( pPlayer ) );
	pGrenade->m_takedamage = DAMAGE_EVENTS_ONLY;
	pGrenade->ApplyAbsVelocityImpulse(vecThrow);
	pGrenade->SetLocalAngularVelocity(
	QAngle( 0, 0, random->RandomFloat ( -100, -500 )));

#endif

	WeaponSound( WPN_DOUBLE );

	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_bRedraw = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CWeaponMolotov::RollGrenade( CBasePlayer *pPlayer )
{/*
#ifndef CLIENT_DLL
	// BUGBUG: Hardcoded grenade width of 4 - better not change the model :)
	Vector vecSrc;
	pPlayer->CollisionProp()->NormalizedToWorldSpace( Vector( 0.5f, 0.5f, 0.0f ), &vecSrc );
	vecSrc.z += GRENADE_RADIUS;

	Vector vecFacing = pPlayer->BodyDirection2D( );
	// no up/down direction
	vecFacing.z = 0;
	VectorNormalize( vecFacing );
	trace_t tr;
	UTIL_TraceLine( vecSrc, vecSrc - Vector(0,0,16), MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_NONE, &tr );
	if ( tr.fraction != 1.0 )
	{
		// compute forward vec parallel to floor plane and roll grenade along that
		Vector tangent;
		CrossProduct( vecFacing, tr.plane.normal, tangent );
		CrossProduct( tr.plane.normal, tangent, vecFacing );
	}
	vecSrc += (vecFacing * 18.0);
	CheckThrowPosition( pPlayer, pPlayer->WorldSpaceCenter(), vecSrc );

	Vector vecThrow;
	pPlayer->GetVelocity( &vecThrow, NULL );
	vecThrow += vecFacing * 700;
	// put it on its side
	QAngle orientation(0,pPlayer->GetLocalAngles().y,-90);
	// roll it
	AngularImpulse rotSpeed(0,0,720);
//	CBaseGrenade *pGrenade = Fraggrenade_Create( vecSrc, orientation, vecThrow, rotSpeed, pPlayer, GRENADE_TIMER );
	CGrenade_Molotov *pGrenade = (CGrenade_Molotov *)CBaseEntity::Create( "grenade_molotov", vecSrc, vec3_angle, pPlayer );
	
	//pGrenade->SetTimer( GRENADE_TIMER, GRENADE_TIMER - 1.5f );
//	pGrenade->SetVelocity( vecThrow, rotSpeed );
	pGrenade->SetThrower( ToBaseCombatCharacter( pPlayer ) );
	pGrenade->m_takedamage = DAMAGE_EVENTS_ONLY;

	if ( pGrenade )
	{
		pGrenade->SetDamage( GetHL2MPWpnData().m_iPlayerDamage );
		pGrenade->SetDamageRadius( GRENADE_DAMAGE_RADIUS );
	}

#endif

	WeaponSound( SPECIAL1 );

	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_bRedraw = true;*/
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Attempt to give an extra molotov to get around the equip with 0 stuff.
//	
// Input  : *pOwner - new owner/operator
//-----------------------------------------------------------------------------
void CWeaponMolotov::Equip( CBaseCombatCharacter *pOwner )
{
	BaseClass::Equip(pOwner);
	if (pOwner)
	{
		if (pOwner->GetAmmoCount(GetPrimaryAmmoType()) == 0)
		{
			pOwner->GiveAmmo( 1, GetPrimaryAmmoType(), true);
		}
	}
}
#endif

//LAWYER:  Remove this weapon when we're out of ammo!
void CWeaponMolotov::CheckForZeroAmmo( CBaseCombatCharacter *pOperator  )
{
#ifndef CLIENT_DLL
	if (pOperator->GetAmmoCount(GetPrimaryAmmoType()) == 0)
	{
			pOperator->Weapon_Drop( this );
			UTIL_Remove( this );	
			pOperator->m_iWeaponFlags -= GetWeaponTypeNumber();
	}
#endif
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Get the attachment point on a viewmodel that a base weapon is using
//-----------------------------------------------------------------------------
bool UTIL_GetWeaponAttachment( C_BaseCombatWeapon *pWeapon, int attachmentID, Vector &absOrigin, QAngle &absAngles )
{
	// This is already correct in third-person
	if ( pWeapon && pWeapon->IsCarriedByLocalPlayer() == false )
	{
		return pWeapon->GetAttachment( attachmentID, absOrigin, absAngles );
	}

	// Otherwise we need to translate the attachment to the viewmodel's version and reformat it
	CBasePlayer *pOwner = ToBasePlayer( pWeapon->GetOwner() );
	
	if ( pOwner != NULL )
	{
		int ret = pOwner->GetViewModel()->GetAttachment( attachmentID, absOrigin, absAngles );
		FormatViewModelAttachment( absOrigin, true );

		return ret;
	}

	// Wasn't found
	return false;
}

void C_WeaponMolotov::ViewModelDrawn( C_BaseViewModel *pBaseViewModel )
{
	
	// Don't bother when we're not deployed
	if ( IsWeaponVisible() )
	{
		/*
		if (m_bLighterFlame && IsCarriedByLocalPlayer())
		{	
			//C_BaseAnimating::AllowBoneAccess( false, true );
			
				// Setup our sprite
			if ( m_hFlameMaterial == NULL )
			{
				m_hFlameMaterial.Init( MOLOTOV_FIRE_SPRITE, TEXTURE_GROUP_CLIENT_EFFECTS );
				
			}
			

			// Setup the attachment
			QAngle angles;
			Vector vecAttach, vecForward, vecRight, vecUp;
			int attachment = LookupAttachment( "lighterflame" );

			UTIL_GetWeaponAttachment(this, attachment, vecAttach, angles);
			
			//vecAttach *= Vector(0,0,5);
			//AngleVectors( angles, &vecUp, &vecRight, &vecForward ); 
			
			//DevMsg("V:%f %f %f || ", vecAttach.x, vecAttach.y, vecAttach.z);

			// Draw the sprite
			materials->Bind( m_hFlameMaterial, this );

			DrawSprite( vecAttach, 0.75f, 1.5f, color ); 				

			

			//m_bLighterFlame = false;

		}
		*/
		if (m_bClothFlame && IsCarriedByLocalPlayer())
			{	
				//DevMsg("Drawing clothflame\n");
			//C_BaseAnimating::AllowBoneAccess( false, true );
			
				// Setup our sprite
			if ( m_hClothFlameMaterial == NULL )
			{
				m_hClothFlameMaterial.Init( MOLOTOV_FIRE_SPRITE, TEXTURE_GROUP_CLIENT_EFFECTS );
			}
			
			// Setup the attachment
			QAngle angles;
			Vector vecAttach, vecForward, vecRight, vecUp;
			int attachment = LookupAttachment( "clothflame" );

			UTIL_GetWeaponAttachment(this, attachment, vecAttach, angles);
			
			//vecAttach *= Vector(0,0,5);
			//AngleVectors( angles, &vecUp, &vecRight, &vecForward ); 
			
			//DevMsg("V:%f %f %f || ", vecAttach.x, vecAttach.y, vecAttach.z);

			// Draw the sprite
			materials->Bind( m_hClothFlameMaterial, this );
			
			DrawJiggeringSprite(vecAttach);

			//DrawSprite( vecAttach, 2.0f, 4.0f, flamecolor ); 				

		}
	}
	
	BaseClass::ViewModelDrawn( pBaseViewModel );
}

//LAWYER:  Randomly flickers our sprite for MASSIVE DAMAGE
void C_WeaponMolotov::DrawJiggeringSprite(Vector vecAttach)
{
	//Colourise...
	//TGB: reduced the green-ness a bit
	int green = 100 - random->RandomInt(0, 64); //The green channel deals with the yellow-redness
	color32 flamecolor={255,green,0,255};

	//Resize...
	if (gpGlobals->curtime >= m_fNextJiggerTime)
	{
		m_iLastJiggerX = random->RandomFloat( 1.0f, 2.0f);
		m_iLastJiggerY = random->RandomFloat( 3.8f, 4.2f);
		m_fNextJiggerTime = gpGlobals->curtime + random->RandomFloat(0.2f, 1.0f); //Reflicker randomly
	}

	DrawSprite( vecAttach, m_iLastJiggerX, m_iLastJiggerY, flamecolor );
}
#endif
