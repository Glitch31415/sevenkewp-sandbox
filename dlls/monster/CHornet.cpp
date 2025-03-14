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
//=========================================================
// Hornets
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"monsters.h"
#include	"weapons.h"
#include	"env/CSoundEnt.h"
#include	"CHornet.h"
#include	"gamerules.h"


int iHornetTrail;
int iHornetPuff;

LINK_ENTITY_TO_CLASS( hornet, CHornet )

const char* CHornet::pBuzzSounds[] =
{
	"hornet/ag_buzz1.wav",
	"hornet/ag_buzz2.wav",
	"hornet/ag_buzz3.wav",
};
const char* CHornet::pHitSounds[] =
{
	"hornet/ag_hornethit1.wav",
	"hornet/ag_hornethit2.wav",
	"hornet/ag_hornethit3.wav",
};

//=========================================================
// Save/Restore
//=========================================================
TYPEDESCRIPTION	CHornet::m_SaveData[] = 
{
	DEFINE_FIELD( CHornet, m_flStopAttack, FIELD_TIME ),
	DEFINE_FIELD( CHornet, m_iHornetType, FIELD_INTEGER ),
	DEFINE_FIELD( CHornet, m_flFlySpeed, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CHornet, CBaseMonster )

//=========================================================
// don't let hornets gib, ever.
//=========================================================
int CHornet :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// filter these bits a little.
	bitsDamageType &= ~ ( DMG_ALWAYSGIB );
	bitsDamageType |= DMG_NEVERGIB;

	return CBaseMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
//=========================================================
void CHornet :: Spawn( void )
{
	Precache();
	
	pev->movetype = MOVETYPE_BOUNCE;
	pev->gravity = FLT_MIN;
	pev->friction = 1.0f;

	pev->solid		= SOLID_TRIGGER;
	pev->takedamage = DAMAGE_YES;
	pev->flags		|= FL_MONSTER;
	SetHealth();// weak!
	
	if ( g_pGameRules->IsMultiplayer() )
	{
		// hornets don't live as long in multiplayer
		m_flStopAttack = gpGlobals->time + 3.5;
	}
	else
	{
		m_flStopAttack	= gpGlobals->time + 5.0;
	}

	m_flStartTrack = gpGlobals->time + 0.2f;

	m_flFieldOfView = 0.9; // +- 25 degrees

	if ( RANDOM_LONG ( 1, 5 ) <= 2 )
	{
		m_iHornetType = HORNET_TYPE_RED;
		m_flFlySpeed = HORNET_RED_SPEED;
	}
	else
	{
		m_iHornetType = HORNET_TYPE_ORANGE;
		m_flFlySpeed = HORNET_ORANGE_SPEED;
	}

	SET_MODEL(ENT( pev ), GetModel());
	SetSize(Vector( -4, -4, -4 ), Vector( 4, 4, 4 ) );

	SetTouch( &CHornet::TrackTouch);
	SetThink( &CHornet::StartTrack );

	edict_t *pSoundEnt = pev->owner;
	if ( !pSoundEnt )
		pSoundEnt = edict();

	if ( !FNullEnt(pev->owner) && (pev->owner->v.flags & FL_CLIENT) )
	{
		pev->dmg = GetDamage(gSkillData.sk_plr_hornet);
	}
	else
	{
		// no real owner, or owner isn't a client. 
		pev->dmg = GetDamage(gSkillData.sk_hornet_dmg);
	}
	
	m_lastPos = pev->origin;

	pev->nextthink = gpGlobals->time;
	ResetSequenceInfo( );
}


void CHornet :: Precache()
{
	CBaseMonster::Precache();

	m_defaultModel = "models/hornet.mdl";
	PRECACHE_MODEL(GetModel());

	// TODO: These could be a sound array to make use of mp_soundvariety.
	// The weapon will need updating in addition to alien grunts
	PRECACHE_SOUND( "agrunt/ag_fire1.wav" );
	PRECACHE_SOUND( "agrunt/ag_fire2.wav" );
	PRECACHE_SOUND( "agrunt/ag_fire3.wav" );

	PRECACHE_SOUND_ARRAY(pBuzzSounds);
	PRECACHE_SOUND_ARRAY(pHitSounds);

	iHornetPuff = PRECACHE_MODEL( "sprites/muz1.spr" );
	iHornetTrail = PRECACHE_MODEL("sprites/laserbeam.spr");
}	

//=========================================================
// hornets will never get mad at each other, no matter who the owner is.
//=========================================================
int CHornet::IRelationship ( CBaseEntity *pTarget )
{
	if ( pTarget->pev->modelindex == pev->modelindex )
	{
		return R_NO;
	}

	// don't track friendly players
	CBaseEntity* owner = CBaseEntity::Instance(pev->owner);
	if (!FNullEnt(pev->owner) && owner) {
		CBaseMonster* mon = owner->MyMonsterPointer();
		if (mon && mon->IsPlayer()) {
			return mon->IRelationship(pTarget);
		}
	}

	return CBaseMonster :: IRelationship( pTarget );
}

//=========================================================
// ID's Hornet as their owner
//=========================================================
int CHornet::Classify ( void )
{

	if ( pev->owner && pev->owner->v.flags & FL_CLIENT)
	{
		return CLASS_PLAYER_BIOWEAPON;
	}

	return	CLASS_ALIEN_BIOWEAPON;
}

const char* CHornet::DisplayName() {
	return m_displayName ? CBaseMonster::DisplayName() : "Hornet";
}

//=========================================================
// StartTrack - starts a hornet out tracking its target
//=========================================================
void CHornet :: StartTrack ( void )
{
	IgniteTrail();

	SetTouch( &CHornet::TrackTouch );
	SetThink( &CHornet::TrackTarget );

	pev->nextthink = gpGlobals->time;
}

//=========================================================
// StartDart - starts a hornet out just flying straight.
//=========================================================
void CHornet :: StartDart ( void )
{
	IgniteTrail();

	SetTouch( &CHornet::DartTouch );

	SetThink( &CHornet::DartThink);
	pev->nextthink = gpGlobals->time;
}

void CHornet::IgniteTrail( void )
{
/*

  ted's suggested trail colors:

r161
g25
b97

r173
g39
b14

old colors
		case HORNET_TYPE_RED:
			WRITE_BYTE( 255 );   // r, g, b
			WRITE_BYTE( 128 );   // r, g, b
			WRITE_BYTE( 0 );   // r, g, b
			break;
		case HORNET_TYPE_ORANGE:
			WRITE_BYTE( 0   );   // r, g, b
			WRITE_BYTE( 100 );   // r, g, b
			WRITE_BYTE( 255 );   // r, g, b
			break;
	
*/
	CBaseEntity* owner = CBaseEntity::Instance(pev->owner);
	CBaseMonster* mon = owner ? owner->MyMonsterPointer() : NULL;
	bool firedByAllyMonster = mon && !mon->IsPlayer() && CBaseEntity::IRelationship(mon->Classify(), CLASS_PLAYER) == R_AL;

	// trail
	RGBA color = RGBA(255,255,255);

	switch (m_iHornetType)
	{
	case HORNET_TYPE_RED:
		if (firedByAllyMonster) {
			color = RGBA(0, 128, 255);
		}
		else {
			color = RGBA(179, 39, 14);
		}

		break;
	case HORNET_TYPE_ORANGE:
		if (firedByAllyMonster) {
			color = RGBA(0, 255, 200);
		}
		else {
			color = RGBA(255, 128, 0);
		}
		break;
	}

	UTIL_BeamFollow(entindex(), iHornetTrail, 10, 2, color);
}

//=========================================================
// Hornet is flying, gently tracking target
//=========================================================
void CHornet :: TrackTarget ( void )
{
	pev->nextthink = gpGlobals->time + 0.01f;

	if (gpGlobals->time < m_flNextTrack || gpGlobals->time < m_flStartTrack) {
		return;
	}

	Vector	vecFlightDir;
	Vector	vecDirToEnemy;
	float	flDelta;

	StudioFrameAdvance( );

	if (gpGlobals->time > m_flStopAttack)
	{
		SetTouch( NULL );
		SetThink( &CHornet::SUB_Remove );
		return;
	}

	// UNDONE: The player pointer should come back after returning from another level
	if ( m_hEnemy == NULL )
	{// enemy is dead.
		Look( 512 );
		m_hEnemy = BestVisibleEnemy( );
	}
	
	if ( m_hEnemy != NULL && FVisible( m_hEnemy ))
	{
		m_vecEnemyLKP = m_hEnemy->BodyTarget( pev->origin );
	}
	else
	{
		m_vecEnemyLKP = m_vecEnemyLKP + pev->velocity * m_flFlySpeed * 0.1;
	}

	vecDirToEnemy = ( m_vecEnemyLKP - pev->origin ).Normalize();

	if (pev->velocity.Length() < 0.1)
		vecFlightDir = vecDirToEnemy;
	else 
		vecFlightDir = pev->velocity.Normalize();

	// measure how far the turn is, the wider the turn, the slow we'll go this time.
	flDelta = DotProduct ( vecFlightDir, vecDirToEnemy );
	
	if ( flDelta < 0.5 )
	{// hafta turn wide again. play sound
		EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pBuzzSounds), HORNET_BUZZ_VOLUME, ATTN_NORM);
	}

	if ( flDelta <= 0 && m_iHornetType == HORNET_TYPE_RED )
	{// no flying backwards, but we don't want to invert this, cause we'd go fast when we have to turn REAL far.
		flDelta = 0.25;
	}

	pev->velocity = ( vecFlightDir + vecDirToEnemy).Normalize();

	if ( pev->owner && (pev->owner->v.flags & FL_MONSTER) )
	{
		// random pattern only applies to hornets fired by monsters, not players. 

		pev->velocity.x += RANDOM_FLOAT ( -0.10, 0.10 );// scramble the flight dir a bit.
		pev->velocity.y += RANDOM_FLOAT ( -0.10, 0.10 );
		pev->velocity.z += RANDOM_FLOAT ( -0.10, 0.10 );
	}
	
	switch ( m_iHornetType )
	{
		case HORNET_TYPE_RED:
			pev->velocity = pev->velocity * ( m_flFlySpeed * flDelta );// scale the dir by the ( speed * width of turn )
			m_flNextTrack = gpGlobals->time + RANDOM_FLOAT(0.1, 0.3);
			break;
		case HORNET_TYPE_ORANGE:
			pev->velocity = pev->velocity * m_flFlySpeed;// do not have to slow down to turn.
			m_flNextTrack = gpGlobals->time + 0.1;// fixed think time
			break;
	}

	pev->angles = UTIL_VecToAngles (pev->velocity);

	if (pev->solid != SOLID_TRIGGER) {
		pev->solid = SOLID_TRIGGER;
		UTIL_SetOrigin(pev, pev->origin);
	}
	

	// if hornet is close to the enemy, jet in a straight line for a half second.
	// (only in the single player game)
	if ( m_hEnemy != NULL && !g_pGameRules->IsMultiplayer() )
	{
		if ( flDelta >= 0.4 && ( pev->origin - m_vecEnemyLKP ).Length() <= 300 )
		{
			UTIL_Sprite(pev->origin, iHornetPuff, 2, 128);
			EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pBuzzSounds), HORNET_BUZZ_VOLUME, ATTN_NORM);
			pev->velocity = pev->velocity * 2;
			m_flNextTrack = gpGlobals->time + 1.0;
			// don't attack again
			m_flStopAttack = gpGlobals->time;
		}
	}
}

void CHornet::DartThink(void) {
	if (gpGlobals->time > m_flStopAttack) {
		SetThink(&CHornet::SUB_Remove);
	}

	pev->nextthink = gpGlobals->time + 0.01f;

	// using parametric for darting hornets only because it fixes the trail
	// stopping short of the impact point. Not using it for tracking hornets
	// because they move erraticaly and sometimes show trails into the world origin
	// or player due getting stuck inside things and then coming back into view.
	pev->movetype = MOVETYPE_FLY;
	ParametricInterpolation(0.01f);
}

//=========================================================
// Tracking Hornet hit something
//=========================================================
void CHornet :: TrackTouch ( CBaseEntity *pOther )
{
	if ( pOther->edict() == pev->owner || pOther->pev->solid == SOLID_TRIGGER)
	{// bumped into the guy that shot it.
		return;
	}

	if ( IRelationship( pOther ) <= R_NO)
	{
		// hit something we don't want to hurt, so turn around.

		pev->velocity = pev->velocity.Normalize();

		pev->velocity.x *= -1;
		pev->velocity.y *= -1;

		pev->origin = pev->origin + pev->velocity * 4; // bounce the hornet off a bit.
		pev->velocity = pev->velocity * m_flFlySpeed;

		return;
	}

	DieTouch( pOther );
}

void CHornet::DartTouch( CBaseEntity *pOther )
{
	if (pOther->edict() == pev->owner || pOther->pev->solid == SOLID_TRIGGER)
	{// bumped into the guy that shot it.
		return;
	}

	DieTouch( pOther );
}

void CHornet::DieTouch ( CBaseEntity *pOther )
{
	if ( pOther && pOther->pev->takedamage )
	{// do the damage

		EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pHitSounds), 1, ATTN_NORM);
			
		pOther->TakeDamage( pev, VARS( pev->owner ), pev->dmg, DMG_BULLET );
	}

	pev->modelindex = 0;// so will disappear for the 0.1 secs we wait until NEXTTHINK gets rid
	pev->solid = SOLID_NOT;

	SetThink ( &CHornet::SUB_Remove );
	pev->nextthink = gpGlobals->time + 1;// stick around long enough for the sound to finish!
}

