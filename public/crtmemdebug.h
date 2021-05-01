//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date: 2006-10-26 18:30:21 $
//
//-----------------------------------------------------------------------------
// $Log: crtmemdebug.h,v $
// Revision 1.2  2006-10-26 18:30:21  tgb
// SDK merge of stuff outside core game dlls
//
//
// $NoKeywords: $
//=============================================================================//

#ifndef CRTMEMDEBUG_H
#define CRTMEMDEBUG_H
#pragma once

#ifdef USECRTMEMDEBUG

#include <crtdbg.h>
#define MEMCHECK CheckHeap()
void CheckHeap( void );

#else

#define MEMCHECK

#endif

void InitCRTMemDebug( void );


#endif // CRTMEMDEBUG_H
