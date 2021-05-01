//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date: 2006-10-26 18:02:06 $
//
//-----------------------------------------------------------------------------
// $Log: iprofiling.h,v $
// Revision 1.2  2006-10-26 18:02:06  Angry_Lawyer
// *** empty log message ***
//
//
// $NoKeywords: $
//=============================================================================//
#if !defined( IPROFILING_H )
#define IPROFILING_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>

namespace vgui
{
	class Panel;
}

abstract_class IProfiling
{
public:
	virtual void		Create( vgui::VPANEL parent ) = 0;
	virtual void		Destroy( void ) = 0;
};

extern IProfiling *profiling;

#endif // IPROFILING_H