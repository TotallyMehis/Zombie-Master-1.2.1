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

//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Console commands for debugging and manipulating NPCs.
//          Also, many many core commands used in making ZM work. It's a mess!
//
//===========================================================================//

#include "cbase.h"
#include "ai_basenpc.h"
#include "player.h"
#include "entitylist.h"
#include "ai_network.h"
#include "ai_node.h"
#include "ai_link.h"
#include "ai_networkmanager.h"
#include "ndebugoverlay.h"
#include "datacache/imdlcache.h"
#include "viewport_panel_names.h"

//qck addon
#include "usermessages.h"
#include "utlmultilist.h"
#include "zombielist.h"

//TGB: we'd like our convars here
#include "game.h"

//TGB: added for special zombie commanding
#include "npc_BaseZombie.h"
//#include "zombiemaster/zombiemaster_specific.h" TGB: we're already getting this through basezombie
//just including basezombie feels kind of messy/clunky, but we need to cast to CNPC_BaseZombie
//the direct commands could be moved to a function in basezombie, but for that we'd still need to cast
//if there's a better way please fix/tell me

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//TGB: someday we should really reorganize all our crap here into separate files



extern CAI_Node*	FindPickerAINode( CBasePlayer* pPlayer, NodeType_e nNodeType );
extern void			SetDebugBits( CBasePlayer* pPlayer, char *name, int bit );

bool g_bAIDisabledByUser = false;

//TGB: costs now convars
ConVar zm_cost_shambler( "zm_cost_shambler", "10", FCVAR_NOTIFY | FCVAR_REPLICATED);
ConVar zm_cost_banshee( "zm_cost_banshee", "60", FCVAR_NOTIFY | FCVAR_REPLICATED);
ConVar zm_cost_hulk( "zm_cost_hulk", "60", FCVAR_NOTIFY | FCVAR_REPLICATED);
ConVar zm_cost_drifter( "zm_cost_drifter", "25", FCVAR_NOTIFY | FCVAR_REPLICATED);
ConVar zm_cost_immolator( "zm_cost_immolator", "100", FCVAR_NOTIFY | FCVAR_REPLICATED);

#define ZM_SELECTAREA_NPC	75
#define ZM_SELECTAREA_OTHER 65

//fairly useless to make this a cvar I'd say
#define ZM_MAX_TRAPS_PER_MANIP 3


CZombieGroupManager* manager; 

//------------------------------------------------------------------------------
// Purpose: Disables all NPCs
//------------------------------------------------------------------------------
void CC_AI_Disable( void )
{
	if (CAI_BaseNPC::m_nDebugBits & bits_debugDisableAI)
	{
		CAI_BaseNPC::m_nDebugBits &= ~bits_debugDisableAI;
		DevMsg("AI Enabled.\n");
	}
	else
	{
		CAI_BaseNPC::m_nDebugBits |= bits_debugDisableAI;
		DevMsg("AI Disabled.\n");
		g_bAIDisabledByUser = true;
	}

	CBaseEntity::m_nDebugPlayer = UTIL_GetCommandClientIndex();
}
static ConCommand ai_disable("ai_disable", CC_AI_Disable, "Bi-passes all AI logic routines and puts all NPCs into their idle animations.  Can be used to get NPCs out of your way and to test effect of AI logic routines on frame rate", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose: Show hint nodes
//------------------------------------------------------------------------------
void CC_AI_ShowHints( void )
{
	// Eventually this will be done by name when mulitple
	// networks are used, but for now have one big AINet
	//static char entName[256];	
	//Q_strncpy( entName, engine->Cmd_Argv(1),sizeof(entName) );
	if ( !g_pAINetworkManager )
		return;

	g_pAINetworkManager->GetEditOps()->SetDebugBits("BigNet",bits_debugOverlayHints);
	CBaseEntity::m_nDebugPlayer = UTIL_GetCommandClientIndex();
}
static ConCommand ai_show_hints("ai_show_hints", CC_AI_ShowHints, "Displays all hints as small boxes\n\tBlue		- hint is available for use\n\tRed		- hint is currently being used by an NPC\n\tOrange		- hint not being used by timed out\n\tGrey		- hint has been disabled", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose: Show node connections with hulls
//------------------------------------------------------------------------------
void CC_AI_ShowHull( void )
{
	// Eventually this will be done by name when mulitple
	// networks are used, but for now have one big AINet
	//static char entName[256];	
	//Q_strncpy( entName, engine->Cmd_Argv(1),sizeof(entName) );

	g_pAINetworkManager->GetEditOps()->SetDebugBits("BigNet",bits_debugOverlayHulls);
	CBaseEntity::m_nDebugPlayer = UTIL_GetCommandClientIndex();
}
static ConCommand ai_show_hull("ai_show_hull", CC_AI_ShowHull, "Displays the allowed hulls between each node for the currently selected hull type.  Hulls are color code as follows:\n\tGreen		- ground movement \n\tBlue		- jumping movement\n\tCyan		- flying movement\n\tMagenta	- climbing movement\n\tArguments: 	-none-", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose: Show node connections with lines
//------------------------------------------------------------------------------
void CC_AI_ShowConnect( void )
{
	// Eventually this will be done by name when mulitple
	// networks are used, but for now have one big AINet
	//static char entName[256];	
	//Q_strncpy( entName, engine->Cmd_Argv(1),sizeof(entName) );
	if ( !g_pAINetworkManager )
		return;

	g_pAINetworkManager->GetEditOps()->SetDebugBits("BigNet",bits_debugOverlayConnections);
	CBaseEntity::m_nDebugPlayer = UTIL_GetCommandClientIndex();

}
static ConCommand ai_show_connect("ai_show_connect", CC_AI_ShowConnect, "Displays the allowed connections between each node for the currently selected hull type.  Hulls are color code as follows:\n\tGreen		- ground movement \n\tBlue		- jumping movement\n\tCyan		- flying movement\n\tMagenta	- climbing movement\n\tRed		- connection disabled", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose: Show node connections with lines
//------------------------------------------------------------------------------
void CC_AI_ShowJumpConnect( void )
{
	// Eventually this will be done by name when mulitple
	// networks are used, but for now have one big AINet
	//static char entName[256];	
	//Q_strncpy( entName, engine->Cmd_Argv(1),sizeof(entName) );
	if ( !g_pAINetworkManager )
		return;

	g_pAINetworkManager->GetEditOps()->SetDebugBits("BigNet",bits_debugOverlayJumpConnections);
	CBaseEntity::m_nDebugPlayer = UTIL_GetCommandClientIndex();

}
static ConCommand ai_show_connect_jump("ai_show_connect_jump", CC_AI_ShowJumpConnect, "Displays the allowed connections between each node for the currently selected hull type.  Hulls are color code as follows:\n\tGreen		- ground movement \n\tBlue		- jumping movement\n\tCyan		- flying movement\n\tMagenta	- climbing movement\n\tRed		- connection disabled", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose: Show node connections with lines
//------------------------------------------------------------------------------
void CC_AI_ShowFlyConnect( void )
{
	// Eventually this will be done by name when mulitple
	// networks are used, but for now have one big AINet
	//static char entName[256];	
	//Q_strncpy( entName, engine->Cmd_Argv(1),sizeof(entName) );
	if ( !g_pAINetworkManager )
		return;

	g_pAINetworkManager->GetEditOps()->SetDebugBits("BigNet",bits_debugOverlayFlyConnections);
	CBaseEntity::m_nDebugPlayer = UTIL_GetCommandClientIndex();

}
static ConCommand ai_show_connect_fly("ai_show_connect_fly", CC_AI_ShowFlyConnect, "Displays the allowed connections between each node for the currently selected hull type.  Hulls are color code as follows:\n\tGreen		- ground movement \n\tBlue		- jumping movement\n\tCyan		- flying movement\n\tMagenta	- climbing movement\n\tRed		- connection disabled", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose: Draw a grid on the screen (good for laying down nodes)
//------------------------------------------------------------------------------
void CC_AI_ShowGrid( void )
{
	// Eventually this will be done by name when mulitple
	// networks are used, but for now have one big AINet
	//static char entName[256];	
	//Q_strncpy( entName, engine->Cmd_Argv(1),sizeof(entName) );
	if ( !g_pAINetworkManager )
		return;

	g_pAINetworkManager->GetEditOps()->SetDebugBits("BigNet",bits_debugOverlayGrid);
	CBaseEntity::m_nDebugPlayer = UTIL_GetCommandClientIndex();
}
static ConCommand ai_show_grid("ai_show_grid", CC_AI_ShowGrid, "Draw a grid on the floor where looking.", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose: NPC step trough AI
//------------------------------------------------------------------------------
void CC_AI_Step( void )
{
	DevMsg("AI Stepping...\n");

	// Start NPC's stepping through tasks
	CAI_BaseNPC::m_nDebugBits |= bits_debugStepAI;
	CAI_BaseNPC::m_nDebugPauseIndex++;
}
static ConCommand ai_step("ai_step", CC_AI_Step, "NPCs will freeze after completing their current task.  To complete the next task, use 'ai_step' again.  To resume processing normally use 'ai_resume'", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose: Resume normal AI processing after stepping
//------------------------------------------------------------------------------
void CC_AI_Resume( void )
{
	DevMsg("AI Resume...\n");

	// End NPC's stepping through tasks
	CAI_BaseNPC::m_nDebugBits &= ~bits_debugStepAI;
}
static ConCommand ai_resume("ai_resume", CC_AI_Resume, "If NPC is stepping through tasks (see ai_step ) will resume normal processing.", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose: Switch to display of next hull type
//------------------------------------------------------------------------------
void CC_AI_NextHull( void )
{
	// Eventually this will be done by name when mulitple
	// networks are used, but for now have one big AINet
	//static char entName[256];	
	//Q_strncpy( entName, engine->Cmd_Argv(1),sizeof(entName) );
	if ( !g_pAINetworkManager )
		return;

	g_pAINetworkManager->GetEditOps()->DrawNextHull("BigNet");
	CBaseEntity::m_nDebugPlayer = UTIL_GetCommandClientIndex();
}
static ConCommand ai_next_hull("ai_next_hull", CC_AI_NextHull, "Cycles through the various hull sizes.  Currently selected hull size is written to the screen.  Controls which connections are shown when ai_show_hull or ai_show_connect commands are used\n\tArguments:	-none-", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose: Show AI nodes
//------------------------------------------------------------------------------
void CC_AI_Nodes( void )
{
	// Eventually this will be done by name when mulitple
	// networks are used, but for now have one big AINet
	//	static char entName[256];	
	//Q_strncpy( entName, engine->Cmd_Argv(1),sizeof(entName) );
	if ( !g_pAINetworkManager )
		return;

	g_pAINetworkManager->GetEditOps()->SetDebugBits("BigNet",bits_debugOverlayNodes);
	CBaseEntity::m_nDebugPlayer = UTIL_GetCommandClientIndex();
}
static ConCommand ai_nodes("ai_nodes", CC_AI_Nodes, "Toggles node display.  First call displays the nodes for the given network as green objects.  Second call  displays the nodes and their IDs.  Nodes are color coded as follows:\n\tGreen		- ground node\n\tCyan		- air node\n\tMagenta	- climb node\n\tGrey		- node not available for selected hull size\n\tOrange 	- node currently locked", FCVAR_CHEAT);


CON_COMMAND(ai_show_node, "Highlight the specified node")
{
	if ( engine->Cmd_Argc() > 1 )
	{
		int node = atoi(engine->Cmd_Argv(1));
		CAI_Node* pAINode = g_pBigAINet->GetNode( node, false );
		if ( pAINode )
		{
			NDebugOverlay::Cross3D(pAINode->GetOrigin(), 1024, 255, 255, 255, true, 5.0 );
		}
	}
}

//------------------------------------------------------------------------------
// Purpose: Show visibility from selected node to all other nodes
//------------------------------------------------------------------------------
void CC_AI_ShowVisibility( void )
{
	// Eventually this will be done by name when mulitple
	// networks are used, but for now have one big AINet
	// static char entName[256];	
	//Q_strncpy( entName, engine->Cmd_Argv(1),sizeof(entName) );
	if ( !g_pAINetworkManager )
		return;

	g_pAINetworkManager->GetEditOps()->SetDebugBits("BigNet",bits_debugOverlayVisibility);

	CAI_Node* pAINode = FindPickerAINode(UTIL_GetCommandClient(), NODE_ANY);
	if (pAINode != NULL)
	{
		g_pAINetworkManager->GetEditOps()->m_iVisibilityNode = pAINode->GetId();
	}
	else
	{
		g_pAINetworkManager->GetEditOps()->m_iVisibilityNode = NO_NODE;
	}

	CBaseEntity::m_nDebugPlayer = UTIL_GetCommandClientIndex();
}
static ConCommand ai_show_visibility("ai_show_visibility", CC_AI_ShowVisibility, "Toggles visibility display for the node that the player is looking at.  Nodes that are visible from the selected node will be drawn in red with yellow lines connecting to the selected node.  Nodes that are not visible from the selected node will be drawn in blue.", FCVAR_CHEAT);


//------------------------------------------------------------------------------
// Purpose: Show what nodes the selected node is connected to using the
//			 netowrk graph
//------------------------------------------------------------------------------
void CC_AI_GraphConnect( void )
{
	if ( !g_pAINetworkManager )
		return;

	// Eventually this will be done by name when mulitple
	// networks are used, but for now have one big AINet
	//static char entName[256];	
	//Q_strncpy( entName, engine->Cmd_Argv(1),sizeof(entName) );
	g_pAINetworkManager->GetEditOps()->SetDebugBits("BigNet",bits_debugOverlayGraphConnect);
	CAI_Node* pAINode = FindPickerAINode(UTIL_GetCommandClient(), NODE_ANY);
	if (pAINode != NULL)
	{
		g_pAINetworkManager->GetEditOps()->m_iGConnectivityNode = pAINode->GetId();
	}
	else
	{
		g_pAINetworkManager->GetEditOps()->m_iGConnectivityNode = NO_NODE;
	}

	CBaseEntity::m_nDebugPlayer = UTIL_GetCommandClientIndex();
}
static ConCommand ai_show_graph_connect("ai_show_graph_connect", CC_AI_GraphConnect, "Toggles graph connection display for the node that the player is looking at.  Nodes that are connected to the selected node by the net graph will be drawn in red with magenta lines connecting to the selected node.  Nodes that are not connected via the net graph from the selected node will be drawn in blue.", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose: Show route triangulation attempts
//------------------------------------------------------------------------------
void CC_NPC_Bipass( void )
{
	SetDebugBits( UTIL_GetCommandClient(),engine->Cmd_Argv(1),OVERLAY_NPC_TRIANGULATE_BIT);
}
static ConCommand npc_bipass("npc_bipass", CC_NPC_Bipass, "Displays the local movement attempts by the given NPC(s) (triangulation detours).  Failed bypass routes are displayed in red, successful bypasses are shown in green.\n\tArguments:   	{entity_name} / {class_name} / no argument picks what player is looking at.", FCVAR_CHEAT);
	
//------------------------------------------------------------------------------
// Purpose: Destroy selected NPC
//------------------------------------------------------------------------------
void CC_NPC_Destroy( void )
{
	SetDebugBits( UTIL_GetCommandClient(),engine->Cmd_Argv(1),OVERLAY_NPC_ZAP_BIT);
}
static ConCommand npc_destroy("npc_destroy", CC_NPC_Destroy, "Removes the given NPC(s) from the universe\nArguments:   	{npc_name} / {npc_class_name} / no argument picks what player is looking at", FCVAR_CHEAT);


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CC_NPC_Kill( void )
{
	SetDebugBits( UTIL_GetCommandClient(),engine->Cmd_Argv(1),OVERLAY_NPC_KILL_BIT);
}
static ConCommand npc_kill("npc_kill", CC_NPC_Kill, "Kills the given NPC(s)\nArguments:   	{npc_name} / {npc_class_name} / no argument picks what player is looking at", FCVAR_CHEAT);


//------------------------------------------------------------------------------
// Purpose: Show selected NPC's enemies
//------------------------------------------------------------------------------
void CC_NPC_Enemies( void )
{
	SetDebugBits( UTIL_GetCommandClient(),engine->Cmd_Argv(1),OVERLAY_NPC_ENEMIES_BIT);
}
static ConCommand npc_enemies("npc_enemies", CC_NPC_Enemies, "Shows memory of NPC.  Draws an X on top of each memory.\n\tEluded entities drawn in blue (don't know where it went)\n\tUnreachable entities drawn in green (can't get to it)\n\tCurrent enemy drawn in red\n\tCurrent target entity drawn in magenta\n\tAll other entities drawn in pink\n\tArguments:   	{npc_name} / {npc class_name} / no argument picks what player is looking at", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose: Show seletected NPC's current enemy and target entity
//------------------------------------------------------------------------------
void CC_NPC_Focus( void )
{
	SetDebugBits( UTIL_GetCommandClient(),engine->Cmd_Argv(1),OVERLAY_NPC_FOCUS_BIT);
}
static ConCommand npc_focus("npc_focus", CC_NPC_Focus, "Displays red line to NPC's enemy (if has one) and blue line to NPC's target entity (if has one)\n\tArguments:   	{npc_name} / {npc class_name} / no argument picks what player is looking at", FCVAR_CHEAT);

ConVar npc_create_equipment("npc_create_equipment", "");
//------------------------------------------------------------------------------
// Purpose: Create an NPC of the given type
//------------------------------------------------------------------------------
void CC_NPC_Create( void )
{
	MDLCACHE_CRITICAL_SECTION();

	bool allowPrecache = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache( true );

	// Try to create entity
	CAI_BaseNPC *baseNPC = dynamic_cast< CAI_BaseNPC * >( CreateEntityByName(engine->Cmd_Argv(1)) );
	if (baseNPC)
	{
		baseNPC->KeyValue( "additionalequipment", npc_create_equipment.GetString() );
		baseNPC->Precache();

		if ( engine->Cmd_Argc() == 3 )
		{
			baseNPC->SetName( AllocPooledString( engine->Cmd_Argv(2) ) );
		}

		DispatchSpawn(baseNPC);
		// Now attempt to drop into the world
		CBasePlayer* pPlayer = UTIL_GetCommandClient();
		trace_t tr;
		Vector forward;
		pPlayer->EyeVectors( &forward );
		AI_TraceLine(pPlayer->EyePosition(),
			pPlayer->EyePosition() + forward * MAX_TRACE_LENGTH,MASK_NPCSOLID, 
			pPlayer, COLLISION_GROUP_NONE, &tr );
		if ( tr.fraction != 1.0)
		{
			if (baseNPC->CapabilitiesGet() & bits_CAP_MOVE_FLY)
			{
				Vector pos = tr.endpos - forward * 36;
				baseNPC->Teleport( &pos, NULL, NULL );
			}
			else
			{
				// Raise the end position a little up off the floor, place the npc and drop him down
				tr.endpos.z += 12;
				baseNPC->Teleport( &tr.endpos, NULL, NULL );
				UTIL_DropToFloor( baseNPC, MASK_NPCSOLID );
			}

			// Now check that this is a valid location for the new npc to be
			Vector	vUpBit = baseNPC->GetAbsOrigin();
			vUpBit.z += 1;

			AI_TraceHull( baseNPC->GetAbsOrigin(), vUpBit, baseNPC->GetHullMins(), baseNPC->GetHullMaxs(), 
				MASK_NPCSOLID, baseNPC, COLLISION_GROUP_NONE, &tr );
			if ( tr.startsolid || (tr.fraction < 1.0) )
			{
				baseNPC->SUB_Remove();
				DevMsg("Can't create %s.  Bad Position!\n",engine->Cmd_Argv(1));
				NDebugOverlay::Box(baseNPC->GetAbsOrigin(), baseNPC->GetHullMins(), baseNPC->GetHullMaxs(), 255, 0, 0, 0, 0);
			}
		}

		baseNPC->Activate();
	}
	CBaseEntity::SetAllowPrecache( allowPrecache );
}
static ConCommand npc_create("npc_create", CC_NPC_Create, "Creates an NPC of the given type where the player is looking (if the given NPC can actually stand at that location).  Note that this only works for npc classes that are already in the world.  You can not create an entity that doesn't have an instance in the level.\n\tArguments:	{npc_class_name}", FCVAR_CHEAT);


//------------------------------------------------------------------------------
// Purpose: Create an NPC of the given type
//------------------------------------------------------------------------------
void CC_NPC_Create_Aimed( void )
{
	MDLCACHE_CRITICAL_SECTION();

	bool allowPrecache = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache( true );

	// Try to create entity
	CAI_BaseNPC *baseNPC = dynamic_cast< CAI_BaseNPC * >( CreateEntityByName(engine->Cmd_Argv(1)) );
	if (baseNPC)
	{
		baseNPC->KeyValue( "additionalequipment", npc_create_equipment.GetString() );
		baseNPC->Precache();
		DispatchSpawn( baseNPC );

		// Now attempt to drop into the world
		QAngle angles;
		CBasePlayer* pPlayer = UTIL_GetCommandClient();
		trace_t tr;
		Vector forward;
		pPlayer->EyeVectors( &forward );
		VectorAngles( forward, angles );
		angles.x = 0; 
		angles.z = 0;
		AI_TraceLine( pPlayer->EyePosition(),
			pPlayer->EyePosition() + forward * MAX_TRACE_LENGTH,MASK_NPCSOLID, 
			pPlayer, COLLISION_GROUP_NONE, &tr );

		if ( tr.fraction != 1.0)
		{
			if (baseNPC->CapabilitiesGet() & bits_CAP_MOVE_FLY)
			{
				Vector pos = tr.endpos - forward * 36;
				baseNPC->Teleport( &pos, &angles, NULL );
			}
			else
			{
				// Raise the end position a little up off the floor, place the npc and drop him down
				tr.endpos.z += 12;
				baseNPC->Teleport( &tr.endpos, &angles, NULL );
				UTIL_DropToFloor( baseNPC, MASK_NPCSOLID );
			}

			// Now check that this is a valid location for the new npc to be
			Vector	vUpBit = baseNPC->GetAbsOrigin();
			vUpBit.z += 1;

			AI_TraceHull( baseNPC->GetAbsOrigin(), vUpBit, baseNPC->GetHullMins(), baseNPC->GetHullMaxs(), 
				MASK_NPCSOLID, baseNPC, COLLISION_GROUP_NONE, &tr );
			if ( tr.startsolid || (tr.fraction < 1.0) )
			{
				baseNPC->SUB_Remove();
				DevMsg("Can't create %s.  Bad Position!\n",engine->Cmd_Argv(1));
				NDebugOverlay::Box(baseNPC->GetAbsOrigin(), baseNPC->GetHullMins(), baseNPC->GetHullMaxs(), 255, 0, 0, 0, 0);
			}
		}
		else
		{
			baseNPC->Teleport( NULL, &angles, NULL );
		}

		baseNPC->Activate();
	}
	CBaseEntity::SetAllowPrecache( allowPrecache );
}
static ConCommand npc_create_aimed("npc_create_aimed", CC_NPC_Create_Aimed, "Creates an NPC aimed away from the player of the given type where the player is looking (if the given NPC can actually stand at that location).  Note that this only works for npc classes that are already in the world.  You can not create an entity that doesn't have an instance in the level.\n\tArguments:	{npc_class_name}", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose: Destroy unselected NPCs
//------------------------------------------------------------------------------
void CC_NPC_DestroyUnselected( void )
{
	CAI_BaseNPC *pNPC = gEntList.NextEntByClass( (CAI_BaseNPC *)NULL );

	while (pNPC)
	{
		if (!(pNPC->m_debugOverlays & OVERLAY_NPC_SELECTED_BIT) && !pNPC->ClassMatches("npc_bullseye"))
		{
			pNPC->m_debugOverlays |= OVERLAY_NPC_ZAP_BIT;
		}
		pNPC = gEntList.NextEntByClass(pNPC);
	}
}
static ConCommand npc_destroy_unselected("npc_destroy_unselected", CC_NPC_DestroyUnselected, "Removes all NPCs from the universe that aren't currently selected", FCVAR_CHEAT);


// FIXME: I can't believe we have no header file for this!
extern CBaseEntity *FindPickerEntity( CBasePlayer *pPlayer );

//------------------------------------------------------------------------------
// Purpose: Freeze or unfreeze the selected NPCs. If no NPCs are selected, the
//			NPC under the crosshair is frozen/unfrozen.
//------------------------------------------------------------------------------
void CC_NPC_Freeze( void )
{
	if (FStrEq(engine->Cmd_Argv(1), "")) 
	{
		//	
		// No NPC was specified, try to freeze selected NPCs.
		//
		bool bFound = false;
		CAI_BaseNPC *npc = gEntList.NextEntByClass( (CAI_BaseNPC *)NULL );
		while (npc)
		{
			if (npc->m_debugOverlays & OVERLAY_NPC_SELECTED_BIT) 
			{
				bFound = true;
				npc->ToggleFreeze();
			}
			npc = gEntList.NextEntByClass(npc);
		}

		if (!bFound)
		{
			//	
			// No selected NPCs, look for the NPC under the crosshair.
			//
			CBaseEntity *pEntity = FindPickerEntity( UTIL_GetCommandClient() );
			if ( pEntity )
			{
				CAI_BaseNPC *pNPC = pEntity->MyNPCPointer();
				if (pNPC)
				{
					pNPC->ToggleFreeze();
				}
			}
		}
	}
	else
	{
		// TODO: look for NPCs by name, classname.
	}
}
static ConCommand npc_freeze("npc_freeze", CC_NPC_Freeze, "Selected NPC(s) will freeze in place (or unfreeze). If there are no selected NPCs, uses the NPC under the crosshair.\n\tArguments:	-none-", FCVAR_CHEAT);


//------------------------------------------------------------------------------
CON_COMMAND(npc_thinknow, "Trigger NPC to think")
{
	CBaseEntity *pEntity = FindPickerEntity( UTIL_GetCommandClient() );
	if ( pEntity )
	{
		CAI_BaseNPC *pNPC = pEntity->MyNPCPointer();
		if (pNPC)
		{
			pNPC->SetThink( &CAI_BaseNPC::CallNPCThink );
			pNPC->SetNextThink( gpGlobals->curtime );
		}
	}
}

//------------------------------------------------------------------------------
// Purpose: Tell selected NPC to go to a where player is looking
//------------------------------------------------------------------------------

void CC_NPC_Teleport( void )
{
	CBasePlayer* pPlayer = UTIL_GetCommandClient();
	trace_t tr;
	Vector forward;
	pPlayer->EyeVectors( &forward );
	AI_TraceLine(pPlayer->EyePosition(),
		pPlayer->EyePosition() + forward * MAX_TRACE_LENGTH,MASK_NPCSOLID, 
		pPlayer, COLLISION_GROUP_NONE, &tr );

	if ( tr.fraction != 1.0)
	{
		CAI_BaseNPC *npc = gEntList.NextEntByClass( (CAI_BaseNPC *)NULL );

		while (npc)
		{
			//Only Teleport one NPC if more than one is selected.
			if (npc->m_debugOverlays & OVERLAY_NPC_SELECTED_BIT) 
			{
                npc->Teleport( &tr.endpos, NULL, NULL );
				break;
			}

			npc = gEntList.NextEntByClass(npc);
		}
	}
}

static ConCommand npc_teleport("npc_teleport", CC_NPC_Teleport, "Selected NPC will teleport to the location that the player is looking (shown with a purple box)\n\tArguments:	-none-", FCVAR_CHEAT);

static ConVar npc_go_do_run( "npc_go_do_run", "1", 0, "Set whether should run on NPC go" );

void CC_NPC_Go( void )
{
	CBasePlayer* pPlayer = UTIL_GetCommandClient();
	trace_t tr;
	Vector forward;
	pPlayer->EyeVectors( &forward );
	AI_TraceLine(pPlayer->EyePosition(),
		pPlayer->EyePosition() + forward * MAX_TRACE_LENGTH,MASK_NPCSOLID, 
		pPlayer, COLLISION_GROUP_NONE, &tr );
	if ( tr.fraction != 1.0)
	{
		CAI_BaseNPC::ForceSelectedGo(pPlayer, tr.endpos, forward, npc_go_do_run.GetBool());
	}
}
static ConCommand npc_go("npc_go", CC_NPC_Go, "Selected NPC(s) will go to the location that the player is looking (shown with a purple box)\n\tArguments:	-none-", FCVAR_CHEAT);


//------------------------------------------------------------------------------
// Purpose: Tell seclected NPC to go to a random node location
//------------------------------------------------------------------------------
void CC_NPC_GoRandom( void )
{
	CAI_BaseNPC::ForceSelectedGoRandom();
}
static ConCommand npc_go_random("npc_go_random", CC_NPC_GoRandom, "Sends all selected NPC(s) to a random node.\n\tArguments:   	-none-", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose: ?Does this work?
//------------------------------------------------------------------------------
void CC_NPC_Reset( void )
{
	CAI_BaseNPC::ClearAllSchedules();
	g_AI_SchedulesManager.LoadAllSchedules();
}
static ConCommand npc_reset("npc_reset", CC_NPC_Reset, "Reloads schedules for all NPC's from their script files\n\tArguments:	-none-", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose: Show the selected NPC's nearest node
//------------------------------------------------------------------------------
void CC_NPC_Nearest( void )
{
	SetDebugBits( UTIL_GetCommandClient(),engine->Cmd_Argv(1),OVERLAY_NPC_NEAREST_BIT);
}
static ConCommand npc_nearest("npc_nearest", CC_NPC_Nearest, "Draw's a while box around the NPC(s) nearest node\n\tArguments:   	{entity_name} / {class_name} / no argument picks what player is looking at  ", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose: Show the selected NPC's route
//------------------------------------------------------------------------------
void CC_NPC_Route( void )
{
	SetDebugBits( UTIL_GetCommandClient(),engine->Cmd_Argv(1),OVERLAY_NPC_ROUTE_BIT);
}
static ConCommand npc_route("npc_route", CC_NPC_Route, "Displays the current route of the given NPC as a line on the screen.  Waypoints along the route are drawn as small cyan rectangles.  Line is color coded in the following manner:\n\tBlue	- path to a node\n\tCyan	- detour around an object (triangulation)\n\tRed	- jump\n\tMaroon - path to final target position\n\tArguments:   	{npc_name} / {npc_class_name} / no argument picks what player is looking at ", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose: Select an NPC
//------------------------------------------------------------------------------
void CC_NPC_Select( void )
{
	SetDebugBits( UTIL_GetCommandClient(),engine->Cmd_Argv(1),OVERLAY_NPC_SELECTED_BIT);
}
static ConCommand npc_select("npc_select", CC_NPC_Select, "Select or deselects the given NPC(s) for later manipulation.  Selected NPC's are shown surrounded by a red translucent box\n\tArguments:   	{entity_name} / {class_name} / no argument picks what player is looking at ", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose: Show combat related data for an NPC
//------------------------------------------------------------------------------
void CC_NPC_Combat( void )
{
	SetDebugBits( UTIL_GetCommandClient(),engine->Cmd_Argv(1),OVERLAY_NPC_SQUAD_BIT);
}
static ConCommand npc_combat("npc_combat", CC_NPC_Combat, "Displays text debugging information about the squad and enemy of the selected NPC  (See Overlay Text)\n\tArguments:   	{npc_name} / {npc class_name} / no argument picks what player is looking at", FCVAR_CHEAT);
// For backwards compatibility
static ConCommand npc_squads("npc_squads", CC_NPC_Combat, "Obsolete.  Replaced by npc_combat", FCVAR_CHEAT);


//------------------------------------------------------------------------------
// Purpose: Show tasks for an NPC
//------------------------------------------------------------------------------
void CC_NPC_Tasks( void )
{
	SetDebugBits( UTIL_GetCommandClient(),engine->Cmd_Argv(1),OVERLAY_NPC_TASK_BIT);
}
static ConCommand npc_tasks("npc_tasks", CC_NPC_Tasks, "Displays detailed text debugging information about the all the tasks of the selected NPC current schedule (See Overlay Text)\n\tArguments:   	{npc_name} / {npc class_name} / no argument picks what player is looking at ", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose: Show tasks (on the console) for an NPC
//------------------------------------------------------------------------------
void CC_NPC_Task_Text( void )
{
	SetDebugBits( UTIL_GetCommandClient(), engine->Cmd_Argv(1), OVERLAY_TASK_TEXT_BIT);
}
static ConCommand npc_task_text("npc_task_text", CC_NPC_Task_Text, "Outputs text debugging information to the console about the all the tasks + break conditions of the selected NPC current schedule\n\tArguments:   	{npc_name} / {npc class_name} / no argument picks what player is looking at ", FCVAR_CHEAT);


//------------------------------------------------------------------------------
// Purpose: Shows all current conditions for an NPC.
//------------------------------------------------------------------------------
void CC_NPC_Conditions( void )
{
	SetDebugBits( UTIL_GetCommandClient(), engine->Cmd_Argv(1), OVERLAY_NPC_CONDITIONS_BIT);
}
static ConCommand npc_conditions("npc_conditions", CC_NPC_Conditions, "Displays all the current AI conditions that an NPC has in the overlay text.\n\tArguments:   	{npc_name} / {npc class_name} / no argument picks what player is looking at", FCVAR_CHEAT);


//------------------------------------------------------------------------------
// Purpose: Show an NPC's viewcone
//------------------------------------------------------------------------------
void CC_NPC_Viewcone( void )
{
	SetDebugBits( UTIL_GetCommandClient(),engine->Cmd_Argv(1),OVERLAY_NPC_VIEWCONE_BIT);
}
static ConCommand npc_viewcone("npc_viewcone", CC_NPC_Viewcone, "Displays the viewcone of the NPC (where they are currently looking and what the extents of there vision is)\n\tArguments:   	{entity_name} / {class_name} / no argument picks what player is looking at", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose: Show an NPC's relationships to other NPCs
//------------------------------------------------------------------------------
void CC_NPC_Relationships( void )
{
	SetDebugBits( UTIL_GetCommandClient(),engine->Cmd_Argv(1), OVERLAY_NPC_RELATION_BIT );
}
static ConCommand npc_relationships("npc_relationships", CC_NPC_Relationships, "Displays the relationships between this NPC and all others.\n\tArguments:   	{entity_name} / {class_name} / no argument picks what player is looking at", FCVAR_CHEAT );

//------------------------------------------------------------------------------
// Purpose: Show an NPC's steering regulations
//------------------------------------------------------------------------------
void CC_NPC_ViewSteeringRegulations( void )
{
	SetDebugBits( UTIL_GetCommandClient(), engine->Cmd_Argv(1), OVERLAY_NPC_STEERING_REGULATIONS);
}
static ConCommand npc_steering("npc_steering", CC_NPC_ViewSteeringRegulations, "Displays the steering obstructions of the NPC (used to perform local avoidance)\n\tArguments:   	{entity_name} / {class_name} / no argument picks what player is looking at", FCVAR_CHEAT);

void CC_NPC_ViewSteeringRegulationsAll( void )
{
	CAI_BaseNPC *pNPC = gEntList.NextEntByClass( (CAI_BaseNPC *)NULL );

	while (pNPC)
	{
		if (!(pNPC->m_debugOverlays & OVERLAY_NPC_SELECTED_BIT))
		{
			pNPC->m_debugOverlays |= OVERLAY_NPC_STEERING_REGULATIONS;
		}
		else
		{
			pNPC->m_debugOverlays &= ~OVERLAY_NPC_STEERING_REGULATIONS;
		}
		pNPC = gEntList.NextEntByClass(pNPC);
	}
}
static ConCommand npc_steering_all("npc_steering_all", CC_NPC_ViewSteeringRegulationsAll, "Displays the steering obstructions of all NPCs (used to perform local avoidance)\n", FCVAR_CHEAT);

//------------------------------------------------------------------------------

//TGB: need a way of hurting npcs to test healthbuffs
CON_COMMAND( npc_hurt, "Hurts target with 25% of maxhealth" )
{
	CBaseEntity *pEntity = FindPickerEntity( UTIL_GetCommandClient() );
	if ( pEntity )
	{
		CAI_BaseNPC *pNPC = pEntity->MyNPCPointer();
		if (pNPC)
		{
			DevMsg("Hurting npc...");
			CTakeDamageInfo info( pNPC, pNPC, pNPC->GetMaxHealth() * 0.25, DMG_GENERIC );
			pNPC->TakeDamage(info);
		}
	}
}

CON_COMMAND( npc_heal, "Heals the target back to full health" )
{
	CBaseEntity *pEntity = FindPickerEntity( UTIL_GetCommandClient() );
	if ( pEntity )
	{
		CAI_BaseNPC *pNPC = pEntity->MyNPCPointer();
		if (pNPC)
		{
			pNPC->SetHealth( pNPC->GetMaxHealth() );
		}
	}
}

CON_COMMAND( npc_ammo_deplete, "Subtracts half of the target's ammo" )
{
	CBaseEntity *pEntity = FindPickerEntity( UTIL_GetCommandClient() );
	if ( pEntity )
	{
		CAI_BaseNPC *pNPC = pEntity->MyNPCPointer();
		if (pNPC && pNPC->GetActiveWeapon())
		{
			pNPC->GetActiveWeapon()->m_iClip1 *= 0.5;
		}
	}
}

CON_COMMAND( ai_clear_bad_links, "Clears bits set on nav links indicating link is unusable " )
{
	CAI_Node *pNode;
	
	for ( int i = 0; i < g_pBigAINet->NumNodes(); i++ )
	{
		pNode = g_pBigAINet->GetNode( i );
		for ( int j = 0; j < pNode->NumLinks(); j++ )
		{
			pNode->GetLinkByIndex( j )->m_LinkInfo &= ~bits_LINK_STALE_SUGGESTED;
		}
	}
}

#ifdef VPROF_ENABLED

CON_COMMAND(ainet_generate_report, "Generate a report to the console.")
{
	g_VProfCurrentProfile.OutputReport( VPRT_FULL, "AINet" );
}

CON_COMMAND(ainet_generate_report_only, "Generate a report to the console.")
{
	g_VProfCurrentProfile.OutputReport( VPRT_FULL, "AINet", g_VProfCurrentProfile.BudgetGroupNameToBudgetGroupID( "AINet" ) );
}

#endif

//------------------------------------------------------------------------------
// LAWYER:  Entity specific selection tool
//------------------------------------------------------------------------------
void CC_Conq_NPC_Select_Index( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );   //find a pointer to the player that the client controls
	if (!pPlayer || pPlayer->GetTeamNumber() != 3)
	{
//		Msg("You aren't a Zombiemaster, and can't do that.\n");
		return;
	}
	//The meat of the selection tool
	CBaseEntity *pEntity = NULL;
	CBaseCombatCharacter *pSelector = NULL;
//	CZombieManipulate *pZombieManipulate = NULL;

	if (pPlayer)
	{
		int entityIndex = atoi(engine->Cmd_Argv(1) ); //LAWYER:  Collect the entity's index

		pEntity = UTIL_EntityByIndex( entityIndex );
		if (pEntity)
		{
/*			pZombieManipulate = dynamic_cast< CZombieManipulate * >(pEntity);
			if (pZombieManipulate)
			{ //LAWYER: It's a manipulatable!  DO SOMETHING!
						//We should stick a gump in here, but for now, we'll skip it
						Msg("Activated!\n");
						pZombieManipulate->Trigger(pPlayer);
						return;
			}*/
			pSelector = dynamic_cast< CBaseCombatCharacter * >(pEntity);
			
			if (pSelector == NULL)
			{ //It's not an NPC.  But, is it a manipulatable?
					
					
//				Msg( "Cannot be selected\n" );
				return;
					
			}
				//if (pSelector->IsPlayer() || pSelector->IsNPC()) //LAWYER:  Make checks for Manipulatables here
				//{
				//	DevMsg("That's not controllable\n");
				//}
					//Right, it's on our team, let's work magic.
					//Check if it's already selected
					if (pSelector->m_pConqSelector != pPlayer)
					{
						pSelector->m_pConqSelector = dynamic_cast< CBaseCombatCharacter * >(pPlayer);
						pSelector->m_bConqSelected = true;
						//Msg( "Selected\n" );
					}
					else
					{
						pSelector->m_bConqSelected = false; //LAWYER:  This needs fixing
						pSelector->m_pConqSelector = NULL;
					//	Msg( "Deselected\n" );
					}
			
		}
	}		

}
static ConCommand conq_npc_select_index("conq_npc_select_index", CC_Conq_NPC_Select_Index, "Selects an NPC by its index");
//------------------------------------------------------------------------------
// LAWYER:  Sphere select
//------------------------------------------------------------------------------
void CC_Conq_NPC_Select_Sphere( void )
{

	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );   //find a pointer to the player that the client controls
	if (!pPlayer || pPlayer->GetTeamNumber() != 3)
	{
//		Msg("You aren't a Zombiemaster, and can't do that.\n");
		return;
	}
	CBaseEntity *pIterated = NULL;
	CAI_BaseNPC *pSelector = NULL;
	//LAWYER:  Build a coordinate based vector here

	Vector forward;
	Vector tracetarget;
	VectorNormalize( tracetarget );

	tracetarget.x = atof(engine->Cmd_Argv(1) );
	tracetarget.y = atof(engine->Cmd_Argv(2) );
	tracetarget.z = atof(engine->Cmd_Argv(3) );
	if (pPlayer)
	{
			//Now we have a traceresult, find some Entities and give them the command!
		while ( (pIterated = gEntList.FindEntityInSphere( pIterated, tracetarget, 256 )) != NULL ) //Should probably be a smaller search area.  Large games could be squidged by this function
		{	//LAWYER:  We could do this by doing a FindEntityByName and going through the list of NPCs, but that's unweildy.
				pSelector = dynamic_cast< CAI_BaseNPC * >(pIterated); //Check if it's a commandable character
				if (pSelector)
				{//We have a valid NPC!  Whoopaaahhh!
					//Now to check if it's on our team
//					if (pSelector->GetConqTeam() == pPlayer->GetConqTeam())
//					{ //It's on our team!  Check if it's selected!

					//Right, it's on our team, let's work magic.
					//Check if it's already selected
					if (pSelector->m_pConqSelector != pPlayer)
					{
						pSelector->m_pConqSelector = dynamic_cast< CBaseCombatCharacter * >(pPlayer);
						pSelector->m_bConqSelected = true;

						//add to selected zombies list
						gEntList.m_ZombieSelected.AddToTail(pSelector);
					}

//					}

				}
		}
	}
	//TGB: update selected count
	pPlayer->m_iZombieSelected = gEntList.m_ZombieSelected.Count();

	DevMsg("\nZombies in selected list (post-sphere select): %i", gEntList.m_ZombieSelected.Count());


}
static ConCommand conq_npc_select_sphere("conq_npc_select_sphere", CC_Conq_NPC_Select_Sphere, "Selects units in a sphere");

void ZM_CreateSquad()
{	
	//qck: If there isn't a manager entity, create one.
	if(manager == NULL)
		manager = dynamic_cast<CZombieGroupManager *>(CreateEntityByName("zm_group_manager"));

	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );  
	
	if (!pPlayer || !pPlayer->IsZM())
		return;
	

	//qck: Don't create zombie groups if you haven't selected any zombies
	if(gEntList.m_ZombieSelected.Count() < 1)
	{
		ClientPrint( pPlayer, HUD_PRINTTALK, "No zombies selected!\n" );
		return;
	}

	CZombieGroup* newGroup = dynamic_cast< CZombieGroup* >(CreateEntityByName("zm_group"));
	
	if (!newGroup) return;

	newGroup->Reset();

	for(int i=0; i < gEntList.m_ZombieSelected.Count(); i++)
	{
		CBaseEntity* pIterator = gEntList.m_ZombieSelected[i];

		if(!newGroup->m_ZombieGroupMembers.HasElement(pIterator)) //LAWYER:  We need a check in here to make sure that zombies aren't added twice
		{
			newGroup->m_ZombieGroupMembers.AddToTail(pIterator);
			DevMsg("%i Squad Member\n", i);
		}
	}
	
	manager->AddGroup( newGroup, pPlayer);
	ClientPrint( pPlayer, HUD_PRINTTALK, "New group created\n" );
}

static ConCommand zm_createsquad("zm_createsquad", ZM_CreateSquad, "Create a squad from the currently selected units");

//qck: Teleport to highlighted squad, select its members.
//qck: Still need to actually do the "teleport" part, heh
void CC_ZM_GotoAndSelectSquad()
{
	
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );
	if (!pPlayer || !pPlayer->IsZM()) return;

	//qck: Grab the handle serial number from the client, find out which group it belongs to, snap origin and angles.
	int serialNumber = atoi(engine->Cmd_Argv(1) );

	//TGB FIXME: manager was unprotected here, added this, but perhaps you want to create a manager ent here qck?
	//qck: we may as well. It's not needed exactly, but it doesn't hurt anything. 
	if (manager == NULL) 
		manager = dynamic_cast<CZombieGroupManager *>(CreateEntityByName("zm_group_manager"));

	//qck: deselect any currently selected zombies
	for(int i=0; i < gEntList.m_ZombieSelected.Count(); i++)
	{
		CNPC_BaseZombie* pSelector = dynamic_cast< CNPC_BaseZombie * >(gEntList.m_ZombieSelected[i]);
		if (pSelector)
		{
			pSelector->m_bConqSelected = false;
			pSelector->m_pConqSelector = NULL;
		}
		
	}		

	gEntList.m_ZombieSelected.Purge();


	for(int i=0; i < manager->m_pZombieLists.Count(); i++)
	{
		int match = manager->m_pZombieLists[i].GetSerialNumber();

		//DevMsg("Serial number of group %i: %i\n", i, match);
		//DevMsg("Serial number received: %i\n", serialNumber);

		if( match == serialNumber)
		{
			EHANDLE handle = manager->m_pZombieLists.Element(i);
			CBaseEntity* entGroup = handle;
			if (!entGroup) return;

			CZombieGroup* zombieGroup = dynamic_cast<CZombieGroup*>(entGroup);

			if (!zombieGroup) return; //TGB: pointer safety GET

			int counter = zombieGroup->m_ZombieGroupMembers.Count();
			DevMsg("Number of zombies in group: %i\n", counter);

			for(int j=0; j<counter; j++)
			{

				CBaseCombatCharacter *pZombie = dynamic_cast< CBaseCombatCharacter * >(zombieGroup->m_ZombieGroupMembers[j]);
				if(!gEntList.m_ZombieSelected.HasElement(pZombie)) //LAWYER:  Make sure the same Zombie isn't selected twice
				{
					if(j == 0) //goto the first zombie in the group 
					{
						Vector		newOrigin;
						Vector		vecSpot;
						Vector		vecTarget;

						trace_t		tr;

						const int	positions = 6;

						int			tries = 0;
						
						//qck: Positions to test for teleporting to zombie
						//Position 5 is the position of last resort (zombies in really tight spaces)
						//TGB: all these positions are really close to the ground in my opinion
						//		moved most of them up, and added extra medium height one
						int			offsetx[positions] = {200,	0,	-200,	0,		100,	10};
						int			offsety[positions] = {0,	200, 0,		-200,	100,	10};
						int			offsetz[positions] = {400,	300, 250,	200,	75,		10};

						do
						{
							tries++;

							newOrigin.x = (pZombie->GetLocalOrigin().x + offsetx[tries]); 
							newOrigin.y = (pZombie->GetLocalOrigin().y + offsety[tries]);
							newOrigin.z = (pZombie->GetLocalOrigin().z + offsetz[tries]);

							pPlayer->SetAbsOrigin(newOrigin); //qck: kinda ugly

							vecSpot = pPlayer->BodyTarget( pPlayer->GetAbsOrigin() , false );
							vecTarget = pZombie->BodyTarget( pZombie->GetAbsOrigin() , false );
							UTIL_TraceLine( vecTarget, vecSpot, MASK_OPAQUE, NULL, COLLISION_GROUP_NONE, &tr );
						}while(tr.fraction != 1.0 && tries < positions);

						DevMsg("Using zoom position index %i\n", tries);

						QAngle targetangles;
						Vector vecDir;

						vecDir = pZombie->GetAbsOrigin() - pPlayer->EyePosition();
						VectorNormalize(vecDir);
						VectorAngles( vecDir, targetangles );

						targetangles.z = 0; //qck: no roll

						pPlayer->SnapEyeAngles(targetangles);
					}

					//TGB: FIXME: if the zombie ptr is null here, should the zombie be removed from the group list?
					if (pZombie) //added safety check
					{
						pZombie->m_pConqSelector = dynamic_cast< CBaseCombatCharacter * >(pPlayer);
						pZombie->m_bConqSelected = true;
						gEntList.m_ZombieSelected.AddToTail( pZombie );
						pPlayer->m_iZombieSelected = gEntList.m_ZombieSelected.Count();
					}
				}
				//DevMsg("Added\n");
			}

			break;
		}
		else
		{
			//DevMsg("No match found.\n"); 
		}
	}

	ClientPrint( pPlayer, HUD_PRINTTALK, "Group members selected...\n" );
}
static ConCommand zm_gotosquad("zm_gotosquad", CC_ZM_GotoAndSelectSquad, "Go to the currently selected squad (in the combo menu) and select them");

//qck: Think I'll make this the one function which takes care of anything from the client.
void ZM_CC_Print_HUD()
{	
	const char* text = engine->Cmd_Args();

	//Msg("String reads %s\n", text);
	//Msg("String size: %i\n", sizeof(text));

	CBasePlayer* pPlayer = ToBasePlayer( UTIL_GetCommandClient() );
	if(pPlayer)
		ClientPrint( pPlayer, HUD_PRINTTALK, text);
}

static ConCommand zm_hudchat("zm_hudchat", ZM_CC_Print_HUD, "Print something to the HUD");

//TGB: am I allowed to just dump a function in here? I'm just going to
//		kinda ugly though, as I fear it may be semi-global now, but then again so are the concommands
//		prefixed name with zm_ to avoid conflicts
int ZM_ScreenTransform( const Vector& point, Vector& screen, VMatrix& worldToScreen )
{
// UNDONE: Clean this up some, handle off-screen vertices
	float w;
	//const VMatrix &worldToScreen = engine->WorldToScreenMatrix();

	screen.x = worldToScreen[0][0] * point[0] + worldToScreen[0][1] * point[1] + worldToScreen[0][2] * point[2] + worldToScreen[0][3];
	screen.y = worldToScreen[1][0] * point[0] + worldToScreen[1][1] * point[1] + worldToScreen[1][2] * point[2] + worldToScreen[1][3];
//	z		 = worldToScreen[2][0] * point[0] + worldToScreen[2][1] * point[1] + worldToScreen[2][2] * point[2] + worldToScreen[2][3];
	w		 = worldToScreen[3][0] * point[0] + worldToScreen[3][1] * point[1] + worldToScreen[3][2] * point[2] + worldToScreen[3][3];

	// Just so we have something valid here
	screen.z = 0.0f;

	bool behind;
	if( w < 0.001f )
	{
		behind = true;
		screen.x *= 100000;
		screen.y *= 100000;
	}
	else
	{
		behind = false;
		float invw = 1.0f / w;
		screen.x *= invw;
		screen.y *= invw;
	}

	return behind;
}

//cheat command for testing
void CC_ZombieMaster_GiveResources (void)
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );   //find a pointer to the player that the client controls
	if (!pPlayer || (pPlayer && pPlayer->GetTeamNumber() != 3))
	{
		Msg("You're not ZM.\n");
		return;
	}

	pPlayer->m_iZombiePool += atoi(engine->Cmd_Argv(1));


}
static ConCommand zm_zombiemaster_giveresources_cc("zm_giveresources", CC_ZombieMaster_GiveResources, "Cheat. Give ZM X amount of extra res.", FCVAR_CHEAT);


static ConVar zm_stickyselect("zm_stickyselect", "0", FCVAR_ARCHIVE, "Toggles whether selected zombies are unselected when ZM selects a different zombie by default. 0 = yes. Hold space while selecting to temporarily enable sticky selection.");

//qck: Nightvision for the zombie master
void CC_ZM_NightVision()
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );   //find a pointer to the player that the client controls
	if (!pPlayer || !pPlayer->IsZM()) 
		return;

	if (Q_strcmp( STRING(pPlayer->GetScreenEffect()), "none" ) == 0 )
	{
		pPlayer->SetScreenEffect(AllocPooledString("nightvision"));
	}
	else if (Q_strcmp( STRING(pPlayer->GetScreenEffect()), "nightvision" ) == 0 ) 
	{
		pPlayer->SetScreenEffect(AllocPooledString("none"));
	}

	/*bool value = atoi(engine->Cmd_Argv(1));
	if(value)
		pPlayer->SetScreenEffect(AllocPooledString("nightvision"));
	else if (!value)
		pPlayer->SetScreenEffect(AllocPooledString("none"));*/
}


static ConCommand zm_nightvison( "zm_nightvision", CC_ZM_NightVision, "Nightvision for the zombie master" );

////qck: Nightvision for the zombie master
//void CC_ZM_NightVisionToggle()
//{
//	bool value = zm_night_vision.GetBool();
//	zm_night_vision.SetValue(!value);
//}


//static ConCommand zm_nightvison_toggle( "zm_nightvision_toggle", CC_ZM_NightVisionToggle, "Nightvis toggle" );

void CC_ZombieMaster_AlternateSelect ( void )
{

	//DevMsg("Zombies in selected list (pre-new select): %i\n", 0gEntList.m_ZombieSelected.Count());

	//CBaseEntity *pLoopEntity = NULL;
	CAI_BaseNPC *pSelector = NULL;
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );   //find a pointer to the player that the client controls
	if (!pPlayer || !pPlayer->IsZM()) return;

	//get mouse coords
	int mousex = atoi(engine->Cmd_Argv(1) );
	int mousey = atoi(engine->Cmd_Argv(2) );

	//get world->screen matrix
	VMatrix worldToScreen(
		atof(engine->Cmd_Argv(3)), atof(engine->Cmd_Argv(4)), atof(engine->Cmd_Argv(5)), atof(engine->Cmd_Argv(6)),
		atof(engine->Cmd_Argv(7)), atof(engine->Cmd_Argv(8)), atof(engine->Cmd_Argv(9)), atof(engine->Cmd_Argv(10)),
		0, 0, 0, 0,
		atof(engine->Cmd_Argv(11)), atof(engine->Cmd_Argv(12)), atof(engine->Cmd_Argv(13)), atof(engine->Cmd_Argv(14))
		);

	//get screen size to determine selectable regions
	const int scrWidth = atoi(engine->Cmd_Argv(15));
	const int scrHeight= atoi(engine->Cmd_Argv(16));

	//TGB: get sticky select setting of this command
	const bool sticky = ( atoi(engine->Cmd_Argv(17)) ) ? true : false; //explicit int->bool conversion

	//TGB: resolution adjustment factor, assuming an 800x600 res as the one the constants were defined for
	//		seeing as I often run it 800x600 windowed while testing
	const float scrScaleFactor = (float)scrHeight / 600.0;

	//loop through list of zombies and check if they're in the selection region
	for ( int i = 0; i < gEntList.m_ZombieList.Count(); i++)
	{
		//for testing purposes this is the same as sphere select for now
		pSelector = dynamic_cast< CAI_BaseNPC * >(gEntList.m_ZombieList[i]);
		if (pSelector)
		{
			Vector vNPCpos = pSelector->GetAbsOrigin();
			Vector vNPCscreen;
			ZM_ScreenTransform(vNPCpos, vNPCscreen, worldToScreen);
			
			//DevMsg("\nNPC @ x: %f, y: %f ;", vNPCscreen.x, vNPCscreen.y );

			//attempt at converting the normalized coords to useful stuff
			int iX =  0.5 * vNPCscreen[0] * scrWidth;
			int iY = -0.5 * vNPCscreen[1] * scrHeight;
			iX += 0.5 * scrWidth;
			iY += 0.5 *	scrHeight;

			//DevMsg(" Fixed @ x: %i, y: %i ;", iX, iY);

			//TGB:
			//The selectregion needs to be scaled to the distance between the NPC and
			//the player. The distance can probably be calculated using vector stuff
			//on vectors of both locations. Hopefully that distance can then be normalized or something
			//into a positive float that can be used to scale the base region
			

			//get world locations for npc and player
			Vector vDistance;
			
			Vector vZMpos = pPlayer->GetAbsOrigin();
			VectorSubtract( vNPCpos, vZMpos, vDistance );

			//clunky but as good as any I've come up with
			float fDistFactor = 1 - (vDistance.Length() / 1000); //1000 = max distance for min regionsize
			if (fDistFactor < 0.02) 
				fDistFactor = 0.02;
			
			//DevMsg("Distvec: %f %f %f ; Distfactor: %f ;\n", vDistance.x, vDistance.y, vDistance.z, fDistFactor);
			//DevMsg("\nNPC mag: %f ; ZM mag: %f ; Dist mag: %f ; fDistFactor: %f", vNPCpos.Length(), vZMpos.Length(), vDistance.Length(), fDistFactor );

			//compare coordinates
			//should perhaps scale region part to game's resolution somehow
			const int region = (ZM_SELECTAREA_NPC * scrScaleFactor) * (fDistFactor * fDistFactor);
			
			if ( iX > mousex-region && iX < mousex+region && iY < mousey+region && iY > mousey-region)
			{
				//DevMsg(" NPC is within select-region!");
				
				CBaseCombatCharacter *pZombie = dynamic_cast< CBaseCombatCharacter * >(pSelector);

				//qck: quick check to make sure zombies are in our los. Prevents them from being selected if they are behind walls, etc.
				//still works fine through grates, windows, and so forth


				//is it selected?
				if (pZombie->m_pConqSelector != pPlayer)
				{

					trace_t		tr;
					Vector		vecSpot;
					Vector		vecTarget;


					vecSpot = pPlayer->BodyTarget( pPlayer->GetAbsOrigin() , false );
					vecTarget = pZombie->BodyTarget( pZombie->GetAbsOrigin() , false );
					UTIL_TraceLine( vecSpot, vecTarget, CONTENTS_SOLID, NULL, COLLISION_GROUP_NONE, &tr ); //test at feet level

					if ( tr.fraction == 1.0 )
					{
						DevMsg("Zombie in view\n");
					}
					else 
					{
						DevMsg("Zombie not in view\n");
					}

					//nope, select it
					pZombie->m_pConqSelector = dynamic_cast< CBaseCombatCharacter * >(pPlayer);
					pZombie->m_bConqSelected = true;
					
					//TGB: if we're not using sticky selection, deselect all currently selected zombies
					if (sticky == false && zm_stickyselect.GetBool() == false)
					{
						for ( int i = 0; i < gEntList.m_ZombieSelected.Count(); i++)
						{
							CBaseCombatCharacter *pSelZombie = dynamic_cast< CBaseCombatCharacter * >(gEntList.m_ZombieSelected[i]);
							pSelZombie->m_bConqSelected = false;
							pSelZombie->m_pConqSelector = NULL;
						}
						//purge selected list
						gEntList.m_ZombieSelected.Purge();
					}

					//add to selected zombies list
					gEntList.m_ZombieSelected.AddToTail(pZombie);
					//TGB: update selected count
					pPlayer->m_iZombieSelected = gEntList.m_ZombieSelected.Count();
					return; //selected something
				}
			}
			
		}
	} //end while

	//check for manipulates/spawns
	CBaseEntity *pSelectEnt = NULL;
	//loop through list of spawns and check if they're in the selection region
	for ( int i = 0; i < gEntList.m_ZombieSpawns.Count(); i++)
	{
		//spawns go first
		pSelectEnt = gEntList.m_ZombieSpawns[i];
		if (pSelectEnt)
		{
			Vector vEntpos = pSelectEnt->GetAbsOrigin();
			Vector vEntscreen;
			ZM_ScreenTransform(vEntpos, vEntscreen, worldToScreen);

			//attempt at converting the normalized coords to useful stuff
			int iX =  0.5 * vEntscreen[0] * scrWidth;
			int iY = -0.5 * vEntscreen[1] * scrHeight;
			iX += 0.5 * scrWidth;
			iY += 0.5 *	scrHeight;

			//get world locations for ent and player
			Vector vDistance;
			Vector vZMpos = pPlayer->GetAbsOrigin();
			VectorSubtract( vEntpos, vZMpos, vDistance );
			
			const float select_dist = vDistance.Length();
			//clunky but as good as any I've come up with
			float fDistFactor = 1.0f;
			
			//more specific calculations for different distances
			//this is to avoid an area that is only just selectable far away but huge when near
			 if (select_dist < 500)
				fDistFactor = 1 - (select_dist / 1000);
			 else if (select_dist < 900)
				fDistFactor = 1 - (select_dist / 1300);
			 else if (select_dist < 2000)
				fDistFactor = 1 - (select_dist / 1700);
			 else
				fDistFactor = 1 - (select_dist / 2000);
			 
			if (fDistFactor < 0.3) 
				fDistFactor = 0.3;

	
			//compare coordinates
			//should perhaps scale region part to game's resolution somehow
			const float region = (ZM_SELECTAREA_OTHER * scrScaleFactor) * (fDistFactor * fDistFactor);
			//DevMsg("Distance = %f ; fDistFactor = %f ; region = %f\n", select_dist, fDistFactor, region);
			if ( iX > mousex-region && iX < mousex+region && iY < mousey+region && iY > mousey-region)
			{
				engine->ClientCommand( pPlayer->edict(), UTIL_VarArgs( "zombiemaster_select_index %i %i", pSelectEnt->entindex(), 1 ) );
				return; //selected something

			}
			
		}
	} //end spawn while

	//loop through list of manips and check if they're in the selection region
	for ( int i = 0; i < gEntList.m_ZombieManipulates.Count(); i++)
	{
		//spawns go first
		pSelectEnt = gEntList.m_ZombieManipulates[i];
		if (pSelectEnt)
		{
			Vector vEntpos = pSelectEnt->GetAbsOrigin();
			Vector vEntscreen;
			ZM_ScreenTransform(vEntpos, vEntscreen, worldToScreen);

			//attempt at converting the normalized coords to useful stuff
			int iX =  0.5 * vEntscreen[0] * scrWidth;
			int iY = -0.5 * vEntscreen[1] * scrHeight;
			iX += 0.5 * scrWidth;
			iY += 0.5 *	scrHeight;

			//get world locations for ent and player
			Vector vDistance;
			Vector vZMpos = pPlayer->GetAbsOrigin();
			VectorSubtract( vEntpos, vZMpos, vDistance );


			const float select_dist = vDistance.Length();
			//clunky but as good as any I've come up with
			float fDistFactor = 1.0f;
			
			//more specific calculations for different distances
			//this is to avoid an area that is only just selectable far away but huge when near
			 if (select_dist < 500)
				fDistFactor = 1 - (select_dist / 1000);
			 else if (select_dist < 900)
				fDistFactor = 1 - (select_dist / 1300);
			 else if (select_dist < 2000)
				fDistFactor = 1 - (select_dist / 1700);
			 else
				fDistFactor = 1 - (select_dist / 2000);
			 
			if (fDistFactor < 0.3) 
				fDistFactor = 0.3;
	
			//compare coordinates
			//should perhaps scale region part to game's resolution somehow
			const float region = (ZM_SELECTAREA_OTHER * scrScaleFactor) * (fDistFactor * fDistFactor);
			//DevMsg("Distance = %f ; fDistFactor = %f ; region = %f\n", vDistance.Length(), fDistFactor, region);
			if ( iX > mousex-region && iX < mousex+region && iY < mousey+region && iY > mousey-region)
			{
				engine->ClientCommand( pPlayer->edict(), UTIL_VarArgs( "zombiemaster_select_index %i %i", pSelectEnt->entindex(), 1 ) );
				return; //selected something

			}
			
		}
	} //end manip while

	for ( int i = 0; i < gEntList.m_ZombieAmbushPoints.Count(); i++)
	{
		//qck: Check for ambush nodes
		pSelectEnt = gEntList.m_ZombieAmbushPoints[i];
		if (pSelectEnt)
		{
			Vector vEntpos = pSelectEnt->GetAbsOrigin();
			Vector vEntscreen;
			ZM_ScreenTransform(vEntpos, vEntscreen, worldToScreen);

			//attempt at converting the normalized coords to useful stuff
			int iX =  0.5 * vEntscreen[0] * scrWidth;
			int iY = -0.5 * vEntscreen[1] * scrHeight;
			iX += 0.5 * scrWidth;
			iY += 0.5 *	scrHeight;

			//get world locations for ent and player
			Vector vDistance;
			Vector vZMpos = pPlayer->GetAbsOrigin();
			VectorSubtract( vEntpos, vZMpos, vDistance );
			
			const float select_dist = vDistance.Length();
			//clunky but as good as any I've come up with
			float fDistFactor = 1.0f;
			
			//more specific calculations for different distances
			//this is to avoid an area that is only just selectable far away but huge when near
			 if (select_dist < 500)
				fDistFactor = 1 - (select_dist / 1000);
			 else if (select_dist < 900)
				fDistFactor = 1 - (select_dist / 1300);
			 else if (select_dist < 2000)
				fDistFactor = 1 - (select_dist / 1700);
			 else
				fDistFactor = 1 - (select_dist / 2000);
			 
			if (fDistFactor < 0.3) 
				fDistFactor = 0.3;

	
			//compare coordinates
			//should perhaps scale region part to game's resolution somehow
			const float region = (ZM_SELECTAREA_OTHER * scrScaleFactor) * (fDistFactor * fDistFactor);
			//DevMsg("Distance = %f ; fDistFactor = %f ; region = %f\n", select_dist, fDistFactor, region);
			if ( iX > mousex-region && iX < mousex+region && iY < mousey+region && iY > mousey-region)
			{
				engine->ClientCommand( pPlayer->edict(), UTIL_VarArgs( "zombiemaster_select_index %i %i", pSelectEnt->entindex(), 1 ) );
				return; //selected something

			}
			
		}
	} //end ambush while

	//we didn't select anything, so we clicked on the ground or something

	//if we are holding space, ie. using temp. sticky select, we won't deselect for ease of use (ie. misclick != screaming fury)
	if ( !sticky )
	{
		//loop through (selected) zombies and deselect them
		for ( int i = 0; i < gEntList.m_ZombieSelected.Count(); i++)
		{
			//DevMsg("\nDeselecting zombie.");
			CBaseCombatCharacter *pZombie = dynamic_cast< CBaseCombatCharacter * >(gEntList.m_ZombieSelected[i]);
			pZombie->m_bConqSelected = false;
			pZombie->m_pConqSelector = NULL;
		}
		//purge selected list
		gEntList.m_ZombieSelected.Purge();
	}
	
}
static ConCommand zm_altselect_cc("zm_altselect_cc", CC_ZombieMaster_AlternateSelect, "Alt selectiont test clientcommand");

ConVar	zm_poweruser("zm_poweruser", "0", FCVAR_ARCHIVE, "Use this if you're a crazy awesome power user like TGB, and want to select stuff through a wall with your laser eyes.");
void CC_ZombieMaster_ZoneSelect ( void )
{
	//DevMsg("\nZombies in selected list (pre-new select): %i", gEntList.m_ZombieSelected.Count());


	//CBaseEntity *pLoopEntity = NULL;
	CAI_BaseNPC *pSelector = NULL;
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );   //find a pointer to the player that the client controls
	if (!pPlayer || !pPlayer->IsZM())  return;

	//get mouse coords
	const int topLeftX = atoi(engine->Cmd_Argv(1) );
	const int topLeftY = atoi(engine->Cmd_Argv(2) );

	//get world->screen matrix
	//TGB: this whole "sending a matrix through a console command" still makes me ill
	VMatrix worldToScreen(
		atof(engine->Cmd_Argv(3)), atof(engine->Cmd_Argv(4)), atof(engine->Cmd_Argv(5)), atof(engine->Cmd_Argv(6)),
		atof(engine->Cmd_Argv(7)), atof(engine->Cmd_Argv(8)), atof(engine->Cmd_Argv(9)), atof(engine->Cmd_Argv(10)),
		0, 0, 0, 0,
		atof(engine->Cmd_Argv(11)), atof(engine->Cmd_Argv(12)), atof(engine->Cmd_Argv(13)), atof(engine->Cmd_Argv(14))
		);

	const int scrWidth = atoi(engine->Cmd_Argv(15));
	const int scrHeight= atoi(engine->Cmd_Argv(16));

	const int botRightX = atoi(engine->Cmd_Argv(17));
	const int botRightY = atoi(engine->Cmd_Argv(18));

	//DevMsg("Attempting to zone select between %i %i and %i %i \n", topLeftX, topLeftY, botRightX, botRightY);

	//TGB: get sticky select setting of this command
	const bool sticky = ( atoi(engine->Cmd_Argv(19)) ) ? true : false; //explicit int->bool conversion

	//TGB: by popular request, deselect selected zombies when zone-selecting new ones
	if ( !sticky && !zm_stickyselect.GetBool() )
	{
		//loop through (selected) zombies and deselect them
		for ( int i = 0; i < gEntList.m_ZombieSelected.Count(); i++)
		{
			//DevMsg("\nDeselecting zombie.");
			CBaseCombatCharacter *pZombie = dynamic_cast< CBaseCombatCharacter * >(gEntList.m_ZombieSelected[i]);
			pZombie->m_bConqSelected = false;
			pZombie->m_pConqSelector = NULL;
		}
		//purge selected list
		gEntList.m_ZombieSelected.Purge();
	}

	bool selected_units = false;

	//loop through list of zombies and check if they're in the selection region
	for ( int i = 0; i < gEntList.m_ZombieList.Count(); i++)
	{
		pSelector = dynamic_cast< CAI_BaseNPC * >(gEntList.m_ZombieList[i]);
		if (pSelector)
		{
			//DevMsg("NPC found... ");

			Vector vNPCpos = pSelector->GetAbsOrigin();
			Vector vNPCscreen;
			ZM_ScreenTransform(vNPCpos, vNPCscreen, worldToScreen);

			//attempt at converting the normalized coords to useful stuff
			int iX =  0.5 * vNPCscreen[0] * scrWidth;
			int iY = -0.5 * vNPCscreen[1] * scrHeight;
			iX += 0.5 * scrWidth;
			iY += 0.5 *	scrHeight;

			//DevMsg("at x: %i, y: %i ...", iX, iY);

			//compare coordinates
			if ( iX > topLeftX && iX < botRightX && iY > topLeftY && iY < botRightY)
			{
				//DevMsg(" NPC is within select-region!\n");
				
				CBaseCombatCharacter *pZombie = dynamic_cast< CBaseCombatCharacter * >(pSelector);

				//qck: Quick check on los; don't want selections through walls and such. Trace masked for windows, grates, etc.
				trace_t		tr;
				trace_t		tr2;
				Vector		vecSpot;
				Vector		vecTarget;
				Vector		vecHeadTarget = vecTarget;

				vecHeadTarget.z += 64;


				vecSpot = pPlayer->BodyTarget( pPlayer->GetAbsOrigin() , false );
				vecTarget = pZombie->BodyTarget( pZombie->GetAbsOrigin() , false );

				//TGB: some changes tried here because the two aft hulks in the "bash open door" manip in warehouse are hard to select
				// they didn't really help, but I'm leaving them in for now.
				CTraceFilterNoNPCs traceFilter( NULL, COLLISION_GROUP_NONE );
				UTIL_TraceLine( vecSpot, vecTarget, MASK_OPAQUE, &traceFilter, &tr ); //test at feet level

				UTIL_TraceLine( vecSpot, vecHeadTarget, MASK_OPAQUE, &traceFilter, &tr2 ); //test at head level
				
				if(!(zm_poweruser.GetBool()))
				{
					if ( (tr.fraction != 1.0) && (tr2.fraction != 1.0) )
					{
						//DevMsg("Zombie not in los: feet: %f head: %f wrld: %i\n", tr.fraction, tr.fraction, (int)tr.DidHitWorld());
						return;
					}
				}

				//is it selected?
				if (pZombie->m_pConqSelector != pPlayer)
				{
					//nope, select it
					pZombie->m_pConqSelector = dynamic_cast< CBaseCombatCharacter * >(pPlayer);
					pZombie->m_bConqSelected = true;
					//add to selected zombies list
					gEntList.m_ZombieSelected.AddToTail(pZombie);
					selected_units = true;
				}
			}

		}
	} //end while

	//TGB: update selected count
	pPlayer->m_iZombieSelected = gEntList.m_ZombieSelected.Count();

	//if ( sticky || selected_units )
	//if (sticky || selected_units || zm_stickyselect.GetBool() )
	//	return;

	//we didn't select anything, so we clicked on the ground or something
	//TGB: don't have to deselect anything as we already did that beforehand
	
}
static ConCommand zm_zoneselect_cc("zm_zoneselect_cc", CC_ZombieMaster_ZoneSelect, "Zone selection clientcommand");

void CC_ZombieMaster_TypeSelect ( void )
{
	CAI_BaseNPC *pSelector = NULL;
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );   //find a pointer to the player that the client controls
	if (!pPlayer || !pPlayer->IsZM() ) return;

	//get mouse coords
	const int mousex = atoi(engine->Cmd_Argv(1) );
	const int mousey = atoi(engine->Cmd_Argv(2) );

	//get world->screen matrix
	VMatrix worldToScreen(
		atof(engine->Cmd_Argv(3)), atof(engine->Cmd_Argv(4)), atof(engine->Cmd_Argv(5)), atof(engine->Cmd_Argv(6)),
		atof(engine->Cmd_Argv(7)), atof(engine->Cmd_Argv(8)), atof(engine->Cmd_Argv(9)), atof(engine->Cmd_Argv(10)),
		0, 0, 0, 0,
		atof(engine->Cmd_Argv(11)), atof(engine->Cmd_Argv(12)), atof(engine->Cmd_Argv(13)), atof(engine->Cmd_Argv(14))
		);

	const int scrWidth = atoi(engine->Cmd_Argv(15));
	const int scrHeight= atoi(engine->Cmd_Argv(16));

	//TGB: resolution adjustment factor, assuming an 800x600 res as the one the constants were defined for
	//		seeing as I often run it 800x600 windowed while testing
	const float scrScaleFactor = (float)scrHeight / 600.0;

	//TGB: get sticky select setting of this command
	const bool sticky = ( atoi(engine->Cmd_Argv(17)) ) ? true : false; //explicit int->bool conversion

	char typeToSelect[30] = "None";
	//FIND TYPE TO SELECT

	//loop through list of zombies and check if they're in the selection region
	for ( int i = 0; i < gEntList.m_ZombieList.Count(); i++)
	{

		//make sure we don't already have a type
		if ( !(Q_strcmp("None", typeToSelect) == 0) )
			break;

		pSelector = dynamic_cast< CAI_BaseNPC * >(gEntList.m_ZombieList[i]);
		if (pSelector)
		{
			Vector vNPCpos = pSelector->GetAbsOrigin();
			Vector vNPCscreen;
			ZM_ScreenTransform(vNPCpos, vNPCscreen, worldToScreen);
			
			//attempt at converting the normalized coords to useful stuff
			int iX =  0.5 * vNPCscreen[0] * scrWidth;
			int iY = -0.5 * vNPCscreen[1] * scrHeight;
			iX += 0.5 * scrWidth;
			iY += 0.5 *	scrHeight;


			//get world locations for npc and player
			Vector vDistance;
			Vector vZMpos = pPlayer->GetAbsOrigin();
			VectorSubtract( vNPCpos, vZMpos, vDistance );

			//clunky but as good as any I've come up with
			float fDistFactor = 1 - (vDistance.Length() / 1000); //1000 = max distance for min regionsize
			if (fDistFactor < 0.03) 
				fDistFactor = 0.03;

			//compare coordinates
			//should perhaps scale region part to game's resolution somehow
			//int region = ZM_SELECTAREA_NPC * (fDistFactor * fDistFactor);
			const int region = (ZM_SELECTAREA_NPC * scrScaleFactor) * (fDistFactor * fDistFactor);
			if ( iX > mousex-region && iX < mousex+region && iY < mousey+region && iY > mousey-region)
			{
				//DevMsg(" NPC is within select-region!");
				
				CBaseCombatCharacter *pZombie = dynamic_cast< CBaseCombatCharacter * >(pSelector);

				//we've found our type
				Q_strcpy( typeToSelect, pZombie->GetClassname());
				DevMsg("Type found. C: %s, T: %s\n", pZombie->GetClassname(), typeToSelect );

				//is it selected?
				if (pZombie->m_pConqSelector != pPlayer)
				{
					//nope, select it
					pZombie->m_pConqSelector = dynamic_cast< CBaseCombatCharacter * >(pPlayer);
					pZombie->m_bConqSelected = true;
					//add to selected zombies list
					gEntList.m_ZombieSelected.AddToTail(pZombie);
					//return; //selected something
				
					break;
				}
			}
			
		}
	} //end typefind while

	if ( Q_strcmp("None", typeToSelect) == 0 )
	{
		Msg("No type found\n");
		return;
	}

	bool selected_units = false;

	//SELECT ZOMBIES OF TYPE
	for ( int i = 0; i < gEntList.m_ZombieList.Count(); i++)
	{
		pSelector = dynamic_cast< CAI_BaseNPC * >(gEntList.m_ZombieList[i]);
		if (pSelector)
		{
			Vector vNPCpos = pSelector->GetAbsOrigin();
			Vector vNPCscreen;
			ZM_ScreenTransform(vNPCpos, vNPCscreen, worldToScreen);
			
			//attempt at converting the normalized coords to useful stuff
			int iX =  0.5 * vNPCscreen[0] * scrWidth;
			int iY = -0.5 * vNPCscreen[1] * scrHeight;
			iX += 0.5 * scrWidth;
			iY += 0.5 *	scrHeight;

			//select zombie if on screen and of correct type
			if ( iX > 0 && iX < scrWidth && iY < scrHeight && iY > 0 && Q_strcmp(pSelector->GetClassname(), typeToSelect) == 0)
			{
				CBaseCombatCharacter *pZombie = dynamic_cast< CBaseCombatCharacter * >(pSelector);
				//is it selected?
				if (pZombie->m_pConqSelector != pPlayer)
				{
					//nope, select it
					pZombie->m_pConqSelector = dynamic_cast< CBaseCombatCharacter * >(pPlayer);
					pZombie->m_bConqSelected = true;
					//add to selected zombies list
					gEntList.m_ZombieSelected.AddToTail(pZombie);
					//return; //selected something
					selected_units = true;
				}
			}
			
		}
	} //end type select while

	//TGB: update selected count
	pPlayer->m_iZombieSelected = gEntList.m_ZombieSelected.Count();

	if (selected_units || sticky)
		return;

	//loop through (selected) zombies and deselect them
	for ( int i = 0; i < gEntList.m_ZombieSelected.Count(); i++)
	{
		//DevMsg("\nDeselecting zombie.");
		CBaseCombatCharacter *pZombie = dynamic_cast< CBaseCombatCharacter * >(gEntList.m_ZombieSelected[i]);
		pZombie->m_bConqSelected = false;
		pZombie->m_pConqSelector = NULL;
    }
	//purge selected list
	gEntList.m_ZombieSelected.Purge();
	
}
static ConCommand zm_typeselect_cc("zm_typeselect_cc", CC_ZombieMaster_TypeSelect, "Type selection clientcommand");

//--------------------------------------------------------------
// TGB: select all zombies we have
//--------------------------------------------------------------
void CC_ZombieMaster_SelectAll ( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );   //find a pointer to the player that the client controls
	if (!pPlayer || pPlayer->GetTeamNumber() != 3) return;

	//SELECT ZOMBIES
	for ( int i = 0; i < gEntList.m_ZombieList.Count(); i++)
	{
		CBaseCombatCharacter *pZombie = dynamic_cast< CBaseCombatCharacter * >(gEntList.m_ZombieList[i]);
		//is it selected?
		if (pZombie && pZombie->m_pConqSelector != pPlayer)
		{
			//nope, select it
			pZombie->m_pConqSelector = dynamic_cast< CBaseCombatCharacter * >(pPlayer);
			pZombie->m_bConqSelected = true;
			//add to selected zombies list
			gEntList.m_ZombieSelected.AddToTail(pZombie);
		}
	}

	//TGB: update selected count
	pPlayer->m_iZombieSelected = gEntList.m_ZombieSelected.Count();
	
}
static ConCommand zm_select_all("zm_select_all", CC_ZombieMaster_SelectAll, "Select all zombies");



//------------------------------------------------------------------------------
// TGB:  Interact with object
//------------------------------------------------------------------------------
void ZM_NPC_Target_Object( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );   //find a pointer to the player that the client controls
	if (!pPlayer || pPlayer->GetTeamNumber() != 3)
		return;

	CAI_BaseNPC *pSelector = NULL;

	//LAWYER:  Build a coordinate based vector here
	Vector forward;
	Vector tracetarget;
	//VectorNormalize( tracetarget );

	tracetarget.x = atof( engine->Cmd_Argv(1) );
	tracetarget.y = atof( engine->Cmd_Argv(2) );
	tracetarget.z = atof( engine->Cmd_Argv(3) );

	const int entityIndex = atoi(engine->Cmd_Argv(4) ); //LAWYER:  Collect the entity's index

	CBaseEntity *pEntity = UTIL_EntityByIndex( entityIndex );
	if (pEntity)
	{
		//figure out what kind of stuff we targeted
		//if it's a physobj we order swatters to go and swat, others to move
		IPhysicsObject *physobj = pEntity->VPhysicsGetObject();
		if ( physobj && physobj->IsAsleep() && physobj->IsMoveable() )
		{
			//it's a physobj
			//DevMsg("Targeted a physobj!\n");
			//set the swatters to SCHED_ZOMBIE_SWATITEM
			for ( int i = 0; i < gEntList.m_ZombieSelected.Count(); i++)
			{	
				CNPC_BaseZombie *pZombie = dynamic_cast< CNPC_BaseZombie * >(gEntList.m_ZombieSelected[i]); //Check if it's a commandable character
				if (pZombie && pZombie->CanSwatPhysicsObjects())
				{
					//DevMsg("Setting swat schedule");
					//pZombie->SetSchedule(SCHED_ZOMBIE_SWATITEM);
					pZombie->ZM_ForceSwat(pEntity);

				}
				else if (pZombie)
				{
					CAI_BaseNPC::ConqCommanded(tracetarget, forward, true, pZombie);
				}
			}
			//all selected zombies have been commanded
			return;
		}
		//if it's a breakable we make them attack it... somehow
		else if (pEntity->GetHealth() > 0)
		{
			//it's a breakable
			//for now we'll try to just make them swat it, which is essentially attacking with added physics shove
			//the physics shove will simply not occur as this has no physobj

			for ( int i = 0; i < gEntList.m_ZombieSelected.Count(); i++)
			{	
				CNPC_BaseZombie *pZombie = dynamic_cast< CNPC_BaseZombie * >(gEntList.m_ZombieSelected[i]); //Check if it's a commandable character
				if (pZombie /*&& pZombie->CanSwatPhysicsObjects()*/)
				{
					DevMsg("Setting breakable-swat schedule");
					//pZombie->SetSchedule(SCHED_ZOMBIE_SWATITEM);
					pZombie->ZM_ForceSwat(pEntity, true);

				}
				/*else if (pZombie)
				{
					CAI_BaseNPC::ConqCommanded(tracetarget, forward, true, pZombie);
				}*/
			}
			//all selected zombies have been commanded
			return;
		}

		
	}
	
	//invalid entity, just do a move

	//new method: loop through selected zombie list
	for ( int i = 0; i < gEntList.m_ZombieSelected.Count(); i++)
	{	
		pSelector = dynamic_cast< CAI_BaseNPC * >(gEntList.m_ZombieSelected[i]); //Check if it's a commandable character
		if (pSelector)
			CAI_BaseNPC::ConqCommanded(tracetarget, forward, true, pSelector);
	}


}
static ConCommand zm_npc_target_object("zm_npc_target_object", ZM_NPC_Target_Object, "Commands an NPC to interact with an object");

//TGB: line formation command handler
void ZM_NPC_Move_Line ( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );   //find a pointer to the player that the client controls
	if (!pPlayer || pPlayer->GetTeamNumber() != 3)
		return;

	const int state = atoi( engine->Cmd_Argv(1) );
	if (state == 1)
		DevMsg("Starting/running the execution of a line formation\n");
	else if (state == 2)
		DevMsg("Finishing the execution of a line formation\n");

	Vector tracetarget, forward;
	tracetarget.x = atof( engine->Cmd_Argv(2) );
	tracetarget.y = atof( engine->Cmd_Argv(3) );
	tracetarget.z = atof( engine->Cmd_Argv(4) );

	const int index = atoi( engine->Cmd_Argv(5) );

	DevMsg("Received coords for zombie #%i\n", index);

	if (gEntList.m_ZombieSelected.IsValidIndex( index ) == false )
	{
		Warning("Invalid index sent in zm_linecommand_cc: %i (0,%i)\n", index, gEntList.m_ZombieSelected.Count());
		return;
	}

	if (pPlayer)
	{
		CAI_BaseNPC *pSelector = dynamic_cast< CAI_BaseNPC * >(gEntList.m_ZombieSelected[index]);
		if (pSelector)
			CAI_BaseNPC::ConqCommanded(tracetarget, forward, true, pSelector);
	}
}

static ConCommand zm_linecommand_cc("zm_linecommand_cc", ZM_NPC_Move_Line, "Commands selected zombie of certain index to move to location");

//------------------------------------------------------------------------------
// LAWYER:  Coordinate based move
//------------------------------------------------------------------------------
void CC_Conq_NPC_Move_Coords( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );   //find a pointer to the player that the client controls
	if (!pPlayer || (pPlayer && pPlayer->GetTeamNumber() != 3))
	{
//		Msg("You aren't a Zombiemaster, and can't do that.\n");
		return;
	}
	//CBaseEntity *pIterated = NULL;
	CAI_BaseNPC *pSelector = NULL;
	//LAWYER:  Build a coordinate based vector here

	Vector forward;
	Vector tracetarget;
	VectorNormalize( tracetarget );

	tracetarget.x = atof( engine->Cmd_Argv(1) );
	tracetarget.y = atof( engine->Cmd_Argv(2) );
	tracetarget.z = atof( engine->Cmd_Argv(3) );

	//Now we have a traceresult, find some Entities and give them the command!
	//while ( (pIterated = gEntList.FindEntityInSphere( pIterated, pPlayer->GetAbsOrigin(), MAX_COORD_RANGE )) != NULL ) //Should probably be a smaller search area.  Large games could be squidged by this function
	//new method: loop through selected zombie list
	for ( int i = 0; i < gEntList.m_ZombieSelected.Count(); i++)
	{	//LAWYER:  We could do this by doing a FindEntityByName and going through the list of NPCs, but that's unweildy.
		pSelector = dynamic_cast< CAI_BaseNPC * >(gEntList.m_ZombieSelected[i]); //Check if it's a commandable character
			if (pSelector)
			{//We have a valid NPC!  Whoopaaahhh!
				//Now to check if it's on our team
//					if (pSelector->GetConqTeam() == pPlayer->GetConqTeam())
//					{ //It's on our team!  Check if it's selected!
//						if (pSelector->m_pConqSelector == pPlayer)
//						{
						//Okay, selected.  Now send some commands down!
						//pSelector->m_vecConqGoal = tr.endpos;
						//pSelector->m_iConqOrderType = 1; //LAWYER:  1 - the universal move order!
						//DevMsg("Order sent");

						//TGB: commented out the ConqSelector check as all the zombies in the list are selected, and by the ZM
						CAI_BaseNPC::ConqCommanded(tracetarget, forward, true, pSelector);
//						}
//					}

			}
	}
	

}
static ConCommand conq_npc_move_coords("conq_npc_move_coords", CC_Conq_NPC_Move_Coords, "Commands an NPC to move to a location specified by coordinates");

//qck: It'd be cooler to have one function to change modes, but for testing I'm leaving it basic.
void ZM_NPC_Switch_Mode_Defense( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );   //find a pointer to the player that the client controls
	if (!pPlayer || pPlayer->GetTeamNumber() != 3)
	{
//		Msg("You aren't a Zombiemaster, and can't do that.\n");
		return;
	}

	CNPC_BaseZombie *pSelector = NULL;

	if (pPlayer)
	{
		for ( int i = 0; i < gEntList.m_ZombieSelected.Count(); i++)
		{	//LAWYER:  We could do this by doing a FindEntityByName and going through the list of NPCs, but that's unweildy.
			pSelector = dynamic_cast< CNPC_BaseZombie * >(gEntList.m_ZombieSelected[i]); //Check if it's a commandable character
			if (pSelector)
			{
				//qck: Go ahead and change the mode of every zombie currently selected
				pSelector->SetZombieMode( ZOMBIE_MODE_DEFENSE );

			}
		}
	}

	//TGB: moved clientside
	//ClientPrint( pPlayer, HUD_PRINTTALK, "Selected zombies now on defense...\n" );
}

static ConCommand zm_npc_switch_mode_defense("zm_switch_to_defense", ZM_NPC_Switch_Mode_Defense, " ");

//qck: It'd be cooler to have one function to change modes, but for testing I'm leaving it basic.
void ZM_NPC_Switch_Mode_Offense( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );   //find a pointer to the player that the client controls
	if (!pPlayer || pPlayer->GetTeamNumber() != 3)
	{
//		Msg("You aren't a Zombiemaster, and can't do that.\n");
		return;
	}

	CNPC_BaseZombie *pSelector = NULL;

	if (pPlayer)
	{
		for ( int i = 0; i < gEntList.m_ZombieSelected.Count(); i++)
		{	//LAWYER:  We could do this by doing a FindEntityByName and going through the list of NPCs, but that's unweildy.
			pSelector = dynamic_cast< CNPC_BaseZombie * >(gEntList.m_ZombieSelected[i]); //Check if it's a commandable character
			if (pSelector)
			{
				//qck: Go ahead and change the mode of every zombie currently selected. Make them forget whatever enemies they had. 
				bool result = pSelector->SetZombieMode( ZOMBIE_MODE_OFFENSE );
			
				//TGB: if the zombie did not ignore us, continue with the other stuff, else don't interfere with whatever it's doing
				if (result)
				{
					pSelector->SetEnemy(NULL);
					pSelector->SetSchedule(SCHED_IDLE_STAND);
				}
			}
		}
	}
	//moved clientside
	//ClientPrint( pPlayer, HUD_PRINTTALK, "Selected zombies now on offense...\n" );
}

static ConCommand zm_npc_switch_mode_offense("zm_switch_to_offense", ZM_NPC_Switch_Mode_Offense, " ");


/*
//TGB: UNUSED in new selection method
void CC_Conq_NPC_Deselect( void )
{

//	Msg( "All units deselected\n" );
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );   //find a pointer to the player that the client controls
	if (pPlayer->GetTeamNumber() != 3)
	{
//		Msg("You aren't a Zombiemaster, and can't do that.\n");
		return;
	}
	//The meat of the selection tool
	CBaseEntity *pEntity = NULL;
	CBaseCombatCharacter *pSelector = NULL;
	if (pPlayer)
	{
		while ( (pEntity = gEntList.FindEntityInSphere( pEntity, pPlayer->GetAbsOrigin(), MAX_COORD_RANGE )) != NULL ) //Should probably be a smaller search area.  Large games could be squidged by this function
		{	//LAWYER:  We could do this by doing a FindEntityByName and going through the list of NPCs, but that's unweildy.
				pSelector = dynamic_cast< CAI_BaseNPC * >(pEntity); //Check if it's a commandable character
				if (pSelector)
				{//We have a valid NPC!  Whoopaaahhh!
					//Now to check if it's on our team
//					if (pSelector->GetConqTeam() == pPlayer->GetConqTeam())
//					{ //It's on our team!  Check if it's selected!
						if (pSelector->m_pConqSelector == pPlayer)
						{
							pSelector->m_bConqSelected = false; //LAWYER:  Needs fixing
							pSelector->m_pConqSelector = NULL;
						//Deselect it!
						}
//					}

				}
		}
	}



}
static ConCommand conq_npc_deselect("conq_npc_deselect", CC_Conq_NPC_Deselect, "Deselects all units");
*/
//------------------------------------------------------------------------------
// LAWYER:  Entity specific selection tool
//------------------------------------------------------------------------------
void CC_ZombieMaster_Select_Index( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );   //find a pointer to the player that the client controls
	if (!pPlayer || pPlayer->GetTeamNumber() != 3)
	{
		return;
	}

	//The meat of the selection tool
	CBaseEntity *pEntity = NULL;
	CZombieManipulate *pZombieManipulate = NULL;
	CZombieSpawn *pZombieSpawn = NULL;
	//TGB: for some reason we're not using the ambushpoint stuff it appears, so I'm commenting this out
	//CZombieAmbushPoint *pAmbushPoint = NULL;


	if (pPlayer)
	{
		int entityIndex = atoi(engine->Cmd_Argv(1) );

		pEntity = UTIL_EntityByIndex( entityIndex );
		if (pEntity)
		{
			pZombieManipulate = dynamic_cast< CZombieManipulate * >(pEntity);
			pZombieSpawn = dynamic_cast< CZombieSpawn * >(pEntity);
			//pAmbushPoint = dynamic_cast< CZombieAmbushPoint * >(pEntity);
			if (pZombieManipulate && pZombieManipulate->IsActive())
			{
				pPlayer->m_iLastSelected = entityIndex;
				pPlayer->m_iLastCost = pZombieManipulate->m_iCost;
				pPlayer->m_iLastTrapCost = pZombieManipulate->m_iTrapCost;
				Q_strncpy( pPlayer->m_szLastDescription.GetForModify(), STRING(pZombieManipulate->m_szDescription), sizeof(pPlayer->m_szLastDescription));

				//TODO: we could just send keyvalues with costs/description/etc along here in a usermessage...
				pPlayer->ShowViewPortPanel( PANEL_MANIPULATE, true ); 
				
				return;
			}
			else if (pZombieSpawn && pZombieSpawn->IsActive())
			{
				//more stuff that could be usermessage'd instead of clunky netvars
				pPlayer->m_iLastSelected = entityIndex;
				pPlayer->m_iLastZombieFlags = pZombieSpawn->m_iZombieFlags;

				DevMsg("Server has Zflags as: %i\n");

				pZombieSpawn->ShowBuildMenu(true);

				return;
			}
			//else if (pAmbushPoint)
			//{
				/*DevMsg("Hmmmmmz\n");
				pPlayer->m_iLastSelected = entityIndex;

				pPlayer->ShowViewPortPanel( PANEL_AMBUSH, true );

				return;*/
			//}
		}
	}		

}
static ConCommand zombiemaster_select_index("zombiemaster_select_index", CC_ZombieMaster_Select_Index, "Messes about with ZombieMaster specific stuff");

//TGB: spawning helpers
//TGB: made to sync with zombiemaster_specific defines
enum {
	ZOMBIE_SHAMBLER = CNPC_BaseZombie::TYPE_SHAMBLER,
	ZOMBIE_BANSHEE = CNPC_BaseZombie::TYPE_BANSHEE,
	ZOMBIE_HULK = CNPC_BaseZombie::TYPE_HULK,
	ZOMBIE_DRIFTER = CNPC_BaseZombie::TYPE_DRIFTER,
	ZOMBIE_BURNZIE = CNPC_BaseZombie::TYPE_IMMOLATOR,

	ZOMBIE_TOTALTYPES = CNPC_BaseZombie::TYPE_TOTAL
};

void SummonZombie(const char* text, CBasePlayer *pZM, int spawnidx, int amount = 1)
{
	if (!pZM)
		return;

	//find spawn globe
	CZombieSpawn *pZombieSpawn = NULL;

	//int entityIndex = pZM->m_iLastSelected; //LAWYER:  Collect the entity's index
	//TGB: now uses idx given by command
	CBaseEntity *pEntity = UTIL_EntityByIndex( spawnidx );

	if (!pEntity)
		return;

	pZombieSpawn = dynamic_cast< CZombieSpawn * >(pEntity);

	if (!pZombieSpawn)
		return;

	//determine what to spawn
	int type = CZombieSpawn::GetTypeCode(text);


	//TGB: we now only need to check on spawnflags here, 

	if (type < 0 || type >= ZOMBIE_TOTALTYPES)
	{
		Warning("Bad zombie type\n");
		return;
	}

	// try to add the zombie to the queue for this spawn
	bool result = false;
	for (int i=0; i < amount; i++)
	{
		result = pZombieSpawn->QueueUnit(type);
	}

	if (result == false)
	{
		if (amount == 1)
			ClientPrint(pZM, HUD_PRINTTALK, "Zombie could not be added to the spawn queue.");
		else
			ClientPrint(pZM, HUD_PRINTTALK, "Not all zombies could be added to the queue.");
	}
	
}

//------------------------------------------------------------------------------
// LAWYER:  Build Command
//------------------------------------------------------------------------------
void CC_Summon( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );   //find a pointer to the player that the client controls
	if (!pPlayer || pPlayer->IsZM() == false)
		return;

	/*int iNumPlayers = 0; //init player counting var
	for (int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		if (UTIL_PlayerByIndex(i))
			iNumPlayers++;
	}*/
	
	//TGB: all moved into a check in CZombieSpawn, for queueing reasons
	/*
	if ((stricmp( engine->Cmd_Argv(1), "zombie_fast" ) == 0) && 
		CZombieSpawn::OverBansheeLimit())
	{
		ClientPrint( pPlayer, HUD_PRINTCENTER, "Maximum number of banshees reached!\n" );
		return;
	}*/

	if (engine->Cmd_Argc() != 3)
	{
		Warning("Bad summon command.\n");
		return;
	}


	//TGB: spawn code heavily refactored, all happens in separate function now
	SummonZombie(V_strlower(engine->Cmd_Argv(1)), pPlayer, atoi(engine->Cmd_Argv(2)));

}
static ConCommand summon("summon", CC_Summon, "Summon a Zombie");

//TGB: quick helper for finding a zspawn
CZombieSpawn *ToZombieSpawn(const int entityindex)
{
	CBaseEntity *pEntity = UTIL_EntityByIndex( entityindex );
	CZombieSpawn *pZombieSpawn = dynamic_cast< CZombieSpawn * >(pEntity);

	return pZombieSpawn;
}

//--------------------------------------------------------------
// TGB: let a zspawn know its build menu has been closed, so it stops sending updates
//--------------------------------------------------------------
void ZM_ClosedBuildMenu(void)
{
	if (engine->Cmd_Argc() != 2)
		return;

	CZombieSpawn *pZombieSpawn = ToZombieSpawn(atoi(engine->Cmd_Argv(1) ));
	if (pZombieSpawn)
		pZombieSpawn->ShowBuildMenu(false);
}
static ConCommand buildmenu_closed("buildmenu_closed", ZM_ClosedBuildMenu);

//--------------------------------------------------------------
// TGB: pull the last zombie out of the queue for the given spawn
//--------------------------------------------------------------
void ZM_RemLastBuildMenu(void)
{
	if (engine->Cmd_Argc() != 2)
		return;

	CZombieSpawn *pZombieSpawn = ToZombieSpawn(atoi(engine->Cmd_Argv(1) ));

	if (pZombieSpawn)
		pZombieSpawn->RemoveLast();
}
static ConCommand buildmenu_remlast("buildmenu_remlast", ZM_RemLastBuildMenu);

//--------------------------------------------------------------
// TGB: clear the queue for the given spawn
//--------------------------------------------------------------
void ZM_ClearBuildMenu(void)
{
	if (engine->Cmd_Argc() != 2)
		return;

	CZombieSpawn *pZombieSpawn = ToZombieSpawn(atoi(engine->Cmd_Argv(1) ));

	if (pZombieSpawn)
		pZombieSpawn->ClearQueue();
}
static ConCommand buildmenu_clear("buildmenu_clear", ZM_ClearBuildMenu);


//------------------------------------------------------------------------------
// LAWYER:  Build Command
//------------------------------------------------------------------------------
void CC_Summongroup( void )
{
	//TGB: group summon now adds 5 to the queue
	
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );   //find a pointer to the player that the client controls
	if (!pPlayer || pPlayer->IsZM() == false)
		return;

	if (engine->Cmd_Argc() != 3)
		return;

	//TGB: spawn code heavily refactored, all happens in separate function now
	SummonZombie(V_strlower(engine->Cmd_Argv(1)), pPlayer, atoi(engine->Cmd_Argv(2)), 5);

}
static ConCommand summongroup("summongroup", CC_Summongroup, "Summon a group of Zombies");


//--------------------------------------------------------------
// TGB: fastie ceiling test
//--------------------------------------------------------------
void CC_ZM_JumpCeiling()
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );
	if (!pPlayer || pPlayer->IsZM() == false)
		return;

	if (pPlayer)
	{
		bool had_fastie = false;
		for ( int i = 0; i < gEntList.m_ZombieSelected.Count(); i++)
		{
			//nuisance: we can't include fastzombie because it's all in a cpp, so hard to typecheck
			//and call a fastie method to make it do its ceiling thing, instead we go all roundabout

			CNPC_BaseZombie *pZombie = dynamic_cast< CNPC_BaseZombie * >(gEntList.m_ZombieSelected[i]);
			if (pZombie && pZombie->GetZombieType() == CNPC_BaseZombie::TYPE_BANSHEE)
			{
				had_fastie = true;

				//pZombie->SetActivity(ACT_HOP);
				//TGB: don't bother ordering if we're already doing this
				if (pZombie->IsCurSchedule(SCHED_FASTZOMBIE_CEILING_JUMP) ||
					pZombie->IsCurSchedule(SCHED_FASTZOMBIE_CEILING_CLING))
					continue;

				pZombie->SetCondition( COND_RECEIVED_ORDERS );
				pZombie->SetSchedule( SCHED_FASTZOMBIE_CEILING_JUMP );
			}
		}

		if (had_fastie == false)
			ClientPrint(pPlayer, HUD_PRINTTALK, "No banshees selected! Only banshees can perform this action.\n");
	
	}
}
static ConCommand zm_jumpceiling("zm_jumpceiling", CC_ZM_JumpCeiling, "Jump to and cling to ceiling, banshees only");

//TGB: rally and trap placement modes were routed through the server, which is really slow and 
//	completely useless. We can just send a command from the build/manip menu to vgui_viewport.

/*
//qck: Global used by a few functions in here
bool rallymode = false; 
bool trapmode = false; 
//------------------------------------------------------------------------------
// Purpose: Toggle rally point creation mode.
//------------------------------------------------------------------------------

void CC_Toggle_Rally_Point_Mode( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );

	CSingleUserRecipientFilter filter ( pPlayer ); // set recipient
	filter.MakeReliable();  // reliable transmission

	rallymode = true; //Toggle it on or off

	UserMessageBegin( filter, "ToggleRally" ); // create message 
		WRITE_BOOL( rallymode ); // fill message
	MessageEnd(); //send message

}

static ConCommand toggle_rally("toggle_rally", CC_Toggle_Rally_Point_Mode, "Toggle rally mode on and off");

//qck: Oh the complexity...
void CC_Reset_Rally_Mode( void )
{
	rallymode = false;
}

static ConCommand reset_rallymode("reset_rallymode", CC_Reset_Rally_Mode);

//------------------------------------------------------------------------------
// Purpose: Toggle trap creation mode.
//------------------------------------------------------------------------------

void CC_Toggle_Trap_Mode( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );

	CSingleUserRecipientFilter filter ( pPlayer ); // set recipient
	filter.MakeReliable();  // reliable transmission

	trapmode = true; //Toggle it on or off

	UserMessageBegin( filter, "ToggleTrap" ); // create message 
		WRITE_BOOL( trapmode ); // fill message
	MessageEnd(); //send message

}

static ConCommand toggle_trap("toggle_trap", CC_Toggle_Trap_Mode, "Toggle trap mode on and off");

//qck: Oh the complexity...
void CC_Reset_Trap_Mode( void )
{
	trapmode = false;
}

static ConCommand reset_trapmode("reset_trapmode", CC_Reset_Trap_Mode);
*/
//------------------------------------------------------------------------------
// Purpose: Set the rally point coordinates
//------------------------------------------------------------------------------
void CC_Set_Rally_Point( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );   

	if (!pPlayer || pPlayer->GetTeamNumber() != 3)
	{
		return;
	}

	CBaseEntity *pEntity = NULL;
	CZombieSpawn *pZombieSpawn = NULL;

	Vector vecRallyCoordiantes;
	VectorNormalize( vecRallyCoordiantes );

	vecRallyCoordiantes.x = atof(engine->Cmd_Argv(1) );
	vecRallyCoordiantes.y = atof(engine->Cmd_Argv(2) );
	vecRallyCoordiantes.z = atof(engine->Cmd_Argv(3) );

	int entityIndex = pPlayer->m_iLastSelected;

	pEntity = UTIL_EntityByIndex( entityIndex );

	//qck: If the last thing we selected was a zombie spawn (it has to be) do some things.
	if (pEntity)
	{
		pZombieSpawn = dynamic_cast< CZombieSpawn * >(pEntity);
		if (pZombieSpawn && pZombieSpawn->rallyPoint)
		{
			pZombieSpawn->rallyPoint->SetCoordinates( vecRallyCoordiantes );
			pZombieSpawn->rallyPoint->ActivateRallyPoint();
			DevMsg("Rally point set at: %f %f %f \n", vecRallyCoordiantes.x, vecRallyCoordiantes.y, vecRallyCoordiantes.z);

			//TGB: if we set a rally point, we had the spawn's menu open and it closed for this, so now reopen it
			pZombieSpawn->ShowBuildMenu(true);
		}


	}
	ClientPrint( pPlayer, HUD_PRINTTALK, "New rally point set...\n" );

}

static ConCommand set_rally_point("set_rally_point", CC_Set_Rally_Point, "Set the rally point location");
//------------------------------------------------------------------------------
// Purpose: Create a Trap point
//------------------------------------------------------------------------------
void CC_Create_Trap( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );   

	if (!pPlayer || pPlayer->IsZM() == false)
	{
		return;
	}

	CBaseEntity *pEntity = NULL;
	CZombieManipulate *pZombieManipulate = NULL;
	CZombieManipulateTrigger *pTrapActual = NULL;

	Vector vecTrapCoordinates;
	//VectorNormalize( vecTrapCoordinates ); //TGB: why normalize an empty vector?

	vecTrapCoordinates.x = atof(engine->Cmd_Argv(1) );
	vecTrapCoordinates.y = atof(engine->Cmd_Argv(2) );
	vecTrapCoordinates.z = atof(engine->Cmd_Argv(3) );

	int entityIndex = pPlayer->m_iLastSelected;

	pEntity = UTIL_EntityByIndex( entityIndex );

	//qck: If the last thing we selected was a zombie spawn (it has to be) do some things.
	//LAWYER:  Actually, in this case, it has to be a Manipulate
	if (pEntity)
	{
		pZombieManipulate = dynamic_cast< CZombieManipulate * >(pEntity);
		if (pZombieManipulate)
		{
			//TGB: make sure we have not hit our trap limit for this manipulate
			if (pZombieManipulate->GetTrapcount() >= ZM_MAX_TRAPS_PER_MANIP)
			{
				//we've already hit the limit
				ClientPrint( pPlayer, HUD_PRINTTALK, "Limit of traps for this manipulate reached, can't create new trap!\n");
				return;
			}

			int trapCost = 0;
			if (pZombieManipulate->m_iTrapCost <= 0)
			{
				trapCost = pZombieManipulate->m_iCost * 1.5;
			}
			else
			{
				trapCost = pZombieManipulate->m_iTrapCost;
			}

			if (pPlayer->m_iZombiePool >= trapCost)
			{
				pPlayer->m_iZombiePool -= trapCost;

				//LAWYER: Here, we must add the stuff to create a trap.
				//REMINDER:  Cost concerns!
				//Create a trap object
				CBaseEntity *pTrapBase = (CBaseEntity *)CreateEntityByName( "info_manipulate_trigger" );

				if ( pTrapBase )
				{
					pTrapBase->SetAbsOrigin( vecTrapCoordinates );

					pTrapBase->Spawn();
					pTrapBase->Activate();
					//TGB: not sure why we do a second cast instead of going straight to Trigger above
					pTrapActual = dynamic_cast< CZombieManipulateTrigger * >(pTrapBase);
					if (pTrapActual)
					{
						//LAWYER:  Assign it to the parent Manipulate
						pTrapActual->m_pParentManipulate = pZombieManipulate;
					}
				}

				//TGB: add to count
				pZombieManipulate->AddedTrap();
				//confirm
				ClientPrint( pPlayer, HUD_PRINTTALK, "Created trap!\n");
			}
			else
			{
				ClientPrint( pPlayer, HUD_PRINTTALK, "Not enough resources!\n");
				return;
			}
		}
	}
}

static ConCommand create_trap("create_trap", CC_Create_Trap, "Creates a trap at the target location");

//------------------------------------------------------------------------------
// Purpose: Create an ambush node
//------------------------------------------------------------------------------
void ZM_CC_Create_Ambush( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );   

	if (!pPlayer || pPlayer->GetTeamNumber() != 3)
	{
		return;
	}

	if(gEntList.m_ZombieSelected.Count() == 0)
	{
		ClientPrint( pPlayer, HUD_PRINTTALK, "No zombies selected! Cannot create an ambush.\n" );
		return;
	}

	CBaseEntity *pEntity = NULL;
	CZombieAmbushPoint *pAmbushPoint = NULL;

	Vector vecTrapCoordinates;
	VectorNormalize( vecTrapCoordinates );

	vecTrapCoordinates.x = atof(engine->Cmd_Argv(1) );
	vecTrapCoordinates.y = atof(engine->Cmd_Argv(2) );
	vecTrapCoordinates.z = atof(engine->Cmd_Argv(3) );

	int entityIndex = pPlayer->m_iLastSelected;

	pEntity = UTIL_EntityByIndex( entityIndex );
	
	CBaseEntity *pTempAmbush = (CBaseEntity *)CreateEntityByName( "info_ambush_point" );

	if ( pTempAmbush  )
	{
		pAmbushPoint = dynamic_cast< CZombieAmbushPoint * >(pTempAmbush );
		if (pAmbushPoint)
		{
			//qck: For all selected zombies, give them an ambush point, and put them in ambush mode.
			for(int i=0; i <gEntList.m_ZombieSelected.Count(); i++)
			{
				CBaseEntity* pEnt = gEntList.m_ZombieSelected.Element(i);

				if (!pEnt) return;

				CNPC_BaseZombie* pAI = dynamic_cast<CNPC_BaseZombie*>(pEnt);

				if (!pAI) return;
					
				//TGB: if this zombie is a fastie who is already on the ceiling, ignore it
				if (pAI->IsCurSchedule(SCHED_FASTZOMBIE_CEILING_CLING) || 
					pAI->IsCurSchedule(SCHED_FASTZOMBIE_CEILING_JUMP))
					continue;
				

				if(pAI->HasAnAmbushPoint())
				{
					//TGB: unregister from existing ambush
					if (pAI->m_pAmbushPoint)
					{
						pAI->m_pAmbushPoint->RemoveFromAmbush(pAI);

						//if old ambush is now empty, it will be removed
						//as all zombies can have only 1 ambush assigned, there can be only as many
						//ambushes as zombies (1 for each max), preventing excessive spam
					}
				}

				//qck: We are in an ambush now. Tell them, and tell them where their point is.
				pAI->m_bIsInAmbush = true;
				pAI->SetZombieMode( ZOMBIE_MODE_AMBUSH );
				pAI->m_pAmbushPoint = pAmbushPoint->AssignAmbushPoint( pAI );
			}
			pTempAmbush->SetAbsOrigin( vecTrapCoordinates );
			pTempAmbush->Spawn();
			pTempAmbush->Activate();
		}
	}
	ClientPrint( pPlayer, HUD_PRINTTALK, "Created a new ambush point.\n" ); //qck: This will work for messages, but it only works on the server, which sucks.
}

static ConCommand zm_create_ambush("zm_create_ambush", ZM_CC_Create_Ambush, "Setup an ambush point");

//------------------------------------------------------------------------------
// Purpose: Get rid of some zombies!
//------------------------------------------------------------------------------
static ConVar zm_sv_gibdeleted("zm_sv_gibdeleted", "1", FCVAR_NONE, "If enabled, zombies that the ZM deletes will explode instead of ragdolling (note that this is set by the server).");
void ZM_CC_Delete_Zombies( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );   

	if (!pPlayer || pPlayer->GetTeamNumber() != 3)
	{
		return;
	}
	if(gEntList.m_ZombieSelected.Count() == 0)
	{
		Msg("No zombies selected! Cannot delete anything!\n");
		return;
	}


	//LAWYER:  Iterate through the selected zombies, kill them,
	for(int i=0; i <gEntList.m_ZombieSelected.Count(); i++)
	{
		CBaseEntity* pEnt = gEntList.m_ZombieSelected[i];
		if (!pEnt) return;

		CNPC_BaseZombie* pAI = dynamic_cast<CNPC_BaseZombie*>(pEnt);
		if (pAI && pPlayer->m_iZombiePool >= 1) //LAWYER: it's cheap to remove zombies
		{
			
			CTakeDamageInfo info;

			info.SetDamage( pAI->m_iHealth * 2 );
			info.SetAttacker( pPlayer );
			info.SetInflictor( pEnt );
			info.SetDamageForce(Vector(0, 0, -10));
			info.SetDamagePosition(pAI->GetAbsOrigin());

			if (zm_sv_gibdeleted.GetBool())
				info.SetDamageType( DMG_ALWAYSGIB );
			else
				info.SetDamageType( DMG_GENERIC );

			pAI->TakeDamage( info );

			//TGB: this is more humane, zombies were people too
			//inputdata_t temp; //not used in inputkill
			//pAI->InputKill(temp);
			//TGB: UNDONE, turns out inputkill leaves no ragdoll

			pPlayer->m_iZombiePool--;
		}

	}
	
	
}
static ConCommand zm_delete_zombies("zm_deletezombies", ZM_CC_Delete_Zombies, "Release selected Zombies");

//qck: Dismantle an ambush which has already been set up.
void CC_ZM_Dismantle_Ambush( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );

	if(!pPlayer || !pPlayer->IsZM())
	{
		return;
	}

	CBaseEntity* pEntity = NULL;
	CZombieAmbushPoint* pAmbushPoint = NULL;

	int entityIndex = atoi(engine->Cmd_Argv(1));

	pEntity = UTIL_EntityByIndex( entityIndex );
	if (!pEntity) return;

	pAmbushPoint = dynamic_cast<CZombieAmbushPoint*>(pEntity);

	if(pAmbushPoint)
	{
		pAmbushPoint->PlayerDismantleAmbush();
	}
}
static ConCommand zm_dismantle_ambush("zm_dismantle_ambush", CC_ZM_Dismantle_Ambush, "Dismantle an ambush point");


//qck: Move an ambush point which has already been set. 
void CC_ZM_Move_Ambush( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );   

	if (pPlayer->GetTeamNumber() != 3)
	{
		return;
	}

	CBaseEntity *pEntity = NULL;
	CZombieAmbushPoint *pAmbushPoint = NULL;

	Vector argsPosition;
	VectorNormalize( argsPosition );

	argsPosition.x = atof(engine->Cmd_Argv(1));
	argsPosition.y = atof(engine->Cmd_Argv(2));
	argsPosition.z = atof(engine->Cmd_Argv(3));

	int entityIndex = atoi(engine->Cmd_Argv(4));


	pEntity = UTIL_EntityByIndex( entityIndex );
	if (!pEntity) return;
	pAmbushPoint = dynamic_cast<CZombieAmbushPoint*>(pEntity);

	if(pAmbushPoint)
	{
		pAmbushPoint->PlayerMoveAmbush( argsPosition );
	}
}
static ConCommand zm_move_ambush("zm_move_ambush", CC_ZM_Move_Ambush, "Move an existing ambush point");


//------------------------------------------------------------------------------
// Purpose: Create an NPC of the given type
//------------------------------------------------------------------------------
void CC_NPC_Create_Loc( void )
{
	MDLCACHE_CRITICAL_SECTION();

	bool allowPrecache = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache( true );

	// Try to create entity
	CAI_BaseNPC *baseNPC = dynamic_cast< CAI_BaseNPC * >( CreateEntityByName(engine->Cmd_Argv(1)) );
	if (baseNPC)
	{
		baseNPC->KeyValue( "additionalequipment", npc_create_equipment.GetString() );
		baseNPC->Precache();
		DispatchSpawn(baseNPC);
		// Now attempt to drop into the world
		CBasePlayer* pPlayer = UTIL_GetCommandClient();
		trace_t tr;
		Vector forward;
		pPlayer->EyeVectors( &forward );
		AI_TraceLine(pPlayer->EyePosition(),
			pPlayer->EyePosition() + forward * MAX_TRACE_LENGTH,MASK_NPCSOLID, 
			pPlayer, COLLISION_GROUP_NONE, &tr );
		if ( tr.fraction != 1.0)
		{
			if (baseNPC->CapabilitiesGet() & bits_CAP_MOVE_FLY)
			{
				Vector pos = tr.endpos - forward * 36;
				baseNPC->Teleport( &pos, NULL, NULL );
			}
			else
			{
				// Raise the end position a little up off the floor, place the npc and drop him down
				tr.endpos.z += 12;
				baseNPC->Teleport( &tr.endpos, NULL, NULL );
				UTIL_DropToFloor( baseNPC, MASK_NPCSOLID );
			}

			// Now check that this is a valid location for the new npc to be
			Vector	vUpBit = baseNPC->GetAbsOrigin();
			vUpBit.z += 1;

			AI_TraceHull( baseNPC->GetAbsOrigin(), vUpBit, baseNPC->GetHullMins(), baseNPC->GetHullMaxs(), 
				MASK_NPCSOLID, baseNPC, COLLISION_GROUP_NONE, &tr );
			if ( tr.startsolid || (tr.fraction < 1.0) )
			{
				baseNPC->SUB_Remove();
				DevMsg("Can't create %s.  Bad Position!\n",engine->Cmd_Argv(1));
				NDebugOverlay::Box(baseNPC->GetAbsOrigin(), baseNPC->GetHullMins(), baseNPC->GetHullMaxs(), 255, 0, 0, 0, 0);
			}
		}
		baseNPC->Activate();
	}
	CBaseEntity::SetAllowPrecache( allowPrecache );
}
static ConCommand npc_create_loc("npc_create_loc", CC_NPC_Create_Loc, "Creates an NPC of the given type where the player is looking (if the given NPC can actually stand at that location).  Note that this only works for npc classes that are already in the world.  You can not create an entity that doesn't have an instance in the level.\n\tArguments:	{npc_class_name}", FCVAR_CHEAT);


//------------------------------------------------------------------------------
// Purpose: Activates a Manipulate
//------------------------------------------------------------------------------
void CC_Manipulate( void )
{
//	Warning("Manipulating\n");

	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );   //find a pointer to the player that the client controls

	if (pPlayer->GetTeamNumber() != 3)
	{
//		Msg("You aren't a Zombiemaster, and can't do that.\n");
		return;
	}
	CBaseEntity *pEntity = NULL;
	CZombieManipulate *pZombieManipulate = NULL;
	

	if (pPlayer)
	{
//			Warning("We have a player\n");
		int entityIndex = pPlayer->m_iLastSelected; //LAWYER:  Collect the entity's index

		pEntity = UTIL_EntityByIndex( entityIndex );
		if (pEntity)
		{
//				Warning("We have an entity\n");
			pZombieManipulate = dynamic_cast< CZombieManipulate * >(pEntity);
			if (pZombieManipulate)
//				Warning("We have a manipulate\n");
			{ //LAWYER: It's a manipulatable!  DO SOMETHING!
						//We should stick a gump in here, but for now, we'll skip it
//						Msg("Activated!\n");
				if (pPlayer->m_iZombiePool >= pZombieManipulate->m_iCost)
				{
					//Warning("We should be activating it\n");
						pPlayer->m_iZombiePool -= pZombieManipulate->m_iCost;
						pZombieManipulate->Trigger(pPlayer);
				}
				else
				{
					//	Warning("Not enough resources!\n");
					ClientPrint( pPlayer, HUD_PRINTTALK, "Not enough resources!\n");

				}
			}
		}
	}
}
static ConCommand manipulate("manipulate", CC_Manipulate, "Activates a Manipulate");