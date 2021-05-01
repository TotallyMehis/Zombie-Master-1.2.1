//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: /home/pimurho/cvs/zm_src_latest/materialsystem/shaderlib/shaderDLL_Global.h,v 1.2 2006-12-15 11:34:02 tgb Exp $
// $NoKeywords: $
//=============================================================================//

#ifndef SHADERDLL_GLOBAL_H
#define SHADERDLL_GLOBAL_H

#ifdef _WIN32
#pragma once
#endif


//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
class IShaderSystem;


//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
inline IShaderSystem *GetShaderSystem()
{
	extern IShaderSystem* g_pSLShaderSystem;
	return g_pSLShaderSystem;
}


#endif	// SHADERDLL_GLOBAL_H