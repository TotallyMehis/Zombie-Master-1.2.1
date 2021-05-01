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

// Ugly dumping zone for ZM entities that ought to have their own file.

#include "cbase.h"
#include "decals.h"
//#include "explode.h"
#include "npc_BaseZombie.h"
#include "IEffects.h"
#include "vstdlib/random.h"
#include "vstdlib/strtools.h"
#include "shareddefs.h"
#include "zombiemaster_specific.h"
#include "game.h"
#include "team.h"

//TGB: for loadout
//#include "items.h"

#include "usermessages.h"

#include "hl2mp/hl2mp_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define HUMAN_WIN_SCORE 50
#define HUMAN_LOSE_SCORE 50

ConVar zm_trap_triggerrange( "zm_trap_triggerrange", "96", FCVAR_NONE, "The range trap trigger points have.");
ConVar zm_ambush_triggerrange( "zm_ambush_triggerrange", "96", FCVAR_REPLICATED, "The range ambush trigger points have.");

ConVar zm_spawndelay("zm_spawndelay", "0.75", FCVAR_NOTIFY, "Delay between creation of zombies at zombiespawn.");

ConVar zm_loadout_disable("zm_loadout_disable", "0", FCVAR_NOTIFY, "If set to 1, any info_loadout entity will not hand out weapons. Not recommended unless you're intentionally messing with game balance and playing on maps that support this move.");

extern ConVar zm_cost_shambler;
extern ConVar zm_cost_banshee;
extern ConVar zm_cost_hulk;
extern ConVar zm_cost_drifter;
extern ConVar zm_cost_immolator;

//extern int ITEM_GiveAmmo( CBasePlayer *pPlayer, float flCount, const char *pszAmmoName, bool bSuppressSound = false );

//TGB: this is cleaner than having CNPC_BaseZombie in front of everything, though still kind of ugly
enum {
	TYPE_SHAMBLER	= CNPC_BaseZombie::TYPE_SHAMBLER,
	TYPE_BANSHEE	= CNPC_BaseZombie::TYPE_BANSHEE,
	TYPE_HULK		= CNPC_BaseZombie::TYPE_HULK,
	TYPE_DRIFTER	= CNPC_BaseZombie::TYPE_DRIFTER,
	TYPE_IMMOLATOR	= CNPC_BaseZombie::TYPE_IMMOLATOR,

	TYPE_TOTAL		= CNPC_BaseZombie::TYPE_TOTAL,
	TYPE_INVALID	= CNPC_BaseZombie::TYPE_INVALID
};

const char* TypeToName[TYPE_TOTAL] = {
		"npc_zombie",
		"npc_fastzombie",
		"npc_poisonzombie",
		"npc_dragzombie",
		"npc_burnzombie"
};

#define TRAP_MODEL     "models/trap.mdl"

class CZombieMaster_HumanWin : public CPointEntity
{
public:
	DECLARE_CLASS( CZombieMaster_HumanWin, CPointEntity );

	CZombieMaster_HumanWin( void )
	{
		// Default to invalid.
		//m_sFireballSprite = -1;
	};

	void Spawn( );



	// Input handlers
	void InputHumanWin( inputdata_t &inputdata );
	void InputHumanLose( inputdata_t &inputdata );

	DECLARE_DATADESC();

/*	int m_iMagnitude;// how large is the fireball? how much damage?
	int m_iRadiusOverride;// For use when m_iMagnitude results in larger radius than designer desires.
	int m_spriteScale; // what's the exact fireball sprite scale? 
	float m_flDamageForce;	// How much damage force should we use?
	string_t m_iszFireballSprite;
	short m_sFireballSprite;
	EHANDLE m_hInflictor;*/
};

LINK_ENTITY_TO_CLASS( func_win, CZombieMaster_HumanWin );

BEGIN_DATADESC( CZombieMaster_HumanWin )

/*	DEFINE_KEYFIELD( m_iMagnitude, FIELD_INTEGER, "iMagnitude" ),
	DEFINE_KEYFIELD( m_iRadiusOverride, FIELD_INTEGER, "iRadiusOverride" ),
	DEFINE_FIELD( m_spriteScale, FIELD_INTEGER ),
	DEFINE_KEYFIELD( m_flDamageForce, FIELD_FLOAT, "DamageForce" ),
	DEFINE_FIELD( m_iszFireballSprite, FIELD_STRING ),
	DEFINE_FIELD( m_sFireballSprite, FIELD_SHORT ),
	DEFINE_FIELD( m_hInflictor, FIELD_EHANDLE ),

	// Function Pointers
	DEFINE_THINKFUNC( Smoke ),
*/
	// Inputs
	DEFINE_INPUTFUNC(FIELD_VOID, "Win", InputHumanWin),
	DEFINE_INPUTFUNC(FIELD_VOID, "Lose", InputHumanLose),

END_DATADESC()

void CZombieMaster_HumanWin::Spawn( void )
{ 
	SetSolid( SOLID_NONE );
	AddEffects( EF_NODRAW );

	SetMoveType( MOVETYPE_NONE );
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for making the round end.
//-----------------------------------------------------------------------------
void CZombieMaster_HumanWin::InputHumanWin( inputdata_t &inputdata )
{ 
//LAWYER:  We need to stick loads of bits and pieces here to make the players know they won
//Perhaps add a sound in here?
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *plr = UTIL_PlayerByIndex( i );

		if ( plr )
		{
			if (plr->GetTeamNumber() == 2) //We really should use DEFINES for the team numbers...
			{
					plr->IncrementFragCount(HUMAN_WIN_SCORE); //LAWYER:  50 points for surviving a round
					//LAWYER:  Add resources and score
					
			}
		}
	}
	/*char text[256];
	Q_snprintf( text,sizeof(text), "The living have prevailed!\n" );*/

	UTIL_ClientPrintAll( HUD_PRINTTALK, "The living have prevailed!\n" );

	HL2MPRules()->TeamVictorious(true, "objective");

	g_pGameRules->FinishingRound();
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for making the round end, but with the ZM winning
//-----------------------------------------------------------------------------
void CZombieMaster_HumanWin::InputHumanLose( inputdata_t &inputdata )
{ 

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *plr = UTIL_PlayerByIndex( i );

		if ( plr )
		{
			if (plr->GetTeamNumber() == 3) //We really should use DEFINES for the team numbers...
			{
					plr->IncrementFragCount(HUMAN_LOSE_SCORE); //LAWYER:  50 points for surviving a round
				
					break;
			}
		}
	}
	/*char text[256];
	Q_snprintf( text,sizeof(text), "The living have failed their objectives!\n" );*/

	UTIL_ClientPrintAll( HUD_PRINTTALK, "The living have failed their objectives!\n" );	

	HL2MPRules()->TeamVictorious( false, "objective" );

	g_pGameRules->FinishingRound();
}


//LAWYER:--------------------------Manipulatable Entities-----------------------------

LINK_ENTITY_TO_CLASS( info_manipulate, CZombieManipulate );
IMPLEMENT_SERVERCLASS_ST(CZombieManipulate, DT_ZombieManipulate)
	//SendPropString	(SENDINFO(m_szDescription)),
END_SEND_TABLE()

// Start of our data description for the class
BEGIN_DATADESC( CZombieManipulate )
    
     // Save/restore our active state
	DEFINE_FIELD( m_bActive, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flNextChangeTime, FIELD_TIME ),

	//Keyfields for Hammer users
	DEFINE_KEYFIELD(m_iCost, FIELD_INTEGER, "Cost"),
	DEFINE_KEYFIELD(m_iTrapCost, FIELD_INTEGER, "TrapCost"), //LAWYER:  Cost for setting as a trap

	DEFINE_KEYFIELD(m_bActive, FIELD_BOOLEAN, "Active"), //LAWYER:  A keyfield for whether this entity should be usable on round start
	DEFINE_KEYFIELD(m_bRemoveOnTrigger, FIELD_BOOLEAN, "RemoveOnTrigger"), //LAWYER:  Remove on use?
	DEFINE_KEYFIELD(m_szDescription, FIELD_STRING, "Description"), //qck: A description for the manipulate. Set in hammer, obviously. 

     // Links our input name from Hammer to our input member function
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Hide", InputHide ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Unhide", InputUnhide ),

	 //Declare our ouput
	DEFINE_OUTPUT( m_OnPressed, "OnPressed" ),



END_DATADESC()

#define ENTITY_MODEL_Z     "models/zombiespawner.mdl"
#define MANIPULATE_MODEL     "models/manipulatable.mdl"

CZombieManipulate::CZombieManipulate()
{
	//TGB: add to zombie manipulate list

	gEntList.m_ZombieManipulates.AddToTail(this);	

	/*char* defaultText = "Default text";
	if(m_szDescription != NULL_STRING)
	{
		Q_strncpy( (char*)STRING(m_szDescription), defaultText, sizeof(m_szDescription));
	}*/


}

CZombieManipulate::~CZombieManipulate()
{

	gEntList.m_ZombieManipulates.FindAndRemove(this);
	//Q_strncpy( m_szDescription, "This is a test of player string sending", sizeof(m_szDescription));

}

void CZombieManipulate::Precache( void )
{
     PrecacheModel( MANIPULATE_MODEL );
#ifndef CLIENT_DLL
	UTIL_PrecacheOther( "info_manipulate_trigger" );
#endif

}

void CZombieManipulate::Spawn( void )
{
       Precache();

       SetModel( MANIPULATE_MODEL );
	   AddSolidFlags( FSOLID_NOT_STANDABLE );
       SetSolid( SOLID_NONE );
	   //TGB: size only affects collision bounds, and this doesn't collide
	   //UTIL_SetSize( this, -Vector(20,20,20), Vector(20,20,20) );
	   SetMoveType( MOVETYPE_FLY );
		
		if ( !m_bActive )
		{
			AddSolidFlags( FSOLID_NOT_SOLID );
			AddEffects( EF_NODRAW );
		}
		else
		{
			RemoveSolidFlags( FSOLID_NOT_SOLID );
			RemoveEffects( EF_NODRAW );
		}

}

void CZombieManipulate::InputToggle( inputdata_t &inputData )
{
	// Toggle our active state
	if ( m_bActive )
	{
		m_bActive = false;
		AddSolidFlags( FSOLID_NOT_SOLID );
		AddEffects( EF_NODRAW );
	}
	else
	{
		m_bActive = true;
		RemoveSolidFlags( FSOLID_NOT_SOLID );
		RemoveEffects( EF_NODRAW );
	}
	//LAWYER:  Destroy all Traps linked to this object
	for ( int i = 0; i < gEntList.m_ZombieTraps.Count(); i++)
	{
		
		CZombieManipulateTrigger * pSelector = dynamic_cast< CZombieManipulateTrigger * >(gEntList.m_ZombieTraps[i]);
		if (pSelector)
		{
			//Cycle through all of the Traps
			if (pSelector->m_pParentManipulate == this)
			{
				UTIL_Remove(pSelector); //Pop them if they're parented
			}
		}
	}
}

void CZombieManipulate::InputHide( inputdata_t &inputData )
{
    // hide this!
	m_bActive = false;
	AddSolidFlags( FSOLID_NOT_SOLID );
	AddEffects( EF_NODRAW );

	//LAWYER:  Destroy all Traps linked to this object
	for ( int i = 0; i < gEntList.m_ZombieTraps.Count(); i++)
	{

		CZombieManipulateTrigger * pSelector = dynamic_cast< CZombieManipulateTrigger * >(gEntList.m_ZombieTraps[i]);
		if (pSelector)
		{
			//Cycle through all of the Traps
			if (pSelector->m_pParentManipulate == this)
			{
				UTIL_Remove(pSelector); //Pop them if they're parented
			}
		}
	}
}

void CZombieManipulate::InputUnhide( inputdata_t &inputData )
{
	// unhide this!
	m_bActive = true;
	RemoveSolidFlags( FSOLID_NOT_SOLID );
	RemoveEffects( EF_NODRAW );
}

void CZombieManipulate::Trigger( CBaseEntity *pActivator )
{
	//TGB: we don't want to be able to activate hidden manips
	if (m_bActive == false)
		return;

//	Msg("Pressed...\n"); //LAWYER
	m_OnPressed.FireOutput(pActivator, this);  //Fire outputs when triggered.

	//LAWYER:  Destroy all Traps linked to this object
	for ( int i = 0; i < gEntList.m_ZombieTraps.Count(); i++)
	{
		
		CZombieManipulateTrigger *pSelector = dynamic_cast< CZombieManipulateTrigger * >(gEntList.m_ZombieTraps[i]);
		if (pSelector)
		{
			//Cycle through all of the Traps
			if (pSelector->m_pParentManipulate == this)
			{
				UTIL_Remove(pSelector); //Pop them if they're parented
			}
		}
	}

	//TGB: zero the trap count seeing as we just removed them all
	m_iTrapCount = 0;

	HL2MPRules()->ManipulateTriggered(this);

	if (m_bRemoveOnTrigger == true)
	{
		//LAWYER:  Remove this entity!
		UTIL_Remove( this );
	}
}

//LAWYER:--------------------------Zombie Spawn-----------------------------

LINK_ENTITY_TO_CLASS( info_zombiespawn, CZombieSpawn );

IMPLEMENT_SERVERCLASS_ST(CZombieSpawn, DT_ZombieSpawn)
END_SEND_TABLE()

// Start of our data description for the class
BEGIN_DATADESC( CZombieSpawn )
	DEFINE_FIELD( m_flNextChangeTime, FIELD_TIME ),
	DEFINE_KEYFIELD(m_bActive, FIELD_BOOLEAN, "Active"), //LAWYER:  A keyfield for whether this entity should be usable on round start
	
	DEFINE_KEYFIELD(m_iZombieFlags, FIELD_INTEGER, "zombieflags"), //FIXME: LAWYER:  A workaround for the Zombie Flags problem.  

	DEFINE_KEYFIELD( rallyName,	FIELD_STRING, "rallyname" ),

	DEFINE_KEYFIELD( nodeName,	FIELD_STRING, "nodename" ),
     // Links our input name from Hammer to our input member function
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Hide", InputHide ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Unhide", InputUnhide ),

	DEFINE_THINKFUNC( SpawnThink ),
END_DATADESC()

CZombieSpawn::CZombieSpawn()
{
	//TGB: add to zombie manipulate list

	gEntList.m_ZombieSpawns.AddToTail(this);	

	spawn_queue.Purge();
	spawn_queue.EnsureCapacity(queue_size);

	m_bShowingMenu = false;
}

CZombieSpawn::~CZombieSpawn()
{
	gEntList.m_ZombieSpawns.FindAndRemove(this);

	//our UtlVectors will Purge() themselves in their destructor
}


void CZombieSpawn::Precache( void )
{
     PrecacheModel( ENTITY_MODEL_Z );
	
}

void CZombieSpawn::Spawn( void )
{
	Precache();

	SetModel( ENTITY_MODEL_Z );
	SetSolid( SOLID_NONE );
	//TGB: size only affects collision bounds, and this doesn't collide
	//UTIL_SetSize( this, -Vector(20,20,20), Vector(20,20,20) );
	SetMoveType( MOVETYPE_FLY );

	AddSolidFlags( FSOLID_NOT_STANDABLE );

	if ( !m_bActive )
	{
		AddSolidFlags( FSOLID_NOT_SOLID );
		AddEffects( EF_NODRAW );
	}
	else
	{
		RemoveSolidFlags( FSOLID_NOT_SOLID );
		RemoveEffects( EF_NODRAW );
	}

	 //rallyPoint = dynamic_cast<CZombieRallyPoint *>(CreateEntityByName("info_rallypoint" ));
	 //rallyPoint->SetOwnerEntity(this);
	 //rallyPoint->Spawn();
	 //rallyPoint->SetSpawnParent( entindex() );

	//TGB: see if we have a map-set rallypoint
	CBaseEntity *pRallyEnt = gEntList.FindEntityByName( NULL, rallyName, this );
	rallyPoint = dynamic_cast<CZombieRallyPoint *>(pRallyEnt);
	if (pRallyEnt && rallyPoint)
	{
		DevMsg("ZSpawn: Map-set rally!\n");
		rallyPoint->SetOwnerEntity(this);
		rallyPoint->SetSpawnParent( entindex() );
		rallyPoint->ActivateRallyPoint();
	}
	else
	{
		DevMsg("ZSpawn: No map-set rally!\n");
		//create a dummy
		rallyPoint = dynamic_cast<CZombieRallyPoint *>(CreateEntityByName("info_rallypoint" ));
		rallyPoint->SetOwnerEntity(this);
		rallyPoint->Spawn();
		rallyPoint->SetSpawnParent( entindex() );
	}

	//LAWYER:  Spawn nodes!  Shamelessly ripped off of TGB
	
//	DevMsg("ZSP: I'm a spawn and nodename is %s\n", nodeName);

	nodePoints.Purge();

	//TGB: previously, we built a list of spawn nodes here, but at this phase of post-mapload 
	//  not all of them would have spawned yet sometimes, it seems, so now we do this the first time
	//  we actually need a spawn location

	m_bDidSpawnSetup = false; //clunky

	SetThink(&CZombieSpawn::SpawnThink);

	m_bSpawning = false;
}

void CZombieSpawn::InputToggle( inputdata_t &inputData )
{
	// Toggle our active state
	if ( m_bActive )
	{
		m_bActive = false;
		AddSolidFlags( FSOLID_NOT_SOLID );
		AddEffects( EF_NODRAW );
	}
	else
	{
		m_bActive = true;
		RemoveSolidFlags( FSOLID_NOT_SOLID );
		RemoveEffects( EF_NODRAW );
	}

}

void CZombieSpawn::InputHide( inputdata_t &inputData )
{
		m_bActive = false;
		AddSolidFlags( FSOLID_NOT_SOLID );
		AddEffects( EF_NODRAW );

}

void CZombieSpawn::InputUnhide( inputdata_t &inputData )
{
		m_bActive = true;
		RemoveSolidFlags( FSOLID_NOT_SOLID );
		RemoveEffects( EF_NODRAW );
}
void CZombieSpawn::Trigger( int iEntIndex )
{
	//Msg("Pressed...\n"); //LAWYER:  We need to feed into this some actual, useful information.
	//m_OnPressed.FireOutput(pActivator, this);
}

//--------------------------------------------------------------
// TGB: spawns a zombie from this zombiespawn, WILL DEDUCT RESOURCES
//--------------------------------------------------------------
bool CZombieSpawn::CreateUnit( int type )
{
	if (m_bActive == false) //LAWYER:  No spawning from inactive spots
		return false;

	CBasePlayer *pZM = CBasePlayer::GetZM();
	if (!pZM)
		return false; //makes no sense for zombiespawns to spawn things without a zm anyway

	ZombieCost cost = GetCostForType(type);

	if (pZM->m_iZombiePool - cost.resources < 0)
	{
		ClientPrint( pZM, HUD_PRINTTALK, "Failed to spawn zombie: not enough resources!\n");
		return false;
	}

	if ((pZM->m_iZombiePopCount + cost.population) > zm_zombiemax.GetInt())
	{
		ClientPrint( pZM, HUD_PRINTCENTER, "Failed to spawn zombie: population limit reached!\n" );
		return false;
	}

	if (type == TYPE_BANSHEE && OverBansheeLimit())
	{
		ClientPrint( pZM, HUD_PRINTCENTER, "Failed to spawn zombie: maximum number of banshees reached!\n" );
		return false;
	}

	Vector vSpawnPoint = FindValidSpawnPoint();

	//point will be 0,0,0 if no spot was found, which means we should delay spawning
	if (vSpawnPoint.IsZero())
		return false;
	

	CNPC_BaseZombie *pZombie = CZombieSpawn::SpawnZombie(TypeToName[type], vSpawnPoint, GetAbsAngles());

	pZM->ZM_RecalcPop();

	//DevMsg("Attempted to create exp_ent\n");
	if (pZombie)
	{
		//finish up with specific stuff
		pZombie->SetOwnerEntity( this );
		pZombie->SetZombieSpawnID(entindex());

		//TGB: success, deduct sauce
		pZM->m_iZombiePool -= cost.resources;

		return true;
	}

	return false;
}

//--------------------------------------------------------------
// TGB: general static method for a proper zombie spawn
//--------------------------------------------------------------
CNPC_BaseZombie* CZombieSpawn::SpawnZombie(const char* entname, Vector origin, QAngle angles)
{
	ZombieCost cost = CZombieSpawn::GetCostForType(CZombieSpawn::GetTypeCode(entname));

	//check whether the zombie fits within the limit
	//have to do this check here as well for non-traditional spawning like hidden spawns, as they use this function
	CBasePlayer *pZM = CBasePlayer::GetZM();
	if (!pZM || (pZM->m_iZombiePopCount + cost.population) > zm_zombiemax.GetInt())
	{
		ClientPrint( pZM, HUD_PRINTCENTER, "Failed to spawn zombie: population limit reached!\n" );
		return NULL;
	}

	// go ahead with spawn
	CNPC_BaseZombie *pZombie = (CNPC_BaseZombie *)CreateEntityByName(entname);

	if (pZombie)
	{
		pZombie->SetAbsOrigin( origin );

		// Strip pitch and roll from the spawner's angles. Pass only yaw to the spawned NPC.
		angles.x = 0.0;
		angles.z = 0.0;
		pZombie->SetAbsAngles( angles );

		pZombie->AddSpawnFlags( SF_NPC_FADE_CORPSE );

		int spawned = DispatchSpawn( pZombie );

		if (spawned != 0 || pZombie == NULL) //Spawn() can theoretically kill the ent
			return NULL;

		pZombie->Activate();	

		return pZombie;
	}
	
	return NULL;
}

Vector CZombieSpawn::FindValidSpawnPoint(void)
{
	//TGB: before we begin, see if we set our nodes up yet
	if (m_bDidSpawnSetup == false)
	{
		nodePoints.Purge();

		CBaseEntity *pNodeEnt = gEntList.FindEntityByName( NULL, nodeName, this );
		if (pNodeEnt)
		{
			CZombieSpawnNode *node = dynamic_cast<CZombieSpawnNode *>(pNodeEnt);
			while (node != NULL)
			{
				nodePoints.AddToTail(node);

				node = node->nodePoint;
			}
		}

		m_bDidSpawnSetup = true;
		DevMsg("ZSpawn: %i spawnnodes found\n", nodePoints.Count());
	}

	Vector vForward;
	Vector vRight;
	Vector vUp;
	AngleVectors( GetAbsAngles(), &vForward, &vRight, &vUp );

	//LAWYER:  Check the spawn spot
	trace_t tr;
	Vector vSpawnPoint(0,0,0);
	
	//CZombieSpawnNode*	iterated;
	//LAWYER:  Added lovin' for the nodes
	//TGB: reworked for random node selection
	

	CUtlVector<CZombieSpawnNode*> untried_nodes; //we haven't tried these yet
	//copy over the nodes, so we can juggle them around safely
	untried_nodes = nodePoints;
	
	//TGB: try more often if mapper went through the trouble of adding shitloads of nodes.
	const int max_attempts = max(25, untried_nodes.Count());

	// TODO: TGB: possible future avenue: spawning on nodegraph nodes as fallback, like L4D does [19-11-2008]

	for (int i = 0; i < max_attempts; i++) //LAWYER:  Check for a valid location - pumped for new spawn method
	{
		//index of the node we tried (if any) in untried_nodes (so we know which one to scrap if it's taken)
		int node_idx = -1;

		//do we still have proper nodes to try?
		if (untried_nodes.Count() > 0)
		{
			//TGB: this was a case where I didn't get lawyer-banana-logic, so just made it go away and rewrote it

			//actually we don't have to do shit here, really, now that we cached our nodes
			int idx = random->RandomInt(0, untried_nodes.Count() - 1); //random is inclusive wrt max

			if (untried_nodes[idx])
			{
				DevMsg("ZSpawn: picked node %i of %i\n", idx, nodePoints.Count());
				vSpawnPoint = untried_nodes[idx]->GetAbsOrigin();

				node_idx = idx;
			}
		}

		//LAWYER:  End of nodes
		//also fallback if untried_nodes[idx] == NULL
		if (node_idx == -1)
		{
				//This will randomly generate spawn points in the local area.
				int xDeviation = random->RandomInt( -128, 128 );
				int yDeviation = random->RandomInt( -128, 128 );

				vSpawnPoint = this->GetAbsOrigin() + (vForward * 64); //Forward of the node a little
				vSpawnPoint.x = vSpawnPoint.x + xDeviation;
				vSpawnPoint.y = vSpawnPoint.y + yDeviation;
				
		}


		UTIL_TraceHull( vSpawnPoint,
						vSpawnPoint + Vector( 0, 0, 1 ),
						NAI_Hull::Mins(HULL_HUMAN),
						NAI_Hull::Maxs(HULL_HUMAN),
						MASK_NPCSOLID,
						NULL,
						COLLISION_GROUP_NONE,
						&tr );

		if( tr.fraction != 1.0 )
		{
			//LAWYER:  The spawn is blocked!
			
			if (node_idx != -1)
				untried_nodes.Remove(node_idx);

			//TGB: zero spawnpoint, so that if this was our last attempt we won't return a bogus one
			//	instead we return something that our caller can use to detect our failure.
			vSpawnPoint.Init();
			//continue
		}
		else
		{
			break;
		}
	}

	return vSpawnPoint;
}

//--------------------------------------------------------------
// TGB: returns true if the ZM can NOT spawn more banshees 
//--------------------------------------------------------------
bool CZombieSpawn::OverBansheeLimit()
{
	//if the bansheemax is inactive, we don't have to check further
	float bmax = zm_banshee_limit.GetFloat();
	if (bmax <= 0.0f)
		return false;

	//this is a clean way of getting the number of players
	CTeam *survivors = GetGlobalTeam( 2 ); //ugly magic number
	if (!survivors) 
		return false;

	const int playercount = survivors->GetNumPlayers();

	const int limit = ceil(bmax * playercount);
	const int current = gEntList.m_BansheeList.Count();

	//if new count is higher than the limit, disallow
	if (limit < (current + 1))
		return true;
	else
		return false;
}

// Spawn the next zombie from the head of the queue
void CZombieSpawn::SpawnThink()
{
	if (spawn_queue.Count() <= 0)
	{
		m_bSpawning = false;
		return; //stop thinking and spawning
	}

	// if there are still units in the queue, we'll always want another think
	SetNextThink(gpGlobals->curtime + zm_spawndelay.GetFloat());

	int current_type = spawn_queue.Element(0); //head of the queue
	
	// check if we can spawn this unit now, or if we should delay
	if (current_type == TYPE_BANSHEE && OverBansheeLimit())
		return; //over the limit right now, maybe next think one will have died

	// same thing for any zombie type and the popcount
	ZombieCost cost = CZombieSpawn::GetCostForType(current_type);
	CBasePlayer *pZM = CBasePlayer::GetZM();

	if (!pZM) return;

	if ((pZM->m_iZombiePopCount + cost.population) > zm_zombiemax.GetInt() ||
		pZM->m_iZombiePool - cost.resources < 0)
	{
		//TGB: in this case, jitter the think time a bit to avoid potential simultaneous spawning when room frees up
		SetNextThink(gpGlobals->curtime + zm_spawndelay.GetFloat() + random->RandomFloat(0.1f, 0.2f));
		return;
	}


	// if we get here, we have room for this zombie
	CreateUnit( current_type );

	// remove it from the top of the queue
	spawn_queue.Remove(0);

	UpdateBuildMenu(false);
}

//--------------------------------------------------------------
// TGB: add a zombie to the queue 
//--------------------------------------------------------------
bool CZombieSpawn::QueueUnit( int type )
{
	if (spawn_queue.Count() >= queue_size)
		return false; //queue full

	if (CanSpawn(type) == false)
		return false;

	spawn_queue.AddToTail(type); //insert adds to the tail of the queue

	if (!m_bSpawning)
		StartSpawning();

	UpdateBuildMenu(false);
	return true;
}

//--------------------------------------------------------------
// TGB: start spawning zombies from the queue, which will end automatically when it's empty 
//--------------------------------------------------------------
void CZombieSpawn::StartSpawning()
{
	SetNextThink(gpGlobals->curtime + zm_spawndelay.GetFloat());
	m_bSpawning = true;
}


//Are we allowed to spawn a zombietype at a spawn?
bool CZombieSpawn::CanSpawn(int type)
{
	if  (m_iZombieFlags == 0)
	{
		return true;
	}
	else
	{
		//TGB: set up list of what we can spawn

		bool allowed[TYPE_TOTAL];
		for (int i = 0; i < TYPE_TOTAL; i++)
			allowed[i] = false;


		int iCalculation = m_iZombieFlags;

		//Burnzombies
		if (iCalculation - 16 >= 0)
		{
			iCalculation -= 16;
			allowed[TYPE_IMMOLATOR] = true;
		}
		//Dragzombies
		if (iCalculation - 8 >= 0)
		{
			iCalculation -= 8;
			allowed[TYPE_DRIFTER] = true;
		}
		//Hulks
		if (iCalculation - 4 >= 0)
		{
			iCalculation -= 4;
			allowed[TYPE_HULK] = true;
		}
		//Fasties
		if (iCalculation - 2 >= 0)
		{
			iCalculation -= 2;
			allowed[TYPE_BANSHEE] = true;
		}
		//Shamblies
		if (iCalculation - 1 >= 0)
		{
			iCalculation -= 1;
			allowed[TYPE_SHAMBLER] = true;
		}

		return allowed[type];

	}

	return false;
}

// Grabs the pop and res costs for a given type
ZombieCost CZombieSpawn::GetCostForType(int type)
{
	switch(type)
	{
	case TYPE_SHAMBLER:
		return ZombieCost(zm_cost_shambler.GetInt(), zm_popcost_shambler.GetInt());
	case TYPE_BANSHEE:
		return ZombieCost(zm_cost_banshee.GetInt(), zm_popcost_banshee.GetInt());
	case TYPE_HULK:
		return ZombieCost(zm_cost_hulk.GetInt(), zm_popcost_hulk.GetInt());
	case TYPE_DRIFTER:
		return ZombieCost(zm_cost_drifter.GetInt(), zm_popcost_drifter.GetInt());
	case TYPE_IMMOLATOR:
		return ZombieCost(zm_cost_immolator.GetInt(), zm_popcost_immolator.GetInt());
	default:
		return ZombieCost(0, 0);
	}
}


//--------------------------------------------------------------
// TGB: helper for finding the type of an ent 
//--------------------------------------------------------------
int CZombieSpawn::GetTypeCode( const char* entname )
{
	if ( V_strcmp(entname, "npc_zombie") == 0  )
		return TYPE_SHAMBLER;
	else if ( V_strcmp(entname, "npc_fastzombie") == 0 )
		return TYPE_BANSHEE;
	else if ( V_strcmp(entname, "npc_poisonzombie") == 0 )
		return TYPE_HULK;
	else if ( V_strcmp(entname, "npc_dragzombie") == 0 )
		return TYPE_DRIFTER;
	else if ( V_strcmp(entname, "npc_burnzombie") == 0 )
		return TYPE_IMMOLATOR;

	return TYPE_INVALID;
}

//--------------------------------------------------------------
// TGB: open the buildmenu for this spawn 
//--------------------------------------------------------------
void CZombieSpawn::ShowBuildMenu( bool state )
{
	m_bShowingMenu = state;

	if (state)
		UpdateBuildMenu(true); //true = always open menu if it was not open
}

//--------------------------------------------------------------
// TGB: send usermessage to spawn with queue info 
//--------------------------------------------------------------
void CZombieSpawn::UpdateBuildMenu( bool force_open )
{
	//despite this check, we still need to send whether we want the message to server as menu-opening
	//because in a laggy situation the client may have closed the menu without us knowing
	if (m_bShowingMenu == false)
		return;
	

	CBasePlayer *pZM = CBasePlayer::GetZM();
	if (pZM)
	{
		CSingleUserRecipientFilter filter( pZM ); // set recipient
		filter.MakeReliable();  // reliable transmission

		UserMessageBegin( filter, "BuildMenuUpdate" ); // create message 
			WRITE_SHORT( entindex() ); //TGB: write our index, simplest identifier, should be unique enough for this purpose

			WRITE_BOOL( force_open ); //true if we really need this msg to open the buildmenu

			for (int i=0; i < queue_size; i++)
			{
				//have to increment by 1 so that type_invalid fits into the unsigned byte
				if (spawn_queue.IsValidIndex(i))
				{
					WRITE_BYTE(spawn_queue[i] + 1);
				}
				else
				{
					WRITE_BYTE(TYPE_INVALID + 1);
				}
			}
			
		MessageEnd(); //send message
	}
}

//--------------------------------------------------------------
// TGB: rip the last zombie out of the queue 
//--------------------------------------------------------------
void CZombieSpawn::RemoveLast( void )
{
	CBasePlayer *pZM = CBasePlayer::GetZM();
	if (!pZM) return;

	int size = spawn_queue.Count();
	if (size > 0)
	{
		spawn_queue.Remove(size - 1);

		ClientPrint(pZM, HUD_PRINTTALK, "Removed zombie from spawn queue.\n");	

		UpdateBuildMenu(false); //non-opening update
	}
	else
		ClientPrint(pZM, HUD_PRINTTALK, "No zombie to remove from queue!\n");	
}

//--------------------------------------------------------------
// TGB: clear out the entire queu 
//--------------------------------------------------------------
void CZombieSpawn::ClearQueue( void )
{
	spawn_queue.Purge();

	UpdateBuildMenu(false); //non-opening
}

//--------------------------------------------------------
// CZombieSpawnNode
//--------------------------------------------------------

LINK_ENTITY_TO_CLASS( info_spawnnode, CZombieSpawnNode );

IMPLEMENT_SERVERCLASS_ST(CZombieSpawnNode, DT_ZombieSpawnNode)
END_SEND_TABLE()

BEGIN_DATADESC( CZombieSpawnNode )
DEFINE_KEYFIELD( nodeName,	FIELD_STRING, "nodename" ),
END_DATADESC()




CZombieSpawnNode::CZombieSpawnNode()
{
}

void CZombieSpawnNode::Precache( void )
{
     PrecacheModel( "models/spawnnode.mdl" );	
}

void CZombieSpawnNode::Spawn()
{
	Precache();

	//for testing
	SetModel( "models/spawnnode.mdl" );
	SetSolid( SOLID_NONE );
	//TGB: size only affects collision bounds, and this doesn't collide
	//UTIL_SetSize( this, -Vector(2,2,2), Vector(2,2,2) );
	SetMoveType( MOVETYPE_FLY );

	
	//LAWYER:  Spawn nodes!  Shamelessly ripped off of TGB
	
	/* TGB: future generations: watch out with GetEntityName, linux didn't like it here
#ifndef _LINUX
	DevMsg("ZSN: my name is %s and nodename is %s\n", GetEntityName(), nodeName);
#endif
	*/
	CBaseEntity *pNodeEnt = gEntList.FindEntityByName( NULL, nodeName, this );
	if (pNodeEnt)
		nodePoint = dynamic_cast<CZombieSpawnNode *>(pNodeEnt);
	else
		nodePoint = NULL;
	/*if (pNodeEnt && nodePoint)
	{
		DevMsg("ZSpawn: Spawn node!\n");
	}
	else
	{
		DevMsg("ZSpawn: No spawn node!\n");
	}*/

}



//--------------------------------------------------------
// CZombieRallyPoint
//--------------------------------------------------------

LINK_ENTITY_TO_CLASS( info_rallypoint, CZombieRallyPoint );

// Start of our data description for the class
//BEGIN_DATADESC( CZombieRallyPoint )
//	DEFINE_KEYFIELD( m_iOwnerName,	FIELD_STRING,	"ownername" ),
//END_DATADESC()

IMPLEMENT_SERVERCLASS_ST(CZombieRallyPoint, DT_ZombieRallyPoint)
END_SEND_TABLE()

//qck: A rally point is created with each Zombie Spawn. It's deactivated and given no coordinates. The only
//thing set is m_iOwner, which is equal to the index of the Zombie Spawn. 

CZombieRallyPoint::CZombieRallyPoint()
{
	//qck: Set the active flag to false by default.
	m_bActive = false;
}

void CZombieRallyPoint::Precache( void )
{
    PrecacheModel( "models/rallypoint.mdl" );
	
}

void CZombieRallyPoint::Spawn()
{
	Precache();

	//for testing
	SetModel( "models/rallypoint.mdl" );
	SetSolid( SOLID_NONE );
	//TGB: size only affects collision bounds, and this doesn't collide
	//UTIL_SetSize( this, -Vector(2,2,2), Vector(2,2,2) );
	SetMoveType( MOVETYPE_FLY );

	//TGB: init location. If we don't, map-set points won't have coords
	//user-set ones override these, so doesn't affect those
	m_vecCoordinates = GetAbsOrigin();
}

Vector CZombieRallyPoint::GetCoordinates()
{
	return m_vecCoordinates;
}

void CZombieRallyPoint::SetCoordinates( Vector vecNewRallyCoordinates )
{
	
	m_vecCoordinates = vecNewRallyCoordinates;

	SetAbsOrigin(vecNewRallyCoordinates);

}

int CZombieRallyPoint::GetSpawnParent()
{
	return m_iOwner; 
}

void CZombieRallyPoint::SetSpawnParent( int entindex )
{
	m_iOwner = entindex;
}

void CZombieRallyPoint::ActivateRallyPoint()
{
	m_bActive = true;
}

void CZombieRallyPoint::DeactivateRallyPoint()
{
	m_bActive = true;
}



//LAWYER:--------------------------Trap System Entities-----------------------------

LINK_ENTITY_TO_CLASS( info_manipulate_trigger, CZombieManipulateTrigger );
IMPLEMENT_SERVERCLASS_ST(CZombieManipulateTrigger, DT_ZombieManipulateTrigger)
//	SendPropInt( SENDINFO( m_iCost ), 8, SPROP_UNSIGNED ), //LAWYER:  Cost is networked
END_SEND_TABLE()

// Start of our data description for the class
BEGIN_DATADESC( CZombieManipulateTrigger )
 
     // Save/restore our active state
	//DEFINE_FIELD (m_pParentManipulate, FIELD_POINTER), //LAWYER:   I think this needs to be an EHandle, otherwise it's going to screw Demos up
/*	DEFINE_FIELD( m_bActive, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flNextChangeTime, FIELD_TIME ),
	//Keyfields for Hammer users
	DEFINE_KEYFIELD(m_iCost, FIELD_INTEGER, "Cost"),
	DEFINE_KEYFIELD(m_bActive, FIELD_BOOLEAN, "Active"), //LAWYER:  A keyfield for whether this entity should be usable on round start
	DEFINE_KEYFIELD(m_bRemoveOnTrigger, FIELD_BOOLEAN, "RemoveOnTrigger"), //LAWYER:  Remove on use?
     // Links our input name from Hammer to our input member function
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),
	 //Declare our ouput
	DEFINE_OUTPUT( m_OnPressed, "OnPressed" ),*/
     // Declare our think function
	 DEFINE_THINKFUNC( ScanThink ),
//     DEFINE_THINKFUNC( MoveThink ),

END_DATADESC()



CZombieManipulateTrigger::CZombieManipulateTrigger()
{
	//TGB: add to zombie manipulate list

	gEntList.m_ZombieTraps.AddToTail(this);

}

CZombieManipulateTrigger::~CZombieManipulateTrigger()
{

	gEntList.m_ZombieTraps.FindAndRemove(this);

}

void CZombieManipulateTrigger::Precache( void )
{
    PrecacheModel( TRAP_MODEL );
}

void CZombieManipulateTrigger::Spawn( void )
{
    Precache();

    SetModel( TRAP_MODEL );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
    SetSolid( SOLID_NONE );
	//TGB: size only affects collision bounds, and this doesn't collide
	//UTIL_SetSize( this, -Vector(20,20,20), Vector(20,20,20) );
	SetMoveType( MOVETYPE_FLY );

	RemoveSolidFlags( FSOLID_NOT_SOLID );
	RemoveEffects( EF_NODRAW );

	SetThink(&CZombieManipulateTrigger::ScanThink);
	SetNextThink( gpGlobals->curtime + 0.5f );
}
void CZombieManipulateTrigger::Trigger( void )
{
//	Msg("Pressed...\n"); //LAWYER
//	m_OnPressed.FireOutput(pActivator, this);  //Fire outputs when triggered.
//	if (m_bRemoveOnTrigger == true)
//	{
	if (m_pParentManipulate && m_pParentManipulate->m_bActive)
	{
		m_pParentManipulate->Trigger(this); //Can only trigger when the original thing is fired!
		m_pParentManipulate->RemovedTrap(); //adjust trap count on manip
	}

	UTIL_Remove( this ); //LAWYER:  kill Traps when they've been triggered
//	}
}

void CZombieManipulateTrigger::ScanThink(void)
{
	//LAWYER:  We need to do a scan thing
	CBaseEntity *pIterated = NULL;
	while ( (pIterated = gEntList.FindEntityInSphere( pIterated, GetAbsOrigin(), zm_trap_triggerrange.GetInt() )) != NULL ) //Should probably be a smaller search area.  Large games could be squidged by this function
	{
		//pPlayer = dynamic_cast< CBasePlayer * >(pIterated);
		CBasePlayer *pPlayer = ToBasePlayer( pIterated ); //TGB: this is safer
		if (pPlayer)
		{
			if (pPlayer->IsSurvivor())
			{
				Trigger();
				return; //TGB: no use looping on if we already triggered
			}
		}
	}

	SetNextThink( gpGlobals->curtime + 0.5f );
}




//--------------------------------------------------------
// CZombieAmbushPoint
//--------------------------------------------------------

LINK_ENTITY_TO_CLASS( info_ambush_point, CZombieAmbushPoint );

IMPLEMENT_SERVERCLASS_ST(CZombieAmbushPoint, DT_ZombieAmbushPoint)
END_SEND_TABLE()


BEGIN_DATADESC( CZombieAmbushPoint )
DEFINE_THINKFUNC( ScanThink ),
END_DATADESC()

CZombieAmbushPoint::CZombieAmbushPoint()
{
	gEntList.m_ZombieAmbushPoints.AddToTail(this);

	//safer to purge utlvectors and such before using them
	m_ZombieListeners.Purge();
}

CZombieAmbushPoint::~CZombieAmbushPoint()
{
	gEntList.m_ZombieAmbushPoints.FindAndRemove(this);
}

void CZombieAmbushPoint::Precache( void )
{
	PrecacheModel( "models/trap.mdl" );
}

void CZombieAmbushPoint::Spawn( void )
{
	m_bStartAmbush = false;

	Precache();
	DevMsg("Spawned an ambush node\n");

	SetModel( TRAP_MODEL );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetSolid( SOLID_NONE );

	//TGB: size only affects collision bounds, and this doesn't collide
	//UTIL_SetSize( this, -Vector(20,20,20), Vector(20,20,20) );

	//float scale = zm_ambush_triggerrange.GetFloat() / 34.5f;
	//SetModelWidthScale(scale);

	SetMoveType( MOVETYPE_FLY );

	SetThink(&CZombieAmbushPoint::ScanThink);
	SetNextThink( gpGlobals->curtime + 0.5f );

}

bool CZombieAmbushPoint::Ambush( void )
{	
	//qck: Start the ambush, and die.
	m_bStartAmbush = true; //TGB: this is used to kill off the entity in the next think
	
	CBasePlayer *plr = CBasePlayer::GetZM();

	if ( plr )
		ClientPrint(plr, HUD_PRINTTALK, "Ambush in progress!\n");


	for(int i=0; i<m_ZombieListeners.Count(); i++)
	{
		CNPC_BaseZombie *pEntity = m_ZombieListeners.Element(i);
		if (pEntity && pEntity->IsAlive())
		{
			pEntity->m_bSwarmAmbushPoint = true;
		}
	}


	//return m_bStartAmbush;
	return true;

}

//qck: Borrowed from manipulate trap system. 
void CZombieAmbushPoint::ScanThink(void)
{
	//qck: Die if we've been used up.
	if(m_bStartAmbush)
	{
		//DevMsg("Ambushpoint: removing myself\n");
		//TGB: zombies will have been triggered and have unregistered themselves already
		UTIL_Remove(this);
		return;
	}
	
	//qck: Just poll our listeners and see if they're alive still. If so, fine. If not, remove them.
	for(int i=0; i<m_ZombieListeners.Count(); i++)
	{
		CNPC_BaseZombie *pEntity = m_ZombieListeners.Element(i);
		if (!pEntity || pEntity->IsAlive() == false)
			m_ZombieListeners.Remove(i);
	}

	//qck: If all of our zombies are dead, vanish
	if(m_ZombieListeners.Count() == 0)
	{
		UTIL_Remove(this);
		return;
	}


	//DevMsg("ZombieListeners: %i\n", m_ZombieListeners.Count());
	CBaseEntity *pIterated = NULL;
	while ( (pIterated = gEntList.FindEntityInSphere( pIterated, GetAbsOrigin(), zm_ambush_triggerrange.GetInt() )) != NULL )
	{
		//pPlayer = dynamic_cast< CBasePlayer * >(pIterated); //Check if it's a commandable character
		CBasePlayer *pPlayer = ToBasePlayer( pIterated ); //TGB: this is safer
		if (pPlayer)
		{
			if (pPlayer->GetTeamNumber() == 2)
			{
				//TGB: a return here was preventing a new think from being scheduled, meaning we never got removed post-ambush
				Ambush();

				//TGB: give zombies some time to start moving to our position
				SetNextThink( gpGlobals->curtime + 1.0f );
			}
			
		}
	}

	SetNextThink( gpGlobals->curtime + 0.3f );
}

void CZombieAmbushPoint::PlayerMoveAmbush( Vector newPosition )
{
	SetAbsOrigin( newPosition );
}

void CZombieAmbushPoint::PlayerDismantleAmbush()
{
	//qck: Let the zombies know we aren't in an ambush anymore without triggering the ambush. 
	for(int i=0; i < m_ZombieListeners.Count(); i++)
	{
		CNPC_BaseZombie* pAI = m_ZombieListeners.Element(i);
		
		if (pAI && pAI->IsAlive())
		{
			pAI->m_bIsInAmbush = false;
			pAI->m_pAmbushPoint = NULL;
		}
	}

	UTIL_Remove(this); 
}

CZombieAmbushPoint* CZombieAmbushPoint::AssignAmbushPoint(CNPC_BaseZombie *pAI)
{
	//qck: Add AI to the ambush point member list, and return a pointer to ourself.
	m_ZombieListeners.AddToTail( pAI );
	return this;
}

void CZombieAmbushPoint::RemoveFromAmbush(CNPC_BaseZombie *pAI)
{
	m_ZombieListeners.FindAndRemove(pAI);

	if(m_ZombieListeners.Count() == 0)
		UTIL_Remove(this);
}

//----------------------------func_giveresources------------------------
class CZombieMaster_GiveResources : public CPointEntity
{
public:
	DECLARE_CLASS( CZombieMaster_GiveResources, CPointEntity );

	CZombieMaster_GiveResources( void )	{};

	void Spawn( );

	// Input handlers
	void InputGiveResources( inputdata_t &inputdata );
	

	DECLARE_DATADESC();

};

LINK_ENTITY_TO_CLASS( func_giveresources, CZombieMaster_GiveResources );

BEGIN_DATADESC( CZombieMaster_GiveResources )

	// Inputs
	DEFINE_INPUTFUNC(FIELD_INTEGER, "GiveResources", InputGiveResources),
	
END_DATADESC()

void CZombieMaster_GiveResources::Spawn( void )
{ 
	SetSolid( SOLID_NONE );
	AddEffects( EF_NODRAW );

	SetMoveType( MOVETYPE_NONE );
}


//-----------------------------------------------------------------------------
// Purpose: Give (or take, in the case of negative numbers) some resources to the ZM
//-----------------------------------------------------------------------------
void CZombieMaster_GiveResources::InputGiveResources( inputdata_t &inputdata )
{ 
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer *plr = UTIL_PlayerByIndex( i );
			if ( plr )
			{
				if (plr->IsZM())
				{
					//TGB: refactorified
					const int newpool = plr->m_iZombiePool + inputdata.value.Int();
					plr->m_iZombiePool = ( newpool < 0 ) ? 0 : newpool; //no negative resources
				}
			}
		}
}


//-----------------------------------------------------------------------------
// Purpose: Block spotcreate
//			
//-----------------------------------------------------------------------------

CTriggerBlockSpotCreate::CTriggerBlockSpotCreate()
{
	gEntList.m_ZombieSpotCreateBlocker.AddToTail(this);
}

CTriggerBlockSpotCreate::~CTriggerBlockSpotCreate()
{
	gEntList.m_ZombieSpotCreateBlocker.FindAndRemove(this);
}

BEGIN_DATADESC( CTriggerBlockSpotCreate )

	// Function Pointers
//	DEFINE_FUNCTION( CountThink ),

	// Fields
//	DEFINE_KEYFIELD( m_iPercentageToFire, FIELD_INTEGER, "percentagetofire"),
	DEFINE_KEYFIELD(m_bActive, FIELD_BOOLEAN, "Active"), 

	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),

//	DEFINE_OUTPUT( m_OnCount, "OnCount" ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( trigger_blockspotcreate, CTriggerBlockSpotCreate );


//-----------------------------------------------------------------------------
// Purpose: Called when spawning, after keyvalues have been handled.
//-----------------------------------------------------------------------------
void CTriggerBlockSpotCreate::Spawn( void )
{
	BaseClass::Spawn();

	InitTrigger();

	//SetThink( &CTriggerBlockSpotCreate::CountThink );
//	SetNextThink( gpGlobals->curtime );
}
/*
//-----------------------------------------------------------------------------
// Purpose: Counts the number of ents
//-----------------------------------------------------------------------------
void CTriggerBlockSpotCreate::CountThink( void )
{
	SetNextThink( gpGlobals->curtime + 1.0f );

//	if (m_bActive == false)
//			return;
	Vector vecMins = CollisionProp()->OBBMins();
	Vector vecMaxs = CollisionProp()->OBBMaxs();
	Warning("Mins - %f,%f,%f\n", vecMins.x, vecMins.y, vecMins.z);
	Warning("Maxs - %f,%f,%f\n", vecMaxs.x, vecMaxs.y, vecMaxs.z);

}
*/
void CTriggerBlockSpotCreate::InputToggle( inputdata_t &inputData )
{
	// Toggle our active state
	if ( m_bActive )
	{
		m_bActive = false;
	}
	else
	{
		m_bActive = true;
	}

}

void CTriggerBlockSpotCreate::InputDisable( inputdata_t &inputData )
{
		m_bActive = false;
}

void CTriggerBlockSpotCreate::InputEnable( inputdata_t &inputData )
{
		m_bActive = true;
}

//LAWYER:  info_loadout



BEGIN_DATADESC( CZombieMaster_LoadOut)

DEFINE_KEYFIELD(m_iMethod, FIELD_INTEGER, "Method"), 

//TGB: switched to an array instead of a mess of individual members
DEFINE_KEYFIELD(m_iWeaponCounts[LO_PISTOL], FIELD_INTEGER, "Pistols"), 
DEFINE_KEYFIELD(m_iWeaponCounts[LO_SHOTGUN], FIELD_INTEGER, "Shotguns"), 
DEFINE_KEYFIELD(m_iWeaponCounts[LO_RIFLE], FIELD_INTEGER, "Rifles"), 
DEFINE_KEYFIELD(m_iWeaponCounts[LO_MAC10], FIELD_INTEGER, "Mac10s"), 
DEFINE_KEYFIELD(m_iWeaponCounts[LO_MOLOTOV], FIELD_INTEGER, "Molotovs"), 
DEFINE_KEYFIELD(m_iWeaponCounts[LO_SLEDGEHAMMER], FIELD_INTEGER, "Sledgehammers"), 
DEFINE_KEYFIELD(m_iWeaponCounts[LO_IMPROVISED], FIELD_INTEGER, "Improvised"), 
DEFINE_KEYFIELD(m_iWeaponCounts[LO_REVOLVER], FIELD_INTEGER, "Revolvers"), 

END_DATADESC()

LINK_ENTITY_TO_CLASS( info_loadout, CZombieMaster_LoadOut );

void CZombieMaster_LoadOut::Spawn( void )
{ 
	SetSolid( SOLID_NONE );
	AddEffects( EF_NODRAW );

	SetMoveType( MOVETYPE_NONE );

	//TGB: figure out what we have to hand out
	FillWeaponLists();
}

//--------------------------------------------------------------
// TGB: fill our lists with the available weapons
//--------------------------------------------------------------
void CZombieMaster_LoadOut::FillWeaponLists( void )
{
	/*TGB: a fundamental difference with the old method is that we shuffle our weapons list
	* instead of our player list. This means we can just randomly pick an entry from the list, hand
	* it to the player and remove it from the list. No shuffling required
	*/

    //TGB: added option to disable this entity, requested by people for RP or difficulty-pumped servers
	if (zm_loadout_disable.GetInt() == 1)
	{
		Msg("info_loadout entities have been disabled with zm_loadout_disable\n");
		return;
	}
	
	if (m_iMethod == 1)
	{
		//CATEGORISED
		//ugh...

		for (int i = 0; i < LO_CATEGORY_COUNT; i++)
			weaponsCategorised[i].Purge();

		//have to resort to ugly churning

		//MELEE
		for (int i = 0; i < m_iWeaponCounts[LO_IMPROVISED]; i++)
			weaponsCategorised[LO_MELEE].AddToTail(LO_IMPROVISED);
		
		for (int i = 0; i < m_iWeaponCounts[LO_SLEDGEHAMMER]; i++)
			weaponsCategorised[LO_MELEE].AddToTail(LO_SLEDGEHAMMER);

		//SMALL
		for (int i = 0; i < m_iWeaponCounts[LO_PISTOL]; i++)
			weaponsCategorised[LO_SMALL].AddToTail(LO_PISTOL);

		for (int i = 0; i < m_iWeaponCounts[LO_REVOLVER]; i++)
			weaponsCategorised[LO_SMALL].AddToTail(LO_REVOLVER);

		//LARGE
		for (int i = 0; i < m_iWeaponCounts[LO_SHOTGUN]; i++)
			weaponsCategorised[LO_LARGE].AddToTail(LO_SHOTGUN);

		for (int i = 0; i < m_iWeaponCounts[LO_RIFLE]; i++)
			weaponsCategorised[LO_LARGE].AddToTail(LO_RIFLE);

		for (int i = 0; i < m_iWeaponCounts[LO_MAC10]; i++)
			weaponsCategorised[LO_LARGE].AddToTail(LO_MAC10);

		//EQUIP
		for (int i = 0; i < m_iWeaponCounts[LO_MOLOTOV]; i++)
			weaponsCategorised[LO_EQUIPMENT].AddToTail(LO_MOLOTOV);
	}
	else
	{
		//INDISCR.
		//just dump all weapons in a big ol list

		//loop through all types
		//basically array copy in this case, we don't want to do destructive things to our defaults-array
		for (int i = 0; i < LO_WEAPONS_TOTAL; i++)
		{
			m_iWeaponsAll[i] = m_iWeaponCounts[i];
		}
	}
}

//--------------------------------------------------------------
// TGB: hand weapon(s) to a given player
//--------------------------------------------------------------
void CZombieMaster_LoadOut::DistributeToPlayer( CBasePlayer *pPlayer )
{
	if (!pPlayer || zm_loadout_disable.GetInt() == 1 || pPlayer->IsSurvivor() == false)
		return;

	if (m_iMethod == 1)
	{
		//categorised

		//hand out a weapon of each type
		for (int i = 0; i < LO_CATEGORY_COUNT; i++)
		{
			//for each category, randomly pick a weapon from the list and give it to the player
			if (weaponsCategorised[i].Count() > 0)
			{
				const int pick = random->RandomInt(0, weaponsCategorised[i].Count() - 1);
				CZombieMaster_LoadOut::CreateAndGiveWeapon(pPlayer, weaponsCategorised[i].Element(pick));
				weaponsCategorised[i].Remove(pick);
			}
		}
	}
	else
	{
		//indiscriminate

		//eh, build a quick vector of types we still have
		CUtlVector<int> remaining;
		remaining.Purge();
		for (int i = 0; i < LO_WEAPONS_TOTAL; i++)
		{
			if (m_iWeaponsAll[i] > 0)
				remaining.AddToTail(i);			
		}
		

		if (remaining.Count() > 0)
		{
			//pick a random weapon type, hand it out, and remove an entry of that type from the list
			const int pick = remaining[random->RandomInt(0, remaining.Count() - 1)];
			CZombieMaster_LoadOut::CreateAndGiveWeapon(pPlayer, pick);
			m_iWeaponsAll[pick] -= 1;
		}
	}
	
}

//--------------------------------------------------------------
// TGB: distribute now simply calls DistributeToPlayer for all survivors
//--------------------------------------------------------------
void CZombieMaster_LoadOut::Distribute( void )
{
	
	//TGB: added option to disable this entity, requested by people for RP or difficulty-pumped servers
	if (zm_loadout_disable.GetInt() == 1)
	{
		Msg("info_loadout entities have been disabled with zm_loadout_disable\n");
		return;
	}

	DevMsg("Loadout of type %i\n", m_iMethod);

	CUtlVector<CBasePlayer*> playerlist;
	playerlist.Purge(); //I just always purge these to be sure

	//LAWYER:  Build an array of players that we can pop things from
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
		if (pPlayer)
		{
			if (pPlayer->IsSurvivor())
			{
				//TGB: have to do some vector hussling so that we avoid categorised handout bias
				playerlist.AddToTail(pPlayer);
			}
		}
	}

	//Fisher-Yates shuffle mm yeah, mostly ripped from wpedia
	int n = playerlist.Count();
	while (n > 1) 
	{
		int k = random->RandomInt(0, n-1);  // 0 <= k < n.
		n--;
		//swappy swaps some values, we don't move actual elements (as in memory blocks)
		CBasePlayer *temp = playerlist[n];
		playerlist[n] = playerlist[k];
		playerlist[k] = temp;
	}

	for (int i = 0; i < playerlist.Count(); i++)
	{
		//TGB: get this man a gun, good sir
		DistributeToPlayer(playerlist[i]);
	}
}

//TGB: quick helper
void CZombieMaster_LoadOut::CreateAndGiveWeapon(CBasePlayer *pPlayer, int weapon_type)
{
	if (!pPlayer) return;
    
	//braap
	const char *WeaponTypeToName[] = {
			"weapon_zm_improvised",
			"weapon_zm_sledge",
			"weapon_zm_pistol",
			"weapon_zm_shotgun",
			"weapon_zm_rifle",
			"weapon_zm_mac10",
			"weapon_zm_revolver",
			"weapon_zm_molotov"
	};

	//determine weapon name
	const char *weapon_name = WeaponTypeToName[weapon_type];

	bool add_ammo = true;

	switch(weapon_type)
	{
	case LO_IMPROVISED:
	case LO_SLEDGEHAMMER:
	case LO_MOLOTOV:
		add_ammo = false;
		break;
	default:
		add_ammo = true;
	}

	CBaseCombatWeapon *weapon = pPlayer->Weapon_Create(weapon_name);
	if (weapon)
	{
		//TGB: loadout was not updating weapon carrying flags properly 0000413
		//could do this in weapon_create or something, but that might affect other systems
		pPlayer->m_iWeaponFlags += weapon->GetWeaponTypeNumber();

		pPlayer->Weapon_Equip(weapon);

		if (add_ammo)
		{
			pPlayer->GiveAmmo(weapon->GetMaxClip1(), weapon->GetPrimaryAmmoType(), true);
		}

		DevMsg("LoadOut: gave %s to %s\n", weapon_name, pPlayer->GetPlayerName());
	}

}
