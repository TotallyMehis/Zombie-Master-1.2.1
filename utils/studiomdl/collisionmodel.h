//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date: 2006-12-15 11:34:02 $
//
//-----------------------------------------------------------------------------
// $Log: collisionmodel.h,v $
// Revision 1.3  2006-12-15 11:34:02  tgb
// Second merge
//
//
// $NoKeywords: $
//=============================================================================//

#ifndef COLLISIONMODEL_H
#define COLLISIONMODEL_H
#pragma once

extern void Cmd_CollisionText( void );
extern int DoCollisionModel( bool separateJoints );

// execute after simplification, before writing
extern void CollisionModel_Build( void );
// execute during writing
extern void CollisionModel_Write( long checkSum );

void CollisionModel_ExpandBBox( Vector &mins, Vector &maxs );

#endif // COLLISIONMODEL_H
