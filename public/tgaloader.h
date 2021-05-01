//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date: 2006-06-01 16:01:23 $
//
//-----------------------------------------------------------------------------
// $Log: tgaloader.h,v $
// Revision 1.1  2006-06-01 16:01:23  tgb
// Initial full source add, take 2
//
// Revision 1.2  2006-05-31 14:40:26  tgb
// Committing code, seeing as the one grabbed from the server had unix linebreaks or something which meant a white line between every normal line resulting in unreadability.
//
// Revision 1.1.1.1  2005/10/11 14:58:06  theGreenBunny
// The latest version of the code for zombie master
//
//
// $NoKeywords: $
//=============================================================================//

#ifndef TGALOADER_H
#define TGALOADER_H
#pragma once

#include "imageloader.h"
#include "interface.h"
#include "utlmemory.h"

class CUtlBuffer;


namespace TGALoader
{

#ifndef TGALOADER_USE_FOPEN
bool SetFileSystem( CreateInterfaceFn fileSystemFactory );
#endif
bool GetInfo( const char *fileName, int *width, int *height, ImageFormat *imageFormat, float *sourceGamma );
bool GetInfo( CUtlBuffer &buf, int *width, int *height, ImageFormat *imageFormat, float *sourceGamma );
bool Load( unsigned char *imageData, const char *fileName, int width, int height, 
		   ImageFormat imageFormat, float targetGamma, bool mipmap );
bool Load( unsigned char *imageData, FILE *fp, int width, int height, 
			ImageFormat imageFormat, float targetGamma, bool mipmap );
bool Load( unsigned char *imageData, CUtlBuffer &buf, int width, int height, 
			ImageFormat imageFormat, float targetGamma, bool mipmap );

bool LoadRGBA8888( const char *pFileName, CUtlMemory<unsigned char> &outputData, int &outWidth, int &outHeight );

} // end namespace TGALoader

#endif // TGALOADER_H
