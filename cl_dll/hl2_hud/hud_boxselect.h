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

#ifndef HUD_BOXSELECT
#define HUD_BOXSELECT
#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include <vgui_controls/Panel.h>

namespace vgui
{
	class IScheme;
};

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Shows the selection box
//-----------------------------------------------------------------------------
class CHudBoxSelect : public CHudElement, public vgui::Panel
{
	
	DECLARE_CLASS_SIMPLE( CHudBoxSelect, vgui::Panel );

public:
	CHudBoxSelect( const char *pElementName );
	bool m_bDrawBox;
	bool m_bDrawLine;
	int	m_iDragMouseX, m_iDragMouseY;
	
	virtual void Init( void );
	

protected:
	virtual void Paint();

private:

};	

#endif
