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
#include "weapons.h"
#include "nodes.h"
#include "CBasePlayer.h"
#include "gamerules.h"
#include "weapon/CShotgun.h"

// special deathmatch shotgun spreads
#define VECTOR_CONE_DM_SHOTGUN	Vector( 0.08716, 0.04362, 0.00  )// 10 degrees by 5 degrees
#define VECTOR_CONE_DM_DOUBLESHOTGUN Vector( 0.17365, 0.04362, 0.00 ) // 20 degrees by 5 degrees

enum shotgun_e {
	SHOTGUN_IDLE = 0,
	SHOTGUN_FIRE,
	SHOTGUN_FIRE2,
	SHOTGUN_RELOAD,
	SHOTGUN_PUMP,
	SHOTGUN_START_RELOAD,
	SHOTGUN_DRAW,
	SHOTGUN_HOLSTER,
	SHOTGUN_IDLE4,
	SHOTGUN_IDLE_DEEP
};

#ifndef CLIENT_DLL
TYPEDESCRIPTION	CShotgun::m_SaveData[] =
{
	DEFINE_FIELD(CShotgun, m_flNextReload, FIELD_TIME),
	DEFINE_FIELD(CShotgun, m_fInSpecialReload, FIELD_INTEGER),
	DEFINE_FIELD(CShotgun, m_flNextReload, FIELD_TIME),
	// DEFINE_FIELD( CShotgun, m_iShell, FIELD_INTEGER ),
	DEFINE_FIELD(CShotgun, m_flPumpTime, FIELD_TIME),
};
IMPLEMENT_SAVERESTORE(CShotgun, CBasePlayerWeapon)
#endif

LINK_ENTITY_TO_CLASS( weapon_shotgun, CShotgun )

void CShotgun::Spawn( )
{
	Precache( );
	m_iId = WEAPON_SHOTGUN;
	SetWeaponModelW();

	m_iDefaultAmmo = SHOTGUN_DEFAULT_GIVE;

	FallInit();// get ready to fall
}


void CShotgun::Precache( void )
{
	m_defaultModelV = "models/v_shotgun.mdl";
	m_defaultModelP = "models/p_shotgun.mdl";
	m_defaultModelW = "models/w_shotgun.mdl";
	CBasePlayerWeapon::Precache();

	m_iShell = PRECACHE_MODEL ("models/shotgunshell.mdl");// shotgun shell

	PRECACHE_SOUND("items/9mmclip1.wav");              

	PRECACHE_SOUND ("weapons/dbarrel1.wav");//shotgun
	PRECACHE_SOUND ("weapons/sbarrel1.wav");//shotgun

	PRECACHE_SOUND ("weapons/reload1.wav");	// shotgun reload
	PRECACHE_SOUND ("weapons/reload3.wav");	// shotgun reload

//	PRECACHE_SOUND ("weapons/sshell1.wav");	// shotgun reload - played on client
//	PRECACHE_SOUND ("weapons/sshell3.wav");	// shotgun reload - played on client
	
	PRECACHE_SOUND ("weapons/357_cock1.wav"); // gun empty sound
	PRECACHE_SOUND ("weapons/scock1.wav");	// cock gun

	UTIL_PrecacheOther("ammo_buckshot");

	PrecacheEvents();
}

void CShotgun::PrecacheEvents() {
	m_usSingleFire = PRECACHE_EVENT(1, "events/shotgun1.sc");
	m_usDoubleFire = PRECACHE_EVENT(1, "events/shotgun2.sc");
}

int CShotgun::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "buckshot";
	p->iMaxAmmo1 = gSkillData.sk_ammo_max_buckshot;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = SHOTGUN_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 1;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_SHOTGUN;
	p->iWeight = SHOTGUN_WEIGHT;

	return 1;
}



BOOL CShotgun::Deploy( )
{
	return DefaultDeploy(GetModelV(), GetModelP(), SHOTGUN_DRAW, "shotgun" );
}

void CShotgun::PrimaryAttack()
{
	CBasePlayer* m_pPlayer = GetPlayer();
	if (!m_pPlayer)
		return;

	// don't fire underwater
	//if (m_pPlayer->pev->waterlevel == 3)
	//{
		//PlayEmptySound( );
		//m_flNextPrimaryAttack = GetNextAttackDelay(0.15);
		//return;
	//}

	if (m_iClip <= 0)
	{
		Reload( );
		if (m_iClip == 0)
			PlayEmptySound( );
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif


	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;
	UTIL_MakeVectors( m_pPlayer->pev->v_angle );

	//Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	//Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	Vector vecDir;
	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usSingleFire, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );
	m_pPlayer->pev->punchangle = Vector(-7.5, 0, 0);
	PLAY_DISTANT_SOUND(m_pPlayer->edict(), DISTANT_556);
	lagcomp_begin(m_pPlayer);

		//vecDir = m_pPlayer->FireBulletsPlayer( 9, vecSrc, vecAiming, VECTOR_CONE_1DEGREES*1.5, 131072, BULLET_PLAYER_BUCKSHOT, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
		
			float flDamage;
			edict_t		*pentIgnore;
			TraceResult tr, beam_tr;
			float flMaxFrac = 1.0;
			int fFirstBeam = 1;
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + Vector(0, 0, 0) );
	Vector vecAiming;
	
		vecAiming = gpGlobals->v_forward;
	Vector vecSrc = m_pPlayer->GetGunPosition( ); // + gpGlobals->v_up * -8 + gpGlobals->v_right * 8;
	ULONG cShots = 9;
	Vector vecSpread = VECTOR_CONE_1DEGREES*1.5;
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
	vecDir = (vecAiming + spread).Normalize();
	Vector vecDest = vecSrc + (vecDir * 8192);
	
	float dmg_mult = GetDamageModifier();

		flDamage = gSkillData.sk_plr_buckshot * dmg_mult * UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.9, 1.1 );
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
			UTIL_BeamPoints(vecSrc + (gpGlobals->v_up * -7) + (gpGlobals->v_forward * 24) + (gpGlobals->v_right * 6), tr.vecEndPos, MODEL_INDEX("sprites/laserbeam.spr"), 0, 0, 1, 8, 0, RGBA(255, 255, 255, flDamage), 0, NULL, NULL, NULL);

		}
		else {
			UTIL_BeamPoints(vecSrc, tr.vecEndPos, MODEL_INDEX("sprites/laserbeam.spr"), 0, 0, 1, 8, 0, RGBA(255, 255, 255, flDamage), 0, NULL, NULL, NULL);
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
	flpDamage = 0.4 * prevmaxhealth * angcheck;
	flDamage = flDamage - flpDamage;
	break;
case 2:
	//chest
	pEntity->TraceAttack( m_pPlayer->pev, flDamage, vecDir, &tr, DMG_BULLET );
	ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);
	flpDamage = 0.75 * prevmaxhealth * angcheck;
	flDamage = flDamage - flpDamage;
	break;
case 3:
	//stomach
	pEntity->TraceAttack( m_pPlayer->pev, flDamage, vecDir, &tr, DMG_BULLET );
	ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);
	flpDamage = 0.5 * prevmaxhealth * angcheck;
	flDamage = flDamage - flpDamage;
	break;
case 4:
case 5:
	//left + right arm
	pEntity->TraceAttack( m_pPlayer->pev, flDamage, vecDir, &tr, DMG_BULLET );
	ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);
	flpDamage = 0.3 * prevmaxhealth * angcheck;
	flDamage = flDamage - flpDamage;
	break;
case 6:
case 7:
	//left + right leg
	pEntity->TraceAttack( m_pPlayer->pev, flDamage, vecDir, &tr, DMG_BULLET );
	ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);
	flpDamage = 0.6 * prevmaxhealth * angcheck;
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
	flpDamage = 0.5 * prevmaxhealth * angcheck;
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
							if (sdm == true) { // if not a damage-able entity
								if (pEntity->pev->rendermode == kRenderNormal) { // if not transparent
									flDamage -= 6*n;
								}
								else {
									flDamage -= n;
								}
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

	m_flPumpTime = gpGlobals->time + 0.5;

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	m_flNextPrimaryAttack = GetNextAttackDelay(0.75);
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.75;
	if (m_iClip != 0)
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0;
	else
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.75;
	m_fInSpecialReload = 0;
}


void CShotgun::SecondaryAttack( void )
{
	CBasePlayer* m_pPlayer = GetPlayer();
	if (!m_pPlayer)
		return;

	// don't fire underwater
	//if (m_pPlayer->pev->waterlevel == 3)
	//{
		//PlayEmptySound( );
		//m_flNextPrimaryAttack = GetNextAttackDelay(0.15);
		//return;
	//}

	if (m_iClip <= 1)
	{
		Reload( );
		PlayEmptySound( );
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip -= 2;


	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	//Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	//Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	Vector vecDir;
	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usDoubleFire, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );
	m_pPlayer->pev->punchangle = Vector(-15, 0, 0);
	PLAY_DISTANT_SOUND(m_pPlayer->edict(), DISTANT_556);
	lagcomp_begin(m_pPlayer);


		// untouched default single player
		//vecDir = m_pPlayer->FireBulletsPlayer( 18, vecSrc, vecAiming, VECTOR_CONE_1DEGREES*3, 131072, BULLET_PLAYER_BUCKSHOT, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
			float flDamage;
			edict_t		*pentIgnore;
			TraceResult tr, beam_tr;
			float flMaxFrac = 1.0;
			int fFirstBeam = 1;
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + Vector(0, 0, 0) );
	Vector vecAiming;
	
		vecAiming = gpGlobals->v_forward;
	Vector vecSrc = m_pPlayer->GetGunPosition( ); // + gpGlobals->v_up * -8 + gpGlobals->v_right * 8;
	ULONG cShots = 18;
	Vector vecSpread = VECTOR_CONE_1DEGREES*3;
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
	vecDir = (vecAiming + spread).Normalize();
	Vector vecDest = vecSrc + (vecDir * 8192);
	
	float dmg_mult = GetDamageModifier();

		flDamage = gSkillData.sk_plr_buckshot * dmg_mult * UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.9, 1.1 );
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
			UTIL_BeamPoints(vecSrc + (gpGlobals->v_up * -7) + (gpGlobals->v_forward * 24) + (gpGlobals->v_right * 6), tr.vecEndPos, MODEL_INDEX("sprites/laserbeam.spr"), 0, 0, 1, 8, 0, RGBA(255, 255, 255, flDamage), 0, NULL, NULL, NULL);

		}
		else {
			UTIL_BeamPoints(vecSrc, tr.vecEndPos, MODEL_INDEX("sprites/laserbeam.spr"), 0, 0, 1, 8, 0, RGBA(255, 255, 255, flDamage), 0, NULL, NULL, NULL);
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
	flpDamage = 0.4 * prevmaxhealth * angcheck;
	flDamage = flDamage - flpDamage;
	break;
case 2:
	//chest
	pEntity->TraceAttack( m_pPlayer->pev, flDamage, vecDir, &tr, DMG_BULLET );
	ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);
	flpDamage = 0.75 * prevmaxhealth * angcheck;
	flDamage = flDamage - flpDamage;
	break;
case 3:
	//stomach
	pEntity->TraceAttack( m_pPlayer->pev, flDamage, vecDir, &tr, DMG_BULLET );
	ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);
	flpDamage = 0.5 * prevmaxhealth * angcheck;
	flDamage = flDamage - flpDamage;
	break;
case 4:
case 5:
	//left + right arm
	pEntity->TraceAttack( m_pPlayer->pev, flDamage, vecDir, &tr, DMG_BULLET );
	ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);
	flpDamage = 0.3 * prevmaxhealth * angcheck;
	flDamage = flDamage - flpDamage;
	break;
case 6:
case 7:
	//left + right leg
	pEntity->TraceAttack( m_pPlayer->pev, flDamage, vecDir, &tr, DMG_BULLET );
	ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);
	flpDamage = 0.6 * prevmaxhealth * angcheck;
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
	flpDamage = 0.5 * prevmaxhealth * angcheck;
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
							if (sdm == true) { // if not a damage-able entity
								if (pEntity->pev->rendermode == kRenderNormal) { // if not transparent
									flDamage -= 6*n;
								}
								else {
									flDamage -= n;
								}
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

	m_flPumpTime = gpGlobals->time + 0.95;

	m_flNextPrimaryAttack = GetNextAttackDelay(1.5);
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.5;
	if (m_iClip != 0)
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 6.0;
	else
		m_flTimeWeaponIdle = 1.5;

	m_fInSpecialReload = 0;

}


void CShotgun::Reload( void )
{
	CBasePlayer* m_pPlayer = GetPlayer();
	if (!m_pPlayer)
		return;

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == SHOTGUN_MAX_CLIP)
		return;

	// don't reload until recoil is done
	if (m_flNextPrimaryAttack > UTIL_WeaponTimeBase())
		return;

	// check to see if we're ready to reload
	if (m_fInSpecialReload == 0)
	{
		SendWeaponAnim( SHOTGUN_START_RELOAD );
		m_pPlayer->SetAnimation(PLAYER_RELOAD, 1.0f);

		m_fInSpecialReload = 1;
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.6;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.6;
		m_flNextPrimaryAttack = GetNextAttackDelay(1.0);
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0;
		return;
	}
	else if (m_fInSpecialReload == 1)
	{
		if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
			return;
		// was waiting for gun to move to side
		m_fInSpecialReload = 2;

		m_pPlayer->SetAnimation(PLAYER_RELOAD, 0.4f);

		if (RANDOM_LONG(0,1))
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/reload1.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0,0x1f));
		else
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/reload3.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0,0x1f));

		SendWeaponAnim( SHOTGUN_RELOAD );

		m_flNextReload = UTIL_WeaponTimeBase() + 0.5;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;
	}
	else
	{
		// Add them to the clip
		m_iClip += 1;
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 1;
		m_fInSpecialReload = 1;
	}
}


void CShotgun::WeaponIdle( void )
{
	CBasePlayer* m_pPlayer = GetPlayer();
	if (!m_pPlayer)
		return;

	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if (m_flTimeWeaponIdle <  UTIL_WeaponTimeBase() )
	{
		if (m_iClip == 0 && m_fInSpecialReload == 0 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
		{
			Reload( );
		}
		else if (m_fInSpecialReload != 0)
		{
			if (m_iClip != 8 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
			{
				Reload( );
			}
			else
			{
				// reload debounce has timed out
				SendWeaponAnim( SHOTGUN_PUMP );
				
				// play cocking sound
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/scock1.wav", 1, ATTN_NORM, 0, 95 + RANDOM_LONG(0,0x1f));
				m_fInSpecialReload = 0;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.5;
			}
		}
		else
		{
			int iAnim;
			float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
			if (flRand <= 0.8)
			{
				iAnim = SHOTGUN_IDLE_DEEP;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (60.0/12.0);// * RANDOM_LONG(2, 5);
			}
			else if (flRand <= 0.95)
			{
				iAnim = SHOTGUN_IDLE;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (20.0/9.0);
			}
			else
			{
				iAnim = SHOTGUN_IDLE4;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (20.0/9.0);
			}
			SendWeaponAnim( iAnim );
		}
	}
}

void CShotgun::ItemPostFrame(void)
{
	if (m_hPlayer && m_flPumpTime && m_flPumpTime < gpGlobals->time)
	{
		// play pumping sound
		EMIT_SOUND_DYN(ENT(m_hPlayer->pev), CHAN_ITEM, "weapons/scock1.wav", 1, ATTN_NORM, 0, 95 + RANDOM_LONG(0, 0x1f));
		m_flPumpTime = 0;
	}

	CBasePlayerWeapon::ItemPostFrame();
}