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
// Purpose: Clientside implementations of stuff related to ZM powers
//
// $Log: zm_powers_client.cpp,v $
// Revision 1.3  2006-12-14 12:32:46  tgb
// *** empty log message ***
//
// Revision 1.2  2006-07-28 15:28:48  tgb
// *** empty log message ***
//
// Revision 1.1  2006-07-26 07:47:29  tgb
// exppower stuff
//
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "materialsystem/IMaterial.h"
#include "materialsystem/imaterialvar.h"
#include "view_scene.h"

/*
extern void DrawSprite( const Vector &vecOrigin, float flWidth, float flHeight, color32 color );

#define PHYSEXP_EFFECT_DELAY 7.33f

//-----------------------------------------------------------------------------
// Purpose: Client-side physexplode.
//-----------------------------------------------------------------------------
class C_DelayedPhysExp_Effects : public C_BaseAnimating
{
public:
	DECLARE_CLASS( C_DelayedPhysExp_Effects, C_BaseAnimating );
	DECLARE_CLIENTCLASS();

	C_DelayedPhysExp_Effects();


	ShadowType_t ShadowCastType();

	int DrawModel( int flags );

private:
//	C_PhysExplodeFX		visualCue;
	float	m_EndTime;
	float	m_time;
};

IMPLEMENT_CLIENTCLASS_DT(C_DelayedPhysExp_Effects, DT_DelayedPhysExp_Effects, CDelayedPhysExp_Effects)
END_RECV_TABLE()

C_DelayedPhysExp_Effects::C_DelayedPhysExp_Effects()
{
	m_EndTime = gpGlobals->curtime + PHYSEXP_EFFECT_DELAY;
	m_time = gpGlobals->curtime;
}

//see c_strider.cpp
int C_DelayedPhysExp_Effects::DrawModel( int flags )
{
	static color32 white = {255,255,255,255};
	static color32 red = {255,50,50,255};


	return 1;
}

ShadowType_t C_DelayedPhysExp_Effects::ShadowCastType()
{
	return SHADOWS_NONE;
}

*/