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

//-------------------------------------------------------------------------------------------
// Total Retribution
//-------------------------------------------------------------------------------------------

#include "cbase.h"
#include "mapentityfilter.h"


CMapEntityFilter::CMapEntityFilter() 
{
	//keepList = new CUtlSortVector< const char *, CMapEntityReportLess> (0,0);
	keepList.Purge();
}



CMapEntityFilter::~CMapEntityFilter() 
{

	//delete keepList; 

}



bool CMapEntityFilter::ShouldCreateEntity( const char *pClassname ) 
{
	//qck: Never bother with worldspawn. You'll get weird behavior. Skip it, and take care of everything else.
	if ( Q_stricmp( pClassname, "worldspawn" ) == 0 )
		return false;
	else
		return true;
}



CBaseEntity* CMapEntityFilter::CreateNextEntity( const char *pClassname ) 
{
	return CreateEntityByName( pClassname);
}



void CMapEntityFilter::AddKeep( const char *sz) 
{
	//keepList->Insert(sz);
	keepList.Insert(sz);
}
