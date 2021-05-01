//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Header file for player-thrown grenades.
//
// $Workfile:     $
// $Date: 2006-10-26 18:02:21 $
//
//-----------------------------------------------------------------------------
// $Log: grenadethrown.h,v $
// Revision 1.2  2006-10-26 18:02:21  Angry_Lawyer
// *** empty log message ***
//
//
// $NoKeywords: $
//=============================================================================//

#ifndef GRENADE_BASE_H
#define GRENADE_BASE_H
#pragma once

#include "basegrenade_shared.h"

class CSprite;

#define GRENADE_TIMER		5		// Try 5 seconds instead of 3?

//-----------------------------------------------------------------------------
// Purpose: Base Thrown-Grenade class
//-----------------------------------------------------------------------------
class CThrownGrenade : public CBaseGrenade
{
public:
	DECLARE_CLASS( CThrownGrenade, CBaseGrenade );

	void	Spawn( void );
	void	Thrown( Vector vecOrigin, Vector vecVelocity, float flExplodeTime );
};



#endif // GRENADE_BASE_H
