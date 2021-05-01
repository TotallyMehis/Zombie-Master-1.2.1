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

//qck: TEST TEST TEST CVS TEST
#include "cbase.h"
#include <cdll_client_int.h>
 
#include "menu_ambush.h"

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

using namespace vgui;

void UpdateCursorState();

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CAmbushMenu::CAmbushMenu(IViewPort *pViewPort) : Frame(NULL, PANEL_AMBUSH )
{
	m_pViewPort = pViewPort;

	// initialize dialog
	SetTitle("", true);

	// load the new scheme early!!
	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFile("resource/ZombieMaster.res", "ZombieMaster");
	SetScheme(scheme);
	SetSizeable(false);

	// hide the system buttons
	SetTitleBarVisible( false );
	SetProportional(true);
	SetSize( 400, 200 );

	// info window about this map
//	m_pMapInfo = new RichText( this, "MapInfo" );

	LoadControlSettings("Resource/UI/AmbushMenu.res");
	InvalidateLayout();

	SetMoveable(true);


}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CAmbushMenu::~CAmbushMenu()
{
}

//-----------------------------------------------------------------------------
// Purpose: sets the text color of the map description field
//-----------------------------------------------------------------------------
void CAmbushMenu::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
}



//-----------------------------------------------------------------------------
// Purpose: shows the manipulate menu
//-----------------------------------------------------------------------------
void CAmbushMenu::ShowPanel(bool bShow)
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
				this->SetControlEnabled( "Activate", false);
			}
			else
			{
				this->SetControlEnabled( "Activate", true);
			}
		}

	
	if ( bShow )
	{
		Activate();

		SetMouseInputEnabled( true );

		// get key bindings if shown

		//if( m_iJumpKey < 0 ) // you need to lookup the jump key AFTER the engine has loaded
		//{
		//	m_iJumpKey = gameuifuncs->GetEngineKeyCodeForBind( "jump" );
		//}

		//if ( m_iScoreBoardKey < 0 ) 
		//{
		//	m_iScoreBoardKey = gameuifuncs->GetEngineKeyCodeForBind( "showscores" );
		//}
	}
	else
	{
		SetVisible( false );
		SetMouseInputEnabled( false );
	}

	m_pViewPort->ShowBackGround( bShow );
}



void CAmbushMenu::Update()
{
}


//-----------------------------------------------------------------------------
// Purpose: Sets the text of a control by name
//-----------------------------------------------------------------------------
void CAmbushMenu::SetLabelText(const char *textEntryName, const char *text)
{
	Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->SetText(text);
	}
}




void CAmbushMenu::OnCommand( const char *command )
{
  if ( Q_stricmp( command, "vguicancel" ) )
  {
    engine->ClientCmd( const_cast<char *>( command ) );
  }
 
	Close();
	gViewPortInterface->ShowBackGround( false );
  
  BaseClass::OnCommand(command);
}


//-----------------------------------------------------------------------------
// Purpose: //LAWYER:  Update the thingy!
//-----------------------------------------------------------------------------
void CAmbushMenu::OnThink()
{
	
}