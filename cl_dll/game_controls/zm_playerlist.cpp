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
// Purpose: Panel listing players and their ZM weighting, and presenting related options
//
// $NoKeywords: $
//=============================================================================//


#include "cbase.h"
#include "zm_playerlist.h"
#include "c_hl2mp_player.h"
#include "c_playerresource.h"
#include <vgui_controls/Frame.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/ListPanel.h>
#include "vgui/ivgui.h"
//#include "commandmenu.h"

#include "filesystem.h"

//for zm_menus_use_keyboard
#include "manipulatemenu.h"
//for zm_participate_saved
#include "startmenu.h"

#include <KeyValues.h>

#include "voice_status.h"

//#include <vgui/IScheme.h>
//#include <vgui/ILocalize.h>
//#include <vgui/ISurface.h>
//
//#include <vgui_controls/Label.h>
#include <vgui_controls/CheckButton.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//using namespace vgui;

#define LIST_UPDATE_DELAY 5.0

//concommand now
//ConVar zm_participate("zm_participate", "0", FCVAR_CLIENTDLL, "Participation type");
ConVar cl_showzmlist("cl_showzmlist", "0", FCVAR_CLIENTDLL, "Sets the state of ZMPlayerList <state>");

CON_COMMAND(ToggleZMList, "Toggles ZMPlayerList on or off")
{
	DevMsg("ZM list is %i\n", cl_showzmlist.GetInt());
	cl_showzmlist.SetValue(!cl_showzmlist.GetInt());
};

// Constuctor: Initializes the Panel
CZMPlayerList::CZMPlayerList(vgui::VPANEL parent)
: BaseClass( NULL, "ZMPlayerList")
{
	
	SetParent( parent );
	
	SetKeyBoardInputEnabled( true );
	SetMouseInputEnabled( true );
	
	SetProportional( false );

	SetTitleBarVisible( true );
	SetMinimizeButtonVisible( false );
	SetMaximizeButtonVisible( false );
//	SetCloseButtonVisible( true );
	
	SetSizeable( false );
	SetMoveable( true );
	SetVisible( false );

	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFile("resource/ZombieMaster.res", "ZombieMaster");
	//vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFile("resource/ClientScheme.res", "ClientScheme");
	SetScheme(scheme);


	vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );

	SetTitle("ZM Game Information and Settings", true);

	//playerlist = dynamic_cast<vgui::SectionedListPanel *>(FindChildByName("zmlist"));
	//playerlist = new SectionedListPanel( this, "zmlist" );
	playerlist = new vgui::ListPanel( this, "zmlist" );

	usekeys = new vgui::CheckButton(this, "use_keyboard", "zm_menus_use_keyboard toggle");

	LoadControlSettings("resource/UI/ZMPlayerList.res");

	//usekeys = dynamic_cast<vgui::Button *>(FindChildByName("use_keyboard"));

	participate = dynamic_cast<vgui::ComboBox *>(FindChildByName("combo_type"));
	default_participate = dynamic_cast<vgui::ComboBox *>(FindChildByName("combo_default"));
	roundrestart = dynamic_cast<vgui::Button *>(FindChildByName("voterr"));
	if (!playerlist || !participate || !roundrestart || !usekeys || !default_participate)
	{
		Warning("ZMPlayerList creation failed!\n");
		return;
	}

	usekeys->SetSelected(zm_menus_use_keyboard.GetBool());	

	playerlist->AddColumnHeader( 0, "name", "Name", playerlist->GetWide() * 0.5);
	playerlist->AddColumnHeader( 1, "priority", "ZM order rating", playerlist->GetWide() * 0.3);
	playerlist->AddColumnHeader( 2, "mute", "Muted", playerlist->GetWide() * 0.2);

	playerlist->SetSortColumn( 1 );
	playerlist->SetAscending( false );

	UpdatePlayerInfo();

	//TGB NOTENOTE: best to use integers for the priority values, because floats == lots of decimals printed

	KeyValues *kv = new KeyValues("zmparticipate.res");
	//itemToLevel.Purge();
	if  ( kv->LoadFromFile( (IBaseFileSystem*)filesystem, "resource/zmparticipate.res", "MOD" ) )
	{
		KeyValues *pKey = kv->GetFirstSubKey();
		while (pKey)
		{
			int itemID = participate->AddItem( pKey->GetString( "label", "Unset" ), pKey);

			//build mapping betwee itemID and participation level
			//part_mapping.Insert(pKey->GetInt("value", 0), itemID);
			part_mapping[pKey->GetInt("value", 0)] = itemID;

			pKey = pKey->GetNextKey();
		}

		kv->deleteThis();
	}

	participate->ActivateItemByRow( 0 );

	kv = new KeyValues("zmparticipate_default.res");
	//itemToLevel.Purge();
	if  ( kv->LoadFromFile( (IBaseFileSystem*)filesystem, "resource/zmparticipate_default.res", "MOD" ) )
	{
		KeyValues *pKey = kv->GetFirstSubKey();
		while (pKey)
		{
			int itemID = default_participate->AddItem( pKey->GetString( "label", "Unset" ), pKey);

			//build mapping betwee itemID and participation level
			//part_mapping.Insert(pKey->GetInt("value", 0), itemID);
			defpart_mapping[pKey->GetInt("value", 0)] = itemID;

			pKey = pKey->GetNextKey();
		}

		kv->deleteThis();
	}

	default_participate->ActivateItemByRow( 0 );

	int level = zm_participate_saved.GetInt();
	if (level < 0)
		level = 3; //remap, because array can't handle negative indices

	if (level < 4)
		default_participate->ActivateItem(defpart_mapping[level]);


	bRoundRestart = false;

	DevMsg("Created ZM game info menu!\n");

}

void CZMPlayerList::UpdatePlayerInfo()
{
	  
	if ( !playerlist || !IsVisible() )		return;

	//vgui::Panel *temp = FindChildByName("use_keyboard");
	//DevMsg(temp->GetName());

	//TGB: save our selected items so our rebuild won't interfere with selection
	const int numSelected = playerlist->GetSelectedItemsCount();
	CUtlVector<int> savedSelection;
	for (int i=0; i < numSelected; i++ )
	{
		const int itemID = playerlist->GetSelectedItem( i );
		savedSelection.AddToHead(itemID);
	}

	int currentID = -1;
	// walk all the players and make sure they're in the list
	for ( int i = 1; i < gpGlobals->maxClients; i++ )
	{
		C_PlayerResource *gr = g_PR;

		if ( gr && gr->IsConnected( i ) )
		{
			// add the player to the list
			KeyValues *playerData = new KeyValues("data");
		
			const char *newName = UTIL_SafeName( gr->GetPlayerName( i ) );

			playerData->SetInt("index", i);
			playerData->SetString("name", newName);
			playerData->SetInt("priority", gr->GetZMPriority(i) );
			
			delete newName;

			if (GetClientVoiceMgr()->IsPlayerBlocked( i ))
			{
				playerData->SetString( "mute", "Muted" );
			}
			
			

			//DevMsg("Player %s (%s) in list\n", newName, oldName);

			int itemID = playerlist->GetItemIDFromUserData( i );
  			//int sectionID = gr->GetTeam( i );
			
			if ( gr->IsLocalPlayer( i ) )
			{
				currentID = itemID;

				//TGB: bit of a hijack to enable the RR button, but it's efficient
				if ( gr->GetZMVoteRR(i) == 0 && roundrestart)
					roundrestart->SetEnabled(true);
			}
			if (itemID == -1)
			{
				// add a new row
				
				itemID = playerlist->AddItem( playerData, i, false, true );
			}
			else
			{
				// modify the current row
				//playerlist->ModifyItem( itemID, sectionID, playerData );
				//itemID = playerlist->GetItemIDFromUserData( i );
				playerlist->RemoveItem(itemID);
				itemID = playerlist->AddItem( playerData, i, false, true );
			}
			playerData->deleteThis();
		}
		else
		{
			// remove the player
			int itemID = playerlist->GetItemIDFromUserData( i );
			if (itemID != -1)
			{
				playerlist->RemoveItem(itemID);
			}
		}
	}

	//TGB: restore selection
	for (int i=0; i < savedSelection.Count(); i++ )
	{
		playerlist->AddSelectedItem( savedSelection[i] );
	}
	
	m_flLastUpdate = gpGlobals->curtime;
}


void CZMPlayerList::doSettings()
{
	if ( !playerlist || !participate || !default_participate )		return;

	KeyValues *kv = participate->GetActiveItemUserData();
	engine->ClientCmd( kv->GetString( "command" ) );

	kv = default_participate->GetActiveItemUserData();
	engine->ClientCmd( kv->GetString( "command" ) );

	playerlist->SortList();

}

//TGB: right, post-merge this simply never gets called
void CZMPlayerList::OnTick()
{
	BaseClass::OnTick();

	//SetVisible(cl_showzmlist.GetBool());
	const bool visible = cl_showzmlist.GetBool();
	if (IsVisible() != visible)
	{
		//did we just turn visible?
		if (visible)
			InitMenus();
		
        SetVisible(visible);		
	}
	
	
	if ( IsVisible() && (m_flLastUpdate + LIST_UPDATE_DELAY) < gpGlobals->curtime)
		UpdatePlayerInfo();
}

void CZMPlayerList::OnCommand(const char* pcCommand)
{
	//DevMsg("PlayerList command: %s\n", pcCommand);
	if(!Q_stricmp(pcCommand, "close"))
	{
		doSettings();
		cl_showzmlist.SetValue(0);
	}
	else if (!Q_stricmp(pcCommand, "voterr"))
	{
		C_PlayerResource *gr = g_PR;
		C_BasePlayer *pPlayer =	C_BasePlayer::GetLocalPlayer();

		if ( !pPlayer ) return;

		const int index = pPlayer->entindex();

		//LAWYER:  This is where we send off Round Restart flags.
		//TGB: prevent multiple voting/spamming
		if ( gr->GetZMVoteRR(index) == 0 && roundrestart)
		{
			roundrestart->SetEnabled(false);
			bRoundRestart = true;
			engine->ClientCmd( "ZM_VoteRoundRestart");
		}
	}
	//TGB: try muting this guy
	else if (!Q_stricmp(pcCommand, "muteplayer"))
	{
		if (!playerlist || !GetClientVoiceMgr()) return;

		const int numSelected = playerlist->GetSelectedItemsCount();

		for (int i=0; i < numSelected; i++ )
		{
			const int itemID = playerlist->GetSelectedItem( i );
			KeyValues *kv = playerlist->GetItem( itemID );
			if ( !kv )
				continue;
			
			const int index = kv->GetInt( "index" );
			
			const bool muted = GetClientVoiceMgr()->IsPlayerBlocked( index );
			GetClientVoiceMgr()->SetPlayerBlockedState( index, !muted );

			if (!muted)
				DevMsg("Muted player %i\n", index);
			else
				DevMsg("Unmuted player %i\n", index);
		}

		UpdatePlayerInfo();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Forces an update
//-----------------------------------------------------------------------------
void CZMPlayerList::OnThink()
{
	//TGBNOTE: isVisible check unneeded, OnThink is only called when we're visible

	/* TGB:
		m_flLastUpdate wasn't reset when disconnecting and reconnecting, while the curtime is.	
		As such the panel would refuse to update for a while because LastUpdate > curtime.
		We now make sure the next update isn't scheduled overly far in the future.
	*/
	const float diff = (m_flLastUpdate + LIST_UPDATE_DELAY) - gpGlobals->curtime;
	if ( diff < 0 || diff > LIST_UPDATE_DELAY )
		UpdatePlayerInfo();
		
	
//	if (m_fNextHackedTick >= gpGlobals->curtime) //LAWYER:  For the time being, this will have to do
//	{
//		UpdatePlayerInfo();

//		CBasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

//	}
}

//--------------------------------------------------------------
// 
//--------------------------------------------------------------
void CZMPlayerList::OnUseKeysChecked( KeyValues *data )
{
	if (!data) return;
	int state = data->GetInt("state", 0);
	zm_menus_use_keyboard.SetValue(state);	
}

//--------------------------------------------------------------
// TGB: like ShowPanel but without being a viewport panel 
//--------------------------------------------------------------
void CZMPlayerList::InitMenus()
{
	DevMsg("ZM game info just became visible, setting up menus...\n");

	//TGB: update participation level
	if (participate)
	{
		C_HL2MP_Player *pPlayer = C_HL2MP_Player::GetLocalHL2MPPlayer();

		//TGB: would crash if disconnecting with menu open
		if (!pPlayer) 
			return;

		const int level = pPlayer->GetZMParticipation();
		if (level < 3 &&											// level 3 is the "haven't picked yet" one
			part_mapping[level] != participate->GetActiveItem())	// see if the menu matches reality
		{
			participate->ActivateItem(part_mapping[level]);
		}

	}

	if (default_participate)
	{
		int level = zm_participate_saved.GetInt();
		if (level < 0)
			level = 3; //remap, because array can't handle negative indices

		if (level < 4 && defpart_mapping[level] != default_participate->GetActiveItem())	// see if the menu matches reality
		{
			default_participate->ActivateItem(defpart_mapping[level]);
		}
	}


	if (usekeys)
		usekeys->SetSelected(zm_menus_use_keyboard.GetBool());	
}