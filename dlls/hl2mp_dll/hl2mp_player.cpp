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

//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:		Player for HL2.
//
//=============================================================================//

#include "weapon_hl2mpbasehlmpcombatweapon.h"

#include "cbase.h"
#include "hl2mp_player.h"
#include "globalstate.h"
#include "game.h"
#include "gamerules.h"
#include "hl2mp_player_shared.h"
#include "predicted_viewmodel.h"
#include "in_buttons.h"
#include "hl2mp_gamerules.h"
#include "KeyValues.h"
#include "team.h"
#include "weapon_hl2mpbase.h"
#include "grenade_satchel.h"
#include "eventqueue.h"
#include "fire.h" //LAWYER:  So I can use my fire hax!
#include "weapon_physcannon.h" //qck: so I can use me carry object hax! ;)

#include "engine/IEngineSound.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"

#include "ilagcompensationmanager.h"

#include "tier0/vprof.h"

int g_iLastCitizenModel = 0;
int g_iLastCombineModel = 0;

CBaseEntity	 *g_pLastCombineSpawn = NULL;
CBaseEntity	 *g_pLastRebelSpawn = NULL;
extern CBaseEntity				*g_pLastSpawn;

//TGB: amount of power we start with, also the maximum (we don't recharge)
#define ZM_FLASHLIGHT_START_AMOUNT 100.0f

ConVar zm_flashlight_drainrate( "zm_flashlight_drainrate", "0.15", FCVAR_NOTIFY, "Units of power to drain per second of flashlight being enabled, of a total 100");


#define HL2MP_COMMAND_MAX_RATE 0.3

void ClientKill( edict_t *pEdict );
//void DropPrimedFragGrenade( CHL2MP_Player *pPlayer, CBaseCombatWeapon *pGrenade ); //LAWYER:  Disabled, until we put frags in

LINK_ENTITY_TO_CLASS( player, CHL2MP_Player );

LINK_ENTITY_TO_CLASS( info_player_combine, CPointEntity );
LINK_ENTITY_TO_CLASS( info_player_rebel, CPointEntity );

IMPLEMENT_SERVERCLASS_ST(CHL2MP_Player, DT_HL2MP_Player)
	SendPropAngle( SENDINFO_VECTORELEM(m_angEyeAngles, 0), 11, SPROP_CHANGES_OFTEN ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angEyeAngles, 1), 11, SPROP_CHANGES_OFTEN ),
	
	//tgb
	SendPropFloat( SENDINFO(m_flFlashlightBattery), 10, SPROP_UNSIGNED | SPROP_ROUNDUP, 0.0, ZM_FLASHLIGHT_START_AMOUNT ), //0 to 100

	SendPropEHandle( SENDINFO( m_hRagdoll ) ),
	SendPropInt( SENDINFO( m_iSpawnInterpCounter), 4 ),
	SendPropInt( SENDINFO( m_iPlayerSoundType), 3 ),
	
	SendPropInt( SENDINFO( m_zmParticipation), 3, SPROP_UNSIGNED ),

	SendPropExclude( "DT_BaseAnimating", "m_flPoseParameter" ),
	SendPropExclude( "DT_BaseFlex", "m_viewtarget" ),

//	SendPropExclude( "DT_ServerAnimationData" , "m_flCycle" ),	
//	SendPropExclude( "DT_AnimTimeMustBeFirst" , "m_flAnimTime" ),
	
END_SEND_TABLE()

BEGIN_DATADESC( CHL2MP_Player )
END_DATADESC()

const char *g_ppszRandomCitizenModels[] = 
{
	"models/humans/group02/male_01.mdl",
	"models/humans/group02/male_02.mdl",
	"models/humans/group02/female_01.mdl",
	"models/humans/group02/male_03.mdl",
	"models/humans/group02/female_02.mdl",
	"models/humans/group02/male_04.mdl",
	"models/humans/group02/female_03.mdl",
	"models/humans/group02/male_05.mdl",
	"models/humans/group02/female_04.mdl",
	"models/humans/group02/male_06.mdl",
	"models/humans/group02/female_06.mdl",
	"models/humans/group02/male_07.mdl",
	"models/humans/group02/female_07.mdl",
	"models/humans/group02/male_08.mdl",
	"models/humans/group02/male_09.mdl",
	"models/male_lawyer.mdl",
	"models/male_pi.mdl",
};

const char *g_ppszRandomCombineModels[] =
{
	"models/combine_soldier.mdl",
//	"models/combine_soldier_prisonguard.mdl",
//	"models/combine_super_soldier.mdl",
	"models/police.mdl", //LAWYER:  Don't use them
	//TGB: We need the combine_soldier.mdl one because it's used (in this file) as default model when the selected one is not found
};


#define MAX_COMBINE_MODELS 4
#define MODEL_CHANGE_INTERVAL 5.0f
#define TEAM_CHANGE_INTERVAL 5.0f

#define HL2MPPLAYER_PHYSDAMAGE_SCALE 4.0f

#pragma warning( disable : 4355 )

CHL2MP_Player::CHL2MP_Player() : m_PlayerAnimState( this )
{
	m_angEyeAngles.Init();

	m_iLastWeaponFireUsercmd = 0;

	m_flNextModelChangeTime = 0.0f;
	m_flNextTeamChangeTime = 0.0f;

	m_iSpawnInterpCounter = 0;

	m_bObserve = false; //LAWYER:  Observer flag!
	m_bCameraMode = false; //LAWYER:  Camera type
	m_bDontWannaZM = false; //LAWYER:  Don't want to play as ZM flag!!
	m_iWeaponFlags = 0; //LAWYER:  We most likely start without weapons.
//	UseClientSideAnimation(); 
}

CHL2MP_Player::~CHL2MP_Player( void )
{

}

void CHL2MP_Player::UpdateOnRemove( void )
{
	if ( m_hRagdoll )
	{
		UTIL_RemoveImmediate( m_hRagdoll );
		m_hRagdoll = NULL;
	}

	BaseClass::UpdateOnRemove();
}

void CHL2MP_Player::Precache( void )
{
	BaseClass::Precache();

	PrecacheModel ( "sprites/glow01.vmt" );

	//Precache Citizen models
	int nHeads = ARRAYSIZE( g_ppszRandomCitizenModels );
	int i;	

	for ( i = 0; i < nHeads; ++i )
	   	 PrecacheModel( g_ppszRandomCitizenModels[i] );

	//Precache Combine Models
	nHeads = ARRAYSIZE( g_ppszRandomCombineModels );

	for ( i = 0; i < nHeads; ++i )
	   	 PrecacheModel( g_ppszRandomCombineModels[i] );

	PrecacheFootStepSounds();

	PrecacheScriptSound( "NPC_MetroPolice.Die" );
	PrecacheScriptSound( "NPC_CombineS.Die" );
	PrecacheScriptSound( "NPC_Citizen.die" );

	//LAWYER:  Extra precaches
	UTIL_PrecacheOther( "npc_zombie" );
	UTIL_PrecacheOther( "npc_fastzombie" );
	UTIL_PrecacheOther( "npc_poisonzombie" );
	UTIL_PrecacheOther( "npc_dragzombie" );
	UTIL_PrecacheOther( "npc_burnzombie" );
}

void CHL2MP_Player::GiveAllItems( void )
{
	EquipSuit();

	CBasePlayer::GiveAmmo( 255,	"Pistol");
	CBasePlayer::GiveAmmo( 255,	"AR2" );
	CBasePlayer::GiveAmmo( 5,	"AR2AltFire" );
	CBasePlayer::GiveAmmo( 255,	"SMG1");
	CBasePlayer::GiveAmmo( 1,	"smg1_grenade");
	CBasePlayer::GiveAmmo( 255,	"Buckshot");
	CBasePlayer::GiveAmmo( 32,	"357" );
	CBasePlayer::GiveAmmo( 3,	"rpg_round");

	CBasePlayer::GiveAmmo( 1,	"grenade" );
	CBasePlayer::GiveAmmo( 2,	"slam" );

	GiveNamedItem( "weapon_crowbar" );
	GiveNamedItem( "weapon_stunstick" );
	GiveNamedItem( "weapon_pistol" );
	GiveNamedItem( "weapon_357" );

	GiveNamedItem( "weapon_smg1" );
	GiveNamedItem( "weapon_ar2" );
	
	GiveNamedItem( "weapon_shotgun" );
	GiveNamedItem( "weapon_frag" );
	
	GiveNamedItem( "weapon_crossbow" );
	
	GiveNamedItem( "weapon_rpg" );

	GiveNamedItem( "weapon_slam" );
	
}

void CHL2MP_Player::GiveDefaultItems( void )
{

	EquipSuit();

	//TGB: fists here make it crash
	//GiveNamedItem("weapon_zm_fists"); // qck edit- see if this works for handing out fists
	//TGB: give carry function

	if (IsSurvivor())
	{
		//TGB: moved to more direct way of equipping the weapons
		CBaseCombatWeapon *carry = Weapon_Create("weapon_zm_carry");
		if (carry)
			Weapon_Equip(carry);

		CBaseCombatWeapon *fists = Weapon_Create("weapon_zm_fists");
		if (fists)
		{
			Weapon_Equip(fists);
			Weapon_Switch(fists);
		}

		//GiveNamedItem("weapon_zm_carry");
		//GiveNamedItem("weapon_zm_fists"); //LAWYER:  Upper Fisting!
		//Weapon_Switch( Weapon_OwnsThisType( "weapon_zm_fists" ) ); //LAWYER:  Switch to fists on spawn
	}
}

void CHL2MP_Player::PickDefaultSpawnTeam( void )
{ 
	if (GetTeamNumber() == 0)
	{
		ChangeTeam( TEAM_SPECTATOR ); //LAWYER:  Force players onto the observer team.

		//TGB: we now already do this in FinishClientPutInServer (which calls this function indirectly)

		//LAWYER:  We need to test for a roundrestart here.
		//Find if we have a Zombie Master
		/*
		bool bHaveZM = false;
		bool bHaveHumans = false;

		int iCount = 0;
		for (int i = 1; i <= gpGlobals->maxClients; i++ ) //Look for a ZM and count the players
		{
			CBasePlayer *pIterated = UTIL_PlayerByIndex( i );

			if ( pIterated )
			{
				iCount++;
				if (pIterated->GetTeamNumber() == 3)
				{
					bHaveZM = true;
				}
				if (pIterated->GetTeamNumber() == 2)
				{
					bHaveHumans = true;
				}
			}
		}
		
		if (iCount > 1 && (bHaveZM == false || bHaveHumans == false))
		{
			//We have more than one player waiting, and there's no ZM...
			g_pGameRules->FinishingRound();

		}
		*/
	}

}

//-----------------------------------------------------------------------------
// Purpose: Sets HL2 specific defaults.
//-----------------------------------------------------------------------------
void CHL2MP_Player::Spawn(void)
{
	//qck: Start out with no screen space effects at all
	m_ScreenEffect = AllocPooledString( "none" );

	m_flNextModelChangeTime = 0.0f;
	m_flNextTeamChangeTime = 0.0f;

	PickDefaultSpawnTeam();

	BaseClass::Spawn();

	pl.deadflag = false;
	RemoveSolidFlags( FSOLID_NOT_SOLID );

	RemoveEffects( EF_NODRAW );
	StopObserverMode();
	
	GiveDefaultItems();

	RemoveEffects( EF_NOINTERP );

	SetNumAnimOverlays( 3 );
	ResetAnimation();

	m_nRenderFX = kRenderNormal;

	m_Local.m_iHideHUD = 0;
	
	AddFlag(FL_ONGROUND); // set the player on the ground at the start of the round.

	m_impactEnergyScale = HL2MPPLAYER_PHYSDAMAGE_SCALE;

	if ( HL2MPRules()->IsIntermission() )
	{
		AddFlag( FL_FROZEN );
	}
	else
	{
		RemoveFlag( FL_FROZEN );
	}

	m_iSpawnInterpCounter = (m_iSpawnInterpCounter + 1) % 8;

	m_Local.m_bDucked = false;

	SetPlayerUnderwater(false);
//LAWYER:  Extra code for Observer mode.  This puts you into roaming mode.
	if (GetTeamNumber() != TEAM_SPECTATOR)
	{
		StopObserverMode();
	}
	else if( !IsBot() )
	{
		StartObserverMode( OBS_MODE_ROAMING);
	}

	if (GetTeamNumber() == 3 || GetTeamNumber() == TEAM_SPECTATOR)
	{
		//if ( this )
		//{
		SetCollisionBounds( VEC_OBS_HULL_MIN, VEC_OBS_HULL_MAX); //qck: Zombie master should be like a tiny floating box, rather than a full player.
		//LAWYER: the actual stripping component
		for ( int i = 0 ; i < WeaponCount(); ++i )
		{

			CBaseCombatWeapon *pWeapon = GetWeapon( i );
			if ( !pWeapon )
				continue;

			Weapon_Drop( pWeapon, (0,0,0), (0,0,0) );
			UTIL_Remove( pWeapon );
		}
		//}

		//TGB: the below causes 0000450 where ZM's spawnpoint angles don't carry over to ZM player
		//	it looks like old code to me for when we didn't have info_player_zombiemaster spawns
		/*
		//LAWYER:  Set some angles!
		QAngle oldang = this->GetAbsAngles(); 

		QAngle newang;
		newang.x = 45;
		newang.y = 0;
		newang.z = oldang.z;

		this->SnapEyeAngles( newang );
		*/
	}

	//TGB: set up flashlight
	m_flFlashlightBattery = ZM_FLASHLIGHT_START_AMOUNT;


	//TGB: execute zm.cfg or survivor.cfg
	const char *cfgfile = ( IsZM() ) ? "zm.cfg" : "survivor.cfg";
	char szCommand[256];
	DevMsg( "Executing config file %s with ZM/survivor-specific settings\n", cfgfile );
	Q_snprintf( szCommand, sizeof(szCommand), "exec %s\n", cfgfile );
	//engine->ClientCmd( szCommand );
	//ClientCommand(szCommand);
	engine->ClientCommand ( this->edict(), szCommand );
	
	m_iWeaponFlags = 0; //TGB: reset weapon flags
	
	//LAWYER: Kick on null string should go here
	const char *pName = this->GetPlayerName();
	if (pName[0] == '\0')
	{
		Msg("Client with invalid name detected! Kicking...\n");
		engine->ServerCommand(UTIL_VarArgs( "kick %s", this->GetNetworkIDString()));
	}
	
}
extern ConVar physcannon_maxmass;
void CHL2MP_Player::PickupObject( CBaseEntity *pObject, bool bLimitMassAndSize )
{
	if (this->GetTeamNumber() != 3)
	{
		// can't pick up what you're standing on 
		if ( GetGroundEntity() == pObject )
			return;

		DevMsg("Picked something up\n");
	    
		if ( bLimitMassAndSize == true )
		{	//TGB: the sizes here need to mirror with those in CWeaponCarry::CanPickupObject
			if ( CBasePlayer::CanPickupObject( pObject, physcannon_maxmass.GetFloat(), 200 ) == false )//if the objects is within the mass limit 35, and size limit 128 
				return;
		}

		// Can't be picked up if NPCs are on me 
		if ( pObject->HasNPCsOnIt() )
			return;

		PlayerPickupObject( this, pObject );
	}    

}

bool CHL2MP_Player::StartObserverMode( int mode )
{
	return BaseClass::StartObserverMode(mode); //LAWYER:  Start observer mode in the base class.
	//Do nothing.

//	return false;
}

bool CHL2MP_Player::ValidatePlayerModel( const char *pModel )
{
	int iModels = ARRAYSIZE( g_ppszRandomCitizenModels );
	int i;	

	for ( i = 0; i < iModels; ++i )
	{
		if ( !Q_stricmp( g_ppszRandomCitizenModels[i], pModel ) )
		{
			return true;
		}
	}

	//TGB HACK: we don't want combines in ZM mp
	/*
	iModels = ARRAYSIZE( g_ppszRandomCombineModels );

	for ( i = 0; i < iModels; ++i )
	{
	   	if ( !Q_stricmp( g_ppszRandomCombineModels[i], pModel ) )
		{
			return true; 
		}
	}
	*/

	return false;
}

void CHL2MP_Player::SetPlayerTeamModel( void ) //LAWYER:  This needs to be changed - no model forcing
{
	const char *szModelName = NULL;
	szModelName = engine->GetClientConVarValue( engine->IndexOfEdict( edict() ), "cl_playermodel" );

	int modelIndex = modelinfo->GetModelIndex( szModelName );

	if ( modelIndex == -1 || ValidatePlayerModel( szModelName ) == false )
	{
		//szModelName = "models/Combine_Soldier.mdl";
		szModelName = "models/humans/group02/male_01.mdl"; //TGB: no mo' combin-o
		m_iModelType = TEAM_HUMANS;

		char szReturnString[512];

		Q_snprintf( szReturnString, sizeof (szReturnString ), "cl_playermodel %s\n", szModelName );
		engine->ClientCommand ( edict(), szReturnString );
	}
/*
	if ( GetTeamNumber() == TEAM_HUMANS )
	{
		if ( Q_stristr( szModelName, "models/human") )
		{
			int nHeads = ARRAYSIZE( g_ppszRandomCombineModels );
		
			g_iLastCombineModel = ( g_iLastCombineModel + 1 ) % nHeads;
			szModelName = g_ppszRandomCombineModels[g_iLastCombineModel];
		}

		m_iModelType = TEAM_HUMANS;
	}
	else if ( GetTeamNumber() == TEAM_ZOMBIEMASTER )
	{
		if ( !Q_stristr( szModelName, "models/human") )
		{
			int nHeads = ARRAYSIZE( g_ppszRandomCitizenModels );

			g_iLastCitizenModel = ( g_iLastCitizenModel + 1 ) % nHeads;
			szModelName = g_ppszRandomCitizenModels[g_iLastCitizenModel];
		}

		m_iModelType = TEAM_ZOMBIEMASTER;
	}
*/ //LAWYER: No team forcing!
	SetModel( szModelName );
	SetupPlayerSoundsByModel( szModelName );

	m_flNextModelChangeTime = gpGlobals->curtime + MODEL_CHANGE_INTERVAL;
}

void CHL2MP_Player::SetPlayerModel( void ) //LAWYER:  Change this, too
{
	const char *szModelName = NULL;
	const char *pszCurrentModelName = modelinfo->GetModelName( GetModel());

	szModelName = engine->GetClientConVarValue( engine->IndexOfEdict( edict() ), "cl_playermodel" );

	if ( ValidatePlayerModel( szModelName ) == false )
	{
		char szReturnString[512];

		if ( ValidatePlayerModel( pszCurrentModelName ) == false )
		{
			//pszCurrentModelName = "models/Combine_Soldier.mdl"; //Set to default model
			pszCurrentModelName = "models/humans/group02/male_01.mdl"; //TGB: no combine
		}

		Q_snprintf( szReturnString, sizeof (szReturnString ), "cl_playermodel %s\n", pszCurrentModelName );
		engine->ClientCommand ( edict(), szReturnString );

		szModelName = pszCurrentModelName;
	}

/*	if ( GetTeamNumber() == TEAM_HUMANS )
	{
		int nHeads = ARRAYSIZE( g_ppszRandomCombineModels );
		
		g_iLastCombineModel = ( g_iLastCombineModel + 1 ) % nHeads;
		szModelName = g_ppszRandomCombineModels[g_iLastCombineModel];

		m_iModelType = TEAM_HUMANS;
	}
	else if ( GetTeamNumber() == TEAM_ZOMBIEMASTER )
	{
		int nHeads = ARRAYSIZE( g_ppszRandomCitizenModels );

		g_iLastCitizenModel = ( g_iLastCitizenModel + 1 ) % nHeads;
		szModelName = g_ppszRandomCitizenModels[g_iLastCitizenModel];

		m_iModelType = TEAM_ZOMBIEMASTER;
	}
	else
	{
		if ( Q_strlen( szModelName ) == 0 ) 
		{
			szModelName = g_ppszRandomCitizenModels[0];
		}

		if ( Q_stristr( szModelName, "models/human") )
		{
			m_iModelType = TEAM_ZOMBIEMASTER;
		}
		else
		{
			m_iModelType = TEAM_HUMANS;
		}
	}
*/ //LAWYER:  Teams aren't defined by model anymore!
	int modelIndex = modelinfo->GetModelIndex( szModelName );

	if ( modelIndex == -1 )
	{
		//szModelName = "models/Combine_Soldier.mdl";
		szModelName = "models/humans/group02/male_01.mdl"; //TGB: no combine
		m_iModelType = TEAM_HUMANS;

		char szReturnString[512];

		Q_snprintf( szReturnString, sizeof (szReturnString ), "cl_playermodel %s\n", szModelName );
		engine->ClientCommand ( edict(), szReturnString );
	}

	SetModel( szModelName );
	SetupPlayerSoundsByModel( szModelName );

	m_flNextModelChangeTime = gpGlobals->curtime + MODEL_CHANGE_INTERVAL;
}

void CHL2MP_Player::SetupPlayerSoundsByModel( const char *pModelName )
{
	if ( Q_stristr( pModelName, "models/human") )
	{
		m_iPlayerSoundType = (int)PLAYER_SOUNDS_CITIZEN;
	}
	else if ( Q_stristr(pModelName, "police" ) )
	{
		m_iPlayerSoundType = (int)PLAYER_SOUNDS_METROPOLICE;
	}
	else if ( Q_stristr(pModelName, "combine" ) )
	{
		m_iPlayerSoundType = (int)PLAYER_SOUNDS_COMBINESOLDIER;
	}
	//TGB: always fall back on citizensounds, as all ZM humans are citizens
	else //if ( Q_stristr(pModelName, "lawyer" ) )
	{
		m_iPlayerSoundType = (int)PLAYER_SOUNDS_CITIZEN;
	}

}

void CHL2MP_Player::ResetAnimation( void )
{
	if ( IsAlive() )
	{
		SetSequence ( -1 );
		SetActivity( ACT_INVALID );

		if (!GetAbsVelocity().x && !GetAbsVelocity().y)
			SetAnimation( PLAYER_IDLE );
		else if ((GetAbsVelocity().x || GetAbsVelocity().y) && ( GetFlags() & FL_ONGROUND ))
			SetAnimation( PLAYER_WALK );
		else if (GetWaterLevel() > 1)
			SetAnimation( PLAYER_WALK );
	}
}


bool CHL2MP_Player::Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex )
{
	bool bRet = BaseClass::Weapon_Switch( pWeapon, viewmodelindex );

	if ( bRet == true )
	{
		ResetAnimation();
	}

	return bRet;
}

void CHL2MP_Player::PreThink( void )
{
	QAngle vOldAngles = GetLocalAngles();
	QAngle vTempAngles = GetLocalAngles();

	vTempAngles = EyeAngles();

	if ( vTempAngles[PITCH] > 180.0f )
	{
		vTempAngles[PITCH] -= 360.0f;
	}

	SetLocalAngles( vTempAngles );

	//TGB: are we trying to stand on something we can't stand on, like zombies?
	if (GetGroundEntity() != NULL)
	{
		//is it a nonstandable npc?
		if (!GetGroundEntity()->IsStandable() && GetGroundEntity()->IsNPC())
		{
			Vector vecDirection;
			
			//the direction doesn't really matter, so I ripped this from the stunstick
			AngleVectors( GetAbsAngles(), &vecDirection );
			VectorNormalize(vecDirection);
			
			//push the player
			this->ApplyAbsVelocityImpulse(33.f * vecDirection);
			
			//we'll be falling
			this->SetGroundEntity(NULL);
		}
	}
	
	//TGB: flashlight update
	if (FlashlightIsOn())
	{
		m_flFlashlightBattery.Set(m_flFlashlightBattery - zm_flashlight_drainrate.GetFloat() * gpGlobals->frametime);

		/*
		const int temp = (int)m_flFlashlightBattery.Get();
		if ((temp % 5) == 0)
		{
			DevMsg("Flashlight at %f\n", m_flFlashlightBattery.Get());
		}
		*/

		if (m_flFlashlightBattery <= 0.0f)
		{
			FlashlightTurnOff(); //out of juice
			m_flFlashlightBattery.Set(0);

			ClientPrint(this, HUD_PRINTTALK, "Your flashlight battery has run out!\n");
		}
			        
	}
	
	BaseClass::PreThink();
	State_PreThink();

	//Reset bullet force accumulator, only lasts one frame
	m_vecTotalBulletForce = vec3_origin;
	SetLocalAngles( vOldAngles );
}

void CHL2MP_Player::PostThink( void )
{
	BaseClass::PostThink();
	
	if ( GetFlags() & FL_DUCKING )
	{
		SetCollisionBounds( VEC_CROUCH_TRACE_MIN, VEC_CROUCH_TRACE_MAX );
	}
	//TGB: we're not using ZM's experimental colliding mode, making this wasteful
	//qck: For the tenth time, zm players should be tiny
// 	if ( GetTeamNumber() == 3)
// 	{
// 		SetCollisionBounds( VEC_OBS_HULL_MIN, VEC_OBS_HULL_MAX);
// 	}

	m_PlayerAnimState.Update();

	// Store the eye angles pitch so the client can compute its animation state correctly.
	m_angEyeAngles = EyeAngles();

	QAngle angles = GetLocalAngles();
	angles[PITCH] = 0;
	SetLocalAngles( angles );
}

void CHL2MP_Player::PlayerDeathThink()
{
	if( !IsObserver() )
	{
		BaseClass::PlayerDeathThink();
	}
}

void CHL2MP_Player::FireBullets ( const FireBulletsInfo_t &info )
{
	// Move other players back to history positions based on local player's lag
	lagcompensation->StartLagCompensation( this, this->GetCurrentCommand() );

	FireBulletsInfo_t modinfo = info;

	CWeaponHL2MPBase *pWeapon = dynamic_cast<CWeaponHL2MPBase *>( GetActiveWeapon() );

	if ( pWeapon )
	{
		modinfo.m_iPlayerDamage = modinfo.m_iDamage = pWeapon->GetHL2MPWpnData().m_iPlayerDamage;
	}

	NoteWeaponFired();

	BaseClass::FireBullets( modinfo );

	// Move other players back to history positions based on local player's lag
	lagcompensation->FinishLagCompensation( this );
}

void CHL2MP_Player::NoteWeaponFired( void )
{
	Assert( m_pCurrentCommand );
	if( m_pCurrentCommand )
	{
		m_iLastWeaponFireUsercmd = m_pCurrentCommand->command_number;
	}
}

extern ConVar sv_maxunlag;

bool CHL2MP_Player::WantsLagCompensationOnEntity( const CBaseEntity *pEntity, const CUserCmd *pCmd, const CBitVec<MAX_EDICTS> *pEntityTransmitBits ) const
{
	// No need to lag compensate at all if we're not attacking in this command and
	// we haven't attacked recently.
	if ( !( pCmd->buttons & IN_ATTACK ) && (pCmd->command_number - m_iLastWeaponFireUsercmd > 5) )
		return false;

	return BaseClass::WantsLagCompensationOnEntity(pEntity,pCmd,pEntityTransmitBits);
}


Activity CHL2MP_Player::TranslateTeamActivity( Activity ActToTranslate )
{
	if ( m_iModelType == TEAM_HUMANS )
		 return ActToTranslate;
	
	if ( ActToTranslate == ACT_RUN )
		 return ACT_RUN_AIM_AGITATED;

	if ( ActToTranslate == ACT_IDLE )
		 return ACT_IDLE_AIM_AGITATED;

	if ( ActToTranslate == ACT_WALK )
		 return ACT_WALK_AIM_AGITATED;

	return ActToTranslate;
}

extern ConVar hl2_normspeed;

// Set the activity based on an event or current state
void CHL2MP_Player::SetAnimation( PLAYER_ANIM playerAnim )
{
	int animDesired;

	float speed;

	speed = GetAbsVelocity().Length2D();

	
	// bool bRunning = true;

	//Revisit!
/*	if ( ( m_nButtons & ( IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT ) ) )
	{
		if ( speed > 1.0f && speed < hl2_normspeed.GetFloat() - 20.0f )
		{
			bRunning = false;
		}
	}*/

	if ( GetFlags() & ( FL_FROZEN | FL_ATCONTROLS ) )
	{
		speed = 0;
		playerAnim = PLAYER_IDLE;
	}

	Activity idealActivity = ACT_HL2MP_RUN;

	// This could stand to be redone. Why is playerAnim abstracted from activity? (sjb)
	if ( playerAnim == PLAYER_JUMP )
	{
		idealActivity = ACT_HL2MP_JUMP;
	}
	else if ( playerAnim == PLAYER_DIE )
	{
		if ( m_lifeState == LIFE_ALIVE )
		{
			return;
		}
	}
	else if ( playerAnim == PLAYER_ATTACK1 )
	{
		if ( GetActivity( ) == ACT_HOVER	|| 
			 GetActivity( ) == ACT_SWIM		||
			 GetActivity( ) == ACT_HOP		||
			 GetActivity( ) == ACT_LEAP		||
			 GetActivity( ) == ACT_DIESIMPLE )
		{
			idealActivity = GetActivity( );
		}
		else
		{
			idealActivity = ACT_HL2MP_GESTURE_RANGE_ATTACK;
		}
	}
	else if ( playerAnim == PLAYER_RELOAD )
	{
		idealActivity = ACT_HL2MP_GESTURE_RELOAD;
	}
	else if ( playerAnim == PLAYER_IDLE || playerAnim == PLAYER_WALK )
	{
		if ( !( GetFlags() & FL_ONGROUND ) && GetActivity( ) == ACT_HL2MP_JUMP )	// Still jumping
		{
			idealActivity = GetActivity( );
		}
		/*
		else if ( GetWaterLevel() > 1 )
		{
			if ( speed == 0 )
				idealActivity = ACT_HOVER;
			else
				idealActivity = ACT_SWIM;
		}
		*/
		else
		{
			if ( GetFlags() & FL_DUCKING )
			{
				if ( speed > 0 )
				{
					idealActivity = ACT_HL2MP_WALK_CROUCH;
				}
				else
				{
					idealActivity = ACT_HL2MP_IDLE_CROUCH;
				}
			}
			else
			{
				if ( speed > 0 )
				{
					/*
					if ( bRunning == false )
					{
						idealActivity = ACT_WALK;
					}
					else
					*/
					{
						idealActivity = ACT_HL2MP_RUN;
					}
				}
				else
				{
					idealActivity = ACT_HL2MP_IDLE;
				}
			}
		}

		idealActivity = TranslateTeamActivity( idealActivity );
	}
	
	if ( idealActivity == ACT_HL2MP_GESTURE_RANGE_ATTACK )
	{
		RestartGesture( Weapon_TranslateActivity( idealActivity ) );

		// FIXME: this seems a bit wacked
		Weapon_SetActivity( Weapon_TranslateActivity( ACT_RANGE_ATTACK1 ), 0 );

		return;
	}
	else if ( idealActivity == ACT_HL2MP_GESTURE_RELOAD )
	{
		RestartGesture( Weapon_TranslateActivity( idealActivity ) );
		return;
	}
	else
	{
		SetActivity( idealActivity );

		animDesired = SelectWeightedSequence( Weapon_TranslateActivity ( idealActivity ) );

		if (animDesired == -1)
		{
			animDesired = SelectWeightedSequence( idealActivity );

			if ( animDesired == -1 )
			{
				animDesired = 0;
			}
		}
	
		// Already using the desired animation?
		if ( GetSequence() == animDesired )
			return;

		m_flPlaybackRate = 1.0;
		ResetSequence( animDesired );
		SetCycle( 0 );
		return;
	}

	// Already using the desired animation?
	if ( GetSequence() == animDesired )
		return;

	//Msg( "Set animation to %d\n", animDesired );
	// Reset to first frame of desired animation
	ResetSequence( animDesired );
	SetCycle( 0 );
}


extern int	gEvilImpulse101;

//TGB: weapon-ignoring tracefilter
class CTraceFilterWeapons : public CTraceFilterSkipTwoEntities
{
public:
	CTraceFilterWeapons( const IHandleEntity *passentity1, const IHandleEntity *passentity2, int collisionGroup )
		: CTraceFilterSkipTwoEntities( passentity1, passentity2, collisionGroup )
	{
	}

	bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask ) {
		if ( CTraceFilterSkipTwoEntities::ShouldHitEntity(pServerEntity, contentsMask) )
		{
			if (contentsMask == COLLISION_GROUP_WEAPON)
				return false; //ignore weapons
		}
		//if baseclass's shouldhit is false, we should be false too
		return false;
	}
};

//-----------------------------------------------------------------------------
// Purpose: Player reacts to bumping a weapon. 
// Input  : pWeapon - the weapon that the player bumped into.
// Output : Returns true if player picked up the weapon
//-----------------------------------------------------------------------------
bool CHL2MP_Player::BumpWeapon( CBaseCombatWeapon *pWeapon )
{	//LAWYER:  This is where we define if you can get a weapon
	CBaseCombatCharacter *pOwner = pWeapon->GetOwner();
	if (IsZM() || IsObserver() || !pWeapon)
	{
		return false; //LAWYER:  Zombiemasters don't get weapons!
	}
	
	// Can I have this weapon type?
	if ( IsEFlagSet( EFL_NO_WEAPON_PICKUP ) )
		return false;

	// qck: Don't let the player fetch weapons while they are holding objects
	if ( m_bHoldingObject == true )
		return false;

	// Don't let the player fetch weapons through walls (use MASK_SOLID so that you can't pickup through windows)
	//TGB: this detected visible weapons as invisible, causing pickup probs, we'll do our own check instead
	//	if( !pWeapon->FVisible( this, MASK_SOLID ) && !(GetFlags() & FL_NOTARGET) )
	//	{
	//		return false;
	//	}

	/*
	//TGB: do our own vis check
	
	trace_t tr;
	// Use the custom trace filter
	CTraceFilterWeapons traceFilter( pWeapon, COLLISION_GROUP_NONE );

	UTIL_TraceLine( pWeapon->GetBumpTracePos(), EyePosition(), MASK_SOLID, &traceFilter, &tr );

	if ((tr.fraction != 1.0 || tr.startsolid)  && tr.m_pEnt)
	{
		if ( tr.m_pEnt != this )
		{
			return false;
		}
	}
	*/

	//TGB: rewrite of pickup vis check number 394273482, this time largely inspired by CItem
	Vector vecStartPos;
	IPhysicsObject *pPhysObj = pWeapon->VPhysicsGetObject();
	if ( pPhysObj != NULL )
	{
		// Use the physics hull's center
		QAngle vecAngles;
		pPhysObj->GetPosition( &vecStartPos, &vecAngles );
	}
	else if ( pWeapon->CollisionProp() != NULL )
	{
		// Use the generic bbox center
		vecStartPos = pWeapon->CollisionProp()->WorldSpaceCenter();
	}
	else
		vecStartPos = pWeapon->EyePosition();

	// Trace between to see if we're occluded
	trace_t tr;
	CTraceFilterWeapons filter( this, pWeapon, COLLISION_GROUP_NONE );
	UTIL_TraceLine( vecStartPos, EyePosition(), MASK_SOLID, &filter, &tr );

	// if the ray did not complete, weapon is occluded
	if ( tr.fraction < 1.0f )
		return false;

	//LAWYER: Do I already have a weapon of this type?
	
	//TGB: to be honest, I don't know why this is done with flags. Not networked or anything. Array of bools is easier.

	//-----Weapon Limiter Code-----
	int iCalculation = this->m_iWeaponFlags;

	//Grenades
	if (iCalculation - 8 >= 0)
	{
		//Already have grenades
		iCalculation -= 8;
		if (pWeapon->GetWeaponTypeNumber() == 8)
		{
			return false; //No grenades for you!
		}
	}
	//Rifles
	if (iCalculation - 4 >= 0)
	{
		iCalculation -= 4;
		if (pWeapon->GetWeaponTypeNumber() == 4)
		{
			return false; //In Soviet Russia, rifle picks up YOU!
		}
	}
	//Pistols
	if (iCalculation - 2 >= 0)
	{
		iCalculation -= 2;
		if (pWeapon->GetWeaponTypeNumber() == 2)
		{
			return false; //This is a pistol-free zone!
		}
	}
	//CCs
	if (iCalculation - 1 >= 0)
	{
		iCalculation -= 1;
		if (pWeapon->GetWeaponTypeNumber() == 1)
		{
			return false; //This offer is open to ONE per household.
		}
	}

				
	
	if ( pOwner || !Weapon_CanUse( pWeapon ) || !g_pGameRules->CanHavePlayerItem( this, pWeapon ) )
	{
		if ( gEvilImpulse101 )
		{
			UTIL_Remove( pWeapon );
		}
		return false;
	}


	bool bOwnsWeaponAlready = !!Weapon_OwnsThisType( pWeapon->GetClassname(), pWeapon->GetSubType());

	if ( bOwnsWeaponAlready == true ) 
	{
		//If we have room for the ammo, then "take" the weapon too.
		 if ( Weapon_EquipAmmoOnly( pWeapon ) )
		 {
			 pWeapon->CheckRespawn();

			 UTIL_Remove( pWeapon );
			 return true;
		 }
		 else
		 {
			 return false;
		 }
	}


	pWeapon->CheckRespawn();
	Weapon_Equip( pWeapon );
	
	this->m_iWeaponFlags += pWeapon->GetWeaponTypeNumber(); //LAWYER:  Add this weapon to our weaponflags
	return true;
}

void CHL2MP_Player::ChangeTeam( int iTeam ) //LAWYER:  The important part of team changes
{	//LAWYER:  We need checks to see how many players are on the Zombie Master team.  There should only be 1.

	//TGB: UNDONE REWRITE, we need a killing changeteam for force* commands, and for switching peeps to spectator

	bool bKill = false;
	if ( HL2MPRules()->IsTeamplay() == true )
	{
		if ( iTeam != GetTeamNumber() && GetTeamNumber() != TEAM_UNASSIGNED )
		{
			bKill = true;
		}
	}
	BaseClass::ChangeTeam( iTeam );

	//m_flNextTeamChangeTime = gpGlobals->curtime + TEAM_CHANGE_INTERVAL;

	SetPlayerModel();

	if ( bKill == true )
	{
		//LAWYER:  This is odd, but it's worth a shot
		Spawn();
		//ClientKill( edict() );
	}
}

//TGB: ZM uses this function in roundrestarts, where we only want to change the team the player will spawn into without already spawning him
void CHL2MP_Player::ChangeTeamDelayed( int iTeam )
{
	
	BaseClass::ChangeTeam( iTeam );
	//m_flNextTeamChangeTime = gpGlobals->curtime + TEAM_CHANGE_INTERVAL;

	SetPlayerModel();
}


bool CHL2MP_Player::ClientCommand( const char *cmd )
{
	if ( FStrEq( cmd, "spectate" ) )
	{
		// do nothing.
		return true;
	}

	return BaseClass::ClientCommand( cmd );
}

void CHL2MP_Player::CheatImpulseCommands( int iImpulse )
{
	if( sv_cheats->GetBool() == false)
		return;

//	CBaseEntity *pEntity;
	trace_t tr;
	switch ( iImpulse )
	{
		case 101:
		{
				Msg(":shh:\n");
				//TGB: these are all hl2 weps anyway, real men have a custom script for this
				//GiveAllItems();

		}
		break;
/*		case	666://LAWYER:  Test the firesystem
		{
			Vector forward;
			EyeVectors( &forward );
			UTIL_TraceLine ( EyePosition(), 
				EyePosition() + forward * 128, 
				MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, & tr);

			if ( tr.fraction != 1.0 )
			{// line hit something, so start a fire
				FireSystem_StartFire( tr.endpos, 1, 1, 30.0f, (SF_FIRE_START_ON|SF_FIRE_SMOKELESS|SF_FIRE_NO_GLOW), (CBaseEntity*) this, FIRE_NATURAL );
			}
		}*/
		default:
			BaseClass::CheatImpulseCommands( iImpulse );
	}
}

bool CHL2MP_Player::ShouldRunRateLimitedCommand( const char *pcmd )
{
	int i = m_RateLimitLastCommandTimes.Find( pcmd );
	if ( i == m_RateLimitLastCommandTimes.InvalidIndex() )
	{
		m_RateLimitLastCommandTimes.Insert( pcmd, gpGlobals->curtime );
		return true;
	}
	else if ( (gpGlobals->curtime - m_RateLimitLastCommandTimes[i]) < HL2MP_COMMAND_MAX_RATE )
	{
		// Too fast.
		return false;
	}
	else
	{
		m_RateLimitLastCommandTimes[i] = gpGlobals->curtime;
		return true;
	}
}

void CHL2MP_Player::CreateViewModel( int index /*=0*/ )
{
	Assert( index >= 0 && index < MAX_VIEWMODELS );

	if ( GetViewModel( index ) )
		return;

	//TGBMERGENOTE: predicted viewmodels were for some reason taken out of the new sdk
	//TGB UNDONE: caused by projfile mistake
	CPredictedViewModel *vm = ( CPredictedViewModel * )CreateEntityByName( "predicted_viewmodel" );
	//CBaseViewModel *vm = ( CBaseViewModel * )CreateEntityByName( "viewmodel" );
	if ( vm )
	{
		vm->SetAbsOrigin( GetAbsOrigin() );
		vm->SetOwner( this );
		vm->SetIndex( index );
		DispatchSpawn( vm );
		vm->FollowEntity( this, false );
		m_hViewModel.Set( index, vm );
	}
}

bool CHL2MP_Player::BecomeRagdollOnClient( const Vector &force )
{
	return true;
}

// -------------------------------------------------------------------------------- //
// Ragdoll entities.
// -------------------------------------------------------------------------------- //

class CHL2MPRagdoll : public CBaseAnimatingOverlay
{
public:
	DECLARE_CLASS( CHL2MPRagdoll, CBaseAnimatingOverlay );
	DECLARE_SERVERCLASS();

	// Transmit ragdolls to everyone.
	virtual int UpdateTransmitState()
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}

public:
	// In case the client has the player entity, we transmit the player index.
	// In case the client doesn't have it, we transmit the player's model index, origin, and angles
	// so they can create a ragdoll in the right place.
	CNetworkHandle( CBaseEntity, m_hPlayer );	// networked entity handle 
	CNetworkVector( m_vecRagdollVelocity );
	CNetworkVector( m_vecRagdollOrigin );
};

LINK_ENTITY_TO_CLASS( hl2mp_ragdoll, CHL2MPRagdoll );

IMPLEMENT_SERVERCLASS_ST_NOBASE( CHL2MPRagdoll, DT_HL2MPRagdoll )
	SendPropVector( SENDINFO(m_vecRagdollOrigin), -1,  SPROP_COORD ),
	SendPropEHandle( SENDINFO( m_hPlayer ) ),
	SendPropModelIndex( SENDINFO( m_nModelIndex ) ),
	SendPropInt		( SENDINFO(m_nForceBone), 8, 0 ),
	SendPropVector	( SENDINFO(m_vecForce), -1, SPROP_NOSCALE ),
	SendPropVector( SENDINFO( m_vecRagdollVelocity ) )
END_SEND_TABLE()


void CHL2MP_Player::CreateRagdollEntity( void )
{
	if ( m_hRagdoll )
	{
		UTIL_RemoveImmediate( m_hRagdoll );
		m_hRagdoll = NULL;
	}

	// If we already have a ragdoll, don't make another one.
	CHL2MPRagdoll *pRagdoll = dynamic_cast< CHL2MPRagdoll* >( m_hRagdoll.Get() );
	
	if ( !pRagdoll )
	{
		// create a new one
		pRagdoll = dynamic_cast< CHL2MPRagdoll* >( CreateEntityByName( "hl2mp_ragdoll" ) );
	}

	if ( pRagdoll )
	{
		pRagdoll->m_hPlayer = this;
		pRagdoll->m_vecRagdollOrigin = GetAbsOrigin();
		pRagdoll->m_vecRagdollVelocity = GetAbsVelocity();
		pRagdoll->m_nModelIndex = m_nModelIndex;
		pRagdoll->m_nForceBone = m_nForceBone;
		pRagdoll->m_vecForce = m_vecTotalBulletForce;
		pRagdoll->SetAbsOrigin( GetAbsOrigin() );
	}

	// ragdolls will be removed on round restart automatically
	m_hRagdoll = pRagdoll;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CHL2MP_Player::FlashlightIsOn( void )
{
	return IsEffectActive( EF_DIMLIGHT );
}

extern ConVar flashlight;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CHL2MP_Player::FlashlightTurnOn( void )
{
	if( flashlight.GetInt() > 0 && IsAlive()
		&& m_flFlashlightBattery > 0) //TGB: can't turn on without power
	{
		AddEffects( EF_DIMLIGHT );
		EmitSound( "HL2Player.FlashlightOn" );
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CHL2MP_Player::FlashlightTurnOff( void )
{
	RemoveEffects( EF_DIMLIGHT );
	
	if( IsAlive() )
	{
		EmitSound( "HL2Player.FlashlightOff" );
	}
}

void CHL2MP_Player::Weapon_Drop( CBaseCombatWeapon *pWeapon, const Vector *pvecTarget, const Vector *pVelocity )
{
	/*//Drop a grenade if it's primed.
	if ( GetActiveWeapon() )
	{
		CBaseCombatWeapon *pGrenade = Weapon_OwnsThisType("weapon_frag");

		if ( GetActiveWeapon() == pGrenade )
		{
			if ( ( m_nButtons & IN_ATTACK ) || (m_nButtons & IN_ATTACK2) )
			{
				DropPrimedFragGrenade( this, pGrenade );
				return;
			}
		}
	}
*/ //LAWYER:  We don't have grenades yet
	BaseClass::Weapon_Drop( pWeapon, pvecTarget, pVelocity );
}


void CHL2MP_Player::DetonateTripmines( void )
{
	CBaseEntity *pEntity = NULL;

	while ((pEntity = gEntList.FindEntityByClassname( pEntity, "npc_satchel" )) != NULL)
	{
		CSatchelCharge *pSatchel = dynamic_cast<CSatchelCharge *>(pEntity);
		if (pSatchel->m_bIsLive && pSatchel->GetThrower() == this )
		{
			g_EventQueue.AddEvent( pSatchel, "Explode", 0.20, this, this );
		}
	}

	// Play sound for pressing the detonator
	//EmitSound( "Weapon_SLAM.SatchelDetonate" );//TGB: potential cause of error about buttonclick.wav precaching
}

void CHL2MP_Player::Event_Killed( const CTakeDamageInfo &info )
{
	//update damage info with our accumulated physics force
	CTakeDamageInfo subinfo = info;
	subinfo.SetDamageForce( m_vecTotalBulletForce );

	SetNumAnimOverlays( 0 );

	// Note: since we're dead, it won't draw us on the client, but we don't set EF_NODRAW
	// because we still want to transmit to the clients in our PVS.
	if (GetTeamNumber() != TEAM_SPECTATOR && GetTeamNumber() != TEAM_ZOMBIEMASTER)
		CreateRagdollEntity(); //LAWYER:  Don't create ragdolls for Zombiemasters or Spectators

	DetonateTripmines();

	BaseClass::Event_Killed( subinfo );

	if ( info.GetDamageType() & DMG_DISSOLVE )
	{
		if ( m_hRagdoll )
		{
			m_hRagdoll->GetBaseAnimating()->Dissolve( NULL, gpGlobals->curtime, false, ENTITY_DISSOLVE_NORMAL );
		}
	}

	CBaseEntity *pAttacker = info.GetAttacker();

	if ( pAttacker )
	{
		int iScoreToAdd = 1;

		if ( pAttacker == this )
		{
			iScoreToAdd = -1;
		}

		GetGlobalTeam( pAttacker->GetTeamNumber() )->AddScore( iScoreToAdd );
	}

	FlashlightTurnOff();

	m_lifeState = LIFE_DEAD;

	RemoveEffects( EF_NODRAW );	// still draw player body
	StopZooming();
}

int CHL2MP_Player::OnTakeDamage( const CTakeDamageInfo &inputInfo )
{
	m_vecTotalBulletForce += inputInfo.GetDamageForce();
	
	return BaseClass::OnTakeDamage( inputInfo );
}

void CHL2MP_Player::DeathSound( const CTakeDamageInfo &info ) //LAWYER:  Fixme!
{
	if (GetTeamNumber() == TEAM_SPECTATOR || GetTeamNumber() == TEAM_ZOMBIEMASTER)
		return;

	if ( m_hRagdoll && m_hRagdoll->GetBaseAnimating()->IsDissolving() )
		 return;

	char szStepSound[128];

	Q_snprintf( szStepSound, sizeof( szStepSound ), "%s.Die", GetPlayerModelSoundPrefix() );

	const char *pModelName = STRING( GetModelName() );
	
	

	CSoundParameters params;
	if ( GetParametersForSound( szStepSound, params, pModelName ) == false )
		return;

	Vector vecOrigin = GetAbsOrigin();
	
	CRecipientFilter filter;
	filter.AddRecipientsByPAS( vecOrigin );

	EmitSound_t ep;
	ep.m_nChannel = params.channel;
	ep.m_pSoundName = params.soundname;
	ep.m_flVolume = params.volume;
	ep.m_SoundLevel = params.soundlevel;
	ep.m_nFlags = 0;
	ep.m_nPitch = params.pitch;
	ep.m_pOrigin = &vecOrigin;

	EmitSound( filter, entindex(), ep );
}

CBaseEntity* CHL2MP_Player::EntSelectSpawnPoint( void )
{
	/*TGB: 
		I found the likely cause of the game not using certain spawnpoints.
		However the HL2DM spawncode is an undebuggable mess meant for purposes 
		other than our own, involving far too much screwing around with 
		iteration to create randomness and whatnot. As such I'm recoding it. 
	*/

	CBaseEntity *pSpot = NULL;
	CBaseEntity *pChosenSpot = NULL;

	//TGB: ZM SPAWNING
	if (IsZM())
	{
		pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_zombiemaster" ); //LAWYER:  Zombiemasters always spawn here
		if ( pSpot  )
		{
			return pSpot; //we're done already
		}
		//if no ZM spawn is present, spawn the ZM as any other player
	}

	//TGB: SURVIVOR SPAWNING
	const char *pSpawnpointName = "info_player_deathmatch";
	CUtlVector<CBaseEntity *> spawn_list;
	spawn_list.Purge();


	//TGB: FIND SPAWNS
	
	//first, find all spawns on the map and dump them into a list
	do
	{
		pSpot = gEntList.FindEntityByClassname( pSpot, pSpawnpointName );
		if (!pSpot)
			break;

		//add randomness here by randomly adding to tail instead
		if (random->RandomInt( 0, 1) == 0)
			spawn_list.AddToHead( pSpot );
		else
			spawn_list.AddToTail( pSpot );

		//DevMsg("Found spawn at %f %f %f\n", pSpot->GetAbsOrigin().x, pSpot->GetAbsOrigin().y, pSpot->GetAbsOrigin().z);
	} while ( pSpot );

	DevMsg("SelectSpawn: found %i spawnpoints!\n", spawn_list.Count());

	if (spawn_list.Count() < 1)
	{
		Warning("No valid spawns found! Place info_player_deathmatch spawns!\n");
		//sigh, guess we'll try info_player_start for the noob mappers
		return gEntList.FindEntityByClassname( pSpot, "info_player_start" );
	}

	//TGB: SELECT SPAWN

	//now loop through the spawn list and find the first valid one
	//to keep things complete and simple we just start from 0 and check them all
	for (int i=0; i < spawn_list.Count(); i++)
	{
		DevMsg("Checking spawn %i... ", i);
		pSpot = spawn_list[i];
		if (!pSpot)
		{
			DevWarning("Non-existent spawn point?\n");
			continue;
		}

		//usable spawn?
		if ( g_pGameRules->IsSpawnPointValid( pSpot, this ) )
		{
			DevMsg("Valid!\n");
			//not sure what use this check is...
			if ( pSpot->GetLocalOrigin() == vec3_origin )
			{
				DevMsg("Spawnpoint w/ invalid spawn location found?\n");
				continue;
			}
			pChosenSpot = pSpot;
			break; //we've found a valid spawnpoint
		}
		else
			DevMsg("Invalid :(!\n");
	}

	if (!pChosenSpot)
	{
		Warning("WARNING: NO VALID SPAWNPOINT FOUND!\n");
	}

	/*TGB: 
		we have to have a spawnpoint anyway, so do that telefraggin' thang.
		Why this instead of the lawyerised version? Because this way it's
		easier to spot problems. And if all works well there should be no
		reason this ever happens unless the mapper fucks up.
	*/

	//TGB: EMERGENCY SPAWN IF NEEDED

	// we haven't found a place to spawn yet,  so kill any guy at the first spawn point and spawn there
	if ( pSpot && !pChosenSpot )
	{
		Warning("Forced to telefrag :( Mapper: make sure you have sufficient spawns that are more than 30 units apart!\n");
		CBaseEntity *ent = NULL;
		edict_t		*player = edict();
		for ( CEntitySphereQuery sphere( pSpot->GetAbsOrigin(), 20 ); (ent = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity() )
		{
			// if ent is a client, kill em (unless they are ourselves)
			if ( ent->IsPlayer() && !(ent->edict() == player) )
				ent->TakeDamage( CTakeDamageInfo( GetContainingEntity(INDEXENT(0)), GetContainingEntity(INDEXENT(0)), 300, DMG_GENERIC ) );
		}
		pChosenSpot = pSpot;
	}

	//TGB: FOUND USABLE SPAWN

	return pChosenSpot;

/* //TGB: START OLDSPAWNCODE
	CBaseEntity *pSpot = NULL;
	CBaseEntity *pSpotNew = NULL; //LAWYER:  For emergency spawnings
	CBaseEntity *pLastSpawnPoint = g_pLastSpawn;
	edict_t		*player = edict();
	const char *pSpawnpointName = "info_player_deathmatch";
	trace_t tr;

	DevMsg("Testing ents at spawn\n");
	gEntList.TestEntities();

	pSpot = pLastSpawnPoint;
	// Randomize the start spot

	for ( int i = random->RandomInt(1,5); i > 0; i-- )
		pSpot = gEntList.FindEntityByClassname( pSpot, pSpawnpointName );
	if ( !pSpot )  // skip over the null point
		pSpot = gEntList.FindEntityByClassname( pSpot, pSpawnpointName );

	CBaseEntity *pFirstSpot = pSpot;

	//TGB: HOLY MOTHER OF GOD, I kill a small town every time I see a goto in c++, which until now was never
	//moved it all into a bool-based structure
	bool finished = false;

	do 
	{
		if ( pSpot )
		{
			// check if pSpot is valid
			if ( g_pGameRules->IsSpawnPointValid( pSpot, this ) )
			{
				if ( pSpot->GetLocalOrigin() == vec3_origin )
				{
					pSpot = gEntList.FindEntityByClassname( pSpot, pSpawnpointName );
					continue;
				}

				// if so, go to pSpot
				//goto ReturnSpot; //TGB: nyaaaaaargh
				finished = true;
				continue; //TGB: this should now finish the while as an end condition is met
			}
		}

		// increment pSpot
		pSpot = gEntList.FindEntityByClassname( pSpot, pSpawnpointName );

	} while ( pSpot != pFirstSpot && finished != true ); // loop if we're not back to the start

	//LAWYER:  Try spawning somewhere else instead, as we've not found anywhere
		//pSpotNew = new CPointEntity();
		//pSpotNew->SetAbsAngles(pSpot->GetAbsAngles());
		//int ii = 0;
		//while (ii < 50) //LAWYER:  M-M-M-MONSTERHACK
		//{
		//	pSpotNew->SetAbsOrigin(Vector((pSpot->GetAbsOrigin().x + random->RandomInt(-128,128)),(pSpot->GetAbsOrigin().y + random->RandomInt(-128,128)),pSpot->GetAbsOrigin().z));
		//	if ( g_pGameRules->IsSpawnPointValid( pSpotNew, this ) )
		//	{
		//		if ( pSpotNew->GetLocalOrigin() == vec3_origin )
		//		{
		//			pSpotNew = gEntList.FindEntityByClassname( pSpotNew, pSpawnpointName );
		//			continue;
		//		}

		//		// if so, go to pSpot
		//		pSpot = pSpotNew;
		//		goto ReturnSpot;
		//	}
		//	ii++;
		//}

		
		
	// we haven't found a place to spawn yet,  so kill any guy at the first spawn point and spawn there
		

	if ( pSpot && !finished )
	{
		
		CBaseEntity *ent = NULL;
		for ( CEntitySphereQuery sphere( pSpot->GetAbsOrigin(), 128 ); (ent = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity() )
		{
			// if ent is a client, kill em (unless they are ourselves)
			if ( ent->IsPlayer() && !(ent->edict() == player) )
			{
				//LAWYER:  Instead of kill them, move us out of the way?
				//ent->TakeDamage( CTakeDamageInfo( GetContainingEntity(INDEXENT(0)), GetContainingEntity(INDEXENT(0)), 300, DMG_GENERIC ) );
				
				pSpotNew = new CPointEntity();
				pSpotNew->SetAbsAngles(pSpot->GetAbsAngles());
				//LAWYER:  Randomize jaunt direction
				for (int x = 0; x <= 100; x++)
				{
					int i = random->RandomInt(1,4);
					if (i == 1)
					{
						UTIL_TraceLine( pSpotNew->GetAbsOrigin(),
						pSpotNew->GetAbsOrigin() + Vector(pSpot->GetAbsOrigin().x + 32, pSpot->GetAbsOrigin().y, pSpot->GetAbsOrigin().z),
						MASK_SOLID,
						NULL,
						COLLISION_GROUP_NONE,
						&tr );

						if( tr.fraction != 1.0 )
						{
							pSpotNew->SetAbsOrigin(Vector(pSpot->GetAbsOrigin().x + 32,pSpot->GetAbsOrigin().y,pSpot->GetAbsOrigin().z));
						}
					}
					else if (i == 2)
					{
						UTIL_TraceLine( pSpotNew->GetAbsOrigin(),
						pSpotNew->GetAbsOrigin() + Vector(pSpot->GetAbsOrigin().x - 32, pSpot->GetAbsOrigin().y, pSpot->GetAbsOrigin().z),
						MASK_SOLID,
						NULL,
						COLLISION_GROUP_NONE,
						&tr );

						if( tr.fraction != 1.0 )
						{
							pSpotNew->SetAbsOrigin(Vector(pSpot->GetAbsOrigin().x - 32,pSpot->GetAbsOrigin().y,pSpot->GetAbsOrigin().z));
						}
					}
					else if (i == 3)
					{
						UTIL_TraceLine( pSpotNew->GetAbsOrigin(),
						pSpotNew->GetAbsOrigin() + Vector(pSpot->GetAbsOrigin().x, pSpot->GetAbsOrigin().y + 32, pSpot->GetAbsOrigin().z),
						MASK_SOLID,
						NULL,
						COLLISION_GROUP_NONE,
						&tr );

						if( tr.fraction != 1.0 )
						{
							pSpotNew->SetAbsOrigin(Vector(pSpot->GetAbsOrigin().x,pSpot->GetAbsOrigin().y + 32,pSpot->GetAbsOrigin().z));
						}
					}
					else if (i == 4)
					{
						UTIL_TraceLine( pSpotNew->GetAbsOrigin(),
						pSpotNew->GetAbsOrigin() + Vector(pSpot->GetAbsOrigin().x, pSpot->GetAbsOrigin().y - 32, pSpot->GetAbsOrigin().z),
						MASK_SOLID,
						NULL,
						COLLISION_GROUP_NONE,
						&tr );

						if( tr.fraction != 1.0 )
						{
							pSpotNew->SetAbsOrigin(Vector(pSpot->GetAbsOrigin().x,pSpot->GetAbsOrigin().y - 32,pSpot->GetAbsOrigin().z));
						}
					}

					pSpot=pSpotNew;
					
					UTIL_TraceHull( pSpotNew->GetAbsOrigin(),
					pSpotNew->GetAbsOrigin() + Vector( 0, 0, 1 ),
					NAI_Hull::Mins(HULL_HUMAN),
					NAI_Hull::Maxs(HULL_HUMAN),
					MASK_SOLID,
					NULL,
					COLLISION_GROUP_NONE,
					&tr );

					if( tr.fraction != 1.0 )
					{
						//LAWYER:  The spawn is blocked!
						x++;
					}
					else
					{
						break; //Found one, it's clear.
					}
				}
			}


				//pPlayer->SetAbsOrigin(Vector(ent->GetAbsOrigin().x + 64,ent->GetAbsOrigin().y,ent->GetAbsOrigin().z)); 
		}
		//goto ReturnSpot; //TGB: go away, foul demon
		finished = true;
	}

	if ( !pSpot && !finished )
	{
		pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_start" );

		//we'll already be where we want to be when out of this if
		//if ( pSpot )
			//goto ReturnSpot;	
			//break;
	}


//ReturnSpot: //TGB: die die die!
	//TGB: that was all of them, and we will still arrive here almost right away whenever finished == true


	g_pLastSpawn = pSpot;
	if (GetTeamNumber() == 3)
	{
		pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_zombiemaster" ); //LAWYER:  Zombiemasters always spawn here
	}
	if ( !pSpot  )
	{
		pSpot = g_pLastSpawn;
	}

	//LAWYER:  I have a suspicion this code was causing the spawning problems.  So, trimmed.

	return pSpot;

	*/ //TGB: END OLDSPAWNCODE
} 

void CHL2MP_Player::Reset()
{	
	ResetDeathCount();
	ResetFragCount();
}

bool CHL2MP_Player::IsReady()
{
	return m_bReady;
}

void CHL2MP_Player::SetReady( bool bReady )
{
	m_bReady = bReady;
}

void CHL2MP_Player::CheckChatText( char *p, int bufsize )
{
	//Look for escape sequences and replace

	char *buf = new char[bufsize];
	int pos = 0;

	// Parse say text for escape sequences
	for ( char *pSrc = p; pSrc != NULL && *pSrc != 0 && pos < bufsize-1; pSrc++ )
	{
		// copy each char across
		buf[pos] = *pSrc;
		pos++;
	}

	buf[pos] = '\0';

	// copy buf back into p
	Q_strncpy( p, buf, bufsize );

	delete[] buf;	

//	const char *pReadyCheck = p;
//TGBMERGENOTE: haven't merged that func in
//	HL2MPRules()->CheckChatForReadySignal( this, pReadyCheck );
}

void CHL2MP_Player::State_Transition( HL2MPPlayerState newState )
{
	State_Leave();
	State_Enter( newState );
}


void CHL2MP_Player::State_Enter( HL2MPPlayerState newState )
{
	m_iPlayerState = newState;
	m_pCurStateInfo = State_LookupInfo( newState );

	// Initialize the new state.
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnEnterState )
		(this->*m_pCurStateInfo->pfnEnterState)();
}


void CHL2MP_Player::State_Leave()
{
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnLeaveState )
	{
		(this->*m_pCurStateInfo->pfnLeaveState)();
	}
}


void CHL2MP_Player::State_PreThink()
{
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnPreThink )
	{
		(this->*m_pCurStateInfo->pfnPreThink)();
	}
}


CHL2MPPlayerStateInfo *CHL2MP_Player::State_LookupInfo( HL2MPPlayerState state )
{
	// This table MUST match the 
	static CHL2MPPlayerStateInfo playerStateInfos[] =
	{
		{ STATE_ACTIVE,			"STATE_ACTIVE",			&CHL2MP_Player::State_Enter_ACTIVE, NULL, &CHL2MP_Player::State_PreThink_ACTIVE },
		{ STATE_OBSERVER_MODE,	"STATE_OBSERVER_MODE",	&CHL2MP_Player::State_Enter_OBSERVER_MODE,	NULL, &CHL2MP_Player::State_PreThink_OBSERVER_MODE }
	};

	for ( int i=0; i < ARRAYSIZE( playerStateInfos ); i++ )
	{
		if ( playerStateInfos[i].m_iPlayerState == state )
			return &playerStateInfos[i];
	}

	return NULL;
}

//TGBMERGENOTE: dupe for us
//bool CHL2MP_Player::StartObserverMode(int mode)
//{
//	//we only want to go into observer mode if the player asked to, not on a death timeout
//	if ( m_bEnterObserver == true )
//	{
//		return BaseClass::StartObserverMode( mode );
//	}
//	return false;
//}
//
//void CHL2MP_Player::StopObserverMode()
//{
//	m_bEnterObserver = false;
//	BaseClass::StopObserverMode();
//}

void CHL2MP_Player::State_Enter_OBSERVER_MODE()
{
	int observerMode = m_iObserverLastMode;
	if ( IsNetClient() )
	{
		const char *pIdealMode = engine->GetClientConVarValue( engine->IndexOfEdict( edict() ), "cl_spec_mode" );
		if ( pIdealMode )
		{
			observerMode = atoi( pIdealMode );
			if ( observerMode <= OBS_MODE_FIXED || observerMode > OBS_MODE_ROAMING )
			{
				observerMode = m_iObserverLastMode;
			}
		}
	}
	m_bEnterObserver = true;
	StartObserverMode( observerMode );
}

void CHL2MP_Player::State_PreThink_OBSERVER_MODE()
{
	// Make sure nobody has changed any of our state.
	//	Assert( GetMoveType() == MOVETYPE_FLY );
	Assert( m_takedamage == DAMAGE_NO );
	Assert( IsSolidFlagSet( FSOLID_NOT_SOLID ) );
	//	Assert( IsEffectActive( EF_NODRAW ) );

	// Must be dead.
	Assert( m_lifeState == LIFE_DEAD );
	Assert( pl.deadflag );
}


void CHL2MP_Player::State_Enter_ACTIVE()
{
	SetMoveType( MOVETYPE_WALK );
	RemoveSolidFlags( FSOLID_NOT_SOLID );
	m_Local.m_iHideHUD = 0;
}


void CHL2MP_Player::State_PreThink_ACTIVE()
{
	//we don't really need to do anything here. 
	//This state_prethink structure came over from CS:S and was doing an assert check that fails the way hl2dm handles death
}
