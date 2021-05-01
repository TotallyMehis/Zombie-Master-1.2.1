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
//=======================================
// A manager class used to control
// client/server networking for groups
//=======================================

#include "cbase.h"

//qck: Class to manage individual group members. 
class CZombieGroup : public CBaseEntity
{
	DECLARE_CLASS(CZombieGroup, CBaseEntity);
public:

	CUtlVector<CBaseEntity *>	m_ZombieGroupMembers;
	void Reset();
	void DropZombieFromGroup(CBaseEntity* pZombie );
};

LINK_ENTITY_TO_CLASS(zm_group, CZombieGroup);

//qck: Trying to network this was a bitch, and not needed. I just go through the player.
class CZombieGroupManager : public CBaseEntity
{
public:
	CZombieGroupManager();
	~CZombieGroupManager();

	void Reset();
	void AddGroup( CZombieGroup* group, CBasePlayer* pPlayer );

	void DropZombieFromAll(CBaseEntity* pZombie );
	CUtlVector< CHandle< CBaseEntity > >	m_pZombieLists;
};

LINK_ENTITY_TO_CLASS(zm_group_manager, CZombieGroupManager);


