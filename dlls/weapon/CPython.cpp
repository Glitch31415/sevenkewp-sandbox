/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#if !defined( OEM_BUILD ) && !defined( HLDEMO_BUILD )

#include "extdll.h"
#include "util.h"
#include "weapons.h"
#include "CBasePlayer.h"
#include "gamerules.h"
#include "weapon/CPython.h"

enum python_e {
	PYTHON_IDLE1 = 0,
	PYTHON_FIDGET,
	PYTHON_FIRE1,
	PYTHON_RELOAD,
	PYTHON_HOLSTER,
	PYTHON_DRAW,
	PYTHON_IDLE2,
	PYTHON_IDLE3
};

LINK_ENTITY_TO_CLASS( weapon_python, CPython )
LINK_ENTITY_TO_CLASS( weapon_357, CPython )
LINK_ENTITY_TO_CLASS(weapon_eagle, CPython) // TODO: implement

int CPython::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "357";
	p->iMaxAmmo1 = gSkillData.sk_ammo_max_357;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = PYTHON_MAX_CLIP;
	p->iFlags = 0;
	p->iSlot = 1;
	p->iPosition = 1;
	p->iId = m_iId = WEAPON_PYTHON;
	p->iWeight = PYTHON_WEIGHT;

	return 1;
}

void CPython::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_357"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_PYTHON;
	SetWeaponModelW();

	m_iDefaultAmmo = PYTHON_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CPython::Precache( void )
{
	m_hasHandModels = true;
	m_defaultModelV = "models/v_357.mdl";
	m_defaultModelP = "models/p_357.mdl";
	m_defaultModelW = "models/w_357.mdl";
	CBasePlayerWeapon::Precache();

	PRECACHE_SOUND("items/9mmclip1.wav");              

	PRECACHE_SOUND ("weapons/357_reload1.wav");
	PRECACHE_SOUND ("weapons/357_cock1.wav");
	PRECACHE_SOUND ("weapons/357_shot1.wav");
	PRECACHE_SOUND ("weapons/357_shot2.wav");

	UTIL_PrecacheOther("ammo_357");

	PrecacheEvents();
}

void CPython::PrecacheEvents() {
	m_usFirePython = PRECACHE_EVENT(1, "events/python.sc");
}

BOOL CPython::Deploy( )
{
#ifdef CLIENT_DLL
	if ( bIsMultiplayer() )
#else
	if ( g_pGameRules->IsMultiplayer() )
#endif
	{
		// enable laser sight geometry.
		pev->body = 1;
	}
	else
	{
		pev->body = 0;
	}

	return DefaultDeploy(GetModelV(), GetModelP(), PYTHON_DRAW, "python", UseDecrement(), pev->body );
}


void CPython::Holster( int skiplocal /* = 0 */ )
{
	CBasePlayer* m_pPlayer = GetPlayer();
	if (!m_pPlayer)
		return;

	m_fInReload = FALSE;// cancel any reload in progress.

	if ( m_fInZoom )
	{
		SecondaryAttack();
	}

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	SendWeaponAnim( PYTHON_HOLSTER );
}

void CPython::SecondaryAttack( void )
{
	CBasePlayer* m_pPlayer = GetPlayer();
	if (!m_pPlayer)
		return;

#ifdef CLIENT_DLL
	if ( !bIsMultiplayer() )
#else
	if ( !g_pGameRules->IsMultiplayer() )
#endif
	{
		return;
	}

	if ( m_pPlayer->pev->fov != 0 )
	{
		m_fInZoom = FALSE;
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0;  // 0 means reset to default fov
	}
	else if ( m_pPlayer->pev->fov != 40 )
	{
		m_fInZoom = TRUE;
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 40;
	}

	m_flNextSecondaryAttack = 0.5;
}

void CPython::PrimaryAttack()
{
	CBasePlayer* m_pPlayer = GetPlayer();
	if (!m_pPlayer)
		return;

	// don't fire underwater
	//if (m_pPlayer->pev->waterlevel == 3)
	//{
		//PlayEmptySound( );
		//m_flNextPrimaryAttack = 0.15;
		//return;
	//}

	if (m_iClip <= 0)
	{
		if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = 0.15;
		}

		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );


	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector vecAiming = gpGlobals->v_forward;
	Vector vecSrc = m_pPlayer->GetGunPosition( ); // + gpGlobals->v_up * -8 + gpGlobals->v_right * 8;
	ULONG cShots = 1;
	Vector vecSpread = VECTOR_CONE_357;
	int shared_rand = m_pPlayer->random_seed;
	float x, y, z;

	for ( ULONG iShot = 1; iShot <= cShots; iShot++ )
	{

			//Use player's random seed.
			// get circular gaussian spread
			x = UTIL_SharedRandomFloat( shared_rand + iShot, -0.5, 0.5 ) + UTIL_SharedRandomFloat( shared_rand + ( 1 + iShot ) , -0.5, 0.5 );
			y = UTIL_SharedRandomFloat( shared_rand + ( 2 + iShot ), -0.5, 0.5 ) + UTIL_SharedRandomFloat( shared_rand + ( 3 + iShot ), -0.5, 0.5 );
			z = x * x + y * y;

			
	}
    Vector spread = Vector ( x * vecSpread.x, y * vecSpread.y, 0.0 );
	Vector vecDir = (vecAiming + spread).Normalize();
	Vector vecDest = vecSrc + (vecDir * 8192);
	edict_t		*pentIgnore;
	TraceResult tr, beam_tr;
	float flMaxFrac = 1.0;
	float dmg_mult = GetDamageModifier();
	float flDamage = gSkillData.sk_plr_357_bullet * dmg_mult * UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.9, 1.1 );
	int fFirstBeam = 1;

	pentIgnore = m_pPlayer->edict();
	PLAY_DISTANT_SOUND(m_pPlayer->edict(), DISTANT_357);

    int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usFirePython, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	lagcomp_begin(m_pPlayer);
	m_pPlayer->pev->punchangle = Vector(-10, 0, 0);
int loops = 0;
while (flDamage > 1 && loops < 25)
	{
		loops = loops + 1;
		bool sdm = true;
		//UTIL_ClientPrintAll(print_chat, UTIL_VarArgs("flDamage begin: %f", flDamage));


		// ALERT( at_console, "." );
		UTIL_TraceLine(vecSrc, vecDest, dont_ignore_monsters, pentIgnore, &tr);


		//if (tr.fAllSolid)
			//break;

		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);
		//UTIL_ClientPrintAll(print_chat, UTIL_VarArgs("edict: %s", pEntity->DisplayName()));

		if (pEntity == NULL)
			break;

		if ( fFirstBeam )
		{
			m_pPlayer->pev->effects |= EF_MUZZLEFLASH;
			fFirstBeam = 0;
			//(vecSrc + (gpGlobals->v_up * -3.5) + (gpGlobals->v_forward * 23) + (gpGlobals->v_right * 5.5), tr.vecEndPos, 1, RGBA(255, 255, 255, flDamage), NULL, NULL);

		}
		else {
			//te_debug_beam(vecSrc, tr.vecEndPos, 1, RGBA(255, 255, 255, flDamage), NULL, NULL);
		}
		float n = 0;
		if (pEntity->pev->takedamage)
		{
			//UTIL_ClientPrintAll(print_chat, "hit monster");
			if (pEntity->pev->health <= 0)
				break;
			ClearMultiDamage();

			// if you hurt yourself clear the headshot bit

			float prevmaxhealth = pEntity->pev->max_health;
			float flpDamage = prevmaxhealth;
			float angcheck = sin(UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, M_PI_2));



			switch ((&tr)->iHitgroup)
			{
			case 0:
				//assume glass
				pEntity->TraceAttack( m_pPlayer->pev, flDamage, vecDir, &tr, DMG_BULLET );
				ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);
				flpDamage = prevmaxhealth * angcheck;
				flDamage = flDamage - flpDamage;
				break;
			case 1:
				//head
				pEntity->TraceAttack( m_pPlayer->pev, flDamage, vecDir, &tr, DMG_BULLET );
				ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);
				flpDamage = 0.6 * prevmaxhealth * angcheck;
				flDamage = flDamage - flpDamage;
				break;
			case 2:
				//chest
				pEntity->TraceAttack( m_pPlayer->pev, flDamage, vecDir, &tr, DMG_BULLET );
				ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);
				flpDamage = 1.125 * prevmaxhealth * angcheck;
				flDamage = flDamage - flpDamage;
				break;
			case 3:
				//stomach
				pEntity->TraceAttack( m_pPlayer->pev, flDamage, vecDir, &tr, DMG_BULLET );
				ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);
				flpDamage = 0.75 * prevmaxhealth * angcheck;
				flDamage = flDamage - flpDamage;
				break;
			case 4:
			case 5:
				//left + right arm
				pEntity->TraceAttack( m_pPlayer->pev, flDamage, vecDir, &tr, DMG_BULLET );
				ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);
				flpDamage = 0.45 * prevmaxhealth * angcheck;
				flDamage = flDamage - flpDamage;
				break;
			case 6:
			case 7:
				//left + right leg
				pEntity->TraceAttack( m_pPlayer->pev, flDamage, vecDir, &tr, DMG_BULLET );
				ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);
				flpDamage = 0.9 * prevmaxhealth * angcheck;
				flDamage = flDamage - flpDamage;
				break;
			case 10:
			case 11:
				//armor, don't know what type, fuck
				pEntity->TraceAttack( m_pPlayer->pev, flDamage, vecDir, &tr, DMG_BULLET );
				ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);
				flpDamage = 70 * angcheck;
				flDamage = flDamage - flpDamage;
				break;
			default:
				pEntity->TraceAttack( m_pPlayer->pev, flDamage, vecDir, &tr, DMG_BULLET );
				ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);
				flpDamage = 0.75 * prevmaxhealth * angcheck;
				flDamage = flDamage - flpDamage;
				//UTIL_ClientPrintAll(print_chat, "uh oh default");
				break;
			}

			



			//if (diffhealth < 0) {
				//diffhealth = pEntity->pev->max_health;
			//}
			//if (diffhealth < pEntity->pev->max_health*0.75) {
				//diffhealth = pEntity->pev->max_health*0.75;
			//}

			
			//UTIL_ClientPrintAll(print_chat, UTIL_VarArgs("flcDamage: %f", flcDamage));
			//UTIL_ClientPrintAll(print_chat, UTIL_VarArgs("flpDamage: %f", flpDamage));
			//UTIL_ClientPrintAll(print_chat, UTIL_VarArgs("flDamage 1: %f", flDamage));
			sdm = false;
			vecSrc = tr.vecEndPos + vecDir;
			pentIgnore = ENT( pEntity->pev );
		}
		else {
		//if ( pEntity->ReflectGauss() )
		//{
			//pentIgnore = NULL;
			//UTIL_ClientPrintAll(print_chat, "hit not monster");
			n = -DotProduct(tr.vecPlaneNormal, vecDir);

			if (n < 0.5) // 60 degrees
			{
				// ALERT( at_console, "reflect %f\n", n );
				// reflect
				Vector r;
			
				r = 2.0 * tr.vecPlaneNormal * n + vecDir;
				flMaxFrac = flMaxFrac - tr.flFraction;
				vecDir = r;
				vecSrc = tr.vecEndPos + vecDir * 8;
				vecDest = vecSrc + (vecDir * 8192);

				// explode a bit
				//m_pPlayer->RadiusDamage( tr.vecEndPos, pev, m_pPlayer->pev, flDamage * n, CLASS_NONE, DMG_BLAST );


				
				// lose energy
				if (n == 0) n = 0.1;
				flDamage = flDamage * (1 - n);
				//UTIL_ClientPrintAll(print_chat, UTIL_VarArgs("flDamage 2: %f", flDamage));
			}
		}


				// limit it to one hole punch

				// try punching through wall if secondary attack (primary is incapable of breaking through)
				//if ( !m_fPrimaryFire )
				//{
					UTIL_TraceLine( tr.vecEndPos + vecDir * 8, vecDest, dont_ignore_monsters, pentIgnore, &beam_tr);
					//if (!beam_tr.fAllSolid)
					//{
						// trace backwards to find exit point
						UTIL_TraceLine( beam_tr.vecEndPos, tr.vecEndPos, dont_ignore_monsters, pentIgnore, &beam_tr);

						n = (beam_tr.vecEndPos - tr.vecEndPos).Length( );

						//if (n < flDamage)
						//{
							if (n == 0) n = 1;
							if (sdm == true && pEntity->pev->rendermode == kRenderNormal) { // if not a damage-able entity and if not transparent
									flDamage -= 7.5*n;

							}
							//UTIL_ClientPrintAll(print_chat, UTIL_VarArgs("flDamage 3: %f", flDamage));

							// ALERT( at_console, "punch %f\n", n );


							// exit blast damage
							//m_pPlayer->RadiusDamage( beam_tr.vecEndPos + vecDir * 8, pev, m_pPlayer->pev, flDamage, CLASS_NONE, DMG_BLAST );

							//::RadiusDamage( beam_tr.vecEndPos + vecDir * 8, pev, m_pPlayer->pev, flDamage, damage_radius, CLASS_NONE, DMG_BLAST );




							vecSrc = beam_tr.vecEndPos + vecDir;
						//}
					//}
					//else
					//{
						 //ALERT( at_console, "blocked %f\n", n );
						//flDamage = 0;
					//}
				//}
				//else
				//{
					//ALERT( at_console, "blocked solid\n" );
					
					//flDamage = 0;
				//}
		//}
		//else
		//{
			//vecSrc = tr.vecEndPos + vecDir;
			//pentIgnore = ENT( pEntity->pev );
		//}
	}
	
	lagcomp_end();



	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = 0.33333;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}


void CPython::Reload( void )
{
	CBasePlayer* m_pPlayer = GetPlayer();
	if (!m_pPlayer)
		return;

	if ( m_pPlayer->ammo_357 <= 0 )
		return;

	if ( m_pPlayer->pev->fov != 0 )
	{
		m_fInZoom = FALSE;
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0;  // 0 means reset to default fov
	}

	int bUseScope = FALSE;
#ifdef CLIENT_DLL
	bUseScope = bIsMultiplayer();
#else
	bUseScope = g_pGameRules->IsMultiplayer();
#endif

	if (DefaultReload(6, PYTHON_RELOAD, 2.0, bUseScope)) {
#ifndef CLIENT_DLL
		m_pPlayer->SetAnimation(PLAYER_RELOAD, 2.0f);

		// send reload sound to everyone except the reloader if they're in first-person mode,
		// because the reloading client will play sounds via model events
		uint32_t messageTargets = 0xffffffff;
		if (m_pPlayer->IsFirstPerson()) {
			messageTargets &= ~PLRBIT(m_pPlayer->edict());
		}
		StartSound(m_pPlayer->edict(), CHAN_ITEM, "weapons/357_reload1.wav", 0.8f,
			ATTN_NORM, SND_FL_PREDICTED, 93 + RANDOM_LONG(0, 15), m_pPlayer->pev->origin, messageTargets);
#endif
	}
}


void CPython::WeaponIdle( void )
{
	CBasePlayer* m_pPlayer = GetPlayer();
	if (!m_pPlayer)
		return;

	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim;
	float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
	if (flRand <= 0.5)
	{
		iAnim = PYTHON_IDLE1;
		m_flTimeWeaponIdle = (70.0/30.0);
	}
	else if (flRand <= 0.7)
	{
		iAnim = PYTHON_IDLE2;
		m_flTimeWeaponIdle = (60.0/30.0);
	}
	else if (flRand <= 0.9)
	{
		iAnim = PYTHON_IDLE3;
		m_flTimeWeaponIdle = (88.0/30.0);
	}
	else
	{
		iAnim = PYTHON_FIDGET;
		m_flTimeWeaponIdle = (170.0/30.0);
	}
	
	int bUseScope = FALSE;
#ifdef CLIENT_DLL
	bUseScope = bIsMultiplayer();
#else
	bUseScope = g_pGameRules->IsMultiplayer();
#endif
	
	SendWeaponAnim( iAnim, UseDecrement() ? 1 : 0, bUseScope );
}

#endif