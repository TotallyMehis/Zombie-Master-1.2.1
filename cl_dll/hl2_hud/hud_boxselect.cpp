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
// Purpose: 
//
//=============================================================================//

//#include "hudelement.h"
//#include <vgui_controls/Panel.h>
//#include "hud_macros.h"

#include "cbase.h"
#include "hud.h"
#include "hud_boxselect.h"
#include "iclientmode.h"
#include "view.h"
#include "vgui_controls/controls.h"
#include "vgui/ISurface.h"
#include "IVRenderView.h"
#include "iclientmode.h"
#include "iinput.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



using namespace vgui;

 DECLARE_HUDELEMENT( CHudBoxSelect );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudBoxSelect::CHudBoxSelect( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudBoxSelect" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
	m_bDrawBox = false;
	//m_iDragMouseX = 0;
	//m_iDragMouseY = 0;

	/*
	const int CORNER_SIZE = 8;
	const int BOTTOMRIGHTSIZE = 18;
	const int CAPTION_SIZE = 23;
	SetBounds(0 - CORNER_SIZE,
		0 - CAPTION_SIZE,
		ScreenWidth() + CORNER_SIZE + BOTTOMRIGHTSIZE,
		ScreenHeight() + CAPTION_SIZE + BOTTOMRIGHTSIZE);
	*/

	SetPos(0,0);
	SetSize(ScreenWidth(), ScreenHeight());

	SetPaintBackgroundEnabled( false );
	//SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBoxSelect::Init()
{
	//m_iDragMouseX = 0;
	//m_iDragMouseY = 0;
}

//-----------------------------------------------------------------------------
// Purpose: draws the box
//-----------------------------------------------------------------------------
void CHudBoxSelect::Paint()
{
	//test rect
	//vgui::surface()->DrawSetColor(  255, 0, 0, 155 ); //RGBA
	//vgui::surface()->DrawFilledRect( 0, 0, 20, 20 ); //x0,y0,x1,y1

	int mousex, mousey;
	::input->GetFullscreenMousePos( &mousex, &mousey );

	//needed to check if we're ZM
	CBasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer(); 
	if (!pPlayer) return;

	//only draw if we're ZM, this is to fix the boxselect from staying on screen when no longer being ZM
	//this would occur if a ZM was boxselecting during a roundrestart or similar teamswitch
	if (m_bDrawBox && pPlayer->GetTeamNumber() == 3)
	{
		int topleft_x, topleft_y, botright_x, botright_y;

		if ( m_iDragMouseX < mousex) 
		{
			topleft_x = m_iDragMouseX ;
			botright_x = mousex;
		}
		else
		{
			topleft_x = mousex;
			botright_x = m_iDragMouseX;
		}

		if ( m_iDragMouseY < mousey) 
		{
			topleft_y = m_iDragMouseY ;
			botright_y = mousey;
		}
		else
		{
			topleft_y = mousey;
			botright_y = m_iDragMouseY;
		}
		

		//DevMsg("\nDrawing: %i, mx: %i my: %i, dx: %i dy: %i", (int)m_bDrawBox, mousex, mousey, m_iDragMouseX, m_iDragMouseY );
		vgui::surface()->DrawSetColor(  150, 0, 0, 40 ); //RGBA
		vgui::surface()->DrawFilledRect( topleft_x, topleft_y, botright_x, botright_y ); //x0,y0,x1,y1
		vgui::surface()->DrawSetColor(  200, 0, 0, 150 ); //RGBA
		vgui::surface()->DrawOutlinedRect( topleft_x, topleft_y, botright_x, botright_y ); //x0,y0,x1,y1
	}

	//TGB: hijacking this to draw a line formation line on the hud
	if (m_bDrawLine && pPlayer->GetTeamNumber() == 3)
	{
		vgui::surface()->DrawSetColor(  200, 0, 0, 150 );
		vgui::surface()->DrawLine( m_iDragMouseX, m_iDragMouseY, mousex, mousey );
	}

}


