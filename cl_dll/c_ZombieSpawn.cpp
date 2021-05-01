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
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_baseanimating.h"
#include "iviewrender_beams.h"
#include "beam_shared.h"
#include "materialsystem/IMaterial.h"
#include "model_types.h"
#include "ClientEffectPrecacheSystem.h"



// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern void DrawSprite( const Vector &vecOrigin, float flWidth, float flHeight, color32 color );

//TGB: was for testing with range indication, unused for now but maybe later
ConVar zm_ambush_triggerrange( "zm_ambush_triggerrange", "96", FCVAR_REPLICATED, "The range ambush trigger points have.");

//-----------------------------------------------------------------------------
// Purpose: Client-side ZombieSpawn.
//-----------------------------------------------------------------------------
class C_ZombieSpawn : public C_BaseAnimating
{
public:
	DECLARE_CLASS( C_ZombieSpawn, C_BaseAnimating );
	DECLARE_CLIENTCLASS();

					C_ZombieSpawn();
	virtual			~C_ZombieSpawn();

	// model specific

//	virtual	void	SetupWeights( );
	bool ShouldDraw();
	int DrawModel( int flags );
	
	

	virtual ShadowType_t			ShadowCastType();

	
//	int m_iCost;

private:
	C_ZombieSpawn( const C_ZombieSpawn & ); // not defined, not accessible
	
};

IMPLEMENT_CLIENTCLASS_DT(C_ZombieSpawn, DT_ZombieSpawn, CZombieSpawn)
//	RecvPropInt( RECVINFO ( m_iCost ) ),
END_RECV_TABLE()

C_ZombieSpawn::C_ZombieSpawn()
{
}


C_ZombieSpawn::~C_ZombieSpawn()
{
}

bool C_ZombieSpawn::ShouldDraw()
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();  //LAWYER:  Get the local player
	if ( !pPlayer )
	{
		return false;
	}
	if (pPlayer->IsZM() == false)
	{
		return false; //LAWYER:  Only Zombiemasters can see this!
	}

	return(BaseClass::ShouldDraw());

}

int C_ZombieSpawn::DrawModel( int flags )
{
	//qck: Selective spawn glow sprite code.
	CBasePlayer *pPlayer = CBasePlayer::GetLocalPlayer();
	if(pPlayer->IsZM() == false)
		return 0;

//	Vector attachOrigin;
//	QAngle attachAngles;
	
	color32 color;
	color.r = 255;
	color.g = 200;
	color.b = 200;
	color.a = 0;

	//TGB: why lookup an attachment if we never use the resulting vectors?
//	int attachment = this->LookupAttachment( "glowsource");
//	GetAttachment( attachment, attachOrigin, attachAngles );

	IMaterial *pMaterial = materials->FindMaterial( "models/red2", TEXTURE_GROUP_CLIENT_EFFECTS, false );
	pMaterial->SetMaterialVarFlag(MATERIAL_VAR_ALPHATEST, true);
    
	materials->Bind( pMaterial );
	DrawSprite( GetAbsOrigin(), 128, 128, color);

	return(BaseClass::DrawModel(flags));
}

//-----------------------------------------------------------------------------
// Should this object cast shadows?
//-----------------------------------------------------------------------------
ShadowType_t C_ZombieSpawn::ShadowCastType()
{
		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();  //LAWYER:  Get the local player
	if ( !pPlayer )
	{
		return SHADOWS_NONE;
	}
	if (pPlayer->IsZM() == false)
	{
		return SHADOWS_NONE; //LAWYER:  Only Zombiemasters can see this!
	}

	return BaseClass::ShadowCastType();
}


//-----------------------------------------------------------------------------
// Purpose: Client-side ZombieManipulate
//-----------------------------------------------------------------------------
class C_ZombieManipulate : public C_BaseAnimating
{
public:
	DECLARE_CLASS( C_ZombieManipulate, C_BaseAnimating );
	DECLARE_CLIENTCLASS();

					C_ZombieManipulate();
	virtual			~C_ZombieManipulate();


	bool ShouldDraw();
	virtual ShadowType_t			ShadowCastType();

	int DrawModel(int flags);
	//qck: Some networked vars.

	char			m_szDescription[255];
private:
	C_ZombieManipulate( const C_ZombieManipulate & ); // not defined, not accessible
};

IMPLEMENT_CLIENTCLASS_DT(C_ZombieManipulate, DT_ZombieManipulate, CZombieManipulate)
	//RecvPropString(RECVINFO(m_szDescription)),
END_RECV_TABLE()

C_ZombieManipulate::C_ZombieManipulate()
{
}


C_ZombieManipulate::~C_ZombieManipulate()
{
}


bool C_ZombieManipulate::ShouldDraw()
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();  //LAWYER:  Get the local player
	if ( !pPlayer )
	{
		return false;
	}
	if (pPlayer->IsZM() == false)
	{
		return false; //LAWYER:  Only Zombiemasters can see this!
	}
	return(BaseClass::ShouldDraw());

	//return (model != 0) && !IsEffectActive(EF_NODRAW) && (index != 0);
}

int C_ZombieManipulate::DrawModel(int flags)
{
	//DevMsg("%s\n", m_szDescription);

	//qck: Selective spawn glow sprite code.
	CBasePlayer *pPlayer = CBasePlayer::GetLocalPlayer();
	if(pPlayer->IsZM() == false)
		return 0;

//	Vector attachOrigin;
//	QAngle attachAngles;
	
	color32 color;
	color.r = 255;
	color.g = 200;
	color.b = 200;
	color.a = 0;
	
//	int attachment = this->LookupAttachment( "glowsource");
//	GetAttachment( attachment, attachOrigin, attachAngles );

	IMaterial *pMaterial = materials->FindMaterial( "models/orange", TEXTURE_GROUP_CLIENT_EFFECTS, false );
	pMaterial->SetMaterialVarFlag(MATERIAL_VAR_ALPHATEST, true);
    
	materials->Bind( pMaterial );
	DrawSprite( this->GetAbsOrigin(), 128, 128, color);

	return(BaseClass::DrawModel(flags));
}

//-----------------------------------------------------------------------------
// Should this object cast shadows?
//-----------------------------------------------------------------------------
ShadowType_t C_ZombieManipulate::ShadowCastType()
{
		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();  //LAWYER:  Get the local player
	if ( !pPlayer )
	{
		return SHADOWS_NONE;
	}
	if (pPlayer->IsZM() == false)
	{
		return SHADOWS_NONE; //LAWYER:  Only Zombiemasters can see this!
	}

	return BaseClass::ShadowCastType();
}

//-----------------------------------------------------------------------------
// Purpose: Client-side ZombieRallyPoint
//-----------------------------------------------------------------------------
class C_ZombieRallyPoint : public C_BaseAnimating
{
public:
	DECLARE_CLASS( C_ZombieRallyPoint, C_BaseAnimating );
	DECLARE_CLIENTCLASS();

					C_ZombieRallyPoint();
	virtual			~C_ZombieRallyPoint();

	// model specific
//	virtual	void	SetupWeights( );
	bool ShouldDraw();
	virtual ShadowType_t			ShadowCastType();

	int DrawModel(int flags);
private:
	C_ZombieRallyPoint( const C_ZombieRallyPoint & ); // not defined, not accessible
};

IMPLEMENT_CLIENTCLASS_DT(C_ZombieRallyPoint, DT_ZombieRallyPoint, CZombieRallyPoint)
END_RECV_TABLE()

C_ZombieRallyPoint::C_ZombieRallyPoint()
{
	
}


C_ZombieRallyPoint::~C_ZombieRallyPoint()
{
}

bool C_ZombieRallyPoint::ShouldDraw()
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();  //LAWYER:  Get the local player
	if ( !pPlayer )
	{
		return false;
	}
	if (pPlayer->IsZM() == false)
	{
		return false; //LAWYER:  Only Zombiemasters can see this!
	}
	//TGB: it was showing on DOTD because 0,0,0 is a visible location on the map there
	if (GetAbsOrigin().x == 0 && GetAbsOrigin().y == 0 && GetAbsOrigin().z == 0 )	//don't draw if our location is 0,0,0
		return false;

	//TGB FIXME should perhaps only draw when a spawn menu is open, bit of a bitch to track that

	return(BaseClass::ShouldDraw());

}

int C_ZombieRallyPoint::DrawModel(int flags)
{
	//TGB: kind of unnecessary considering shoulddraw checks, isn't it?
	CBasePlayer *pPlayer = CBasePlayer::GetLocalPlayer();
	if(pPlayer->IsZM() == false)
		return 0;

	return(BaseClass::DrawModel(flags));
}

//-----------------------------------------------------------------------------
// Should this object cast shadows?
//-----------------------------------------------------------------------------
ShadowType_t C_ZombieRallyPoint::ShadowCastType()
{
	return SHADOWS_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: Client-side ZombieManipulateTrigger
//-----------------------------------------------------------------------------
class C_ZombieManipulateTrigger : public C_BaseAnimating
{
public:
	DECLARE_CLASS( C_ZombieManipulateTrigger, C_BaseAnimating );
	DECLARE_CLIENTCLASS();

					C_ZombieManipulateTrigger();
	virtual			~C_ZombieManipulateTrigger();

	// model specific
//	virtual	void	SetupWeights( );
	bool ShouldDraw();
	virtual ShadowType_t			ShadowCastType();

	int DrawModel(int flags);
private:
	C_ZombieManipulateTrigger( const C_ZombieManipulateTrigger & ); // not defined, not accessible
};

IMPLEMENT_CLIENTCLASS_DT(C_ZombieManipulateTrigger, DT_ZombieManipulateTrigger, CZombieManipulateTrigger)
END_RECV_TABLE()

C_ZombieManipulateTrigger::C_ZombieManipulateTrigger()
{
}


C_ZombieManipulateTrigger::~C_ZombieManipulateTrigger()
{
}

bool C_ZombieManipulateTrigger::ShouldDraw()
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();  //LAWYER:  Get the local player
	if ( !pPlayer )
	{
		return false;
	}
	if (pPlayer->IsZM() == false)
	{
		return false; //LAWYER:  Only Zombiemasters can see this!
	}
	return(BaseClass::ShouldDraw());

	//return (model != 0) && !IsEffectActive(EF_NODRAW) && (index != 0);
}

int C_ZombieManipulateTrigger::DrawModel(int flags)
{
/*	//qck: Selective spawn glow sprite code.
	CBasePlayer *pPlayer = CBasePlayer::GetLocalPlayer();
	if(pPlayer->IsZM() == false)
		return 0;

	Vector attachOrigin;
	QAngle attachAngles;
	
	color32 color;
	color.r = 255;
	color.g = 255;
	color.b = 255;
	color.a = 0;
	
	int attachment = this->LookupAttachment( "glowsource");
	GetAttachment( attachment, attachOrigin, attachAngles );

	IMaterial *pMaterial = materials->FindMaterial( "models/orange", TEXTURE_GROUP_CLIENT_EFFECTS, false );
	pMaterial->SetMaterialVarFlag(MATERIAL_VAR_ALPHATEST, true);
    
	materials->Bind( pMaterial );
	DrawSprite( this->GetAbsOrigin(), 128, 128, color);
*/ //LAWYER;  Traps don't glow.
	return(BaseClass::DrawModel(flags));
}

//-----------------------------------------------------------------------------
// Should this object cast shadows?
//-----------------------------------------------------------------------------
ShadowType_t C_ZombieManipulateTrigger::ShadowCastType()
{
		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();  //LAWYER:  Get the local player
	if ( !pPlayer )
	{
		return SHADOWS_NONE;
	}
	if (pPlayer->IsZM() == false)
	{
		return SHADOWS_NONE; //LAWYER:  Only Zombiemasters can see this!
	}

	return BaseClass::ShadowCastType();
}





//-----------------------------------------------------------------------------
// Purpose: Client-side ZombieAmbushPoint
//-----------------------------------------------------------------------------
class C_ZombieAmbushPoint : public C_BaseAnimating
{
public:
	DECLARE_CLASS( C_ZombieAmbushPoint, C_BaseAnimating );
	DECLARE_CLIENTCLASS();

	C_ZombieAmbushPoint();
	~C_ZombieAmbushPoint();

	// model specific
	//	virtual	void	SetupWeights( );
	bool ShouldDraw();
	virtual ShadowType_t			ShadowCastType();

	int DrawModel(int flags);
private:

	C_ZombieAmbushPoint( const C_ZombieAmbushPoint & ); // not defined, not accessible
};

IMPLEMENT_CLIENTCLASS_DT(C_ZombieAmbushPoint, DT_ZombieAmbushPoint, CZombieAmbushPoint)
END_RECV_TABLE()

C_ZombieAmbushPoint::C_ZombieAmbushPoint()
{
}


C_ZombieAmbushPoint::~C_ZombieAmbushPoint()
{
}

bool C_ZombieAmbushPoint::ShouldDraw()
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();  //LAWYER:  Get the local player
	if ( !pPlayer )
	{
		return false;
	}
	if (pPlayer->IsZM() == false)
	{
		return false; //LAWYER:  Only Zombiemasters can see this!
	}
	return(BaseClass::ShouldDraw());
}

int C_ZombieAmbushPoint::DrawModel(int flags)
{
	//TGB: tried to do a trigger range indicator sprite here, but it seems impossible to easily draw
	//	a horizontal sprite. DrawSprite hardcodes it to be vertical to the player.
/*
	CBasePlayer *pPlayer = CBasePlayer::GetLocalPlayer();
	if(pPlayer->IsZM() == false)
		return 0;

	//Vector attachOrigin;
	//QAngle attachAngles;

	color32 color;
	color.r = 1;
	color.g = 200;
	color.b = 200;
	color.a = 50;

	//const int attachment = LookupAttachment( "glowsource");
	//GetAttachment( attachment, attachOrigin, attachAngles );

	IMaterial *pMaterial = materials->FindMaterial( "effects/zm_radius", TEXTURE_GROUP_CLIENT_EFFECTS, false );
	pMaterial->SetMaterialVarFlag(MATERIAL_VAR_ALPHATEST, true);

	materials->Bind( pMaterial );

	DrawSprite( GetAbsOrigin(), radius, radius, color);
*/
	return(BaseClass::DrawModel(flags));
}

//-----------------------------------------------------------------------------
// Should this object cast shadows?
//-----------------------------------------------------------------------------
ShadowType_t C_ZombieAmbushPoint::ShadowCastType()
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();  //LAWYER:  Get the local player
	if ( !pPlayer )
	{
		return SHADOWS_NONE;
	}
	if (pPlayer->IsZM() == false)
	{
		return SHADOWS_NONE; //LAWYER:  Only Zombiemasters can see this!
	}

	return BaseClass::ShadowCastType();
}


//-----------------------------------------------------------------------------
// Purpose: Client-side ZombieSpawnNode
//-----------------------------------------------------------------------------
class C_ZombieSpawnNode : public C_BaseAnimating
{
public:
	DECLARE_CLASS( C_ZombieSpawnNode, C_BaseAnimating );
	DECLARE_CLIENTCLASS();

					C_ZombieSpawnNode();
	virtual			~C_ZombieSpawnNode();

	// model specific
//	virtual	void	SetupWeights( );
	bool ShouldDraw();
	virtual ShadowType_t			ShadowCastType();

	int DrawModel(int flags);
private:
	C_ZombieSpawnNode( const C_ZombieSpawnNode & ); // not defined, not accessible
};

IMPLEMENT_CLIENTCLASS_DT(C_ZombieSpawnNode, DT_ZombieSpawnNode, CZombieSpawnNode)
END_RECV_TABLE()

C_ZombieSpawnNode::C_ZombieSpawnNode()
{
	
}


C_ZombieSpawnNode::~C_ZombieSpawnNode()
{
}

bool C_ZombieSpawnNode::ShouldDraw()
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();  //LAWYER:  Get the local player
	if ( !pPlayer )
	{
		return false;
	}
	if (pPlayer->IsZM() == false)
	{
		return false; //LAWYER:  Only Zombiemasters can see this!
	}
	//TGB: it was showing on DOTD because 0,0,0 is a visible location on the map there
	if (GetAbsOrigin().x == 0 && GetAbsOrigin().y == 0 && GetAbsOrigin().z == 0 )	//don't draw if our location is 0,0,0
		return false;

	//TGB FIXME should perhaps only draw when a spawn menu is open, bit of a bitch to track that

	return(BaseClass::ShouldDraw());

}

int C_ZombieSpawnNode::DrawModel(int flags)
{
	//TGB: kind of unnecessary considering shoulddraw checks, isn't it?
	CBasePlayer *pPlayer = CBasePlayer::GetLocalPlayer();
	if(pPlayer->IsZM() == false)
		return 0;

	return(BaseClass::DrawModel(flags));
}

//-----------------------------------------------------------------------------
// Should this object cast shadows?
//-----------------------------------------------------------------------------
ShadowType_t C_ZombieSpawnNode::ShadowCastType()
{
	return SHADOWS_NONE;
}