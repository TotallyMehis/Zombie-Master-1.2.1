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
// $NoKeywords: $
//
//=============================================================================//
//
//
// implementation of CHudRes class
//
#include "cbase.h"
#include "hud.h"
#include "hud_macros.h"
#include "view.h"

#include "iclientmode.h"

#include <KeyValues.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/Label.h>
#include <vgui/ILocalize.h>

#include <vgui_BitmapImage.h>
#include <vgui_BitmapPanel.h>

using namespace vgui;

#include "hudelement.h"
#include "hud_numericdisplay.h"

//for zombiemax cvar
#include "buildmenu.h"

#include "ConVar.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define INIT_RES -1


//TGB: icon stuff
enum Icon { 
	ICON_RES,	//resource counter
	ICON_ZEDS,	//zombiecount

	NUM_ICONS,	//needs to be last! holds number of icons in enum
};

//TGB @ 05-06-2006: attempt to turn this into a more multipurpose info panel
//-----------------------------------------------------------------------------
// Purpose: ZM info panel
//-----------------------------------------------------------------------------
class CHudRes : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudRes, vgui::Panel );

public:
	CHudRes( const char *pElementName );
	~CHudRes();
	virtual void Init( void );
	virtual void VidInit( void );
	virtual void Reset( void );
	virtual void OnThink();

	void Paint();

	void Reposition(); //TGB: position the panel in the bottom left manually (ie. without panel proportionality turned on)

private:
	int		m_iResCount;
	int		m_iResIncrements;
	int		m_iZombiePopCount; 
	int		m_iZombieSelected; //qck: Maybe we can show number of zombies selected / number of total zombies?

	vgui::Label *m_pResLabel;
	vgui::HScheme scheme;
	vgui::HFont m_hLargeFont;
	vgui::HFont m_hMediumFont;

	CHudTexture *m_pIcons[NUM_ICONS];

	void LoadIcons();
	void PaintScaledNumbers( int numbers, int x, int y, bool large );
	void PaintScaledText ( wchar_t *symbol, int x, int y, bool large );
};	

DECLARE_HUDELEMENT( CHudRes );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudRes::CHudRes( const char *pElementName ) : CHudElement( pElementName ),  vgui::Panel(NULL, "HudRes")
{
	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );

	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	scheme = vgui::scheme()->LoadSchemeFromFile("resource/ClientScheme.res", "ClientScheme");
	m_hLargeFont = vgui::scheme()->GetIScheme(scheme)->GetFont("Trebuchet30", false);
	m_hMediumFont = vgui::scheme()->GetIScheme(scheme)->GetFont("Trebuchet20", false);
	SetFgColor(Color(255,255,255,255));
	
	//TGB: no resizing huddage
	SetProportional(false);

	/*
	m_pResLabel = vgui::SETUP_PANEL(new vgui::Label(this,"ResLabel",""));		
	m_pResLabel->SetScheme(scheme);	
	m_pResLabel->SetFont(m_hNumberFont);	
	m_pResLabel->SetPos(35,5);	
	m_pResLabel->SetSize(GetWide(),GetTall());	//hrm
	//m_pResLabel->SetPaintBackgroundEnabled( false );
	//m_pResLabel->SetPaintBorderEnabled( false );	
	//m_pResLabel->SetBgColor(Color(0,255,0,255));
	//m_pResLabel->SetFgColor(Color(255,255,255,255));
	m_pResLabel->SetProportional(true);
	*/
	Reposition();
}

//deconstructor, remove icons
CHudRes::~CHudRes ()
{
	DevMsg("CHudRes: removing icons.\n");

	for (int i=0; i<NUM_ICONS; i++ )
	{
		if (m_pIcons[i] != NULL)
		{
			delete m_pIcons[i];
			m_pIcons[i] = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Load all used icon images
//-----------------------------------------------------------------------------
void CHudRes::LoadIcons ()
{
	DevMsg("CHudRes: loading icons.\n");

	//load resources icon
	m_pIcons[ICON_RES] = gHUD.GetIcon( "icon_resources" );
	//load icon_figures
	m_pIcons[ICON_ZEDS] = gHUD.GetIcon( "icon_figures" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudRes::Init()
{
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudRes::Reset()
{
//	C_BasePlayer *local = C_BasePlayer::GetLocalPlayer();
	m_iResCount		= INIT_RES;
	m_iResIncrements = 0;
	m_iZombiePopCount	= 0;
	m_iZombieSelected = 0;

	Reposition();

	//wchar_t *tempString = vgui::localize()->Find("#Valve_Hud_HEALTH");
	//if (local)
	//{
	//	if (tempString)
	//	{
	//		SetLabelText(tempString);
	//	}
	//	else
	//	{
	//		SetLabelText(L"RES.");
	//	}
	//}
	//SetDisplayValue(m_iResCount);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudRes::VidInit()
{
	Reset();
	LoadIcons();
}

//-----------------------------------------------------------------------------
// Purpose: TGB: position panel in bottom left.
//-----------------------------------------------------------------------------
void CHudRes::Reposition()
{
	//this function is not usable if proportionality is turned on, because then we'll be positioned by vgui already
	if (IsProportional())
		return;

	//TGB:TODO: overrides for these in cvars to allow users to customize

	//CONSTANT JUGGLING AHOY

	const int h = GetTall();
	//const int w = GetWide();
	
	const int sH = ScreenHeight();
	//const int sW = ScreenWidth();

	const int padding = 0;//5
	//positioning on the left is easy
	const int goalX = padding;
	//we can use the screenheight and panel height to determine what the Y should be to place it at the bottom of screen
	const int goalY = sH - h - padding;

	//apply
	SetPos(goalX, goalY);
	
}

//-----------------------------------------------------------------------------
// Purpose: //LAWYER:  To satisfy everyone, we could stick the resource function in here, too.
//-----------------------------------------------------------------------------
void CHudRes::OnThink()
{
	C_BasePlayer *local = C_BasePlayer::GetLocalPlayer();
	
	//TGB: it crashed here when connecting to a server because local did not exist

	//do resources
	if (local)
	{
		bool visible = local->IsZM();
		SetVisible( visible );

		if ( visible )
		{
			//SetLabelText(L"RES."); //RESOURCES	
			
			int newRes = max( local->m_iZombiePool, 0 );

			//simplistic representation of res increase/decrease trends
			int newInc = newRes - m_iResCount;
			//will be negative when sauce is deducted for something
			//will be 0 in all ticks where sauce is not incremented
			//so prevent that from appearing
			if ( newInc > 0 )
				m_iResIncrements = newInc; 

			m_iResCount = newRes;

			int newZeds = max( local->m_iZombiePopCount, 0);
			int selectedZeds = local->m_iZombieSelected;

			//DevMsg("Zombies: %i\n", selectedZeds);

			m_iZombieSelected = selectedZeds; 
			m_iZombiePopCount = newZeds;
		}
	}
}

//passthrough function
void CHudRes::PaintScaledNumbers( int numbers, int x, int y, bool large )
{
	wchar_t text[20];
	swprintf(text, L"%i", numbers);
	PaintScaledText( text, x, y, large );
}

//TGB: adaption of hud_numericdisplay.cpp paintlabel() func
//scales position to resolution
void CHudRes::PaintScaledText ( wchar_t *text, int x, int y, bool large )
{
	//TGB (08-10-2006): UNDONE: no more scaling
	//const float flVerScale = (float)ScreenHeight() / 480.0;
	const float flVerScale = 1;
	x = (int)(x * flVerScale);
	y = (int)(y * flVerScale);

	if ( large )
		surface()->DrawSetTextFont(m_hLargeFont);
	else
		surface()->DrawSetTextFont(m_hMediumFont);

	surface()->DrawSetTextColor(GetFgColor());
	surface()->DrawSetTextPos(x, y);

	for (wchar_t *wch = text; *wch != 0; wch++)
	{
		surface()->DrawUnicodeChar(*wch);
	}

}

void CHudRes::Paint (void)
{
	//TGB (08-10-2006): UNDONE: no more scaling
	//TGB:
	//scale manually for consistency
	//640x480 is the base res for vgui
	//work with just vertical scale for now
	//would be nice to calc scale just once instead of every paint
	//const float flVerScale = (float)ScreenHeight() / 480.0;
	const float flVerScale = 1;

	//DRAW RES TEXT
	PaintScaledNumbers( m_iResCount, 45, 5, true );

	//DRAW INCREMENTS TEXT
	//do the sprintf here because we want to draw a + sign
	wchar_t inc_text[10];
	wchar_t spaces[25]; //qck edit

	swprintf(inc_text, L"+ %i", m_iResIncrements);
	PaintScaledText( inc_text, 70, 30, false );

	//DRAW ZEDCOUNT TEXT
	//PaintScaledNumbers( m_iZombieSelected, 30, 50, false );
	//PaintScaledNumbers( m_iZombieCount, 60, 50, false); //qck edit
	//TGB: added custom printf to avoid overlap of numbers on slash
	//swprintf(spaces, L"%i / %i / %i", m_iZombieSelected, m_iZombiePopCount , zm_zombiemax.GetInt()); //qck edit

	//TGB: added selected zombie indicator back in by popular demand
	if (m_iZombieSelected > 0)
	{
		//only print it if there are zombies selected
		//hopefully that will help in communicating to the player what the indicator means
		swprintf(spaces, L"%i / %i (%i)", m_iZombiePopCount , zm_zombiemax.GetInt(), m_iZombieSelected);
	}
	else
	{
		swprintf(spaces, L"%i / %i", m_iZombiePopCount , zm_zombiemax.GetInt());
	}
	
	

	PaintScaledText( spaces, 35, 58, false ); //qck edit

	//DRAW ICONS
	int x = 2;
	int y = 5;
	for (int i=0; i < NUM_ICONS; i++ )
	{
		if ( m_pIcons[i] )
		{
			//when we get better/larger icons than the current 32x32 ones, replace Width/Height calls with hardcoded 32 size
			const int w = (int)(m_pIcons[i]->Width() * flVerScale);
			const int h = (int)(m_pIcons[i]->Height() * flVerScale);

			//can't avoid some icon-specific positioning
			int indent = 2;
			switch ( i )
			{
				case ICON_ZEDS:
					indent = 10;
					break;
				default:
					break;
			}

			m_pIcons[i]->DrawSelf( x + indent, y, w, h, Color(255, 255, 255, 255) );

			//for now just assume we move down
			y += h + (25 * flVerScale);
		}
	}


}
