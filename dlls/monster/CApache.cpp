/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
#include "extdll.h"
#include "util.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "effects.h"
#include "weapon/CGrenade.h"
#include "skill.h"
#include "te_effects.h"

#define SF_WAITFORTRIGGER	(0x04 | 0x40) // UNDONE: Fix!
#define SF_NOWRECKAGE		0x08

class CApache : public CBaseMonster
{
	virtual int	ObjectCaps(void) { return CBaseMonster::ObjectCaps() & ~FCAP_IMPULSE_USE; }
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void Spawn( void );
	void Precache( void );
	int  Classify( void ) { return CBaseMonster::Classify(CLASS_HUMAN_MILITARY); }
	BOOL IsMachine() { return 1; } // ignore classification overrides
	const char* DisplayName() { return m_displayName ? CBaseMonster::DisplayName() : "Apache"; }
	int  BloodColor( void ) { return DONT_BLEED; }
	void Killed( entvars_t *pevAttacker, int iGib );
	void GibMonster( void );

	// when crashing, show an explosion kill icon
	const char* GetDeathNoticeWeapon() { return IsAlive() ? "weapon_9mmAR" : "grenade"; }

	void SetObjectCollisionBox( void )
	{
		pev->absmin = pev->origin + Vector( -300, -300, -172);
		pev->absmax = pev->origin + Vector(300, 300, 8);
	}

	void EXPORT HuntThink( void );
	void EXPORT FlyTouch( CBaseEntity *pOther );
	void EXPORT CrashTouch( CBaseEntity *pOther );
	void EXPORT DyingThink( void );
	void EXPORT StartupUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT NullThink( void );

	void ShowDamage( void );
	void Flight( void );
	void FireRocket( void );
	BOOL FireGun( void );
	
	int  TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

	int m_iRockets;
	float m_flForce;
	float m_flNextRocket;

	Vector m_vecTarget;
	Vector m_posTarget;

	Vector m_vecDesired;
	Vector m_posDesired;

	Vector m_vecGoal;

	Vector m_angGun;
	float m_flLastSeen;
	float m_flPrevSeen;

	int m_iSoundState; // don't save this

	int m_iSpriteTexture;
	int m_iExplode;
	int m_iBodyGibs;
	int m_iGlassHit;
	int m_iEngineHit;
	int m_iGlassGibs;
	int m_iEngineGibs;

	float m_flGoalSpeed;

	int m_iDoSmokePuff;
};

LINK_ENTITY_TO_CLASS( monster_apache, CApache )
LINK_ENTITY_TO_CLASS( monster_blkop_apache, CApache )

TYPEDESCRIPTION	CApache::m_SaveData[] = 
{
	DEFINE_FIELD( CApache, m_iRockets, FIELD_INTEGER ),
	DEFINE_FIELD( CApache, m_flForce, FIELD_FLOAT ),
	DEFINE_FIELD( CApache, m_flNextRocket, FIELD_TIME ),
	DEFINE_FIELD( CApache, m_vecTarget, FIELD_VECTOR ),
	DEFINE_FIELD( CApache, m_posTarget, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CApache, m_vecDesired, FIELD_VECTOR ),
	DEFINE_FIELD( CApache, m_posDesired, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CApache, m_vecGoal, FIELD_VECTOR ),
	DEFINE_FIELD( CApache, m_angGun, FIELD_VECTOR ),
	DEFINE_FIELD( CApache, m_flLastSeen, FIELD_TIME ),
	DEFINE_FIELD( CApache, m_flPrevSeen, FIELD_TIME ),
//	DEFINE_FIELD( CApache, m_iSoundState, FIELD_INTEGER ),		// Don't save, precached
//	DEFINE_FIELD( CApache, m_iSpriteTexture, FIELD_INTEGER ),
//	DEFINE_FIELD( CApache, m_iExplode, FIELD_INTEGER ),
//	DEFINE_FIELD( CApache, m_iBodyGibs, FIELD_INTEGER ),
	DEFINE_FIELD( CApache, m_flGoalSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CApache, m_iDoSmokePuff, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CApache, CBaseMonster )


void CApache :: Spawn( void )
{
	Precache( );
	// motor
	pev->solid = SOLID_BBOX;

	// FLY movetype but with client interpolation
	pev->movetype = MOVETYPE_BOUNCE;
	pev->gravity = FLT_MIN;
	pev->friction = 1.0f;

	InitModel();
	SetSize(Vector( -32, -32, -64 ), Vector( 32, 32, 0 ) );
	UTIL_SetOrigin( pev, pev->origin );

	pev->flags |= FL_MONSTER;
	pev->takedamage		= DAMAGE_AIM;
	SetHealth();

	m_flFieldOfView = -0.707; // 270 degrees

	pev->sequence = 0;
	ResetSequenceInfo( );
	pev->frame = RANDOM_LONG(0, 0xFF);

	InitBoneControllers();

	if (pev->spawnflags & SF_WAITFORTRIGGER)
	{
		SetThink( &CApache::NullThink );
		SetUse( &CApache::StartupUse );
	}
	else
	{
		SetThink( &CApache::HuntThink );
		SetTouch( &CApache::FlyTouch );
	}
	pev->nextthink = gpGlobals->time + 1.0;

	m_iRockets = 10;
}


void CApache::Precache( void )
{
	CBaseMonster::Precache();
	m_defaultModel = FClassnameIs(pev, "monster_blkop_apache") ? "models/blkop_apache.mdl" : "models/apache.mdl";

	PRECACHE_MODEL(GetModel());

	//PRECACHE_SOUND("apache/ap_rotor1.wav");
	PRECACHE_SOUND("apache/ap_rotor2.wav");
	//PRECACHE_SOUND("apache/ap_rotor3.wav");
	//PRECACHE_SOUND("apache/ap_whine1.wav");

	PRECACHE_SOUND("weapons/mortarhit.wav");

	m_iSpriteTexture = PRECACHE_MODEL( "sprites/white.spr" );

	PRECACHE_SOUND("turret/tu_fire1.wav");

	PRECACHE_MODEL("sprites/lgtning.spr");

	m_iExplode	= PRECACHE_MODEL( "sprites/fexplo.spr" );
	m_iBodyGibs = PRECACHE_MODEL( "models/metalplategibs_green.mdl" );

	m_iGlassHit = PRECACHE_MODEL("sprites/xfire2.spr");
	m_iEngineHit = PRECACHE_MODEL("sprites/muz1.spr");
	m_iGlassGibs = PRECACHE_MODEL("models/chromegibs.mdl");
	m_iEngineGibs = PRECACHE_MODEL("models/mechgibs.mdl");

	UTIL_PrecacheOther( "hvr_rocket" );
}



void CApache::NullThink( void )
{
	StudioFrameAdvance( );
	FCheckAITrigger();
	UpdateShockEffect();
	pev->nextthink = gpGlobals->time + 0.5;
}


void CApache::StartupUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetThink( &CApache::HuntThink );
	SetTouch( &CApache::FlyTouch );
	pev->nextthink = gpGlobals->time + 0.1;
	SetUse( NULL );
}

void CApache :: Killed( entvars_t *pevAttacker, int iGib )
{
	CBaseMonster::Killed(pevAttacker, GIB_NEVER); // for monstermaker death notice + death trigger

	pev->movetype = MOVETYPE_TOSS;
	pev->gravity = 0.3;

	STOP_SOUND(ENT(pev), CHAN_ITEM, "apache/ap_rotor2.wav");

	UTIL_SetSize( pev, Vector( -32, -32, -64), Vector( 32, 32, 0) );
	SetThink( &CApache::DyingThink );
	SetTouch( &CApache::CrashTouch );
	pev->nextthink = gpGlobals->time + 0.1;
	pev->health = 0;
	pev->takedamage = DAMAGE_NO;

	if (pev->spawnflags & SF_NOWRECKAGE)
	{
		m_flNextRocket = gpGlobals->time + 4.0;
	}
	else
	{
		m_flNextRocket = gpGlobals->time + 15.0;
	}
}

void CApache :: DyingThink( void )
{
	UpdateShockEffect();
	StudioFrameAdvance( );
	pev->nextthink = gpGlobals->time + 0.1;

	pev->avelocity = pev->avelocity * 1.02;

	// still falling?
	if (m_flNextRocket > gpGlobals->time )
	{
		// random explosions
		Vector ori = pev->origin + Vector(RANDOM_FLOAT(-150, 150), RANDOM_FLOAT(-150, 150), RANDOM_FLOAT(-150, -50));
		UTIL_Explosion(ori, g_sModelIndexFireball, RANDOM_LONG(0, 29) + 30, 12, TE_EXPLFLAG_NONE);

		// lots of smoke
		UTIL_Smoke(pev->origin, g_sModelIndexSmoke, 100, 10);

		Vector vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;
		int gibCount = UTIL_IsValidTempEntOrigin(vecSpot) ? 4 : 1;
		UTIL_BreakModel(vecSpot, Vector(400, 400, 132), pev->velocity, 50,
			m_iBodyGibs, gibCount, 30, BREAK_METAL);

		// don't stop it we touch a entity
		pev->flags &= ~FL_ONGROUND;
		pev->nextthink = gpGlobals->time + 0.2;
		return;
	}
	else
	{
		Vector vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;

		// fireball
		UTIL_Sprite(vecSpot, m_iExplode, 120, 255);
		
		// big smoke
		UTIL_Smoke(vecSpot, g_sModelIndexSmoke, 250, 5);

		// blast circle
		UTIL_BeamCylinder(pev->origin, 2000, m_iSpriteTexture, 0, 0, 4, 32, 0, RGBA(255, 255, 192, 128), 0);

		EMIT_SOUND(ENT(pev), CHAN_STATIC, "weapons/mortarhit.wav", 1.0, 0.3);

		RadiusDamage( pev->origin, pev, pev, 300, CLASS_NONE, DMG_BLAST );

		if (/*!(pev->spawnflags & SF_NOWRECKAGE) && */(pev->flags & FL_ONGROUND))
		{
			CBaseEntity *pWreckage = Create( "cycler_wreckage", pev->origin, pev->angles );
			// SET_MODEL( ENT(pWreckage->pev), STRING(pev->model) );
			UTIL_SetSize( pWreckage->pev, Vector( -200, -200, -128 ), Vector( 200, 200, -32 ) );
			pWreckage->pev->frame = pev->frame;
			pWreckage->pev->sequence = pev->sequence;
			pWreckage->pev->framerate = 0;
			pWreckage->pev->dmgtime = gpGlobals->time + 5;
		}

		// gibs
		vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;
		UTIL_BreakModel(vecSpot, Vector(400, 400, 128), Vector(0,0,200), 30,
			m_iBodyGibs, 200, 200, BREAK_METAL);

		SetThink( &CApache::SUB_Remove );
		pev->nextthink = gpGlobals->time + 0.1;
	}
}


void CApache::FlyTouch( CBaseEntity *pOther )
{
	// bounce if we hit something solid
	if ( pOther->pev->solid == SOLID_BSP) 
	{
		TraceResult tr = UTIL_GetGlobalTrace( );

		// UNDONE, do a real bounce
		pev->velocity = pev->velocity + tr.vecPlaneNormal * (pev->velocity.Length() + 200);
	}
}


void CApache::CrashTouch( CBaseEntity *pOther )
{
	// only crash if we hit something solid
	if ( pOther->pev->solid == SOLID_BSP) 
	{
		SetTouch( NULL );
		m_flNextRocket = gpGlobals->time;
		pev->nextthink = gpGlobals->time;
	}
}



void CApache :: GibMonster( void )
{
	// EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "common/bodysplat.wav", 0.75, ATTN_NORM, 0, 200);		
}


void CApache :: HuntThink( void )
{
	StudioFrameAdvance( );
	pev->nextthink = gpGlobals->time + 0.1;

	ShowDamage( );
	FCheckAITrigger();
	UpdateShockEffect();

	if ( !m_hGoalEnt && !FStringNull(pev->target) )// this monster has a target
	{
		m_hGoalEnt = UTIL_FindEntityByTargetname( NULL, STRING( pev->target ) );
		if (m_hGoalEnt)
		{
			m_posDesired = m_hGoalEnt->pev->origin;
			UTIL_MakeAimVectors(m_hGoalEnt->pev->angles );
			m_vecGoal = gpGlobals->v_forward;
		}
	}

	// if (m_hEnemy == NULL)
	{
		Look( 4092 );
		m_hEnemy = BestVisibleEnemy( );

		//If i have an enemy i'm in combat, otherwise i'm patrolling.
		if (m_hEnemy != NULL)
		{
			m_MonsterState = MONSTERSTATE_COMBAT;
		}
		else
		{
			m_MonsterState = MONSTERSTATE_ALERT;
		}
	}

	Listen(); // Listen for sounds so AI triggers work.

	// generic speed up
	if (m_flGoalSpeed < 800)
		m_flGoalSpeed += 5;

	if (m_hEnemy != NULL)
	{
		// ALERT( at_console, "%s\n", STRING( m_hEnemy->pev->classname ) );
		if (FVisible( m_hEnemy ))
		{
			if (m_flLastSeen < gpGlobals->time - 5)
				m_flPrevSeen = gpGlobals->time;
			m_flLastSeen = gpGlobals->time;
			m_posTarget = m_hEnemy->Center( );
		}
		else
		{
			m_hEnemy = NULL;
		}
	}

	m_vecTarget = (m_posTarget - pev->origin).Normalize();

	float flLength = (pev->origin - m_posDesired).Length();

	if (m_hGoalEnt)
	{
		// ALERT( at_console, "%.0f\n", flLength );

		if (flLength < 128)
		{
			m_hGoalEnt = UTIL_FindEntityByTargetname( NULL, STRING(m_hGoalEnt->pev->target ) );
			if (m_hGoalEnt)
			{
				m_posDesired = m_hGoalEnt->pev->origin;
				UTIL_MakeAimVectors(m_hGoalEnt->pev->angles );
				m_vecGoal = gpGlobals->v_forward;
				flLength = (pev->origin - m_posDesired).Length();
			}
		}
	}
	else
	{
		m_posDesired = pev->origin;
	}

	if (flLength > 250) // 500
	{
		// float flLength2 = (m_posTarget - pev->origin).Length() * (1.5 - DotProduct((m_posTarget - pev->origin).Normalize(), pev->velocity.Normalize() ));
		// if (flLength2 < flLength)
		if (m_flLastSeen + 90 > gpGlobals->time && DotProduct( (m_posTarget - pev->origin).Normalize(), (m_posDesired - pev->origin).Normalize( )) > 0.25)
		{
			m_vecDesired = (m_posTarget - pev->origin).Normalize( );
		}
		else
		{
			m_vecDesired = (m_posDesired - pev->origin).Normalize( );
		}
	}
	else
	{
		m_vecDesired = m_vecGoal;
	}

	Flight( );

	// ALERT( at_console, "%.0f %.0f %.0f\n", gpGlobals->time, m_flLastSeen, m_flPrevSeen );
	if ((m_flLastSeen + 1 > gpGlobals->time) && (m_flPrevSeen + 2 < gpGlobals->time))
	{
		if (FireGun( ))
		{
			// slow down if we're fireing
			if (m_flGoalSpeed > 400)
				m_flGoalSpeed = 400;
		}

		// don't fire rockets and gun on easy mode
		if (gSkillData.sk_apache_rockets)
			m_flNextRocket = gpGlobals->time + 10.0;
	}

	UTIL_MakeAimVectors( pev->angles );
	Vector vecEst = (gpGlobals->v_forward * 800 + pev->velocity).Normalize( );
	// ALERT( at_console, "%d %d %d %4.2f\n", pev->angles.x < 0, DotProduct( pev->velocity, gpGlobals->v_forward ) > -100, m_flNextRocket < gpGlobals->time, DotProduct( m_vecTarget, vecEst ) );

	if ((m_iRockets % 2) == 1)
	{
		FireRocket( );
		m_flNextRocket = gpGlobals->time + 0.5;
		if (m_iRockets <= 0)
		{
			m_flNextRocket = gpGlobals->time + 10;
			m_iRockets = 10;
		}
	}
	else if (pev->angles.x < 0 && DotProduct( pev->velocity, gpGlobals->v_forward ) > -100 && m_flNextRocket < gpGlobals->time)
	{
		if (m_flLastSeen + 60 > gpGlobals->time)
		{
			if (m_hEnemy != NULL)
			{
				// make sure it's a good shot
				if (DotProduct( m_vecTarget, vecEst) > .965)
				{
					TraceResult tr;
					
					UTIL_TraceLine( pev->origin, pev->origin + vecEst * 4096, ignore_monsters, edict(), &tr );
					if ((tr.vecEndPos - m_posTarget).Length() < 512)
						FireRocket( );
				}
			}
			else
			{
				TraceResult tr;
				
				UTIL_TraceLine( pev->origin, pev->origin + vecEst * 4096, dont_ignore_monsters, edict(), &tr );
				// just fire when close
				if ((tr.vecEndPos - m_posTarget).Length() < 512)
					FireRocket( );
			}
		}
	}
}


void CApache :: Flight( void )
{
	// tilt model 5 degrees
	Vector vecAdj = Vector( 5.0, 0, 0 );

	// estimate where I'll be facing in one seconds
	UTIL_MakeAimVectors( pev->angles + pev->avelocity * 2 + vecAdj);
	// Vector vecEst1 = pev->origin + pev->velocity + gpGlobals->v_up * m_flForce - Vector( 0, 0, 384 );
	// float flSide = DotProduct( m_posDesired - vecEst1, gpGlobals->v_right );
	
	float flSide = DotProduct( m_vecDesired, gpGlobals->v_right );

	if (flSide < 0)
	{
		if (pev->avelocity.y < 60)
		{
			pev->avelocity.y += 8; // 9 * (3.0/2.0);
		}
	}
	else
	{
		if (pev->avelocity.y > -60)
		{
			pev->avelocity.y -= 8; // 9 * (3.0/2.0);
		}
	}
	pev->avelocity.y *= 0.98;

	// estimate where I'll be in two seconds
	UTIL_MakeAimVectors( pev->angles + pev->avelocity * 1 + vecAdj);
	Vector vecEst = pev->origin + pev->velocity * 2.0 + gpGlobals->v_up * m_flForce * 20 - Vector( 0, 0, 384 * 2 );

	// add immediate force
	UTIL_MakeAimVectors( pev->angles + vecAdj);
	pev->velocity.x += gpGlobals->v_up.x * m_flForce;
	pev->velocity.y += gpGlobals->v_up.y * m_flForce;
	pev->velocity.z += gpGlobals->v_up.z * m_flForce;
	// add gravity
	pev->velocity.z -= 38.4; // 32ft/sec


	float flSpeed = pev->velocity.Length();
	float flDir = DotProduct( Vector( gpGlobals->v_forward.x, gpGlobals->v_forward.y, 0 ), Vector( pev->velocity.x, pev->velocity.y, 0 ) );
	if (flDir < 0)
		flSpeed = -flSpeed;

	float flDist = DotProduct( m_posDesired - vecEst, gpGlobals->v_forward );

	// float flSlip = DotProduct( pev->velocity, gpGlobals->v_right );
	float flSlip = -DotProduct( m_posDesired - vecEst, gpGlobals->v_right );

	// fly sideways
	if (flSlip > 0)
	{
		if (pev->angles.z > -30 && pev->avelocity.z > -15)
			pev->avelocity.z -= 4;
		else
			pev->avelocity.z += 2;
	}
	else
	{

		if (pev->angles.z < 30 && pev->avelocity.z < 15)
			pev->avelocity.z += 4;
		else
			pev->avelocity.z -= 2;
	}

	// sideways drag
	pev->velocity.x = pev->velocity.x * (1.0 - fabs( gpGlobals->v_right.x ) * 0.05);
	pev->velocity.y = pev->velocity.y * (1.0 - fabs( gpGlobals->v_right.y ) * 0.05);
	pev->velocity.z = pev->velocity.z * (1.0 - fabs( gpGlobals->v_right.z ) * 0.05);

	// general drag
	pev->velocity = pev->velocity * 0.995;
	
	// apply power to stay correct height
	if (m_flForce < 80 && vecEst.z < m_posDesired.z) 
	{
		m_flForce += 12;
	}
	else if (m_flForce > 30)
	{
		if (vecEst.z > m_posDesired.z) 
			m_flForce -= 8;
	}

	// pitch forward or back to get to target
	if (flDist > 0 && flSpeed < m_flGoalSpeed /* && flSpeed < flDist */ && pev->angles.x + pev->avelocity.x > -40)
	{
		// ALERT( at_console, "F " );
		// lean forward
		pev->avelocity.x -= 12.0;
	}
	else if (flDist < 0 && flSpeed > -50 && pev->angles.x + pev->avelocity.x  < 20)
	{
		// ALERT( at_console, "B " );
		// lean backward
		pev->avelocity.x += 12.0;
	}
	else if (pev->angles.x + pev->avelocity.x > 0)
	{
		// ALERT( at_console, "f " );
		pev->avelocity.x -= 4.0;
	}
	else if (pev->angles.x + pev->avelocity.x < 0)
	{
		// ALERT( at_console, "b " );
		pev->avelocity.x += 4.0;
	}

	// ALERT( at_console, "%.0f %.0f : %.0f %.0f : %.0f %.0f : %.0f\n", pev->origin.x, pev->velocity.x, flDist, flSpeed, pev->angles.x, pev->avelocity.x, m_flForce ); 
	// ALERT( at_console, "%.0f %.0f : %.0f %0.f : %.0f\n", pev->origin.z, pev->velocity.z, vecEst.z, m_posDesired.z, m_flForce ); 

	static int lastPitch[33];

	// make rotor, engine sounds
	if (m_iSoundState == 0)
	{
		StartSound(edict(), CHAN_ITEM, "apache/ap_rotor2.wav", 1.0f, 0.3f, 0, 110, g_vecZero, 0xffffffff);
		//EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, "apache/ap_rotor2.wav", 1.0, 0.3, 0, 110 );
		// EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, "apache/ap_whine1.wav", 0.5, 0.2, 0, 110 );

		memset(lastPitch, 0, sizeof(int) * 33);
		m_iSoundState = SND_CHANGE_PITCH; // hack for going through level transitions
	}
	else
	{
		for (int i = 1; i < gpGlobals->maxClients; i++) {
			CBaseEntity* pPlayer = (CBaseEntity*)UTIL_PlayerByIndex(i);

			if (!pPlayer) {
				continue;
			}

			float dot = DotProduct(pev->velocity - pPlayer->pev->velocity, (pPlayer->pev->origin - pev->origin).Normalize());

			int pitch = (int)(100 + dot / 50.0);

			if (pitch > 250)
				pitch = 250;
			if (pitch < 50)
				pitch = 50;

			pitch = (pitch / 2) * 2; // reduce the amount of network messages

			if (pitch == 100)
				pitch = 101; // SND_CHANGE_PITCH will not work for 100 pitch (random sounds will play)

			/*
			float flVol = (m_flForce / 100.0) + .1;
			if (flVol > 1.0)
				flVol = 1.0;
				*/

			if (pitch == lastPitch[pPlayer->entindex()]) {
				continue;
			}

			StartSound(edict(), CHAN_ITEM, "apache/ap_rotor2.wav", 1.0f, 0.3f, SND_CHANGE_PITCH,
				pitch, g_vecZero, PLRBIT(pPlayer->edict()));
			lastPitch[pPlayer->entindex()] = pitch;
		}
		// EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, "apache/ap_whine1.wav", flVol, 0.2, SND_CHANGE_PITCH | SND_CHANGE_VOL, pitch);
	
		// ALERT( at_console, "%.0f %.2f\n", pitch, flVol );
	}
}


void CApache :: FireRocket( void )
{
	static float side = 1.0;

	if (m_iRockets <= 0)
		return;

	UTIL_MakeAimVectors( pev->angles );
	Vector vecSrc = pev->origin + 1.5 * (gpGlobals->v_forward * 21 + gpGlobals->v_right * 70 * side + gpGlobals->v_up * -79);

	switch( m_iRockets % 5)
	{
	case 0:	vecSrc = vecSrc + gpGlobals->v_right * 10; break;
	case 1: vecSrc = vecSrc - gpGlobals->v_right * 10; break;
	case 2: vecSrc = vecSrc + gpGlobals->v_up * 10; break;
	case 3: vecSrc = vecSrc - gpGlobals->v_up * 10; break;
	case 4: break;
	}

	UTIL_Smoke(vecSrc, g_sModelIndexSmoke, 20, 12);

	CBaseEntity *pRocket = CBaseEntity::Create( "hvr_rocket", vecSrc, pev->angles, true, edict() );
	if (pRocket)
		pRocket->pev->velocity = pev->velocity + gpGlobals->v_forward * 100;

	m_iRockets--;

	side = - side;
}



BOOL CApache :: FireGun( )
{
	UTIL_MakeAimVectors( pev->angles );
		
	Vector posGun, angGun;
	GetAttachment( 1, posGun, angGun );

	Vector vecTarget = (m_posTarget - posGun).Normalize( );

	Vector vecOut;

	vecOut.x = DotProduct( gpGlobals->v_forward, vecTarget );
	vecOut.y = -DotProduct( gpGlobals->v_right, vecTarget );
	vecOut.z = DotProduct( gpGlobals->v_up, vecTarget );

	Vector angles = UTIL_VecToAngles (vecOut);

	angles.x = -angles.x;
	if (angles.y > 180)
		angles.y = angles.y - 360;
	if (angles.y < -180)
		angles.y = angles.y + 360;
	if (angles.x > 180)
		angles.x = angles.x - 360;
	if (angles.x < -180)
		angles.x = angles.x + 360;

	if (angles.x > m_angGun.x)
		m_angGun.x = V_min( angles.x, m_angGun.x + 12 );
	if (angles.x < m_angGun.x)
		m_angGun.x = V_max( angles.x, m_angGun.x - 12 );
	if (angles.y > m_angGun.y)
		m_angGun.y = V_min( angles.y, m_angGun.y + 12 );
	if (angles.y < m_angGun.y)
		m_angGun.y = V_max( angles.y, m_angGun.y - 12 );

	m_angGun.y = SetBoneController( 0, m_angGun.y );
	m_angGun.x = SetBoneController( 1, m_angGun.x );

	Vector posBarrel, angBarrel;
	GetAttachment( 0, posBarrel, angBarrel );
	Vector vecGun = (posBarrel - posGun).Normalize( );

	if (DotProduct( vecGun, vecTarget ) > 0.98)
	{
#if 1
		FireBullets( 1, posGun, vecGun, VECTOR_CONE_1DEGREES*3, 131072, BULLET_MONSTER_12MM, 1 );
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "turret/tu_fire1.wav", 1, 0.3);
		PLAY_DISTANT_SOUND(edict(), DISTANT_357);
#else
		static float flNext;
		TraceResult tr;
		UTIL_TraceLine( posGun, posGun + vecGun * 131072, dont_ignore_monsters, ENT( pev ), &tr );

		if (!m_pBeam)
		{
			m_pBeam = CBeam::BeamCreate( "sprites/lgtning.spr", 80 );
			m_pBeam->PointEntInit( pev->origin, entindex( ) );
			m_pBeam->SetEndAttachment( 1 );
			m_pBeam->SetColor( 255, 180, 96 );
			m_pBeam->SetBrightness( 192 );
		}

		if (flNext < gpGlobals->time)
		{
			flNext = gpGlobals->time + 0.5;
			m_pBeam->SetStartPos( tr.vecEndPos );
		}
#endif
		return TRUE;
	}
	return FALSE;
}



void CApache :: ShowDamage( void )
{
	if (m_iDoSmokePuff > 0 || RANDOM_LONG(0,99) > pev->health)
	{
		UTIL_Smoke(pev->origin, g_sModelIndexSmoke, RANDOM_LONG(0, 9) + 20, 12);
	}
	if (m_iDoSmokePuff > 0)
		m_iDoSmokePuff--;
}


int CApache :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	if (IsImmune(pevAttacker, flDamage))
		return 0;

	if (pevInflictor->owner == edict())
		return 0;

	if (bitsDamageType & DMG_BLAST)
	{
		flDamage *= 2;
	}

	GiveScorePoints(pevAttacker, flDamage);

	/*
	if ( (bitsDamageType & DMG_BULLET) && flDamage > 50)
	{
		// clip bullet damage at 50
		flDamage = 50;
	}
	*/

	// ALERT( at_console, "%.0f\n", flDamage );
	//Are we damaged at all?
	if (pev->health < pev->max_health)
	{
		//Took some damage.
		SetConditions(bits_COND_LIGHT_DAMAGE);
		if (pev->health < (pev->max_health / 2))
		{
			//Seriously damaged now.
			SetConditions(bits_COND_HEAVY_DAMAGE);
		}
		else
		{
			//Maybe somebody healed us somehow (trigger_hurt with negative damage?), clear this.
			ClearConditions(bits_COND_HEAVY_DAMAGE);
		}
	}
	else
	{
		//Maybe somebody healed us somehow (trigger_hurt with negative damage?), clear this.
		ClearConditions(bits_COND_LIGHT_DAMAGE);
	}
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}



void CApache::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	// ALERT( at_console, "%d %.0f\n", ptr->iHitgroup, flDamage );

	// ignore blades
	if (ptr->iHitgroup == 6 && (bitsDamageType & (DMG_ENERGYBEAM|DMG_BULLET|DMG_CLUB)))
		return;

	int gibModel = m_iEngineGibs;
	int gibCount = 0;
	int gibFlags = BREAK_METAL;
	bool isBlast = bitsDamageType & DMG_BLAST;

	if (isBlast) {
		gibModel = m_iBodyGibs;

		if (flDamage > 80) {
			gibCount = 16;
		}
		else if (flDamage > 30) {
			gibCount = 8;
		}
		else {
			gibCount = 4;
		}
	}

	Vector dir = ptr->vecPlaneNormal;
	Vector pos = ptr->vecEndPos;

	bool hitHard = flDamage >= 50;
	bool isShock = bitsDamageType & DMG_SHOCK;
	bool isEgon = bitsDamageType & DMG_ENERGYBEAM;
	bool hitWeakpoint = ptr->iHitgroup == 1 || ptr->iHitgroup == 2;

	if (isShock) {
		gibCount = 0;
	}
	else if (!isBlast && (hitWeakpoint || hitHard)) {
		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pos);
		WRITE_BYTE(TE_STREAK_SPLASH);
		WRITE_COORD(pos.x);
		WRITE_COORD(pos.y);
		WRITE_COORD(pos.z);
		WRITE_COORD(dir.x);
		WRITE_COORD(dir.y);
		WRITE_COORD(dir.z);
		WRITE_BYTE(ptr->iHitgroup == 1 ? 0 : 5);
		WRITE_SHORT(flDamage >= 40 ? 32 : 16);
		WRITE_SHORT(768);
		WRITE_SHORT(256);
		MESSAGE_END();

		Vector sprPos = pos + dir * 4;
		if (hitWeakpoint) {
			if (ptr->iHitgroup == 1) {
				UTIL_Explosion(sprPos, m_iGlassHit, 24, 80, 2 | 4 | 8);
			}
			else {
				UTIL_Explosion(sprPos, m_iEngineHit, 12, 50, 2 | 4 | 8);
			}
		}

		gibCount = 1 + (int)(flDamage / 30.0);

		if (ptr->iHitgroup == 1) {
			gibModel = m_iGlassGibs;
			gibFlags = BREAK_GLASS | BREAK_TRANS;
		}
	}

	if (gibCount) {
		if (dir.Length() < 1.0f) {
			dir = (ptr->vecEndPos - pev->origin).Normalize();
			dir.z *= 0.2f;
			dir = dir.Normalize();
		}
		dir = dir * (isBlast ? 400 : 200);

		UTIL_BreakModel(pos, g_vecZero, dir, isBlast ? 30 : 15, gibModel, gibCount, 1, gibFlags);
	}

	// hit hard, hits cockpit, hits engines
	if (hitHard || hitWeakpoint || isBlast || isEgon)
	{
		// ALERT( at_console, "%.0f\n", flDamage );
		AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );
		m_iDoSmokePuff = 3 + (flDamage / 5.0);
	}
	else
	{
		// do half damage in the body
		// AddMultiDamage( pevAttacker, this, flDamage / 2.0, bitsDamageType );
		UTIL_Ricochet( ptr->vecEndPos, 2.0 );
	}
}





class CApacheHVR : public CGrenade
{
	void Spawn( void );
	void Precache( void );
	void EXPORT IgniteThink( void );
	void EXPORT AccelerateThink( void );
	virtual const char* GetDeathNoticeWeapon() { return "rpg_rocket"; };

	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	virtual int MergedModelBody() { return MERGE_MDL_HVR; }

	int m_iTrail;
	Vector m_vecForward;
};
LINK_ENTITY_TO_CLASS( hvr_rocket, CApacheHVR )

TYPEDESCRIPTION	CApacheHVR::m_SaveData[] = 
{
//	DEFINE_FIELD( CApacheHVR, m_iTrail, FIELD_INTEGER ),	// Dont' save, precache
	DEFINE_FIELD( CApacheHVR, m_vecForward, FIELD_VECTOR ),
};

IMPLEMENT_SAVERESTORE( CApacheHVR, CGrenade )

void CApacheHVR :: Spawn( void )
{
	Precache( );
	// motor
	pev->solid = SOLID_BBOX;
	pev->movetype = MOVETYPE_FLY;

	SetGrenadeModel();
	UTIL_SetSize(pev, Vector( 0, 0, 0), Vector(0, 0, 0));
	UTIL_SetOrigin( pev, pev->origin );

	SetThink( &CApacheHVR::IgniteThink );
	SetTouch( &CApacheHVR::ExplodeTouch );

	UTIL_MakeAimVectors( pev->angles );
	m_vecForward = gpGlobals->v_forward;
	pev->gravity = 0.5;

	pev->nextthink = gpGlobals->time + 0.1;

	pev->dmg = 150;
}


void CApacheHVR :: Precache( void )
{
	m_defaultModel = "models/HVR.mdl";
	PRECACHE_MODEL(GetModel());
	m_iTrail = PRECACHE_MODEL("sprites/smoke.spr");
	PRECACHE_SOUND ("weapons/rocket1.wav");
}


void CApacheHVR :: IgniteThink( void  )
{
	pev->effects |= EF_LIGHT;

	// make rocket sound
	EMIT_SOUND( ENT(pev), CHAN_VOICE, "weapons/rocket1.wav", 1, 0.5 );

	UTIL_BeamFollow(entindex(), m_iTrail, 15, 5, RGBA(224, 224, 255, 255), MSG_BROADCAST, NULL);

	// set to accelerate
	SetThink( &CApacheHVR::AccelerateThink );
	pev->nextthink = gpGlobals->time + 0.1;
	ParametricInterpolation(0.1f);
}


void CApacheHVR :: AccelerateThink( void  )
{
	// check world boundaries
	if( !IsInWorld() )
	{
		UTIL_Remove( this );
		return;
	}

	// accelerate
	float flSpeed = pev->velocity.Length();
	if (flSpeed < 1800)
	{
		pev->velocity = pev->velocity + m_vecForward * 200;
	}

	// re-aim
	pev->angles = UTIL_VecToAngles( pev->velocity );

	pev->nextthink = gpGlobals->time + 0.1;
	ParametricInterpolation(0.1f);
}
