//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: HUD Target ID element
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "c_hl2mp_player.h"
#include "c_playerresource.h"
#include "vgui_EntityPanel.h"
#include "iclientmode.h"
#include "vgui/ILocalize.h"
#include "hl2mp_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define PLAYER_HINT_DISTANCE	150
#define PLAYER_HINT_DISTANCE_SQ	(PLAYER_HINT_DISTANCE*PLAYER_HINT_DISTANCE)

static ConVar hud_centerid( "hud_centerid", "0" );
static ConVar hud_showtargetid( "hud_showtargetid", "1" );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTargetID : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CTargetID, vgui::Panel );

public:
	CTargetID( const char *pElementName );
	void Init( void );
	virtual void	ApplySchemeSettings( vgui::IScheme *scheme );
	virtual void	Paint( void );
	void VidInit( void );

private:
	Color			GetColorForTargetTeam( int iTeamNumber );

	vgui::HFont		m_hFont;
	int				m_iLastEntIndex;
	float			m_flLastChangeTime;
};

DECLARE_HUDELEMENT( CTargetID );

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTargetID::CTargetID( const char *pElementName ) :
	CHudElement( pElementName ), BaseClass( NULL, "TargetID" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_hFont = g_hFontTrebuchet24;
	m_flLastChangeTime = 0;
	m_iLastEntIndex = 0;

	SetHiddenBits( HIDEHUD_MISCSTATUS );
}

//-----------------------------------------------------------------------------
// Purpose: Setup
//-----------------------------------------------------------------------------
void CTargetID::Init( void )
{
};

void CTargetID::ApplySchemeSettings( vgui::IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	m_hFont = scheme->GetFont( "TargetID", IsProportional() );

	SetPaintBackgroundEnabled( false );
}

//-----------------------------------------------------------------------------
// Purpose: clear out string etc between levels
//-----------------------------------------------------------------------------
void CTargetID::VidInit()
{
	CHudElement::VidInit();

	m_flLastChangeTime = 0;
	m_iLastEntIndex = 0;
}

Color CTargetID::GetColorForTargetTeam( int iTeamNumber )
{
	return GameResources()->GetTeamColor( iTeamNumber );
} 

//-----------------------------------------------------------------------------
// Purpose: Draw function for the element
//-----------------------------------------------------------------------------
void CTargetID::Paint()
{
#define MAX_ID_STRING 256
	wchar_t sIDString[ MAX_ID_STRING ];
	sIDString[0] = 0;

	C_HL2MP_Player *pLocalPlayer = C_HL2MP_Player::GetLocalHL2MPPlayer();

	if ( !pLocalPlayer || (pLocalPlayer && pLocalPlayer->IsZM()) )
		return;

	//Color c;

	// Get our target's ent index
	int iEntIndex = pLocalPlayer->GetIDTarget();
	// Didn't find one?
	if ( !iEntIndex )
	{
		// Check to see if we should clear our ID
		if ( m_flLastChangeTime && (gpGlobals->curtime > (m_flLastChangeTime + 0.5)) )
		{
			m_flLastChangeTime = 0;
			sIDString[0] = 0;
			m_iLastEntIndex = 0;

		}
		else
		{
			// Keep re-using the old one
			iEntIndex = m_iLastEntIndex;
		}
	}
	else
	{
		m_flLastChangeTime = gpGlobals->curtime;
	}

	// Is this an entindex sent by the server?
	if ( iEntIndex && IsPlayerIndex( iEntIndex ))
	{
		//TGB: valve made this a static cast, guess it's faster, added playerindex check above cause that's all we identify
		C_BasePlayer *pPlayer = static_cast<C_BasePlayer*>(cl_entitylist->GetEnt( iEntIndex ));
		//C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();

		//TGB: we never identify anything other than players
		if (!pPlayer)
			return;

		wchar_t wszPlayerName[ MAX_PLAYER_NAME_LENGTH ];
		wchar_t wszHealthText[ 15 ];
		bool bShowHealth = true;
		bool bShowPlayerName = true;

		// Some entities we always want to check, cause the text may change
		// even while we're looking at it
		// Is it a player?
//		if ( IsPlayerIndex( iEntIndex ) )
//		{
			//TGB: we set colour later on
			//c = GetColorForTargetTeam( pPlayer->GetTeamNumber() );

			vgui::localize()->ConvertANSIToUnicode( pPlayer->GetPlayerName(),  wszPlayerName, sizeof(wszPlayerName) );
			
			if ( bShowHealth )
			{
				//GetMaxHealth returns 1, so just assume it's 100, saves us some division too
				const int level = pPlayer->GetHealth();
				char *string;

				if ( level > 90 )
					string = "Healthy";
				else if ( level > 66 )
					string = "Hurt";
				else if ( level > 35 )
					string = "Wounded";
				else
					string = "Near Death";

				vgui::localize()->ConvertANSIToUnicode( string,  wszHealthText, sizeof(wszHealthText) );
			}
//		}

		//TGB: make sure we don't show observers or the zm
		if ( !pPlayer || pPlayer->IsZM() || pPlayer->IsObserver() )
			bShowHealth = bShowPlayerName = false;
		

		if ( bShowPlayerName && bShowHealth )
		{	//TGB: we don't use the localize stuff, just use a manual formatting string, easier than figuring out all this wchar crap
			vgui::localize()->ConstructString( sIDString, sizeof(sIDString), L"%s1 (%s2)", 2, wszPlayerName, wszHealthText );
		}
		else if ( bShowPlayerName )
		{
			vgui::localize()->ConstructString( sIDString, sizeof(sIDString), L"%s1 (%s2)", 1, wszPlayerName );
		}
		else if ( bShowHealth )
		{
			vgui::localize()->ConstructString( sIDString, sizeof(sIDString), L"%s1 (%s2)", 1, wszHealthText );
		}
		else
		{
			vgui::localize()->ConstructString( sIDString, sizeof(sIDString), L"%s1 (%s2)", 0 );
		}


		if ( sIDString[0] )
		{
			int wide, tall;
			int ypos = YRES(260);
			int xpos = XRES(10);

			vgui::surface()->GetTextSize( m_hFont, sIDString, wide, tall );

			if( hud_centerid.GetInt() == 0 )
			{
				ypos = YRES(260); //TGB: 420 is too low for us
			}
			else
			{
				xpos = (ScreenWidth() - wide) / 2;
			}
			
			vgui::surface()->DrawSetTextFont( m_hFont );
			vgui::surface()->DrawSetTextPos( xpos, ypos );
			vgui::surface()->DrawSetTextColor( Color(255,255,255, 255) );
			vgui::surface()->DrawPrintText( sIDString, wcslen(sIDString) );
		}
	}
}
