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
//#include <cdll_client_int.h>

#include "buildmenu.h"

#include "c_hl2mp_player.h"

#include <vgui/IVGui.h>
#include <vgui/IScheme.h>
#include <KeyValues.h>
#include <vgui_controls/ImagePanel.h>

#include "filesystem.h"

#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Image.h>

#include "vgui_bitmapbutton.h"

#include "IGameUIFuncs.h" // for key bindings
#include <igameresources.h>

#include "c_user_message_register.h"

extern IGameUIFuncs *gameuifuncs; // for key binding details

#include <cl_dll/iviewport.h>
/*
#include <stdlib.h> // MAX_PATH define
#include <stdio.h>*/

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//using namespace vgui; //don't do this, causes problems with "filesystem" object

//TGB: for usermessage listener efficiency
CBuildMenu *gBuildMenu = NULL;

//TGB: replicate some cvars of zombie costs from server
ConVar zm_cost_shambler( "zm_cost_shambler", "10", FCVAR_NOTIFY | FCVAR_REPLICATED);
ConVar zm_cost_banshee( "zm_cost_banshee", "70", FCVAR_NOTIFY | FCVAR_REPLICATED);
ConVar zm_cost_hulk( "zm_cost_hulk", "60", FCVAR_NOTIFY | FCVAR_REPLICATED);
ConVar zm_cost_drifter( "zm_cost_drifter", "25", FCVAR_NOTIFY | FCVAR_REPLICATED);
ConVar zm_cost_immolator( "zm_cost_immolator", "100", FCVAR_NOTIFY | FCVAR_REPLICATED);

ConVar zm_popcost_banshee("zm_popcost_banshee", "5", FCVAR_NOTIFY | FCVAR_REPLICATED);
ConVar zm_popcost_hulk("zm_popcost_hulk", "4", FCVAR_NOTIFY | FCVAR_REPLICATED);
ConVar zm_popcost_shambler("zm_popcost_shambler", "1", FCVAR_NOTIFY | FCVAR_REPLICATED);
ConVar zm_popcost_immolator("zm_popcost_immolator", "5", FCVAR_NOTIFY | FCVAR_REPLICATED);
ConVar zm_popcost_drifter("zm_popcost_drifter", "3", FCVAR_NOTIFY | FCVAR_REPLICATED);

ConVar zm_zombiemax("zm_zombiemax", "50", FCVAR_ARCHIVE|FCVAR_NOTIFY|FCVAR_REPLICATED, "Sets maximum number of zombies the ZM is allowed to have active at once. Works like typical unit limit in RTS games.");

const char *TypeToImage[TYPE_TOTAL] = {
		"zombies/info_shambler",
		"zombies/info_banshee",
		"zombies/info_hulk",
		"zombies/info_drifter",
		"zombies/info_immolator"
};

const char *TypeToQueueImage[TYPE_TOTAL] = {
		"zombies/queue_shambler",
		"zombies/queue_banshee",
		"zombies/queue_hulk",
		"zombies/queue_drifter",
		"zombies/queue_immolator",
};

//TGB: if we ever go above 255 here, increase networking bits for LastZombieflags in baseplayer
#define BURNZOMBIE_FLAG 16
#define DRAGZOMBIE_FLAG 8
#define HULK_FLAG 4  
#define FASTIE_FLAG 2
#define SHAMBLIE_FLAG 1

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CBuildMenu::CBuildMenu(IViewPort *pViewPort) : Frame(NULL, PANEL_BUILD )
{
	gBuildMenu = this;

	m_pViewPort = pViewPort;
	m_iJumpKey = -1; // this is looked up in Activate()
//	m_iScoreBoardKey = -1; // this is looked up in Activate()

	// initialize dialog
	SetTitle("Spawn Menu", true);

	// load the new scheme early!!
	scheme = vgui::scheme()->LoadSchemeFromFile("resource/ZombieMaster.res", "ZombieMaster");
	SetScheme(scheme);
	SetSizeable(false);

	m_hLargeFont = vgui::scheme()->GetIScheme(scheme)->GetFont("Trebuchet18", true);
	m_hMediumFont = vgui::scheme()->GetIScheme(scheme)->GetFont("Trebuchet16", true);

	SetProportional(false);

	LoadControlSettings("Resource/UI/BuildMenu.res");

	vgui::ivgui()->AddTickSignal(GetVPanel(), 150);

	//TGB: moved here because the .res seemed to override it
	SetMoveable(true);
	InvalidateLayout();

	//we fetch a bunch of pointers to various elements here so we can alter them quickly and easily
	info_image = dynamic_cast<vgui::ImagePanel*>(FindChildByName("ZombieImage"));

	info_rescost = dynamic_cast<vgui::Label*>(FindChildByName("CostRes"));
	info_popcost = dynamic_cast<vgui::Label*>(FindChildByName("CostPop"));
	info_description = dynamic_cast<vgui::Label*>(FindChildByName("LabelDescription"));

	removelast =  dynamic_cast<vgui::Button*>(FindChildByName("RemoveLast"));
	clearqueue =  dynamic_cast<vgui::Button*>(FindChildByName("ClearQueue"));

	//prepare a list of our spawn buttons etc so we can easily iterate over them
	for (int i=0; i < TYPE_TOTAL; i++)
	{
		char buffer[25];
		Q_snprintf(buffer, sizeof(buffer), "z_spawn1_%02d", i);
		spawnbuttons[i] = FindChildByName(buffer);

		Q_snprintf(buffer, sizeof(buffer), "z_spawn5_%02d", i);
		spawnfives[i] = FindChildByName(buffer);

		zombieimages[i] = vgui::scheme()->GetImage(TypeToImage[i], true);
		zombiequeue[i] = vgui::scheme()->GetImage(TypeToQueueImage[i], false);
	
	}
	

	KeyValues *kv = new KeyValues("zombiedesc.res");
	if  ( kv->LoadFromFile( (IBaseFileSystem*)filesystem, "resource/zombiedesc.res", "MOD" ) )
	{
		//braaaaaaah, char juggling is pain

		const char *temp = kv->GetString("shambler", "Shambler");
		int length = 128;
		char *saved = new char[length];
		Q_strncpy(saved, temp, strlen(temp) + 1);
		zombiedescriptions[TYPE_SHAMBLER] = saved;

		temp = kv->GetString("banshee", "Banshee");
		saved = new char[length];
		Q_strncpy(saved, temp, strlen(temp) + 1);
		zombiedescriptions[TYPE_BANSHEE] = saved;

		temp = kv->GetString("hulk", "Hulk");
		saved = new char[length];
		Q_strncpy(saved, temp, strlen(temp) + 1);
		zombiedescriptions[TYPE_HULK] = saved;

		temp = kv->GetString("drifter", "Drifter");
		saved = new char[length];
		Q_strncpy(saved, temp, strlen(temp) + 1);
		zombiedescriptions[TYPE_DRIFTER] = saved;

		temp = kv->GetString("immolator", "Immolator");
		saved = new char[length];
		Q_strncpy(saved, temp, strlen(temp) + 1);
		zombiedescriptions[TYPE_IMMOLATOR] = saved;
	}
	//will delete its child keys as well
	kv->deleteThis();

	for (int i=0; i < BM_QUEUE_SIZE; i++)
	{
		char buffer[10];
		Q_snprintf(buffer, sizeof(buffer), "queue%02d", i);
		queueimages[i] = dynamic_cast<vgui::ImagePanel*>(FindChildByName(buffer));
	}
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CBuildMenu::~CBuildMenu()
{
	for (int i=0; i < TYPE_TOTAL; i++)
	{
		if (zombiedescriptions[i])
			delete zombiedescriptions[i];
	}

}

void CBuildMenu::Update()
{

}


//-----------------------------------------------------------------------------
// Purpose: shows the build menu
//-----------------------------------------------------------------------------
void CBuildMenu::ShowPanel(bool bShow)
{
//	CBasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( BaseClass::IsVisible() == bShow )
		return;


	if ( bShow )
	{
		//TGB: manipmenu cannot be open open as it will conflict
		m_pViewPort->ShowPanel( PANEL_MANIPULATE, false );
		//viewport has to go too, or the build menu can sometimes become unresponsive to commands
		m_pViewPort->ShowPanel( PANEL_VIEWPORT, false );

		//TGB: update costs if necessary
		/*we print these live from the cvar now
		if (NeedCostsUpdate())
		{
			LoadControlSettings("Resource/UI/BuildMenu.res");
			PrintAllCosts();

			//it would be nice to save away the original unprinted strings for easy cost updating without reloading the .res
			//however, a change of zombiecosts should not happen often at all, at most once for each time you join a server
			
			DevMsg("Reloaded buildmenu\n");
		}*/

		//LAWYER:  ZombieFlags stuff
		CalculateButtonState();

		Activate();

		SetMouseInputEnabled( true ); 

		SetKeyBoardInputEnabled( zm_menus_use_keyboard.GetBool() );
	
		//// get key bindings if shown

		if( m_iJumpKey < 0 ) // you need to lookup the jump key AFTER the engine has loaded
		{
			m_iJumpKey = gameuifuncs->GetEngineKeyCodeForBind( "jump" ); 
		}

		//if ( m_iScoreBoardKey < 0 ) 
		//{
		//	m_iScoreBoardKey = gameuifuncs->GetEngineKeyCodeForBind( "showscores" );
		//}
	}
	else
	{
		SetVisible( false );
		SetMouseInputEnabled( false );

		//bring viewport back up
		C_HL2MP_Player *pPlayer = C_HL2MP_Player::GetLocalHL2MPPlayer();
		if (pPlayer && pPlayer->IsZM())
		{
			//prevent edge cases with roundrestarts while panel is open
			m_pViewPort->ShowPanel( PANEL_VIEWPORT, true );
		}

	}

	//m_pViewPort->ShowBackGround( bShow );
}


//-----------------------------------------------------------------------------
// Purpose: Sets the text of a control by name
//-----------------------------------------------------------------------------
/*void CBuildMenu::SetLabelText(const char *textEntryName, const char *text)
{
	Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->SetText(text);
	}
}

void CBuildMenu::GetLabelText(const char* element, char *buffer, int buflen)
{
	Label *entry = dynamic_cast<Label *>(FindChildByName(element));
	if (entry)
	{
		entry->GetText(buffer, buflen);
	}
}

void CBuildMenu::ReloadLabel( const char* element )
{
	Label *entry = dynamic_cast<Label *>(FindChildByName(element));
	if (entry)
	{
		entry->InvalidateLayout(true, true);
	}
}

void CBuildMenu::OnKeyCodePressed(KeyCode code)
{
		BaseClass::OnKeyCodePressed( code );
}
*/

//LAWYER: (BUILD MENU) Define the pressing of buttons
void CBuildMenu::OnCommand( const char *command )
{
	if (Q_stricmp( command, "vguicancel" ) != 0)
	{
		//TGB: some special commands need the current spawn idx
		if (Q_strnicmp( command, "summon", 6 ) == 0 || //both summon commands
			Q_strnicmp( command, "buildmenu_", 10 ) == 0) //remove last and clear queue commands
		{
			char buffer[256];
			Q_snprintf(buffer, sizeof(buffer), command, m_iCurrentSpawn);
			engine->ClientCmd( const_cast<char *>( buffer ) );
			return;
		}
		else if (Q_stricmp( command, "toggle_rally") == 0)
		{
			//engine->ClientCmd( const_cast<char *>( command ) );

			//TGB: instead of going through server, send a rallymode cmd to vgui_viewport
			KeyValues *msg = new KeyValues("ButtonCommand");
			msg->SetString("command", "MODE_RALLY");
			IViewPortPanel *pview = gViewPortInterface->FindPanelByName(PANEL_VIEWPORT);
			if (pview)
				PostMessage(pview->GetVPanel(), msg);

			//no return here means the buildmenu will close, as it should in this case
		}
		else
		{
			engine->ClientCmd( const_cast<char *>( command ) );
			return;
		}
	}

	DoClose();

	BaseClass::OnCommand(command);
}

/*
void CBuildMenu::Paint()
{
	vgui::surface()->DrawSetColor(  255, 0, 0, 255 ); //RGBA
	vgui::surface()->DrawFilledRect( 0, 0, 20, 20 ); //x0,y0,x1,y1
}
*/


//-----------------------------------------------------------------------------
// Purpose: Forces an update
//-----------------------------------------------------------------------------
void CBuildMenu::OnThink()
{
	//	CBasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( BaseClass::IsVisible() == false )
		return;

	CalculateButtonState();

	for (int i = 0; i < TYPE_TOTAL; i++)
	{
		if ((spawnbuttons[i] && spawnbuttons[i]->IsCursorOver()) ||
			(spawnfives[i] && spawnfives[i]->IsCursorOver()))
		{
			ShowZombieInfo(i);
		}
	}

	//TGB: force close if not ZM
	C_HL2MP_Player *pPlayer = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (pPlayer && pPlayer->IsZM() == false)
	{
		//prevent edge cases with roundrestarts while panel is open
		DoClose();
	}
}


//TGB: renamed from CalculateFlags to reflect increased functionality
void CBuildMenu::CalculateButtonState()
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();  //LAWYER:  Get the local player
	if (pPlayer)
	{
		//TODO: usermessage-ify
		if (m_iLastFlags == pPlayer->m_iLastZombieFlags)
			return;

		bool button_states[TYPE_TOTAL]; //five buttons

		m_iLastFlags = pPlayer->m_iLastZombieFlags;

		//TGB: if the flags are 0/unset, all zombies should be available
		//so changed from != 0 to == 0
		if  (pPlayer->m_iLastZombieFlags == 0)
		{
			for (int type=0; type < TYPE_TOTAL; type++)
				button_states[type] = true;
		}
		else
		{
			//Someone's defined ZombieFlags here, so start disabling things
			for (int type=0; type < TYPE_TOTAL; type++)
				button_states[type] = false;

			int iCalculation = pPlayer->m_iLastZombieFlags;

			//Burnzombies
			if (iCalculation - BURNZOMBIE_FLAG >= 0)
			{
				iCalculation -= BURNZOMBIE_FLAG;
				button_states[TYPE_IMMOLATOR] = true;
			}
			//Dragzombies
			if (iCalculation - DRAGZOMBIE_FLAG >= 0)
			{
				iCalculation -= DRAGZOMBIE_FLAG;
				button_states[TYPE_DRIFTER] = true;
			}
			//Hulks
			if (iCalculation - HULK_FLAG >= 0)
			{
				iCalculation -= HULK_FLAG;
				button_states[TYPE_HULK] = true;
			}
			//Fasties
			if (iCalculation - FASTIE_FLAG >= 0)
			{
				iCalculation -= FASTIE_FLAG;
				button_states[TYPE_BANSHEE] = true;
			}
			//Shamblies
			if (iCalculation - SHAMBLIE_FLAG >= 0)
			{
				iCalculation -= SHAMBLIE_FLAG;
				button_states[TYPE_SHAMBLER] = true;
			}
		}

		//TGB: we now have the current state of the buttons as the flags set them
		//	now we can just disable the ones that we don't have the res or pop for

		//TGB: while writing this I decided that it should be possible to queue up zombies you can't yet pay for
		/*
		int pool = pPlayer->m_iZombiePool;
		int pop = pPlayer->m_iZombiePopCount;
		int popmax = zm_zombiemax.GetInt();
		for (int type=0; type < TYPE_TOTAL; type++)
		{
			ZombieCost cost = GetCostForType(type);

			if ((pool - cost.resources < 0) ||
				(pop + cost.population > popmax))
			{
				button_states[type] = false;
				continue;
			}
		}
		*/
		
		//TGB: apply state to buttons
		for (int type=0; type < TYPE_TOTAL; type++)
		{
			if (spawnbuttons[type])
				spawnbuttons[type]->SetEnabled(button_states[type]);

			if (spawnfives[type])
				spawnfives[type]->SetEnabled(button_states[type]);
			
		}
	}
}

// Grabs the pop and res costs for a given type
ZombieCost CBuildMenu::GetCostForType(int type) const
{
	switch(type)
	{
	case TYPE_SHAMBLER:
		return ZombieCost(zm_cost_shambler.GetInt(), zm_popcost_shambler.GetInt());
	case TYPE_BANSHEE:
		return ZombieCost(zm_cost_banshee.GetInt(), zm_popcost_banshee.GetInt());
	case TYPE_HULK:
		return ZombieCost(zm_cost_hulk.GetInt(), zm_popcost_hulk.GetInt());
	case TYPE_DRIFTER:
		return ZombieCost(zm_cost_drifter.GetInt(), zm_popcost_drifter.GetInt());
	case TYPE_IMMOLATOR:
		return ZombieCost(zm_cost_immolator.GetInt(), zm_popcost_immolator.GetInt());
	default:
		return ZombieCost(0, 0);
	}
}

//--------------------------------------------------------------
// TGB: show the zombie img and info in the middle area 
//--------------------------------------------------------------
void CBuildMenu::ShowZombieInfo( int type )
{
	if (!info_image || !info_rescost || !info_popcost || !info_description)
		return;

	info_image->SetImage(zombieimages[type]);

	ZombieCost cost = GetCostForType(type);

	char buffer[50];
	Q_snprintf(buffer, sizeof(buffer), "%d", cost.resources);
	info_rescost->SetText(buffer);

	Q_snprintf(buffer, sizeof(buffer), "%d", cost.population);
	info_popcost->SetText(buffer);

	info_description->SetText(zombiedescriptions[type]);
}

//--------------------------------------------------------------
// Update the queue images to reflect the types in the given array 
//--------------------------------------------------------------
void CBuildMenu::UpdateQueue( const int q[], int size /*= TYPE_TOTAL*/ )
{
	bool zombies_present = false;
	for (int i=0; i < size; i++)
	{
		const int type = q[i];

		if (!queueimages[i])
			return;

		// Is there a zombie queued at this spot?
		if (type > TYPE_INVALID && type < TYPE_TOTAL)
		{
			vgui::IImage *given_img = zombiequeue[type];

			if (given_img != queueimages[i]->GetImage())
			{
				//queueimages[i]->SetShouldScaleImage(true);
				queueimages[i]->SetImage(given_img);
			}
			queueimages[i]->SetVisible(true);

			zombies_present = true;
		}
		else
		{
			// no valid type, so don't draw an image
			queueimages[i]->SetVisible(false);
		}
		
	}

	if (removelast)
		removelast->SetEnabled(zombies_present);
	if (clearqueue)
		clearqueue->SetEnabled(zombies_present);
	
}

//--------------------------------------------------------------
// TGB: tell the server we're not showing a spawn's menu anymore 
//--------------------------------------------------------------
void CBuildMenu::OnClose()
{
	char buffer[50];
	Q_snprintf(buffer, sizeof(buffer), "buildmenu_closed %i", m_iCurrentSpawn);
	engine->ExecuteClientCmd(buffer);

	m_iCurrentSpawn = -1;

	BaseClass::OnClose();
}

//--------------------------------------------------------------
// TGB: we have this so that when keyboard grabbing is disabled for this menu we still have a close-menu-shortcut
//--------------------------------------------------------------

void CBuildMenu::OnKeyCodePressed(vgui::KeyCode code)
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
// 
//--------------------------------------------------------------
void CBuildMenu::SetCurrentSpawn( int idx )
{
	if (m_iCurrentSpawn != idx)
        m_iCurrentSpawn = idx;
}

//--------------------------------------------------------------
// Helper that closes the menu 
//--------------------------------------------------------------
void CBuildMenu::DoClose()
{
	Close();
	SetVisible( false );
	SetMouseInputEnabled( false );

	//bring viewport back up
	C_HL2MP_Player *pPlayer = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (pPlayer && pPlayer->IsZM())
	{
		//prevent edge cases with roundrestarts while panel is open
		m_pViewPort->ShowPanel( PANEL_VIEWPORT, true );
	}
	
	gViewPortInterface->ShowBackGround( false );
}


//--------------------------------------------------------------
// TGB: usermessage for updating the buildmenu
//--------------------------------------------------------------
void __MsgFunc_BuildMenuUpdate( bf_read &msg )
{
	//DevMsg("Received BuildMenuUpdate on client\n");

	if (gBuildMenu == NULL)
	{
		gBuildMenu = dynamic_cast<CBuildMenu *>(gViewPortInterface->FindPanelByName(PANEL_BUILD));
		if(gBuildMenu == NULL)
			return;
	}

	//read spawn entindex
	int spawnidx = msg.ReadShort();

	//read whether this is an opener msg or update msg
	bool force_open = (msg.ReadOneBit() == 1);

	//read queue from usermessage
	int queue[BM_QUEUE_SIZE];
	for (int i=0; i < BM_QUEUE_SIZE; i++)
	{
		//every type was increased by 1 so that type_invalid could be 0 (byte is unsigned)
		queue[i] = (msg.ReadByte() - 1);
	}

	if (force_open)
	{
		//if we weren't visible, this is also an opening message
		gViewPortInterface->ShowPanel(gBuildMenu, true);
	}

	gBuildMenu->SetCurrentSpawn(spawnidx);
	gBuildMenu->UpdateQueue(queue);

}

USER_MESSAGE_REGISTER(BuildMenuUpdate);
