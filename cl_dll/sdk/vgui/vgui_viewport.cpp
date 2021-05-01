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
// Purpose: Terrifying amalgamation of prototype-quality interface code that
//          somehow makes the ZM RTS interface work.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "c_user_message_register.h" //qck


#include <cdll_client_int.h>

#include "VGUI_Viewport.h"

#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <KeyValues.h>
#include <vgui_controls/ImageList.h>
#include <FileSystem.h>
#include <keydefs.h>

#include <vgui_controls/RichText.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/HTML.h>
#include <vgui_controls/Menu.h>

#include "IGameUIFuncs.h" // for key bindings
#include <igameresources.h>
extern IGameUIFuncs *gameuifuncs; // for key binding details

#include <cl_dll/iviewport.h>
#include "vgui/ivgui.h" //LAWYER

#include <stdlib.h> // MAX_PATH define
#include <stdio.h>
//#include "iinput.h" //moved to header
#include "view.h"
#include "view_scene.h" //TGB
#include "fx_quad.h"
#include "hud.h"

#include "zm_controlpanel.h"

#include "in_buttons.h"
#include "vgui/Cursor.h"
#include "iclientmode.h"

#include "hud_tooltip.h"

#include "vstdlib/strtools.h"

#include "hud_basechat.h"

#include "ClientEffectPrecacheSystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//TGB: idlecheck vars
ConVar zm_idlecheck_warningtime( "zm_idlecheck_warningtime", "60", FCVAR_REPLICATED, "Time spent idling (in seconds) before the player gets a warning on screen." );
ConVar zm_idlecheck_restarttime( "zm_idlecheck_restarttime", "120", FCVAR_REPLICATED, "Time spent idling (in seconds) before the round is restarted and a new ZM is picked. Also see zm_idlecheck_warningtime." );


void UpdateCursorState();
// void DuckMessage(const char *str);


//TGB: precache our sprites
CLIENTEFFECT_REGISTER_BEGIN( PrecacheZMSprites )
CLIENTEFFECT_MATERIAL( "effects/zm_ring" )
CLIENTEFFECT_MATERIAL( "effects/zm_arrows" )
CLIENTEFFECT_REGISTER_END()


#define SCROLL_THRESHOLD 8
#define	ZM_BOXSELECT_DELAY	0.1f

//extern ConVar zm_night_vision;

//qck: Global used by a variety of functions in here
//bool rallymode = false;
//bool trapmode = false;

//TGB: global pointer to viewport for usermessage use
CBaseZombieMasterViewPort *gZMViewPort = NULL;

enum DragStatus {
	LMOUSE_DOWN = 1,
	LMOUSE_DRAG,
	RMOUSE_DOWN,
	RMOUSE_DRAG
};

//TGB: enum for the various modes we'll have here, like for powers and rally etc

enum Modes{

	MODE_DEFAULT,

	//movement modes
	MODE_RALLY = 10,
	MODE_TRAP, //LAWYER:  For the trap system, I'd assume
	MODE_DEFENSIVE,
	MODE_OFFENSIVE,
	MODE_AMBUSH_CREATE,
	MODE_AMBUSH_MOVE,

	//power modes
	MODE_POWER_PHYSEXP = 20,
	MODE_POWER_NIGHTVISION,
	MODE_POWER_SPOTCREATE,
	MODE_POWER_DELETEZOMBIES,

	MODE_MAX
};

int g_iCommandMode = MODE_DEFAULT;


//#define ZM_PHYSEXP_COST		zm_physexp_cost.GetInt() //pretty damn clever if you ask me, but too hacky
//#define ZM_SPOTCREATE_COST	100	

/*TGB:	it's still best to mirror changes to zm_powers_concom, because whatever happens
		the values in zm_powers_concom will be the ones used, they'll override the defaults set here. */
static ConVar zm_physexp_cost( "zm_physexp_cost", "400", FCVAR_REPLICATED, "Explosion cost" );
static ConVar zm_spotcreate_cost( "zm_spotcreate_cost", "100", FCVAR_REPLICATED, "Explosion cost" );


//TGB: print to the chat, not unlike ClientPrint
void HudChatPrint( const char *msg )
{
	CBaseHudChat *chat = g_pClientMode->GetChat();
	if (chat)
	{
		chat->Printf(msg);
	}
}

//TGB: no longer using usermessages for rally/trap mode, it's much better to send rally/trap commands
//	directly to this panel rather than going through the server, so that's what we do now.

//----------------------------------------------------------------------------
// Purpose: ToggleRally user message callback -qck
//----------------------------------------------------------------------------
/*
void __MsgFunc_ToggleRally( bf_read &msg )
{
	rallymode = msg.ReadOneBit();
	DevMsg("UserMessage called. \n Bit Value: %i\n", rallymode);
	engine->ClientCmd("reset_rallymode");

}

USER_MESSAGE_REGISTER( ToggleRally );
*/
//----------------------------------------------------------------------------
// Purpose: ToggleTrap user message callback, copied from Qck's stuff
//----------------------------------------------------------------------------
/*
void __MsgFunc_ToggleTrap( bf_read &msg )
{
	trapmode = msg.ReadOneBit();
	DevMsg("UserMessage called. \n Bit Value: %i\n", trapmode);
	engine->ClientCmd("reset_trapmode");

}

USER_MESSAGE_REGISTER( ToggleTrap );
*/

//TGB: technically the removegroup usermsg could be reworked like the trap/rally ones have been,
//	however the consistency with the server has some advantages in this case (eg. if the deletion
//	cmd never reaches the server, the group won't disappear from the interface either, which could
//	prevent annoying issues).

//--------------------------------------------------------------
// TGB: usermessage for handling group removal from the group listing
//--------------------------------------------------------------
void __MsgFunc_RemoveGroup( bf_read &msg )
{
	//the message contains a single (long) int holding a serial number
	const int serial = msg.ReadLong();

	DevMsg("Received RemoveGroup on client\n");

	if (gZMViewPort && gZMViewPort->m_ZMControls)
	{
		gZMViewPort->m_ZMControls->RemoveGroup(serial);
	}
}

USER_MESSAGE_REGISTER(RemoveGroup );

static ConVar alternateselect("zm_altselect", "1", FCVAR_CHEAT, "Experimental alt. selection");
static ConVar zmtips("zm_tips", "1", FCVAR_ARCHIVE, "Toggles basic ZM tooltips"); //TGB: for tips about boxselect at one point

//TGB START CZMControlsHandler

CZMControlsHandler::CZMControlsHandler( vgui::Panel *pParent )
{
	LoadButtons( pParent );

	UpdateTabs( 0 );
}
CZMControlsHandler::~CZMControlsHandler()
{
	RemoveButtons();
}

void CZMControlsHandler::LoadButtons( vgui::Panel *pParent )
{
	const color32 white = { 255, 255, 255, 255 };
	const color32 grey = { 155, 155, 155, 255 };
	const color32 red = { 200, 55, 55, 255 };
	//-------
	//BUTTONS
	//-------

	// TODO: could mostly be moved into keyvalue files...

	//order needs to correspond with button enum
	const char *buttonMat[NUM_BUTTONS] = 
	{
		"VGUI/minishockwave",					//physexp
		"VGUI/minieye",							//night vision
		"VGUI/minicrosshair",					//offensive mode
		"VGUI/minishield",						//defensive mode
		"VGUI/miniselectall",					//select all
		"VGUI/miniarrows",						//ambush mode
		"VGUI/minigroupadd",					//create group
		"VGUI/minigroupselect",					//select group
		"VGUI/minispotcreate",					//spot create
		"VGUI/minideletezombies",				//Delete Zombies
		"VGUI/miniceiling",						//banshee ceiling jump/ambush
	};

	const char *buttonCmd[NUM_BUTTONS] =
	{
		"MODE_POWER_PHYSEXP",
		"MODE_POWER_NIGHTVISION",
		"MODE_OFFENSIVE",
		"MODE_DEFENSIVE",
		"MODE_SELECT_ALL",
		"MODE_AMBUSH_CREATE",
		"MODE_CREATE_GROUP",
		"MODE_GOTO_GROUP",
		"MODE_POWER_SPOTCREATE", //spot create
		"MODE_POWER_DELETEZOMBIES",
		"MODE_JUMP_CEILING",
	};

	//TGB: power costs are now printf'd into these, see toolTipCosts array below
	const char *toolTip[NUM_BUTTONS] =
	{
		"Explosion: Click in the world to blast objects away. \n[Cost: %i]",
		"Nightvision: Toggles your nightvision.",
		"Attack: Order selected units to attack any humans they see.",
		"Defend: Order selected units to defend their current location.",
		"Select all: Select all your zombies.",
		"Ambush: Set up an ambush using selected units. The units will stay put until a human comes near the ambush trigger.",
		"Create squad: Create a squad from selected units.",
		"Select squad: Select the chosen squad. The units in this squad will be selected.",
		"Hidden Summon: Click in the world to create a Shambler. Only works out of sight of the humans. \n[Cost: %i]",
		"Expire: Relinquish your control of the currently selected units.",
		"Banshee ceiling ambush: Order selected banshees to cling to the ceiling and hide until humans pass underneath."
	};

	const int toolTipCosts[NUM_BUTTONS] =
	{
		zm_physexp_cost.GetInt(),	//explosion
		0,							//nightvis
		0,							//attack mode
		0,							//defend mode
		0,							//select all
		0,							//ambush mode
		0,							//squad create
		0,							//squad select
		zm_spotcreate_cost.GetInt(),//hidden summon
		0,							//expire
		0,							//ceiling ambush
	};

	DevMsg("CZMControlsHandler: creating buttons.\n");

	//load buttons
	for (int i = 0; i < NUM_BUTTONS; i++)
	{
		m_pButtons[i] = new CBitmapButton( pParent, buttonCmd[i], "" ); 
		m_pButtons[i]->SetImage( CBitmapButton::BUTTON_ENABLED, buttonMat[i], white );
		m_pButtons[i]->SetImage( CBitmapButton::BUTTON_ENABLED_MOUSE_OVER, buttonMat[i], red );
		m_pButtons[i]->SetImage( CBitmapButton::BUTTON_PRESSED, buttonMat[i], grey );
		m_pButtons[i]->SetButtonBorderEnabled( false );
		m_pButtons[i]->SetPaintBorderEnabled( false );
		m_pButtons[i]->SetProportional(false);

		//basic command stuff
		//m_pButtons[i]->SetCommand( buttonCmd[i] );
		KeyValues *msg = new KeyValues("ButtonCommand");
		msg->SetString("command", buttonCmd[i]);
		m_pButtons[i]->SetCommand( msg );
		m_pButtons[i]->AddActionSignalTarget( pParent );
		
		m_pButtons[i]->m_bHasTooltip = true;

		//TGB: string juggling ahoy!
		char buffer[256];
		Q_snprintf(buffer, sizeof(buffer), toolTip[i], toolTipCosts[i]);

		m_pButtons[i]->m_szMouseOverText = new char[sizeof(buffer)];
		Q_strcpy(m_pButtons[i]->m_szMouseOverText, buffer);
		
	}

	//-------
	//TABS
	//-------
	const char *tabMat[NUM_TABS] = 
	{
		//temp mats
		"VGUI/minicrosshair",					//offensive mode
		"VGUI/minishockwave",					//physexp
		"VGUI/minigroupadd",					//zombie groups
	};

	const char *tabCmd[NUM_TABS] =
	{
		"TAB_MODES",
		"TAB_POWERS",
		"TAB_ZEDS",
	};

	//load tab buttons
	for (int i = 0; i < NUM_TABS; i++)
	{
		m_pTabs[i] = new CBitmapButton( pParent, tabCmd[i], "" ); 
		m_pTabs[i]->SetImage( CBitmapButton::BUTTON_ENABLED, tabMat[i], white );
		m_pTabs[i]->SetImage( CBitmapButton::BUTTON_ENABLED_MOUSE_OVER, tabMat[i], red );
		m_pTabs[i]->SetImage( CBitmapButton::BUTTON_PRESSED, tabMat[i], grey );
		
		//die borders! DIIIIEEEEE
		m_pTabs[i]->SetButtonBorderEnabled( false );
		m_pTabs[i]->SetPaintBorderEnabled( false );

		//basic command stuff
		//m_pTabs[i]->SetCommand( tabCmd[i] );
		KeyValues *msg = new KeyValues("ButtonCommand");
		msg->SetString("command", tabCmd[i]);
		m_pTabs[i]->SetCommand( msg );
		m_pTabs[i]->AddActionSignalTarget( pParent );
		m_pTabs[i]->m_bHasTooltip = false;
	}

	m_pZombieGroups = new ComboBox(pParent, "groupscombo", 5 , false); 
	//m_pZombieGroups->SetOpenDirection( ComboBox::UP );
	m_pZombieGroups->SetText("None");
	m_pZombieGroups->GetMenu()->MakeReadyForUse();
	m_pZombieGroups->GetMenu()->SetBgColor( BLACK_BAR_COLOR );
}

void CZMControlsHandler::RemoveButtons()
{
	DevMsg("CZMControlsHandler: removing buttons.\n");

	for (int i=0; i<NUM_BUTTONS; i++ )
	{
		if (m_pButtons[i] != NULL)
		{
			delete m_pButtons[i];
			m_pButtons[i] = NULL;
		}
	}

	m_ComboBoxItems.Purge();
	m_pZombieGroups->RemoveAll();
}

void CZMControlsHandler::GroupsListUpdate()
{
	//qck: Keep track of groups inside of our combo box. No duplicates.	
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	if(pPlayer)
	{
		for(int i=0; i < pPlayer->m_ZombieGroupSerial.Count(); i++)
		{
			int serialNumber = pPlayer->m_ZombieGroupSerial[i];
			 
			if( m_ComboBoxItems.Find( serialNumber ) == -1)
			{
				char groupName[ 16 ];
				Q_snprintf( groupName, sizeof( groupName ), "Group %i", i );

				KeyValues* kv = new KeyValues("group"); //qck: Associate entity serial number with its menu listing under key "serial"
				if (!kv || !m_pZombieGroups) return;
				
				kv->SetInt("serial", serialNumber);
				m_pZombieGroups->AddItem(groupName, kv); 
				kv->deleteThis();

				m_ComboBoxItems.AddToTail( serialNumber );

				DevMsg("Number of groups: %i\n", (i + 1));
			}
		}
	}
}

//--------------------------------------------------------------
// TGB: remove a group from the dropdown list
//--------------------------------------------------------------
void CZMControlsHandler::RemoveGroup(int serial)
{
    Menu *dropdown = m_pZombieGroups->GetMenu();
	if (!dropdown) return;

	// removing based purely on a bit of userdata turns out to be a hassle
	for (int i = 0; dropdown->GetItemCount(); i++)
	{
		int index = dropdown->GetMenuID(i);

		KeyValues *kv = dropdown->GetItemUserData(index);
		if (kv && kv->GetInt("serial") == serial)
		{
			dropdown->DeleteItem(index);
			DevMsg("Removed zombie group from menu\n");
			break;
		}
	}

	//more clearing up from the various tracking lists
	m_ComboBoxItems.FindAndRemove(serial);

	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	if(pPlayer)
		pPlayer->m_ZombieGroupSerial.FindAndRemove(serial);


	//reset the text area of the combobox
	m_pZombieGroups->SetText("None");
}

void CZMControlsHandler::PositionButtons()
{
	//TGB: this is ugly and should be reworked to a keyvalues approach where the "slot" icon is in
	//	is specified, so we can have stuff like A _ C, where _ is an open space.

	//do the scaling
	//TGB: UNDONE: no more scaling
	//const float flVerScale = (float)ScreenHeight() / 480.0f;
	const float flVerScale = 1;

	const int PANEL_TOPLEFT_X = ScreenWidth() - PANEL_SIZE_X + HOR_ADJUST - PANEL_SPACING;
	const int PANEL_TOPLEFT_Y = ScreenHeight() - PANEL_SIZE_Y + VER_ADJUST - PANEL_SPACING;
	//-------
	//BUTTONS
	//-------

	const int scaledsize = (int)(BUTTON_SIZE * flVerScale);
	const int scaledspacing = (int)(BUTTON_SPACING * flVerScale);

	//working in unscaled values
	int start_x = PANEL_TOPLEFT_X + BUTTON_SPACING;
	int start_y = PANEL_TOPLEFT_Y + BUTTON_SPACING;
	//and now they're scaled
	start_x = (int)(start_x * flVerScale);
	start_y = (int)(start_y * flVerScale);

	//int x = start_x;
	//int y = start_y;

	//build array of per-tab start positions
	int tab_x[NUM_TABS];
	int tab_y[NUM_TABS];
	//keep track of how many buttons have been positioned per tab
	int tab_count[NUM_TABS];
	for (int i = 0; i < NUM_TABS; i++ )
	{
		tab_x[i] = start_x;
		tab_y[i] = start_y;
		tab_count[i] = 0;
	}

	int curTab = 0;
	for (int i = 0; i < NUM_BUTTONS; i++)
	{
		if (!m_pButtons[i])
		{
			Warning("CZMControlsHandler: Attempted to position nonexistant button.");
			return;
		}

		//look up tab for this button
		curTab = buttonToTab[i];

		//apply scaled values
		m_pButtons[i]->SetPos( tab_x[curTab], tab_y[curTab] );
		m_pButtons[i]->SetSize( scaledsize, scaledsize );

		tab_count[curTab] += 1;

		//hardcoded line switch
		//0 1 2
		//3 4 5
		//6 7 8
		if ( tab_count[curTab] == 3 || tab_count[curTab] == 6)
		{
			tab_x[curTab] = start_x;
			tab_y[curTab] += scaledspacing + scaledsize;
		}
		else
		{
			tab_x[curTab] += scaledspacing + scaledsize;
		}
	}

	//-------
	//TABS
	//-------
	//unscaled values
	int tabpos_x = PANEL_TOPLEFT_X + BUTTON_SPACING;
	int tabpos_y = PANEL_TOPLEFT_Y - BUTTON_SIZE;
	//scale them...
	tabpos_x = (int)(tabpos_x * flVerScale);
	tabpos_y = (int)(tabpos_y * flVerScale);

	for (int i = 0; i < NUM_TABS; i++)
	{
		if (!m_pTabs[i])
		{
			Warning("CZMControlsHandler: Attempted to position nonexistant tab.");
			return;
		}

		//apply scaled values
		m_pTabs[i]->SetPos( tabpos_x, tabpos_y );
		m_pTabs[i]->SetSize( scaledsize, scaledsize );

		tabpos_x += ( scaledsize + scaledspacing );

	}
}

void CZMControlsHandler::PositionComboBox()
{
	//do the scaling
	//TGB: UNDONE: no more scaling
	//const float flVerScale = (float)ScreenHeight() / 480.0f;
	const float flVerScale = 1;

	const int PANEL_TOPLEFT_X = ScreenWidth() - PANEL_SIZE_X + HOR_ADJUST - PANEL_SPACING;
	const int PANEL_TOPLEFT_Y = ScreenHeight() - PANEL_SIZE_Y + VER_ADJUST - PANEL_SPACING;
	int combo_start_x = PANEL_TOPLEFT_X + COMBO_BOX_X_OFFSET;
	int combo_start_y = PANEL_TOPLEFT_Y + COMBO_BOX_Y_OFFSET;

	combo_start_x = (int)(combo_start_x * flVerScale);
	combo_start_y = (int)(combo_start_y * flVerScale);


	m_pZombieGroups->SetDrawWidth( COMBO_BOX_WIDTH );
	m_pZombieGroups->SetWide( COMBO_BOX_WIDTH );
	m_pZombieGroups->SetPos( combo_start_x, combo_start_y );
	m_pZombieGroups->SetVisible( false );
}

void CZMControlsHandler::UpdateTabs( int activatedTab )
{
	//need to update our active tab?
	if ( activatedTab != -1 && activatedTab < NUM_TABS)
		m_iActiveTab = activatedTab;

	DevMsg("Tab set to %i\n", m_iActiveTab);

	//TGB: handle button visibility

	for (int i = 0; i < NUM_BUTTONS; i++)
	{
		if ( buttonToTab[i] == m_iActiveTab )
			m_pButtons[i]->SetVisible( true  );
		else
			m_pButtons[i]->SetVisible( false );
	}

	//TGB: if our tab is buttonless, all buttons will be set to invisible now
	//this would be a good place to update visibility of other elements

	//qck: Take care of other element visibility

	if ( m_iActiveTab == TAB_ZEDS )
		m_pZombieGroups->SetVisible( true );
	else 
		m_pZombieGroups->SetVisible( false );

	//qck: Testing to see if KeyValues stay
	//if(m_pZombieGroups->GetActiveItemUserData() != NULL)
	//{
	//	KeyValues* kv = m_pZombieGroups->GetActiveItemUserData();
	//	int test = kv->GetInt("serial");
	//	DevMsg("Serial number of selected item: %i\n", test);
	//}
	
}

//draw background onto given surface
void CZMControlsHandler::PaintControls( vgui::ISurface *surface )
{
	if (!surface)
		return;

	//TGB: UNDONE: no more scaling
	//const float flVerScale = (float)ScreenHeight() / 480.0f;
//	const float flVerScale = 1;

	//TGB: replaced with unscaled method of positioning
	//const int x_tl = (int)(PANEL_TOPLEFT_X * flVerScale);
	//const int y_tl = (int)(PANEL_TOPLEFT_Y * flVerScale);
	//const int x_br = (int)(PANEL_BOTRIGHT_X * flVerScale);
	//const int y_br = (int)(PANEL_BOTRIGHT_Y * flVerScale);
	const int x_tl = ScreenWidth() - PANEL_SIZE_X + HOR_ADJUST - PANEL_SPACING;
	const int y_tl = ScreenHeight() - PANEL_SIZE_Y + VER_ADJUST - PANEL_SPACING;
	//the bottom is simpler
	const int x_br = ScreenWidth() + HOR_ADJUST - PANEL_SPACING;
	const int y_br = ScreenHeight() + VER_ADJUST - PANEL_SPACING;

	surface->DrawSetColor( Color( 70, 0, 0, 76 ) );
	surface->DrawFilledRect( x_tl, y_tl, x_br, y_br ); 

	//TGB: draw tab bgs
	int top_x = 0, top_y = 0;
	int bot_x = 0, bot_y = 0;
//	const int spacing = 0;
	for (int i = 0; i < NUM_TABS; i++)
	{
		if (!m_pTabs[i]) return;
		
		m_pTabs[i]->GetPos( top_x, top_y );
		int w, h;
		m_pTabs[i]->GetSize( w, h );
		bot_x = top_x + w;
		bot_y = top_y + h;
		
		if ( m_iActiveTab == i )
		{
			surface->DrawSetColor( Color( 70, 0, 0, 76 ) );
			m_pTabs[i]->SetAlpha( 255 );
		}
		else
		{
			m_pTabs[i]->SetAlpha( 100 );
			surface->DrawSetColor( Color( 70, 0, 0, 40 ) );
		}

		surface->DrawFilledRect( top_x, top_y, bot_x, bot_y ); 
	}
}

//TGB END CZMControlsHandler

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CBaseZombieMasterViewPort::CBaseZombieMasterViewPort(IViewPort *pViewPort) : Frame(NULL, PANEL_VIEWPORT )
{
	m_pViewPort = pViewPort;

	gZMViewPort = this;

	Activate();

	SetVisible( false );
	
	SetProportional(false); 


	const int CORNER_SIZE = 8;
	const int BOTTOMRIGHTSIZE = 18;
	const int CAPTION_SIZE = 23;

	//TGB: I'm not sure why this is done, and it shifts the vgui::surface which makes me account for it when drawing buttons
	//is it really necessary?
	SetBounds(0 - CORNER_SIZE,
		0 - CAPTION_SIZE,
		ScreenWidth() + CORNER_SIZE + BOTTOMRIGHTSIZE,
		ScreenHeight() + CAPTION_SIZE + BOTTOMRIGHTSIZE);

	SetSizeable(false); 
	SetMoveable(false);
	SetCloseButtonVisible(false);
	SetTitleBarVisible(false);
	SetMaximizeButtonVisible(false);
	SetMinimizeButtonVisible(false);
	SetBorder(NULL);

	//TGB: no draggin' whatsoever
	m_iDragStatus = 0;

	m_flLastClick = 0;
	
	g_iCommandMode = MODE_DEFAULT;

	SetAlpha(255);
	SetPaintBackgroundEnabled( false );
	SetKeyBoardInputEnabled( false );
	vgui::ivgui()->AddTickSignal( GetVPanel(), 150 );

	//gameeventmanager->AddListener(this, "round_restart", false); qck note - This doesn't seem to work anymore. Just not picking up here. No idea why. 

	//TGB: test button
	//color32 white = { 255, 255, 255, 255 };
	//color32 grey = { 155, 155, 155, 255 };
	//color32 red = { 200, 55, 55, 255 };

	//DevMsg("viewport: creating button.\n");

	//CBitmapButton *m_pButton = new CBitmapButton( this, "TestButton", "" ); 

	//m_pButton->SetImage( CBitmapButton::BUTTON_ENABLED, "VGUI/miniskull", white );
	//m_pButton->SetImage( CBitmapButton::BUTTON_ENABLED_MOUSE_OVER, "VGUI/miniskull", red );
	//m_pButton->SetImage( CBitmapButton::BUTTON_PRESSED, "VGUI/miniskull", grey );
	//m_pButton->SetProportional(true);
	//m_pButton->SetPos( 548, 388 );
	//m_pButton->SetSize( 24, 24 );

	//TGB: create zmcontrols
	m_ZMControls = new CZMControlsHandler( this ); 
	m_ZMControls->PositionButtons();
	m_ZMControls->PositionComboBox();

	//qck: Create the listener
	m_ZMListen = new CZMPanelEventListener();

}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CBaseZombieMasterViewPort::~CBaseZombieMasterViewPort()
{
	if (m_ZMControls)
		delete m_ZMControls;

	//TGB: think we ought to clean the listener up as well:
	if (m_ZMListen)
		delete m_ZMListen;
}

void CBaseZombieMasterViewPort::FireGameEvent(IGameEvent *pEvent)
{

	if (!strcmp("round_restart", pEvent->GetName()))
	{
		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
		if(pPlayer)
		{
			pPlayer->m_ZombieGroupSerial.RemoveAll();
		}
		m_ZMControls->m_ComboBoxItems.Purge();
		m_ZMControls->m_pZombieGroups->RemoveAll();
		m_ZMControls->m_pZombieGroups->DeleteAllItems();
		m_ZMControls->m_pZombieGroups->Delete();
		DevMsg("Delete GUI groups stuff!\n");
	}
}

void CBaseZombieMasterViewPort::TraceScreenToWorld(int mousex, int mousey, int iClickType)
{
		trace_t tr;

		//TGB: moved tracing into function for portability
		ScreenToWorld(mousex, mousey, tr);

		//LAWYER:  Now to check if we hit anything.
		//TGB: only used for certain modes now
		if (iClickType == 1) 
		{
			
			//Selection of spawns, manipulates, or NPC's.
			if ( tr.fraction != 1.0 && tr.DidHitNonWorldEntity() )
			{
				//LAWYER:  The server can pick up entities using UTIL_EntityByIndex( int entityIndex )
				//engine->ClientCmd( VarArgs( "zombiemaster_select_index %i %i", tr.m_pEnt->entindex(), iClickType ) ); //qck: send off the ent index with the click type 
				//engine->ClientCmd( VarArgs( "conq_npc_select_index %i", tr.m_pEnt->entindex(), iClickType ) ); //qck: send off the ent index with the click type 
			}
			//Setting of the rally point if the user commands it

			else if( g_iCommandMode == MODE_RALLY )
			{
				engine->ClientCmd( VarArgs( "set_rally_point %f %f %f", tr.endpos.x, tr.endpos.y, tr.endpos.z) ); 
				
				FX_AddQuad( tr.endpos,
					tr.plane.normal,
					100,
					0,
					0.75f, 
					1.0f,
					0.0f,
					0.4f,
					random->RandomInt( 0, 360 ), 
					0,
					Vector( 1.0f, 1.0f, 1.0f ), 
					1.0f, 
					"effects/zm_arrows",
					(FXQUAD_BIAS_SCALE|FXQUAD_BIAS_ALPHA) );

				//rallymode = false;
				g_iCommandMode = MODE_DEFAULT;

				return;
			}


			//LAWYER:  Placement of traps
			if( g_iCommandMode == MODE_TRAP ) //TGB: no longer an elseif, because we can place trap triggers on non-worldgeometry stuff fine
			{
				engine->ClientCmd( VarArgs( "create_trap %f %f %f", tr.endpos.x, tr.endpos.y, tr.endpos.z) );  //LAWYER:  Do stuff!
				
/*				FX_AddQuad( tr.endpos,
					tr.plane.normal,
					100,
					0,
					0.75f, 
					1.0f,
					0.0f,
					0.4f,
					random->RandomInt( 0, 360 ), 
					0,
					Vector( 1.0f, 1.0f, 1.0f ), 
					1.0f, 
					"effects/zm_arrows",
					(FXQUAD_BIAS_SCALE|FXQUAD_BIAS_ALPHA) );
*/
				//trapmode = false;
				g_iCommandMode = MODE_DEFAULT;

				return;
			}

			if ( g_iCommandMode == MODE_AMBUSH_CREATE )
			{
				//qck: Do a trace, play an effect...right?
				engine->ClientCmd( VarArgs( "zm_create_ambush %f %f %f", tr.endpos.x, tr.endpos.y, tr.endpos.z ) );
				DevMsg("Sent out the command for zombie ambush creation\n");

				g_iCommandMode = MODE_DEFAULT;

				return;
			}

			if ( g_iCommandMode == MODE_AMBUSH_MOVE )
			{
				//qck: Do a trace, play an effect...right?
				//engine->ClientCmd( VarArgs( "zm_create_ambush %f %f %f", tr.endpos.x, tr.endpos.y, tr.endpos.z ) );
				DevMsg("Ready to move the ambush point\n");

				g_iCommandMode = MODE_DEFAULT;

				return;
			}
			
			if ( g_iCommandMode == MODE_POWER_PHYSEXP )
			{
				CBasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
				if (!pPlayer || pPlayer->m_iZombiePool < zm_physexp_cost.GetInt() )
				{
					//TODO: play a deny sound
					//string_t text = ("Not enough resources!\n");
					//engine->ClientCmd(VarArgs("zm_hudchat %s", text ));
					HudChatPrint("Insufficient resources!");
					g_iCommandMode = MODE_DEFAULT;
					return;
				}

				//just use this sprite for now, but make it expand instead of contract
				FX_AddQuad( tr.endpos,
					Vector(0,0,1),
					10.0f,
					325.0f,
					0, 
					0.2f,
					0.8f,
					0.4f,
					random->RandomInt( 0, 360 ), 
					0,
					Vector( 1.0f, 1.0f, 1.0f ), 
					0.4f, 
					"effects/zm_ring",
					(FXQUAD_BIAS_ALPHA) );

				engine->ClientCmd( VarArgs( "zm_power_physexplode %f %f %f", tr.endpos.x, tr.endpos.y, tr.endpos.z ) );

				g_iCommandMode = MODE_DEFAULT;

				return;
			}
			if ( g_iCommandMode == MODE_POWER_SPOTCREATE)
			{
				//LAWYER:  Do your stuff!
				CBasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
				if (!pPlayer || pPlayer->m_iZombiePool < zm_spotcreate_cost.GetInt() )
				{
					//TODO: play a deny sound
					HudChatPrint("Insufficient resources!");

					g_iCommandMode = MODE_DEFAULT;
					return;
				}

				//just use this sprite for now, but make it expand instead of contract
				FX_AddQuad( tr.endpos,
					Vector(0,0,1),
					10.0f,
					25.0f,
					0, 
					0.2f,
					0.8f,
					0.4f,
					random->RandomInt( 0, 360 ), 
					0,
					Vector( 1.0f, 1.0f, 1.0f ), 
					0.4f, 
					"effects/zm_ring",
					(FXQUAD_BIAS_ALPHA) );

				engine->ClientCmd( VarArgs( "zm_power_spotcreate %f %f %f", tr.endpos.x, tr.endpos.y, tr.endpos.z ) );

				g_iCommandMode = MODE_DEFAULT;

				return;

			}

			//Deselection of NPC's
			//else
			//{
			//	engine->ClientCmd( VarArgs( "conq_npc_deselect") );
			//}
		}

		//TGB: still using this in new selection method
		if (iClickType == 2)
		{
			//qck: If we're here then we mean to move our zombies someplace.
			FX_AddQuad( tr.endpos,
				tr.plane.normal,
				10 * 10.0f,
				0,
				0.75f, 
				1.0f,
				0.0f,
				0.4f,
				random->RandomInt( 0, 360 ), 
				0,
				Vector( 1.0f, 1.0f, 1.0f ), 
				0.25f, 
				"effects/zm_ring",
				(FXQUAD_BIAS_SCALE|FXQUAD_BIAS_ALPHA) );
			
			//TGB: if we click on an object, we want the zombies to attack it or otherwise interact
			//Selection of objects
			if ( tr.fraction != 1.0 && tr.DidHitNonWorldEntity() )
			{
				engine->ClientCmd( VarArgs( "zm_npc_target_object %f %f %f %i", tr.endpos.x, tr.endpos.y, tr.endpos.z, tr.m_pEnt->entindex() ) );
			}
			else
				engine->ClientCmd( VarArgs( "conq_npc_move_coords %f %f %f", tr.endpos.x, tr.endpos.y, tr.endpos.z ) );

		}

		//TGB: still using this in new selection method, at least until box select is in
		if( iClickType == 3 )
		{
			//qck: Group Select
			FX_AddQuad( tr.endpos,
				tr.plane.normal,
				10 * 51.2f,
				0,
				0.75f, 
				1.0f,
				0.0f,
				0.4f,
				random->RandomInt( 0, 360 ), 
				0,
				Vector( 1.0f, 1.0f, 1.0f ), 
				0.25f, 
				"effects/zm_ring",
				(FXQUAD_BIAS_SCALE|FXQUAD_BIAS_ALPHA) );

			engine->ClientCmd( VarArgs( "conq_npc_select_sphere %f %f %f", tr.endpos.x, tr.endpos.y, tr.endpos.z ) );	

		}
		

}

//TGB: seperate function for just a trace
void CBaseZombieMasterViewPort::ScreenToWorld( int mousex, int mousey, trace_t &tr, bool ignore_npc /*= true*/ )
{
	//moved from TraceScreenToWorld to here
	//trace_t tr; //getting by ref
	CBasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if(pPlayer)
	{
		float fov = pPlayer->GetFOV();
		float dx, dy;
		float c_x, c_y;
		float dist;
		// First, we need the aspect as float!
		float aspect = (float)ScreenWidth() / (float)ScreenHeight();

		// Then we need to calculate the difference between a 4:3 aspect and our current aspect
		aspect = ( 4.0f / 3.0f ) / aspect;

		Vector vecPickingRay;

		c_x = ScreenWidth() / 2;
		c_y = ScreenHeight() / 2;

		dx = ((float)mousex - c_x) / aspect;

		// Invert Y
		dy = (c_y - (float)mousey) / aspect;

		// Convert view plane distance
		dist = c_x / tan( M_PI * fov / 360.0 );

		// Offset forward by view plane distance, and then by pixel offsets
		vecPickingRay = (MainViewForward() * dist) + (MainViewRight() * dx) + (MainViewUp() * dy);

		// Convert to unit vector
		VectorNormalize( vecPickingRay );
		CTraceFilterNoNPCs traceFilter( NULL, COLLISION_GROUP_NONE );

		if (ignore_npc)
			UTIL_TraceLine( MainViewOrigin(), MainViewOrigin() + vecPickingRay * 8192, MASK_SOLID, &traceFilter, &tr);
		else //don't use tracefilter if we want to hit npcs (tooltip trace)
			UTIL_TraceLine( MainViewOrigin(), MainViewOrigin() + vecPickingRay * 8192, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);
		//UTIL_TraceLine( eye, GetCenter() + Vector( 0, 0, offset ), MASK_OPAQUE | CONTENTS_MONSTER, &traceFilter, &result );

	}
}

void CBaseZombieMasterViewPort::LineCommand ( int mousex, int mousey )
{
	//grab number of zombies to line up
	CBasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if (!pPlayer) return;

	const int num_zombies_all = pPlayer->m_iZombieSelected;
	int num_zombies = num_zombies_all - 1; //-1 because first zombie goes on start of drag
	
	if (num_zombies <= 0)
	{
		DevMsg("Can't linecommand with no zombies selected!\n");
		return;
	}

	//start and end coordinates
	Vector2D vecStart = Vector2D(m_iDragMouseX, m_iDragMouseY);
	Vector2D vecEnd = Vector2D(mousex, mousey);
	//vector of the line
	Vector2D vecLine = vecEnd - vecStart;
	//length of the line
	double linelength = vecLine.Length();
	//line length could in theory be far too short to fit all zombies, but that's a job for the pathfinding to work out and just bunch 'em up
	
	double spacing = linelength / (double)(num_zombies);

	//find the line's starting point
	trace_t tr;
	ScreenToWorld(vecStart.x, vecStart.y, tr);

	//send the cmd for the first zombie, this will initiate line-mode as well
	//format: zm_linecommand <start=1/stop=2> <x> <y> <z> <zombie_index>
	int state = 1;
	engine->ClientCmd( VarArgs( "zm_linecommand_cc %i %f %f %f %i", state, tr.endpos.x, tr.endpos.y, tr.endpos.z, 0 ) );

	//this is nice, I feared I'd have to do this manually
	Vector2D vecGoalPos;
	Vector2D vecPrevPos = vecStart;

	//for all selected zombies...
	for (int i=1; i <= num_zombies; i++)
	{
		//find next screen position along the line at the calculated spacing
		ComputeClosestPoint2D( vecPrevPos, spacing, vecEnd, &vecGoalPos );

		//we can keep reusing our tr
		//I'm assuming data is copied into the trace_t struct, so we don't have to delete
		ScreenToWorld(vecGoalPos.x, vecGoalPos.y, tr);

		//kind of hacky
		if ( i == num_zombies )
		{
			DevMsg("Sending last linecommand!\n");
			state = 2;
		}
		DevMsg("Sending coords for zombie #%i\n", i);
		//dispatch the clientcmd
		engine->ClientCmd( VarArgs( "zm_linecommand_cc %i %f %f %f %i", state, tr.endpos.x, tr.endpos.y, tr.endpos.z, i ) );

		vecPrevPos = vecGoalPos; //save away to continue the line properly (ie. spacing from last position)
	}

	//that should be it

}

//TGB: Experimenting with an alternate way of selecting stuff
void CBaseZombieMasterViewPort::WorldToScreenSelect(int mousex, int mousey, int iClickType, bool sticky)
{
	const VMatrix &worldToScreen = engine->WorldToScreenMatrix();

	//TGB: need to convert sticky select bool to something we can send
	int stick_send = 0;
	if (sticky)
		//ooh, it's sticky
		stick_send = 1;

	//TGB: these matrix sends through a clientcmd make me cry

	//TGB: send the server the goods
	engine->ClientCmd( VarArgs( "zm_altselect_cc %i %i %f %f %f %f %f %f %f %f %f %f %f %f %i %i %i", 
		mousex, 
		mousey,
		worldToScreen[0][0],
		worldToScreen[0][1],
		worldToScreen[0][2],
		worldToScreen[0][3],
		worldToScreen[1][0],
		worldToScreen[1][1],
		worldToScreen[1][2],
		worldToScreen[1][3],
		//we don't need row 2 'cause we don't need z
		worldToScreen[3][0],
		worldToScreen[3][1],
		worldToScreen[3][2],
		worldToScreen[3][3],
		ScreenWidth(),
		ScreenHeight(),
		stick_send			//oh yes, I just made this mammoth command even longer
	) );	
}

//TGB: general function that sends a command to select between (x1,y1) and (x2,y2)
// more server-side changes may be required to make it usable as such (eg. used for normal selection too)
void CBaseZombieMasterViewPort::ZoneSelect( const int x1, const int y1, const int x2, const int y2 ) const
{
	//do we want a sticky select action?
	CBasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer(); 
	const int stick_send = ( pPlayer && pPlayer->m_nButtons & IN_DUCK ) ? 1 : 0;

	//if ( pPlayer && pPlayer->m_nButtons & IN_DUCK )
	//	stick_send = 1;

	const VMatrix &worldToScreen = engine->WorldToScreenMatrix();

	//TGB: send the server the goods
	engine->ClientCmd( VarArgs( "zm_zoneselect_cc %i %i %f %f %f %f %f %f %f %f %f %f %f %f %i %i %i %i %i", 
		x1, 
		y1,
		worldToScreen[0][0],
		worldToScreen[0][1],
		worldToScreen[0][2],
		worldToScreen[0][3],
		worldToScreen[1][0],
		worldToScreen[1][1],
		worldToScreen[1][2],
		worldToScreen[1][3],
		//we don't need row 2 'cause we don't need z
		worldToScreen[3][0],
		worldToScreen[3][1],
		worldToScreen[3][2],
		worldToScreen[3][3],
		ScreenWidth(),
		ScreenHeight(),
		x2,
		y2,
		stick_send
	) );	
}

//TGB: select the contents of a box drawn on screen by ZM
void CBaseZombieMasterViewPort::BoxSelect( int mousex, int mousey )
{
	int topleft_x, topleft_y, botright_x, botright_y;

	//find the correct coordinates (in case of inverted dragging)
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

	//select the zone between these coords
	ZoneSelect( topleft_x, topleft_y, botright_x, botright_y );

}

void CBaseZombieMasterViewPort::ShowPanel( bool bShow )
{
	SetVisible(bShow);
	//LAWYER:  Hopefully, stops the thing from spinning.
	engine->ClientCmd("-left");
	engine->ClientCmd("-right");
	engine->ClientCmd("-lookdown");
	engine->ClientCmd("-lookup");

	//LAWYER:  Kill tooltips

	CHudToolTip *m_pZMTooltip = GET_HUDELEMENT( CHudToolTip );
	if(m_pZMTooltip)
	{
		m_pZMTooltip->ClearTooltip();
		m_pZMTooltip->Hide(true);
	}
}

void CBaseZombieMasterViewPort::doTooltipThink()
{
	//TGB: don't trace every frame, it's inefficient
	const int TRACE_DELAY = 0.2f;
	if ( gpGlobals->curtime <= ( m_flLastTipTrace + TRACE_DELAY ) )
	{
		return;
	}
	else
		m_flLastTipTrace = gpGlobals->curtime;

	//TGB: would crash if round ended, as the local player is then gone and ScreenToWorld did nothing with the trace
	//hence, sometimes the trace's uninit'd garbage data would pass a check somewhere below and cause a crash
	//so we want out if there's no local player
	CBasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if(!pPlayer)
		return;

	if (!::input) return;

	int mousex, mousey;
	::input->GetFullscreenMousePos( &mousex, &mousey );

	trace_t tr;
	ScreenToWorld(mousex, mousey, tr, false);

	bool bFound = false; //track whether we've found something to tooltip
	char text[256];

	//hit something gooood?
	if( ( tr.fraction != 1.0f ) && ( tr.GetEntityIndex() > 0 )) //World ent = 0, no ent = -1
	{

		C_BaseEntity *pEnt = tr.m_pEnt;

		if ( !pEnt ) 
			return;

		//is it a player?
		if( pEnt->IsPlayer() )
		{
			C_BasePlayer *pPlayer = dynamic_cast<C_BasePlayer *>(pEnt);
			if(pPlayer)
			{
				bFound = true;
				Q_snprintf(text, sizeof(text), "%s: A human who as of yet has survived. The insolent fool.", pPlayer->GetPlayerName() );				
			}
		}

		//does it have a tooltip specified then?
		const char *setText = pEnt->GetZMTooltip();
		if ( !bFound && setText != NULL )
		{
			bFound = true;
			Q_snprintf(text, sizeof(text), "%s", setText);
		}
		/*else
			DevMsg(pEnt->GetClassname());*/

		//Any strcmping we may want to do here
		//note that it usually won't do much good as classnames that exist on the client mean there is a class where you can declare the virtual to specify a tooltip
		/*
		if (!bFound)
		{

			//doh, looks like we need to go on a strcmp spree instead
			//DevMsg("Class: %s\n", pEnt->GetClassname());
			//if ( Q_strcmp(pEnt->GetClassname(), "npc_zombie") == 0)
			//{
			//	bFound = true;
			//	sprintf(text, "%s", "Shambler: slow zombie that dies (again) quickly, but packs a mean swipe.");
			//}
		}
		*/
	}

	if ( bFound )
	{
		CHudToolTip *m_pZMTooltip = GET_HUDELEMENT( CHudToolTip );
		if(m_pZMTooltip && m_pZMTooltip->IsHidden() )
		{
			m_pZMTooltip->SetTooltipTimed( text, GetVPanel(), 2.5 );
		}
	}

//	if (text != NULL) delete text;
}

void CBaseZombieMasterViewPort::OnThink()
{

	//TGB FIXME: scroll threshold should scale with resolution

	if (IsVisible()) //are we even visible?
	{
		if (m_iDragStatus < 1) //if we're not dragging we can rotate
		{
			int mousex, mousey;
			::input->GetFullscreenMousePos( &mousex, &mousey );  //LAWYER:  Grasp the extreme mouseness of the coords	
			//LAWYER:  Left-right scrolling
			if (mousex <= SCROLL_THRESHOLD)
			{
				engine->ClientCmd("+left");
				engine->ClientCmd("-right");
			}
			else if (mousex >= (ScreenWidth() - SCROLL_THRESHOLD))
			{
				engine->ClientCmd("+right");
				engine->ClientCmd("-left");
			}
			else
			{
				engine->ClientCmd("-right");
				engine->ClientCmd("-left");
			}
			//LAWYER:  Up-down scrolling
			//qck edit- fixed this up, got rid of the jitter
			if (mousey <= SCROLL_THRESHOLD)
			{
				engine->ClientCmd("+lookup");
				engine->ClientCmd("-lookdown");
			}
			else if (mousey >= (ScreenHeight() - SCROLL_THRESHOLD))
			{
				engine->ClientCmd("+lookdown");
				engine->ClientCmd("-lookup");
			}
			else
			{
				engine->ClientCmd("-lookup");
				engine->ClientCmd("-lookdown");
			}
		}

		//TGB: tooltip stuff
		doTooltipThink(); //TGB: doubt we want to do this while not visible, as no tooltip can be displayed then
	}
}

//TGB: called every 150 msec, which is plenty for our idle checking
void CBaseZombieMasterViewPort::OnTick()
{
	BaseClass::OnTick();

	//TGB: check our idlestuffs, if this guy's idle there will be much roundrestarting
	idleData.UpdateAndCheck();
}


//TGB: box select stuff
void CBaseZombieMasterViewPort::OnCursorMoved(int x, int y)
{
	idleData.MouseMoved(); //TGB: tell our idletracker that the mouse has been moved

	//this dragstatus thing is kinda redundant now that there's the trigger delay
	if (m_iDragStatus < 1)
		return; //only interested in this if mousebutton is down

    if (m_flDragTrigger < gpGlobals->curtime && m_iDragStatus == LMOUSE_DOWN)
	{
		//DevMsg("We're dragging LMOUSE. %i %i\n", x,y);
		m_iDragStatus = LMOUSE_DRAG;
	}
	else if (m_flDragTrigger < gpGlobals->curtime && m_iDragStatus == RMOUSE_DOWN)
	{
		//DevMsg("We're dragging RMOUSE. %i %i\n", x,y);
		m_iDragStatus = RMOUSE_DRAG;
	}

	//if we're dragging, do a tooltip
	if (m_iDragStatus > 1 && zmtips.GetBool() == true)
	{
		//get tooltip pointer
		//CBasePlayer *pPlayer = CBasePlayer::GetLocalPlayer();
		CHudToolTip *m_pZMTooltip = GET_HUDELEMENT( CHudToolTip );
		//only set and display tooltip if we have the ptrs and if we're not already displaying
		if(m_pZMTooltip && m_pZMTooltip->IsHidden() )
		{
			//DevMsg("Inside cursormoved, called SetToolTip at %i\n", (int)gpGlobals->curtime);

			//TGB: these don't need to be timed, but handy for testing the function
			if ( m_iDragStatus == LMOUSE_DRAG )
				m_pZMTooltip->SetTooltipTimed("Box select: Drag the left mouse button to draw a selection area. Release to select all units in that area.", GetVPanel(), 3);
			else if ( m_iDragStatus == RMOUSE_DRAG )
				m_pZMTooltip->SetTooltipTimed("Form line: Drag the right mouse button to draw a line. Release to order units to take up positions along the line.", GetVPanel(), 3);

			//pPlayer->m_Local.m_iHideHUD &= ~HIDEHUD_TOOLTIP;
		}
	}

	//draw a nice rectangle
	if( m_HudBoxSelect)
	{
		if ( m_iDragStatus == LMOUSE_DRAG )
		{
			m_HudBoxSelect->m_bDrawBox = true;
			m_HudBoxSelect->m_bDrawLine = false;
		}
		else if ( m_iDragStatus == RMOUSE_DRAG )
		{
			m_HudBoxSelect->m_bDrawLine = true;
			m_HudBoxSelect->m_bDrawBox = false;
		}
		m_HudBoxSelect->m_iDragMouseX = m_iDragMouseX;
		m_HudBoxSelect->m_iDragMouseY = m_iDragMouseY;
	}

}

void CBaseZombieMasterViewPort::Paint(void)
{
	//TGB: draw zmcontrols
	//move to paintbackground?

	if ( m_ZMControls != NULL)
	{
		m_ZMControls->PaintControls(vgui::surface());
		m_ZMControls->GroupsListUpdate();
	}
}

/*
void CBaseZombieMasterViewPort::PaintBackground(void)
{
	vgui::surface()->DrawSetColor(  255, 0, 0, 255 ); //RGBA
	vgui::surface()->DrawFilledRect( 0, 0, 20, 20 ); //x0,y0,x1,y1
}
*/
void CBaseZombieMasterViewPort::OnMousePressed(MouseCode code)
{
	//TGB: just making sure
	if (IsVisible() == false)
		return;

	//DevMsg("Click at %f\n", gpGlobals->curtime);

	//TGB: some weirdness is making us receive clicks twice. I blame vgui.
	if (m_flLastClick == gpGlobals->curtime)
		return;
	else
		m_flLastClick = gpGlobals->curtime;
	

	//TGB: wait, why do this?
	SetMouseInputEnabled( true );

	int mousex, mousey;
	::input->GetFullscreenMousePos( &mousex, &mousey );


	if (m_iDragStatus > 1)
	{
	/* 0000277 TGB:	if the user has moved the mouse out of the screen area while dragging
					and let go of the mouse, this is a click we can't detect, and the drag
					is not ended. Instead we let the user end the drag succesfully with his
					first click, allowing him to recover it. Previously, the click would be
					needed to clear the box/line but have no effect. 
					This may not be entirely expected behaviour, but it's learnable and it's
					the best we can do as long as we can't detect off-screen events. 
					Long story short: we don't want to reset dragstatus here if we're dragging.*/

		return;
	}

	m_iDragStatus = 0; //if we just clicked, we're not dragging (yet)

	m_iDragMouseX = mousex;
	m_iDragMouseY = mousey;

	//DevMsg("\nGrabbed mouse as x: %i, y: %i ; mx %i my %i", mousex, mousey, m_iDragMouseX, m_iDragMouseY);
	//TGB: not sure why this is here, for if we start a drag, I assume
	m_HudBoxSelect = GET_HUDELEMENT( CHudBoxSelect );

	//The rebinding code is below -qck
	const char* currentBinding;

	if (gameuifuncs == NULL) return; //TGB: again, just making sure

	switch ( code )
	{
	case MOUSE_LEFT:
		currentBinding = gameuifuncs->Key_BindingForKey( K_MOUSE1 );
		break;
	case MOUSE_RIGHT:
		currentBinding = gameuifuncs->Key_BindingForKey( K_MOUSE2 );
		break;
	case MOUSE_MIDDLE:
		currentBinding = gameuifuncs->Key_BindingForKey( K_MOUSE3 );
		break;
	default:
		return; //this ain't no mouseclick like we used to have em back in the day!
	}

	//TGB: just making sure
	if (currentBinding == NULL) return;

	if( Q_strcmp(currentBinding, "+attack") == 0 )
	{
        //TGB: alt select exp.
		if ( alternateselect.GetBool() )
		{	
			if((g_iCommandMode == MODE_POWER_PHYSEXP) || 
			   (g_iCommandMode == MODE_AMBUSH_CREATE) || 
			   (g_iCommandMode == MODE_POWER_SPOTCREATE) ||
			   (g_iCommandMode == MODE_RALLY) || 
			   (g_iCommandMode == MODE_TRAP))
			{
				TraceScreenToWorld(mousex, mousey, 1);
			}
			else
			{
				//TGB: we're not selecting anything until we release the button
				//WorldToScreenSelect(mousex, mousey, 1);
				m_iDragStatus = 1; //mouse is down, may start dragging
				m_flDragTrigger = gpGlobals->curtime + ZM_BOXSELECT_DELAY;

			}
		}
		else
			TraceScreenToWorld(mousex, mousey, 1);

		return;
	}

	//right click
	if( Q_strcmp(currentBinding, "+attack2") == 0 )
	{
		//user may be starting a drag
		m_iDragStatus = RMOUSE_DOWN;
		//identical delay to boxselect
		m_flDragTrigger = gpGlobals->curtime + ZM_BOXSELECT_DELAY;

		//moved to mouserelease
		//TraceScreenToWorld(mousex, mousey, 2);
		return;
	}
	//middle click
	if( Q_strcmp(currentBinding, "+attack3") == 0 )
	{
		TraceScreenToWorld(mousex, mousey, 3);
		return;
	}

}
//TGB:
//Purpose: handle normal select and dragging
void CBaseZombieMasterViewPort::OnMouseReleased(MouseCode code)
{
	if (IsVisible() == false)
		return;

	int mousex, mousey;
	::input->GetFullscreenMousePos( &mousex, &mousey );

	//handle tooltip
	
	//get tooltip pointer
	//CBasePlayer *pPlayer = CBasePlayer::GetLocalPlayer();
	CHudToolTip *m_pZMTooltip = GET_HUDELEMENT( CHudToolTip );
	//only set and display tooltip if we have the ptrs and if we're not already displaying
	if(m_iDragStatus > 1 && m_pZMTooltip && m_pZMTooltip->IsHidden() == false )
	{
		//DevMsg("Tooltip cleared in mousereleased\n");
		m_pZMTooltip->ClearTooltip();
		//pPlayer->m_Local.m_iHideHUD |= HIDEHUD_TOOLTIP;
		m_pZMTooltip->Hide(true);
	}

	if ( m_iDragStatus == LMOUSE_DOWN) //if we left clicked, normal select
	{
		//DevMsg("Normal select\n");

		//typeselect?
		CBasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer(); 
		if ( pPlayer && pPlayer->m_nButtons & IN_JUMP ) //are pressing duck?
		{
			//we need to select all of type on screen
			DevMsg("Type-select detected!\n");

			//stickystyle?
			int stick_send = 0;
			if ( pPlayer && pPlayer->m_nButtons & IN_DUCK )
                stick_send = 1;

			//here we go again
			const VMatrix &worldToScreen = engine->WorldToScreenMatrix();

			//send the server the goods
			engine->ClientCmd( VarArgs( "zm_typeselect_cc %i %i %f %f %f %f %f %f %f %f %f %f %f %f %i %i %i", 
				mousex, 
				mousey,
				worldToScreen[0][0],
				worldToScreen[0][1],
				worldToScreen[0][2],
				worldToScreen[0][3],
				worldToScreen[1][0],
				worldToScreen[1][1],
				worldToScreen[1][2],
				worldToScreen[1][3],
				//we don't need row 2 'cause we don't need z
				worldToScreen[3][0],
				worldToScreen[3][1],
				worldToScreen[3][2],
				worldToScreen[3][3],
				ScreenWidth(),
				ScreenHeight(),
				stick_send
			) );	
	
		}
		else
		{
			if ( pPlayer && pPlayer->m_nButtons & IN_DUCK )
                WorldToScreenSelect(mousex, mousey, 1, true); //sticky select
			else
				WorldToScreenSelect(mousex, mousey, 1, false); //normal select
		}
	}
	else if ( m_iDragStatus == LMOUSE_DRAG) //if we finished drawing a box, process selection here
	{
		//DevMsg("Ended boxselect drag\n");
		BoxSelect( mousex, mousey );
	}


	if ( m_iDragStatus == RMOUSE_DOWN )
	{
		//command click
		TraceScreenToWorld(mousex, mousey, 2);
	}
	else if ( m_iDragStatus == RMOUSE_DRAG )
	{
		//line formation drag
		DevMsg("Ended RDrag\n");
		LineCommand( mousex, mousey );
	}
	
	m_iDragStatus = 0; //mouse is up, no dragging anymore

	if ( m_HudBoxSelect ) //TGB: this part was unsafe, added a check
	{
		m_HudBoxSelect->m_bDrawBox = false;
		m_HudBoxSelect->m_bDrawLine = false;
	}
}

//TGB: handle button msges
//void CBaseZombieMasterViewPort::OnMessage(const KeyValues *params, VPANEL fromPanel)
//moved into our own handler for our own ButtonCommand event, so we only do this whole thing if we
//actually got something from our buttons
void CBaseZombieMasterViewPort::OnButtonCommand(const char *command)
{
	DevMsg("Viewport received cmd: %s\n", command);

	//leave this to true if it's a toggle, to false if it's a single command (like defense mode currently is)
	//if true, g_iCommandMode is set to the mode specified in the current command's if block
	bool toggle = true;

	int mode_new = g_iCommandMode;
	if (Q_strcmp( command, "MODE_POWER_PHYSEXP" ) == 0 )
	{
		mode_new = MODE_POWER_PHYSEXP;

		//string_t text = "Entered explosion mode...";
		//Q_strncpy(text, "Entered explosion mode...", sizeof( text ) );
		//engine->ClientCmd(VarArgs("zm_hudchat %s", text )); //qck: I can't even begin to describe my hatred for doing things this way, but I haven't found another way to print to the HUD, and I'm in a hurry.
		HudChatPrint("Entered explosion mode...");
	}
	else if (Q_strcmp( command, "MODE_POWER_SPOTCREATE" ) == 0 ) //LAWYER:  Spot create stuff
	{
		mode_new = MODE_POWER_SPOTCREATE;

		HudChatPrint("Entered hidden spawn mode...");
	}
	else if (Q_strcmp( command, "MODE_POWER_NIGHTVISION" ) == 0 )
	{
		mode_new = MODE_POWER_NIGHTVISION;
		HudChatPrint("Nightvision toggled");
		engine->ClientCmd("zm_nightvision");
	}
	else if (Q_strcmp( command, "MODE_POWER_DELETEZOMBIES" ) == 0 )
	{
		mode_new = MODE_POWER_DELETEZOMBIES;
		HudChatPrint("Expiring selected zombies...");
		engine->ClientCmd("zm_deletezombies");
	}
	else if (Q_strcmp( command, "MODE_DEFENSIVE" ) == 0 )
	{
		mode_new = MODE_DEFENSIVE;
		toggle = false;
		HudChatPrint("Selected zombies are now in defensive mode");
		engine->ClientCmd("zm_switch_to_defense");
	}
	else if (Q_strcmp( command, "MODE_OFFENSIVE" ) == 0 )
	{
		mode_new = MODE_OFFENSIVE;
		toggle = false;
		HudChatPrint("Selected zombies are now in offensive mode");
		engine->ClientCmd("zm_switch_to_offense");
	}
	else if (Q_strcmp( command, "MODE_AMBUSH_CREATE" ) == 0 )
	{
		mode_new = MODE_AMBUSH_CREATE;
		HudChatPrint("Creating ambush...");
	}
	else if (Q_strcmp( command, "MODE_CREATE_GROUP") == 0)
	{
		mode_new = MODE_DEFAULT;
		toggle = false;
		HudChatPrint("Creating group...");
		engine->ClientCmd("zm_createsquad");
	
	}
	else if (Q_strcmp( command, "MODE_GOTO_GROUP") == 0)
	{
		mode_new = MODE_DEFAULT;
		toggle = false;
		if(m_ZMControls->m_pZombieGroups->GetActiveItemUserData())
		{
			KeyValues* kv = m_ZMControls->m_pZombieGroups->GetActiveItemUserData() ;
			int serial = kv->GetInt("serial");
			DevMsg("Serial number of selected: %i\n", serial);
			HudChatPrint("Moving to group...");
			engine->ClientCmd(VarArgs("zm_gotosquad %i", serial));
		}
		//engine->ClientCmd("zm_createsquad");
	}
	else if (Q_strcmp( command, "MODE_SELECT_ALL") == 0)
	{
		mode_new = MODE_DEFAULT;
		toggle = false;
		HudChatPrint("Selected all zombies");
		engine->ClientCmd("zm_select_all");
	}
	else if (Q_strcmp( command, "MODE_JUMP_CEILING") == 0)
	{
		mode_new = MODE_DEFAULT;
		toggle = false;
		HudChatPrint("Ordering selected banshees to attempt a ceiling ambush...");
		engine->ClientCmd("zm_jumpceiling");
	}
	else if (Q_strcmp( command, "MODE_RALLY")  == 0)
	{
		mode_new = MODE_RALLY;
		//toggle = false;

		HudChatPrint("Creating rally point...");
	}
	else if (Q_strcmp( command, "MODE_TRAP")  == 0)
	{
		mode_new = MODE_TRAP;
		//toggle = false;

		HudChatPrint("Creating trap trigger...");
	}
	else if (Q_strcmp( command, "TAB_POWERS" ) == 0 )
	{
		//no msg
		//mode_new = TAB_POWERS;
		toggle = false;
		
		if (m_ZMControls)
			m_ZMControls->UpdateTabs ( TAB_POWERS );
	}
	else if (Q_strcmp( command, "TAB_MODES" ) == 0 )
	{
		//no msg
		//mode_new = TAB_MODES;
		toggle = false;
		
		if (m_ZMControls)
			m_ZMControls->UpdateTabs ( TAB_MODES );
	}
	else if (Q_strcmp( command, "TAB_ZEDS") == 0 )
	{
		//no msg
		//mode_new = TAB_MODES;
		toggle = false;
		
		if (m_ZMControls)
			m_ZMControls->UpdateTabs ( TAB_ZEDS );
	}

	if ( g_iCommandMode != mode_new)
	{
		if (toggle)
			g_iCommandMode = mode_new;
	}

//	BaseClass::OnCommand(command);
}