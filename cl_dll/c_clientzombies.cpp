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
// Purpose: TGB: clientside for zombie NPCs, for use in tooltips and perhaps future endeavours
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_AI_BaseNPC.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//TGB: hopefully this won't create gobs of extra network traffic. I can't imagine why it would.

class C_Zombie : public C_AI_BaseNPC
{
	DECLARE_CLASS( C_Zombie, C_AI_BaseNPC );

public:
	DECLARE_CLIENTCLASS();

	C_Zombie() {
		SetPredictionEligible(true);
	}

	//TGB: virtual that can be overridden by clientside implementations to return the tooltip text
	const char*		GetZMTooltip() { return "Shambler: slow zombie that dies (again) quickly, but packs a mean swipe. The only anti-barricade unit."; }

	/*
	void SetRandomExpression( void );

	void Spawn() {
		BaseClass::Spawn();

		SetRandomExpression();
	}*/

private:
	C_Zombie( const C_Zombie & ); // not defined, not accessible
	bool		ShouldHaveHealthCircle(){return true;};
};

IMPLEMENT_CLIENTCLASS_DT( C_Zombie, DT_Zombie, CZombie )
END_RECV_TABLE()

//TGB: looks like not enough of the zombie is clientside to make this work
//void C_Zombie::SetRandomExpression( void )
//{
//#define TEMPFLEX(name, weight)	SetFlexWeight(FindFlexController(name), weight)
//
//	//sad zombies
//	/* for testing
//	TEMPFLEX("bite", 1.000);
//	TEMPFLEX("jaw_drop", 0.600);
//	TEMPFLEX("right_corner_depressor", 1.000);
//	TEMPFLEX("left_corner_depressor", 1.000);
//	TEMPFLEX("left_lowerer", 1.000);
//	TEMPFLEX("right_lowerer", 1.000);
//	*/
//
//	/*
//	the setting of flexweights is going to be rather verbose, each controller on a
//	seperate line, because it makes it much easier to adjust individual max rand values
//	*/
//
//	//sqrt(random->RandomFloat( 0.000, 1.000 )) for biasing upwards
//
//	//DevMsg("\tRandomising z'expression: Non-randoms... ");
//	//always set these, no conflicts
//	TEMPFLEX( "right_cheek_raiser", sqrt(random->RandomFloat( 0.000, 1.000 )));
//	TEMPFLEX( "left_cheek_raiser", sqrt(random->RandomFloat( 0.000, 1.000 )));
//	TEMPFLEX( "wrinkler", random->RandomFloat( 0.000, 1.000 ));
//	TEMPFLEX( "dilator", random->RandomFloat( 0.000, 1.000 ));
//	TEMPFLEX( "right_corner_depressor", sqrt(random->RandomFloat( 0.000, 1.000 )));
//	TEMPFLEX( "left_corner_depressor", sqrt(random->RandomFloat( 0.000, 1.000 )));
//	TEMPFLEX( "right_part", sqrt(random->RandomFloat( 0.000, 1.000 )));
//	TEMPFLEX( "left_part", sqrt(random->RandomFloat( 0.000, 1.000 )));
//	TEMPFLEX( "jaw_clencher", random->RandomFloat( 0.000, 1.000 ));
//	//DevMsg("done -- ");
//
//	//raise or lower eyebrows
//	//DevMsg("Eyebrows... ");
//	if (random->RandomInt(0, 1))
//	{
//		//DevMsg("up -- ");
//		TEMPFLEX( "right_inner_raiser", sqrt(random->RandomFloat( 0.000, 1.000 )));
//		TEMPFLEX( "left_inner_raiser", sqrt(random->RandomFloat( 0.000, 1.000 )));
//		TEMPFLEX( "right_outer_raiser", random->RandomFloat( 0.000, 1.000 ));
//		TEMPFLEX( "left_outer_raiser", random->RandomFloat( 0.000, 1.000 ));
//	}
//	else
//	{
//		//DevMsg("down -- ");
//		TEMPFLEX( "right_lowerer", sqrt(random->RandomFloat( 0.000, 1.000 )));
//		TEMPFLEX( "left_lowerer", sqrt(random->RandomFloat( 0.000, 1.000 )));
//	}
//
//	//for copying: 
//	//TEMPFLEX( "name", random->RandomFloat( 0.000, 1.000 ));
//
//	//bite or do lots of different stuff
//	//DevMsg("Mouth... ");
//	if (random->RandomInt(1, 10) < 2) //some 10% chance of bite
//	{
//		//DevMsg("biting -- ");
//		TEMPFLEX( "bite", random->RandomFloat( 0.000, 1.000 ));
//	}
//	else
//	{
//		//DevMsg("not biting -- ");
//		TEMPFLEX( "right_upper_raiser", random->RandomFloat( 0.000, 1.000 ));
//		TEMPFLEX( "left_upper_raiser", random->RandomFloat( 0.000, 1.000 ));
//		TEMPFLEX( "lower_lip", random->RandomFloat( 0.000, 1.000 ));
//
//		//smile or stretch things a bit
//		if (random->RandomInt(1, 10) < 3) //some 20% chance of smile, it's not very zombielike
//		{
//			//DevMsg("Smiling... ");
//			TEMPFLEX( "smile", random->RandomFloat( 0.000, 1.000 ));
//			//DevMsg("done -- ");
//		}
//		else
//		{
//			//DevMsg("Misc... ");
//			TEMPFLEX( "chin_raiser", random->RandomFloat( 0.000, 1.000 ));
//			TEMPFLEX( "right_stretcher", random->RandomFloat( 0.000, 1.000 ));
//			TEMPFLEX( "left_stretcher", random->RandomFloat( 0.000, 1.000 ));
//			//DevMsg("done -- ");
//		}
//
//		//pucker/funnel or pull up
//		//DevMsg("Lip shaping... ");
//		if (random->RandomInt(1, 10) < 5) //40% or so, it can look pretty retarded, but so can corner pulls
//		{
//			//DevMsg("pucker/funnel ");
//			TEMPFLEX( "right_puckerer", random->RandomFloat( 0.000, 0.333 ));		//below standard values
//			TEMPFLEX( "left_puckerer", random->RandomFloat( 0.000, 0.333 ));		//below standard values
//			TEMPFLEX( "right_funneler", random->RandomFloat( 0.000, 1.000 ));
//			TEMPFLEX( "left_funneler", random->RandomFloat( 0.000, 1.000 ));
//			//DevMsg("done -- ");
//		}
//		else
//		{
//			//DevMsg("corner pulling ");
//			TEMPFLEX( "right_corner_puller", random->RandomFloat( 0.000, 0.750 ));	//below standard values
//			TEMPFLEX( "left_corner_puller", random->RandomFloat( 0.000, 0.750 ));	//below standard values
//			//DevMsg("done -- ");
//		}
//
//		//we're jaw droppin' aw yeah
//		//... I don't know what that was about either
//
//		//DevMsg("Jaw dropping... ");
//		TEMPFLEX( "jaw_drop", sqrt(random->RandomFloat( 0.000, 1.200 )));
//		//the following two are only visible if jaw_drop is of a decent value
//		//1.3, because anything > 1.0 gets clamped down anyway, this just makes a high factor more likely. Hacky but hopefully working.
//		TEMPFLEX( "right_mouth_drop", sqrt(random->RandomFloat( 0.000, 1.300 )));
//		TEMPFLEX( "left_mouth_drop", sqrt(random->RandomFloat( 0.000, 1.300 )));
//		//DevMsg("done. ");
//
//	}
//	//DevMsg("Expression randomised!\t");
//}


//-----------------------------------------------
//-----------------------------------------------

class C_NPC_PoisonZombie : public C_AI_BaseNPC
{
	DECLARE_CLASS( C_NPC_PoisonZombie, C_AI_BaseNPC );

public:
	DECLARE_CLIENTCLASS();

	C_NPC_PoisonZombie() {}

	//TGB: virtual that can be overridden by clientside implementations to return the tooltip text
	const char*		GetZMTooltip() { return "Hulk: very tough and deadly, but not fast or cheap."; }

private:
	C_NPC_PoisonZombie( const C_NPC_PoisonZombie & ); // not defined, not accessible
	bool		ShouldHaveHealthCircle(){return true;};
};

IMPLEMENT_CLIENTCLASS_DT( C_NPC_PoisonZombie, DT_NPC_PoisonZombie, CNPC_PoisonZombie )
END_RECV_TABLE()
//-----------------------------------------------
//-----------------------------------------------

class C_FastZombie : public C_AI_BaseNPC
{
	DECLARE_CLASS( C_FastZombie, C_AI_BaseNPC );

public:
	DECLARE_CLIENTCLASS();

	C_FastZombie() {}

	//TGB: virtual that can be overridden by clientside implementations to return the tooltip text
	const char*		GetZMTooltip() { return "Banshee: quick, agile and fragile."; }

private:
	C_FastZombie( const C_FastZombie & ); // not defined, not accessible
	bool		ShouldHaveHealthCircle(){return true;};
};

IMPLEMENT_CLIENTCLASS_DT( C_FastZombie, DT_FastZombie, CFastZombie )
END_RECV_TABLE()

//-----------------------------------------------
//-----------------------------------------------

class C_NPC_DragZombie : public C_AI_BaseNPC
{
	DECLARE_CLASS( C_NPC_DragZombie, C_AI_BaseNPC );

public:
	DECLARE_CLIENTCLASS();

	C_NPC_DragZombie() {}

	//TGB: virtual that can be overridden by clientside implementations to return the tooltip text
	const char*		GetZMTooltip() { return "Drifter: spits acidic blood to disorient players from a short distance."; }

private:
	C_NPC_DragZombie( const C_NPC_DragZombie & ); // not defined, not accessible
	bool		ShouldHaveHealthCircle(){return true;};
};

IMPLEMENT_CLIENTCLASS_DT( C_NPC_DragZombie, DT_NPC_DragZombie, CNPC_DragZombie )
END_RECV_TABLE()

//-----------------------------------------------
//-----------------------------------------------

class C_NPC_BurnZombie : public C_AI_BaseNPC
{
	DECLARE_CLASS( C_NPC_BurnZombie, C_AI_BaseNPC );

public:
	DECLARE_CLIENTCLASS();

	C_NPC_BurnZombie() {}

	//TGB: virtual that can be overridden by clientside implementations to return the tooltip text
	const char*		GetZMTooltip() { return "Immolator: a fiery package."; }

private:
	C_NPC_BurnZombie( const C_NPC_BurnZombie & ); // not defined, not accessible
	bool		ShouldHaveHealthCircle(){return true;};
};

IMPLEMENT_CLIENTCLASS_DT( C_NPC_BurnZombie, DT_NPC_BurnZombie, CNPC_BurnZombie )
END_RECV_TABLE()