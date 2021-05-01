//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: This is a panel which is rendered image on top of an entity
//
// $Revision: 1.6 $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#pragma warning (disable: 4514)

#include "vgui_bitmapimage.h"
#include "vgui_BitmapButton.h"
#include "iinput.h"
#include <KeyValues.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CBitmapButton::CBitmapButton( vgui::Panel *pParent, const char *pName, const char *pText ) : 
	BaseClass( pParent, pName, pText )
{
	SetPaintBackgroundEnabled( false );
	for ( int i = 0; i < BUTTON_STATE_COUNT; ++i )
	{
		m_bImageLoaded[i] = false;
	}

	m_pZMTooltip = NULL;
		
}

CBitmapButton::~CBitmapButton()
{
}


//-----------------------------------------------------------------------------
// initialization
//-----------------------------------------------------------------------------
bool CBitmapButton::Init( KeyValues* pInitData )
{
	// Unimplemented
	Assert( 0 );
	m_szMouseOverText = "You forgot to set my tooltip!";
	return true;
}


//-----------------------------------------------------------------------------
// initialization from build-mode dialog style .res files
//-----------------------------------------------------------------------------
void CBitmapButton::ApplySettings(KeyValues *pInitData)
{
	BaseClass::ApplySettings(pInitData);

	COMPILE_TIME_ASSERT( BUTTON_STATE_COUNT == 4 );
	const char *pSectionName[BUTTON_STATE_COUNT] = 
	{
		"enabledImage",
		"mouseOverImage",
		"pressedImage",
		"disabledImage"
	};

	for ( int i = 0; i < BUTTON_STATE_COUNT; ++i )
	{
		m_bImageLoaded[i] = InitializeImage(pInitData, pSectionName[i], this, &m_pImage[i] );
	}
}


//-----------------------------------------------------------------------------
// Sets the image
//-----------------------------------------------------------------------------
void CBitmapButton::SetImage( ButtonImageType_t type, const char *pMaterialName, color32 color )
{
	m_bImageLoaded[type] = m_pImage[type].Init( GetVPanel(), pMaterialName );
	if (m_bImageLoaded[type])
	{
		Color vcol( color.r, color.g, color.b, color.a );
		m_pImage[type].SetColor( vcol );
	}
}

bool CBitmapButton::IsImageLoaded( ButtonImageType_t type ) const
{
	return m_bImageLoaded[type];
}

void CBitmapButton::OnCursorEntered()
{
	//CBasePlayer *pPlayer = CBasePlayer::GetLocalPlayer();
	m_pZMTooltip = GET_HUDELEMENT( CHudToolTip );

	if(m_pZMTooltip /*&& pPlayer*/ && m_bHasTooltip && m_pZMTooltip->GetOwner() == NULL )
	{
		//qck: Send our string to the display
		//DevMsg("Inside button paint, called SetToolTip\n");
		m_pZMTooltip->SetTooltip(m_szMouseOverText, GetVPanel());
		//pPlayer->m_Local.m_iHideHUD &= ~HIDEHUD_TOOLTIP;

		//TGB: done in SetTooltip now
		//m_pZMTooltip->Hide( false );
	}
	BaseClass::OnCursorEntered();
}

void CBitmapButton::OnCursorExited()
{
	m_pZMTooltip = GET_HUDELEMENT( CHudToolTip );
	//CBasePlayer *pPlayer = CBasePlayer::GetLocalPlayer();

	if(m_pZMTooltip /*&& pPlayer*/ && m_bHasTooltip && GetVPanel() == m_pZMTooltip->GetOwner() )
	{
		//DevMsg("BITMAPBUTTON: Trying to hide tooltip\n");
		m_pZMTooltip->ClearTooltip();
		//pPlayer->m_Local.m_iHideHUD |= HIDEHUD_TOOLTIP;
		m_pZMTooltip->Hide( true );
	}

	BaseClass::OnCursorExited();
}

//-----------------------------------------------------------------------------
// Draws the puppy
//-----------------------------------------------------------------------------
void CBitmapButton::Paint( void )
{
	//TGB: done in the hud element now
	//CBasePlayer *pPlayer = CBasePlayer::GetLocalPlayer();
	//if(pPlayer)
	//{
	//	if(IsCursorOver() && m_bHasTooltip)
	//		pPlayer->m_Local.m_iHideHUD &= ~HIDEHUD_TOOLTIP;
	//}

	// Determine the image to use based on the state
	int nCurrentImage = BUTTON_ENABLED;
	if (IsArmed())
	{
		if (IsDepressed())
		{
			nCurrentImage = BUTTON_PRESSED;
		}
		else
		{
			nCurrentImage = BUTTON_ENABLED_MOUSE_OVER;
		}
	}
	else if (!IsEnabled())
	{
		nCurrentImage = BUTTON_DISABLED;
	}

	if (m_bImageLoaded[nCurrentImage])
	{
		m_pImage[nCurrentImage].DoPaint( GetVPanel() );
	}


	BaseClass::Paint();
}


