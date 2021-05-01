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

#ifndef VGUI_VIEWPORT_H
#define VGUI_VIEWPORT_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui/ISurface.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>
#include <vgui/Mousecode.h>
#include <cl_dll/iviewport.h>
#include <vgui/KeyCode.h>
#include <UtlVector.h>
#include "hud_boxselect.h"

#include "iinput.h"

#include "vgui_BitmapButton.h"
#include "vgui_controls/ComboBox.h"

//TGB: this lets you use internalCenterPrint to print to the center of the HUD
#include "vguicenterprint.h"

#define BLACK_BAR_COLOR	Color(0, 0, 0, 196)

namespace vgui
{
	class RichText;
	class HTML;
}



//TGB: button stuff
//IMPORTANT: any additions here _must_ have an entry in:
// - buttonMat, in CZMControlsHandler::LoadButtons()
// - buttonCmd, as above
// - toolTip, as above
// - toolTipCosts, as above
// - buttonToTab, scroll down
//may want to turn all that into one big array of arrays at some point
enum Button { 
	BUTTON_POWER_PHYSEXP,
	BUTTON_POWER_NIGHTVISION,
	BUTTON_MODE_OFFENSIVE,
	BUTTON_MODE_DEFENSIVE,
	BUTTON_TOOL_SELECTALL,
	BUTTON_MODE_AMBUSH,
	BUTTON_GROUP_CREATE,
	BUTTON_GROUP_GOTO,
	BUTTON_POWER_SPOTCREATE, // LAWYER: Spot create stuff
	BUTTON_POWER_DELETEZOMBIES, // LAWYER: Spot create stuff
	BUTTON_MODE_CEILING, //fastie ceiling ambush
	NUM_BUTTONS,	//needs to be last! holds number of icons in enum
};

enum Tab {
	TAB_MODES,
	TAB_POWERS,
	TAB_ZEDS,
	NUM_TABS,
};

const int buttonToTab[NUM_BUTTONS] = 
{
	TAB_POWERS, //physexp
	TAB_POWERS, //night vision
	TAB_MODES, //defense
	TAB_MODES, //offense,
	TAB_MODES, //select all
	TAB_MODES, //ambush
	TAB_ZEDS, //create group
	TAB_ZEDS, //goto group
	TAB_POWERS, //spotcreate
	TAB_POWERS, //delete
	TAB_MODES //ceiling ambush
};


//qck: Need a way to keep an ear out for game wide events. 
class CZMPanelEventListener : public IGameEventListener2
{
public:
	CZMPanelEventListener(){};

	virtual void FireGameEvent(IGameEvent* pEvent){};
};

//-----------------------------------------------------------------------------
// Purpose: TGB: A simple class to handle the drawing of a zm controls panel onto CBaseZombieMasterViewPort
//-----------------------------------------------------------------------------
class CZMControlsHandler
{
public:
	CZMControlsHandler( vgui::Panel *pParent ); //pParent = viewport ptr for parenting buttons
	~CZMControlsHandler();

	//scaling and positioning
	void PositionButtons();
	void PositionComboBox();

	//groups
	void GroupsListUpdate();
	void RemoveGroup(int serial);

	//draw background
	void PaintControls( vgui::ISurface *surface);

	//update the active tab and visible buttons
	void UpdateTabs( int activatedTab = -1 );
	

	//qck: Keeps track of ents by handle serial numbers. They are unique, so there shouldn't be any problem.
	CUtlVector< int >	m_ComboBoxItems;
	vgui::ComboBox *m_pZombieGroups;

private:

	//create buttons
	void LoadButtons( vgui::Panel *pParent );
	//remove buttons
	void RemoveButtons();

	CBitmapButton *m_pButtons[NUM_BUTTONS];
	CBitmapButton *m_pTabs[NUM_TABS];

	int m_iActiveTab;

	//see CBaseZombieMasterViewPort constructor on why these are needed -> the SetBounds part
	static const int HOR_ADJUST = 8; //8
	static const int VER_ADJUST = 28; //28
	//base positioning values
	static const int BUTTON_SIZE = 32; //32 //40
	static const int BUTTON_SPACING = 10;
	static const int PANEL_SPACING = 5;

	//TGB: the constants here assumed a 4:3 resolution
	//TGB: to fix we can simply define desired panel size here, and use that in combo with ScreenWidth to place on right
	//static const int PANEL_TOPLEFT_X = 540 + HOR_ADJUST - PANEL_SPACING;
	//static const int PANEL_TOPLEFT_Y = 380 + VER_ADJUST - PANEL_SPACING;
	//static const int PANEL_BOTRIGHT_X = 640 + HOR_ADJUST - PANEL_SPACING;
	//static const int PANEL_BOTRIGHT_Y = 480 + VER_ADJUST - PANEL_SPACING;
	
	static const int PANEL_SIZE_X = 156;//128 //100
	static const int PANEL_SIZE_Y = 156;//128 //100

	static const int COMBO_BOX_X_OFFSET = 11;
	static const int COMBO_BOX_Y_OFFSET = 64;
	static const int COMBO_BOX_WIDTH = 100;
};

//--------------------------------------------------------------
// Purpose: used for keeping tabs on player's activity
//--------------------------------------------------------------
//see the .cpp
extern ConVar zm_idlecheck_warningtime;
extern ConVar zm_idlecheck_restarttime;

//TGB: struct that holds our idle data records
struct IdleCheckData
{
private:
	//recorded data
	QAngle	m_angLastEyeAngles;	//last eye angles
	Vector	m_vecLastPosition;	//last position
	int		m_iLastMouseX;		//last mouse X coord
	//We only track X because I find it highly unlikely that anyone can stay on one X column for t seconds while being furiously active.

	//the great activity clock
	float	m_flLastActivity;	//last time any activity (mouse, angles, pos) was detected

public:
	//init our goods
	IdleCheckData() : m_flLastActivity(0)
	{	
		m_angLastEyeAngles.Init();
		m_vecLastPosition.Init();
	}

	//we've been told the mouse has moved
	void	MouseMoved()	
	{ 
		//when we're told by the viewport the mouse has been moved, we already know there is activity
		m_flLastActivity = gpGlobals->curtime; 

		//however, the viewport doesn't see it when the player moves his mouse above a subpanel, like the build menu
		//so we still have to check a coord just in case

		int mousey; //unused
		::input->GetFullscreenMousePos( &m_iLastMouseX, &mousey );
	}

	//update our records
	void	UpdateAndCheck()
	{	
		CBasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
		if (!pPlayer) return;
	
		//grab data and compare
		//angle+pos
		const QAngle angles = pPlayer->EyeAngles();
		const Vector pos = pPlayer->GetAbsOrigin();
		//mousex
		int mousex;
		int mousey; //unused
		::input->GetFullscreenMousePos( &mousex, &mousey );

		if ( m_angLastEyeAngles != angles || m_vecLastPosition != pos || m_iLastMouseX != mousex )
		{	
			m_flLastActivity = gpGlobals->curtime;
			m_angLastEyeAngles = angles;
			m_vecLastPosition = pos;
			m_iLastMouseX = mousex;
		}
		else
		{
			if (pPlayer->IsZM())
			{
				const float idletime = gpGlobals->curtime - m_flLastActivity;
				//DevMsgRT("Idle time is %f\n", idletime);
				if ( idletime > zm_idlecheck_restarttime.GetFloat() )
					IdleRestart();
				else if ( idletime > zm_idlecheck_warningtime.GetFloat() )
					IdleWarning();
			}
		}

	}

	void IdleWarning()
	{
		internalCenterPrint->Print( "Warning: You appear to be idle. If you do not show a sign of activity soon the round will be restarted." );
	}

	void IdleRestart()
	{
		engine->ClientCmd("zm_idlecheck_roundrestart");
		m_flLastActivity = gpGlobals->curtime;
	}

};



//-----------------------------------------------------------------------------
// Purpose: Displays the team menu
//-----------------------------------------------------------------------------
class CBaseZombieMasterViewPort : public vgui::Frame, public IViewPortPanel, public CZMPanelEventListener
{
private:
	DECLARE_CLASS_SIMPLE( CBaseZombieMasterViewPort, vgui::Frame );
	int		m_iDragStatus;		//0 = none, 1 = mousedown, may be dragging, 2 = definite drag
	float	m_flDragTrigger;	//time until we trigger before starting to boxselect
	int		m_iDragMouseX, m_iDragMouseY;

	CHudBoxSelect	*m_HudBoxSelect;

	float	m_flLastTipTrace; //last tooltip trace, used to prevent unneeded tracefloods

	float	m_flLastClick;

	IdleCheckData idleData;


public:
	CZMControlsHandler *m_ZMControls;
	CZMPanelEventListener *m_ZMListen;

	CBaseZombieMasterViewPort(IViewPort *pViewPort);
	virtual ~CBaseZombieMasterViewPort();

	virtual const char *GetName( void ) { return PANEL_VIEWPORT; }
	virtual void SetData(KeyValues *data) {};
	virtual void Reset() {};
	virtual void Update() {};
	virtual bool NeedsUpdate( void ) { return false; }
	virtual bool HasInputElements( void ) { return true; }
	virtual void ShowPanel( bool bShow );
	void FireGameEvent(IGameEvent *pEvent);
	void OnThink();
	void OnTick();
	

	//both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui

	vgui::VPANEL GetVPanel( void ) { return BaseClass::GetVPanel(); }
  	virtual bool IsVisible() { return BaseClass::IsVisible(); }
	virtual void SetParent( vgui::VPANEL parent ) { BaseClass::SetParent( parent ); }

	void OnMousePressed	( vgui::MouseCode code );
	void OnMouseReleased(vgui::MouseCode code );	// respond to mouse up events
	void OnCursorMoved	(int x, int y);  // respond to moving the cursor with mouse button down

	virtual void Paint(); //for drawing box select rectangle
	//virtual void PaintBackground();

	virtual void TraceScreenToWorld(int mousex, int mousey, int iClickType);

	//TGB: strange experiments with alternate selection
	/*virtual*/ void WorldToScreenSelect(int mousex, int mousey, int iClickType, bool sticky = false);
	void ScreenToWorld( int mousex, int mousey, trace_t &tr, bool ignore_npc = true);
	void LineCommand( int mousex, int mousey );

	void BoxSelect( int mousex, int mousey ); //select box
	void ZoneSelect( const int x1, const int y1, const int x2, const int y2 ) const; //select zone between (x1,y1)->(x2,y2)
	
	
	//TGB: other menus may tell us not to do boxselect/drag-related stuff because of click-things they do
	void ResetDragStatus() { m_iDragStatus = 0;	}

protected:
	IViewPort	*m_pViewPort;

	//TGB: for receiving button messages
//	void	OnMessage(const KeyValues *params, vgui::VPANEL fromPanel);
	//TGB: this is more in line with how you're supposed to do message handling:
	MESSAGE_FUNC_CHARPTR(OnButtonCommand, "ButtonCommand", command);


	//TGB: trace for objects to set tooltips about
	void doTooltipThink();


};



#endif // VGUI_Viewport_H

