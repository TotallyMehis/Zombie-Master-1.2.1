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

#ifndef MENU_AMBUSH_H
#define MENU_AMBUSH_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>
#include "sdk/vgui/vgui_viewport.h"

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
// Purpose: Displays the manipulate menu
//-----------------------------------------------------------------------------
class CAmbushMenu : public vgui::Frame, public IViewPortPanel
{
private:
	DECLARE_CLASS_SIMPLE( CAmbushMenu, vgui::Frame );

public:
	CAmbushMenu(IViewPort *pViewPort);
	virtual ~CAmbushMenu();

	virtual const char *GetName( void ) { return PANEL_AMBUSH; }
	virtual void SetData(KeyValues *data) {};
	virtual void Reset() {};
	virtual void Update();
	virtual bool NeedsUpdate( void ) { return false; }
	virtual bool HasInputElements( void ) { return true; }
	virtual void ShowPanel( bool bShow );
	virtual void OnThink( void );

	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel( void ) { return BaseClass::GetVPanel(); }
  	virtual bool IsVisible() { return BaseClass::IsVisible(); }
	virtual void SetParent( vgui::VPANEL parent ) { BaseClass::SetParent( parent ); }



public:
	
	void AutoAssign();
	
protected:
	
	// VGUI2 overrides
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

	// helper functions
	virtual void SetLabelText(const char *textEntryName, const char *text);

	// command callbacks
	void OnCommand( const char *command ); 

	IViewPort	*m_pViewPort;

};


#endif // MENU_AMBUSH_H