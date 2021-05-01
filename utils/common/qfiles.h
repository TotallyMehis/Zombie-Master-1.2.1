//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date: 2006-12-15 11:34:02 $
//
//-----------------------------------------------------------------------------
// $Log: qfiles.h,v $
// Revision 1.3  2006-12-15 11:34:02  tgb
// Second merge
//
//
// $NoKeywords: $
//=============================================================================//

#ifndef QFILES_H
#define QFILES_H
#pragma once


//
// qfiles.h: quake file formats
// This file must be identical in the quake and utils directories
//

#include "basetypes.h"
#include "commonmacros.h"
#include "worldsize.h"
#include "bspfile.h"

#define MAX_OSPATH	260
#define MAX_QPATH	64

/*
========================================================================

The .pak files are just a linear collapse of a directory tree

========================================================================
*/

#define IDPAKHEADER		(('K'<<24)+('C'<<16)+('A'<<8)+'P')

#endif // QFILES_H
