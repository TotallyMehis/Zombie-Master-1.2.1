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
// Purpose: STARTMENU - That panel that makes players have to choose at the beginning of the game
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include <cdll_client_int.h>
 
#include "startmenu.h"

#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <KeyValues.h>
#include <vgui_controls/ImageList.h>
#include <FileSystem.h>

#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/CheckButton.h>

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


ConVar zm_participate_saved( "zm_participate_saved", "-1", FCVAR_ARCHIVE, "If not -1, this value is what the zm_participate level is set to automatically on joining a server." );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CStartMenu::CStartMenu(IViewPort *pViewPort) : Frame(NULL, PANEL_START )
{
	m_pViewPort = pViewPort;
	m_iJumpKey = -1; // this is looked up in Activate()
	m_iScoreBoardKey = -1; // this is looked up in Activate()

	// initialize dialog
	SetTitle("", true);

	// load the new scheme early!!
	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFile("resource/ZombieMaster.res", "ZombieMaster");
	SetScheme(scheme);
	SetSizeable(false);

	// hide the system buttons
	SetTitleBarVisible( false );
	SetProportional(false); //TGB: no more proportional

	// info window about this map
//	m_pMapInfo = new RichText( this, "MapInfo" );
	SetMoveable(true);

	remember = new vgui::CheckButton(this, "remember", "Participate level saving toggle");

	LoadControlSettings("Resource/UI/StartMenu.res");
	InvalidateLayout();

	should_save = false;

}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CStartMenu::~CStartMenu()
{
}

//-----------------------------------------------------------------------------
// Purpose: sets the text color of the map description field
//-----------------------------------------------------------------------------
void CStartMenu::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
}



//-----------------------------------------------------------------------------
// Purpose: shows the start menu
//-----------------------------------------------------------------------------
void CStartMenu::ShowPanel(bool bShow)
{
//	CBasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( BaseClass::IsVisible() == bShow )
		return;

	//m_pViewPort->ShowBackGround( bShow );

	if ( bShow )
	{
		//TGB: do we have a default? 'cause then don't bother opening this
		const int saved = zm_participate_saved.GetInt();
		if (saved != -1)
		{
			engine->ClientCmd(VarArgs( "zm_participate %i", saved));

			SetVisible( false );
			SetMouseInputEnabled( false );
			return;
		}

		should_save = false;
		

		//TGB: spawnmenu cannot be open open as it will conflict
		m_pViewPort->ShowPanel( PANEL_BUILD, false );
		//no viewport either until we've picked
		m_pViewPort->ShowPanel( PANEL_VIEWPORT, false );

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

}



void CStartMenu::Update()
{
}


//-----------------------------------------------------------------------------
// Purpose: Sets the text of a control by name
//-----------------------------------------------------------------------------
void CStartMenu::SetLabelText(const char *textEntryName, const char *text)
{
	Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->SetText(text);
	}
}


//LAWYER: (START MENU) Define the pressing of buttons
void CStartMenu::OnCommand( const char *command )
{
	if ( V_stricmp( command, "vguicancel" ) )
	{
		engine->ClientCmd( const_cast<char *>( command ) );
	}
	
	if (should_save == true )
	{
		if (V_stricmp( command, "zm_participate 0") == 0)
			zm_participate_saved.SetValue(0);
		else if (V_stricmp( command, "zm_participate 1") == 0)
			zm_participate_saved.SetValue(1);
	}

	//remember choice

	//close panel
	Close();
	gViewPortInterface->ShowBackGround( false );

	//TGB: only if we're zm
	CBasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if (pPlayer && pPlayer->IsZM())
		//bring viewport back up
		m_pViewPort->ShowPanel( PANEL_VIEWPORT, true );

	BaseClass::OnCommand(command);
}

void CStartMenu::OnRememberChecked( KeyValues *data )
{
	if (!data) return;
	int state = data->GetInt("state", 0);
	if (state == 1)
		should_save = true;
	else
		should_save = false;
	
}

