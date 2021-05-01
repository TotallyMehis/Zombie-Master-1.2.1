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
#include <cdll_client_int.h>

#include "zm_controlpanel.h"

#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <KeyValues.h>
#include <vgui_controls/ImageList.h>
#include <FileSystem.h>

#include <vgui_controls/RichText.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/HTML.h>

#include "vgui_BitmapButton.h" //TGB test

#include "IGameUIFuncs.h" // for key bindings
#include <igameresources.h>
extern IGameUIFuncs *gameuifuncs; // for key binding details

#include <cl_dll/iviewport.h>

#include <stdlib.h> // MAX_PATH define
#include <stdio.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

void UpdateCursorState();
// void DuckMessage(const char *str);



//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CZM_ControlPanel::CZM_ControlPanel(IViewPort *pViewPort) : Frame(NULL, PANEL_ZMCP )
{
	m_pViewPort = pViewPort;
	m_iJumpKey = -1; // this is looked up in Activate()
	m_iScoreBoardKey = -1; // this is looked up in Activate()

	// initialize dialog
	SetTitle("OBSOLETE! Control Panel", true);

	// load the new scheme early!!
	scheme = vgui::scheme()->LoadSchemeFromFile("resource/ZombieMaster.res", "ZombieMaster");
	SetScheme(scheme);

	//No crazy large fonts please
	m_hLargeFont = vgui::scheme()->GetIScheme(scheme)->GetFont("Trebuchet18", true);
	m_hMediumFont = vgui::scheme()->GetIScheme(scheme)->GetFont("Trebuchet16", true);

	SetMoveable(true); //qck: I find this to be quite helpful 

	SetSizeable(false);



	SetTitleBarVisible( false );
	SetProportional(true);

	LoadControlSettings("Resource/UI/ControlPanel.res");
	InvalidateLayout();

}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CZM_ControlPanel::~CZM_ControlPanel()
{
	//DevMsg("CZM_ControlPanel: removing buttons.\n");

	//for (int i=0; i<NUM_BUTTONS; i++ )
	//{
	//	if (m_pButtons[i] != NULL)
	//	{
	//		delete m_pButtons[i];
	//		m_pButtons[i] = NULL;
	//	}
	//}
}

//-----------------------------------------------------------------------------
// Purpose: sets the text color of the map description field
//-----------------------------------------------------------------------------
void CZM_ControlPanel::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	pScheme->GetFont( "Trebuchet18" );
}

void CZM_ControlPanel::Update()
{

}


//-----------------------------------------------------------------------------
// Purpose: shows the build menu
//-----------------------------------------------------------------------------
void CZM_ControlPanel::ShowPanel(bool bShow)
{
//	CBasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( BaseClass::IsVisible() == bShow )
		return;

	
	if ( bShow )
	{
		Activate();

		SetMouseInputEnabled( true ); 

		// get key bindings if shown

		if( m_iJumpKey < 0 ) // you need to lookup the jump key AFTER the engine has loaded
		{
			m_iJumpKey = gameuifuncs->GetEngineKeyCodeForBind( "jump" ); 
		}

		if ( m_iScoreBoardKey < 0 ) 
		{
			m_iScoreBoardKey = gameuifuncs->GetEngineKeyCodeForBind( "showscores" );
		}
	}
	else
	{
		SetVisible( false );
		SetMouseInputEnabled( false );
	}

	m_pViewPort->ShowBackGround( bShow );
}


void CZM_ControlPanel::OnKeyCodePressed(KeyCode code)
{
		BaseClass::OnKeyCodePressed( code );
}

//LAWYER: (BUILD MENU) Define the pressing of buttons
void CZM_ControlPanel::OnCommand( const char *command )
{
  if ( Q_stricmp( command, "vguicancel" ) )
  {
	  engine->ClientCmd( const_cast<char *>( command ) );
  }
  //LAWYER:  Checks for 
	Close();
	gViewPortInterface->ShowBackGround( false );
  
  BaseClass::OnCommand(command);
}

/*
void CZM_ControlPanel::Paint()
{
	vgui::surface()->DrawSetColor(  255, 0, 0, 255 ); //RGBA
	vgui::surface()->DrawFilledRect( 0, 0, 20, 20 ); //x0,y0,x1,y1
}
*/

//void CZM_ControlPanel::LoadButtons ()
//{
//	color32 white = { 255, 255, 255, 255 };
//	color32 grey = { 155, 155, 155, 255 };
//	color32 red = { 200, 55, 55, 255 };
//
//	DevMsg("CZM_ControlPanel: creating buttons.\n");
//
//	m_pButtons[BUTTON_TEST] = new CBitmapButton( this, "TestButton", "" ); 
//
//	m_pButtons[BUTTON_TEST]->SetImage( CBitmapButton::BUTTON_ENABLED, "VGUI/miniskull", white );
//	m_pButtons[BUTTON_TEST]->SetImage( CBitmapButton::BUTTON_ENABLED_MOUSE_OVER, "VGUI/miniskull", red );
//	m_pButtons[BUTTON_TEST]->SetImage( CBitmapButton::BUTTON_PRESSED, "VGUI/miniskull", grey );
//	m_pButtons[BUTTON_TEST]->SetProportional(true);
//	m_pButtons[BUTTON_TEST]->SetPos( m_iButtonX, m_iButtonY );
//	m_pButtons[BUTTON_TEST]->SetSize( m_iButtonSize, m_iButtonSize );
//}

