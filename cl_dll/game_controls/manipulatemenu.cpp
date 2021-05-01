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
//=============================================================================//

#include "cbase.h"
#include <cdll_client_int.h>
 
#include "manipulatemenu.h"

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

#include "IGameUIFuncs.h" // for key bindings
#include <igameresources.h>
extern IGameUIFuncs *gameuifuncs; // for key binding details

#include <cl_dll/iviewport.h>

#include <stdlib.h> // MAX_PATH define
#include <stdio.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar zm_menus_use_keyboard("zm_menus_use_keyboard", "0", FCVAR_CLIENTDLL|FCVAR_ARCHIVE, "Enables keyboard shortcuts in the spawn and manipulate menus, but disables normal keyboard functions such as movement and voicechat while the menus are open.");

using namespace vgui;

void UpdateCursorState();

//qck: Removed a huge amount of unused code from this file. I have no idea what it was here for, but it took up a ton of room
//and made things hard to read. 

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CManipulateMenu::CManipulateMenu(IViewPort *pViewPort) : Frame(NULL, PANEL_MANIPULATE )
{
	m_pViewPort = pViewPort;
	m_iJumpKey = -1; // this is looked up in Activate()
	//m_iScoreBoardKey = -1; // this is looked up in Activate()

	// initialize dialog
	SetTitle("Manipulate", true);

	// load the new scheme early!!
	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFile("resource/ZombieMaster.res", "ZombieMaster");
	SetScheme(scheme);
	SetSizeable(false);

	SetProportional(false); //TGB: no more proportional

	// info window about this map
//	m_pMapInfo = new RichText( this, "MapInfo" );
	SetMoveable(true);

	LoadControlSettings("Resource/UI/ManipulateMenu.res");
	InvalidateLayout();

	

	m_szMapName[0] = 0;

}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CManipulateMenu::~CManipulateMenu()
{
}

//-----------------------------------------------------------------------------
// Purpose: sets the text color of the map description field
//-----------------------------------------------------------------------------
void CManipulateMenu::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
}



//-----------------------------------------------------------------------------
// Purpose: shows the manipulate menu
//-----------------------------------------------------------------------------
void CManipulateMenu::ShowPanel(bool bShow)
{
//	CBasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( BaseClass::IsVisible() == bShow )
		return;

	//LAWYER:  Extra stuff to get the value of the Manipulate
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();  //LAWYER:  Get the local player
	if (pPlayer)
	{
		char szCmd[64];
		Q_snprintf( szCmd, sizeof( szCmd ), "\nActivate for %i",  pPlayer->m_iLastCost);

		SetLabelText( "Activate", szCmd);
		if (pPlayer->m_iLastCost > pPlayer->m_iZombiePool)
		{
			SetControlEnabled( "Activate", false);
		}
		else
		{
			SetControlEnabled( "Activate", true);
		}
	}

	
	if ( bShow )
	{
		//TGB: spawnmenu cannot be open open as it will conflict
		m_pViewPort->ShowPanel( PANEL_BUILD, false );
		//also hide viewport
		m_pViewPort->ShowPanel( PANEL_VIEWPORT, false );

		Activate();

		SetMouseInputEnabled( true );

		SetKeyBoardInputEnabled( zm_menus_use_keyboard.GetBool() );	

		// get key bindings if shown

		if( m_iJumpKey < 0 ) // you need to lookup the jump key AFTER the engine has loaded
		{
			m_iJumpKey = gameuifuncs->GetEngineKeyCodeForBind( "jump" );
		}

		/*if ( m_iScoreBoardKey < 0 ) 
		{
			m_iScoreBoardKey = gameuifuncs->GetEngineKeyCodeForBind( "showscores" );
		}*/
	}
	else
	{
		SetVisible( false );
		SetMouseInputEnabled( false );
	}

//	m_pViewPort->ShowBackGround( bShow );
}



void CManipulateMenu::Update()
{
}


//-----------------------------------------------------------------------------
// Purpose: Sets the text of a control by name
//-----------------------------------------------------------------------------
void CManipulateMenu::SetLabelText(const char *textEntryName, const char *text)
{
	Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->SetText(text);
	}
}


//LAWYER: (MANIPULATE MENU) Define the pressing of buttons
void CManipulateMenu::OnCommand( const char *command )
{
	if ( Q_stricmp( command, "vguicancel" ) != 0 ) //not a cancel msg
	{
		if (Q_stricmp( command, "toggle_trap") == 0)
		{
			//TGB: instead of going through server, send a rallymode cmd to vgui_viewport
			KeyValues *msg = new KeyValues("ButtonCommand");
			msg->SetString("command", "MODE_TRAP");
			IViewPortPanel *pview = gViewPortInterface->FindPanelByName(PANEL_VIEWPORT);
			if (pview)
				PostMessage(pview->GetVPanel(), msg);
		}
		else
		{
			//probably the "manipulate" manip-activation command
			engine->ClientCmd( const_cast<char *>( command ) );
		}
	}
	
	DoClose();
  
	BaseClass::OnCommand(command);
}


//-----------------------------------------------------------------------------
// Purpose: //LAWYER:  Update the thingy!
//-----------------------------------------------------------------------------
void CManipulateMenu::OnThink()
{
	if ( BaseClass::IsVisible() == false )
		return;

		//LAWYER:  Extra stuff to get the value of the Manipulate
		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();  //LAWYER:  Get the local player
		if (pPlayer)
		{

			char szCmd[64];
			Q_snprintf( szCmd, sizeof( szCmd ), "Activate for %i",  pPlayer->m_iLastCost);

			SetLabelText( "Activate", szCmd);
			if (pPlayer->m_iLastCost > pPlayer->m_iZombiePool)
			{
				this->SetControlEnabled( "Activate", false);
			}
			else
			{
				this->SetControlEnabled( "Activate", true);
			}
			//LAWYER:  Stuff for Trap system
			//char szCmd2[64];
			int i = 0;
			if (pPlayer->m_iLastTrapCost <= 0)
			{
				i = (pPlayer->m_iLastCost * 1.5);
			}
			else
			{
				i = (pPlayer->m_iLastTrapCost);
			}
			Q_snprintf( szCmd, sizeof( szCmd ), "Create Trap for %i",  i ); //LAWYER;  We need this to bind properly

			SetLabelText( "Trap", szCmd);
			if (i > pPlayer->m_iZombiePool)
			{
				this->SetControlEnabled( "Trap", false);
			}
			else
			{
				this->SetControlEnabled( "Trap", true);
			}
			
			if(pPlayer->m_szLastDescription)
			{
				//DevMsg("%s\n", pPlayer->m_szLastDescription);
				//qck: Set label text equal to description, etc.
				SetLabelText("Description", pPlayer->m_szLastDescription);
			}

		}
	
}

void CManipulateMenu::OnKeyCodePressed(vgui::KeyCode code)
{
	int lastPressedEngineKey = engine->GetLastPressedEngineKey();

	if( m_iJumpKey >= 0 && m_iJumpKey == lastPressedEngineKey )
	{
		DoClose();
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}

//--------------------------------------------------------------
// Helper that closes this menu 
//--------------------------------------------------------------
void CManipulateMenu::DoClose()
{
	Close();
	gViewPortInterface->ShowBackGround( false );

	//TGB: only if we're zm
	CBasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if (pPlayer && pPlayer->IsZM())
		//bring viewport back up
		m_pViewPort->ShowPanel( PANEL_VIEWPORT, true );
}

