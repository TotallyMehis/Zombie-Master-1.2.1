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


//
// Purpose:		Molotov grenades
//
// $Workfile:     $
// $Date: 2006-06-01 15:57:38 $
//
//-----------------------------------------------------------------------------
// $Log: grenade_molotov.h,v $
// Revision 1.1  2006-06-01 15:57:38  tgb
// Initial full source add, take 2
//
// Revision 1.2  2006-05-31 14:38:17  tgb
// Committing code, seeing as the one grabbed from the server had unix linebreaks or something which meant a white line between every normal line resulting in unreadability.
//
// Revision 1.5  2006/03/10 13:26:22  qckbeam
// *** empty log message ***
//
// Revision 1.4  2006/02/09 03:42:07  qckbeam
// Made some changes to the fire
//
// Revision 1.3  2006/02/07 16:49:45  qckbeam
// no message
//
// Revision 1.2  2005/12/12 16:23:28  Angry_Lawyer
// no message
//
// Revision 1.1.1.1  2005/10/11 14:57:56  theGreenBunny
// The latest version of the code for zombie master
//
//
// $NoKeywords: $
//=============================================================================//

#ifndef	GRENADEMOLOTOV_H
#define	GRENADEMOLOTOV_H

#include "basegrenade_shared.h"
#include "smoke_trail.h"
#include "gib.h"

//TGB: we don't appear to be using this
/*class CMolotovFire : public CBaseEntity
{
public:
	DECLARE_CLASS( CMolotovFire, CBaseEntity);

	virtual void Spawn( void );
	virtual void Think( void );

	void CreateFlamingBits( CBaseEntity* pOwner, const Vector &vecChunkPos, const QAngle &vecChunkAngles, const char *pszChunkName );
private:
	//qck: The chunk to set on fire, and the flames
	CFireTrail *pFireTrail;
	CGib *pChunk;

	DECLARE_DATADESC();
	
};*/

class CGrenade_Molotov : public CBaseGrenade
{
public:
	DECLARE_CLASS( CGrenade_Molotov, CBaseGrenade );

	virtual void	Spawn( void );
	virtual void	Precache( void );
	virtual void	Detonate( void );
	void			MolotovTouch( CBaseEntity *pOther );
	void			MolotovThink( void );
	void			CreateFlamingChunk( const Vector &vecChunkPos, const char *pszChunkName);
	void			CreateFlyingChunk( const Vector &vecChunkPos, const QAngle &vecChunkAngles, const char *pszChunkName, bool bSmall ); //qck: Creating chunks of flaming stuff
	void	SetVelocity( const Vector &velocity, const AngularImpulse &angVelocity );
	CBasePlayer		*m_pDamageParent;

	~CGrenade_Molotov() {
		UTIL_Remove(m_pFireTrail);
	}

protected:

	SmokeTrail		*m_pFireTrail;

	DECLARE_DATADESC();
private:

};

#endif	//GRENADEMOLOTOV_H
