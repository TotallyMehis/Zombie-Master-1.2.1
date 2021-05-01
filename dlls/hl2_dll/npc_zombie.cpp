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
// Purpose: The shambler
//
//=============================================================================//

#include "cbase.h"

#include "doors.h"

#include "simtimer.h"
#include "npc_BaseZombie.h"
#include "ai_hull.h"
#include "ai_navigator.h"
#include "ai_memory.h"
#include "gib.h"
//#include "soundenvelope.h"
#include "props.h"
#include "engine/IEngineSound.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

const int ZOMBIE_MODEL_COUNT = 9;
const int ZOMBIE_SKIN_COUNT = 3;

//#define TGB_DISMEMBER 0

ConVar	sk_zombie_health( "sk_zombie_health","55");
ConVar	happy_zombies( "happyzombies","0");
ConVar	debug_skinchange_test( "debug_skinchange_test","0", FCVAR_CHEAT);

//static ConVar zm_popcost_shambler("zm_popcost_shambler", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Population points taken up by shamblers");

/*envelopePoint_t envZombieMoanVolumeFast[] =
{
	{	7.0f, 7.0f,
		0.1f, 0.1f,
	},
	{	0.0f, 0.0f,
		0.2f, 0.3f,
	},
};

envelopePoint_t envZombieMoanVolume[] =
{
	{	1.0f, 1.0f,
		0.1f, 0.1f,
	},
	{	1.0f, 1.0f,
		0.2f, 0.2f,
	},
	{	0.0f, 0.0f,
		0.3f, 0.4f,
	},
};

envelopePoint_t envZombieMoanVolumeLong[] =
{
	{	1.0f, 1.0f,
		0.3f, 0.5f,
	},
	{	1.0f, 1.0f,
		0.6f, 1.0f,
	},
	{	0.0f, 0.0f,
		0.3f, 0.4f,
	},
};

envelopePoint_t envZombieMoanIgnited[] =
{
	{	1.0f, 1.0f,
		0.5f, 1.0f,
	},
	{	1.0f, 1.0f,
		30.0f, 30.0f,
	},
	{	0.0f, 0.0f,
		0.5f, 1.0f,
	},
};

*/
//=============================================================================
//=============================================================================

class CZombie : public CAI_BlendingHost<CNPC_BaseZombie>
{
	DECLARE_DATADESC();
	DECLARE_CLASS( CZombie, CAI_BlendingHost<CNPC_BaseZombie> );

	//TGB: we have a client of this now
	DECLARE_SERVERCLASS();

public:
	CZombie()
	 : m_DurationDoorBash( 2, 6),
	   m_NextTimeToStartDoorBash( 3.0 )
	{
		//TGB CYCLEDEBUG
		//TGB: keeping this to basezombie
		//UseClientSideAnimation(); //LAWYER:  Hack

	}

	void Spawn( void );
	void Precache( void );

	Class_T Classify( void );
	Disposition_t IRelationType( CBaseEntity *pTarget );

	void SetZombieModel( void );

	//TGB: set randomised zombie expression
	void SetRandomExpression ( void );

//	void MoanSound( envelopePoint_t *pEnvelope, int iEnvelopeSize );
	bool ShouldBecomeTorso( const CTakeDamageInfo &info, float flDamageThreshold );
	bool CanBecomeLiveTorso() { return !m_fIsHeadless; }
	void TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr ); // qck edit

	void GatherConditions( void );

	int SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode );
	int TranslateSchedule( int scheduleType );

	//TGB: use default flinch checks
	//void CheckFlinches() {} // Zombie has custom flinch code

	void PostscheduleThink( void );

	Activity NPC_TranslateActivity( Activity newActivity );

	void OnStateChange( NPC_STATE OldState, NPC_STATE NewState );

	void StartTask( const Task_t *pTask );
	void RunTask( const Task_t *pTask );

	//virtual const char *GetLegsModel( void );
	//virtual const char *GetTorsoModel( void );
//	virtual const char *GetHeadcrabClassname( void );
//	virtual const char *GetHeadcrabModel( void );

	virtual bool OnObstructingDoor( AILocalMoveGoal_t *pMoveGoal, 
								 CBaseDoor *pDoor,
								 float distClear, 
								 AIMoveResult_t *pResult );

	Activity SelectDoorBash();

	void Ignite( float flFlameLifetime, bool bNPCOnly = true, float flSize = 0.0f, bool bCalledByLevelDesigner = false );
	void Extinguish();
	int OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo );
	bool IsSquashed( const CTakeDamageInfo &info );
	void BuildScheduleTestBits( void );

	void PrescheduleThink( void );
	int SelectSchedule ( void );

	void PainSound( const CTakeDamageInfo &info );
	void DeathSound(const CTakeDamageInfo &info);
	void AlertSound( void );
	void IdleSound( void );
	void AttackSound( void );
	void AttackHitSound( void );
	void AttackMissSound( void );
	void FootstepSound( bool fRightFoot );
	void FootscuffSound( bool fRightFoot );

	const char *GetMoanSound( int nSound );

	int GetPopCost() { return zm_popcost_shambler.GetInt(); }
	int	GetZombieType() { return TYPE_SHAMBLER; }

	bool CanSwatPhysicsObjects( void ) { return true; } //TGB: we can swat
public:
	DEFINE_CUSTOM_AI;

protected:
	static const char *pMoanSounds[];


private:
	CHandle< CBaseDoor > m_hBlockingDoor;
	float				 m_flDoorBashYaw;
	
	CRandSimTimer 		 m_DurationDoorBash;
	CSimTimer 	  		 m_NextTimeToStartDoorBash;

	bool				 m_bIsSlumped;
	
	Vector				 m_vPositionCharged;
};

LINK_ENTITY_TO_CLASS( npc_zombie, CZombie );
LINK_ENTITY_TO_CLASS( npc_zombie_torso, CZombie );

//---------------------------------------------------------
//---------------------------------------------------------
const char *CZombie::pMoanSounds[] =
{
	 "NPC_BaseZombie.Moan1",
	 "NPC_BaseZombie.Moan2",
	 "NPC_BaseZombie.Moan3",
	 "NPC_BaseZombie.Moan4",
};

//=========================================================
// Conditions
//=========================================================
enum
{
	COND_BLOCKED_BY_DOOR = LAST_BASE_ZOMBIE_CONDITION,
	COND_DOOR_OPENED,
	COND_ZOMBIE_CHARGE_TARGET_MOVED,
};

//=========================================================
// Schedules
//=========================================================
enum
{
	SCHED_ZOMBIE_BASH_DOOR = LAST_BASE_ZOMBIE_SCHEDULE,
	SCHED_ZOMBIE_WANDER_ANGRILY,
	SCHED_ZOMBIE_CHARGE_ENEMY,
	SCHED_ZOMBIE_FAIL,
};

//=========================================================
// Tasks
//=========================================================
enum
{
	TASK_ZOMBIE_EXPRESS_ANGER = LAST_BASE_ZOMBIE_TASK,
	TASK_ZOMBIE_YAW_TO_DOOR,
	TASK_ZOMBIE_ATTACK_DOOR,
	TASK_ZOMBIE_CHARGE_ENEMY,
};

//-----------------------------------------------------------------------------

int ACT_ZOMBIE_TANTRUM;
int ACT_ZOMBIE_WALLPOUND;

BEGIN_DATADESC( CZombie )

	DEFINE_FIELD( m_hBlockingDoor, FIELD_EHANDLE ),
	DEFINE_FIELD( m_flDoorBashYaw, FIELD_FLOAT ),
	DEFINE_EMBEDDED( m_DurationDoorBash ),
	DEFINE_EMBEDDED( m_NextTimeToStartDoorBash ),
	DEFINE_FIELD( m_bIsSlumped, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_vPositionCharged, FIELD_POSITION_VECTOR ),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CZombie, DT_Zombie )
END_SEND_TABLE()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CZombie::Precache( void )
{
	BaseClass::Precache();

	//PrecacheModel( "models/zombie/zm_classic.mdl" );
	PrecacheModel( "models/zombie/zm_classic_01.mdl" );
	PrecacheModel( "models/zombie/zm_classic_02.mdl" );
	PrecacheModel( "models/zombie/zm_classic_03.mdl" );
	PrecacheModel( "models/zombie/zm_classic_04.mdl" );
	PrecacheModel( "models/zombie/zm_classic_05.mdl" );
	PrecacheModel( "models/zombie/zm_classic_06.mdl" );
	PrecacheModel( "models/zombie/zm_classic_07.mdl" );
	PrecacheModel( "models/zombie/zm_classic_08.mdl" );
	PrecacheModel( "models/zombie/zm_classic_09.mdl" );
	//PrecacheModel( "models/zombie/classic_torso.mdl" );
	//PrecacheModel( "models/zombie/classic_legs.mdl" );

#ifdef TGB_DISMEMBER
	PrecacheModel( "models/zombie/gibtest.mdl" );
#endif

	PrecacheScriptSound( "Zombie.FootstepRight" );
	PrecacheScriptSound( "Zombie.FootstepLeft" );
	PrecacheScriptSound( "Zombie.FootstepLeft" );
	PrecacheScriptSound( "Zombie.ScuffRight" );
	PrecacheScriptSound( "Zombie.ScuffLeft" );
	PrecacheScriptSound( "Zombie.AttackHit" );
	PrecacheScriptSound( "Zombie.AttackMiss" );
	PrecacheScriptSound( "Zombie.Pain" );
	PrecacheScriptSound( "Zombie.Die" );
	PrecacheScriptSound( "Zombie.Alert" );
	PrecacheScriptSound( "Zombie.Idle" );
	PrecacheScriptSound( "Zombie.Attack" );

	PrecacheScriptSound( "NPC_BaseZombie.Moan1" );
	PrecacheScriptSound( "NPC_BaseZombie.Moan2" );
	PrecacheScriptSound( "NPC_BaseZombie.Moan3" );
	PrecacheScriptSound( "NPC_BaseZombie.Moan4" );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
Class_T CZombie::Classify( void )
{
	if ( m_bIsSlumped )
		return CLASS_NONE;

	return BaseClass::Classify();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
Disposition_t CZombie::IRelationType( CBaseEntity *pTarget )
{
	// Slumping should not affect Zombie's opinion of others
	if ( m_bIsSlumped )
	{
		m_bIsSlumped = false;
		Disposition_t result = BaseClass::IRelationType( pTarget );
		m_bIsSlumped = true;
		return result;
	}
	
	//qck: Special check to make sure zombies don't get pissy about snipers taking pot shots at them. 
	if (pTarget->IsNPC())
	{
		return D_NU;
	}

	return BaseClass::IRelationType( pTarget );
}

void CZombie::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr )
{

	CTakeDamageInfo infoCopy = info;
#ifdef TGB_DISMEMBER
	//TGB: it appears this is called when getting shot, so try some dismemberment here
#define L_ARM 1
#define GONE 1

	bool dismembered = false;
	if (ptr->hitgroup == HITGROUP_LEFTARM)
	{
		if (GetBodygroup(L_ARM) != GONE )
		{
			DevMsg("Zombie gots left arm shot arf :O\n");
			SetBodygroup( L_ARM, GONE );

			dismembered = true;
		}
		else
		{
			//DevMsg("Zombie has no left arm to hit :shh: no damage\n");
			return;
		}
	}
#endif
	UTIL_BloodSpray( ptr->endpos, vecDir, BLOOD_COLOR_RED, RandomInt( 4, 8 ), FX_BLOODSPRAY_ALL);
	//SpawnBlood(ptr->endpos, vecDir, BloodColor(), info.GetDamage());// a little surface blood.
	TraceBleed( info.GetDamage(), vecDir, ptr, info.GetDamageType() );

	BaseClass::TraceAttack( infoCopy, vecDir, ptr );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CZombie::Spawn( void )
{
	Precache();

	if( FClassnameIs( this, "npc_zombie" ) )
	{
		m_fIsTorso = false;
	}
	else
	{
		// This was placed as an npc_zombie_torso
		m_fIsTorso = true;
	}

	m_fIsHeadless = false;

	SetBloodColor( BLOOD_COLOR_RED );
	m_iHealth			= sk_zombie_health.GetFloat();
	m_flFieldOfView		= 0.2;

	m_bIsSlumped = false;

	CapabilitiesClear();

	CapabilitiesAdd( bits_CAP_ANIMATEDFACE );

	//GetNavigator()->SetRememberStaleNodes( false );

	BaseClass::Spawn();

	m_flNextMoanSound = gpGlobals->curtime + random->RandomFloat( 1.0, 4.0 );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CZombie::PrescheduleThink( void )
{
  	if( gpGlobals->curtime > m_flNextMoanSound )
  	{
  		if( CanPlayMoanSound() )
  		{
			// Classic guy idles instead of moans.
			IdleSound();

			//TGB: less moaning
  			m_flNextMoanSound = gpGlobals->curtime + random->RandomFloat( 5.0, 15.0 );
  		}
  		else
 		{
  			m_flNextMoanSound = gpGlobals->curtime + random->RandomFloat( 5.0, 12.0 );
  		}
  	}

	BaseClass::PrescheduleThink();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CZombie::SelectSchedule ( void )
{
	if( HasCondition( COND_PHYSICS_DAMAGE ) )
	{
		return SCHED_FLINCH_PHYSICS;
	}

	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
// Purpose: Sound of a footstep
//-----------------------------------------------------------------------------
void CZombie::FootstepSound( bool fRightFoot )
{
	if( fRightFoot )
	{
		EmitSound(  "Zombie.FootstepRight" );
	}
	else
	{
		EmitSound( "Zombie.FootstepLeft" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sound of a foot sliding/scraping
//-----------------------------------------------------------------------------
void CZombie::FootscuffSound( bool fRightFoot )
{
	if( fRightFoot )
	{
		EmitSound( "Zombie.ScuffRight" );
	}
	else
	{
		EmitSound( "Zombie.ScuffLeft" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Play a random attack hit sound
//-----------------------------------------------------------------------------
void CZombie::AttackHitSound( void )
{
	EmitSound( "Zombie.AttackHit" );
}

//-----------------------------------------------------------------------------
// Purpose: Play a random attack miss sound
//-----------------------------------------------------------------------------
void CZombie::AttackMissSound( void )
{
	// Play a random attack miss sound
	EmitSound( "Zombie.AttackMiss" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CZombie::PainSound( const CTakeDamageInfo &info )
{
	// We're constantly taking damage when we are on fire. Don't make all those noises!
	if ( IsOnFire() )
	{
		return;
	}

	EmitSound( "Zombie.Pain" );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CZombie::DeathSound(const CTakeDamageInfo &info) 
{
	EmitSound( "Zombie.Die" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CZombie::AlertSound( void )
{
	EmitSound( "Zombie.Alert" );

	// Don't let a moan sound cut off the alert sound.
	m_flNextMoanSound += random->RandomFloat( 2.0, 4.0 );
}

//-----------------------------------------------------------------------------
// Purpose: Returns a moan sound for this class of zombie.
//-----------------------------------------------------------------------------
const char *CZombie::GetMoanSound( int nSound )
{
	return pMoanSounds[ nSound % ARRAYSIZE( pMoanSounds ) ];
}

//-----------------------------------------------------------------------------
// Purpose: Play a random idle sound.
//-----------------------------------------------------------------------------
void CZombie::IdleSound( void )
{
	if( GetState() == NPC_STATE_IDLE && random->RandomFloat( 0, 1 ) == 0 )
	{
		// Moan infrequently in IDLE state.
		return;
	}

	if( m_bIsSlumped )
	{
		// Sleeping zombies are quiet.
		return;
	}

	EmitSound( "Zombie.Idle" );
}

//-----------------------------------------------------------------------------
// Purpose: Play a random attack sound.
//-----------------------------------------------------------------------------
void CZombie::AttackSound( void )
{
	EmitSound( "Zombie.Attack" );
}

//-----------------------------------------------------------------------------
// Purpose: Returns the classname (ie "npc_headcrab") to spawn when our headcrab bails.
//-----------------------------------------------------------------------------
/*const char *CZombie::GetHeadcrabClassname( void )
{
	return "npc_headcrab";
}
*/
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/*const char *CZombie::GetHeadcrabModel( void )
{
	return "models/headcrabclassic.mdl";
}
*/
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
//const char *CZombie::GetLegsModel( void )
//{
//	return "models/zombie/classic_legs.mdl";
//}
//
////-----------------------------------------------------------------------------
////-----------------------------------------------------------------------------
//const char *CZombie::GetTorsoModel( void )
//{
//	return "models/zombie/classic_torso.mdl";
//}


//---------------------------------------------------------
//---------------------------------------------------------
void CZombie::SetZombieModel( void )
{
	Hull_t lastHull = GetHullType();


	const int rand_model = random->RandomInt( 1, ZOMBIE_MODEL_COUNT );

	//DevMsg ("\nPicked zombie model %i; ", rand_model);
	//TGB: allright, I wrestled here with integer->char conversion for ages. I'm just going to hack something up now
	char* modelname[] = {
		"models/zombie/zm_classic_01.mdl", //default, rand_model = 0
		"models/zombie/zm_classic_01.mdl", //1
		"models/zombie/zm_classic_02.mdl", //2
		"models/zombie/zm_classic_03.mdl", //3
		"models/zombie/zm_classic_04.mdl", //4
		"models/zombie/zm_classic_05.mdl", //5
		"models/zombie/zm_classic_06.mdl", //6
		"models/zombie/zm_classic_07.mdl", //7
		"models/zombie/zm_classic_08.mdl", //8
		"models/zombie/zm_classic_09.mdl", //9
	};

	
	//DevMsg ("name: %s", modelname[rand_model]);
#ifdef TGB_DISMEMBER
	SetModel("models/zombie/gibtest.mdl");
#else
	SetModel(modelname[rand_model]);
#endif
	//set random skin
	m_nSkin = random->RandomInt( 0, ZOMBIE_SKIN_COUNT );

	SetHullType( HULL_HUMAN );

	SetHullSizeNormal( true );
	SetDefaultEyeOffset();
	SetActivity( ACT_IDLE );

	

	// hull changed size, notify vphysics
	// UNDONE: Solve this generally, systematically so other
	// NPCs can change size
	if ( lastHull != GetHullType() )
	{
		if ( VPhysicsGetObject() )
		{
			SetupVPhysicsHull();
		}
	}

	//syntax: SetFlexWeight("flexcontroller", float);

	//TGB: doesn't work since late '06 SDK update, the flex controllers have since been removed from the zombie models
	//TGB UNDONE, it seems it does work when connecting to a DS, needs testing
	//TGB: re-disabled, really can't get it to work, leaving the flex controllers in the models for now

	//TGB: this is me indulging with a happyzombies convar
	/*
	if (happy_zombies.GetBool())
	{
		
		//cheese!
		SetFlexWeight("smile", 1.000);
		SetFlexWeight("jaw_clench", 1.000);
		SetFlexWeight("right_inner_raiser", 1.000);
		SetFlexWeight("right_outer_raiser", 1.000);
		SetFlexWeight("left_lowerer", 1.000);

	}
	else
	{
		SetRandomExpression();
	}*/
}
//---------------------------------------------------------
// TGB: Neat (hopefully) random zombie expressions
//---------------------------------------------------------
void CZombie::SetRandomExpression( void )
{
	//sad zombies
	/* for testing
	SetFlexWeight("bite", 1.000);
	SetFlexWeight("jaw_drop", 0.600);
	SetFlexWeight("right_corner_depressor", 1.000);
	SetFlexWeight("left_corner_depressor", 1.000);
	SetFlexWeight("left_lowerer", 1.000);
	SetFlexWeight("right_lowerer", 1.000);
	*/

	/*
	the setting of flexweights is going to be rather verbose, each controller on a
	seperate line, because it makes it much easier to adjust individual max rand values
	*/
	
	//sqrt(random->RandomFloat( 0.000, 1.000 )) for biasing upwards

	//DevMsg("\tRandomising z'expression: Non-randoms... ");
	//always set these, no conflicts
	SetFlexWeight( "right_cheek_raiser", sqrt(random->RandomFloat( 0.000, 1.000 )));
	SetFlexWeight( "left_cheek_raiser", sqrt(random->RandomFloat( 0.000, 1.000 )));
	SetFlexWeight( "wrinkler", random->RandomFloat( 0.000, 1.000 ));
	SetFlexWeight( "dilator", random->RandomFloat( 0.000, 1.000 ));
	SetFlexWeight( "right_corner_depressor", sqrt(random->RandomFloat( 0.000, 1.000 )));
	SetFlexWeight( "left_corner_depressor", sqrt(random->RandomFloat( 0.000, 1.000 )));
	SetFlexWeight( "right_part", sqrt(random->RandomFloat( 0.000, 1.000 )));
	SetFlexWeight( "left_part", sqrt(random->RandomFloat( 0.000, 1.000 )));
	SetFlexWeight( "jaw_clencher", random->RandomFloat( 0.000, 1.000 ));
	//DevMsg("done -- ");

	//raise or lower eyebrows
	//DevMsg("Eyebrows... ");
	if (random->RandomInt(0, 1))
	{
		//DevMsg("up -- ");
		SetFlexWeight( "right_inner_raiser", sqrt(random->RandomFloat( 0.000, 1.000 )));
		SetFlexWeight( "left_inner_raiser", sqrt(random->RandomFloat( 0.000, 1.000 )));
		SetFlexWeight( "right_outer_raiser", random->RandomFloat( 0.000, 1.000 ));
		SetFlexWeight( "left_outer_raiser", random->RandomFloat( 0.000, 1.000 ));
	}
	else
	{
		//DevMsg("down -- ");
		SetFlexWeight( "right_lowerer", sqrt(random->RandomFloat( 0.000, 1.000 )));
		SetFlexWeight( "left_lowerer", sqrt(random->RandomFloat( 0.000, 1.000 )));
	}

	//for copying: 
	//SetFlexWeight( "name", random->RandomFloat( 0.000, 1.000 ));
	
	//bite or do lots of different stuff
	//DevMsg("Mouth... ");
	if (random->RandomInt(1, 10) < 2) //some 10% chance of bite
	{
		//DevMsg("biting -- ");
        SetFlexWeight( "bite", random->RandomFloat( 0.000, 1.000 ));
	}
	else
	{
		//DevMsg("not biting -- ");
		SetFlexWeight( "right_upper_raiser", random->RandomFloat( 0.000, 1.000 ));
		SetFlexWeight( "left_upper_raiser", random->RandomFloat( 0.000, 1.000 ));
		SetFlexWeight( "lower_lip", random->RandomFloat( 0.000, 1.000 ));
		
		//smile or stretch things a bit
		if (random->RandomInt(1, 10) < 3) //some 20% chance of smile, it's not very zombielike
		{
			//DevMsg("Smiling... ");
			SetFlexWeight( "smile", random->RandomFloat( 0.000, 1.000 ));
			//DevMsg("done -- ");
		}
		else
		{
			//DevMsg("Misc... ");
			SetFlexWeight( "chin_raiser", random->RandomFloat( 0.000, 1.000 ));
			SetFlexWeight( "right_stretcher", random->RandomFloat( 0.000, 1.000 ));
			SetFlexWeight( "left_stretcher", random->RandomFloat( 0.000, 1.000 ));
			//DevMsg("done -- ");
		}

		//pucker/funnel or pull up
		//DevMsg("Lip shaping... ");
		if (random->RandomInt(1, 10) < 5) //40% or so, it can look pretty retarded, but so can corner pulls
		{
			//DevMsg("pucker/funnel ");
			SetFlexWeight( "right_puckerer", random->RandomFloat( 0.000, 0.333 ));		//below standard values
			SetFlexWeight( "left_puckerer", random->RandomFloat( 0.000, 0.333 ));		//below standard values
			SetFlexWeight( "right_funneler", random->RandomFloat( 0.000, 1.000 ));
			SetFlexWeight( "left_funneler", random->RandomFloat( 0.000, 1.000 ));
			//DevMsg("done -- ");
		}
		else
		{
			//DevMsg("corner pulling ");
			SetFlexWeight( "right_corner_puller", random->RandomFloat( 0.000, 0.750 ));	//below standard values
			SetFlexWeight( "left_corner_puller", random->RandomFloat( 0.000, 0.750 ));	//below standard values
			//DevMsg("done -- ");
		}
		
		//DevMsg("Jaw dropping... ");
		SetFlexWeight( "jaw_drop", sqrt(random->RandomFloat( 0.000, 1.200 )));
		//the following two are only visible if jaw_drop is of a decent value
		//1.3, because anything > 1.0 gets clamped down anyway, this just makes a high factor more likely. Hacky but hopefully working.
		SetFlexWeight( "right_mouth_drop", sqrt(random->RandomFloat( 0.000, 1.300 )));
		SetFlexWeight( "left_mouth_drop", sqrt(random->RandomFloat( 0.000, 1.300 )));
		//DevMsg("done. ");

	}
	//DevMsg("Expression randomised!\t");
}

//---------------------------------------------------------
// Classic zombie only uses moan sound if on fire.
//---------------------------------------------------------
/*void CZombie::MoanSound( envelopePoint_t *pEnvelope, int iEnvelopeSize )
{
	if( IsOnFire() )
	{
		BaseClass::MoanSound( pEnvelope, iEnvelopeSize );
	}
}
*/
//---------------------------------------------------------
//---------------------------------------------------------
bool CZombie::ShouldBecomeTorso( const CTakeDamageInfo &info, float flDamageThreshold )
{
	if( m_bIsSlumped ) 
	{
		// Never break apart a slouched zombie. This is because the most fun
		// slouched zombies to kill are ones sleeping leaning against explosive
		// barrels. If you break them in half in the blast, the force of being
		// so close to the explosion makes the body pieces fly at ridiculous 
		// velocities because the pieces weigh less than the whole.
		return false;
	}

	return BaseClass::ShouldBecomeTorso( info, flDamageThreshold );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CZombie::GatherConditions( void )
{
	BaseClass::GatherConditions();

	static int conditionsToClear[] = 
	{
		COND_BLOCKED_BY_DOOR,
		COND_DOOR_OPENED,
		COND_ZOMBIE_CHARGE_TARGET_MOVED,
	};

	ClearConditions( conditionsToClear, ARRAYSIZE( conditionsToClear ) );

	if ( m_hBlockingDoor == NULL || 
		 ( m_hBlockingDoor->m_toggle_state == TS_AT_TOP || 
		   m_hBlockingDoor->m_toggle_state == TS_GOING_UP )  )
	{
		ClearCondition( COND_BLOCKED_BY_DOOR );
		if ( m_hBlockingDoor != NULL )
		{
			SetCondition( COND_DOOR_OPENED );
			m_hBlockingDoor = NULL;
		}
	}
	else
		SetCondition( COND_BLOCKED_BY_DOOR );

	if ( ConditionInterruptsCurSchedule( COND_ZOMBIE_CHARGE_TARGET_MOVED ) )
	{
		if ( GetNavigator()->IsGoalActive() )
		{
			const float CHARGE_RESET_TOLERANCE = 60.0;
			if ( !GetEnemy() ||
				 ( m_vPositionCharged - GetEnemyLKP()  ).Length() > CHARGE_RESET_TOLERANCE )
			{
				SetCondition( COND_ZOMBIE_CHARGE_TARGET_MOVED );
			}
				 
		}
	}
}

//---------------------------------------------------------
//---------------------------------------------------------

int CZombie::SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode )
{
	if ( HasCondition( COND_BLOCKED_BY_DOOR ) && m_hBlockingDoor != NULL )
	{
		ClearCondition( COND_BLOCKED_BY_DOOR );
		if ( m_NextTimeToStartDoorBash.Expired() && failedSchedule != SCHED_ZOMBIE_BASH_DOOR )
			return SCHED_ZOMBIE_BASH_DOOR;
		m_hBlockingDoor = NULL;
	}

	if ( failedSchedule != SCHED_ZOMBIE_CHARGE_ENEMY && 
		 IsPathTaskFailure( taskFailCode ) &&
		 random->RandomInt( 1, 100 ) < 50 )
	{
		return SCHED_ZOMBIE_CHARGE_ENEMY;
	}

	if ( failedSchedule != SCHED_ZOMBIE_WANDER_ANGRILY &&
		 ( failedSchedule == SCHED_TAKE_COVER_FROM_ENEMY || 
		   failedSchedule == SCHED_CHASE_ENEMY_FAILED ) )
	{
		return SCHED_ZOMBIE_WANDER_ANGRILY;
	}

	return BaseClass::SelectFailSchedule( failedSchedule, failedTask, taskFailCode );
}

//---------------------------------------------------------
//---------------------------------------------------------

int CZombie::TranslateSchedule( int scheduleType )
{
	if ( scheduleType == SCHED_COMBAT_FACE && IsUnreachable( GetEnemy() ) )
		return SCHED_TAKE_COVER_FROM_ENEMY;

	if ( !m_fIsTorso && scheduleType == SCHED_FAIL )
		return SCHED_ZOMBIE_FAIL;

	return BaseClass::TranslateSchedule( scheduleType );
}

//---------------------------------------------------------

void CZombie::PostscheduleThink( void )
{
	int sequence = GetSequence();
	if ( sequence != -1 )
	{
		m_bIsSlumped = ( strncmp( GetSequenceName( sequence ), "slump", 5 ) == 0 );
	}
}

//---------------------------------------------------------

Activity CZombie::NPC_TranslateActivity( Activity newActivity )
{
	newActivity = BaseClass::NPC_TranslateActivity( newActivity );

	if ( newActivity == ACT_RUN )
		return ACT_WALK;

	return newActivity;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CZombie::OnStateChange( NPC_STATE OldState, NPC_STATE NewState )
{
	BaseClass::OnStateChange( OldState, NewState );
}

//---------------------------------------------------------
//---------------------------------------------------------

void CZombie::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_ZOMBIE_EXPRESS_ANGER:
		{
			if ( random->RandomInt( 1, 4 ) == 2 )
			{
				SetIdealActivity( (Activity)ACT_ZOMBIE_TANTRUM );
			}
			else
			{
				TaskComplete();
			}

			break;
		}

	case TASK_ZOMBIE_YAW_TO_DOOR:
		{
			AssertMsg( m_hBlockingDoor != NULL, "Expected condition handling to break schedule before landing here" );
			if ( m_hBlockingDoor != NULL )
			{
				GetMotor()->SetIdealYaw( m_flDoorBashYaw );
			}
			TaskComplete();
			break;
		}

	case TASK_ZOMBIE_ATTACK_DOOR:
		{
		 	m_DurationDoorBash.Reset();
			SetIdealActivity( SelectDoorBash() );
			break;
		}

	case TASK_ZOMBIE_CHARGE_ENEMY:
		{
			if ( !GetEnemy() )
				TaskFail( FAIL_NO_ENEMY );
			else if ( GetNavigator()->SetVectorGoalFromTarget( GetEnemy()->GetLocalOrigin() ) )
			{
				m_vPositionCharged = GetEnemy()->GetLocalOrigin();
				TaskComplete();
			}
			else
				TaskFail( FAIL_NO_ROUTE );
			break;
		}

	default:
		BaseClass::StartTask( pTask );
		break;
	}
}

//---------------------------------------------------------
//---------------------------------------------------------

void CZombie::RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_ZOMBIE_ATTACK_DOOR:
		{
			if ( IsActivityFinished() )
			{
				if ( m_DurationDoorBash.Expired() )
				{
					TaskComplete();
					m_NextTimeToStartDoorBash.Reset();
				}
				else
					ResetIdealActivity( SelectDoorBash() );
			}
			break;
		}

	case TASK_ZOMBIE_CHARGE_ENEMY:
		{
			break;
		}

	case TASK_ZOMBIE_EXPRESS_ANGER:
		{
			if ( IsActivityFinished() )
			{
				TaskComplete();
			}
			break;
		}

	default:
		BaseClass::RunTask( pTask );
		break;
	}
}

//---------------------------------------------------------
//---------------------------------------------------------

bool CZombie::OnObstructingDoor( AILocalMoveGoal_t *pMoveGoal, CBaseDoor *pDoor, 
							  float distClear, AIMoveResult_t *pResult )
{
	if ( BaseClass::OnObstructingDoor( pMoveGoal, pDoor, distClear, pResult ) )
	{
		if  ( IsMoveBlocked( *pResult ) && pMoveGoal->directTrace.vHitNormal != vec3_origin )
		{
			m_hBlockingDoor = pDoor;
			m_flDoorBashYaw = UTIL_VecToYaw( pMoveGoal->directTrace.vHitNormal * -1 );	
		}
		DevMsg("Zombie found obstructing door.\n");
		return true;
	}

	return false;
}

//---------------------------------------------------------
//---------------------------------------------------------

Activity CZombie::SelectDoorBash()
{
	if ( random->RandomInt( 1, 3 ) == 1 )
		return ACT_MELEE_ATTACK1;
	return (Activity)ACT_ZOMBIE_WALLPOUND;
}

//---------------------------------------------------------
// Zombies should scream continuously while burning, so long
// as they are alive.
//---------------------------------------------------------
void CZombie::Ignite( float flFlameLifetime, bool bNPCOnly, float flSize, bool bCalledByLevelDesigner )
{
	if( !IsOnFire() && IsAlive() )
	{
		BaseClass::Ignite( flFlameLifetime, bNPCOnly, flSize, bCalledByLevelDesigner );

		RemoveSpawnFlags( SF_NPC_GAG );
		
/*		MoanSound( envZombieMoanIgnited, ARRAYSIZE( envZombieMoanIgnited ) );

		if( m_pMoanSound )
		{
			ENVELOPE_CONTROLLER.SoundChangePitch( m_pMoanSound, 120, 1.0 );
			ENVELOPE_CONTROLLER.SoundChangeVolume( m_pMoanSound, 1, 1.0 );
		}*/
	}
}

//---------------------------------------------------------
// If a zombie stops burning and hasn't died, quiet him down
//---------------------------------------------------------
void CZombie::Extinguish()
{
/*	if( m_pMoanSound )
	{
		ENVELOPE_CONTROLLER.SoundChangeVolume( m_pMoanSound, 0, 2.0 );
		ENVELOPE_CONTROLLER.SoundChangePitch( m_pMoanSound, 100, 2.0 );
		m_flNextMoanSound = gpGlobals->curtime + random->RandomFloat( 2.0, 4.0 );
	}
*/
	BaseClass::Extinguish();
}

//---------------------------------------------------------
//---------------------------------------------------------
int CZombie::OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo )
{
	/*
	if( inputInfo.GetDamageType() & DMG_BUCKSHOT )
	{
		if( !m_fIsTorso && inputInfo.GetDamage() > (m_iMaxHealth/3) )
		{
			// Always flinch if damaged a lot by buckshot, even if not shot in the head.
			// The reason for making sure we did at least 1/3rd of the zombie's max health
			// is so the zombie doesn't flinch every time the odd shotgun pellet hits them,
			// and so the maximum number of times you'll see a zombie flinch like this is 2.(sjb)

			//TGB:no use calling this, current zombie model does not have this animation
			//AddGesture( ACT_GESTURE_FLINCH_HEAD );

		}
	}
	*/

	//TGB: give the damage dealer our best look
	/* TGB: commented out because it didn't work, and for debug reasons
	if (inputInfo.GetAttacker())
        AddLookTarget(inputInfo.GetAttacker(), 1, 1000, 1);
	*/

	return BaseClass::OnTakeDamage_Alive( inputInfo );
}

//---------------------------------------------------------
//---------------------------------------------------------
#define ZOMBIE_SQUASH_MASS	300.0f  // Anything this heavy or heavier squashes a zombie good. (show special fx)
bool CZombie::IsSquashed( const CTakeDamageInfo &info )
{
	if( GetHealth() > 0 )
	{
		return false;
	}

	//TGB: zombies never need squashgibs. This commenting is to make sure zombies always ragdoll.

	//if( info.GetDamageType() & DMG_CRUSH )
	//{
	//	IPhysicsObject *pCrusher = info.GetInflictor()->VPhysicsGetObject();
	//	if( pCrusher && pCrusher->GetMass() >= ZOMBIE_SQUASH_MASS && info.GetInflictor()->WorldSpaceCenter().z > EyePosition().z )
	//	{
	//		// This heuristic detects when a zombie has been squashed from above by a heavy
	//		// item. Done specifically so we can add gore effects to Ravenholm cartraps.
	//		// The zombie must take physics damage from a 300+kg object that is centered above its eyes (comes from above)
	//		return true;
	//	}
	//}

	return false;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CZombie::BuildScheduleTestBits( void )
{
	BaseClass::BuildScheduleTestBits();

	if( !m_fIsTorso && !IsCurSchedule( SCHED_FLINCH_PHYSICS ) )
	{
		SetCustomInterruptCondition( COND_PHYSICS_DAMAGE );
	}
}

	
//=============================================================================

AI_BEGIN_CUSTOM_NPC( npc_zombie, CZombie )

	DECLARE_CONDITION( COND_BLOCKED_BY_DOOR )
	DECLARE_CONDITION( COND_DOOR_OPENED )
	DECLARE_CONDITION( COND_ZOMBIE_CHARGE_TARGET_MOVED )

	DECLARE_TASK( TASK_ZOMBIE_EXPRESS_ANGER )
	DECLARE_TASK( TASK_ZOMBIE_YAW_TO_DOOR )
	DECLARE_TASK( TASK_ZOMBIE_ATTACK_DOOR )
	DECLARE_TASK( TASK_ZOMBIE_CHARGE_ENEMY )
	
	DECLARE_ACTIVITY( ACT_ZOMBIE_TANTRUM );
	DECLARE_ACTIVITY( ACT_ZOMBIE_WALLPOUND );

	DEFINE_SCHEDULE
	( 
		SCHED_ZOMBIE_BASH_DOOR,

		"	Tasks"
		"		TASK_SET_ACTIVITY				ACTIVITY:ACT_ZOMBIE_TANTRUM"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_TAKE_COVER_FROM_ENEMY"
		"		TASK_ZOMBIE_YAW_TO_DOOR			0"
		"		TASK_FACE_IDEAL					0"
		"		TASK_ZOMBIE_ATTACK_DOOR			0"
		""
		"	Interrupts"
		//"		COND_ZOMBIE_RELEASECRAB"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
		"		COND_DOOR_OPENED"
	)

	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_WANDER_ANGRILY,

		"	Tasks"
		"		TASK_WANDER						480240" // 48 units to 240 units.
		"		TASK_WALK_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			4"
		""
		"	Interrupts"
		//"		COND_ZOMBIE_RELEASECRAB"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
		"		COND_DOOR_OPENED"
	)

	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_CHARGE_ENEMY,


		"	Tasks"
		"		TASK_ZOMBIE_CHARGE_ENEMY		0"
		"		TASK_WALK_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_MELEE_ATTACK1" /* placeholder until frustration/rage/fence shake animation available */
		""
		"	Interrupts"
		//"		COND_ZOMBIE_RELEASECRAB"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
		"		COND_DOOR_OPENED"
		"		COND_ZOMBIE_CHARGE_TARGET_MOVED"
	)

	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_FAIL,

		"	Tasks"
		"		TASK_STOP_MOVING		0"
		"		TASK_SET_ACTIVITY		ACTIVITY:ACT_ZOMBIE_TANTRUM"
		"		TASK_WAIT				1"
		"		TASK_WAIT_PVS			0"
		""
		"	Interrupts"
		"		COND_CAN_RANGE_ATTACK1 "
		"		COND_CAN_RANGE_ATTACK2 "
		"		COND_CAN_MELEE_ATTACK1 "
		"		COND_CAN_MELEE_ATTACK2"
		"		COND_GIVE_WAY"
		"		COND_DOOR_OPENED"
	)

AI_END_CUSTOM_NPC()

//=============================================================================
