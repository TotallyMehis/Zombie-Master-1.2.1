//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include <cdll_client_int.h>

#include "radiopanel.h"

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

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CRadioMenu::CRadioMenu(IViewPort *pViewPort) : Panel(NULL, PANEL_RADIO )
{
	m_pViewPort = pViewPort;

	// initialize dialog
//	SetTitle("Radio", true);

	// load the new scheme early!!
	SetScheme("ClientScheme");
//	SetMoveable(false);
//	SetSizeable(false);

	// hide the system buttons
//	SetTitleBarVisible( false );
	SetProportional(true);

	SetPos( 25, 200);
	SetSize( 100, 128);

	SetPaintBackgroundEnabled( true );
	SetPaintBackgroundType (2); // Rounded corner box

//	LoadControlSettings("Resource/UI/RadioPanel.res");
	InvalidateLayout();

}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CRadioMenu::~CRadioMenu()
{
}

//-----------------------------------------------------------------------------
// Purpose: sets the text color of the map description field
//-----------------------------------------------------------------------------
void CRadioMenu::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
}

void CRadioMenu::Update()
{

}


//-----------------------------------------------------------------------------
// Purpose: shows the build menu
//-----------------------------------------------------------------------------
void CRadioMenu::ShowPanel(bool bShow)
{
//	CBasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( BaseClass::IsVisible() == bShow )
		return;

	
	if ( bShow )
	{
		//Activate();

		SetMouseInputEnabled( false ); 
		SetKeyBoardInputEnabled( true );
	}
	else
	{
		SetVisible( false );
		SetMouseInputEnabled( false );
		SetKeyBoardInputEnabled( false );
	}

	m_pViewPort->ShowBackGround( bShow );
}


//-----------------------------------------------------------------------------
// Purpose: Sets the text of a control by name
//-----------------------------------------------------------------------------
//void CRadioMenu::SetLabelText(const char *textEntryName, const char *text)
//{
//	Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
//	if (entry)
//	{
//		entry->SetText("Spawn Minions");
//	}
//}

void CRadioMenu::OnKeyCodePressed(KeyCode code)
{
		BaseClass::OnKeyCodePressed( code );
}

//LAWYER: (BUILD MENU) Define the pressing of buttons
void CRadioMenu::OnCommand( const char *command )
{
 // if ( Q_stricmp( command, "vguicancel" ) )
 // {
	//  engine->ClientCmd( const_cast<char *>( command ) );
 // }
 // //LAWYER:  Checks for 
	//Close();
	//gViewPortInterface->ShowBackGround( false );
  
  BaseClass::OnCommand(command);
}

void CRadioMenu::paintString(wchar_t *string, int x, int y)
{

	surface()->DrawSetTextPos( x, y);
	surface()->DrawPrintText(string, wcslen(string));
}

void CRadioMenu::Paint()
{
	int tX = 10;
	int tY = 20;
	const int spacing = 15;

	surface()->DrawSetTextColor( 255, 255, 255, 255);

	paintString(L"1. First", tX, tY);
	tY += spacing;
	paintString(L"2. Second", tX, tY);
	tY += spacing;
	paintString(L"3. Third", tX, tY);
	tY += spacing;
	paintString(L"4. Fourth", tX, tY);
	tY += spacing;
	paintString(L"5. Last", tX, tY);
}
