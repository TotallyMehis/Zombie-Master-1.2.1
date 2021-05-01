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


#include "cbase.h"
#include "hud.h"
#include "hud_tooltip.h"
#include "iclientmode.h"
#include "view.h"
#include "vgui_controls/controls.h"
#include "vgui/ISurface.h"
#include "IVRenderView.h"
#include "iclientmode.h"
#include "iinput.h"
#include "vgui_controls/TextImage.h"
//#include <vgui/VGUI.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

 DECLARE_HUDELEMENT( CHudToolTip );

#define PAD_HOR 5
#define PAD_VER 3
#define DRAWWIDTH 300

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudToolTip::CHudToolTip( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudToolTip" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
	SetHiddenBits( HIDEHUD_TOOLTIP );
	SetProportional( false );
	//SetAutoResize( vgui::Panel::AUTORESIZE_DOWN );
	SetAutoResize( PIN_TOPLEFT, AUTORESIZE_DOWN, 0, 0, 0, 0  );
	SetVisible(false);
	SetSize(1,1);
	SetAlpha(0);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudToolTip::Init()
{
	m_szTooltip = NULL;
	
	m_Text = vgui::SETUP_PANEL(new vgui::Label(this,"ToolTipText",""));
	m_Text->SetParent(this);
	m_Text->SetProportional( false );
	m_Text->SetPos( PAD_HOR, PAD_VER );
	m_Text->SetSize( GetWide() - PAD_HOR, GetTall() - PAD_VER );
	m_Text->SetContentAlignment( vgui::Label::a_northwest );
//	m_Text->SetAutoResize( vgui::Label::AUTORESIZE_DOWN );
	m_Text->SetText("");

	SetAlpha(0);

	//TGBNOTE: this part is weird. Breakpoints appear to be ignored, devmsges don't appear, etc.
}

//-----------------------------------------------------------------------------
// Purpose: TGB: wrapper to set a tooltip that will clear itself after X seconds
//-----------------------------------------------------------------------------
void CHudToolTip::SetTooltipTimed( const char* szToolTip, vgui::VPANEL _owner, const double duration  )
{
	//no need to even set it if the endtime is in the past
	if ( duration > 0 )
	{
		m_bTimed = true;
		m_iEndTime = gpGlobals->curtime + duration;
		SetTooltip( szToolTip, _owner );
		//DevMsg("Timed tooltip set that will clear in %i\n", duration );
	}
	else
		Warning("Timed tooltip set with 0 duration! Discarded.\n");
}

//-----------------------------------------------------------------------------
// Purpose: sets a tooltip AND unhides it
//-----------------------------------------------------------------------------
void CHudToolTip::SetTooltip( const char* szToolTip, vgui::VPANEL _owner )
{
	//TGB: remember who set us
	m_Owner = _owner;

	//DevMsg("Called SetText!\n");
	m_Text->SetText(szToolTip);

	//TGB: got some wrapping in here

	//seems we need to do some manual scaling
	const float ratio = (float)ScreenWidth() / 640.0f;

	TextImage *tiptext = m_Text->GetTextImage();
	tiptext->SetWrap( true );
	tiptext->SetDrawWidth( DRAWWIDTH * ratio );
	tiptext->RecalculateNewLinePositions();
	tiptext->ResizeImageToContent();

	int tw, th;
	tiptext->GetSize( tw, th );			//get sizes
	//DevMsg("tooltiptext is %i by %i\n", tw, th);

	//manually resize height of it all to adjust to wrapped text
	const int newHeight = th + 8;
	m_Text->SetTall( newHeight );			//set label
	SetTall( newHeight );		//set element

	//manually resize width and position centrally
	const int newWidth = tw + 5;
	m_Text->SetWide( newWidth );
	SetWide( newWidth + PAD_HOR );

	//half of screen width minus half of element width
	//so if we're 200 wide our left corner is positioned at 220
	const int newX = (ScreenWidth() / 2) - ( GetWide() / 2 ) ;
	//TGB: also set our own height now that we don't proportionalise
	const int newY = ScreenHeight() - newHeight - 30;
	//int px, py;
	//GetPos( px, py );
	SetPos( newX, newY );

	//make sure we can be seen
	SetVisible( true );
	//DevMsg("nW = %i; nH = %i; nX = %i\n", newWidth, newHeight, newX );

	//DevMsg("Tooltip set to %s\n", szToolTip);

	//TGB: always show a tooltip we've set, as it's only logical. Special cases can do their own Hide(true)
	Hide( false );
}

void CHudToolTip::ClearTooltip()
{
	m_Text->SetText("");
	m_Owner = NULL;
	m_bTimed = false;
}

void CHudToolTip::Hide( bool shouldHide )
{
	CBasePlayer *pPlayer = CBasePlayer::GetLocalPlayer();
	if ( !pPlayer ) 
	{
		DevMsg("TOOLTIP: can't find local player\n");
		return;
	}

	//TGB: for some reason hidehud doesn't appear to work, it seems to get reset every couple of ticks
	//TGB: using SetAlpha to hide it now, very hacky but appears to work for now
	if (shouldHide)
	{
		pPlayer->m_Local.m_iHideHUD |= HIDEHUD_TOOLTIP;
		//TGB: force off
		SetVisible(false);
		//DevMsg("Tooltip hidden in Hide()\n");
		m_bForceVisible = false;

		//hack it up
		SetAlpha(0);
	}
	else
	{
		pPlayer->m_Local.m_iHideHUD &= ~HIDEHUD_TOOLTIP;
		//DevMsg("Tooltip UNhidden in Hide()\n");
		//TGB: force off
		SetVisible(true);
		m_bForceVisible = true;
		

		SetAlpha(255);
	}

	
}

//-----------------------------------------------------------------------------
// Purpose: draws the box
//-----------------------------------------------------------------------------
void CHudToolTip::Paint()
{
}

//-----------------------------------------------------------------------------
// Purpose: clears if time has passed
//-----------------------------------------------------------------------------
void CHudToolTip::OnThink()
{
	if ( m_bTimed && m_iEndTime < gpGlobals->curtime )
	{
		ClearTooltip();
		
		Hide( true ); //TGB: blah
		
		//DevMsg("Timed tooltip cleared and hidden\n");
		
		//m_bTimed is false'd in ClearTooltip to provide for premature clearing
	}

	//if (m_bForceVisible)
	//{
	//	CBasePlayer *pPlayer = CBasePlayer::GetLocalPlayer();
	//	if ( pPlayer )
	//		pPlayer->m_Local.m_iHideHUD &= ~HIDEHUD_TOOLTIP;
	//}
}
