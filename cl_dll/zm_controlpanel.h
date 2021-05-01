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
#ifndef ZM_ControlPanel_H
#define ZM_ControlPanel_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>

#include "vgui_BitmapButton.h" //TGB test

#include <cl_dll/iviewport.h>

#include <vgui/KeyCode.h>
#include <UtlVector.h>

namespace vgui
{
	class RichText;
	class HTML;
}
class TeamFortressViewport;

//-----------------------------------------------------------------------------
// Purpose: Displays the build menu
//-----------------------------------------------------------------------------
class CZM_ControlPanel : public vgui::Frame, public IViewPortPanel
{
private:
	DECLARE_CLASS_SIMPLE( CZM_ControlPanel, vgui::Frame );

public:
	CZM_ControlPanel(IViewPort *pViewPort);
	virtual ~CZM_ControlPanel();

	virtual const char *GetName( void ) { return PANEL_ZMCP; }
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
	//virtual void Paint();

public:
	
	void AutoAssign();
	
protected:

	// int GetNumBuilds() { return m_iNumBuilds; }
	
	// VGUI2 overrides
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void OnKeyCodePressed(vgui::KeyCode code);

	// helper functions
	//virtual void SetLabelText(const char *textEntryName, const char *text);
	
	
	// command callbacks
	void OnCommand( const char *command ); 


	IViewPort	*m_pViewPort;
	vgui::HScheme scheme;
	vgui::HFont m_hMediumFont;
	vgui::HFont m_hLargeFont;

	int m_iJumpKey;
	int m_iScoreBoardKey;
private:
	
//	CBitmapButton *m_pButtons[NUM_BUTTONS];

//	void LoadButtons();

	//I hope these will scale right by themselves
	CPanelAnimationVarAliasType( float, m_iButtonSize, "button_size", "24", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_iButtonY, "button_ypos", "8", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_iButtonX, "button_xpos", "8", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_iButtonGapX, "button_xgap", "8", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_iButtonGapY, "button_ygap", "8", "proportional_float" );
};

#endif // ZM_ControlPanel_H