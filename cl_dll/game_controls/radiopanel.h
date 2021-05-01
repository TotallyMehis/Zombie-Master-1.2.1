//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef RADIOMENU_H
#define RADIOMENU_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Panel.h>
#include <vgui_controls/Button.h>

#include <cl_dll/iviewport.h>

#include <vgui/KeyCode.h>
#include <UtlVector.h>

namespace vgui
{
	class RichText;
//	class HTML;
}
class TeamFortressViewport;

//-----------------------------------------------------------------------------
// Purpose: Displays the Radio menu
//-----------------------------------------------------------------------------
class CRadioMenu : public vgui::Panel, public IViewPortPanel
{
private:
	DECLARE_CLASS_SIMPLE( CRadioMenu, vgui::Panel );

	void paintString ( wchar_t *string, int x, int y );
public:
	CRadioMenu(IViewPort *pViewPort);
	virtual ~CRadioMenu();

	virtual const char *GetName( void ) { return PANEL_RADIO; }
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
	virtual void Paint();

protected:
	
	// VGUI2 overrides
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void OnKeyCodePressed(vgui::KeyCode code);

	// helper functions
	//virtual void SetLabelText(const char *textEntryName, const char *text);
	
	
	// command callbacks
	void OnCommand( const char *command ); //LAWYER: (RADIO MENU) Adds command callbackishness

	IViewPort	*m_pViewPort;
};


#endif // RadioMENU_H
