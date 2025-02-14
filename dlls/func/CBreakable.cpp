#include "extdll.h"
#include "util.h"
#include "saverestore.h"
#include "CBreakable.h"
#include "decals.h"
#include "explode.h"
#include "CBaseMonster.h"
#include "monsters.h"

// =================== FUNC_Breakable ==============================================

// Just add more items to the bottom of this array and they will automagically be supported
// This is done instead of just a classname in the FGD so we can control which entities can
// be spawned, and still remain fairly flexible
const char* CBreakable::pSpawnObjects[] =
{
	NULL,				// 0
	"item_battery",		// 1
	"item_healthkit",	// 2
	"weapon_9mmhandgun",// 3
	"ammo_9mmclip",		// 4
	"weapon_9mmAR",		// 5
	"ammo_9mmAR",		// 6
	"ammo_ARgrenades",	// 7
	"weapon_shotgun",	// 8
	"ammo_buckshot",	// 9
	"weapon_crossbow",	// 10
	"ammo_crossbow",	// 11
	"weapon_357",		// 12
	"ammo_357",			// 13
	"weapon_rpg",		// 14
	"ammo_rpgclip",		// 15
	"ammo_gaussclip",	// 16
	"weapon_handgrenade",// 17
	"weapon_tripmine",	// 18
	"weapon_satchel",	// 19
	"weapon_snark",		// 20
	"weapon_hornetgun",	// 21
};

void CBreakable::KeyValue(KeyValueData* pkvd)
{
	// UNDONE_WC: explicitly ignoring these fields, but they shouldn't be in the map file!
	if (FStrEq(pkvd->szKeyName, "deadmodel"))
	{
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "shards"))
	{
		//			m_iShards = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "spawnobject"))
	{
		int object = atoi(pkvd->szValue);
		if (object > 0 && object < (int)ARRAYSIZE(pSpawnObjects))
			m_iszSpawnObject = MAKE_STRING(pSpawnObjects[object]);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "lip")) {
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue(pkvd);
}


//
// func_breakable - bmodel that breaks into pieces after taking damage
//
LINK_ENTITY_TO_CLASS(func_breakable, CBreakable)
TYPEDESCRIPTION CBreakable::m_SaveData[] =
{
	DEFINE_FIELD(CBreakable, m_breakMaterial, FIELD_INTEGER),

	// Don't need to save/restore these because we precache after restore
	//	DEFINE_FIELD( CBreakable, m_idShard, FIELD_INTEGER ),

		DEFINE_FIELD(CBreakable, m_angle, FIELD_FLOAT),
		DEFINE_FIELD(CBreakable, m_breakModel, FIELD_STRING),
		DEFINE_FIELD(CBreakable, m_iszSpawnObject, FIELD_STRING),

		// Explosion magnitude is stored in pev->impulse
};

IMPLEMENT_SAVERESTORE(CBreakable, CBaseEntity)

void CBreakable::Spawn(void)
{
	// translate func_breakable flags into common breakable flags
	m_breakFlags |= FL_BREAK_IS_BREAKABLE | FL_BREAK_CAN_TRIGGER;
	if (pev->spawnflags & SF_BREAK_TRIGGER_ONLY)
		m_breakFlags |= FL_BREAK_TRIGGER_ONLY;
	if (pev->spawnflags & SF_BREAK_INSTANT)
		m_breakFlags |= FL_BREAK_INSTANT;
	if (pev->spawnflags & SF_BREAK_EXPLOSIVES_ONLY)
		m_breakFlags |= FL_BREAK_EXPLOSIVES_ONLY;
	if (pev->spawnflags & SF_BREAK_REPAIRABLE)
		m_breakFlags |= FL_BREAK_REPAIRABLE;

	CBaseEntity::Spawn();
	Precache();

	if (FBitSet(pev->spawnflags, SF_BREAK_TRIGGER_ONLY))
		pev->takedamage = DAMAGE_NO;
	else
		pev->takedamage = DAMAGE_YES;

	pev->solid = SOLID_BSP;
	pev->movetype = MOVETYPE_PUSH;
	m_angle = pev->angles.y;
	pev->angles.y = 0;
	pev->max_health = pev->health;

	// HACK:  matGlass can receive decals, we need the client to know about this
	//  so use class to store the material flag
	if (m_breakMaterial == matGlass)
	{
		pev->playerclass = 1;
	}

	SET_MODEL(ENT(pev), STRING(pev->model));//set size and link into world.

	SetTouch(&CBreakable::BreakTouch);
	if (FBitSet(pev->spawnflags, SF_BREAK_TRIGGER_ONLY))		// Only break on trigger
		SetTouch(NULL);
	else {
		pev->flags |= FL_POSSIBLE_TARGET;
	}

	// Flag unbreakable glass as "worldbrush" so it will block ALL tracelines
	if (!IsBreakable() && pev->rendermode != kRenderNormal)
		pev->flags |= FL_WORLDBRUSH;
}

void CBreakable::Precache(void)
{
	// Precache the spawn item's data
	if (m_iszSpawnObject)
		UTIL_PrecacheOther((char*)STRING(m_iszSpawnObject));
}

// play shard sound when func_breakable takes damage.
// the more damage, the louder the shard sound.

void CBreakable::BreakTouch(CBaseEntity* pOther)
{
	float flDamage;
	entvars_t* pevToucher = pOther->pev;

	// only players can break these right now
	if (!pOther->IsPlayer() || !IsBreakable())
	{
		return;
	}

	if (FBitSet(pev->spawnflags, SF_BREAK_TOUCH))
	{// can be broken when run into 
		flDamage = pevToucher->velocity.Length() * 0.01;

		if (flDamage >= pev->health)
		{
			SetTouch(NULL);
			TakeDamage(pevToucher, pevToucher, flDamage, DMG_CRUSH);

			// do a little damage to player if we broke glass or computer
			pOther->TakeDamage(pev, pev, flDamage / 4, DMG_SLASH);
		}
	}

	if (FBitSet(pev->spawnflags, SF_BREAK_PRESSURE) && pevToucher->absmin.z >= pev->maxs.z - 2)
	{// can be broken when stood upon

		// play creaking sound here.
		BreakableDamageSound();

		SetTouch(NULL);
		BreakableDie(pOther);
	}
}

// Break when triggered
void CBreakable::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	BreakableUse(pActivator, pCaller, useType, value);
}


void CBreakable::BreakableDie(CBaseEntity* pActivator)
{
	CBaseEntity::BreakableDie(pActivator);

<<<<<<< HEAD
	// random spark if this is a 'computer' object
	if (RANDOM_LONG(0, 1))
	{
		switch (m_Material)
		{
		case matComputer:
		{
			UTIL_Sparks(ptr->vecEndPos);

			float flVolume = RANDOM_FLOAT(0.7, 1.0);//random volume range
			switch (RANDOM_LONG(0, 1))
			{
			case 0: EMIT_SOUND(ENT(pev), CHAN_VOICE, "buttons/spark5.wav", flVolume, ATTN_NORM);	break;
			case 1: EMIT_SOUND(ENT(pev), CHAN_VOICE, "buttons/spark6.wav", flVolume, ATTN_NORM);	break;
			}
		}
		break;

		case matUnbreakableGlass:
			UTIL_Ricochet(ptr->vecEndPos, RANDOM_FLOAT(0.5, 1.5));
			break;
		default:
			break;
		}
	}

	CBaseDelay::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}

//=========================================================
// Special takedamage for func_breakable. Allows us to make
// exceptions that are breakable-specific
// bitsDamageType indicates the type of damage sustained ie: DMG_CRUSH
//=========================================================
int CBreakable::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	if (flDamage > 0 && ShouldBlockFriendlyFire(pevAttacker)) {
		return 0;
	}

	Vector	vecTemp;

	if ((pev->spawnflags & SF_BREAK_EXPLOSIVES_ONLY) && !(bitsDamageType & (DMG_BLAST | DMG_MORTAR))) {
		return 0;
	}

	// if Attacker == Inflictor, the attack was a melee or other instant-hit attack.
	// (that is, no actual entity projectile was involved in the attack so use the shooter's origin). 
	if (pevAttacker == pevInflictor)
	{
		vecTemp = pevInflictor->origin - (pev->absmin + (pev->size * 0.5));

		if (flDamage > 0 &&
			FBitSet(pevAttacker->flags, FL_CLIENT) &&
			FBitSet(pev->spawnflags, SF_BREAK_INSTANT) && (bitsDamageType & DMG_CLUB)) {
			// if a client hit the breakable with a crowbar, and breakable is crowbar-sensitive, break it now.

			bool isWrench = FStrEq(getActiveWeapon(pevAttacker), "weapon_pipewrench");
			bool requiresWrench = m_instantBreakWeapon == BREAK_INSTANT_WRENCH;

			if (requiresWrench == isWrench) {
				flDamage = pev->health;
			}
		}
	}
	else
		// an actual missile was involved.
	{
		vecTemp = pevInflictor->origin - (pev->absmin + (pev->size * 0.5));
	}

	if (!IsBreakable())
		return 0;

	// Breakables take double damage from the crowbar
	if (bitsDamageType & DMG_CLUB)
		flDamage *= 2;

	// Boxes / glass / etc. don't take much poison damage, just the impact of the dart - consider that 10%
	if (bitsDamageType & DMG_POISON)
		flDamage *= 0.1;

	// this global is still used for glass and other non-monster killables, along with decals.
	g_vecAttackDir = vecTemp.Normalize();

	// give points if this breakable triggers something, unless it's supposed to be invincible
	// and the mapper didn't know how to check the "only trigger" flag. Giving half points to compensate
	// for many breakables being useless and taking no skill to destroy
	// TODO: determine if a breakable is useless. Hard problem. Opening a shortcut is not useless.
	// Need the path finder and map solver done first.
	if ((pev->target || m_iszKillTarget) && pev->max_health <= 10000)
		GiveScorePoints(pevAttacker, flDamage*0.5f);

	// do the damage
	pev->health = V_min(pev->max_health, pev->health - flDamage);

	if (pev->health <= 0)
	{
		//Killed(pevAttacker, GIB_NORMAL);
		Killed(pevAttacker, GIB_NEVER);
		m_hActivator = Instance(pevAttacker);
		Die();
		return 0;
	}

	// Make a shard noise each time func breakable is hit.
	// Don't play shard noise if cbreakable actually died.

	if (flDamage > 0)
		DamageSound();

	return 1;
}


void CBreakable::Die()
{
	Vector vecSpot;// shard origin
	Vector vecVelocity;// shard velocity
	char cFlag = 0;
	int pitch;
	float fvol;

	pitch = 95 + RANDOM_LONG(0, 29);

	if (pitch > 97 && pitch < 103)
		pitch = 100;

	// The more negative pev->health, the louder
	// the sound should be.

	fvol = RANDOM_FLOAT(0.85, 1.0) + (fabs(pev->health) / 100.0);

	if (fvol > 1.0)
		fvol = 1.0;


	switch (m_Material)
	{
	case matGlass:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pBustSoundsGlass), fvol, ATTN_NORM, 0, pitch);
		cFlag = BREAK_GLASS;
		break;

	case matWood:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pBustSoundsWood), fvol, ATTN_NORM, 0, pitch);
		cFlag = BREAK_WOOD;
		break;

	case matComputer:
	case matMetal:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pBustSoundsMetal), fvol, ATTN_NORM, 0, pitch);
		cFlag = BREAK_METAL;
		break;

	case matFlesh:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pBustSoundsFlesh), fvol, ATTN_NORM, 0, pitch);
		cFlag = BREAK_FLESH;
		break;

	case matRocks:
	case matCinderBlock:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pBustSoundsConcrete), fvol, ATTN_NORM, 0, pitch);
		cFlag = BREAK_CONCRETE;
		break;

	case matCeilingTile:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "debris/bustceiling.wav", fvol, ATTN_NORM, 0, pitch);
		break;

	case matNone:
	default:
		break;
	}


	if (m_Explosion == expDirected)
		vecVelocity = g_vecAttackDir * 200;
	else
	{
		vecVelocity.x = 0;
		vecVelocity.y = 0;
		vecVelocity.z = 0;
	}

	vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;
	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, vecSpot);
	WRITE_BYTE(TE_BREAKMODEL);

	// position
	WRITE_COORD(vecSpot.x);
	WRITE_COORD(vecSpot.y);
	WRITE_COORD(vecSpot.z);

	// size
	WRITE_COORD(pev->size.x);
	WRITE_COORD(pev->size.y);
	WRITE_COORD(pev->size.z);

	// velocity
	WRITE_COORD(vecVelocity.x);
	WRITE_COORD(vecVelocity.y);
	WRITE_COORD(vecVelocity.z);

	// randomization
	WRITE_BYTE(10);

	// Model
	WRITE_SHORT(m_idShard);	//model id#

	// # of shards
	WRITE_BYTE(0);	// let client decide

	// duration
	WRITE_BYTE(25);// 2.5 seconds

	// flags
	WRITE_BYTE(cFlag);
	MESSAGE_END();

	float size = pev->size.x;
	if (size < pev->size.y)
		size = pev->size.y;
	if (size < pev->size.z)
		size = pev->size.z;

	// !!! HACK  This should work!
	// Build a box above the entity that looks like an 8 pixel high sheet
	Vector mins = pev->absmin;
	Vector maxs = pev->absmax;
	mins.z = pev->absmax.z;
	maxs.z += 8;

	// BUGBUG -- can only find 256 entities on a breakable -- should be enough
	CBaseEntity* pList[256];
	int count = UTIL_EntitiesInBox(pList, 256, mins, maxs, FL_ONGROUND, false);
	if (count)
	{
		for (int i = 0; i < count; i++)
		{
			ClearBits(pList[i]->pev->flags, FL_ONGROUND);
			pList[i]->pev->groundentity = NULL;
		}
	}

	// Don't fire something that could fire myself
	pev->targetname = 0;

	pev->solid = SOLID_NOT;
	// Fire targets on break
	SUB_UseTargets(m_hActivator.GetEntity(), USE_TOGGLE, 0);

	SetThink(&CBreakable::SUB_Remove);
	pev->nextthink = pev->ltime + 0.1;
=======
	// Fire targets on break (CBaseEntity doesn't handle trigger delay, so a separate target field is used)
	SUB_UseTargets(pActivator, USE_TOGGLE, 0);
	
>>>>>>> 6f00d2f7fb6be8070f0e941e1d3bd5bba5d74494
	if (m_iszSpawnObject)
		CBaseEntity::Create((char*)STRING(m_iszSpawnObject), VecBModelOrigin(pev), pev->angles, true, edict());
}
