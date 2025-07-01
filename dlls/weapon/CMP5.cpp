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
#include "skill.h"
#include "weapons.h"
#include "nodes.h"
#include "CBasePlayer.h"
#include "env/CSoundEnt.h"
#include "gamerules.h"
#include "weapon/CMP5.h"
#include "weapon/CGrenade.h"

enum mp5_e
{
	MP5_LONGIDLE = 0,
	MP5_IDLE1,
	MP5_LAUNCH,
	MP5_RELOAD,
	MP5_DEPLOY,
	MP5_FIRE1,
	MP5_FIRE2,
	MP5_FIRE3,
};


LINK_ENTITY_TO_CLASS( weapon_mp5, CMP5 )
LINK_ENTITY_TO_CLASS( weapon_9mmAR, CMP5 )
LINK_ENTITY_TO_CLASS( weapon_9mmar, CMP5 )

// TODO: implement these weapons instead of remapping them
LINK_ENTITY_TO_CLASS(weapon_uzi, CMP5)
LINK_ENTITY_TO_CLASS(weapon_uziakimbo, CMP5)
LINK_ENTITY_TO_CLASS(weapon_m16, CMP5)
LINK_ENTITY_TO_CLASS(weapon_m249, CMP5)
LINK_ENTITY_TO_CLASS(weapon_saw, CMP5)
LINK_ENTITY_TO_CLASS(weapon_minigun, CMP5)

//=========================================================
//=========================================================
int CMP5::SecondaryAmmoIndex( void )
{
	return m_iSecondaryAmmoType;
}

void CMP5::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_9mmAR"); // hack to allow for old names
	Precache( );
	SetWeaponModelW();
	m_iId = WEAPON_MP5;

	m_iDefaultAmmo = MP5_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CMP5::Precache( void )
{
	m_hasHandModels = true;
	m_defaultModelV = "models/v_9mmAR.mdl";
	m_defaultModelP = "models/p_9mmAR.mdl";
	m_defaultModelW = "models/w_9mmAR.mdl";
	CBasePlayerWeapon::Precache();

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shellTE_MODEL

	PRECACHE_MODEL("models/grenade.mdl");	// grenade
	PRECACHE_SOUND("items/9mmclip1.wav");              

	PRECACHE_SOUND("items/clipinsert1.wav");
	PRECACHE_SOUND("items/cliprelease1.wav");
	PRECACHE_SOUND("weapons/9mmar_reload.wav"); // combined reload sound for less network usage and code

	PRECACHE_SOUND ("weapons/hks1.wav");// H to the K
	PRECACHE_SOUND ("weapons/hks2.wav");// H to the K
	PRECACHE_SOUND ("weapons/hks3.wav");// H to the K

	PRECACHE_SOUND( "weapons/glauncher.wav" );
	PRECACHE_SOUND( "weapons/glauncher2.wav" );

	PRECACHE_SOUND ("weapons/357_cock1.wav");

	UTIL_PrecacheOther("ammo_9mmAR");
	UTIL_PrecacheOther("ammo_ARgrenades");

	PrecacheEvents();
}

void CMP5::PrecacheEvents() {
	m_usMP5 = PRECACHE_EVENT(1, "events/mp5.sc");
	m_usMP52 = PRECACHE_EVENT(1, "events/mp52.sc");
}

int CMP5::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = gSkillData.sk_ammo_max_9mm;
	p->pszAmmo2 = "ARgrenades";
	p->iMaxAmmo2 = gSkillData.sk_ammo_max_argrenades;
	p->iMaxClip = MP5_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_MP5;
	p->iWeight = MP5_WEIGHT;

	return 1;
}

BOOL CMP5::Deploy( )
{
	return DefaultDeploy(GetModelV(), GetModelP(), MP5_DEPLOY, "mp5" );
}


void CMP5::PrimaryAttack()
{
	CBasePlayer* m_pPlayer = GetPlayer();
	if (!m_pPlayer)
		return;

	// JoshA: Sanitize this so it's not total garbage on level transition
	// and we end up ear blasting the player!

	float flDamage;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + Vector(0, 0, 0) );
	Vector vecAiming;
	
		vecAiming = gpGlobals->v_forward;
	Vector vecSrc = m_pPlayer->GetGunPosition( ); // + gpGlobals->v_up * -8 + gpGlobals->v_right * 8;
	ULONG cShots = 1;
	Vector vecSpread = VECTOR_CONE_9MMAR;
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
	
	float dmg_mult = GetDamageModifier();

		flDamage = gSkillData.sk_plr_9mmAR_bullet * dmg_mult * UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.9, 1.1 );


	if (m_fInAttack != 3)
	{
		//ALERT ( at_console, "Time:%f Damage:%f\n", gpGlobals->time - m_pPlayer->m_flStartCharge, flDamage );

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	}

	// time until aftershock 'static discharge' sound


	edict_t		*pentIgnore;
	TraceResult tr, beam_tr;
	float flMaxFrac = 1.0;
	int fFirstBeam = 1;

	pentIgnore = m_pPlayer->edict();
	// don't fire underwater
	//if (m_pPlayer->pev->waterlevel == 3)
	//{
		//PlayEmptySound( );
		//m_flNextPrimaryAttack = 0.15;
		//return;
	//}

	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;


	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	//Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	//Vector vecDir = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	  int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usMP5, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

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
			//te_debug_beam(vecSrc + (gpGlobals->v_up * -8) + (gpGlobals->v_forward * 36) + (gpGlobals->v_right * 4), tr.vecEndPos, 1, RGBA(255, 255, 255, flDamage), NULL, NULL);

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

	m_flNextPrimaryAttack = GetNextAttackDelay(0.1);

	if ( m_flNextPrimaryAttack < UTIL_WeaponTimeBase() )
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}



void CMP5::SecondaryAttack( void )
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

	if (m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] == 0)
	{
		PlayEmptySound( );
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	m_pPlayer->m_iExtraSoundTypes = bits_SOUND_DANGER;
	m_pPlayer->m_flStopExtraSoundTime = UTIL_WeaponTimeBase() + 0.2;
			
	m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType]--;

	strcpy_safe(m_pPlayer->m_szAnimExtention, "gauss", 32); // not enough recoil in the mp5 animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);
	strcpy_safe(m_pPlayer->m_szAnimExtention, "mp5", 32);

 	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	// we don't add in player velocity anymore.
	// yes we do
	CGrenade::ShootContact( m_pPlayer->pev, 
							m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16, 
							(gpGlobals->v_forward * 800)+m_pPlayer->pev->velocity );

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT( flags, m_pPlayer->edict(), m_usMP52 );
	
	m_flNextPrimaryAttack = GetNextAttackDelay(1);
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5;// idle pretty soon after shooting.

	if (!m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType])
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
}

void CMP5::Reload( void )
{
	CBasePlayer* m_pPlayer = GetPlayer();
	if (!m_pPlayer)
		return;

	if ( m_pPlayer->ammo_9mm <= 0 )
		return;

	if (DefaultReload(MP5_MAX_CLIP, MP5_RELOAD, 1.5)) {
#ifndef CLIENT_DLL
		m_pPlayer->SetAnimation(PLAYER_RELOAD, 1.5f);
			
		// send reload sound to everyone except the reloader if they're in first-person mode,
		// because the reloading client will play sounds via model events
		uint32_t messageTargets = 0xffffffff;
		if (m_pPlayer->IsFirstPerson()) {
			messageTargets &= ~PLRBIT(m_pPlayer->edict());
		}
		StartSound(m_pPlayer->edict(), CHAN_ITEM, "weapons/9mmar_reload.wav", 0.8f,
			ATTN_NORM, SND_FL_PREDICTED, 93 + RANDOM_LONG(0, 15), m_pPlayer->pev->origin, messageTargets);
#endif
	}
}


void CMP5::WeaponIdle( void )
{
	CBasePlayer* m_pPlayer = GetPlayer();
	if (!m_pPlayer)
		return;

	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim;
	switch ( RANDOM_LONG( 0, 1 ) )
	{
	case 0:	
		iAnim = MP5_LONGIDLE;	
		break;
	
	default:
	case 1:
		iAnim = MP5_IDLE1;
		break;
	}

	SendWeaponAnim( iAnim );

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 ); // how long till we do this again.
}
