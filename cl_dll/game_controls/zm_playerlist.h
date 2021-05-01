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

#ifndef ZMPLAYERLIST_H
#define ZMPLAYERLIST_H
#ifdef _WIN32
#pragma once
#endif

#include "i_zm_playerlist.h"
#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ComboBox.h>
class CZMPlayerList : public vgui::Frame
{
public:
	DECLARE_CLASS_SIMPLE(CZMPlayerList, vgui::Frame); 
	//CZMPlayerList : This Class / vgui::Frame : BaseClass

	CZMPlayerList(vgui::VPANEL parent); 	// Constructor
	~CZMPlayerList()	{};				// Destructor
	
	void	UpdatePlayerInfo();

	void InitMenus();

protected:
	//VGUI overrides:
	virtual void OnTick();
	virtual void OnThink();
	virtual void OnCommand(const char* pcCommand);

private:
	
	void		doSettings();

	//TGB: listen for checkbutton changes
	MESSAGE_FUNC_PARAMS( OnUseKeysChecked, "CheckButtonChecked", data );


	//Other used VGUI control Elements:
	vgui::ComboBox				*participate;
	vgui::ComboBox				*default_participate;
	//vgui::SectionedListPanel	*playerlist;
	vgui::ListPanel	*playerlist;
	vgui::Button	*roundrestart;

	vgui::CheckButton *usekeys;

	bool bRoundRestart;

	float m_flLastUpdate;

	int part_mapping[3]; //TGB: maps participation level to itemID
	int defpart_mapping[4];

};


//Class: CZMPlayerListInterface Class. Used for construction.
class CZMPlayerListInterface : public IZMPlayerList
{
private:
	CZMPlayerList *ZMPlayerList;
public:
	CZMPlayerListInterface()
	{
		ZMPlayerList = NULL;
	}
	void Create(vgui::VPANEL parent)
	{
		ZMPlayerList = new CZMPlayerList(parent);
	}
	void Destroy()
	{
		if (ZMPlayerList)
		{
			ZMPlayerList->SetParent( (vgui::Panel *)NULL);
			delete ZMPlayerList;
		}
	}
};
static CZMPlayerListInterface g_ZMPlayerList;
IZMPlayerList* zm_playerlist = (IZMPlayerList*)&g_ZMPlayerList;

#endif // BUILDMENU_H