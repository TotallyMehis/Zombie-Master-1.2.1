//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:		Namespace for functions having to do with WC Edit mode
//
// $Workfile:     $
// $Date: 2006-10-26 18:02:22 $
//
//-----------------------------------------------------------------------------
// $Log: wcedit.h,v $
// Revision 1.2  2006-10-26 18:02:22  Angry_Lawyer
// *** empty log message ***
//
//
// $NoKeywords: $
//=============================================================================//

#ifndef WCEDIT_H
#define WCEDIT_H
#pragma once

class CBaseEntity;

//=============================================================================
//	>> NWCEdit
//=============================================================================
namespace NWCEdit
{
	Vector	AirNodePlacementPosition( void );
	bool	IsWCVersionValid(void);
	void	CreateAINode(   CBasePlayer *pPlayer );
	void	DestroyAINode(  CBasePlayer *pPlayer );
	void	CreateAILink(	CBasePlayer *pPlayer );
	void	DestroyAILink(  CBasePlayer *pPlayer );
	void	UndoDestroyAINode(void);
	void	RememberEntityPosition( CBaseEntity *pEntity );
	void	UpdateEntityPosition( CBaseEntity *pEntity );
};

#endif // WCEDIT_H