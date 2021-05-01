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
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"

#include <vgui_controls/Panel.h>
#include <vgui_controls/Frame.h>
#include <vgui/IScheme.h>
#include <vgui_controls/Label.h>
#include "iclientmode.h"

#include "shareddefs.h" //For Observer functions


using namespace vgui;

//TGB: needed for mouse traces
#include "iinput.h"
#include "view.h"

#include "c_baseplayer.h"
#include "hud_name.h"
//#include "c_baseplayer.h" //TGB: two seems a bit much

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//qck: This is mostly from the Imperio tutorial, so thanks to him. 
DECLARE_HUDELEMENT( CHudName );

CHudName::CHudName( const char *pElementName ) : CHudElement( pElementName ), vgui::Panel( NULL, "HudName" )
{
	scheme = vgui::scheme()->LoadSchemeFromFile("resource/ClientScheme.res", "ClientScheme");
	SetScheme(scheme);	
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );	
	SetHiddenBits( HIDEHUD_PLAYERDEAD | HIDEHUD_CROSSHAIR );
	SetBgColor(Color(0,0,0,0));	
	SetAutoResize( PIN_BOTTOMLEFT, AUTORESIZE_RIGHT, 0, 0, 0, 0  );
	//SetPinCorner( PIN_BOTTOMLEFT );
	SetPaintBackgroundEnabled(false);

	m_pNameLabel = vgui::SETUP_PANEL(new vgui::Label(this,"NameLabel",""));		
	m_pNameLabel->SetScheme(scheme);	
	m_pNameLabel->SetFont(vgui::scheme()->GetIScheme(scheme)->GetFont("TargetID", true));	
	m_pNameLabel->SetKeyBoardInputEnabled(false);	
	m_pNameLabel->SetMouseInputEnabled(false);	
	m_pNameLabel->SetPos(0,0);	
	m_pNameLabel->SetSize(GetWide(),GetTall());	
//	m_pNameLabel->SetAutoResize(AUTORESIZE_RIGHT); 
	m_pNameLabel->SetPaintBackgroundEnabled( false );
	m_pNameLabel->SetPaintBorderEnabled( false );	
	m_pNameLabel->SetContentAlignment( vgui::Label::a_center );	
	m_pNameLabel->SetBgColor(Color(0,0,0,0));	
} 

void CHudName::Init( void )
{
	Reset();
}

void CHudName::Reset( void )
{
	m_pName = NULL;	
	m_pNameLabel->SetVisible( false );
	m_flLastTrace = 0;
}

void CHudName::VidInit( void )
{
	Reset();
}

void CHudName::Paint( void )
{
	if (m_pName && m_pName[0])	
	{
		//DevMsg("Hudname has a name to draw\n");
		
		char text[512];

		sprintf(text,"%s",m_pName);
		m_pNameLabel->SetFgColor( Color(255,255,255,255) );

		m_pNameLabel->SetText(text);	
		m_pNameLabel->SetVisible(true);
		m_pNameLabel->SizeToContents();
		//size panel to label
		SetWide(m_pNameLabel->GetWide());
	}
	else
	{
		//DevMsg("Hudname invisible \n");
		m_pNameLabel->SetText((char*)0);	
		m_pNameLabel->SetFgColor(Color(0,0,0,0));	
		m_pNameLabel->SetVisible(false);	
	}

}

void CHudName::OnThink(void)
{
	
	C_BasePlayer* player = C_BasePlayer::GetLocalPlayer();

	if (!player)
		return;

	//TGB: the ZM doesn't need this anymore with the tooltips now doing IDing
	if (player->IsZM())
		return;
	
	//TGB: don't trace every frame, it's inefficient and makes the names blink in and out
	const int NAMETRACE_DELAY = 1.0f;
	if ( gpGlobals->curtime <= ( m_flLastTrace + NAMETRACE_DELAY ) )
	{
		return;
	}
	else
		m_flLastTrace = gpGlobals->curtime;

	trace_t tr;
	
	//do trace in different ways depending on team
	if ( false && player->IsZM() )
	{
		//TGB: the ZM doesn't need this anymore with the tooltips now doing IDing
		/*int mousex, mousey;
		::input->GetFullscreenMousePos( &mousex, &mousey );

		//TGB: code grabbed from TraceScreenToWorld, compressed a bit
		const float fov = player->GetFOV();

		float aspect = (float)ScreenWidth() / (float)ScreenHeight();
		aspect = ( 4.0f / 3.0f ) / aspect;

		const float c_x = ScreenWidth() / 2;
		const float c_y = ScreenHeight() / 2;

		const float dx = ((float)mousex - c_x) / aspect;
		const float dy = (c_y - (float)mousey) / aspect;

		const float dist = c_x / tan( M_PI * fov / 360.0 );

		Vector vecPickingRay = (MainViewForward() * dist) + (MainViewRight() * dx) + (MainViewUp() * dy);
		VectorNormalize( vecPickingRay );

		UTIL_TraceLine( MainViewOrigin(), MainViewOrigin() + vecPickingRay * 8192, MASK_SHOT, player, COLLISION_GROUP_NONE, &tr);
		*/
	}

	else if (player->GetTeamNumber() == 1)
	{
	
		//LAWYER:  Identify doesn't happen for 3rd/1st person spectators!
		if (player->GetObserverMode() == OBS_MODE_ROAMING)
		{
			Vector forward;
			AngleVectors(player->EyeAngles(),&forward);	
			UTIL_TraceLine(player->Weapon_ShootPosition(),player->Weapon_ShootPosition() + (forward *  8192), MASK_SHOT , 0 , &tr);	
		}
		else
		{
			//HACKHACKHACK - quick workaround to stymie the crash
			Reset();
			return;
		}


	}

	else //human team
	{
		Vector forward;
		AngleVectors(player->EyeAngles(),&forward);	
		UTIL_TraceLine(player->Weapon_ShootPosition(),player->Weapon_ShootPosition() + (forward *  8192), MASK_SHOT , 0 , &tr);	
	}
	
	//process trace results in same way for ZM and others
	if( tr.m_pEnt )
	{
		//TGB: this makes no sense when we have IsPlayer
		//if(Q_strcmp(tr.m_pEnt->GetClassname(), "player") == 0)
		if (tr.m_pEnt->IsPlayer())
		{
			//TGB: uglywugly
			//m_pPlayer = (C_BasePlayer*)tr.m_pEnt;	
			m_pPlayer = ToBasePlayer(tr.m_pEnt);

			if(m_pPlayer && m_pPlayer->GetTeamNumber() == 2 && m_pPlayer->IsAlive())
			{
				//DevMsg("Found a player\n");
				if(!(Q_strcmp(player->GetPlayerName(), m_pPlayer->GetPlayerName()) == 0)) //qck: This has to be done due to really odd spawnpoint conflicts which I do not understand. Forgive the ugly. 
					m_pName = m_pPlayer->GetPlayerName();
			}
		}
		else
		{
			Reset();	//We hit nothing, so reset the name and player pointer, so the text won't be displayed.
		}
	}

}