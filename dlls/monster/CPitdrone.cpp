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
//=========================================================
// pit drone - medium sized, fires sharp teeth like spikes and swipes with sharp appendages
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"nodes.h"
#include	"effects.h"
#include	"decals.h"
#include	"CSoundEnt.h"
#include	"game.h"

#define		SQUID_SPRINT_DIST	256 // how close the squid has to get before starting to sprint and refusing to swerve

int			   iSpikeTrail;
int iPitdroneSpitSprite;
#define PITDRONE_CLIP_SIZE 6
	

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_PITDRONE_HURTHOP = LAST_COMMON_SCHEDULE + 1,
	SCHED_PITDRONE_SMELLFOOD,
	SCHED_PITDRONE_EAT,
	SCHED_PITDRONE_SNIFF_AND_EAT,
	SCHED_PITDRONE_WALLOW,
	SCHED_PITDRONE_COVER_AND_RELOAD,
	SCHED_PITDRONE_WAIT_FACE_ENEMY,
	SCHED_PITDRONE_TAKECOVER_FAILED,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum 
{
	TASK_PITDRONE_HOPTURN = LAST_COMMON_TASK + 1,
};

//=========================================================
// Bullsquid's spit projectile
//=========================================================
class CPitdroneSpike : public CBaseEntity
{
public:
	void Precache() override;
	void Spawn( void );

	static void Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, Vector vecAngles );
	void EXPORT SpikeTouch( CBaseEntity *pOther );

	void EXPORT StartTrail();
	void EXPORT FlyThink();

	const char* GetDeathNoticeWeapon() { return "weapon_crowbar"; }

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int  m_maxFrame;
};

LINK_ENTITY_TO_CLASS( pitdronespike, CPitdroneSpike )

TYPEDESCRIPTION	CPitdroneSpike::m_SaveData[] =
{
	DEFINE_FIELD( CPitdroneSpike, m_maxFrame, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CPitdroneSpike, CBaseEntity )

void CPitdroneSpike::Precache()
{
	PRECACHE_MODEL("models/pit_drone_spike.mdl" );
	PRECACHE_SOUND( "weapons/xbow_hitbod1.wav" );
	PRECACHE_SOUND( "weapons/xbow_hit1.wav" );

	iSpikeTrail = PRECACHE_MODEL("sprites/spike_trail.spr" );
}

void CPitdroneSpike:: Spawn( void )
{
	pev->classname = MAKE_STRING( "pitdronespike" );
	pev->movetype = MOVETYPE_FLY;

	pev->solid = SOLID_BBOX;
	pev->takedamage = DAMAGE_YES;
	pev->flags |= FL_MONSTER;
	pev->health = 1;

	SET_MODEL(ENT(pev), "models/pit_drone_spike.mdl");
	pev->frame = 0;
	pev->scale = 0.5;

	UTIL_SetSize( pev, Vector( -4, -4, -4 ), Vector( 4, 4, 4 ) );

	m_maxFrame = (float) MODEL_FRAMES( pev->modelindex ) - 1;
}

void CPitdroneSpike::Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, Vector vecAngles )
{
	CPitdroneSpike *pSpit = GetClassPtr( ( CPitdroneSpike *)NULL );
	
	pSpit->pev->angles = vecAngles;
	UTIL_SetOrigin( pSpit->pev, vecStart );

	pSpit->Spawn();

	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->owner = ENT(pevOwner);

	pSpit->SetThink ( &CPitdroneSpike::StartTrail );
	pSpit->pev->nextthink = gpGlobals->time + 0.1;
	pSpit->ParametricInterpolation(0.1f);
}

void CPitdroneSpike::SpikeTouch( CBaseEntity *pOther )
{
	// splat sound
	int iPitch = RANDOM_FLOAT( 120, 140 );

	if( !pOther->pev->takedamage )
	{
		EMIT_SOUND_DYN( edict(), CHAN_VOICE, "weapons/xbow_hit1.wav", VOL_NORM, ATTN_NORM, 0, iPitch );
	}
	else
	{
		pOther->TakeDamage( pev, &pev->owner->v, gSkillData.sk_pitdrone_dmg_spit, DMG_GENERIC );
		EMIT_SOUND_DYN( edict(), CHAN_VOICE, "weapons/xbow_hitbod1.wav", VOL_NORM, ATTN_NORM, 0, iPitch );
	}

	SetTouch( nullptr );

	//Stick it in the world for a bit
	//TODO: maybe stick it on any entity that reports FL_WORLDBRUSH too?
	if( !strcmp( "worldspawn", STRING( pOther->pev->classname ) ) )
	{
		const auto vecDir = pev->velocity.Normalize();

		const auto vecOrigin = pev->origin - vecDir * 6;

		UTIL_SetOrigin( pev, vecOrigin );

		pev->angles = UTIL_VecToAngles( vecDir );
		pev->solid = SOLID_NOT;
		pev->movetype = MOVETYPE_FLY;

		pev->angles.z = RANDOM_LONG( 0, 360 );

		pev->velocity = g_vecZero;
		pev->avelocity = g_vecZero;

		SetThink( &CBaseEntity::SUB_FadeOut );
		pev->nextthink = gpGlobals->time + 10.0;
	}
	else
	{
		//Hit something else, remove
		SetThink( &CBaseEntity::SUB_Remove );
		pev->nextthink = gpGlobals->time + 0.1;
	}
}

void CPitdroneSpike::StartTrail()
{
	UTIL_BeamFollow(entindex(), iSpikeTrail, 2, 1, RGBA(197, 194, 11, 192));

	SetTouch( &CPitdroneSpike::SpikeTouch );
	SetThink( &CPitdroneSpike::FlyThink );

	pev->nextthink = gpGlobals->time + 0.1f;
	ParametricInterpolation(0.1f);
}

void CPitdroneSpike::FlyThink() {
	pev->nextthink = gpGlobals->time + 0.1f;
	ParametricInterpolation(0.1f);
}

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		PITDRONE_AE_SPIT		( 1 )
#define		PITDRONE_AE_BITE		( 2 )
#define		PITDRONE_AE_TAILWHIP	( 4 )
#define		PITDRONE_AE_HOP		( 5 )
#define		PITDRONE_AE_THROW		( 6 )
#define PITDRONE_AE_RELOAD	7

namespace PitdroneBodygroup
{
enum PitdroneBodygroup
{
	Weapons = 1
};
}

namespace PitdroneWeapon
{
enum PitdroneWeapon
{
	Empty = 0,
	Full,
	Two = 6,
	One = 7
};
}

class CPitdrone : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  ISoundMask( void );
	int  Classify ( void );
	const char* DisplayName();
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void IdleSound( void );
	void PainSound( void );
	void AlertSound ( void );
	void StartFollowingSound();
	void StopFollowingSound();
	void CantFollowSound();
	void StartTask ( Task_t *pTask );
	void RunTask ( Task_t *pTask );
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	BOOL CheckMeleeAttack2 ( float flDot, float flDist );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	void RunAI( void );
	BOOL FValidateHintType ( short sHint );
	Schedule_t *GetSchedule( void );
	Schedule_t *GetScheduleOfType ( int Type );
	const char* GetTaskName(int taskIdx);
	int IRelationship ( CBaseEntity *pTarget );
	int IgnoreConditions ( void );
	const char* GetDeathNoticeWeapon() { return "weapon_crowbar"; }

	void CheckAmmo() override;
	void Killed(entvars_t* pevAttacker, int iGib) override;
	void GibMonster() override;
	void KeyValue( KeyValueData* pkvd ) override;

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	float m_flLastHurtTime;// we keep track of this, because if something hurts a squid, it will forget about its love of headcrabs for a while.
	float m_flNextSpikeTime;// last time the pit drone used the spike attack.
	int m_iInitialAmmo;
	float m_flNextEatTime;
	bool m_hasAmmoGroup;
	bool m_hasSpitEvent;
	bool m_didSpit;

private:
	static const char* pAlertSounds[];
	//static const char* pTalkSounds[];
	static const char* pDieSounds[];
	//static const char* pHuntSounds[];
	static const char* pIdleSounds[];
	static const char* pPainSounds[];
	static const char* pBiteSounds[];
};
LINK_ENTITY_TO_CLASS( monster_pitdrone, CPitdrone )

const char* CPitdrone::pAlertSounds[] =
{
	"pitdrone/pit_drone_alert1.wav",
	"pitdrone/pit_drone_alert2.wav",
	"pitdrone/pit_drone_alert3.wav",
};
/*
const char* CPitdrone::pTalkSounds[] =
{
	"pitdrone/pit_drone_communicate1.wav",
	"pitdrone/pit_drone_communicate2.wav",
	"pitdrone/pit_drone_communicate3.wav",
	"pitdrone/pit_drone_communicate4.wav",
};
const char* CPitdrone::pHuntSounds[] =
{
	"pitdrone/pit_drone_hunt1.wav",
	"pitdrone/pit_drone_hunt2.wav",
	"pitdrone/pit_drone_hunt3.wav",
};
*/
const char* CPitdrone::pDieSounds[] =
{
	"pitdrone/pit_drone_die1.wav",
	"pitdrone/pit_drone_die2.wav",
	"pitdrone/pit_drone_die3.wav",
};
const char* CPitdrone::pIdleSounds[] =
{
	"pitdrone/pit_drone_idle1.wav",
	"pitdrone/pit_drone_idle2.wav",
	"pitdrone/pit_drone_idle3.wav",
};
const char* CPitdrone::pPainSounds[] =
{
	"pitdrone/pit_drone_pain1.wav",
	"pitdrone/pit_drone_pain2.wav",
	"pitdrone/pit_drone_pain3.wav",
	"pitdrone/pit_drone_pain4.wav",
};
const char* CPitdrone::pBiteSounds[] =
{
	"bullchicken/bc_bite2.wav",
	"bullchicken/bc_bite3.wav",
};

TYPEDESCRIPTION	CPitdrone::m_SaveData[] = 
{
	DEFINE_FIELD( CPitdrone, m_flLastHurtTime, FIELD_TIME ),
	DEFINE_FIELD( CPitdrone, m_flNextSpikeTime, FIELD_TIME ),
	DEFINE_FIELD( CPitdrone, m_flNextEatTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CPitdrone, CBaseMonster )

//=========================================================
// IgnoreConditions 
//=========================================================
int CPitdrone::IgnoreConditions ( void )
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if ( gpGlobals->time - m_flLastHurtTime <= 20 )
	{
		// haven't been hurt in 20 seconds, so let the drone care about stink. 
		iIgnore = bits_COND_SMELL | bits_COND_SMELL_FOOD;
	}

	return iIgnore;
}

int CPitdrone::IRelationship ( CBaseEntity *pTarget )
{
	// Mark pit drones as allies, but only if we're on the same class
	if ( FClassnameIs ( pTarget->pev, "monster_pitdrone" ) && ( Classify() == pTarget->Classify() ) )
	{
		return R_AL;
	}

	return CBaseMonster :: IRelationship ( pTarget );
}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CPitdrone :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( m_iInitialAmmo == -1
		|| GetBodygroup( PitdroneBodygroup::Weapons ) == PitdroneWeapon::Empty
		|| ( IsMoving() && flDist >= 512 ) )
	{
		// squid will far too far behind if he stops running to spit at this distance from the enemy.
		return FALSE;
	}

	if ( flDist > 128 && flDist <= 784 && flDot >= 0.5 && gpGlobals->time >= m_flNextSpikeTime )
	{
		if ( m_hEnemy != NULL )
		{
			if ( fabs( pev->origin.z - m_hEnemy->pev->origin.z ) > 256 )
			{
				// don't try to spit at someone up really high or down really low.
				return FALSE;
			}
		}

		if ( IsMoving() )
		{
			// don't spit again for a long time, resume chasing enemy.
			m_flNextSpikeTime = gpGlobals->time + 5;
		}
		else
		{
			// not moving, so spit again pretty soon.
			m_flNextSpikeTime = gpGlobals->time + 0.5;
		}

		return TRUE;
	}

	return FALSE;
}

BOOL CPitdrone :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	if ( flDist <= 64 && flDot >= 0.7 )
	{
		return RANDOM_LONG( 0, 3 ) == 0;
	}
	return FALSE;
}

BOOL CPitdrone :: CheckMeleeAttack2 ( float flDot, float flDist )
{
	if ( flDist <= 64 && flDot >= 0.7 && !HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
	{
		return TRUE;
	}
	return FALSE;
}  

//=========================================================
//  FValidateHintType 
//=========================================================
BOOL CPitdrone :: FValidateHintType ( short sHint )
{
	static short sSquidHints[] =
	{
		HINT_WORLD_HUMAN_BLOOD,
	};

	for ( int i = 0 ; i < (int)ARRAYSIZE ( sSquidHints ) ; i++ )
	{
		if ( sSquidHints[ i ] == sHint )
		{
			return TRUE;
		}
	}

	ALERT ( at_aiconsole, "Couldn't validate hint type" );
	return FALSE;
}

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. In the base class implementation,
// monsters care about all sounds, but no scents.
//=========================================================
int CPitdrone :: ISoundMask ( void )
{
	//return	bits_SOUND_WORLD	|
			//bits_SOUND_COMBAT	|
			//bits_SOUND_CARCASS	|
			//bits_SOUND_MEAT		|
			//bits_SOUND_GARBAGE	|
			//bits_SOUND_PLAYER;
	return bits_ALL_SOUNDS;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CPitdrone :: Classify ( void )
{
	return	CBaseMonster::Classify(CLASS_ALIEN_PREDATOR);
}

const char* CPitdrone::DisplayName() {
	return m_displayName ? CBaseMonster::DisplayName() : "Pit Drone";
}

//=========================================================
// IdleSound 
//=========================================================
void CPitdrone :: IdleSound ( void )
{
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pIdleSounds), 1, ATTN_NORM, 0, PITCH_NORM);
}

//=========================================================
// PainSound 
//=========================================================
void CPitdrone :: PainSound ( void )
{
	int iPitch = RANDOM_LONG( 85, 120 );

	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pPainSounds), 1, ATTN_NORM, 0, iPitch);
}

//=========================================================
// AlertSound
//=========================================================
void CPitdrone :: AlertSound ( void )
{
	int iPitch = RANDOM_LONG( 140, 160 );

	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAlertSounds), 1, ATTN_NORM, 0, iPitch);
}

void CPitdrone::StartFollowingSound() {
	int iPitch = RANDOM_LONG(85, 120);
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pPainSounds), 1, ATTN_NORM, 0, iPitch);
}

void CPitdrone::StopFollowingSound() {
	int iPitch = 100 + RANDOM_LONG(0, 9);
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pDieSounds), 1, ATTN_NORM, 0, iPitch);
}

void CPitdrone::CantFollowSound() {
	int iPitch = 100 + RANDOM_LONG(0, 9);
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pDieSounds), 1, ATTN_NORM, 0, iPitch);
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CPitdrone :: SetYawSpeed ( void )
{
	int ys;

	ys = 0;

	switch ( m_Activity )
	{
	case	ACT_WALK:			ys = 90;	break;
	case	ACT_RUN:			ys = 90;	break;
	case	ACT_IDLE:			ys = 90;	break;
	case	ACT_RANGE_ATTACK1:	ys = 90;	break;
	default:
		ys = 90;
		break;
	}

	pev->yaw_speed = ys * gSkillData.sk_yawspeed_mult;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CPitdrone :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case PITDRONE_AE_SPIT:
		{
			bool hasAmmoLoaded = GetBodygroup(PitdroneBodygroup::Weapons) != PitdroneWeapon::Empty;

			if( m_iInitialAmmo != -1 && (!m_hasAmmoGroup || hasAmmoLoaded))
			{
				Vector	vecSpitOffset;
				Vector	vecSpitDir;

				UTIL_MakeVectors( pev->angles );

				// !!!HACKHACK - the spot at which the spit originates (in front of the mouth) was measured in 3ds and hardcoded here.
				// we should be able to read the position of bones at runtime for this info.
				vecSpitOffset = ( gpGlobals->v_forward * 15 + gpGlobals->v_up * 36 );
				vecSpitOffset = ( pev->origin + vecSpitOffset );

				if (m_hEnemy)
					vecSpitDir = ((m_hEnemy->GetTargetOrigin() + m_hEnemy->pev->view_ofs) - vecSpitOffset).Normalize();
				else
					vecSpitDir = gpGlobals->v_forward;

				vecSpitDir.x += RANDOM_FLOAT( -0.05, 0.05 );
				vecSpitDir.y += RANDOM_FLOAT( -0.05, 0.05 );
				vecSpitDir.z += RANDOM_FLOAT( -0.05, 0 );

				// spew the spittle temporary ents.
				int count = UTIL_IsValidTempEntOrigin(vecSpitOffset) ? 10 : 5;
				UTIL_SpriteSpray(vecSpitOffset, vecSpitDir, iPitdroneSpitSprite, count, 110, 25);

				CPitdroneSpike::Shoot( pev, vecSpitOffset, vecSpitDir * 900, UTIL_VecToAngles( vecSpitDir ) );
				CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, NORMAL_GUN_VOLUME, 0.3);

				if (m_hasAmmoGroup) {
					auto ammoSubModel = GetBodygroup(PitdroneBodygroup::Weapons);

					//TODO: needs to be fixed so all spikes can be fired?
					if (ammoSubModel == PitdroneWeapon::Two)
					{
						ammoSubModel = PitdroneWeapon::Empty;
					}
					else
					{
						++ammoSubModel;
					}

					SetBodygroup(PitdroneBodygroup::Weapons, ammoSubModel);
					--m_cAmmoLoaded;
				}
			}
		}
		break;

		case PITDRONE_AE_BITE:
		{
			// SOUND HERE!
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.sk_pitdrone_dmg_bite, DMG_SLASH );
			CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, NORMAL_GUN_VOLUME, 0.3);
			
			if ( pHurt )
			{
				//pHurt->pev->punchangle.z = -15;
				//pHurt->pev->punchangle.x = -45;
				pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_forward * 100;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 100;
			}
		}
		break;

		case PITDRONE_AE_TAILWHIP:
		{
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.sk_pitdrone_dmg_whip, DMG_CLUB | DMG_ALWAYSGIB );
			CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, NORMAL_GUN_VOLUME, 0.3);
			
			if ( pHurt ) 
			{
				pHurt->pev->punchangle.z = -20;
				pHurt->pev->punchangle.x = 20;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 200;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 100;
			}
		}
		break;

		case PITDRONE_AE_HOP:
		{
			float flGravity = g_psv_gravity->value;

			// throw the squid up into the air on this frame.
			if ( FBitSet ( pev->flags, FL_ONGROUND ) )
			{
				pev->flags -= FL_ONGROUND;
			}

			// jump into air for 0.8 (24/30) seconds
//			pev->velocity.z += (0.875 * flGravity) * 0.5;
			pev->velocity.z += (0.625 * flGravity) * 0.5;
		}
		break;

		case PITDRONE_AE_THROW:
			{
				int iPitch;

				// squid throws its prey IF the prey is a client. 
				CBaseEntity *pHurt = CheckTraceHullAttack( 70, 0, 0 );


				if ( pHurt )
				{
					// croonchy bite sound
					iPitch = RANDOM_FLOAT( 90, 110 );
					EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pBiteSounds), 1, ATTN_NORM, 0, iPitch);

					
					//pHurt->pev->punchangle.x = RANDOM_LONG(0,34) - 5;
					//pHurt->pev->punchangle.z = RANDOM_LONG(0,49) - 25;
					//pHurt->pev->punchangle.y = RANDOM_LONG(0,89) - 45;
		
					// screeshake transforms the viewmodel as well as the viewangle. No problems with seeing the ends of the viewmodels.
					UTIL_ScreenShake( pHurt->pev->origin, 25.0, 1.5, 0.7, 2 );

					if ( pHurt->IsPlayer() )
					{
						UTIL_MakeVectors( pev->angles );
						pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 300 + gpGlobals->v_up * 300;
					}
				}
			}
		break;

		case PITDRONE_AE_RELOAD:
			{
				if( m_iInitialAmmo == -1 )
				{
					m_cAmmoLoaded = 0;
				}
				else
				{
					SetBodygroup( PitdroneBodygroup::Weapons, PitdroneWeapon::Full );
					m_cAmmoLoaded = PITDRONE_CLIP_SIZE;
				}

				ClearConditions( bits_COND_NO_AMMO_LOADED );
			}
			break;

		default:
			CBaseMonster::HandleAnimEvent( pEvent );
	}
}

//=========================================================
// Spawn
//=========================================================
void CPitdrone :: Spawn()
{
	Precache( );

	InitModel();
	SetSize(Vector( -16, -16, 0 ), Vector( 16, 16, 48 ) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->effects		= 0;
	m_flFieldOfView		= VIEW_FIELD_WIDE;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	m_flNextSpikeTime = gpGlobals->time;

	// if not specified, randomly decide to havepreloaded ammo
	if (m_iInitialAmmo == 0 && RANDOM_LONG(0,2) >= 1) {
		m_iInitialAmmo = PITDRONE_CLIP_SIZE;
	}

	m_hasAmmoGroup = GetBodygroups() > PitdroneBodygroup::Weapons;

	if (m_hasAmmoGroup) {
		if (m_iInitialAmmo < 0)
		{
			m_iInitialAmmo = 0;
			SetBodygroup(PitdroneBodygroup::Weapons, PitdroneWeapon::Empty);
		}
		else
		{
			SetBodygroup(PitdroneBodygroup::Weapons, PitdroneWeapon::One - m_iInitialAmmo);
		}
	}
	else {
		m_iInitialAmmo = 10;
	}
	

	m_cAmmoLoaded = m_iInitialAmmo;

	m_flNextEatTime = gpGlobals->time;

	m_hasSpitEvent = ActivityHasEvent(ACT_RANGE_ATTACK1, PITDRONE_AE_SPIT);

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CPitdrone :: Precache()
{
	CBaseMonster::Precache();

	m_defaultModel = "models/pit_drone.mdl";
	PRECACHE_MODEL(GetModel());
	PRECACHE_REPLACEMENT_MODEL("models/pit_drone_gibs.mdl" );

	UTIL_PrecacheOther( "pitdronespike" );
	
	iPitdroneSpitSprite = PRECACHE_MODEL("sprites/tinyspit.spr");// client side spittle.

	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event

	PRECACHE_SOUND_ARRAY(pAlertSounds);
	//PRECACHE_SOUND_ARRAY(pTalkSounds); // TODO: use these?
	//PRECACHE_SOUND_ARRAY(pHuntSounds); // TODO: Precache hunt3 sound if animations is used in the map
	PRECACHE_SOUND_ARRAY(pDieSounds);
	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pBiteSounds);

	PRECACHE_SOUND("pitdrone/pit_drone_melee_attack1.wav" );
	PRECACHE_SOUND("pitdrone/pit_drone_melee_attack2.wav" );
	PRECACHE_SOUND("pitdrone/pit_drone_attack_spike1.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_eat.wav" );
}

//========================================================
// RunAI - overridden for bullsquid because there are things
// that need to be checked every think.
//========================================================
void CPitdrone :: RunAI ( void )
{
	// first, do base class stuff
	CBaseMonster :: RunAI();

	if ( m_hEnemy != NULL && m_Activity == ACT_RUN )
	{
		// chasing enemy. Sprint for last bit
		if ( (pev->origin - m_hEnemy->pev->origin).Length2D() < SQUID_SPRINT_DIST )
		{
			pev->framerate = 1.25;
		}
	}

	if (m_Activity == ACT_RANGE_ATTACK1) {
		if (!m_hasSpitEvent && !m_didSpit && pev->frame > 128) {
			m_didSpit = true;
			MonsterEvent_t fakeEvt;
			fakeEvt.event = PITDRONE_AE_SPIT;
			fakeEvt.options = "";
			HandleAnimEvent(&fakeEvt);
		}
	}
	else {
		m_didSpit = false;
	}
}

//========================================================
// AI Schedules Specific to this monster
//=========================================================

// primary range attack
Task_t	tlPitdroneRangeAttack1[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
};

Schedule_t	slPitdroneRangeAttack1[] =
{
	{ 
		tlPitdroneRangeAttack1,
		ARRAYSIZE ( tlPitdroneRangeAttack1 ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED,
		0,
		"PIT_RANGE_ATTACK"
	},
};

// Chase enemy schedule
Task_t tlPitdroneChaseEnemy1[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_RANGE_ATTACK1	},// !!!OEM - this will stop nasty squid oscillation.
	{ TASK_GET_PATH_TO_ENEMY,	(float)0					},
	{ TASK_RUN_PATH,			(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0					},
};

Schedule_t slPitdroneChaseEnemy[] =
{
	{ 
		tlPitdroneChaseEnemy1,
		ARRAYSIZE ( tlPitdroneChaseEnemy1 ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_SMELL_FOOD		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK2	|
		bits_COND_TASK_FAILED		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER			|
		bits_SOUND_MEAT,
		"PIT_CHASE_ENEMY"
	},
};

Task_t tlPitdroneHurtHop[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_SOUND_WAKE,			(float)0		},
	{ TASK_PITDRONE_HOPTURN,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},// in case squid didn't turn all the way in the air.
};

Schedule_t slPitdroneHurtHop[] =
{
	{
		tlPitdroneHurtHop,
		ARRAYSIZE ( tlPitdroneHurtHop ),
		0,
		0,
		"PIT_HURT_HOP"
	}
};

// squid walks to something tasty and eats it.
Task_t tlPitdroneEat[] =
{
	{ TASK_STOP_MOVING,				(float)0				},
	{ TASK_EAT,						(float)10				},// this is in case the squid can't get to the food
	{ TASK_STORE_LASTPOSITION,		(float)0				},
	{ TASK_GET_PATH_TO_BESTSCENT,	(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_EAT,						(float)50				},
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_CLEAR_LASTPOSITION,		(float)0				},
};

Schedule_t slPitdroneEat[] =
{
	{
		tlPitdroneEat,
		ARRAYSIZE( tlPitdroneEat ),
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_NEW_ENEMY	,
		
		// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
		// here or the monster won't detect these sounds at ALL while running this schedule.
		bits_SOUND_MEAT			|
		bits_SOUND_CARCASS,
		"PIT_EAT"
	}
};

// this is a bit different than just Eat. We use this schedule when the food is far away, occluded, or behind
// the squid. This schedule plays a sniff animation before going to the source of food.
Task_t tlPitdroneSniffAndEat[] =
{
	{ TASK_STOP_MOVING,				(float)0				},
	{ TASK_EAT,						(float)10				},// this is in case the squid can't get to the food
	{ TASK_PLAY_SEQUENCE,			(float)ACT_DETECT_SCENT },
	{ TASK_STORE_LASTPOSITION,		(float)0				},
	{ TASK_GET_PATH_TO_BESTSCENT,	(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_EAT,						(float)50				},
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_CLEAR_LASTPOSITION,		(float)0				},
};

Schedule_t slPitdroneSniffAndEat[] =
{
	{
		tlPitdroneSniffAndEat,
		ARRAYSIZE( tlPitdroneSniffAndEat ),
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_NEW_ENEMY	,
		
		// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
		// here or the monster won't detect these sounds at ALL while running this schedule.
		bits_SOUND_MEAT			|
		bits_SOUND_CARCASS,
		"PIT_SNIFF_AND_EAT"
	}
};

// squid does this to stinky things. 
Task_t tlPitdroneWallow[] =
{
	{ TASK_STOP_MOVING,				(float)0				},
	{ TASK_EAT,						(float)10				},// this is in case the squid can't get to the stinkiness
	{ TASK_STORE_LASTPOSITION,		(float)0				},
	{ TASK_GET_PATH_TO_BESTSCENT,	(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_INSPECT_FLOOR},
	{ TASK_EAT,						(float)50				},// keeps squid from eating or sniffing anything else for a while.
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_CLEAR_LASTPOSITION,		(float)0				},
};

Schedule_t slPitdroneWallow[] =
{
	{
		tlPitdroneWallow,
		ARRAYSIZE( tlPitdroneWallow ),
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_NEW_ENEMY	,
		
		// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
		// here or the monster won't detect these sounds at ALL while running this schedule.
		bits_SOUND_GARBAGE,

		"PIT_WALLOW"
	}
};

Task_t tlPitdroneHideReload[] =
{
	{ TASK_STOP_MOVING,				( float ) 0			},
	{ TASK_SET_FAIL_SCHEDULE,		ACT_MELEE_ATTACK1	},
	{ TASK_FIND_COVER_FROM_ENEMY,	0					},
	{ TASK_RUN_PATH,				0					},
	{ TASK_WAIT_FOR_MOVEMENT,		0					},
	{ TASK_REMEMBER,				bits_MEMORY_INCOVER	},
	{ TASK_FACE_ENEMY,				0					},
	{ TASK_PLAY_SEQUENCE,			ACT_RELOAD			},
};

Schedule_t slPitdroneHideReload[] =
{
	{
		tlPitdroneHideReload,
		ARRAYSIZE( tlPitdroneHideReload ),
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,

		"PIT_HIDE_RELOAD"
	}
};

Task_t tlPitdroneWaitInCover[] =
{
	{ TASK_STOP_MOVING,			( float ) 0		},
	{ TASK_SET_ACTIVITY,		ACT_IDLE		},
	{ TASK_WAIT_FACE_ENEMY,		1				},
};

Schedule_t slPitdroneWaitInCover[] =
{
	{
		tlPitdroneWaitInCover,
		ARRAYSIZE( tlPitdroneWaitInCover ),
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2 |
		bits_COND_CAN_MELEE_ATTACK2 |
		bits_COND_NEW_ENEMY |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,

		"PIT_WAIT_IN_COVER"
	}
};

DEFINE_CUSTOM_SCHEDULES( CPitdrone ) 
{
	slPitdroneRangeAttack1,
	slPitdroneChaseEnemy,
	slPitdroneHurtHop,
	slPitdroneEat,
	slPitdroneSniffAndEat,
	slPitdroneWallow,
	slPitdroneHideReload,
	slPitdroneWaitInCover
};

IMPLEMENT_CUSTOM_SCHEDULES( CPitdrone, CBaseMonster )

//=========================================================
// GetSchedule 
//=========================================================
Schedule_t *CPitdrone :: GetSchedule( void )
{
	switch	( m_MonsterState )
	{
	case MONSTERSTATE_ALERT:
		{
			if ( HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE) )
			{
				return GetScheduleOfType ( SCHED_PITDRONE_HURTHOP );
			}

			if( m_flNextEatTime <= gpGlobals->time )
			{
				if( HasConditions( bits_COND_SMELL_FOOD ) )
				{
					CSound		*pSound;

					pSound = PBestScent();

					if( pSound && ( !FInViewCone( &pSound->m_vecOrigin ) || !FVisible( pSound->m_vecOrigin ) ) )
					{
						m_flNextEatTime = gpGlobals->time + 90;

						// scent is behind or occluded
						return GetScheduleOfType( SCHED_PITDRONE_SNIFF_AND_EAT );
					}

					m_flNextEatTime = gpGlobals->time + 90;

					// food is right out in the open. Just go get it.
					return GetScheduleOfType( SCHED_PITDRONE_EAT );
				}

				if( HasConditions( bits_COND_SMELL ) )
				{
					// there's something stinky. 
					CSound		*pSound;

					pSound = PBestScent();
					if( pSound )
					{
						m_flNextEatTime = gpGlobals->time + 90;
						return GetScheduleOfType( SCHED_PITDRONE_WALLOW );
					}
				}
			}

			break;
		}
	case MONSTERSTATE_COMBAT:
		{
// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster :: GetSchedule();
			}

			if ( HasConditions(bits_COND_NEW_ENEMY) )
			{
				return GetScheduleOfType ( SCHED_WAKE_ANGRY );
			}

			if( HasConditions( bits_COND_SEE_HATE ) && m_iInitialAmmo != -1 && LookupActivity(ACT_RELOAD) != ACTIVITY_NOT_AVAILABLE)
			{
				int ammoSubModel = GetBodygroup(PitdroneBodygroup::Weapons);
				if (ammoSubModel == 0)
					return GetScheduleOfType( SCHED_PITDRONE_COVER_AND_RELOAD );
			}

			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) && !HasConditions(bits_COND_NO_AMMO_LOADED))
			{
				return GetScheduleOfType(SCHED_RANGE_ATTACK1);
			}

			if ( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
			}

			if ( HasConditions( bits_COND_CAN_MELEE_ATTACK2 ) )
			{
				return GetScheduleOfType ( SCHED_MELEE_ATTACK2 );
			}
			
			return GetScheduleOfType ( SCHED_CHASE_ENEMY );

			break;
		}
	default:
		break;
	}

	return CBaseMonster :: GetSchedule();
}

//=========================================================
// GetScheduleOfType
//=========================================================
Schedule_t* CPitdrone :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_RANGE_ATTACK1:
		if (!HasConditions(bits_COND_NO_AMMO_LOADED)) {
			return &slPitdroneRangeAttack1[0];
		}
		else {
			return &slPitdroneHideReload[0];
		}
		break;
	case SCHED_PITDRONE_HURTHOP:
		return &slPitdroneHurtHop[ 0 ];
		break;
	case SCHED_PITDRONE_EAT:
		return &slPitdroneEat[ 0 ];
		break;
	case SCHED_PITDRONE_SNIFF_AND_EAT:
		return &slPitdroneSniffAndEat[ 0 ];
		break;
	case SCHED_PITDRONE_WALLOW:
		return &slPitdroneWallow[ 0 ];
		break;
	case SCHED_CHASE_ENEMY:
		return &slPitdroneChaseEnemy[ 0 ];
		break;

	case SCHED_PITDRONE_COVER_AND_RELOAD:
		return &slPitdroneHideReload[ 0 ];
		break;

	case SCHED_PITDRONE_WAIT_FACE_ENEMY:
		return &slPitdroneWaitInCover[ 0 ];
		break;
	}

	return CBaseMonster :: GetScheduleOfType ( Type );
}

const char* CPitdrone::GetTaskName(int taskIdx) {
	switch (taskIdx) {
	case TASK_PITDRONE_HOPTURN: return "TASK_PITDRONE_HOPTURN";
	default:
		return CBaseMonster::GetTaskName(taskIdx);
	}
}

//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule.
//=========================================================
void CPitdrone :: StartTask ( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_PITDRONE_HOPTURN:
		{
			SetActivity ( ACT_HOP );
			MakeIdealYaw ( m_vecEnemyLKP );
			break;
		}
	case TASK_GET_PATH_TO_ENEMY:
		{
			if (m_hEnemy && BuildRoute ( m_hEnemy->pev->origin, bits_MF_TO_ENEMY, m_hEnemy, true) )
			{
				m_iTaskStatus = TASKSTATUS_COMPLETE;
			}
			else
			{
				ALERT ( at_aiconsole, "GetPathToEnemy failed!!\n" );
				TaskFail();
			}
			break;
		}
	default:
		{
			CBaseMonster :: StartTask ( pTask );
			break;
		}
	}
}

//=========================================================
// RunTask
//=========================================================
void CPitdrone :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_PITDRONE_HOPTURN:
		{
			MakeIdealYaw( m_vecEnemyLKP );
			ChangeYaw( pev->yaw_speed );

			if ( m_fSequenceFinished )
			{
				m_iTaskStatus = TASKSTATUS_COMPLETE;
			}
			break;
		}
	default:
		{
			CBaseMonster :: RunTask( pTask );
			break;
		}
	}
}

void CPitdrone::CheckAmmo()
{
	if( m_iInitialAmmo != -1 && m_cAmmoLoaded <= 0 )
	{
		SetConditions( bits_COND_NO_AMMO_LOADED );
	}
}

void CPitdrone::Killed(entvars_t* pevAttacker, int iGib)
{
	if (!ShouldGibMonster(iGib))
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pDieSounds), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(0, 9));

	CBaseMonster::Killed(pevAttacker, iGib);
}

void CPitdrone::GibMonster()
{
	EMIT_SOUND( ENT( pev ), CHAN_WEAPON, "common/bodysplat.wav", 1, ATTN_NORM );

	if( CVAR_GET_FLOAT( "violence_agibs" ) != 0 )	// Should never get here, but someone might call it directly
	{
		//Note: the original doesn't check for German censorship
		CGib::SpawnRandomMergedGibs( pev, 6, MERGE_MDL_PIT_DRONE_GIBS, 0 );	// Throw alien gibs
	}

	// don't remove players!
	SetThink( &CBaseMonster::SUB_Remove );
	pev->nextthink = gpGlobals->time;

	// if the entity is outside the valid range for sound origins, then it needs
	// to be networked at least until the sound starts playing or else clients won't hear it.
	if (!UTIL_IsValidTempEntOrigin(pev->origin)) {
		pev->flags &= ~FL_MONSTER; // prevent the crowbar thinking this is a valid target
		pev->renderamt = 0;
		pev->rendermode = kRenderTransTexture;
		pev->nextthink = gpGlobals->time + 0.1f;
	}
}

void CPitdrone::KeyValue( KeyValueData* pkvd )
{
	if( FStrEq( "initammo", pkvd->szKeyName ) )
	{
		m_iInitialAmmo = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}
