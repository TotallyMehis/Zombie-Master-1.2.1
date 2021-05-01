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
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "hl2mp_gamerules.h"
#include "viewport_panel_names.h"
#include "gameeventdefs.h"
#include <KeyValues.h>
#include "ammodef.h"
#include "edict.h"

#ifdef CLIENT_DLL
	#include "c_hl2mp_player.h"
	#include "c_basetempentity.h"
#else
	#include "eventqueue.h"
	#include "player.h"
	#include "gamerules.h"
	#include "game.h"
	#include "items.h"
	#include "entitylist.h"
	#include "mapentities.h"
	#include "in_buttons.h"
	#include <ctype.h>
	#include "voice_gamemgr.h"
	#include "iscorer.h"
	#include "hl2mp_player.h"
	#include "weapon_hl2mpbasehlmpcombatweapon.h"
	#include "team.h"
	#include "voice_gamemgr.h"
	#include "hl2mp_gameinterface.h"
	#include "mapentityfilter.h"
	#include "zombielist.h"
	#include "utlvector.h" //tgb
	#include "zombiemaster/zombiemaster_specific.h"

	#include "ilagcompensationmanager.h"

#ifdef DEBUG	
	#include "hl2mp_bot_temp.h"
#endif

extern void respawn(CBaseEntity *pEdict, bool fCopyCorpse);


ConVar sv_hl2mp_weapon_respawn_time( "sv_hl2mp_weapon_respawn_time", "600", FCVAR_GAMEDLL | FCVAR_NOTIFY );
ConVar sv_hl2mp_item_respawn_time( "sv_hl2mp_item_respawn_time", "30", FCVAR_GAMEDLL | FCVAR_NOTIFY );
ConVar mp_restartgame( "mp_restartgame", "0", 0, "If non-zero, game will restart in the specified number of seconds" );
ConVar sv_report_client_settings("sv_report_client_settings", "0", FCVAR_GAMEDLL | FCVAR_NOTIFY );

ConVar zm_weaponreset( "zm_weaponreset", "0", FCVAR_GAMEDLL | FCVAR_NOTIFY, "If set to 1, makes weapons be reset to their spawn after being dropped by (dead) players. Default and recommended off (= 0)." );
//ConVar zm_sv_randommaster( "zm_sv_randommaster", "1", FCVAR_GAMEDLL | FCVAR_NOTIFY );

ConVar zm_debugevents("zm_debugevents", "0", FCVAR_GAMEDLL | FCVAR_CHEAT, "Set to 1 to force zm game events to fire without a listener");

extern ConVar mp_chattime;

extern CBaseEntity	 *g_pLastCombineSpawn;
extern CBaseEntity	 *g_pLastRebelSpawn;

#define WEAPON_MAX_DISTANCE_FROM_SPAWN 64

#endif


REGISTER_GAMERULES_CLASS( CHL2MPRules );

BEGIN_NETWORK_TABLE_NOBASE( CHL2MPRules, DT_HL2MPRules )

	#ifdef CLIENT_DLL
		RecvPropBool( RECVINFO( m_bTeamPlayEnabled ) ),
		RecvPropBool( RECVINFO( m_bIsRestarting ) ), //qck edit
	#else
		SendPropBool( SENDINFO( m_bTeamPlayEnabled ) ),
		SendPropBool( SENDINFO( m_bIsRestarting) ) //qck edit
	#endif

END_NETWORK_TABLE()


LINK_ENTITY_TO_CLASS( hl2mp_gamerules, CHL2MPGameRulesProxy );
IMPLEMENT_NETWORKCLASS_ALIASED( HL2MPGameRulesProxy, DT_HL2MPGameRulesProxy )

static HL2MPViewVectors g_HL2MPViewVectors(
	Vector( 0, 0, 64 ),        //m_vView
							  
	Vector(-16, -16, 0 ),	  //m_vHullMin
	Vector( 16,  16,  72 ),	  //m_vHullMax
							  
	Vector(-16, -16, 0 ),	  //m_vDuckHullMin
	Vector( 16,  16,  36 ),	  //m_vDuckHullMax
	Vector( 0, 0, 28 ),		  //m_vDuckView
							  
	Vector(-10, -10, -10 ),	  //m_vObsHullMin
	Vector( 10,  10,  10 ),	  //m_vObsHullMax
							  
	Vector( 0, 0, 14 ),		  //m_vDeadViewHeight

	Vector(-16, -16, 0 ),	  //m_vCrouchTraceMin
	Vector( 16,  16,  60 )	  //m_vCrouchTraceMax
);

//TGB: HL2DM'S LIST, NOT OURS
static const char *s_PreserveEnts[] =
{
	"ai_network",
	"ai_hint",
	"hl2mp_gamerules",
	"team_manager",
	"player_manager",
	"env_soundscape",
	"env_soundscape_proxy",
	"env_soundscape_triggerable",
	"env_sun",
	"env_wind",
	"env_fog_controller",
	"func_brush",
	"func_wall",
	"func_buyzone",
	"func_illusionary",
	"infodecal",
	"info_projecteddecal",
	"info_node",
	"info_target",
	"info_node_hint",
	"info_player_deathmatch",
	"info_player_combine",
	"info_player_rebel",
	"info_map_parameters",
	"keyframe_rope",
	"move_rope",
	"info_ladder",
	"player",
	"point_viewcontrol",
	"scene_manager",
	"shadow_control",
	"sky_camera",
	"soundent",
	"trigger_soundscape",
	"viewmodel",
	"predicted_viewmodel",
	"worldspawn",
	"point_devshot_camera",
	"", // END Marker
};



#ifdef CLIENT_DLL
	void RecvProxy_HL2MPRules( const RecvProp *pProp, void **pOut, void *pData, int objectID )
	{
		CHL2MPRules *pRules = HL2MPRules();
		Assert( pRules );
		*pOut = pRules;
	}

	BEGIN_RECV_TABLE( CHL2MPGameRulesProxy, DT_HL2MPGameRulesProxy )
		RecvPropDataTable( "hl2mp_gamerules_data", 0, 0, &REFERENCE_RECV_TABLE( DT_HL2MPRules ), RecvProxy_HL2MPRules )
	END_RECV_TABLE()
#else
	void* SendProxy_HL2MPRules( const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
	{
		CHL2MPRules *pRules = HL2MPRules();
		Assert( pRules );
		return pRules;
	}

	BEGIN_SEND_TABLE( CHL2MPGameRulesProxy, DT_HL2MPGameRulesProxy )
		SendPropDataTable( "hl2mp_gamerules_data", 0, &REFERENCE_SEND_TABLE( DT_HL2MPRules ), SendProxy_HL2MPRules )
	END_SEND_TABLE()
#endif

	//qck: Simple little function to look through an array and find what we need
	bool ArraySearch( const char **pStrings, const char *pToFind, int nStrings )
	{
		for ( int i=0; i < nStrings; i++ )
			if ( Q_stricmp( pStrings[i], pToFind ) == 0 )
				return true;

		return false;
	}

#ifndef CLIENT_DLL

	class CVoiceGameMgrHelper : public IVoiceGameMgrHelper
	{
	public:
		virtual bool		CanPlayerHearPlayer( CBasePlayer *pListener, CBasePlayer *pTalker )
		{
			return ( pListener->GetTeamNumber() == pTalker->GetTeamNumber() );
		}
	};
	CVoiceGameMgrHelper g_VoiceGameMgrHelper;
	IVoiceGameMgrHelper *g_pVoiceGameMgrHelper = &g_VoiceGameMgrHelper;

#endif

// NOTE: the indices here must match TEAM_TERRORIST, TEAM_CT, TEAM_SPECTATOR, etc.
char *sTeamNames[] =
{
	"Unassigned",
	"Spectator",
	"Human Survivors",
	"The Zombie Master",
};


//qck: Define up here, rather than in a local function. Might need it someplace else, and I find it cleaner.
//TGB: extended with entities taken from HL2MP preservation list, seeing as I see no reason ZM would be different wrt those ents
const char *entitiesToKeep[] =
{
		//START OLDLIST
		//"worldspawn",
		//"predicted_viewmodel",
		//"player",
		//"hl2mp_gamerules",
		//"ai_network",
		//"soundent",
		//"info_player_start",
		//"info_player_combine",
		//"info_player_rebel",
		//"info_player_deathmatch",
		//"player_manager",
		//"event_queue_saveload_proxy", //qck: Do we need this? I removed it without any problems, but added it back just in case.
		//"team_manager",
		//"scene_manager",
		//END OLDLIST

	"ai_network",
	"ai_hint",
	"hl2mp_gamerules",
	"team_manager",
	"player_manager",
	"env_soundscape",
	"env_soundscape_proxy",
	"env_soundscape_triggerable",
	"env_sun",
	"env_wind",
	"env_fog_controller",
//	"func_brush", //TGB: causes trouble in dotd
	"func_wall",
	"func_buyzone",
	"func_illusionary",
	"infodecal",
	"info_projecteddecal",
	"info_node",
	"info_target",
	"info_node_hint",
	"info_player_deathmatch",
	"info_player_combine",
	"info_player_rebel",
	"info_player_zombiemaster",
	"info_map_parameters",
//	"keyframe_rope", //TGB: causes trouble in miner
//	"move_rope",
	"info_ladder",
	"player",
	"point_viewcontrol",
	"scene_manager",
	"shadow_control",
	"sky_camera",
	"soundent",
	"trigger_soundscape",
	"viewmodel",
	"predicted_viewmodel",
	"worldspawn",
	"point_devshot_camera",
	"", // END Marker
};

CHL2MPRules::CHL2MPRules()
{
#ifndef CLIENT_DLL
	// Create the team managers
	for ( int i = 0; i < ARRAYSIZE( sTeamNames ); i++ )
	{
		CTeam *pTeam = static_cast<CTeam*>(CreateEntityByName( "team_manager" ));
		pTeam->Init( sTeamNames[i], i );

		g_Teams.AddToTail( pTeam );
	}

	m_bTeamPlayEnabled = true;
	m_flIntermissionEndTime = 0.0f;
	//m_flRoundRestart = gpGlobals->curtime + 1.0f; //tgb: always begin with a RR so everything is proper //UNDONE interferes with other RR mechanics
	m_flRoundRestart = 0.0f;
	m_flRoundStartTime = 0.0f;
	m_flGameStartTime = 0;

	m_hRespawnableItemsAndWeapons.RemoveAll();
	m_tmNextPeriodicThink = 0;
	m_flRestartGameTime = 0;
	m_bCompleteReset = false;
	m_bHeardAllPlayersReady = false;
	m_bAwaitingReadyRestart = false;

	m_bIsRestarting = false;
	cMapEntityCache = NULL;

	gEntList.m_ZombieList.Purge();
	gEntList.m_ZombieSelected.Purge();
	gEntList.m_BansheeList.Purge();

	m_pLoadOut = NULL;
#endif

}

const CViewVectors* CHL2MPRules::GetViewVectors()const
{
	return &g_HL2MPViewVectors;
}

const HL2MPViewVectors* CHL2MPRules::GetHL2MPViewVectors()const
{
	return &g_HL2MPViewVectors;
}
	
CHL2MPRules::~CHL2MPRules( void )
{
#ifndef CLIENT_DLL
	// Note, don't delete each team since they are in the gEntList and will 
	// automatically be deleted from there, instead.
	g_Teams.Purge();
#endif
}

void CHL2MPRules::CreateStandardEntities( void )
{

#ifndef CLIENT_DLL
	// Create the entity that will send our data to the client.

	BaseClass::CreateStandardEntities();

	g_pLastCombineSpawn = NULL;
	g_pLastRebelSpawn = NULL;

#ifdef _DEBUG
	CBaseEntity *pEnt = 
#endif
	CBaseEntity::Create( "hl2mp_gamerules", vec3_origin, vec3_angle );
	Assert( pEnt );
#endif
}

//=========================================================
// FlWeaponRespawnTime - what is the time in the future
// at which this weapon may spawn?
//=========================================================
float CHL2MPRules::FlWeaponRespawnTime( CBaseCombatWeapon *pWeapon )
{
#ifndef CLIENT_DLL
	if ( weaponstay.GetInt() > 0 )
	{
		// make sure it's only certain weapons
		if ( !(pWeapon->GetWeaponFlags() & ITEM_FLAG_LIMITINWORLD) )
		{
			return 0;		// weapon respawns almost instantly
		}
	}

	return sv_hl2mp_weapon_respawn_time.GetFloat();
#endif

	return 0;		// weapon respawns almost instantly
}


bool CHL2MPRules::IsIntermission( void )
{
#ifndef CLIENT_DLL
	return m_flIntermissionEndTime > gpGlobals->curtime;
#endif

	return false;
}

//--------------------------------------------------------------
// TGB: EVENTS BEGIN
//--------------------------------------------------------------


void CHL2MPRules::PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info )
{
#ifndef CLIENT_DLL
	if ( IsIntermission() )
		return;
	BaseClass::PlayerKilled( pVictim, info );
#endif
}

void CHL2MPRules::ZombieKilled( CBaseEntity *pVictim, const CTakeDamageInfo &info ) 
{
#ifndef CLIENT_DLL
	if (!pVictim) 
		return;

	// Find the killer & the scorer
	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();
	CBasePlayer *pScorer = GetDeathScorer( pKiller, pInflictor );

	int killerID = (pScorer) ? pScorer->GetUserID() : -1;

	//TGB: copied from DeathNotice for players, throwing a zombie death event lets server plugins
	//	do nicer stat tracking, as requested by server-admin-dude asceth
	IGameEvent * event = gameeventmanager->CreateEvent( "zombie_death", zm_debugevents.GetBool() );
	if ( event )
	{
		const char *weapon = "world";
		//TGB: molotov/barrel hack
		if (info.GetDamageType() & DMG_BURN)
			weapon = "fire";
		else if (info.GetDamageType() & DMG_BLAST)
			weapon = "explosion";
		else if (pScorer && pScorer->GetActiveWeapon())
			weapon = pScorer->GetActiveWeapon()->GetClassname();
		//else "world"

		event->SetString("type", pVictim->GetClassname());
		event->SetInt("attacker", killerID );
		event->SetInt("damage", ceil(info.GetDamage()));
		event->SetString("weapon", weapon);
		event->SetInt("z_id", pVictim->entindex());
		

		gameeventmanager->FireEvent( event );

		//DevMsg("Throwing zombie_death event: type %s, attacker %i, dmg %i, weapon %s\n", pVictim->GetClassname(), killerID, ceil(info.GetDamage()), weapon);
	}
#endif
}

void CHL2MPRules::ZombieSpawned( CBaseEntity *pZombie) const
{
#ifndef CLIENT_DLL
	if (!pZombie) return;

	IGameEvent * event = gameeventmanager->CreateEvent( "zombie_spawn", zm_debugevents.GetBool() );
	if ( event )
	{
		event->SetString("type", pZombie->GetClassname());
		event->SetInt("z_id", pZombie->entindex());
		gameeventmanager->FireEvent( event );

		//DevMsg("Throwing zombie_create event: type %s\n", pZombie->GetClassname());
	}
#endif
}

//TGB: another event, all these together pretty much allow achievements through server plugins
void CHL2MPRules::ZombieHurt( CBaseEntity *pVictim, const CTakeDamageInfo &info )
{
#ifndef CLIENT_DLL
	if (!pVictim) 
		return;

	// Find the killer & the scorer
	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();
	CBasePlayer *pScorer = GetDeathScorer( pKiller, pInflictor );

	int killerID = (pScorer) ? pScorer->GetUserID() : -1;

	IGameEvent * event = gameeventmanager->CreateEvent( "zombie_hurt", zm_debugevents.GetBool() );
	if ( event )
	{
		const char *weapon = "world";
		//TGB: molotov/barrel hack
		if (info.GetDamageType() & DMG_BURN)
			weapon = "fire";
		else if (info.GetDamageType() & DMG_BLAST)
			weapon = "explosion";
		else if (pScorer && pScorer->GetActiveWeapon())
			weapon = pScorer->GetActiveWeapon()->GetClassname();
		//else "world"

		
		//DevMsg("Inflictor: %s\n", (pInflictor) ? pInflictor->GetClassname() : "null");

		event->SetString("type", pVictim->GetClassname());
		event->SetInt("attacker", killerID );
		event->SetInt("damage", ceil(info.GetDamage()));
		event->SetString("weapon", weapon);
		event->SetInt("z_id", pVictim->entindex());

		// This event is not transmitted to clients, as that'd be a waste of bandwidth
		gameeventmanager->FireEvent( event, true );

		//DevMsg("Throwing zombie_death event: type %s, attacker %i, dmg %i, weapon %s\n", pVictim->GetClassname(), killerID, ceil(info.GetDamage()), weapon);
	}
#endif
}

//TGB: game event for getting points
void CHL2MPRules::PlayerGotPoints( CBaseEntity *pPlayer, const int points, CBaseEntity *pEntity ) const
{
#ifndef CLIENT_DLL
	CBasePlayer *pBasePlayer = ToBasePlayer(pPlayer);
	if (pBasePlayer && pEntity)
	{
		
		IGameEvent * event = gameeventmanager->CreateEvent( "player_got_points", zm_debugevents.GetBool() );
		if ( event )
		{
			event->SetInt("player", pBasePlayer->GetUserID());
			event->SetInt("points", points);
			event->SetInt("score_ent", pEntity->entindex());

			gameeventmanager->FireEvent( event, true );
		}
	}
#endif
}

//TGB: game event for win condition
void CHL2MPRules::TeamVictorious( bool humans_won, const char* cause ) const
{
#ifndef CLIENT_DLL
	IGameEvent * event = gameeventmanager->CreateEvent( "round_victory", zm_debugevents.GetBool() );
	if ( event )
	{
		//const char *name = STRING( pManipulate->GetEntityName() ); //I remember linux hating getentityname
		event->SetBool("humans_won", humans_won);
		event->SetString("cause", cause);

		gameeventmanager->FireEvent( event, true );
	}
#endif
}


//TGB: game event for triggered manip
void CHL2MPRules::ManipulateTriggered( CZombieManipulate *pManipulate ) const
{
#ifndef CLIENT_DLL
	if (!pManipulate) return;

	IGameEvent * event = gameeventmanager->CreateEvent( "triggered_manipulate", zm_debugevents.GetBool() );
	if ( event )
	{
		//const char *name = STRING( pManipulate->GetEntityName() ); //I remember linux hating getentityname
		event->SetInt("entindex", pManipulate->entindex());

		gameeventmanager->FireEvent( event, true );
	}
#endif
}

//--------------------------------------------------------------
// TGB: EVENTS END
//--------------------------------------------------------------



void CHL2MPRules::Think( void )
{

#ifndef CLIENT_DLL
	
	CGameRules::Think();

		//LAWYER:  Check if we want a round restart!
		if (g_pGameRules->m_flRoundRestart != 0.0f && gpGlobals->curtime >= g_pGameRules->m_flRoundRestart)
		{				
				//LAWYER:  Restart the round!
				g_pGameRules->m_flRoundRestart = 0.0f; //Reset the clock!
//				Warning("Restarting round...\n");
				g_pGameRules->EndRound();
		}


	if ( g_fGameOver )   // someone else quit the game already
	{
		// check to see if we should change levels now
		if ( m_flIntermissionEndTime < gpGlobals->curtime )
		{
			ChangeLevel(); // intermission is over
		}

		return;
	}

//	float flTimeLimit = mp_timelimit.GetFloat() * 60;
	const float flFragLimit = fraglimit.GetFloat();
	const float flRoundLimit = roundlimit.GetFloat(); //LAWYER:  Maximum number of rounds

	if ( GetMapRemainingTime() < 0 )
	{
		GoToIntermission();
		return;
	}
	//LAWYER:  We need to check for round limit!
	if (flRoundLimit)
	{
		if (this->m_iRoundsCompleted > (flRoundLimit + 1)) //LAWYER:  Fudged, to give a more accurate round count
		{
			//Number of rounds exceeded - next map!
			GoToIntermission();
		}
	}
	if ( flFragLimit )
	{
		if( IsTeamplay() == true )
		{
			CTeam *pCombine = g_Teams[TEAM_HUMANS];
			CTeam *pRebels = g_Teams[TEAM_ZOMBIEMASTER];

			if ( pCombine->GetScore() >= flFragLimit || pRebels->GetScore() >= flFragLimit )
			{
				GoToIntermission();
				return;
			}

		}
		else
		{
			// check if any player is over the frag limit
			for ( int i = 1; i <= gpGlobals->maxClients; i++ )
			{
				CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

				if ( pPlayer && pPlayer->FragCount() >= flFragLimit )
				{
					GoToIntermission();
					return;
				}
			}

		}
	}

	if ( gpGlobals->curtime > m_tmNextPeriodicThink )
	{
		CheckRestartGame();
		m_tmNextPeriodicThink = gpGlobals->curtime + 1.0;
	}

	if ( m_flRestartGameTime > 0.0f && m_flRestartGameTime <= gpGlobals->curtime )
	{
		RestartGame();
	}

	if (zm_weaponreset.GetBool())
		ManageObjectRelocation();

#endif
}

void CHL2MPRules::GoToIntermission( void )
{
#ifndef CLIENT_DLL
	if ( g_fGameOver )
		return;

	g_fGameOver = true;

	m_flIntermissionEndTime = gpGlobals->curtime + mp_chattime.GetInt();

	for ( int i = 0; i < MAX_PLAYERS; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

		if ( !pPlayer )
			continue;

		pPlayer->ShowViewPortPanel( PANEL_SCOREBOARD );
		pPlayer->AddFlag( FL_FROZEN );
	}
#endif
	
}

bool CHL2MPRules::CheckGameOver()
{
#ifndef CLIENT_DLL
	if ( g_fGameOver )   // someone else quit the game already
	{
		// check to see if we should change levels now
		if ( m_flIntermissionEndTime < gpGlobals->curtime )
		{
			ChangeLevel(); // intermission is over			
		}

		return true;
	}
#endif

	return false;
}

// when we are within this close to running out of entities,  items 
// marked with the ITEM_FLAG_LIMITINWORLD will delay their respawn
#define ENTITY_INTOLERANCE	100

//=========================================================
// FlWeaponRespawnTime - Returns 0 if the weapon can respawn 
// now,  otherwise it returns the time at which it can try
// to spawn again.
//=========================================================
float CHL2MPRules::FlWeaponTryRespawn( CBaseCombatWeapon *pWeapon )
{
#ifndef CLIENT_DLL
	if ( pWeapon && (pWeapon->GetWeaponFlags() & ITEM_FLAG_LIMITINWORLD) )
	{
		if ( gEntList.NumberOfEntities() < (gpGlobals->maxEntities - ENTITY_INTOLERANCE) )
			return 0;

		// we're past the entity tolerance level,  so delay the respawn
		return FlWeaponRespawnTime( pWeapon );
	}
#endif
	return 0;
}

//=========================================================
// VecWeaponRespawnSpot - where should this weapon spawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHL2MPRules::VecWeaponRespawnSpot( CBaseCombatWeapon *pWeapon )
{
#ifndef CLIENT_DLL
	CWeaponHL2MPBase *pHL2Weapon = dynamic_cast< CWeaponHL2MPBase*>( pWeapon );

	if ( pHL2Weapon )
	{
		return pHL2Weapon->GetOriginalSpawnOrigin();
	}
#endif
	
	return pWeapon->GetAbsOrigin();
}

#ifndef CLIENT_DLL

CItem* IsManagedObjectAnItem( CBaseEntity *pObject )
{
	return dynamic_cast< CItem*>( pObject );
}

CWeaponHL2MPBase* IsManagedObjectAWeapon( CBaseEntity *pObject )
{
	return dynamic_cast< CWeaponHL2MPBase*>( pObject );
}

bool GetObjectsOriginalParameters( CBaseEntity *pObject, Vector &vOriginalOrigin, QAngle &vOriginalAngles )
{
	if ( CItem *pItem = IsManagedObjectAnItem( pObject ) )
	{
		if ( pItem->m_flNextResetCheckTime > gpGlobals->curtime )
			 return false;
		
		vOriginalOrigin = pItem->GetOriginalSpawnOrigin();
		vOriginalAngles = pItem->GetOriginalSpawnAngles();

		pItem->m_flNextResetCheckTime = gpGlobals->curtime + sv_hl2mp_item_respawn_time.GetFloat();
		return true;
	}
	else if ( CWeaponHL2MPBase *pWeapon = IsManagedObjectAWeapon( pObject )) 
	{
		if ( pWeapon->m_flNextResetCheckTime > gpGlobals->curtime )
			 return false;

		vOriginalOrigin = pWeapon->GetOriginalSpawnOrigin();
		vOriginalAngles = pWeapon->GetOriginalSpawnAngles();

		pWeapon->m_flNextResetCheckTime = gpGlobals->curtime + sv_hl2mp_weapon_respawn_time.GetFloat();
		return true;
	}

	return false;
}
//TGBNOTE: this could be what makes weapons "respawn", or at least one of the causes
void CHL2MPRules::ManageObjectRelocation( void )
{
	int iTotal = m_hRespawnableItemsAndWeapons.Count();

	if ( iTotal > 0 )
	{
		for ( int i = 0; i < iTotal; i++ )
		{
			CBaseEntity *pObject = m_hRespawnableItemsAndWeapons[i].Get();
			
			if ( pObject )
			{
				Vector vSpawOrigin;
				QAngle vSpawnAngles;

				if ( GetObjectsOriginalParameters( pObject, vSpawOrigin, vSpawnAngles ) == true )
				{
					float flDistanceFromSpawn = (pObject->GetAbsOrigin() - vSpawOrigin ).Length();

					if ( flDistanceFromSpawn > WEAPON_MAX_DISTANCE_FROM_SPAWN )
					{
						bool shouldReset = false;
						IPhysicsObject *pPhysics = pObject->VPhysicsGetObject();

						if ( pPhysics )
						{
							shouldReset = pPhysics->IsAsleep();
						}
						else
						{
							shouldReset = (pObject->GetFlags() & FL_ONGROUND) ? true : false;
						}

						if ( shouldReset )
						{
							pObject->Teleport( &vSpawOrigin, &vSpawnAngles, NULL );
							//TGB: removing this as well to make sure we get no spawn sound
							//pObject->EmitSound( "AlyxEmp.Charge" );

							IPhysicsObject *pPhys = pObject->VPhysicsGetObject();

							if ( pPhys )
							{
								pPhys->Wake();
							}
						}
					}
				}
			}
		}
	}
}

//=========================================================
//AddLevelDesignerPlacedWeapon
//=========================================================
void CHL2MPRules::AddLevelDesignerPlacedObject( CBaseEntity *pEntity )
{
	if ( m_hRespawnableItemsAndWeapons.Find( pEntity ) == -1 )
	{
		m_hRespawnableItemsAndWeapons.AddToTail( pEntity );
	}
}

//=========================================================
//RemoveLevelDesignerPlacedWeapon
//=========================================================
void CHL2MPRules::RemoveLevelDesignerPlacedObject( CBaseEntity *pEntity )
{
	if ( m_hRespawnableItemsAndWeapons.Find( pEntity ) != -1 )
	{
		m_hRespawnableItemsAndWeapons.FindAndRemove( pEntity );
	}
}

//=========================================================
// Where should this item respawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHL2MPRules::VecItemRespawnSpot( CItem *pItem )
{
	return pItem->GetOriginalSpawnOrigin();
}

//=========================================================
// What angles should this item use to respawn?
//=========================================================
QAngle CHL2MPRules::VecItemRespawnAngles( CItem *pItem )
{
	return pItem->GetOriginalSpawnAngles();
}

//=========================================================
// At what time in the future may this Item respawn?
//=========================================================
float CHL2MPRules::FlItemRespawnTime( CItem *pItem )
{
	return sv_hl2mp_item_respawn_time.GetFloat();
}


//=========================================================
// CanHaveWeapon - returns false if the player is not allowed
// to pick up this weapon
//=========================================================
bool CHL2MPRules::CanHavePlayerItem( CBasePlayer *pPlayer, CBaseCombatWeapon *pItem )
{
	if ( weaponstay.GetInt() > 0 )
	{
		if ( pPlayer->Weapon_OwnsThisType( pItem->GetClassname(), pItem->GetSubType() ) )
			 return false;
	}

	return BaseClass::CanHavePlayerItem( pPlayer, pItem );
}

#endif

//=========================================================
// WeaponShouldRespawn - any conditions inhibiting the
// respawning of this weapon?
//=========================================================
int CHL2MPRules::WeaponShouldRespawn( CBaseCombatWeapon *pWeapon )
{
#ifndef CLIENT_DLL
	if ( pWeapon->HasSpawnFlags( SF_NORESPAWN ) )
	{
		return GR_WEAPON_RESPAWN_NO;
	}
#endif

	//return GR_WEAPON_RESPAWN_YES;
	return GR_WEAPON_RESPAWN_NO; //LAWYER:  HACKHACKHACK - weapons don't respawn, as I can't trace the bug that causes them to fall from the sky.
}

#ifndef CLIENT_DLL
//--------------------------------------------------------------
// TGB: this player just latespawned, hand out some guns
//	NOTE: roundrestart still uses loadout->Distribute()
//--------------------------------------------------------------
void CHL2MPRules::LoadOutPlayer(CBasePlayer *pPlayer)
{
	if (!pPlayer)
		return;
	
	if (pPlayer->IsSurvivor() == false)
		return;
	
	if (m_pLoadOut == NULL)
		return;

	m_pLoadOut->DistributeToPlayer(pPlayer);	
    	
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Player has just left the game
//-----------------------------------------------------------------------------
void CHL2MPRules::ClientDisconnected( edict_t *pClient )  //LAWYER:  We need to add checks in here
{
#ifndef CLIENT_DLL
	// Msg( "CLIENT DISCONNECTED, REMOVING FROM TEAM.\n" );

	CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( pClient );
	if ( pPlayer )
	{
		// Remove the player from his team
		if ( pPlayer->GetTeam() )
		{
			pPlayer->GetTeam()->RemovePlayer( pPlayer );
		}
	}

	BaseClass::ClientDisconnected( pClient );

#endif
}


//=========================================================
// Deathnotice. 
//=========================================================
void CHL2MPRules::DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info )
{
#ifndef CLIENT_DLL
	// Work out what killed the player, and send a message to all clients about it
	const char *killer_weapon_name = "world";		// by default, the player is killed by the world
	int killer_ID = 0;
	bool bZombieKill = false;

	// Find the killer & the scorer
	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();
	CBasePlayer *pScorer = GetDeathScorer( pKiller, pInflictor );

	if (!pKiller || !pVictim) return; //just in case

	// Custom kill type?
	if ( info.GetCustomKill() )
	{
		killer_weapon_name = GetCustomKillString( info );
		if ( pScorer )
		{
			killer_ID = pScorer->GetUserID();
		}
	}
	else
	{
		// Is the killer a client?
		if ( pScorer )
		{
			killer_ID = pScorer->GetUserID();
			
			if ( pInflictor )
			{
				if ( pInflictor == pScorer )
				{
					// If the inflictor is the killer,  then it must be their current weapon doing the damage
					if ( pScorer->GetActiveWeapon() )
					{
						killer_weapon_name = pScorer->GetActiveWeapon()->GetClassname();
					}
				}
				else
				{
					killer_weapon_name = pInflictor->GetClassname();  // it's just that easy
				}
			}
		}
		else
		{
			killer_weapon_name = pInflictor->GetClassname();
		}

		// strip the NPC_* or weapon_* from the inflictor's classname
		if ( V_strncmp( killer_weapon_name, "weapon_", 7 ) == 0 )
		{
			killer_weapon_name += 7;
		}
		else if ( V_strncmp( killer_weapon_name, "npc_", 4 ) == 0 )
		{
			killer_weapon_name += 4;
		}
		else if ( V_strncmp( killer_weapon_name, "func_", 5 ) == 0 )
		{
			killer_weapon_name += 5;
		}
		else if ( V_strstr( killer_weapon_name, "physics" ) )
		{
			killer_weapon_name = "physics";
		}

		if ( V_strcmp( killer_weapon_name, "prop_combine_ball" ) == 0 )
		{
			killer_weapon_name = "combine_ball";
		}
		else if ( V_strcmp( killer_weapon_name, "grenade_ar2" ) == 0 )
		{
			killer_weapon_name = "smg1_grenade";
		}
		else if ( V_strcmp( killer_weapon_name, "satchel" ) == 0 || V_strcmp( killer_weapon_name, "tripmine" ) == 0)
		{
			killer_weapon_name = "slam";
		}


		//TGB: if a human was killed by a zombie
		if (pVictim->GetTeamNumber() == 2 && pKiller->Classify() == CLASS_ZOMBIE )
		{
			killer_weapon_name = "Zombie";
			//DevMsg("Killed by a zombie. pKiller class = %s \n", pKiller->GetClassname());

				 if ( V_strcmp( pKiller->GetClassname(), "npc_zombie" ) == 0)
				killer_weapon_name = "Shambler";
			else if ( V_strcmp( pKiller->GetClassname(), "npc_fastzombie" ) == 0)
				killer_weapon_name = "Banshee";
			else if ( V_strcmp( pKiller->GetClassname(), "npc_poisonzombie" ) == 0)
				killer_weapon_name = "Hulk";
			else if ( V_strcmp( pKiller->GetClassname(), "npc_burnzombie" ) == 0)
				killer_weapon_name = "Immolator";
			else if ( V_strcmp( pKiller->GetClassname(), "npc_dragzombie" ) == 0)
				killer_weapon_name = "Drifter";

			bZombieKill = true;
			//find the ZM id
			for ( int i = 1; i <= gpGlobals->maxClients; i++ )
			{
				CBasePlayer *plr = UTIL_PlayerByIndex( i );

				if ( plr )
				{
					if (plr->GetTeamNumber() == 3)
					{
						killer_ID = plr->GetUserID();
						break;
					}
				}
			}
		}

	}

	IGameEvent *event = gameeventmanager->CreateEvent( "player_death" );
	if( event )
	{
		event->SetInt("userid", pVictim->GetUserID() );
		event->SetInt("attacker", killer_ID );
		event->SetString("weapon", killer_weapon_name );
		//event->SetInt( "priority", 7 );
		event->SetBool("zombie", bZombieKill);
		if (bZombieKill)
			event->SetInt("z_id", pKiller->entindex() );
		else
			event->SetInt("z_id", 0 );

		gameeventmanager->FireEvent( event );
	}
#endif

}

void CHL2MPRules::ClientSettingsChanged( CBasePlayer *pPlayer )
{
#ifndef CLIENT_DLL
	
	CHL2MP_Player *pHL2Player = ToHL2MPPlayer( pPlayer );

	if ( pHL2Player == NULL )
		return;

	const char *pCurrentModel = modelinfo->GetModelName( pPlayer->GetModel() );
	const char *szModelName = engine->GetClientConVarValue( engine->IndexOfEdict( pPlayer->edict() ), "cl_playermodel" );

	//If we're different.
	if ( stricmp( szModelName, pCurrentModel ) )
	{
		//Too soon, set the cvar back to what it was.
		//Note: this will make this function be called again
		//but since our models will match it'll just skip this whole dealio.
		if ( pHL2Player->GetNextModelChangeTime() >= gpGlobals->curtime )
		{
			char szReturnString[512];

			Q_snprintf( szReturnString, sizeof (szReturnString ), "cl_playermodel %s\n", pCurrentModel );
			engine->ClientCommand ( pHL2Player->edict(), szReturnString );

			Q_snprintf( szReturnString, sizeof( szReturnString ), "Please wait %d more seconds before trying to switch.\n", (int)(pHL2Player->GetNextModelChangeTime() - gpGlobals->curtime) );
			ClientPrint( pHL2Player, HUD_PRINTTALK, szReturnString );
			return;
		}

//		if ( HL2MPRules()->IsTeamplay() == false )
//		{
			pHL2Player->SetPlayerModel();

			const char *pszCurrentModelName = modelinfo->GetModelName( pHL2Player->GetModel() );

			char szReturnString[128];
			Q_snprintf( szReturnString, sizeof( szReturnString ), "Your player model is: %s\n", pszCurrentModelName );

			ClientPrint( pHL2Player, HUD_PRINTTALK, szReturnString );
//		}
/*		else
		{
			if ( Q_stristr( szModelName, "models/human") )
			{
				pHL2Player->ChangeTeam( TEAM_ZOMBIEMASTER );
			}
			else
			{
				pHL2Player->ChangeTeam( TEAM_HUMANS );
			}
		}*/ //LAWYER:  No team switches based on model!
	}

	BaseClass::ClientSettingsChanged( pPlayer );
#endif
	
}

int CHL2MPRules::PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget )
{
#ifndef CLIENT_DLL
	// half life multiplay has a simple concept of Player Relationships.
	// you are either on another player's team, or you are not.
	if ( !pPlayer || !pTarget || !pTarget->IsPlayer() || IsTeamplay() == false )
		return GR_NOTTEAMMATE;

	if ( (*GetTeamID(pPlayer) != '\0') && (*GetTeamID(pTarget) != '\0') && !stricmp( GetTeamID(pPlayer), GetTeamID(pTarget) ) )
	{
		return GR_TEAMMATE;
	}
#endif

	return GR_NOTTEAMMATE;
}

const char *CHL2MPRules::GetGameDescription( void )
{ 
//	if ( IsTeamplay() )
//		return "Team Deathmatch"; 

	return "Zombie Master 1.2.1 Open Source"; //LAWYER: REMEMBER!  Change me once each release
} 


float CHL2MPRules::GetMapRemainingTime()
{
	// if timelimit is disabled, return 0
	if ( mp_timelimit.GetInt() <= 0 )
		return 0;

	// timelimit is in minutes

	float timeleft = (m_flGameStartTime + mp_timelimit.GetInt() * 60.0f ) - gpGlobals->curtime;

	return timeleft;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2MPRules::Precache( void )
{
	//TGB: removed its emit
	//CBaseEntity::PrecacheScriptSound( "AlyxEmp.Charge" );
}

bool CHL2MPRules::ShouldCollide( int collisionGroup0, int collisionGroup1 )
{
	//TGB: integrating fix for 'physical mayhem bug' though we have not encountered it yet, can't hurt
	if ( collisionGroup0 > collisionGroup1 )
	{
		// swap so that lowest is always first
		swap(collisionGroup0,collisionGroup1);
	}

	if ( (collisionGroup0 == COLLISION_GROUP_PLAYER || collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT) &&
		collisionGroup1 == COLLISION_GROUP_WEAPON )
	{
		return false;
	}

	if ( collisionGroup0 == COLLISION_GROUP_DEBRIS && collisionGroup1 == COLLISION_GROUP_PUSHAWAY )
	{
		// let debris and multiplayer objects collide
		return true;
	}

	return BaseClass::ShouldCollide( collisionGroup0, collisionGroup1 ); 

}

bool CHL2MPRules::ClientCommand(const char *pcmd, CBaseEntity *pEdict )
{

#ifndef CLIENT_DLL
	if( BaseClass::ClientCommand(pcmd, pEdict) )
		return true;


	CHL2MP_Player *pPlayer = (CHL2MP_Player *) pEdict;

	if ( pPlayer->ClientCommand( pcmd ) )
		return true;
#endif

	return false;
}

// shared ammo definition
// JAY: Trying to make a more physical bullet response
#define BULLET_MASS_GRAINS_TO_LB(grains)	(0.002285*(grains)/16.0f)
#define BULLET_MASS_GRAINS_TO_KG(grains)	lbs2kg(BULLET_MASS_GRAINS_TO_LB(grains))

// exaggerate all of the forces, but use real numbers to keep them consistent
#define BULLET_IMPULSE_EXAGGERATION			3.5
// convert a velocity in ft/sec and a mass in grains to an impulse in kg in/s
#define BULLET_IMPULSE(grains, ftpersec)	((ftpersec)*12*BULLET_MASS_GRAINS_TO_KG(grains)*BULLET_IMPULSE_EXAGGERATION)


CAmmoDef *GetAmmoDef()
{
	static CAmmoDef def;
	static bool bInitted = false;
	
	if ( !bInitted )
	{
		bInitted = true;

		def.AddAmmoType("AR2",				DMG_BULLET,					TRACER_LINE_AND_WHIZ,	0,			0,			60,			BULLET_IMPULSE(200, 1225),	0 );
		def.AddAmmoType("AR2AltFire",		DMG_DISSOLVE,				TRACER_NONE,			0,			0,			3,			0,							0 );
//		def.AddAmmoType("Pistol",			DMG_BULLET,					TRACER_LINE_AND_WHIZ,	0,			0,			150,		BULLET_IMPULSE(200, 1225),	0 );
		def.AddAmmoType("Pistol",			DMG_BULLET,					TRACER_LINE_AND_WHIZ,	0,			0,			80,			BULLET_IMPULSE(200, 1225),	0 );
		def.AddAmmoType("SMG1",				DMG_BULLET,					TRACER_LINE_AND_WHIZ,	0,			0,			60,			BULLET_IMPULSE(200, 1225),	0 );
		def.AddAmmoType("357",				DMG_BULLET,					TRACER_LINE_AND_WHIZ,	0,			0,			20,			BULLET_IMPULSE(800, 5000),	0 );
		def.AddAmmoType("XBowBolt",			DMG_BULLET,					TRACER_LINE,			0,			0,			10,			BULLET_IMPULSE(800, 8000),	0 );
//		def.AddAmmoType("Buckshot",			DMG_BULLET | DMG_BUCKSHOT,	TRACER_LINE,			0,			0,			30,			BULLET_IMPULSE(400, 1200),	0 );
		def.AddAmmoType("Buckshot",			DMG_BULLET | DMG_BUCKSHOT,	TRACER_LINE,			0,			0,			24,			BULLET_IMPULSE(400, 1600),	0 );
		def.AddAmmoType("RPG_Round",		DMG_BURN,					TRACER_NONE,			0,			0,			3,			0,							0 );
		def.AddAmmoType("SMG1_Grenade",		DMG_BURN,					TRACER_NONE,			0,			0,			3,			0,							0 );
		def.AddAmmoType("Grenade",			DMG_BURN,					TRACER_NONE,			0,			0,			5,			0,							0 );
		def.AddAmmoType("slam",				DMG_BURN,					TRACER_NONE,			0,			0,			5,			0,							0 );
		//LAWYER:  Extra ammo types for ZM
		def.AddAmmoType("Molotov",			DMG_BURN,					TRACER_NONE,			0,			0,			1,			0,							0);
		def.AddAmmoType("Revolver",			DMG_BULLET,					TRACER_LINE_AND_WHIZ,	0,			0,			36,			BULLET_IMPULSE(800, 5000),	0 );
	}

	return &def;
}

#ifdef CLIENT_DLL

	ConVar cl_autowepswitch(
		"cl_autowepswitch",
		"1",
		FCVAR_ARCHIVE | FCVAR_USERINFO,
		"Automatically switch to picked up weapons (if more powerful)" );

#else
/*
#ifdef DEBUG

	// Handler for the "bot" command.
	void Bot_f()
	{		
		// Look at -count.
		int count = 1;
		count = clamp( count, 1, 16 );

		int iTeam = TEAM_COMBINE;
				
		// Look at -frozen.
		bool bFrozen = false;
			
		// Ok, spawn all the bots.
		while ( --count >= 0 )
		{
			BotPutInServer( bFrozen, iTeam );
		}
	}


	ConCommand cc_Bot( "bot", Bot_f, "Add a bot.", FCVAR_CHEAT );

#endif
*/
	bool CHL2MPRules::FShouldSwitchWeapon( CBasePlayer *pPlayer, CBaseCombatWeapon *pWeapon )
	{		
		if ( pPlayer->GetActiveWeapon() && pPlayer->IsNetClient() )
		{
			// Player has an active item, so let's check cl_autowepswitch.
			const char *cl_autowepswitch = engine->GetClientConVarValue( engine->IndexOfEdict( pPlayer->edict() ), "cl_autowepswitch" );
			if ( cl_autowepswitch && atoi( cl_autowepswitch ) <= 0 )
			{
				return false;
			}
		}

		return BaseClass::FShouldSwitchWeapon( pPlayer, pWeapon );
	}

#endif


//qck: I rewrote some of this to avoid skipping entites, like before.
void CHL2MPRules::WorldReset( void )
{
#ifndef CLIENT_DLL
	//qck: COMMENT OUT FOR LINUX TEST
	//qck: I found that only worldspawn should be kept out of the whole "reset" phase. Everything else should
	//pass through it, but certain entities can't be passed through UTIL_Remove(). So we setup an array to keep 
	//things out of UTIL_Remove(), and a small, normal filter to keep worldspawn out of ParseAllEntities()

	//qck: Get rid of the entities we don't need to keep for the next round.
	CBaseEntity *pEnt = gEntList.FirstEnt();
	while ( pEnt )
	{	
		if ( !ArraySearch( entitiesToKeep, pEnt->GetClassname(), ARRAYSIZE( entitiesToKeep ) ) )
		{	
			UTIL_Remove( pEnt );
		}

		pEnt = gEntList.NextEnt( pEnt );
	}
	gEntList.CleanupDeleteList();
	g_EventQueue.Clear(); //qck: This clears out the global queue, which stores all entity output. Reset to avoid logic running over into the next round.

	//TGB: lets clear out our nasty global lists as well
	gEntList.m_ZombieList.Purge();
	gEntList.m_BansheeList.Purge();
	gEntList.m_ZombieSelected.Purge();
	//these aren't as round dependent
	//TGB: I put these back in, because we want to be safe and it won't hurt
	gEntList.m_ZombieSpawns.Purge();
	gEntList.m_ZombieManipulates.Purge();
	
	/*qck: Parse all entities using the pristine map entity lump. Spawn, Reset their keyfields, etc.
		   Using CMapEntityFilter with a slight modification to be sure it skips worldspawn */
	/*TGB: I've found this is not the case. Entities we preserve have to be filtered, or they will duplicate.
	 With every preserved ent duplicating on roundrestart, entity limit overflows are unavoidable. */
			
	//TGB: copying the filter in from HL2MP function, adapted for ZM

	// Now reload the map entities.
	class CZMMapEntityFilter : public IMapEntityFilter
	{
	public:
		virtual bool ShouldCreateEntity( const char *pClassname )
		{
			// Don't recreate the preserved entities.
			if ( !ArraySearch( entitiesToKeep, pClassname, ARRAYSIZE( entitiesToKeep ) ) )
			{
				return true;
			}
			else
			{
				// Increment our iterator since it's not going to call CreateNextEntity for this ent.
				if ( m_iIterator != g_MapEntityRefs.InvalidIndex() )
					m_iIterator = g_MapEntityRefs.Next( m_iIterator );

				return false;
			}
		}


		virtual CBaseEntity* CreateNextEntity( const char *pClassname )
		{
			if ( m_iIterator == g_MapEntityRefs.InvalidIndex() )
			{
				// This shouldn't be possible. When we loaded the map, it should have used 
				// CCSMapLoadEntityFilter, which should have built the g_MapEntityRefs list
				// with the same list of entities we're referring to here.
				Assert( false );
				return NULL;
			}
			else
			{
				CMapEntityRef &ref = g_MapEntityRefs[m_iIterator];
				m_iIterator = g_MapEntityRefs.Next( m_iIterator );	// Seek to the next entity.

				if ( ref.m_iEdict == -1 || engine->PEntityOfEntIndex( ref.m_iEdict ) )
				{
					// Doh! The entity was delete and its slot was reused.
					// Just use any old edict slot. This case sucks because we lose the baseline.
					return CreateEntityByName( pClassname );
				}
				else
				{
					// Cool, the slot where this entity was is free again (most likely, the entity was 
					// freed above). Now create an entity with this specific index.
					return CreateEntityByName( pClassname, ref.m_iEdict );
				}
			}
		}

	public:
		int m_iIterator; // Iterator into g_MapEntityRefs.
	};
	CZMMapEntityFilter filter;
	filter.m_iIterator = g_MapEntityRefs.Head();

	// DO NOT CALL SPAWN ON info_node ENTITIES!

	//CMapEntityFilter filter;
	Msg("Parsing entities...\n"); 
	MapEntity_ParseAllEntities( engine->GetMapEntitiesString(), &filter, true );
#endif
}

#ifndef CLIENT_DLL

bool FindInList( const char **pStrings, const char *pToFind )
{
	int i = 0;
	while ( pStrings[i][0] != 0 )
	{
		if ( Q_stricmp( pStrings[i], pToFind ) == 0 )
			return true;
		i++;
	}
	return false;
}

void CHL2MPRules::CheckRestartGame( void )
{
	// Restart the game if specified by the server
	int iRestartDelay = mp_restartgame.GetInt();

	if ( iRestartDelay > 0 )
	{
		if ( iRestartDelay > 60 )
			iRestartDelay = 60;


		// let the players know
		char strRestartDelay[64];
		Q_snprintf( strRestartDelay, sizeof( strRestartDelay ), "%d", iRestartDelay );
		UTIL_ClientPrintAll( HUD_PRINTCENTER, "Game will restart in %s1 %s2", strRestartDelay, iRestartDelay == 1 ? "SECOND" : "SECONDS" );
		UTIL_ClientPrintAll( HUD_PRINTCONSOLE, "Game will restart in %s1 %s2", strRestartDelay, iRestartDelay == 1 ? "SECOND" : "SECONDS" );

		m_flRestartGameTime = gpGlobals->curtime + iRestartDelay;
		m_bCompleteReset = true;
		mp_restartgame.SetValue( 0 );
	}
}
//TGBMERGENOTE: This is Valve's implementation of roundrestart. It speaks of combine and rebels and therefore errors, but we may be able to copy stuff from it to enhance our implementation.

void CHL2MPRules::RestartGame()
{
	DevMsg("RUNNING VALVE'S RESTARTGAME OH NOE\n");
/*	// bounds check
	if ( mp_timelimit.GetInt() < 0 )
	{
		mp_timelimit.SetValue( 0 );
	}
	m_flGameStartTime = gpGlobals->curtime;
	if ( !IsFinite( m_flGameStartTime.Get() ) )
	{
		Warning( "Trying to set a NaN game start time\n" );
		m_flGameStartTime.GetForModify() = 0.0f;
	}

	CleanUpMap();
	
	// now respawn all players
	for (int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CHL2MP_Player *pPlayer = (CHL2MP_Player*) UTIL_PlayerByIndex( i );

		if ( !pPlayer )
			continue;

		if ( pPlayer->GetActiveWeapon() )
		{
			pPlayer->GetActiveWeapon()->Holster();
		}
		pPlayer->RemoveAllItems( true );
		respawn( pPlayer, false );
		pPlayer->Reset();
	}

	// Respawn entities (glass, doors, etc..)

	CTeam *pRebels = GetGlobalTeam( TEAM_REBELS );
	CTeam *pCombine = GetGlobalTeam( TEAM_COMBINE );

	if ( pRebels )
	{
		pRebels->SetScore( 0 );
	}

	if ( pCombine )
	{
		pCombine->SetScore( 0 );
	}

	m_flIntermissionEndTime = 0;
	m_flRestartGameTime = 0.0;		
	m_bCompleteReset = false;

	IGameEvent * event = gameeventmanager->CreateEvent( "round_start" );
	if ( event )
	{
		event->SetInt("fraglimit", 0 );
		event->SetInt( "priority", 6 ); // HLTV event priority, not transmitted

		event->SetString("objective","DEATHMATCH");

		gameeventmanager->FireEvent( event );
	}*/
}

void CHL2MPRules::CleanUpMap()
{
	// Recreate all the map entities from the map data (preserving their indices),
	// then remove everything else except the players.

	// Get rid of all entities except players.
	CBaseEntity *pCur = gEntList.FirstEnt();
	while ( pCur )
	{
		CBaseHL2MPCombatWeapon *pWeapon = dynamic_cast< CBaseHL2MPCombatWeapon* >( pCur );
		// Weapons with owners don't want to be removed..
		if ( pWeapon )
		{
			if ( !pWeapon->GetPlayerOwner() )
			{
				UTIL_Remove( pCur );
			}
		}
		// remove entities that has to be restored on roundrestart (breakables etc)
		else if ( !FindInList( s_PreserveEnts, pCur->GetClassname() ) )
		{
			UTIL_Remove( pCur );
		}

		pCur = gEntList.NextEnt( pCur );
	}

	// Really remove the entities so we can have access to their slots below.
	gEntList.CleanupDeleteList();

	// Cancel all queued events, in case a func_bomb_target fired some delayed outputs that
	// could kill respawning CTs
	g_EventQueue.Clear();

	// Now reload the map entities.
	class CHL2MPMapEntityFilter : public IMapEntityFilter
	{
	public:
		virtual bool ShouldCreateEntity( const char *pClassname )
		{
			// Don't recreate the preserved entities.
			if ( !FindInList( s_PreserveEnts, pClassname ) )
			{
				return true;
			}
			else
			{
				// Increment our iterator since it's not going to call CreateNextEntity for this ent.
				if ( m_iIterator != g_MapEntityRefs.InvalidIndex() )
					m_iIterator = g_MapEntityRefs.Next( m_iIterator );

				return false;
			}
		}


		virtual CBaseEntity* CreateNextEntity( const char *pClassname )
		{
			if ( m_iIterator == g_MapEntityRefs.InvalidIndex() )
			{
				// This shouldn't be possible. When we loaded the map, it should have used 
				// CCSMapLoadEntityFilter, which should have built the g_MapEntityRefs list
				// with the same list of entities we're referring to here.
				Assert( false );
				return NULL;
			}
			else
			{
				CMapEntityRef &ref = g_MapEntityRefs[m_iIterator];
				m_iIterator = g_MapEntityRefs.Next( m_iIterator );	// Seek to the next entity.

				if ( ref.m_iEdict == -1 || engine->PEntityOfEntIndex( ref.m_iEdict ) )
				{
					// Doh! The entity was delete and its slot was reused.
					// Just use any old edict slot. This case sucks because we lose the baseline.
					return CreateEntityByName( pClassname );
				}
				else
				{
					// Cool, the slot where this entity was is free again (most likely, the entity was 
					// freed above). Now create an entity with this specific index.
					return CreateEntityByName( pClassname, ref.m_iEdict );
				}
			}
		}

	public:
		int m_iIterator; // Iterator into g_MapEntityRefs.
	};
	CHL2MPMapEntityFilter filter;
	filter.m_iIterator = g_MapEntityRefs.Head();

	// DO NOT CALL SPAWN ON info_node ENTITIES!

	MapEntity_ParseAllEntities( engine->GetMapEntitiesString(), &filter, true );
}

//TGB: endround helpers

//TGB: any players present?
bool HavePlayers()
{
	for (int i = 0; i < g_Teams.Count(); i++)
	{
		if (g_Teams[i] && g_Teams[i]->GetNumPlayers() > 0)
		{
			return true;
		}
	}

	return false;
}

#endif

//TGB: New EndRound function
void CHL2MPRules::EndRound()
{
#ifndef CLIENT_DLL
	/*
	TGB: of 16nov08, this is what happens during roundrestart:

		1. fire round_restart event
		2. clear lagcompensation history
		3. destroy zm_group_manager			(seems redundant, as it'd be killed by WorldReset, but can't hurt)
		4. count players and remove their weapons
		5. IF NO PLAYERS, RR ENDS HERE

		6. reset players to the survivor team, also find the player with highest ZMing priority
		7. build some lists to use in ZM picking (more detail in the relevant code)
		8. pick ZM and move him to the right team
		9. RESET WORLD
	   10. spawn players
	   11. process info_loadout


		there are some potential issues with this. For example, why is the world only reset with no
		players present? It seems healthy to reset the world regularly, though it would be a bit odd
		if our current memleak comes from there, seeing as servers with active players often get RRs
		with proper worldresets, and still have memleaks. Might be better to leave that alone until
		other options have been exhausted.

		The bits that occur before the check need looking at. Also, the check should perhaps be
		moved up, as that would be more logical: the RR event would only be fired for an actual RR, 
		etc. Checking for players is trivial by getting at the CTeam classes.
	*/

	//TGB: made player check occur earlier [19nov08]


	Msg("Round ending...\n");

	if (HavePlayers() == false)
	{
		Msg("No players present, aborting round restart.\n");
		return;
	}

	//TGB: go ahead with the RR

	UTIL_ClientPrintAll(HUD_PRINTTALK, "Round has ended. Starting new round...\n");

	//LAWYER:  -----Perform clean-up operations-----

	//TGB: currently only exists to tell server plugins about the restart
	IGameEvent * event = gameeventmanager->CreateEvent( "round_restart" );
	if ( event )
	{
		//TGB: below seems useless, our modevents.res has no properties set
// 		event->SetInt("fraglimit", 0 );
// 		event->SetInt( "priority", 6 ); // HLTV event priority, not transmitted
// 
// 		event->SetString("objective","DEATHMATCH");

		gameeventmanager->FireEvent( event );
	}

	//TGB: clear lagcomp history
	lagcompensation->ClearHistory();

	//qck: Clean up groups

	CBaseEntity *pManEnt = gEntList.FindEntityByClassname(NULL, "zm_group_manager");
	if (pManEnt)
	{
		CZombieGroupManager* pManager = dynamic_cast<CZombieGroupManager*>(pManEnt);
		if( pManager )
		{
			//TGB: ack! just calling the destructor does not deallocate the memory! delete does both and should be used! exclamation!
			//pManager->~CZombieGroupManager();
			//delete pManager;

			//TGB: rather than deleting directly, I'd prefer to use UTIL_Remove here
			//	probably not a big difference, but can't be too careful
			UTIL_Remove(pManager);

			DevMsg("Reset server group manager\n");
		}
	}

	//int iNumPlayers = 0; //init player counting var

	//Msg("Counting players AND Stripping weapons...\n");
	Msg("Stripping weapons...\n");
	//TGB: Count players AND strip weapons
	//LAWYER:  -----Count the players-----
	//LAWYER:  -----Weapon Strip-----
	for (int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		//TGB:should be possible, but is unnecessary right now
		//CHL2MP_Player *pPlayer = dynamic_cast< CHL2MP_Player * >(UTIL_PlayerByIndex( i ));

		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

		if ( pPlayer )
		{
			// TODO: refactor into a Reset() function for players? [19-11-2008]

			pPlayer->m_iZombieGroup = NULL;
			//TGB: count player
			//iNumPlayers++;

			//TGB: 0000106 make sure players don't get to keep ammo
			pPlayer->RemoveAllAmmo();

			//LAWYER: the actual stripping component
			for ( int i = 0 ; i < pPlayer->WeaponCount(); ++i )
			{
				CBaseCombatWeapon *pWeapon = pPlayer->GetWeapon( i );
				if ( !pWeapon )
					continue;

				DevMsg("Removing weapon #%i: %s\n", i, pWeapon->GetClassname());

				pPlayer->Weapon_Drop( pWeapon, (0,0,0), (0,0,0) );
				UTIL_Remove( pWeapon );
			}
			DevMsg("After stripping weapons %i remain\n", pPlayer->WeaponCount());
		}
	}

	//TGB: 0-player check moved to occur before anything else, makes more sense that way

	//TGB: 0-player check
// 	if (iNumPlayers == 0)
// 	{
// 		Msg("Cannot RoundRestart, there are no (connected) players!\n"); //LAWYER:  Don't restart without players!
// 		return;
// 	}
	
	/* TGB: 
		Right, I want to add some randomization here, but looking at it I don't like the multiple spawns.
		UPDATE: modified ChangeTeam not to spawn a player yet, so we can shuffle them around all we want.
	*/

	CHL2MP_Player *pZMPlayer = NULL;

	Msg("Resetting players to survivor team...\n");

	int iHighest = -1; //we already want to know the highest priority level
	for (int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CHL2MP_Player *pPlayerMP = dynamic_cast< CHL2MP_Player * >(UTIL_PlayerByIndex( i ));
		
		if (pPlayerMP) //Flash everyone onto the Human team
		{
			//LAWYER:  Reset RR will
			pPlayerMP->m_iZMVoteRR = 0;
			pPlayerMP->ChangeTeamDelayed(TEAM_HUMANS); //set player as human
			//Try to kill crazy lists	
			pPlayerMP->ShowViewPortPanel( PANEL_START, false );

			//TGB: determine highest ZM priority of all players
			if (pPlayerMP->m_iZMPriority > iHighest && pPlayerMP->m_zmParticipation == 0)
				iHighest = pPlayerMP->m_iZMPriority;
		}
	}

	/*TGB:
		The below works like this:
			1. Dump all ZMs that have the highest priority in a list
			2. Dump all other non-observers in a list
			3. If the list from 1 has players in it, randomly pick one as ZM
			4. Else, randomly pick one from list 2 as ZM
	*/

	Msg("Building list of potential ZM players...\n");

	//TGB: build a list of all players with the top priority
	CUtlVector<CHL2MP_Player*> pZMs;
	//TGB: and a list of all players that prefer survivor and/or do not have top priority
	CUtlVector<CHL2MP_Player*> pEmergencyZMs;

	pZMs.Purge();
	pEmergencyZMs.Purge();
	for (int i = 1; i <= gpGlobals->maxClients; i++ ) //Now check, assuming there are available players
	{
		CHL2MP_Player *pPlayerMP = dynamic_cast< CHL2MP_Player * >(UTIL_PlayerByIndex( i ));
		if (pPlayerMP)
		{
			//TGB: this was below the continues before. However, when we want an emergency ZM, we
			//	want anyone at all. Doesn't make sense to exclude observers etc.
			//Emergency ZM? = any priority + any willingness
			pEmergencyZMs.AddToHead(pPlayerMP); //add this player to the list of emergency ZMs

			//Observer?
			if (pPlayerMP->m_zmParticipation == 2) 
			{
				pPlayerMP->ChangeTeamDelayed(TEAM_SPECTATOR); //LAWYER:  This player doesn't want to play in this round
				continue; //NEXT!
			}

			//First-choice ZM? = high priority + wants to zm
			if ( pPlayerMP->m_iZMPriority == iHighest && pPlayerMP->m_zmParticipation == 0)
			{
				pZMs.AddToHead(pPlayerMP); //add this player to the list of first-choice ZMs
				continue; //NEXT
			}
		}
	}

	//TGB: we now have two lists: 
	//one with people we would typically choose as ZM, ie. w/ highest priority and zm-willingness
	//one with people that we will only choose if no other options are available

	//TGB: ----- SELECT ZM -----

	if (pZMs.Count() > 0) //TGB: got any good candidates?
	{
		Msg("Picking willing ZM...\n");
		//yuppers, select a random one
		const int randnum = random->RandomInt(0, pZMs.Count() - 1);
		pZMPlayer = pZMs[randnum]; //select a random one

		if (!pZMPlayer)
			DevWarning("Invalid player in ZMs list!\n");
	}
	else //oh sh, gotta use the emergency list
	{
		//LAWYER:  1-player check
		//TGB: did not seem useful: all players are in the pEmergencyZMs list, the single player
		//	will therefore always get picked.
		/*if (iNumPlayers == 1) //nom nom nom
		{
			CBaseEntity *pOnePlayer = NULL;
			pOnePlayer = gEntList.FindEntityByClassname( pOnePlayer, "player" );
			pZMPlayer = dynamic_cast< CHL2MP_Player * >(pOnePlayer);
		}
		else
		{
			Msg("Picking unwilling ZM as no willing ones are available...\n");
			const int randnum = random->RandomInt(0, pEmergencyZMs.Count() - 1);
			pZMPlayer = pEmergencyZMs[randnum];
		}*/

		Msg("Picking unwilling ZM as no willing ones are available...\n");
		const int randnum = random->RandomInt(0, pEmergencyZMs.Count() - 1);
		pZMPlayer = pEmergencyZMs[randnum];

		if (!pZMPlayer)
			DevWarning("Invalid player in emergency ZMs list!\n");
	}
	
	//LAWYER:  -----Emergency player picker, in case of all players being unwilling-----
	//TGB: super last chance check, should never occur
	while (pZMPlayer == NULL)
	{
		Msg("Attempting emergency ZM player pick(s)...\n");
		CBaseEntity *pForcedZM = NULL;
		pForcedZM = gEntList.FindEntityByClassname( pForcedZM, "player" );
		pZMPlayer = dynamic_cast< CHL2MP_Player * >(pForcedZM);
	}


	//TGB: ----- ZOMBIEMASTERIFY THE CHOSEN ZM ----- 
	if (pZMPlayer)
	{
		Msg("Moving chosen ZM to Zombie Master position...\n");
		//TGB: reset ZMpriority
		pZMPlayer->m_iZMPriority = 0;
		pZMPlayer->ChangeTeamDelayed(3);
	}

	//LAWYER:  -----Reset the world-----
	Msg("Resetting entities...\n");
	WorldReset();
	
	//LAWYER: -----Spawn the players-----
	Msg("Spawning players...\n");
	for (int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
		CHL2MP_Player *pPlayerMP = NULL;
		pPlayerMP = dynamic_cast< CHL2MP_Player * >(pPlayer);

		if (pPlayerMP)
		{
			//TGB: handled in spawn() now
			//pPlayer->m_iWeaponFlags = 0; //LAWYER:  Reset weaponflags.
			
			//TGB: cleaned up a bit

			//observers get off easy
			if (pPlayerMP->m_bObserve)
				continue;

			//show participation picker
			if (pPlayerMP->m_zmParticipation == 3)
			{
				pPlayerMP->ShowViewPortPanel( PANEL_START, true ); //LAWYER:  Show them the start panel again, if they've not picked
			}
		
			//TGB: everyone being spawned here gets a ZMrating increase, if not the ZM
			if (pPlayerMP->GetTeamNumber() != 3)
			{
                pPlayerMP->m_iZMPriority += 10;
			}
			
			//LAWYER:  Display map objectives on spawn
			KeyValues *data = new KeyValues("data");
			if (data)
			{
				data->SetString( "title", "Objectives" );		// info panel title
				data->SetString( "type", "1" );			// show userdata from stringtable entry
				data->SetString( "msg",	"mapinfo" );		// use this stringtable entry

				pPlayerMP->ShowViewPortPanel( PANEL_INFO, true, data );

				data->deleteThis();
			}

			//TGB: finally spawn this chap
			pPlayerMP->Spawn();
		
		}
				
	}

	//LAWYER:  Increase the roundcounter!

	m_flRoundStartTime = gpGlobals->curtime; //we just started a new round
	m_iRoundsCompleted++;

	//LAWYER:  Apply any weapons!
	CBaseEntity *pFound = NULL;
	m_pLoadOut = NULL;
	pFound = gEntList.FindEntityByClassname( pFound, "info_loadout" );
	Msg("Finding a Loadout entity...\n");
	if (pFound)
	{
		m_pLoadOut = dynamic_cast<CZombieMaster_LoadOut*>( pFound );
		if (m_pLoadOut)
		{
			Msg("Found a Loadout! Distributing...\n");
			m_pLoadOut->Distribute();
		}
	}
	else
	{
		Msg("No loadout.\n");
	}



	Msg("RoundRestart complete!\n");
#endif
} //EndRound (new)


//#ifdef CLIENT_DLL
//void CHL2MPRules::PreRender( )
//{
//
//}
//#endif















////TGB: This is the OLD endround function. I just know I'm going to accidentally code in this one so:
////OLDOLDOLDOLDOLDOLDOLDOLDOLDOLDOLDOLDOLD
//void CHL2MPRules::EndRoundOld()
//{
//#ifndef CLIENT_DLL
//
//
//	Msg("Round ending...\n");
//	//LAWYER:  -----Perform clean-up operations-----
//	
//	//LAWYER:  -----Count the players-----
//	int iNumPlayers = 0;
//
//	Msg("Counting players...\n");
//	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
//	{
//		CBaseEntity *plr = UTIL_PlayerByIndex( i );
//
//		if ( plr )
//		{
//			iNumPlayers++;
//		}
//	} 
//
//	if (iNumPlayers == 0)
//	{
//		Msg("Cannot RoundRestart, there are no players!\n"); //LAWYER:  Don't restart without players!
//		return;
//	}
//
//
//	Msg("Stripping weapons...\n");
//	//LAWYER:  -----Weapon Strip-----
//	for (int i = 1; i <= gpGlobals->maxClients; i++ )
//	{
//		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
//		if ( pPlayer )
//		{
//			//LAWYER: the actual stripping component
//			for ( int i = 0 ; i < pPlayer->WeaponCount(); ++i )
//			{
//
//				CBaseCombatWeapon *pWeapon = pPlayer->GetWeapon( i );
//				if ( !pWeapon )
//					continue;
//
//				pPlayer->Weapon_Drop( pWeapon, (0,0,0), (0,0,0) );
//				UTIL_Remove( pWeapon );
//			}
//		}
//	}
//
//	//LAWYER:  -----Humanise all willing players-----
//	
//	Msg("Humanising teams...\n");
//	CHL2MP_Player *pPlayerLastMaster = NULL;
//	for (int i = 1; i <= gpGlobals->maxClients; i++ )
//	{
//		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
//		CHL2MP_Player *pPlayerMP = NULL;
//
//			pPlayerMP = dynamic_cast< CHL2MP_Player * >(pPlayer);
//			if (pPlayerMP)
//			{
//				if (pPlayerMP->m_bObserve == false)
//				{
//					pPlayerMP->ChangeTeam(2); //LAWYER:  Force all players to the Human team
//					if (pPlayerMP->GetTeamNumber() == 3)
//					{
//						pPlayerLastMaster = pPlayerMP; //LAWYER: Remember the Zombiemaster!
//					}
//				}
//				else if (pPlayerMP->m_bObserve == true)
//				{
//						pPlayerMP->ChangeTeam(TEAM_SPECTATOR); //LAWYER:  This player doesn't want to play in this round
//				}
//			}
//				
//	}
//	
//	//LAWYER: -----Attempt to pick a Zombie Master, try up to 30 times-----
//	CBaseEntity *pSpot = NULL;
//	CHL2MP_Player *pPlayerMPAgain = NULL;
//	int iIterations = 0;
//	Msg("Picking a Zombie Master...\n");
//	while (!pSpot && iIterations < 30)
//	{
//			for ( int i = random->RandomInt(1,(iNumPlayers)); i > 0; i-- )
//			{
//				pSpot = gEntList.FindEntityByClassname( pSpot, "player" );
//			}
//			if ( !pSpot )  // skip over the null point
//				pSpot = gEntList.FindEntityByClassname( pSpot, "player" );
//			
//		pPlayerMPAgain = dynamic_cast< CHL2MP_Player * >(pSpot);
//		if (pPlayerMPAgain) //LAWYER:  do a check to see if this person is a valid ZombieMaster
//		{
//			if (pPlayerLastMaster)
//			{
//				if (!(pPlayerLastMaster == pPlayerMPAgain || pPlayerMPAgain->m_bObserve == true || pPlayerMPAgain->m_bDontWannaZM == true))  
//				//LAWYER:  People who have been the ZombieMaster just before, and people who want to remain observers
//				//will not be picked.  Also, we should add a "Don't want to be ZM!" toggle
//				{
//					pPlayerMPAgain->ChangeTeam(3);
//				}
//			}
//			else
//			{
//				if (pPlayerMPAgain->m_bObserve == false || pPlayerMPAgain->m_bDontWannaZM == true)  
//				{
//					pPlayerMPAgain->ChangeTeam(3);
//				}
//
//			}
//		}
//		iIterations++;
//	}
//	//LAWYER:  -----Emergency player picker, in case of all players being unwilling-----
//	while (!pSpot)
//	{
//		pSpot = gEntList.FindEntityByClassname( pSpot, "player" );
//		pPlayerMPAgain = dynamic_cast< CHL2MP_Player * >(pSpot);
//		if (pPlayerMPAgain) //LAWYER:  do a check to see if this person is a valid ZombieMaster
//		{
//			pPlayerMPAgain->ChangeTeam(3);
//
//		}
//	}
//	Msg("Spawning players...\n");
//	//LAWYER: -----Spawn the players-----
//	for (int i = 1; i <= gpGlobals->maxClients; i++ )
//	{
//		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
//		CHL2MP_Player *pPlayerMP = NULL;
//		pPlayerMP = dynamic_cast< CHL2MP_Player * >(pPlayer);
//
//			if (pPlayerMP)
//			{
//				if (pPlayerMP->m_bObserve == false)
//				{
//					pPlayerMP->Spawn();
//				}
//			}
//				
//	}
//	
//	//LAWYER:  -----Reset the world-----
//	Msg("Resetting entities...\n");
//	WorldReset();
//	Msg("RoundRestart complete!\n");
//	
//#endif
//} //endroundOLD


