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
#include "skill.h"
#include "CSoundEnt.h"
#include "decals.h"
#include "effects.h"
#include "CSpore.h"

#ifndef CLIENT_DLL
TYPEDESCRIPTION CSpore::m_SaveData[] =
{
	DEFINE_FIELD(CSpore, m_SporeType, FIELD_INTEGER),
	DEFINE_FIELD(CSpore, m_flIgniteTime, FIELD_TIME),
	DEFINE_FIELD(CSpore, m_bIsAI, FIELD_BOOLEAN),
	DEFINE_FIELD(CSpore, m_hSprite, FIELD_EHANDLE) };

IMPLEMENT_SAVERESTORE(CSpore, CSpore::BaseClass)
#endif

LINK_ENTITY_TO_CLASS(spore, CSpore)

void CSpore::Precache()
{
	m_defaultModel = "models/spore.mdl";
	PRECACHE_MODEL( GetModel() );
	PRECACHE_MODEL( "sprites/glow01.spr" );

	m_iBlow = PRECACHE_MODEL("sprites/spore_exp_01.spr" );
	m_iBlowSmall = PRECACHE_MODEL("sprites/spore_exp_c_01.spr" );
	m_iSpitSprite = m_iTrail = PRECACHE_MODEL("sprites/tinyspit.spr" );

	PRECACHE_SOUND("weapons/splauncher_impact.wav" );
	PRECACHE_SOUND("weapons/splauncher_bounce.wav" );
}

void CSpore::Spawn()
{
	Precache();

	if( m_SporeType == SporeType::GRENADE )
		pev->movetype = MOVETYPE_BOUNCE;
	else {
		pev->movetype = MOVETYPE_FLY;
	}

	pev->solid = SOLID_BBOX;

	SetGrenadeModel();

	UTIL_SetSize( pev, g_vecZero, g_vecZero );

	UTIL_SetOrigin( pev, pev->origin );

	SetThink( &CSpore::FlyThink );

	if( m_SporeType == SporeType::GRENADE )
	{
		SetTouch( &CSpore::MyBounceTouch );

		if( !m_bPuked )
		{
			pev->angles.x -= RANDOM_LONG( -5, 5 ) + 30;
		}
	}
	else
	{
		SetTouch( &CSpore::RocketTouch );
	}

	UTIL_MakeVectors( pev->angles );

	if( !m_bIsAI )
	{
		if( m_SporeType != SporeType::GRENADE )
		{
			pev->velocity = gpGlobals->v_forward * 800;
		}
		else {
			pev->gravity = 1;
		}
	}
	else
	{
		pev->gravity = 0.5;
		pev->friction = 0.7;
	}

	pev->dmg = GetDamage(gSkillData.sk_plr_spore);

	m_flIgniteTime = gpGlobals->time;

	pev->nextthink = gpGlobals->time + 0.01;

	auto sprite = CSprite::SpriteCreate("sprites/glow01.spr", pev->origin, false);

	m_hSprite = sprite;

	sprite->SetTransparency(kRenderTransAdd, 180, 180, 40, 100, kRenderFxDistort);
	sprite->SetScale(0.8);
	sprite->SetAttachment(edict(), 0);

	m_fRegisteredSound = false;

	m_flSoundDelay = gpGlobals->time;
}

void CSpore::BounceSound()
{
	//Nothing
}

void CSpore::IgniteThink()
{
	SetThink(nullptr);
	SetTouch(nullptr);

	if (m_hSprite)
	{
		UTIL_Remove(m_hSprite);
		m_hSprite = nullptr;
	}

	CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, NORMAL_GUN_VOLUME, 0.3);
	EMIT_SOUND(edict(), CHAN_WEAPON, "weapons/splauncher_impact.wav", VOL_NORM, ATTN_NORM);

	const auto vecDir = pev->velocity.Normalize();

	TraceResult tr;

	UTIL_TraceLine( 
		pev->origin, pev->origin + vecDir * ( m_SporeType == SporeType::GRENADE ? 64 : 32 ),
		dont_ignore_monsters, edict(), &tr );

	UTIL_DecalTrace( &tr, DECAL_SPR_SPLT1 + RANDOM_LONG( 0, 2 ) );

	UTIL_SpriteSpray(pev->origin, tr.vecPlaneNormal, m_iSpitSprite, 100, 40, 180);

	UTIL_DLight(pev->origin, 10, RGB(15, 220, 40), 5, 10);

	int spr = RANDOM_LONG(0, 1) ? m_iBlow : m_iBlowSmall;
	UTIL_Sprite(pev->origin, spr, 20, 128);

	UTIL_SpriteSpray(pev->origin, Vector(RANDOM_FLOAT(-1, 1), 1, RANDOM_FLOAT(-1, 1)), m_iTrail, 2, 20, 80);

	::RadiusDamage( pev->origin, pev, VARS( pev->owner ), pev->dmg, 200, CLASS_NONE, DMG_ALWAYSGIB | DMG_POISON);

	SetThink( &CSpore::SUB_Remove );
	pev->nextthink = gpGlobals->time + 0.1f; // give time for sound
	pev->solid = SOLID_NOT;
	pev->rendermode = kRenderTransTexture;
	pev->renderamt = 0;
}

void CSpore::FlyThink()
{
	const float flDelay = m_bIsAI ? 4.0 : 2.0;

	if( m_SporeType != SporeType::GRENADE || ( gpGlobals->time <= m_flIgniteTime + flDelay ) )
	{
		if (UTIL_IsValidTempEntOrigin(pev->origin)) {
			UTIL_SpriteSpray(pev->origin, pev->velocity.Normalize(), m_iTrail, 2, 20, 80);
		}
		else {
			if (RANDOM_LONG(0,1) == 0)
				UTIL_SpriteSpray(pev->origin, pev->velocity.Normalize(), m_iTrail, 1, 20, 80);
		}
	}
	else
	{
		SetThink( &CSpore::IgniteThink );
	}

	pev->nextthink = gpGlobals->time + 0.03;

	if (pev->movetype == MOVETYPE_FLY)
		ParametricInterpolation(0.03f);
}

void CSpore::GibThink()
{
	//Nothing
}

void CSpore::RocketTouch( CBaseEntity* pOther )
{
	if( pOther->pev->takedamage != DAMAGE_NO )
	{
		pOther->TakeDamage( pev, VARS( pev->owner ), gSkillData.sk_plr_spore, DMG_POISON);
	}

	IgniteThink();
}

void CSpore::MyBounceTouch( CBaseEntity* pOther )
{
	if( pOther->pev->takedamage == DAMAGE_NO )
	{
		if( pOther->edict() != pev->owner )
		{
			if( gpGlobals->time > m_flSoundDelay )
			{
				CSoundEnt::InsertSound( bits_SOUND_DANGER, pev->origin, static_cast<int>( pev->dmg / 0.4 ), 0.3 );

				m_flSoundDelay = gpGlobals->time + 1.0;
			}

			if ((pev->flags & FL_ONGROUND) != 0)
			{
				pev->velocity = pev->velocity * 0.5;
			}
			else
			{
				EMIT_SOUND_DYN( edict(), CHAN_VOICE, "weapons/splauncher_bounce.wav", 1.0f, ATTN_NORM, 0, PITCH_NORM );
			}
		}
	}
	else
	{
		pOther->TakeDamage( pev, VARS( pev->owner ), gSkillData.sk_plr_spore, DMG_POISON);

		IgniteThink();
	}
}

CSpore* CSpore::CreateSpore( 
	const Vector& vecOrigin, const Vector& vecAngles, CBaseEntity* pOwner,
	SporeType sporeType, bool bIsAI, bool bPuked )
{
	auto pSpore = GetClassPtr<CSpore>( nullptr );

	UTIL_SetOrigin( pSpore->pev, vecOrigin );

	pSpore->m_SporeType = sporeType;

	if( bIsAI )
	{
		pSpore->pev->velocity = vecAngles;

		pSpore->pev->angles = UTIL_VecToAngles( vecAngles );
	}
	else
	{
		pSpore->pev->angles = vecAngles;
	}

	pSpore->m_bIsAI = bIsAI;

	pSpore->m_bPuked = bPuked;

	pSpore->Spawn();

	pSpore->pev->owner = pOwner->edict();

	pSpore->pev->classname = MAKE_STRING( "spore" );

	return pSpore;
}
