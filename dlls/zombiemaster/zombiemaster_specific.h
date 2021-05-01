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

#include "cbase.h"
#include "decals.h"
#include "Sprite.h"
#include "IEffects.h"
#include "vstdlib/random.h"
#include "vstdlib/strtools.h"
#include "shareddefs.h"
#include "ai_basenpc.h"
#include "npc_BaseZombie.h" //TGB: always check case in includes! threw a linux error
#include "triggers.h"

//#include "utlqueue.h"

#ifndef ZOMBIE_MASTER_SPECIFIC
#define ZOMBIE_MASTER_SPECIFIC

class CNPC_BaseZombie;

class CZombieManipulate : public CBaseAnimating
{
public:
	DECLARE_CLASS( CZombieManipulate, CBaseAnimating );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CZombieManipulate();	//for adding to maniplist
	~CZombieManipulate();	//for removing from list

	void Spawn( void );
	void Precache( void );
	void Trigger( CBaseEntity *pActivator );
	
	bool IsActive()				{ return m_bActive; }

	int		GetTrapcount()		{ return m_iTrapCount; }
	void	AddedTrap()			{ m_iTrapCount++; }
	void	RemovedTrap()		{ m_iTrapCount--; }

	// Input function
	void InputToggle( inputdata_t &inputData );
	void InputHide( inputdata_t &inputData );
	void InputUnhide( inputdata_t &inputData );

	//Outputs
	COutputEvent m_OnPressed;

	//Variables
	int m_iCost;
	int m_iTrapCost;

	bool		m_bActive;
	bool		m_bRemoveOnTrigger;

	

	//CNetworkString( m_szDescription, 255 );
	string_t			m_szDescription;

private:
	float        m_flNextChangeTime;

	int			m_iTrapCount;
};

//qck: An entity which is placed in the map by the player and acts as a gathering 
//area for all zombies owned by the same spawn point which owns it. 

class CZombieRallyPoint : public CBaseAnimating
{
public:
	DECLARE_CLASS( CZombieRallyPoint, CBaseAnimating );
	DECLARE_SERVERCLASS();
	//DECLARE_DATADESC();

	CZombieRallyPoint();

	void Spawn( void );
	void Precache( void );
	int GetSpawnParent();
	void SetSpawnParent( const int entindex );

	void SetCoordinates( Vector vecNewRallyCoordinates );
	Vector GetCoordinates();

	void ActivateRallyPoint();
	void DeactivateRallyPoint();

	bool IsActive() {return m_bActive;}

	//string_t m_iOwnerName; //keyfield-set name

private:
	Vector m_vecCoordinates;

	
	int m_iOwner;
	bool m_bActive; //qck: If the player sets a spawn point, this is activated. 
};


class CZombieSpawnNode : public CBaseAnimating
{
public:
	DECLARE_CLASS( CZombieSpawnNode, CBaseAnimating );
	DECLARE_SERVERCLASS();

	DECLARE_DATADESC();

	CZombieSpawnNode();

	void Spawn( void );
	void Precache( void );
//	int GetSpawnParent();
//	void SetSpawnParent( const int entindex );

//	void SetCoordinates( Vector vecNewRallyCoordinates );
//	Vector GetCoordinates();

//	void ActivateRallyPoint();
//	void DeactivateRallyPoint();

//	bool IsActive() {return m_bActive;}

	//string_t m_iOwnerName; //keyfield-set name
	
	string_t			nodeName;
	CZombieSpawnNode*	nodePoint;  //For sequential spawn points

private:
//	Vector m_vecCoordinates;

	
//	int m_iOwner;
//	bool m_bActive; //qck: If the player sets a spawn point, this is activated. 

};

struct ZombieCost {
	int resources;
	int population;

	ZombieCost(int res = 0, int pop = 0)
		:resources(res), population(pop)
	{
	}
};

// enum {
// 	TYPE_SHAMBLER = 0,
// 	TYPE_BANSHEE,
// 	TYPE_HULK,
// 	TYPE_DRIFTER,
// 	TYPE_IMMOLATOR,
// 
// 	TYPE_TOTAL,
// 	TYPE_INVALID = -1
// };


class CZombieSpawn : public CBaseAnimating
{
public:
	DECLARE_CLASS( CZombieSpawn, CBaseAnimating );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CZombieSpawn();		//for adding to spawnlist
	~CZombieSpawn();	//for removing from list

	void Spawn( void );
	void Precache( void );
	void Trigger( int iEntIndex );

	void ShowBuildMenu( bool state);
	void UpdateBuildMenu( bool force_open );

	void RemoveLast(void);
	void ClearQueue(void);

	bool CreateUnit( int type);
	bool QueueUnit( int type );
	void StartSpawning();

	bool CanSpawn( int type );

	void InputToggle( inputdata_t &inputData );
	void InputHide( inputdata_t &inputData );
	void InputUnhide( inputdata_t &inputData );

	bool IsActive(void)				{ return m_bActive; }

	void SpawnThink(void);

	Vector FindValidSpawnPoint(void);
	int m_iZombieFlags; //LAWYER:  for the Zombie Flags workaround

	string_t			rallyName;
	CZombieRallyPoint*	rallyPoint;
	
	string_t			nodeName;
	CUtlVector<CZombieSpawnNode*>	nodePoints; //TGB: vector'd, we now preload all spawnnodes

	
	bool				m_bActive;
	

	//TGB: universal-ish function for spawning zombies, use this if you need stuff like checking for popcap, does not check bansheemax
	static CNPC_BaseZombie* SpawnZombie(const char* entname, Vector origin, QAngle angles);

	//TGB: another one of these that returns whether spawning 1 banshee now would put the ZM over the limit
	static bool				OverBansheeLimit();

	static ZombieCost		GetCostForType(int type);
	
	static int				GetTypeCode(const char* entname);

private:
	bool			m_bShowingMenu;
	bool			m_bSpawning;

	bool			m_bDidSpawnSetup;

	float			m_flNextChangeTime;

	CUtlVector<int>	spawn_queue; //list of zombie types

	static const int queue_size = 10;
};



//Trap system
class CZombieManipulateTrigger : public CBaseAnimating
{
public:
	DECLARE_CLASS( CZombieManipulateTrigger, CBaseAnimating );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CZombieManipulateTrigger();	//for adding to maniplist
	~CZombieManipulateTrigger();	//for removing from list

	void Spawn( void );
	void Precache( void );
	void Trigger( void );
	void SetParentManipulate( CZombieManipulate *pParent);
	CZombieManipulate GetParentManipulate(void);
	void ScanThink( void );

	CZombieManipulate* m_pParentManipulate; //LAWYER:  This probably should be done with getters and setters, but I'm tired.
private:
	float        m_flNextChangeTime;
	
};

//qck: Entity spawned at selected location. Selected zombies stay quiet and watch it for activity. 
//It watches for players in surrounding area. It sees one, flips a bool, the zombies force march 
//to its origin, it destroys itself. 
class CZombieAmbushPoint : public CBaseAnimating
{
public:
	DECLARE_CLASS( CZombieAmbushPoint, CBaseAnimating );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CZombieAmbushPoint();	
	~CZombieAmbushPoint();	

	void Spawn( void );
	void Precache( void );
	bool Ambush( void );
	void ScanThink( void );

	void PlayerMoveAmbush( Vector newPos );
	void PlayerDismantleAmbush();
	CZombieAmbushPoint* AssignAmbushPoint( CNPC_BaseZombie* pAI ); //qck: Assign an AI an ambush point. Add AI to internal list.
	void RemoveFromAmbush(CNPC_BaseZombie* pAI); //TGB: living zombies may want to opt out sometime

	bool		 m_bStartAmbush;

	
private:
	float        m_flNextChangeTime;

	//TGB: this used BaseEntity before, and there was some type hassling of basenpc->baseentity->basezombie
	//	I don't think that's necessary as all our listeners will be zombies anyway.
	CUtlVector<CNPC_BaseZombie*> m_ZombieListeners;
};

class CTriggerBlockSpotCreate : public CBaseTrigger
{
public:


	DECLARE_CLASS( CTriggerBlockSpotCreate, CBaseTrigger );
	
	CTriggerBlockSpotCreate();	//for adding to maniplist
	~CTriggerBlockSpotCreate();	//for removing from l

	void Spawn( void );
//	void CountThink( void );	
	void InputToggle( inputdata_t &inputData );
	void InputDisable( inputdata_t &inputData );
	void InputEnable( inputdata_t &inputData );

	DECLARE_DATADESC();
	
//	int		m_iPercentageToFire; //LAWYER:  The solid count to fire this
	bool	m_bActive;
	

	COutputEvent m_OnCount;
};

enum LO_WEAPONS {
	LO_IMPROVISED = 0, 
	LO_SLEDGEHAMMER, 
	LO_PISTOL, 
	LO_SHOTGUN, 
	LO_RIFLE, 
	LO_MAC10, 
	LO_REVOLVER, 
	LO_MOLOTOV,

	LO_WEAPONS_TOTAL
};

class CZombieMaster_LoadOut : public CPointEntity
{
public:
	DECLARE_CLASS( CZombieMaster_LoadOut, CPointEntity );

	CZombieMaster_LoadOut( void )	{};

	void Spawn( );

	void Distribute();
	/*
	void HandOutIndiscriminate();
	void HandOutCategorized();
	*/

	//TGB: latejoin compat
	void FillWeaponLists();
	void DistributeToPlayer(CBasePlayer *pPlayer);

	DECLARE_DATADESC();

	static void CreateAndGiveWeapon(CBasePlayer *pPlayer, int weapon_type);

private:
	int m_iMethod;

	/*
	int m_iPistols;
	int m_iShotguns;
	int m_iRifles;
	int m_iMac10s;
	int m_iMolotovs;
	int m_iSledges;
	int m_iImprovised;
	int m_iRevolvers;
	*/
	int m_iWeaponCounts[LO_WEAPONS_TOTAL];

	//don't need a vector for this
	int m_iWeaponsAll[LO_WEAPONS_TOTAL];

	/*
	CUtlVector<CBaseEntity *>	playersConsidered;
	CUtlVector<CBaseEntity *>	playersConsideredShuffled;
	*/
	
	enum LO_CATEGORIES {
		LO_MELEE = 0,
		LO_SMALL,
		LO_LARGE,
		LO_EQUIPMENT,

		LO_CATEGORY_COUNT
	};

	CUtlVector<int>	weaponsCategorised[LO_CATEGORY_COUNT];

	/*
	CUtlVector<int>	weaponsMelee;
	CUtlVector<int>	weaponsSmall;
	CUtlVector<int>	weaponsLarge;
	CUtlVector<int>	weaponsEquipment;
	*/

};

#endif //ZOMBIE_MASTER_SPECIFIC