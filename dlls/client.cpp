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
//
//=============================================================================//
/*

===== client.cpp ========================================================

  client/server game specific stuff

*/

#include "cbase.h"
#include "player.h"
#include "client.h"
#include "soundent.h"
#include "gamerules.h"
#include "game.h"
#include "physics.h"
#include "entitylist.h"
#include "shake.h"
#include "globalstate.h"
#include "event_tempentity_tester.h"
#include "ndebugoverlay.h"
#include "engine/IEngineSound.h"
#include <ctype.h>
#include "vstdlib/strtools.h"
#include "te_effect_dispatch.h"
#include "globals.h"
#include "nav_mesh.h"
#include "team.h"
#include "hl2mp_gamerules.h"
#include "hl2mp_player.h"
#include "viewport_panel_names.h"
#include "items.h" //for ammo giving
#include "world.h" //qck: This must be last for Linux. Please don't move. 


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern int giPrecacheGrunt;

// For not just using one big ai net
extern CBaseEntity*	FindPickerEntity( CBasePlayer* pPlayer );

ConVar  *sv_cheats = NULL;
/*
============
ClientKill

Player entered the suicide command

============
*/
void ClientKill( edict_t *pEdict )
{
	CBasePlayer *pl = (CBasePlayer*) GetContainingEntity( pEdict );
	pl->CommitSuicide();
}

char * CheckChatText( char *text )
{
	char *p = text;

	// invalid if NULL or empty
	if ( !text || !text[0] )
		return NULL;

	int length = Q_strlen( text );

	// remove quotes (leading & trailing) if present
	if (*p == '"')
	{
		p++;
		length -=2;
		p[length] = 0;
	}

	// cut off after 127 chars
	if ( length > 127 )
		text[127] = 0;

	return p;
}

//// HOST_SAY
// String comes in as
// say blah blah blah
// or as
// blah blah blah
//
void Host_Say( edict_t *pEdict, bool teamonly )
{
	CBasePlayer *client;
	int		j;
	char	*p;
	char	text[256];
	char    szTemp[256];
	const char *cpSay = "say";
	const char *cpSayTeam = "say_team";
	const char *pcmd = engine->Cmd_Argv(0);
	bool bSenderDead = false;

	// We can get a raw string now, without the "say " prepended
	if ( engine->Cmd_Argc() == 0 )
		return;

	if ( !stricmp( pcmd, cpSay) || !stricmp( pcmd, cpSayTeam ) )
	{
		if ( engine->Cmd_Argc() >= 2 )
		{
			p = (char *)engine->Cmd_Args();
		}
		else
		{
			// say with a blank message, nothing to do
			return;
		}
	}
	else  // Raw text, need to prepend argv[0]
	{
		if ( engine->Cmd_Argc() >= 2 )
		{
			Q_snprintf( szTemp,sizeof(szTemp), "%s %s", ( char * )pcmd, (char *)engine->Cmd_Args() );
		}
		else
		{
			// Just a one word command, use the first word...sigh
			Q_snprintf( szTemp,sizeof(szTemp), "%s", ( char * )pcmd );
		}
		p = szTemp;
	}

	// make sure the text has valid content
	p = CheckChatText( p );

	if ( !p )
		return;

	CBasePlayer *pPlayer = NULL;
	if ( pEdict )
	{
		pPlayer = ((CBasePlayer *)CBaseEntity::Instance( pEdict ));

		Assert( pPlayer );

		if ( !pPlayer->CanSpeak() )
			return;

		// See if the player wants to modify of check the text
		pPlayer->CheckChatText( p, 127 );	// though the buffer szTemp that p points to is 256, 
											// chat text is capped to 127 in CheckChatText above

		Assert( strlen( pPlayer->GetPlayerName() ) > 0 );

		bSenderDead = ( pPlayer->m_lifeState != LIFE_ALIVE );
	}
	else
	{
		bSenderDead = false;
	}

	const char *pszFormat = NULL;
	const char *pszPrefix = NULL;
	const char *pszLocation = NULL;
	if ( g_pGameRules )
	{
		pszFormat = g_pGameRules->GetChatFormat( teamonly, pPlayer );
		pszPrefix = g_pGameRules->GetChatPrefix( teamonly, pPlayer );	
		pszLocation = g_pGameRules->GetChatLocation( teamonly, pPlayer );
	}

	const char *pszPlayerName = pPlayer ? pPlayer->GetPlayerName():"Console";

	if ( pszPrefix && strlen( pszPrefix ) > 0 )
	{
		if ( pszLocation && strlen( pszLocation ) )
		{
			Q_snprintf( text, sizeof(text), "%s %s @ %s: ", pszPrefix, pszPlayerName, pszLocation );
		}
		else
		{
			Q_snprintf( text, sizeof(text), "%s %s: ", pszPrefix, pszPlayerName );
		}
	}
	else
	{
		Q_snprintf( text, sizeof(text), "%s: ", pszPlayerName );
	}

	j = sizeof(text) - 2 - strlen(text);  // -2 for /n and null terminator
	if ( (int)strlen(p) > j )
		p[j] = 0;

	Q_strncat( text, p, sizeof( text ), COPY_ALL_CHARACTERS );
	Q_strncat( text, "\n", sizeof( text ), COPY_ALL_CHARACTERS );

	// loop through all players
	// Start with the first player.
	// This may return the world in single player if the client types something between levels or during spawn
	// so check it, or it will infinite loop

	client = NULL;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		client = UTIL_PlayerByIndex( i );
		if ( !client || !client->edict() )
			continue;
		
		if ( client->edict() == pEdict )
			continue;

		if ( !(client->IsNetClient()) )	// Not a client ? (should never be true)
			continue;

		if ( teamonly && g_pGameRules->PlayerCanHearChat( client, pPlayer ) != GR_TEAMMATE )
			continue;

		if ( !client->CanHearChatFrom( pPlayer ) )
			continue;

		CSingleUserRecipientFilter user( client );
		user.MakeReliable();

		if ( pszFormat )
		{
			UTIL_SayText2Filter( user, pPlayer, true, pszFormat, pszPlayerName, p, pszLocation );
		}
		else
		{
			UTIL_SayTextFilter( user, text, pPlayer, true );
		}
	}

	if ( pPlayer )
	{
		// print to the sending client
		CSingleUserRecipientFilter user( pPlayer );
		user.MakeReliable();

		if ( pszFormat )
		{
			UTIL_SayText2Filter( user, pPlayer, true, pszFormat, pszPlayerName, p, pszLocation );
		}
		else
		{
			UTIL_SayTextFilter( user, text, pPlayer, true );
		}
	}

	// echo to server console
	// Adrian: Only do this if we're running a dedicated server since we already print to console on the client.
	if ( engine->IsDedicatedServer() )
		 Msg( "%s", text );

	Assert( p );

	int userid = 0;
	const char *networkID = "Console";
	const char *playerName = "Console";
	const char *playerTeam = "Console";
	if ( pPlayer )
	{
		userid = pPlayer->GetUserID();
		networkID = pPlayer->GetNetworkIDString();
		playerName = pPlayer->GetPlayerName();
		CTeam *team = pPlayer->GetTeam();
		if ( team )
		{
			playerTeam = team->GetName();
		}
	}
		
	if ( teamonly )
		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" say_team \"%s\"\n", playerName, userid, networkID, playerTeam, p );
	else
		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" say \"%s\"\n", playerName, userid, networkID, playerTeam, p );

	IGameEvent * event = gameeventmanager->CreateEvent( "player_say" );

	if ( event )	// will be null if there are no listeners!
	{
		event->SetInt("userid", userid );
		event->SetString("text", p );
		event->SetInt("priority", 1 );	// HLTV event priority, not transmitted
		gameeventmanager->FireEvent( event );
	}
}


void ClientPrecache( void )
{
	// Precache cable textures.
	CBaseEntity::PrecacheModel( "cable/cable.vmt" );	
	CBaseEntity::PrecacheModel( "cable/cable_lit.vmt" );	
	CBaseEntity::PrecacheModel( "cable/chain.vmt" );	
	CBaseEntity::PrecacheModel( "cable/rope.vmt" );	
	CBaseEntity::PrecacheModel( "sprites/blueglow1.vmt" );	
	CBaseEntity::PrecacheModel( "sprites/purpleglow1.vmt" );	
	CBaseEntity::PrecacheModel( "sprites/purplelaser1.vmt" );	
	
	CBaseEntity::PrecacheScriptSound( "Player.FallDamage" );
	CBaseEntity::PrecacheScriptSound( "Player.Swim" );

	// General HUD sounds
	CBaseEntity::PrecacheScriptSound( "Player.PickupWeapon" );
	CBaseEntity::PrecacheScriptSound( "Player.DenyWeaponSelection" );
	CBaseEntity::PrecacheScriptSound( "Player.WeaponSelected" );
	CBaseEntity::PrecacheScriptSound( "Player.WeaponSelectionClose" );
	CBaseEntity::PrecacheScriptSound( "Player.WeaponSelectionMoveSlot" );

	// General legacy temp ents sounds
	CBaseEntity::PrecacheScriptSound( "Bounce.Glass" );
	CBaseEntity::PrecacheScriptSound( "Bounce.Metal" );
	CBaseEntity::PrecacheScriptSound( "Bounce.Flesh" );
	CBaseEntity::PrecacheScriptSound( "Bounce.Wood" );
	CBaseEntity::PrecacheScriptSound( "Bounce.Shrapnel" );
	CBaseEntity::PrecacheScriptSound( "Bounce.ShotgunShell" );
	CBaseEntity::PrecacheScriptSound( "Bounce.Shell" );
	CBaseEntity::PrecacheScriptSound( "Bounce.Concrete" );

	ClientGamePrecache();
}

CON_COMMAND_F( cast_ray, "Tests collision detection", FCVAR_CHEAT )
{
	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	
	Vector forward;
	trace_t tr;

	pPlayer->EyeVectors( &forward );
	Vector start = pPlayer->EyePosition();
	UTIL_TraceLine(start, start + forward * MAX_COORD_RANGE, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr );

	if ( tr.DidHit() )
	{
		DevMsg(1, "Hit %s\nposition %.2f, %.2f, %.2f\nangles %.2f, %.2f, %.2f\n", tr.m_pEnt->GetClassname(),
			tr.m_pEnt->GetAbsOrigin().x, tr.m_pEnt->GetAbsOrigin().y, tr.m_pEnt->GetAbsOrigin().z,
			tr.m_pEnt->GetAbsAngles().x, tr.m_pEnt->GetAbsAngles().y, tr.m_pEnt->GetAbsAngles().z );
		DevMsg(1, "Hit: hitbox %d, hitgroup %d, physics bone %d, solid %d, surface %s, surfaceprop %s\n", tr.hitbox, tr.hitgroup, tr.physicsbone, tr.m_pEnt->GetSolid(), tr.surface.name, physprops->GetPropName( tr.surface.surfaceProps ) );
		NDebugOverlay::Line( start, tr.endpos, 0, 255, 0, false, 10 );
		NDebugOverlay::Line( tr.endpos, tr.endpos + tr.plane.normal * 12, 255, 255, 0, false, 10 );
	}
}

CON_COMMAND_F( cast_hull, "Tests hull collision detection", FCVAR_CHEAT )
{
	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	
	Vector forward;
	trace_t tr;

	Vector extents;
	extents.Init(16,16,16);
	pPlayer->EyeVectors( &forward );
	Vector start = pPlayer->EyePosition();
	UTIL_TraceHull(start, start + forward * MAX_COORD_RANGE, -extents, extents, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr );
	if ( tr.DidHit() )
	{
		DevMsg(1, "Hit %s\nposition %.2f, %.2f, %.2f\nangles %.2f, %.2f, %.2f\n", tr.m_pEnt->GetClassname(),
			tr.m_pEnt->GetAbsOrigin().x, tr.m_pEnt->GetAbsOrigin().y, tr.m_pEnt->GetAbsOrigin().z,
			tr.m_pEnt->GetAbsAngles().x, tr.m_pEnt->GetAbsAngles().y, tr.m_pEnt->GetAbsAngles().z );
		DevMsg(1, "Hit: hitbox %d, hitgroup %d, physics bone %d, solid %d, surface %s, surfaceprop %s\n", tr.hitbox, tr.hitgroup, tr.physicsbone, tr.m_pEnt->GetSolid(), tr.surface.name, physprops->GetPropName( tr.surface.surfaceProps ) );
		NDebugOverlay::SweptBox( start, tr.endpos, -extents, extents, vec3_angle, 0, 0, 255, 0, 10 );
		Vector end = tr.endpos;// - tr.plane.normal * DotProductAbs( tr.plane.normal, extents );
		NDebugOverlay::Line( end, end + tr.plane.normal * 24, 255, 255, 64, false, 10 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Used to find targets for ent_* commands
//			Without a name, returns the entity under the player's crosshair.
//			With a name it finds entities via name/classname/index
//-----------------------------------------------------------------------------
CBaseEntity *GetNextCommandEntity( CBasePlayer *pPlayer, char *name, CBaseEntity *ent )
{
	if ( !pPlayer )
		return NULL;

	// If no name was given set bits based on the picked
	if (FStrEq(name,"")) 
	{
		// If we've already found an entity, return NULL. 
		// Makes it easier to write code using this func.
		if ( ent )
			return NULL;

		return FindPickerEntity( pPlayer );
	}

	int index = atoi( name );
	if ( index )
	{
		// If we've already found an entity, return NULL. 
		// Makes it easier to write code using this func.
		if ( ent )
			return NULL;

		return CBaseEntity::Instance( index );
	}
		
	// Loop through all entities matching, starting from the specified previous
	while ( (ent = gEntList.NextEnt(ent)) != NULL )
	{
		if (  (ent->GetEntityName() != NULL_STRING	&& ent->NameMatches(name))	|| 
			  (ent->m_iClassname != NULL_STRING && ent->ClassMatches(name)) )
		{
			return ent;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: called each time a player uses a "cmd" command
// Input  : pPlayer - the player who issued the command
//			Use engine->Cmd_Argv,  engine->Cmd_Argv, and engine->Cmd_Argc to get 
//			pointers the character string command.
//-----------------------------------------------------------------------------
void SetDebugBits( CBasePlayer* pPlayer, char *name, int bit )
{
	if ( !pPlayer )
		return;

	CBaseEntity *pEntity = NULL;
	while ( (pEntity = GetNextCommandEntity( pPlayer, name, pEntity )) != NULL )
	{
		if (pEntity->m_debugOverlays & bit)
			pEntity->m_debugOverlays &= ~bit;
		else
			pEntity->m_debugOverlays |= bit;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pKillTargetName - 
//-----------------------------------------------------------------------------
void KillTargets( const char *pKillTargetName )
{
	CBaseEntity *pentKillTarget = NULL;

	DevMsg( 2, "KillTarget: %s\n", pKillTargetName );
	pentKillTarget = gEntList.FindEntityByName( NULL, pKillTargetName );
	while ( pentKillTarget )
	{
		UTIL_Remove( pentKillTarget );

		DevMsg( 2, "killing %s\n", STRING( pentKillTarget->m_iClassname ) );
		pentKillTarget = gEntList.FindEntityByName( pentKillTarget, pKillTargetName );
	}
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void ConsoleKillTarget( CBasePlayer *pPlayer, char *name)
{
	// If no name was given use the picker
	if (FStrEq(name,"")) 
	{
		CBaseEntity *pEntity = FindPickerEntity( pPlayer );
		if ( pEntity )
		{
			UTIL_Remove( pEntity );
			Msg( "killing %s\n", pEntity->GetDebugName() );
			return;
		}
	}
	// Otherwise use name or classname
	KillTargets( name );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CPointClientCommand : public CPointEntity
{
public:
	DECLARE_CLASS( CPointClientCommand, CPointEntity );
	DECLARE_DATADESC();

	void InputCommand( inputdata_t& inputdata );
};

void CPointClientCommand::InputCommand( inputdata_t& inputdata )
{
	if ( !inputdata.value.String()[0] )
		return;

	edict_t *pClient = NULL;
	if ( gpGlobals->maxClients == 1 )
	{
		pClient = engine->PEntityOfEntIndex( 1 );
	}
	else
	{
		// In multiplayer, send it back to the activator
		CBasePlayer *player = dynamic_cast< CBasePlayer * >( inputdata.pActivator );
		if ( player )
		{
			pClient = player->edict();
		}
	}

	if ( !pClient || !pClient->GetUnknown() )
		return;

	engine->ClientCommand( pClient, UTIL_VarArgs( "%s\n", inputdata.value.String() ) );
}

BEGIN_DATADESC( CPointClientCommand )
	DEFINE_INPUTFUNC( FIELD_STRING, "Command", InputCommand ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( point_clientcommand, CPointClientCommand );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CPointServerCommand : public CPointEntity
{
public:
	DECLARE_CLASS( CPointServerCommand, CPointEntity );
	DECLARE_DATADESC();
	void InputCommand( inputdata_t& inputdata );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : inputdata - 
//-----------------------------------------------------------------------------
void CPointServerCommand::InputCommand( inputdata_t& inputdata )
{
	if ( !inputdata.value.String()[0] )
		return;

	engine->ServerCommand( UTIL_VarArgs( "%s\n", inputdata.value.String() ) );
}

BEGIN_DATADESC( CPointServerCommand )
	DEFINE_INPUTFUNC( FIELD_STRING, "Command", InputCommand ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( point_servercommand, CPointServerCommand );

//------------------------------------------------------------------------------
// Purpose : Draw a line betwen two points.  White if no world collisions, red if collisions
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CC_DrawLine( void )
{
	Vector startPos;
	Vector endPos;

	startPos.x = atof(engine->Cmd_Argv(1));
	startPos.y = atof(engine->Cmd_Argv(2));
	startPos.z = atof(engine->Cmd_Argv(3));
	endPos.x = atof(engine->Cmd_Argv(4));
	endPos.y = atof(engine->Cmd_Argv(5));
	endPos.z = atof(engine->Cmd_Argv(6));

	UTIL_AddDebugLine(startPos,endPos,true,true);
}
static ConCommand drawline("drawline", CC_DrawLine, "Draws line between two 3D Points.\n\tGreen if no collision\n\tRed is collides with something\n\tArguments: x1 y1 z1 x2 y2 z2", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose : Draw a cross at a points.  
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CC_DrawCross( void )
{
	Vector vPosition;

	vPosition.x = atof(engine->Cmd_Argv(1));
	vPosition.y = atof(engine->Cmd_Argv(2));
	vPosition.z = atof(engine->Cmd_Argv(3));

	// Offset since min and max z in not about center
	Vector mins = Vector(-5,-5,-5);
	Vector maxs = Vector(5,5,5);

	Vector start = mins + vPosition;
	Vector end   = maxs + vPosition;
	UTIL_AddDebugLine(start,end,true,true);

	start.x += (maxs.x - mins.x);
	end.x	-= (maxs.x - mins.x);
	UTIL_AddDebugLine(start,end,true,true);

	start.y += (maxs.y - mins.y);
	end.y	-= (maxs.y - mins.y);
	UTIL_AddDebugLine(start,end,true,true);

	start.x -= (maxs.x - mins.x);
	end.x	+= (maxs.x - mins.x);
	UTIL_AddDebugLine(start,end,true,true);
}
static ConCommand drawcross("drawcross", CC_DrawCross, "Draws a cross at the given location\n\tArguments: x y z", FCVAR_CHEAT);


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CC_Player_Kill( void )
{
	DevMsg("Kill called\n");
	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	if (pPlayer)
	{
		//LAWYER:  We need a check for team here
		if (pPlayer->GetTeamNumber() != 2)
		{
			Msg("You can't suicide.  You're not even alive.\n");
			return;
		}
#ifdef _DEBUG
			if ( engine->Cmd_Argc() > 1	)
#else
			if ( engine->Cmd_Argc() > 1 && !g_pGameRules->IsMultiplayer() )
#endif
			{

				// Find the matching netname
				for ( int i = 1; i <= gpGlobals->maxClients; i++ )
				{
					CBasePlayer *pPlayer = ToBasePlayer( UTIL_PlayerByIndex(i) );
					if ( pPlayer )
					{
						if ( Q_strstr( pPlayer->GetPlayerName(), engine->Cmd_Argv(1)) )
						{
							ClientKill( pPlayer->edict() );
						}
					}
				}
			}
			else
			{
				ClientKill( pPlayer->edict() );
			}

	}
}
static ConCommand kill("kill", CC_Player_Kill, "kills the player");

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CC_Player_Buddha( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() ); 
	if (pPlayer)
	{
		if (pPlayer->m_debugOverlays & OVERLAY_BUDDHA_MODE)
		{
			pPlayer->m_debugOverlays &= ~OVERLAY_BUDDHA_MODE;
			Msg("Buddha Mode off...\n");
		}
		else
		{
			pPlayer->m_debugOverlays |= OVERLAY_BUDDHA_MODE;
			Msg("Buddha Mode on...\n");
		}
	}
}
static ConCommand buddha("buddha", CC_Player_Buddha, "Toggle.  Player takes damage but won't die. (Shows red cross when health is zero)", FCVAR_CHEAT);

#define TALK_INTERVAL 0.66 // min time between say commands from a client
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CC_Player_Say( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() ); 
	if (pPlayer )
	{
		if (( pPlayer->LastTimePlayerTalked() + TALK_INTERVAL ) < gpGlobals->curtime) 
		{
			Host_Say( pPlayer->edict(), 0 );
			pPlayer->NotePlayerTalked();
		}
	}
	else
	{
		Host_Say( NULL, 0 );
	}
}
static ConCommand say("say", CC_Player_Say, "Display player message");

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CC_Player_SayTeam( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() ); 
	if (pPlayer)
	{
		if (( pPlayer->LastTimePlayerTalked() + TALK_INTERVAL ) < gpGlobals->curtime) 
		{
			Host_Say( pPlayer->edict(), 1 );
			pPlayer->NotePlayerTalked();
		}
	}
}
static ConCommand say_team("say_team", CC_Player_SayTeam, "Display player message to team");

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CC_Player_Give( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() ); 
	if ( pPlayer 
		&& (gpGlobals->maxClients == 1 || sv_cheats->GetBool()) 
		&& engine->Cmd_Argc() >= 2 )
	{
		char item_to_give[ 256 ];
		Q_strncpy( item_to_give, engine->Cmd_Argv(1), sizeof( item_to_give ) );
		Q_strlower( item_to_give );

		// Dirty hack to avoid suit playing it's pickup sound
		if ( !stricmp( item_to_give, "item_suit" ) )
		{
			pPlayer->EquipSuit( false );
			return;
		}

		string_t iszItem = AllocPooledString( item_to_give );	// Make a copy of the classname
		pPlayer->GiveNamedItem( STRING(iszItem) );
	}
}
static ConCommand give("give", CC_Player_Give, "Give item to player.\n\tArguments: <item_name>");

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CC_Player_FOV( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );
	if ( pPlayer && sv_cheats->GetBool() )
	{
		if ( engine->Cmd_Argc() > 1)
		{
			int FOV = atoi( engine->Cmd_Argv(1) );

			pPlayer->SetDefaultFOV( FOV );
		}
		else
		{
			ClientPrint( pPlayer, HUD_PRINTCONSOLE, UTIL_VarArgs( "\"fov\" is \"%d\"\n", pPlayer->GetFOV() ) );
		}
	}
}
static ConCommand fov("fov", CC_Player_FOV, "Change players FOV");

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CC_Player_SetModel( void )
{
	if ( gpGlobals->deathmatch )
		return;

	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );
	if ( pPlayer && engine->Cmd_Argc() == 2)
	{
		if(!(pPlayer->IsZM()))
		{
			static char szName[256];
			Q_snprintf( szName, sizeof( szName ), "models/%s.mdl", engine->Cmd_Argv(1) );
			pPlayer->SetModel( szName );
			UTIL_SetSize(pPlayer, VEC_HULL_MIN, VEC_HULL_MAX);
		}

	}
}
static ConCommand setmodel("setmodel", CC_Player_SetModel, "Changes's player's model", FCVAR_CHEAT );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CC_Player_TestDispatchEffect( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );
	if ( !pPlayer)
		return;
	
	if ( engine->Cmd_Argc() < 2 )
	{
		Msg(" Usage: test_dispatcheffect <effect name> <distance away> <flags> <magnitude> <scale>\n " );
		Msg("		 defaults are: <distance 1024> <flags 0> <magnitude 0> <scale 0>\n" );
		return;
	}

	// Optional distance
	float flDistance = 1024;
	if ( engine->Cmd_Argc() >= 3 )
	{
		flDistance = atoi( engine->Cmd_Argv( 2 ) );
	}

	// Optional flags
	float flags = 0;
	if ( engine->Cmd_Argc() >= 4 )
	{
		flags = atoi( engine->Cmd_Argv( 3 ) );
	}

	// Optional magnitude
	float magnitude = 0;
	if ( engine->Cmd_Argc() >= 5 )
	{
		magnitude = atof( engine->Cmd_Argv( 4 ) );
	}

	// Optional scale
	float scale = 0;
	if ( engine->Cmd_Argc() >= 6 )
	{
		scale = atof( engine->Cmd_Argv( 5 ) );
	}

	Vector vecForward;
	QAngle vecAngles = pPlayer->EyeAngles();
	AngleVectors( vecAngles, &vecForward );

	// Trace forward
	trace_t tr;
	Vector vecSrc = pPlayer->EyePosition();
	Vector vecEnd = vecSrc + (vecForward * flDistance);
	UTIL_TraceLine( vecSrc, vecEnd, MASK_ALL, pPlayer, COLLISION_GROUP_NONE, &tr );

	// Fill out the generic data
	CEffectData data;
	// If we hit something, use that data
	if ( tr.fraction < 1.0 )
	{
		data.m_vOrigin = tr.endpos;
		VectorAngles( tr.plane.normal, data.m_vAngles );
		data.m_vNormal = tr.plane.normal;
	}
	else
	{
		data.m_vOrigin = vecEnd;
		data.m_vAngles = vecAngles;
		AngleVectors( vecAngles, &data.m_vNormal );
	}
	data.m_nEntIndex = pPlayer->entindex();
	data.m_fFlags = flags;
	data.m_flMagnitude = magnitude;
	data.m_flScale = scale;
	DispatchEffect( (char *)engine->Cmd_Argv(1), data );
}

static ConCommand test_dispatcheffect("test_dispatcheffect", CC_Player_TestDispatchEffect, "Test a clientside dispatch effect.\n\tUsage: test_dispatcheffect <effect name> <distance away> <flags> <magnitude> <scale>\n\tDefaults are: <distance 1024> <flags 0> <magnitude 0> <scale 0>\n", FCVAR_CHEAT);

//-----------------------------------------------------------------------------
// Purpose: Quickly switch to the physics cannon, or back to previous item
//-----------------------------------------------------------------------------
void CC_Player_PhysSwap( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );
	
	if ( pPlayer )
	{
		CBaseCombatWeapon *pWeapon = pPlayer->GetActiveWeapon();

		if ( pWeapon )
		{
			// Tell the client to stop selecting weapons
			engine->ClientCommand( UTIL_GetCommandClient()->edict(), "cancelselect" );

			const char *strWeaponName = pWeapon->GetName();

			if ( !Q_stricmp( strWeaponName, "weapon_physcannon" ) )
			{
				pPlayer->SelectLastItem();
			}
			else
			{
				pPlayer->SelectItem( "weapon_physcannon" );
			}
		}
	}
}
static ConCommand physswap("phys_swap", CC_Player_PhysSwap, "Automatically swaps the current weapon for the physcannon and back again." );

//-----------------------------------------------------------------------------
// Purpose: Quickly switch to the bug bait, or back to previous item
//-----------------------------------------------------------------------------
void CC_Player_BugBaitSwap( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );
	
	if ( pPlayer )
	{
		CBaseCombatWeapon *pWeapon = pPlayer->GetActiveWeapon();

		if ( pWeapon )
		{
			// Tell the client to stop selecting weapons
			engine->ClientCommand( UTIL_GetCommandClient()->edict(), "cancelselect" );

			const char *strWeaponName = pWeapon->GetName();

			if ( !Q_stricmp( strWeaponName, "weapon_bugbait" ) )
			{
				pPlayer->SelectLastItem();
			}
			else
			{
				pPlayer->SelectItem( "weapon_bugbait" );
			}
		}
	}
}
static ConCommand bugswap("bug_swap", CC_Player_BugBaitSwap, "Automatically swaps the current weapon for the bug bait and back again.", FCVAR_CHEAT );

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CC_Player_Use( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() ); 
	if ( pPlayer)
	{
		pPlayer->SelectItem((char *)engine->Cmd_Argv(1));
	}
}
static ConCommand use("use", CC_Player_Use, "Use a particular weapon\t\nArguments: <weapon_name>");


//------------------------------------------------------------------------------
// A small wrapper around SV_Move that never clips against the supplied entity.
//------------------------------------------------------------------------------
static bool TestEntityPosition ( CBasePlayer *pPlayer )
{	
	trace_t	trace;
	UTIL_TraceEntity( pPlayer, pPlayer->GetAbsOrigin(), pPlayer->GetAbsOrigin(), MASK_PLAYERSOLID, &trace );
	return (trace.startsolid == 0);
}


//------------------------------------------------------------------------------
// Searches along the direction ray in steps of "step" to see if 
// the entity position is passible.
// Used for putting the player in valid space when toggling off noclip mode.
//------------------------------------------------------------------------------
static int FindPassableSpace( CBasePlayer *pPlayer, const Vector& direction, float step, Vector& oldorigin )
{
	int i;
	for ( i = 0; i < 100; i++ )
	{
		Vector origin = pPlayer->GetAbsOrigin();
		VectorMA( origin, step, direction, origin );
		pPlayer->SetAbsOrigin( origin );
		if ( TestEntityPosition( pPlayer ) )
		{
			VectorCopy( pPlayer->GetAbsOrigin(), oldorigin );
			return 1;
		}
	}
	return 0;
}


//------------------------------------------------------------------------------
// Noclip
//------------------------------------------------------------------------------
void CC_Player_NoClip( void )
{
	if ( !sv_cheats->GetBool() )
		return;

	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() ); 
	if ( !pPlayer )
		return;

	CPlayerState *pl = pPlayer->PlayerData();
	Assert( pl );

	if (pPlayer->GetMoveType() != MOVETYPE_NOCLIP)
	{
		// Disengage from hierarchy
		pPlayer->SetParent( NULL );
		pPlayer->SetMoveType( MOVETYPE_NOCLIP );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "noclip ON\n");
		pPlayer->AddEFlags( EFL_NOCLIP_ACTIVE );
		return;
	}

	pPlayer->RemoveEFlags( EFL_NOCLIP_ACTIVE );
	pPlayer->SetMoveType( MOVETYPE_WALK );

	Vector oldorigin = pPlayer->GetAbsOrigin();
	ClientPrint( pPlayer, HUD_PRINTCONSOLE, "noclip OFF\n");
	if ( !TestEntityPosition( pPlayer ) )
	{
		Vector forward, right, up;

		AngleVectors ( pl->v_angle, &forward, &right, &up);
		
		// Try to move into the world
		if ( !FindPassableSpace( pPlayer, forward, 1, oldorigin ) )
		{
			if ( !FindPassableSpace( pPlayer, right, 1, oldorigin ) )
			{
				if ( !FindPassableSpace( pPlayer, right, -1, oldorigin ) )		// left
				{
					if ( !FindPassableSpace( pPlayer, up, 1, oldorigin ) )	// up
					{
						if ( !FindPassableSpace( pPlayer, up, -1, oldorigin ) )	// down
						{
							if ( !FindPassableSpace( pPlayer, forward, -1, oldorigin ) )	// back
							{
								Msg( "Can't find the world\n" );
							}
						}
					}
				}
			}
		}

		pPlayer->SetAbsOrigin( oldorigin );
	}
}

static ConCommand noclip("noclip", CC_Player_NoClip, "Toggle. Player becomes non-solid and flies.", FCVAR_CHEAT);


//------------------------------------------------------------------------------
// Sets client to godmode
//------------------------------------------------------------------------------
void CC_God_f (void)
{
	if ( !sv_cheats->GetBool() )
		return;

	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() ); 
	if ( !pPlayer )
		return;

	if ( gpGlobals->deathmatch )
		return;

	pPlayer->ToggleFlag( FL_GODMODE );
	if (!(pPlayer->GetFlags() & FL_GODMODE ) )
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "godmode OFF\n");
	else
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "godmode ON\n");
}

static ConCommand god("god", CC_God_f, "Toggle. Player becomes invulnerable.", FCVAR_CHEAT );


//------------------------------------------------------------------------------
// Sets client to godmode
//------------------------------------------------------------------------------
void CC_setpos_f (void)
{
	if ( !sv_cheats->GetBool() )
		return;

	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() ); 
	if ( !pPlayer )
		return;

	if ( engine->Cmd_Argc() < 3 )
	{
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "Usage:  setpos x y <z optional>\n");
		return;
	}

	Vector oldorigin = pPlayer->GetAbsOrigin();

	Vector newpos;
	newpos.x = atof( engine->Cmd_Argv(1) );
	newpos.y = atof( engine->Cmd_Argv(2) );
	newpos.z = engine->Cmd_Argc() == 4 ? atof( engine->Cmd_Argv(3) ) : oldorigin.z;

	pPlayer->SetAbsOrigin( newpos );

	if ( !TestEntityPosition( pPlayer ) )
	{
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "setpos into world, use noclip to unstick yourself!\n");
	}
}

static ConCommand setpos("setpos", CC_setpos_f, "Move player to specified origin (must have sv_cheats).", FCVAR_CHEAT );

//------------------------------------------------------------------------------
// Sets client to godmode
//------------------------------------------------------------------------------
void CC_setang_f (void)
{
	if ( !sv_cheats->GetBool() )
		return;

	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() ); 
	if ( !pPlayer )
		return;

	if ( engine->Cmd_Argc() < 3 )
	{
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "Usage:  setang pitch yaw <roll optional>\n");
		return;
	}

	QAngle oldang = pPlayer->GetAbsAngles();

	QAngle newang;
	newang.x = atof( engine->Cmd_Argv(1) );
	newang.y = atof( engine->Cmd_Argv(2) );
	newang.z = engine->Cmd_Argc() == 4 ? atof( engine->Cmd_Argv(3) ) : oldang.z;

	pPlayer->SnapEyeAngles( newang );
}

static ConCommand setang("setang", CC_setang_f, "Snap player eyes to specified pitch yaw <roll:optional> (must have sv_cheats).", FCVAR_CHEAT );

//------------------------------------------------------------------------------
// Sets client to notarget mode.
//------------------------------------------------------------------------------
void CC_Notarget_f (void)
{
	if ( !sv_cheats->GetBool() )
		return;

	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() ); 
	if ( !pPlayer )
		return;

	if ( gpGlobals->deathmatch )
		return;

	pPlayer->ToggleFlag( FL_NOTARGET );
	if ( !(pPlayer->GetFlags() & FL_NOTARGET ) )
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "notarget OFF\n");
	else
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "notarget ON\n");
}

ConCommand notarget("notarget", CC_Notarget_f, "Toggle. Player becomes hidden to NPCs.", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Damage the client the specified amount
//------------------------------------------------------------------------------
void CC_HurtMe_f(void)
{
	if ( !sv_cheats->GetBool() )
		return;

	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() ); 
	if ( !pPlayer )
		return;

	int iDamage = 10;
	if ( engine->Cmd_Argc() >= 2 )
	{
		iDamage = atoi( engine->Cmd_Argv( 1 ) );
	}

	pPlayer->TakeDamage( CTakeDamageInfo( pPlayer, pPlayer, iDamage, DMG_GENERIC ) );
}

static ConCommand hurtme("hurtme", CC_HurtMe_f, "Hurts the player.\n\tArguments: <health to lose>", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// LAWYER:  Restart the round!
//------------------------------------------------------------------------------
void CC_RoundRestart_f(void) //LAWYER:  This should be removed upon release
{
//	if(!(engine->IsDedicatedServer())) //dedicated servers don't like the following block of code...at all -qck
//	{ //LAWYER:  Problems associated with this seem to be associated with needing a baselplayer.

	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() ); 
		if (pPlayer) //If a player calls this...
		{
			bool bHaveZM = false;
			bool bHaveHumans = false;
		/*
			//Find if we have a Zombie Master, and count players;
			for (int i = 1; i <= gpGlobals->maxClients; i++ )
			{
				CBasePlayer *pIterated = UTIL_PlayerByIndex( i );

				if ( pIterated )
				{

					if (pIterated->GetTeamNumber() == 3)
					{
						bHaveZM = true;
					}
				}
			}
		
			//Find if we have a human team, and count players;
			for (int i = 1; i <= gpGlobals->maxClients; i++ )
			{
				CBasePlayer *pIterated = UTIL_PlayerByIndex( i );

				if ( pIterated )
				{

					if (pIterated->GetTeamNumber() == 2)
					{
						bHaveHumans = true;
					}
				}
			}
			*/
			CTeam *team = GetGlobalTeam(TEAM_ZOMBIEMASTER);
			CTeam *team2 = GetGlobalTeam(TEAM_HUMANS);
			if ( team )
			{
				if (team->GetNumPlayers() > 0)
				{
					bHaveZM = true; //Restart the round if we find ourselves without a Zombiemaster!
				}
			}
			
			if ( team2 )
			{
				if (team2->GetNumPlayers() > 0)
				{
					bHaveHumans = true; //Restart the round if we find ourselves without a human team!
				}
			}

			//only allow if ZM or cheats are on
			if ( !sv_cheats->GetBool() && !(pPlayer->IsZM()) && bHaveZM == true && bHaveHumans == true)
			{
				return;
			}
			else
			{
		
				/*char text[100];
				Q_snprintf( text,sizeof(text), "The Zombie Master has submitted.\n" );*/

				UTIL_ClientPrintAll( HUD_PRINTTALK, "The Zombie Master has submitted.\n" );	

				HL2MPRules()->TeamVictorious( true, "zm_submitted" );

				g_pGameRules->FinishingRound();
			}
		}
		else
		{
			g_pGameRules->FinishingRound();
		}
//	}


}

static ConCommand roundrestart("roundrestart", CC_RoundRestart_f, "Restarts the round.");

//--------------------------------------------------------------
// TGB: idling zm means we need to roundrestart, so this is called
//--------------------------------------------------------------
void CC_IdleRestart_f(void)
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() ); 
	if (pPlayer && pPlayer->IsZM() == false)
		return;
	//sadly we can't check if it's the player being a punk and manually calling this, really ought to use usermessages etc

	UTIL_ClientPrintAll( HUD_PRINTCENTER, "The current Zombie Master appears to be idle. The round will be restarted." );	
	g_pGameRules->FinishingRound();
}
static ConCommand zm_idlecheck_roundrestart("zm_idlecheck_roundrestart", CC_IdleRestart_f, "Notifies players of an idling ZM and restarts the round.");
//TGB: declare the server-side versions of these convars so server admins can modify them and they'll be replicated to the clients
static ConVar zm_idlecheck_warningtime( "zm_idlecheck_warningtime", "60", FCVAR_REPLICATED, "Time spent idling (in seconds) before the player gets a warning on screen." );
static ConVar zm_idlecheck_restarttime( "zm_idlecheck_restarttime", "120", FCVAR_REPLICATED, "Time spent idling (in seconds) before the round is restarted and a new ZM is picked. Also see zm_idlecheck_warningtime." );

//------------------------------------------------------------------------------
// LAWYER:  Force yourself onto the ZombieMaster team!
//------------------------------------------------------------------------------
void CC_ForceMaster_f(void)
{
	if ( !sv_cheats->GetBool() )
		return; //LAWYER:  Decheated, for testing purposes
	
	CHL2MP_Player *pPlayer = (CHL2MP_Player *)ToBasePlayer( UTIL_GetCommandClient() ); //LAWYER:  This needs sorting
	if (!pPlayer) return;
	pPlayer->ChangeTeam(3);
	pPlayer->Spawn();
}

static ConCommand forcemaster("forcemaster", CC_ForceMaster_f, "Forces yourself to be the Zombie Master.", FCVAR_CHEAT); //LAWYER:  Turn this into a cheat


//------------------------------------------------------------------------------
// LAWYER:  Force yourself onto the ZombieMaster team!
//------------------------------------------------------------------------------
void CC_GivePriority_f(void)
{
	if ( !sv_cheats->GetBool() )
		return;
	
	CHL2MP_Player *pPlayer = (CHL2MP_Player *)ToBasePlayer( UTIL_GetCommandClient() ); //LAWYER:  This needs sorting
#ifndef CLIENT_DLL
	if (pPlayer)
	{
		pPlayer->m_iZMPriority += atoi( engine->Cmd_Argv(1) );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "Priority level altered\n");
		DevMsg("New priority level: %i\n",pPlayer->m_iZMPriority);
	}

#endif
}

static ConCommand zm_givepriority("zm_givepriority", CC_GivePriority_f, "Gives you increased ZM priority.", FCVAR_CHEAT);


//------------------------------------------------------------------------------
// TGB:  Alter participation level
//------------------------------------------------------------------------------
void CC_Participate_f(void)
{
	CHL2MP_Player *pPlayer = (CHL2MP_Player *)ToBasePlayer( UTIL_GetCommandClient() ); //LAWYER:  This needs sorting
#ifndef CLIENT_DLL
	if (pPlayer)
	{
		int level = atoi( engine->Cmd_Argv(1) );
		if (level < 0 || level > 3)
			level = 0;

		pPlayer->m_zmParticipation = level;
		//pPlayer->m_iZMPriority = pPlayer->m_zmParticipation;
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "Participation level altered\n");
	}

#endif
}
static ConCommand zm_participate("zm_participate", CC_Participate_f, "Changes your participation level. 0 = full, 1 = no ZMing, 2 = always observe.");

//------------------------------------------------------------------------------
// LAWYER:  Request the round is restarted
//------------------------------------------------------------------------------
void CC_VoteRoundRestart_f(void)
{
	CHL2MP_Player *pPlayer = (CHL2MP_Player *)ToBasePlayer( UTIL_GetCommandClient() ); //LAWYER:  This needs sorting
#ifndef CLIENT_DLL
	if (pPlayer && pPlayer->m_iZMVoteRR == 0) //not sure why voteRR wasn't checked before
	{
		pPlayer->m_iZMVoteRR = 1;
		//pPlayer->m_iZMPriority = pPlayer->m_zmParticipation;
		char text[256];
		Q_snprintf( text,sizeof(text), "%s has voted for a new round to begin.\n", pPlayer->GetPlayerName()  );
		UTIL_ClientPrintAll( HUD_PRINTTALK, text );	
				
		//LAWYER:  Ask the server to check for round restarters.
		g_pGameRules->CheckRoundRestart();
	}

#endif
}
static ConCommand zm_voteroundrestart("zm_voteroundrestart", CC_VoteRoundRestart_f, "Votes for a Roundrestart.  1 = true, 0 = false");

/* TGB: these are broken and serve no purpose, therefore removing them


//------------------------------------------------------------------------------
// LAWYER:  I don't want to play!
//------------------------------------------------------------------------------
void CC_Observe_f(void)
{
	//TGB: broken these days
	if ( !sv_cheats->GetBool() )
		return;

	CHL2MP_Player *pPlayer = (CHL2MP_Player *)ToBasePlayer( UTIL_GetCommandClient() ); //LAWYER:  This needs sorting
#ifndef CLIENT_DLL
	if (pPlayer->m_bObserve == true)
	{
		pPlayer->m_bObserve = false;
		pPlayer->m_zmParticipation = 0;
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "You will spawn in the game on the next round\n");
	}
	else
	{
		pPlayer->m_bObserve = true;
		pPlayer->m_zmParticipation = 2; //TGB: fixed values here
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "You will remain an observer\n");
	}

#endif
}

static ConCommand observe("observe", CC_Observe_f, "Makes you observer on every round."); //LAWYER:  Turn this into a cheat

//------------------------------------------------------------------------------
// LAWYER:  I don't want to be the ZM!
//------------------------------------------------------------------------------
void CC_DontWannaZM_f(void)
{
	CHL2MP_Player *pPlayer = (CHL2MP_Player *)ToBasePlayer( UTIL_GetCommandClient() ); //LAWYER:  This needs sorting
#ifndef CLIENT_DLL
	if (pPlayer->m_bDontWannaZM == true)
	{
		pPlayer->m_bDontWannaZM = false;
		pPlayer->m_zmParticipation = 0;
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "You can be picked for the Zombie Master\n");
	}
	else
	{
		pPlayer->m_bDontWannaZM = true;
		pPlayer->m_zmParticipation = 1;
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "You will avoid becoming the Zombie Master\n");
	}

#endif
}

static ConCommand DontWannaZM("dontwannazm", CC_DontWannaZM_f, "Avoid becoming the Zombie Master."); //LAWYER:  Turn this into a cheat
*/
//LAWYER: Display map rules!
static void ShowMapText_f( void )
{
	CHL2MP_Player *pPlayer = (CHL2MP_Player *)ToBasePlayer( UTIL_GetCommandClient() ); //LAWYER:  This needs sorting
	KeyValues *data = new KeyValues("data");
	data->SetString( "title", "Objectives" );		// info panel title
	data->SetString( "type", "1" );			// show userdata from stringtable entry
	data->SetString( "msg",	"mapinfo" );		// use this stringtable entry

	pPlayer->ShowViewPortPanel( PANEL_INFO, true, data );

	data->deleteThis();

}
static ConCommand showmaptext( "showmaptext", ShowMapText_f, "Displays the map's objectives");

//------------------------------------------------------------------------------
// LAWYER:  Force yourself onto the Human team!
//------------------------------------------------------------------------------
void CC_ForceHuman_f(void)
{
	if ( !sv_cheats->GetBool() )
		return;
	
	CHL2MP_Player *pPlayer = (CHL2MP_Player *)ToBasePlayer( UTIL_GetCommandClient() ); //LAWYER:  This needs sorting
	if (!pPlayer) return;
	pPlayer->ChangeTeam(2);
	pPlayer->Spawn();

	pPlayer->m_iWeaponFlags = 0; //TGB: to be completely certain
}

static ConCommand forcehuman("forcehuman", CC_ForceHuman_f, "Forces yourself to the Human team.", FCVAR_CHEAT); //LAWYER:  Turn this into a cheat

//TGB: useful for testing spec stuff
void CC_ForceSpectator_f(void)
{
	if ( !sv_cheats->GetBool() )
		return;

	CHL2MP_Player *pPlayer = (CHL2MP_Player *)ToBasePlayer( UTIL_GetCommandClient() );
	if (!pPlayer) return;
	pPlayer->ChangeTeam(1);
	pPlayer->Spawn();

	pPlayer->m_iWeaponFlags = 0; //TGB: to be completely certain
}

static ConCommand forcespectator("forcespectator", CC_ForceSpectator_f, "Forces yourself to the spectator team.", FCVAR_CHEAT);


static bool IsInGroundList( CBaseEntity *ent, CBaseEntity *ground )
{
	if ( !ground || !ent )
		return false;

	groundlink_t *root = ( groundlink_t * )ground->GetDataObject( GROUNDLINK );
	if ( root )
	{
		groundlink_t *link = root->nextLink;
		while ( link != root )
		{
			CBaseEntity *other = link->entity;
			if ( other == ent )
				return true;
			link = link->nextLink;
		}
	}

	return false;

}

//------------------------------------------------------------------------------
// LAWYER:  Idenfity a target!
//------------------------------------------------------------------------------
void CC_Identify_f(void)
{
				hudtextparms_s tTextParam;
			tTextParam.x			= 0.7;
			tTextParam.y			= 0.65;
			tTextParam.effect		= 0;
			tTextParam.r1			= 255;
			tTextParam.g1			= 255;
			tTextParam.b1			= 255;
			tTextParam.a1			= 255;
			tTextParam.r2			= 255;
			tTextParam.g2			= 255;
			tTextParam.b2			= 255;
			tTextParam.a2			= 255;
			tTextParam.fadeinTime	= 1;
			tTextParam.fadeoutTime	= 1;
			tTextParam.holdTime		= 2;
			tTextParam.fxTime		= 0;
			tTextParam.channel		= 1;

	CHL2MP_Player *pPlayer = (CHL2MP_Player *)ToBasePlayer( UTIL_GetCommandClient() ); //LAWYER:  This needs sorting
		trace_t tr;
		
		Vector forward;
		pPlayer->EyeVectors( &forward );
		UTIL_TraceLine(pPlayer->EyePosition(),
			pPlayer->EyePosition() + forward * 256,MASK_SOLID, 
			pPlayer, COLLISION_GROUP_NONE, &tr );

	if ( tr.DidHitNonWorldEntity() )
	{
		CBaseEntity *pEntity = tr.m_pEnt;
		
		if ( pEntity && (pEntity != pPlayer) )
		{
			//LAWYER:  We found an entity.  Check for playerdom.
			if (pEntity->IsPlayer())
			{
				CHL2MP_Player *pTargeted = dynamic_cast< CHL2MP_Player* >( pEntity );
				if (pTargeted)
				{

			
					UTIL_HudMessage( pPlayer, tTextParam, ("That looks like %s\n", pTargeted->GetPlayerName()) );
				}
			}
			else if (pEntity->IsNPC())
			{
				UTIL_HudMessage( pPlayer, tTextParam, "That looks undead\n" );
			}
			else
			{
				UTIL_HudMessage( pPlayer, tTextParam, "That looks inanimate\n" );
			}
		}

	}
	else
	{
				
		
			UTIL_HudMessage( pPlayer, tTextParam, "There's nothing there.\n" );
		
	}
}

static ConCommand identify("identify", CC_Identify_f, "Identify a player.");


//qck:Some testing stuff for temp ent reset
void CC_Round_Restart_Toggle()
{
	CWorld* world = GetWorldEntity();
	world->ToggleRoundRestart();
}

static ConCommand cl_roundrestart_toggle("cl_roundrestart_toggle", CC_Round_Restart_Toggle, "Toggle the roundrestart flag");

static int DescribeGroundList( CBaseEntity *ent )
{
	if ( !ent )
		return 0;

	int c = 1;

	Msg( "%i : %s (ground %i %s)\n", ent->entindex(), ent->GetClassname(), 
		ent->GetGroundEntity() ? ent->GetGroundEntity()->entindex() : -1,
		ent->GetGroundEntity() ? ent->GetGroundEntity()->GetClassname() : "NULL" );
	groundlink_t *root = ( groundlink_t * )ent->GetDataObject( GROUNDLINK );
	if ( root )
	{
		groundlink_t *link = root->nextLink;
		while ( link != root )
		{
			CBaseEntity *other = link->entity;
			if ( other )
			{
				Msg( "  %02i:  %i %s\n", c++, other->entindex(), other->GetClassname() );

				if ( other->GetGroundEntity() != ent )
				{
					Assert( 0 );
					Msg( "   mismatched!!!\n" );
				}
			}
			else
			{
				Assert( 0 );
				Msg( "  %02i:  NULL link\n", c++ );
			}
			link = link->nextLink;
		}
	}

	if ( ent->GetGroundEntity() != NULL )
	{
		Assert( IsInGroundList( ent, ent->GetGroundEntity() ) );
	}

	return c - 1;
}

void CC_GroundList_f(void)
{
	if ( engine->Cmd_Argc() == 2 )
	{
		int idx = atoi( engine->Cmd_Argv(1) );

		CBaseEntity *ground = CBaseEntity::Instance( idx );
		if ( ground )
		{
			DescribeGroundList( ground );
		}
	}
	else
	{
		CBaseEntity *ent = NULL;
		int linkCount = 0;
		while ( (ent = gEntList.NextEnt(ent)) != NULL )
		{
			linkCount += DescribeGroundList( ent );
		}

		extern int groundlinksallocated;
		Assert( linkCount == groundlinksallocated );

		Msg( "--- %i links\n", groundlinksallocated );
	}
}

static ConCommand groundlist("groundlist", CC_GroundList_f, "Display ground entity list <index>" );

//-----------------------------------------------------------------------------
// Purpose: Toggle ZM camera type
//-----------------------------------------------------------------------------

void CC_FullObserver_f(void) //LAWYER: Toggle camera type
{
	CHL2MP_Player *pPlayer = (CHL2MP_Player *)ToBasePlayer( UTIL_GetCommandClient() ); //LAWYER:  This needs sorting
	if (pPlayer)
	{
		if (pPlayer->IsZM())
		{
			if (pPlayer->m_bCameraMode == true)
			{
				pPlayer->ShowViewPortPanel( PANEL_VIEWPORT, true ); //LAWYER:  show the Zombie panel!
				pPlayer->m_bCameraMode = false;
			}
			else if (pPlayer->m_bCameraMode == false)
			{
				pPlayer->ShowViewPortPanel( PANEL_VIEWPORT, false ); //LAWYER:  hide# the Zombie panel!
				pPlayer->m_bCameraMode = true;
			}

		}
	}
}

static ConCommand fullobserver("fullobserver", CC_FullObserver_f, "Toggles the ZombieMaster camera. Use zm_showviewport if you want an explicit toggle." );

//-----------------------------------------------------------------------------
// Purpose: Controllable toggle for explicitly turning fullobserver on/off
//-----------------------------------------------------------------------------
void CC_ShowViewport_f(void) 
{
	if ( engine->Cmd_Argc() != 2 )
		return;
	
	bool show = (bool)atoi( engine->Cmd_Argv(1) );

	CHL2MP_Player *pPlayer = (CHL2MP_Player *)ToBasePlayer( UTIL_GetCommandClient() ); //LAWYER:  This needs sorting
	if (pPlayer && pPlayer->IsZM())
	{
		pPlayer->ShowViewPortPanel( PANEL_VIEWPORT, show );
		pPlayer->m_bCameraMode = !show;
	}
}
static ConCommand zm_showviewport("zm_showviewport", CC_ShowViewport_f, "Turns viewport on or off, usage: zm_showviewport <1 or 0>");

//-----------------------------------------------------------------------------
// Purpose: Throw a unit of ammo -TGB
//-----------------------------------------------------------------------------
void CC_GiveAmmo_f(void) //why the _f? meh
{
	//get ammo type
	//check for sufficient ammo to throw
	//throw clip
	//deduct ammo

	//TGB: this was a basecombatcharacter cast which makes no sense, not sure why I did it at the time
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );
	if (!pPlayer) return;

	CBaseCombatWeapon *pWeapon = pPlayer->GetActiveWeapon();
	
	if (!pWeapon) return;

	//we need sufficient ammo to throw a full ammo box, because ammo boxes always give that amount
	//if we want smaller numbers on this we'll need to redo the ammo boxes for a part

	int clip = -1; 
	const char *ammo_type = pWeapon->GetWpnData().szAmmo1;
	
	const char *ammo_name[] = {"item_ammo_pistol", "item_box_buckshot", "item_ammo_357", "item_ammo_smg1", "item_ammo_revolver"};
	enum ammo_guntype { PISTOL, SHOTGUN, RIFLE, SMG, REVOLVER };


	int ammo_index = -1;
	if (Q_stricmp(ammo_type, "Pistol") == 0) { ammo_index = PISTOL; clip = SIZE_AMMO_PISTOL; }
	if (Q_stricmp(ammo_type, "Buckshot") == 0) { ammo_index = SHOTGUN; clip = SIZE_AMMO_BUCKSHOT; }
	if (Q_stricmp(ammo_type, "357") == 0) { ammo_index = RIFLE; clip = SIZE_AMMO_357; }
	if (Q_stricmp(ammo_type, "SMG1") == 0) { ammo_index = SMG; clip = SIZE_AMMO_SMG1; }
	if (Q_stricmp(ammo_type, "Revolver") == 0) { ammo_index = REVOLVER; clip = SIZE_AMMO_REVOLVER; }

	if (ammo_index == -1)
	{
		DevWarning("Ammo type to spawn not found.\n");
		return;
	}
	//if the size of one ammobox is larger than the amount of ammo we have...
	if (clip > pPlayer->GetAmmoCount(pWeapon->GetPrimaryAmmoType()))
	{
		//... we don't have enough ammo	
		//Msg("\nInsufficient ammo!"); //TGB: This needs a game_text or similar message
		ClientPrint( pPlayer, HUD_PRINTCENTER, "Insufficient ammo!\n");
		return;
	}

	//get a nice position to spawn the ammo box
	Vector	vecEye = pPlayer->EyePosition();
	Vector	vForward, vRight;

	pPlayer->EyeVectors( &vForward, &vRight, NULL );
	Vector vecSrc = vecEye + vForward * 60.0f + vRight * -8.0f;

	//Vector vecSrc = pPlayer->Weapon_ShootPosition() - Vector(0,0,10);

//	Msg("\nB: X: %f || Y: %f || Z: %f\n", vecSrc.x, vecSrc.y, vecSrc.z);
//	Msg("\nP: X: %f || Y: %f || Z: %f\n", pPlayer->Weapon_ShootPosition().x, pPlayer->Weapon_ShootPosition().y, pPlayer->Weapon_ShootPosition().z);
	//create bawx of chawc'late
	CItem *ammobox = (CItem *)CBaseEntity::Create( ammo_name[ammo_index], vecSrc, pWeapon->GetAbsAngles(), NULL );
	
	if (!ammobox) return;
	
	IPhysicsObject *pObj = ammobox->VPhysicsGetObject();
	
	Vector vecVelocity = pPlayer->BodyDirection3D() * 200.0f;
	
	if ( pObj != NULL )
	{
		AngularImpulse angImp( 200, 200, 200 );
		pObj->AddVelocity( &vecVelocity, &angImp );
	}
	else
	{
		ammobox->SetAbsVelocity( vecVelocity );
	}

	//TGB: never respawn this cookie
	ammobox->AddSpawnFlags( SF_NORESPAWN );

	//... but let's make sure there's a sane place for it to spawn anyway
	ammobox->SetOriginalSpawnOrigin(ammobox->GetAbsOrigin());
		
	pPlayer->RemoveAmmo(clip, pWeapon->GetPrimaryAmmoType());
}

static ConCommand giveammo("giveammo", CC_GiveAmmo_f, "Throws a pack of current weapon's ammo forward for other players to pick up" );

//-----------------------------------------------------------------------------
// Purpose: Throw weapon -TGB
//-----------------------------------------------------------------------------
void CC_GiveWeapon_f(void)
{
	//check for weapon availability
	//create weapon, throw
	//remove from inventory

	//CBaseCombatCharacter *pPlayer = (CBaseCombatCharacter *)ToBasePlayer( UTIL_GetCommandClient() );
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );
	if (!pPlayer)
		return;

	CBaseCombatWeapon *pWeapon = pPlayer->GetActiveWeapon();
	
	//we do have one right?
	if (!pWeapon) return;

	/*
	const char *weapon_name = pWeapon->GetName();

	//oh, and we can't throw our hands
	if (strcmp(weapon_name, "weapon_zm_fists") == 0) //strcmp == 0 == equal
		return;

	//or our carryweaponthing
	if (strcmp(weapon_name, "weapon_zm_carry") == 0) //strcmp == 0 == equal
		return;
	*/
	if (pWeapon->CanBeDropped() == false)
		return;
	
	
	//oh luvverly, we already have a prebuilt function for this
	pPlayer->Weapon_Drop(pWeapon, NULL, NULL);
	//look at it go!
	//LAWYER:  Now we need to make sure it's removed from our flagpile!
	pPlayer->m_iWeaponFlags -= pWeapon->GetWeaponTypeNumber();

}

static ConCommand giveweapon("giveweapon", CC_GiveWeapon_f, "Throws current weapon forward for other players to pick up" );


//--------------------------------------------------------------
//TGB: test func
//--------------------------------------------------------------
void CC_TestEntList_f( void )
{
	if (sv_cheats->GetBool() == false) return;
	DevMsg("Testing ent list...\n");
	gEntList.TestEntities();
	DevMsg("Test complete.\n");
}
static ConCommand testentlist("zm_testentlist", CC_TestEntList_f, "debug cmd for testing ent list contents" );

//-----------------------------------------------------------------------------
// Purpose: called each time a player uses a "cmd" command
// Input  : *pEdict - the player who issued the command
//			Use engine->Cmd_Argv,  engine->Cmd_Argv, and engine->Cmd_Argc to get 
//			pointers the character string command.
//-----------------------------------------------------------------------------
void ClientCommand( CBasePlayer *pPlayer )
{
	const char *pcmd = engine->Cmd_Argv(0);

	// Is the client spawned yet?
	if ( !pPlayer )
		return;

	/*
	const char *pstr;

	if (((pstr = strstr(pcmd, "weapon_")) != NULL)  && (pstr == pcmd))
	{
		// Subtype may be specified
		if ( engine->Cmd_Argc() == 2 )
		{
			pPlayer->SelectItem( pcmd, atoi( engine->Cmd_Argv( 1 ) ) );
		}
		else
		{
			pPlayer->SelectItem(pcmd);
		}
	}
	*/
	
	if ( FStrEq( pcmd, "killtarget" ) )
	{
		if ( g_pDeveloper->GetBool() && sv_cheats->GetBool() && UTIL_IsCommandIssuedByServerAdmin() )
		{
			ConsoleKillTarget(pPlayer, engine->Cmd_Argv(1));
		}
	}
	else if ( FStrEq( pcmd, "demorestart" ) ) 
	{
		pPlayer->ForceClientDllUpdate(); 
	}
	else if ( FStrEq( pcmd, "fade" ) )
	{
		color32 black = {32,63,100,200};
		UTIL_ScreenFade( pPlayer, black, 3, 3, FFADE_OUT  );
	} 
	else if ( FStrEq( pcmd, "te" ) )
	{
		if ( sv_cheats->GetBool() && UTIL_IsCommandIssuedByServerAdmin() )
		{
			if ( FStrEq( engine->Cmd_Argv(1), "stop" ) )
			{
				// Destroy it
				//
				CBaseEntity *ent = gEntList.FindEntityByClassname( NULL, "te_tester" );
				while ( ent )
				{
					CBaseEntity *next = gEntList.FindEntityByClassname( ent, "te_tester" );
					UTIL_Remove( ent );
					ent = next;
				}
			}
			else
			{
				CTempEntTester::Create( pPlayer->WorldSpaceCenter(), pPlayer->EyeAngles(), engine->Cmd_Argv(1), engine->Cmd_Argv(2) );
			}
		}
	}
	else 
	{
		if (!g_pGameRules->ClientCommand( pcmd, pPlayer ))
		{
			if ( Q_strlen(pcmd) > 128 )
			{
				ClientPrint( pPlayer, HUD_PRINTCONSOLE, "Console command too long.\n" );
			}
			else
			{
				// tell the user they entered an unknown command
				ClientPrint( pPlayer, HUD_PRINTCONSOLE, UTIL_VarArgs( "Unknown command: %s\n", pcmd ) );
			}
		}
	}
}
