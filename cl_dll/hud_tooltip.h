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
//=============================================================================//

/*
#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>

#include "hud_macros.h"
#include "iclientmode.h"
#include "iinput.h"
#include "view.h"
#include "vgui_controls/controls.h"
#include "vgui/ISurface.h"

#include "IVRenderView.h"
*/

#ifndef HUD_TOOLTIP
#define HUD_TOOLTIP
#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include <vgui_controls/Panel.h>
#include <vgui_controls/Label.h>

namespace vgui
{
	class IScheme;
};

//this will cause ambiguity errors
//using namespace vgui;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Shows the selection box
//-----------------------------------------------------------------------------
class CHudToolTip : public CHudElement, public vgui::Panel
{
	
	DECLARE_CLASS_SIMPLE( CHudToolTip, vgui::Panel);

public:
	CHudToolTip( const char *pElementName );

	virtual void Init( void );
	void SetTooltip( const char* szToolTip, vgui::VPANEL _owner = NULL );
	void ClearTooltip();

	vgui::Label *m_Text;
	
	//TGB: timed setting and owner check
	vgui::VPANEL GetOwner()		
			{ return m_Owner; }

	void SetTooltipTimed( const char* szToolTip, vgui::VPANEL _owner = NULL, const double duration = 0 );

	//TGB: hides/shows this
	void Hide( bool shouldHide );
	bool IsHidden ()
			{ return (!m_bForceVisible); }


protected:
	virtual void Paint();
	virtual void OnThink();	//to check for timed clearing


private:
	char*		m_szTooltip;
	vgui::VPANEL		m_Owner;		//panel that set the tooltip

	//timed tooltips stuff (ones that are displayed for a certain time)
	bool		m_bTimed;		//if the current tooltip is timed
	int			m_iEndTime;		//when it should be cleared

	bool		m_bForceVisible;	//hack to force this to show up

};	

#endif