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

#ifndef BUILDMENU_H
#define BUILDMENU_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>

#include <cl_dll/iviewport.h>

#include <vgui/KeyCode.h>
#include <UtlVector.h>

//TGB: for zm_menus_use_keyboard
#include "manipulatemenu.h"

class TeamFortressViewport;
class IImage;

extern ConVar zm_zombiemax;

#define BM_QUEUE_SIZE 10

//indices of the button/image/price lists
enum {
	TYPE_SHAMBLER = 0,
	TYPE_BANSHEE,
	TYPE_HULK,
	TYPE_DRIFTER,
	TYPE_IMMOLATOR,

	TYPE_TOTAL,
	TYPE_INVALID = -1
};

struct ZombieCost {
	int resources;
	int population;

	ZombieCost(int res = 0, int pop = 0)
		:resources(res), population(pop)
	{
	}
};

//-----------------------------------------------------------------------------
// Purpose: Displays the build menu
//-----------------------------------------------------------------------------
class CBuildMenu : public vgui::Frame, public IViewPortPanel
{
private:
	DECLARE_CLASS_SIMPLE( CBuildMenu, vgui::Frame );

public:
	CBuildMenu(IViewPort *pViewPort);
	~CBuildMenu();

	const char *GetName( void ) { return PANEL_BUILD; }
	void SetData(KeyValues *data) {};
	void Reset() {};
	void Update();
	bool NeedsUpdate( void ) { return false; }
	bool HasInputElements( void ) { return true; }
	void ShowPanel( bool bShow );

	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel( void ) { return BaseClass::GetVPanel(); }
  	bool IsVisible() { return BaseClass::IsVisible(); }
	void SetParent( vgui::VPANEL parent ) { BaseClass::SetParent( parent ); }

	vgui::HFont m_hMediumFont;
	vgui::HFont m_hLargeFont;
	//virtual void Paint();
	void CalculateButtonState();
	void OnThink( void );
	void OnClose();

	void AutoAssign();
	int m_iLastFlags;
	
	void UpdateQueue(const int q[], int size = BM_QUEUE_SIZE);

	void SetCurrentSpawn(int idx);

	void OnKeyCodePressed(vgui::KeyCode code);

protected:
	
	void			DoClose();


	ZombieCost		GetCostForType(int type) const;
	void			ShowZombieInfo(int type);


	// helper functions
	void			SetLabelText(const char *textEntryName, const char *text);
	void			GetLabelText(const char* element, char *buffer, int buflen);
	
	// command callbacks
	void			OnCommand( const char *command ); //LAWYER: (BUILD MENU) Adds command callbackishness

	int				m_iCurrentSpawn; //what we're showing the menu for right now

	IViewPort		*m_pViewPort;
	vgui::HScheme	scheme;

	int				m_iJumpKey;
	//int				m_iScoreBoardKey;

	vgui::Panel		*spawnbuttons[TYPE_TOTAL];
	vgui::Panel		*spawnfives[TYPE_TOTAL];
	vgui::IImage	*zombieimages[TYPE_TOTAL];
	vgui::IImage	*zombiequeue[TYPE_TOTAL];
	char*			zombiedescriptions[TYPE_TOTAL];

	//unit info area
	vgui::ImagePanel *info_image;
	vgui::Label		*info_rescost;
	vgui::Label		*info_popcost;
	vgui::Label		*info_description;

	//queue
	//CUtlLinkedList<vgui::ImagePanel*> queueimages; //we'll be allocating these
	vgui::ImagePanel *queueimages[BM_QUEUE_SIZE];

	vgui::Button	*removelast;
	vgui::Button	*clearqueue;
};

/*TGB: unused
//-----------------------------------------------------------------------------
// Purpose: Displays the alternate build menu
//-----------------------------------------------------------------------------
class CAltBuildMenu : public vgui::Frame, public IViewPortPanel
{
private:
	DECLARE_CLASS_SIMPLE( CAltBuildMenu, vgui::Frame );

public:
	CAltBuildMenu(IViewPort *pViewPort);
	virtual ~CAltBuildMenu();

	virtual const char *GetName( void ) { return PANEL_ALTBUILD; }
	virtual void SetData(KeyValues *data) {};
	virtual void Reset() {};
	virtual void Update();
	virtual bool NeedsUpdate( void ) { return false; }
	virtual bool HasInputElements( void ) { return true; }
	virtual void ShowPanel( bool bShow );

	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel( void ) { return BaseClass::GetVPanel(); }
  	virtual bool IsVisible() { return BaseClass::IsVisible(); }
	virtual void SetParent( vgui::VPANEL parent ) { BaseClass::SetParent( parent ); }

public:
	
	void AutoAssign();
	
protected:

	
	// VGUI2 overrides
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void OnKeyCodePressed(vgui::KeyCode code);

	// helper functions
	virtual void SetLabelText(const char *textEntryName, const char *text);
	
	
	// command callbacks
	void OnCommand( const char *command ); //LAWYER: (BUILD MENU) Adds command callbackishness
	// MESSAGE_FUNC_INT( OnBuildButton, "BuildButton", build );

	IViewPort	*m_pViewPort;

	int m_iJumpKey;
	int m_iScoreBoardKey;

};
*/
#endif // BUILDMENU_H
