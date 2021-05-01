//=============================================================================
// Copyright (c) Zombie Master Development Team. All rights reserved.
// The use and distribution terms for this software are covered by the MIT
// License (http://opensource.org/licenses/mit-license.php) which
// can be found in the file LICENSE.TXT at the root of this distribution. By
// using this software in any fashion, you are agreeing to be bound by the
// terms of this license. You must not remove this notice, or any other, from
// this software.
//
// Note that due to the number of files included in the SDK, it is not feasible
// to include this notice in all of them. All original files or files 
// containing large modifications should contain this notice. If in doubt,
// assume the above notice applies, and refer to the included LICENSE.TXT text.
//=============================================================================

#ifndef MAPENTITYFILTER_H
#define MAPENTITYFILTER_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "mapentities.h"
#include "UtlSortVector.h"


typedef const char* strptr;


//static bool StrLessThan(  const strptr &src1,  const strptr &src2, void *pCtx ) 
//{
//	if( strcmp(src1, src2) >= 0)
//		return false;
//	else
//		return true;
//}




class CMapEntityFilter : public IMapEntityFilter
{
public:
	CMapEntityFilter();
	~CMapEntityFilter();

	class CMapEntityReportLess
	{
	public:
		bool Less( const strptr &src1, const strptr &src2, void *pCtx )
		{
			if( strcmp(src1, src2) >= 0)
				return false;
			else
				return true;
		}
	};

	virtual bool ShouldCreateEntity( const char *pClassname );
	virtual CBaseEntity* CreateNextEntity( const char *pClassname );
	void AddKeep( const char*);

private:
	//TGB: why was this a pointer?
	//CUtlSortVector< const char*, CMapEntityReportLess > *keepList;
	CUtlSortVector< const char*, CMapEntityReportLess > keepList;

	//TGB: in retrospect we aren't using this, but hey

};

#endif // MAPENTITYFILTER_H
