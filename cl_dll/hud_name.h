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
#ifndef HUD_NAME_H
#define HUD_NAME_H

class CHudName : public CHudElement , public vgui::Panel
{
DECLARE_CLASS_SIMPLE( CHudName, vgui::Panel );
public:
	CHudName( const char *pElementName );	
	void Init( void );	
	void VidInit( void );	
	void Reset();	
	void Paint( void );	
	void OnThink(void);	
private:
 	vgui::Label *m_pNameLabel;	
 	vgui::HScheme scheme; 	 
 	const char *m_pName;	
 	C_BasePlayer *m_pPlayer;
	float m_flLastTrace; //TGB

};
#endif