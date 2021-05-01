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
// Purpose: Client's C_BaseCombatCharacter entity
//
// $Workfile:     $
// $Date: 2006-10-09 13:58:49 $
//
//-----------------------------------------------------------------------------
// $Log: c_basecombatcharacter.cpp,v $
// Revision 1.2  2006-10-09 13:58:49  tgb
// Interface made fixed (ie. nonproportional); minor misc stuff
//
// Revision 1.1  2006-06-01 15:57:33  tgb
// Initial full source add, take 2
//
// Revision 1.2  2006-05-31 14:38:10  tgb
// Committing code, seeing as the one grabbed from the server had unix linebreaks or something which meant a white line between every normal line resulting in unreadability.
//
// Revision 1.8  2006/01/28 15:14:06  qckbeam
// *Fixed zombie color issues
//
// Revision 1.7  2005/11/20 18:20:37  qckbeam
// Moved Spawn glowing to the cliet
//
// No weapons on ladders
//
// Revision 1.6  2005/11/07 12:10:11  theGreenBunny
// Fixed minor assert bugs and added sledgehammer. -TGB
//
// Revision 1.5  2005/11/05 23:34:12  qckbeam
// Fixed the windows dedicated server with a few small changes in here
//
// Revision 1.4  2005/11/05 21:15:57  qckbeam
// *GLAR GLAR GLAR
//
// Revision 1.3  2005/11/05 10:07:33  Angry_Lawyer
// Reverted some buggy bits :P -Angry Lawyer
//
// Revision 1.2  2005/11/04 17:35:30  qckbeam
// no message
//
// Revision 1.1.1.1  2005/10/11 14:57:49  theGreenBunny
// The latest version of the code for zombie master
//
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_basecombatcharacter.h"
//#include "fx_quad.h" done in header
#include "ClientEffectPrecacheSystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#if defined( CBaseCombatCharacter )
#undef CBaseCombatCharacter	
#endif

//TGB: you forgot the "" around false here which threw an assert
ConVar r_ZombieLights("r_ZombieLights", "false", FCVAR_CHEAT, "Turns zombie selection lights on and off");

//TGB: healthcircle config

ConVar zm_healthcircle_brightness( "zm_healthcircle_brightness", "0.15", FCVAR_ARCHIVE, "Healthcircle brightness between 1.0 and 0.0, where 1.0 is brightest and 0.0 is off. Clientside." );

	//Precahce the effects
	CLIENTEFFECT_REGISTER_BEGIN( PrecacheEffectHealthCircle )
	CLIENTEFFECT_MATERIAL( "effects/zombie_select" )
	CLIENTEFFECT_MATERIAL( "effects/zm_healthring" )
	CLIENTEFFECT_MATERIAL( "effects/yellowflare" )
	CLIENTEFFECT_REGISTER_END()


//TGB: new class for healthcircle and selection effects, that avoids overloading the sprite system by constantly drawing new sprites

void CFXCharSprite::Update( Vector newpos, Vector newcolor, bool draw )
{
	//TGB: is this a new position/color?
	if ( (newpos != m_FXData.m_vecOrigin) || (newcolor != m_FXData.m_Color))
	{
		//update position
		m_FXData.m_vecOrigin = newpos;
		//and color
		m_FXData.m_Color = newcolor;
	}
/*
	if (m_FXData.m_flYaw < 360)
	{
		m_FXData.m_flYaw = min(360, m_FXData.m_flYaw + 25);
	}
	else
		m_FXData.m_flYaw = 25;
*/	
	if ( draw )
		Draw();
}
void CFXCharSprite::Draw( )
{
//	VPROF_BUDGET( "CFXCharSprite::Draw", VPROF_BUDGETGROUP_PARTICLE_RENDERING );

	float scale = m_FXData.m_flStartScale;
	float alpha = m_FXData.m_flStartAlpha;
	
	//Bind the material
	IMesh* pMesh = materials->GetDynamicMesh( true, NULL, NULL, m_FXData.m_pMaterial );
	CMeshBuilder meshBuilder;

	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	Vector	pos;
	Vector	vRight, vUp;

	float color[4];

	color[0] = m_FXData.m_Color[0];
	color[1] = m_FXData.m_Color[1];
	color[2] = m_FXData.m_Color[2];
	color[3] = alpha;

	VectorVectors( m_FXData.m_vecNormal, vRight, vUp );

	Vector	rRight, rUp;

	rRight	= ( vRight * cos( DEG2RAD( m_FXData.m_flYaw ) ) ) - ( vUp * sin( DEG2RAD( m_FXData.m_flYaw ) ) );
	rUp		= ( vRight * cos( DEG2RAD( m_FXData.m_flYaw+90.0f ) ) ) - ( vUp * sin( DEG2RAD( m_FXData.m_flYaw+90.0f ) ) );

	vRight	= rRight * ( scale * 0.5f );
	vUp		= rUp * ( scale * 0.5f );

	pos = m_FXData.m_vecOrigin + vRight - vUp;

	meshBuilder.Position3fv( pos.Base() );
	meshBuilder.Normal3fv( m_FXData.m_vecNormal.Base() );
	meshBuilder.TexCoord2f( 0, 1.0f, 1.0f );
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	pos = m_FXData.m_vecOrigin - vRight - vUp;

	meshBuilder.Position3fv( pos.Base() );
	meshBuilder.Normal3fv( m_FXData.m_vecNormal.Base() );
	meshBuilder.TexCoord2f( 0, 0.0f, 1.0f );
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	pos = m_FXData.m_vecOrigin - vRight + vUp;

	meshBuilder.Position3fv( pos.Base() );
	meshBuilder.Normal3fv( m_FXData.m_vecNormal.Base() );
	meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	pos = m_FXData.m_vecOrigin + vRight + vUp;

	meshBuilder.Position3fv( pos.Base() );
	meshBuilder.Normal3fv( m_FXData.m_vecNormal.Base() );
	meshBuilder.TexCoord2f( 0, 1.0f, 0.0f );
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
}

void CFXCharSprite::Destroy( void )
{
	//Release the material
	if ( m_FXData.m_pMaterial != NULL )
	{
		m_FXData.m_pMaterial->DecrementReferenceCount();
		m_FXData.m_pMaterial = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_BaseCombatCharacter::C_BaseCombatCharacter()
{
	for ( int i=0; i < m_iAmmo.Count(); i++ )
		m_iAmmo.Set( i, 0 );

	C_BasePlayer *local = C_BasePlayer::GetLocalPlayer();
	if (local && local->GetTeamNumber() == 3)
	{
		//TGB: create healthcircle
		CreateHealthCircle();

		//TGB: create select circle
		//only if we're not a player ie. if we're a zombie
		if (!this->IsPlayer())
			CreateSelectCircle();

	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_BaseCombatCharacter::~C_BaseCombatCharacter()
{
	C_BasePlayer *local = C_BasePlayer::GetLocalPlayer();
	//not sure about this, forcemaster seems like a recipe for screwup-cake
	if ( local && local->IsZM() == false)
		return;

	if (m_HealthCircle)
		delete m_HealthCircle;

	if (m_SelectCircle)
		delete m_SelectCircle;
}

int C_BaseCombatCharacter::DrawModel( int flags ) //Note to self: Selective drawing stuff goes here- qck
{
	ZMSelectionEffects();
	
	/*
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if (pPlayer && pPlayer->m_bNightVis)
	{
		float color[3] = { 0.0f, 255.0f, 0.0f };
		render->SetColorModulation(	color );
	}*/

	return BaseClass::DrawModel(flags);
}

void C_BaseCombatCharacter::CreateHealthCircle()
{
	//this and CreateSelectCircle() could probably be merged... oh well

	if (m_HealthCircle) return;

	//doubling brightness because the sprite is less visible now that it's drawn only once
	const float brightness = clamp((zm_healthcircle_brightness.GetFloat() * 3), 0.0, 1.0);

	//TGB: this defines a colour that moves from green to red depending on character's health
	//used in selection material and dedicated health circle
	//color should be redder as this guy approaches death
	const float redness = 1.0f - m_flHealthRatio;
	//as the ring gets redder it should become less green, or we'll end up yellow when near death
	const float greenness = 1.0f - redness;

	//make quad data
	FXQuadData_t data;
	//only using start values
	data.SetAlpha( 1.0, 0 );
	data.SetScale( 4 * 10.0f, 0 );
	data.SetMaterial( "effects/zm_healthring" );
	data.SetNormal( Vector(0,0,1) );
	data.SetOrigin( GetAbsOrigin() + Vector(0,0,3) );
	data.SetColor( redness*brightness, greenness*brightness, 0 );
	data.SetScaleBias( 0 );
	data.SetAlphaBias( 0 );
	data.SetYaw( 0, 0 );

	//TGB: create healthcircle
	m_HealthCircle = new CFXCharSprite( data );
}

void C_BaseCombatCharacter::CreateSelectCircle()
{
	//only if we don't already have one
	if (m_SelectCircle) return;

	//doubling brightness because the sprite is less visible now that it's drawn only once
	const float brightness = clamp((zm_healthcircle_brightness.GetFloat() * 3), 0.1, 1.0);

	//TGB: this defines a colour that moves from green to red depending on character's health
	//used in selection material and dedicated health circle
	//color should be redder as this guy approaches death
	const float redness = 1.0f - m_flHealthRatio;
	//as the ring gets redder it should become less green, or we'll end up yellow when near death
	const float greenness = 1.0f - redness;

	//make quad data
	FXQuadData_t data;
	//only using start values
	data.SetAlpha( 1.0, 0 );
	data.SetScale( 4 * 10.0f, 0 );
	data.SetMaterial( "effects/zombie_select" );
	data.SetNormal( Vector(0,0,1) );
	data.SetOrigin( GetAbsOrigin() + Vector(0,0,3) );
	data.SetColor( redness*brightness, greenness*brightness, 0 );
	data.SetScaleBias( 0 );
	data.SetAlphaBias( 0 );
	data.SetYaw( 0, 0 );

	//TGB: create selectcircle
	m_SelectCircle = new CFXCharSprite( data );
}

void C_BaseCombatCharacter::ZMSelectionEffects()
{

		//TGB FIXME: should rework this into a better suited 'permanent' material approach, instead of constantly redrawing

		C_BasePlayer *local = C_BasePlayer::GetLocalPlayer(); //LAWYER:  This probably could be done in a much cleaner way...
		
		//just don't let anyone but ZM even go below this
		if (!local || ( local && local->GetTeamNumber() != 3 ) )
			return;

		//color32 oldColor;
		Vector health_color(1.0f, 1.0f, 1.0f);
		const float brightness = clamp(zm_healthcircle_brightness.GetFloat()*2, 0.0, 1.0);

		//adjusted position
		const Vector liftedorigin = GetAbsOrigin() + Vector(0,0,3);

		//TGB: this defines a colour that moves from green to red depending on character's health
		//used in selection material and dedicated health circle

		const float health_ratio = m_flHealthRatio;
		//color should be redder as this guy approaches death
		const float redness = 1.0f - health_ratio;
		//as the ring gets redder it should become less green, or we'll end up yellow when near death
		const float greenness = 1.0f - redness;

		health_color.Init(redness, greenness, 0);

		//LAWYER: Selection sparkles!
		if (this->m_bConqSelected == true)
		{
			if (m_SelectCircle)
			{
				m_SelectCircle->Update(liftedorigin, health_color*brightness, true);
			}
		}
		else
		{
			if (m_SelectCircle)
			{
				m_SelectCircle->Update(liftedorigin, health_color, false); //removed brightness multiply to simplify this call seeing as we won't draw anyway
			}
		}

		if (IsPlayer()) //LAWYER:  Players glow!
		{
			//TGB: this is a bit too spammy, throwing "no room" errors when looking down upon 15 players
			//Draw material under the players for the ZM
			/*FX_AddQuad( liftedorigin,
					Vector(0,0,1),
					38.0f,
					25.0f,
					1.0f, 
					1.0f, //pumped
					0.0f,
					0.5f,
					random->RandomInt( 0, 360 ), 
					0,
					vFullbright, 
					0.05f, 
					"effects/yellowflare",
					FXQUAD_BIAS_ALPHA);*/

			//TGB: rather than going through a helper function, just construct fxdata directly
			FXQuadData_t data;

			//Setup the data
			data.SetAlpha( 1.0f, 0.1f );
			data.SetScale( 38.0f, 28.0f );
			data.SetFlags( FXQUAD_BIAS_SCALE );
			data.SetMaterial( "effects/yellowflare" );
			data.SetNormal( Vector(0,0,1) );
			data.SetOrigin( liftedorigin );
			data.SetLifeTime( 0.05f );
			data.SetColor( 1.0f, 1.0f, 1.0f );
			data.SetScaleBias( 0.5f );
			data.SetYaw( random->RandomInt( 0, 360 ), 0 );

			FX_AddQuad(data);
	
		}

		//TGB: reworked health circly bits
		if (ShouldHaveHealthCircle()) //LAWYER:  Check whether we need to really display it!
		{
			if ( m_HealthCircle )
			{
				m_HealthCircle->Update(liftedorigin, health_color*brightness);
			}
			else
			{
				//seriously, we want one
				DevMsg("Second-chance healthcircle creation call.\n");
				CreateHealthCircle();
			}
		}
	

}

IMPLEMENT_CLIENTCLASS(C_BaseCombatCharacter, DT_BaseCombatCharacter, CBaseCombatCharacter);

// Only send active weapon index to local player
BEGIN_RECV_TABLE_NOBASE( C_BaseCombatCharacter, DT_BCCLocalPlayerExclusive )
	RecvPropTime( RECVINFO( m_flNextAttack ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_hMyWeapons), RecvPropEHandle( RECVINFO( m_hMyWeapons[0] ) ) ),
END_RECV_TABLE();


BEGIN_RECV_TABLE(C_BaseCombatCharacter, DT_BaseCombatCharacter)
	RecvPropDataTable( "bcc_localdata", 0, 0, &REFERENCE_RECV_TABLE(DT_BCCLocalPlayerExclusive) ),
	RecvPropEHandle( RECVINFO( m_hActiveWeapon ) ),
	RecvPropBool(RECVINFO(m_bConqSelected)), //LAWYER:  Necessary for selection overlays
	RecvPropFloat(RECVINFO(m_flHealthRatio)), //TGB: needed to make GetHealth work clientside
#ifdef TF2_CLIENT_DLL
	RecvPropInt( RECVINFO( m_iPowerups ) ),
#endif

END_RECV_TABLE()


BEGIN_PREDICTION_DATA( C_BaseCombatCharacter )

	DEFINE_PRED_ARRAY( m_iAmmo, FIELD_INTEGER,  MAX_AMMO_TYPES, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flNextAttack, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_hActiveWeapon, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_ARRAY( m_hMyWeapons, FIELD_EHANDLE, MAX_WEAPONS, FTYPEDESC_INSENDTABLE ),

END_PREDICTION_DATA()
