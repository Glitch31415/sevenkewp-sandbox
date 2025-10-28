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

#include "extdll.h"
#include "util.h"
#include "weapon/weapons.h"
#include "nodes.h"
#include "CBasePlayer.h"
#include "weapon/CGlock.h"
#include "skill.h"

enum glock_e {
	GLOCK_IDLE1 = 0,
	GLOCK_IDLE2,
	GLOCK_IDLE3,
	GLOCK_SHOOT,
	GLOCK_SHOOT_EMPTY,
	GLOCK_RELOAD,
	GLOCK_RELOAD_NOT_EMPTY,
	GLOCK_DRAW,
	GLOCK_HOLSTER,
	GLOCK_ADD_SILENCER
};

LINK_ENTITY_TO_CLASS( weapon_glock, CGlock )
LINK_ENTITY_TO_CLASS( weapon_9mmhandgun, CGlock )


void CGlock::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_9mmhandgun"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_GLOCK;
	SetWeaponModelW();

	m_iDefaultAmmo = GLOCK_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CGlock::Precache( void )
{
	m_hasHandModels = true;
	m_defaultModelV = "models/v_9mmhandgun.mdl";
	m_defaultModelP = "models/p_9mmhandgun.mdl";
	m_defaultModelW = "models/w_9mmhandgun.mdl";
	CBasePlayerWeapon::Precache();

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell

	PRECACHE_SOUND("items/9mmclip1.wav");
	PRECACHE_SOUND("items/9mmclip2.wav");
	PRECACHE_SOUND("weapons/glock_reload.wav");

	PRECACHE_SOUND ("weapons/pl_gun1.wav");//silenced handgun
	PRECACHE_SOUND ("weapons/pl_gun2.wav");//silenced handgun
	PRECACHE_SOUND ("weapons/pl_gun3.wav");//handgun

	UTIL_PrecacheOther("ammo_9mm");
	UTIL_PrecacheOther("ammo_9mmclip");

	PrecacheEvents();
}

void CGlock::PrecacheEvents() {
	m_usFireGlock1 = PRECACHE_EVENT(1, "events/glock1.sc");
	m_usFireGlock2 = PRECACHE_EVENT(1, "events/glock2.sc");
}

int CGlock::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = gSkillData.sk_ammo_max_9mm;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = GLOCK_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_GLOCK;
	p->iWeight = GLOCK_WEIGHT;

	p->fAccuracyDeg = 2.0f * atanf(0.01f) * (180.0f / M_PI);
	p->fAccuracyDeg2 = 2.0f * atanf(0.1f) * (180.0f / M_PI);
	p->iFlagsEx = WEP_FLAG_SECONDARY_ACCURACY;
	return 1;
}

BOOL CGlock::Deploy( )
{
	// pev->body = 1;
	return DefaultDeploy(GetModelV(), GetModelP(), GLOCK_DRAW, "onehanded", /*UseDecrement() ? 1 : 0*/ 0 );
}

void CGlock::SecondaryAttack( void )
{

	GlockFire( 0.0086825, 0.25, FALSE );
}

void CGlock::PrimaryAttack( void )
{
	GlockFire( 0.0023153, 0.75, TRUE );
}

void CGlock::GlockFire( float flSpread , float flCycleTime, BOOL fUseAutoAim )
{
	CBasePlayer* m_pPlayer = GetPlayer();
	if (!m_pPlayer)
		return;

	if (m_iClip <= 0)
	{
		if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = GetNextAttackDelay(0.2);
		}

		return;
	}

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	int flags;

#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	// silenced
	if (pev->body == 1)
	{
		m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;
	}
	else
	{
		// non-silenced
		m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
	}
		Vector vecSrc = m_pPlayer->GetGunPosition( );
	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector vecAiming;
	

	vecAiming = gpGlobals->v_forward;
	ULONG cShots = 1;
	Vector vecSpread = Vector( flSpread, flSpread, flSpread );
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
	Vector vecDest = vecSrc + (vecDir * 131072);
	edict_t		*pentIgnore;
	TraceResult tr, beam_tr;
	float flMaxFrac = 1.0;
	float dmg_mult = GetDamageModifier();

	float flDamage = gSkillData.sk_plr_9mm_bullet * dmg_mult * UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.9, 1.1 );
	int fFirstBeam = 1;

	pentIgnore = m_pPlayer->edict();





	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), fUseAutoAim ? m_usFireGlock1 : m_usFireGlock2, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, ( m_iClip == 0 ) ? 1 : 0, 0 );

	PLAY_DISTANT_SOUND(m_pPlayer->edict(), DISTANT_9MM);

	lagcomp_begin(m_pPlayer);
	m_pPlayer->pev->punchangle = Vector(-5, 0, 0);
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
			//te_debug_beam(vecSrc + (gpGlobals->v_up * -7) + (gpGlobals->v_forward * 24) + (gpGlobals->v_right * 6), tr.vecEndPos, 1, RGBA(255, 255, 255, flDamage), NULL, NULL);

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
				vecDest = vecSrc + (vecDir * 131072);

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
	
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(flCycleTime);

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}


void CGlock::Reload( void )
{
	CBasePlayer* m_pPlayer = GetPlayer();
	if (!m_pPlayer)
		return;

	if ( m_pPlayer->ammo_9mm <= 0 )
		 return;


	if (DefaultReload(17, m_iClip ? GLOCK_RELOAD_NOT_EMPTY : GLOCK_RELOAD, 1.5)) {
#ifndef CLIENT_DLL
		m_pPlayer->SetAnimation(PLAYER_RELOAD, 1.5f);

		// send reload sound to everyone except the reloader if they're in first-person mode,
		// because the reloading client will play sounds via model events
		uint32_t messageTargets = 0xffffffff;
		if (m_pPlayer->IsFirstPerson()) {
			messageTargets &= ~PLRBIT(m_pPlayer->edict());
		}
		StartSound(m_pPlayer->edict(), CHAN_ITEM, "weapons/glock_reload.wav", 0.8f,
			ATTN_NORM, SND_FL_PREDICTED, 93 + RANDOM_LONG(0, 15), m_pPlayer->pev->origin, messageTargets);

		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
#endif
	}
}



void CGlock::WeaponIdle( void )
{
	CBasePlayer* m_pPlayer = GetPlayer();
	if (!m_pPlayer)
		return;

	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	// only idle if the slid isn't back
	if (m_iClip != 0)
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0, 1.0 );

		if (flRand <= 0.3 + 0 * 0.75)
		{
			iAnim = GLOCK_IDLE3;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 49.0 / 16;
		}
		else if (flRand <= 0.6 + 0 * 0.875)
		{
			iAnim = GLOCK_IDLE1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 60.0 / 16.0;
		}
		else
		{
			iAnim = GLOCK_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 40.0 / 16.0;
		}
		SendWeaponAnim( iAnim, 1 );
	}
}

void CGlock::GetAmmoDropInfo(bool secondary, const char*& ammoEntName, int& dropAmount) {
	ammoEntName = "ammo_9mm";
	dropAmount = 17;
}