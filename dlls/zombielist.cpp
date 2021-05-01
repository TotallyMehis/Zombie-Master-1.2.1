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
#include "cbase.h"
#include "zombielist.h"

void CZombieGroup::Reset()
{
	m_ZombieGroupMembers.Purge();
}

void CZombieGroup::DropZombieFromGroup(CBaseEntity* pZombie) //LAWYER:  General cleanup operation
{
	//Our Zombie has died, it seems, so let's drop it from the group.
	if (pZombie)
	{
		if (m_ZombieGroupMembers.HasElement(pZombie))
		{
			m_ZombieGroupMembers.Remove(m_ZombieGroupMembers.Find(pZombie));
		}
	}
}

CZombieGroupManager::CZombieGroupManager()
{
	m_pZombieLists.Purge();
}

CZombieGroupManager::~CZombieGroupManager()
{
	m_pZombieLists.Purge();
}

void CZombieGroupManager::Reset()
{	
	for(int i = 0; i < m_pZombieLists.Count(); i++)
	{
		CBaseEntity* pEnt = m_pZombieLists.Element(i);
		if(pEnt)
		{
			CZombieGroup* pGroup = dynamic_cast<CZombieGroup*>(pEnt);
			delete pGroup;
		}
	}
	m_pZombieLists.Purge();
}

//qck: Add group to server list, and send it to the client. 
void CZombieGroupManager::AddGroup( CZombieGroup* group, CBasePlayer* pPlayer )
{
	if (!pPlayer) return;

	EHANDLE newGroup = group;

	m_pZombieLists.AddToTail( newGroup );
	pPlayer->m_iZombieGroup = newGroup.GetSerialNumber(); 

	DevMsg("Groups in the master list: %i\n", m_pZombieLists.Count());
}

//TGB: moved here from basezombie
void CZombieGroupManager::DropZombieFromAll(CBaseEntity* pZombie )
{
	for(int i = 0; i < m_pZombieLists.Count(); i++)
	{
		CBaseEntity* pEnt = m_pZombieLists.Element(i);
		if(pEnt)
		{
			CZombieGroup* pGroup = dynamic_cast<CZombieGroup*>(pEnt);
			if (pGroup)
			{
				pGroup->DropZombieFromGroup(pZombie);

				//TGB: remove the group
				if (pGroup->m_ZombieGroupMembers.Count() == 0) //not too pretty
				{
					EHANDLE hGroup = pEnt;
					const int serial = hGroup.GetSerialNumber();

					//remove from list
					m_pZombieLists.FindAndRemove(pGroup);

					//kill the entity
					UTIL_Remove(pGroup);

					DevMsg("Removed empty group from master list (serial %i)\n", serial);

					//problem is, due to the sorta clunky networking here we can't tell the client about this easily
					//TGB: therefore, it's time for a usermessage, they are remarkably easy and I wish I'd known about them before
					CBasePlayer *pZM = CBasePlayer::GetZM();
					if (pZM)
					{
						CSingleUserRecipientFilter filter( pZM ); // set recipient
						filter.MakeReliable();  // reliable transmission

						UserMessageBegin( filter, "RemoveGroup" ); // create message 
							WRITE_LONG( serial ); //TGB: write the serial, which just to be sure is sized for a full int
						MessageEnd(); //send message
					}
				}
			}
		}
	}

}