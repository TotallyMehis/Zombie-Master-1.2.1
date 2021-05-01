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

#ifndef NPC_BASEZOMBIE_H
#define NPC_BASEZOMBIE_H
#ifdef _WIN32
#pragma once
#endif

#include "ai_basenpc.h"
#include "ai_blended_movement.h"
#include "zombiemaster/zombiemaster_specific.h"
//#include "RallyPoint.h" 
//#include "soundenvelope.h"
#include "ai_behavior_actbusy.h"

#define ZOM_ATTN_FOOTSTEP ATTN_IDLE

//#define	ENVELOPE_CONTROLLER		(CSoundEnvelopeController::GetController())

#define ZOMBIE_MELEE_REACH	55

extern int AE_ZOMBIE_ATTACK_RIGHT;
extern int AE_ZOMBIE_ATTACK_LEFT;
extern int AE_ZOMBIE_ATTACK_BOTH;
extern int AE_ZOMBIE_SWATITEM;
extern int AE_ZOMBIE_STARTSWAT;
extern int AE_ZOMBIE_STEP_LEFT;
extern int AE_ZOMBIE_STEP_RIGHT;
extern int AE_ZOMBIE_SCUFF_LEFT;
extern int AE_ZOMBIE_SCUFF_RIGHT;
extern int AE_ZOMBIE_ATTACK_SCREAM;
extern int AE_ZOMBIE_GET_UP;
extern int AE_ZOMBIE_POUND;

extern ConVar zm_popcost_shambler;
extern ConVar zm_popcost_banshee;
extern ConVar zm_popcost_drifter;
extern ConVar zm_popcost_hulk;
extern ConVar zm_popcost_immolator;

//#define ZOMBIE_BODYGROUP_HEADCRAB	1	// The crab on our head

// Pass these to claw attack so we know where to draw the blood.
#define ZOMBIE_BLOOD_LEFT_HAND		0
#define ZOMBIE_BLOOD_RIGHT_HAND		1
#define ZOMBIE_BLOOD_BOTH_HANDS		2
#define ZOMBIE_BLOOD_BITE			3
	
/*enum HeadcrabRelease_t
{
	RELEASE_NO,
	RELEASE_IMMEDIATE,		// release the headcrab right now!
	RELEASE_SCHEDULED,		// release the headcrab through the AI schedule.
	RELEASE_VAPORIZE,		// just destroy the crab.	
	RELEASE_RAGDOLL,		// release a dead crab
	RELEASE_RAGDOLL_SLICED_OFF	// toss the crab up a bit
};
*/

//=========================================================
// schedules
//=========================================================
enum
{
	SCHED_ZOMBIE_CHASE_ENEMY = LAST_SHARED_SCHEDULE,
	SCHED_ZOMBIE_MOVE_SWATITEM,
	SCHED_ZOMBIE_SWATITEM,
	SCHED_ZOMBIE_ATTACKITEM,
//	SCHED_ZOMBIE_RELEASECRAB,
	SCHED_ZOMBIE_MOVE_TO_AMBUSH,
	SCHED_ZOMBIE_WAIT_AMBUSH,
	SCHED_ZOMBIE_WANDER_MEDIUM,	// medium range wandering behavior.
	SCHED_ZOMBIE_WANDER_SHORT,	// TGB: ZM: very(!) short range wandering behavior.
	SCHED_ZOMBIE_WANDER_FAIL,
	SCHED_ZOMBIE_WANDER_STANDOFF,
	SCHED_ZOMBIE_MELEE_ATTACK1,
	SCHED_ZOMBIE_FORCED_SWAT_GO,
	SCHED_ZOMBIE_AMBUSH_MODE, //qck: Ambush mode waiting stuff
	SCHED_ZOMBIE_DEFEND_POSITION, //qck: don't attack on sight, only when player is in very close proximity.
	SCHED_ZOMBIE_RETURN_TO_POSITION, //qck: If the player has run out on us, return to the position we were defending.

	LAST_BASE_ZOMBIE_SCHEDULE,
};

//TGB: moved over from npc_fastzombie.cpp for easier access
enum
{
	SCHED_FASTZOMBIE_RANGE_ATTACK1 = LAST_SHARED_SCHEDULE + 100, // hack to get past the base zombie's schedules
	SCHED_FASTZOMBIE_UNSTICK_JUMP,
	SCHED_FASTZOMBIE_CLIMBING_UNSTICK_JUMP,
	SCHED_FASTZOMBIE_MELEE_ATTACK1,
	SCHED_FASTZOMBIE_CEILING_JUMP, //tgb
	SCHED_FASTZOMBIE_CEILING_CLING,
};


//=========================================================
// tasks
//=========================================================
enum 
{
	TASK_ZOMBIE_DELAY_SWAT = LAST_SHARED_TASK,
	TASK_ZOMBIE_GET_PATH_TO_PHYSOBJ,
	TASK_ZOMBIE_SWAT_ITEM,
	TASK_ZOMBIE_DIE,
	TASK_ZOMBIE_DEFEND,
//	TASK_ZOMBIE_RELEASE_HEADCRAB,

	LAST_BASE_ZOMBIE_TASK,
};


//=========================================================
// Zombie conditions
//=========================================================
enum Zombie_Conds
{
	COND_ZOMBIE_CAN_SWAT_ATTACK = LAST_SHARED_CONDITION,
//	COND_ZOMBIE_RELEASECRAB,
	COND_ZOMBIE_LOCAL_MELEE_OBSTRUCTION,
	COND_ZOMBIE_DEFENDING, //qck edit
	COND_ZOMBIE_DEFENSE_MODE, //qck edit
	COND_ZOMBIE_RETURN_TO_POSITION,
	COND_ENEMY_IN_PERSONAL_SPACE, //qck edit
	COND_ZOMBIE_RETURNING, //qck edit
	COND_ZOMBIE_IN_AMBUSH,

	LAST_BASE_ZOMBIE_CONDITION,
};

//===================================================================================
//qck: Keep track of various zombie modes. Just offense, defense, and ambush for now. 
//===================================================================================
enum Zombie_Modes
{
	ZOMBIE_MODE_OFFENSE,
	ZOMBIE_MODE_DEFENSE,
	ZOMBIE_MODE_AMBUSH,
};


typedef CAI_BlendingHost< CAI_BehaviorHost<CAI_BaseNPC> > CAI_BaseZombieBase;

//=========================================================
//=========================================================
abstract_class CNPC_BaseZombie : public CAI_BaseZombieBase
{
	DECLARE_CLASS( CNPC_BaseZombie, CAI_BaseZombieBase );

public:
	CNPC_BaseZombie( void );
	~CNPC_BaseZombie( void );

	void Spawn( void );
	void Precache( void );
	float MaxYawSpeed( void );
	bool OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval );
	Class_T Classify( void );
	void HandleAnimEvent( animevent_t *pEvent );

	bool HasAnAmbushPoint(){return m_bIsInAmbush;}
	

	void OnStateChange( NPC_STATE OldState, NPC_STATE NewState );

	void KillMe( void )
	{
		m_iHealth = 5;
		OnTakeDamage( CTakeDamageInfo( this, this, m_iHealth * 2, DMG_GENERIC ) );
	}

	int MeleeAttack1Conditions ( float flDot, float flDist );
	int MeleeAttack1ConditionsVsPlayerInVehicle( CBasePlayer *pPlayer, float flDot );
	virtual float GetClawAttackRange() const { return ZOMBIE_MELEE_REACH; }
	
	//TGB: swat force function
	void ZM_ForceSwat ( CBaseEntity *pTarget, bool breakable = false );
	
	//qck: Go back to defending your turf yo
	void ReturnToDefending(const Vector &targetPos, const Vector &traceDir);

	//qck: Ambush point pointer, ambush mode toggle.
	bool m_bIsInAmbush;
	bool m_bSwarmAmbushPoint;
	CZombieAmbushPoint* m_pAmbushPoint;

	// No range attacks
	int RangeAttack1Conditions ( float flDot, float flDist ) { return( 0 ); }
	
	virtual float GetHitgroupDamageMultiplier( int iHitGroup, const CTakeDamageInfo &info );
	virtual void TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );
	int OnTakeDamage_Alive( const CTakeDamageInfo &info );
	virtual float	GetReactionDelay( CBaseEntity *pEnemy ) { return 0.0; }

	virtual int SelectSchedule ( void );
	virtual int	SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode );
	virtual void BuildScheduleTestBits( void );
	bool FValidateHintType( CAI_Hint *pHint);

	virtual int TranslateSchedule( int scheduleType );
	virtual Activity NPC_TranslateActivity( Activity baseAct );

	void StartTask( const Task_t *pTask );
	void RunTask( const Task_t *pTask );

	void GatherConditions( void );
	void PrescheduleThink( void );

	//TGB: virtual'd because fastie needs to un-ceiling itself when this happens
	//	also made bool so we know if a zombie ignored us
	virtual bool SetZombieMode( Zombie_Modes zomMode ); //qck: Toggle between defense/offense mode

	virtual void Event_Killed( const CTakeDamageInfo &info );
	virtual bool BecomeRagdoll( const CTakeDamageInfo &info, const Vector &forceVector );
	void StopLoopingSounds();
	virtual void OnScheduleChange( void );

	virtual void PoundSound();

	// Custom damage/death 
	bool ShouldIgnite( const CTakeDamageInfo &info );
	virtual bool IsChopped( const CTakeDamageInfo &info );
	virtual bool IsSquashed( const CTakeDamageInfo &info ) { return false; }
	virtual void DieChopped( const CTakeDamageInfo &info );
	virtual void Ignite( float flFlameLifetime, bool bNPCOnly = true, float flSize = 0.0f, bool bCalledByLevelDesigner = false );
	void CopyRenderColorTo( CBaseEntity *pOther );

	virtual bool ShouldBecomeTorso( const CTakeDamageInfo &info, float flDamageThreshold );
//	virtual HeadcrabRelease_t ShouldReleaseHeadcrab( const CTakeDamageInfo &info, float flDamageThreshold );

	// Headcrab releasing/breaking apart
//	void RemoveHead( void );
	virtual void SetZombieModel( void ) { };
	virtual void BecomeTorso( const Vector &vecTorsoForce, const Vector &vecLegsForce );
	virtual bool CanBecomeLiveTorso() { return false; }
//	virtual bool CNPC_BaseZombie::HeadcrabFits( CBaseAnimating *pCrab );
//	void ReleaseHeadcrab( const Vector &vecOrigin, const Vector &vecVelocity, bool fRemoveHead, bool fRagdollBody, bool fRagdollCrab = false );
//	void SetHeadcrabSpawnLocation( int iCrabAttachment, CBaseAnimating *pCrab );

	// Swatting physics objects
	int GetSwatActivity( void );
	bool FindNearestPhysicsObject( int iMaxMass, int iMaxDistance = 0 );
	float DistToPhysicsEnt( void );
	virtual bool CanSwatPhysicsObjects( void ) { return false; } //TGB: made false by default: only shamblers can swat typically
	void SwatObject( IPhysicsObject *pPhysObj, Vector direction ); //the actual swat func

	// Returns whether we must be very near our enemy to attack them.
	virtual bool MustCloseToAttack(void) { return true; }

	virtual CBaseEntity *ClawAttack( float flDist, int iDamage, QAngle &qaViewPunch, Vector &vecVelocityPunch, int BloodOrigin );

	// Sounds & sound envelope
	virtual bool ShouldPlayFootstepMoan( void );
	//virtual void PainSound( ) = 0; //TGB: instead use the basenpc virtual
	virtual void AlertSound( void ) = 0;
	virtual void IdleSound( void ) = 0;
	virtual void AttackSound( void ) = 0;
	virtual void AttackHitSound( void ) = 0;
	virtual void AttackMissSound( void ) = 0;
	virtual void FootstepSound( bool fRightFoot ) = 0;
	virtual void FootscuffSound( bool fRightFoot ) = 0;

	virtual bool CanPlayMoanSound();
//	virtual void MoanSound( envelopePoint_t *pEnvelope, int iEnvelopeSize );
	bool ShouldPlayIdleSound( void ) { return false; }

	virtual const char *GetMoanSound( int nSound ) = 0;
//	virtual const char *GetHeadcrabClassname( void ) = 0;
	//virtual const char *GetLegsModel( void ) = 0;
	//virtual const char *GetTorsoModel( void ) = 0;
//	virtual const char *GetHeadcrabModel( void ) = 0;

//	virtual Vector HeadTarget( const Vector &posSrc );

	bool OnInsufficientStopDist( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult );

	virtual	bool		AllowedToIgnite( void ) { return true; }

	//TGB: can use this for health buff [0000359]
	// Notifier that I've killed some other entity. (called from Victim's Event_Killed).
	virtual void	Event_KilledOther( CBaseEntity *pVictim, const CTakeDamageInfo &info ) 
	{	//basic bonus
		if (pVictim == this) return; //generally only happens with kill commands

		const int prev = GetHealth();
		TakeHealth( GetMaxHealth() * 0.5, DMG_GENERIC );
		DevMsg("Giving health to zombie for succesful kill (was: %i, now: %i)", prev, GetHealth() );
	}

	//TGB: for variable population costs, override in subclasses
	virtual int GetPopCost() { return 1; }

	//TGB: gibs! GIIIIIIIIIBS!
	virtual bool		ShouldGib( const CTakeDamageInfo &info );
	virtual bool		HasHumanGibs( void ) { return true; }

	
	enum {
		TYPE_SHAMBLER = 0,
		TYPE_BANSHEE,
		TYPE_HULK,
		TYPE_DRIFTER,
		TYPE_IMMOLATOR,

		TYPE_TOTAL,
		TYPE_INVALID = -1
	};

	//TGB: returns type of zombie. Kind of ugly, but most our zombie code is in .cpp files we can't include, making casting just to typecheck hard
	virtual int			GetZombieType() { return TYPE_INVALID; }

protected:

	CSoundPatch	*m_pMoanSound;
	CZombieSpawn	*m_pSpawnPoint;

	bool			m_fIsTorso;						// is this is a half-zombie?
	bool			m_fIsHeadless;					// is this zombie headless

	bool			m_bSwatBreakable;				// is the object we will be swatting a breakable?

	Zombie_Modes	m_zombieMode;					//qck: Keep track of current mode

	float	m_flNextFlinch;

//	bool m_bHeadShot;			// Used to determine the survival of our crab beyond our death.

	//
	// Zombies catch on fire if they take too much burn damage in a given time period.
	//
	float	m_flBurnDamage;				// Keeps track of how much burn damage we've incurred in the last few seconds.
	float	m_flBurnDamageResetTime;	// Time at which we reset the burn damage.

	EHANDLE m_hPhysicsEnt;

	float m_flNextMoanSound;
	float m_flNextSwat;
	float m_flNextSwatScan;
//	float m_crabHealth;
	float m_flMoanPitch;

	EHANDLE	m_hObstructor;

	static int g_numZombies;	// counts total number of existing zombies.

	int m_iMoanSound; // each zombie picks one of the 4 and keeps it.

	static int ACT_ZOM_SWATLEFTMID;
	static int ACT_ZOM_SWATRIGHTMID;
	static int ACT_ZOM_SWATLEFTLOW;
	static int ACT_ZOM_SWATRIGHTLOW;
//	static int ACT_ZOM_RELEASECRAB;
	static int ACT_ZOM_FALL;
	float m_flNextBurnFrame;

	//TGB: fadein stuff
	float	m_fFadeFinish;

	DECLARE_DATADESC();

	DEFINE_CUSTOM_AI;
};

#endif // NPC_BASEZOMBIE_H
