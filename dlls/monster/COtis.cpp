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
// monster template
//=========================================================
// UNDONE: Holster weapon?

#include	"extdll.h"
#include	"util.h"
#include	"monsters.h"
#include	"CTalkSquadMonster.h"
#include	"schedule.h"
#include	"defaultai.h"
#include	"CCineMonster.h"
#include	"weapons.h"
#include	"CSoundEnt.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
// first flag is otis dying for scripted sequences?
#define		OTIS_AE_DRAW		( 2 )
#define		OTIS_AE_SHOOT		( 3 )
#define		OTIS_AE_HOLSTER	( 4 )

#define	OTIS_BODY_GUNHOLSTERED	0
#define	OTIS_BODY_GUNDRAWN		1
#define OTIS_BODY_GUNGONE			2



namespace OtisBodyGroup
{
enum OtisBodyGroup
{
	Weapons = 1,
	Heads = 2
};
}

namespace OtisWeapon
{
enum OtisWeapon
{
	Random = -1,
	None = 0,
	DesertEagle,
	Donut
};
}

namespace OtisHead
{
enum OtisHead
{
	Random = -1,
	Hair = 0,
	Balding
};
}

class COtis : public CTalkSquadMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  ISoundMask( void );
	void OtisFirePistol( void );
	void AlertSound( void );
	int  Classify ( void );
	const char* DisplayName();
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	
	void RunTask( Task_t *pTask );
	void StartTask( Task_t *pTask );
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	
	void DeclineFollowing( void );

	// Override these to set behavior
	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule ( void );
	MONSTERSTATE GetIdealState ( void );

	void DeathSound( void );
	void PainSound( void );
	
	void TalkInit( void );

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	void Killed( entvars_t *pevAttacker, int iGib );

	void KeyValue( KeyValueData* pkvd ) override;

	const char* GetDeathNoticeWeapon() { return "weapon_357"; }

	void ShuffleSoundArrays();
	void PlaySentence(const char* pszSentence, float duration, float volume, float attenuation);

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	BOOL	m_fGunDrawn;
	float	m_painTime;
	float	m_checkAttackTime;
	float   m_timefinishcheck;
	BOOL	m_lastAttackCheck;

	//These were originally used to store off the setting AND track state,
	//but state is now tracked by calling GetBodygroup
	int m_iOtisBody;
	int m_iOtisHead;

	// UNDONE: What is this for?  It isn't used?
	float	m_flPlayerDamage;// how much pain has the player inflicted on me?

	CUSTOM_SCHEDULES;

private:
	static const char* pPainSounds[];
	static const char* pDieSounds[];
	static const char* pAnswerSounds[];
	static const char* pQuestionSounds[];
	static const char* pIdleSounds[];
	static const char* pOkSounds[];
	static const char* pWaitSounds[];
	static const char* pScaredSounds[];
	static const char* pHelloSounds[];
	static const char* pSmellSounds[];
	static const char* pWoundSounds[];
	static const char* pMortalSounds[];
	static const char* pMadSounds[];
	static const char* pShotSounds[];
	static const char* pKillSounds[];
};

// TODO: is he supposed to be using barney sounds?
const char* COtis::pPainSounds[] =
{
	"barney/ba_pain1.wav",
	"barney/ba_pain2.wav",
	"barney/ba_pain3.wav",
};
const char* COtis::pDieSounds[] =
{
	"barney/ba_die1.wav",
	"barney/ba_die2.wav",
	"barney/ba_die3.wav",
};
const char* COtis::pAnswerSounds[] =
{
	"otis/yes.wav",
	"otis/no.wav",
	"otis/dontask.wav",
	"otis/yessir.wav",
	"otis/youbet.wav",
	"otis/alright.wav",
	"otis/yup.wav",
	"otis/yeah.wav",
	"otis/noway.wav",
	"otis/doubt.wav",
	"otis/breath.wav",
	"otis/dontguess.wav",
	"otis/nope.wav",
	"otis/nosir.wav",
	"otis/suppose.wav",
	"otis/hell.wav",
	"otis/cantfigure.wav",
	"otis/maybe.wav",
	"otis/talkmuch.wav",
	"otis/dejavu.wav",
};

const char* COtis::pQuestionSounds[] =
{
	"otis/beer.wav",
	"otis/rightdirection.wav",
	"otis/getanyworse.wav",
	"otis/gladof.wav",
	"otis/aliencombat.wav",
	"otis/somethingmoves.wav",
	"otis/noise.wav",
};

const char* COtis::pIdleSounds[] =
{
	"otis/mom.wav",
	"otis/aliens.wav",
	"otis/wuss.wav",
	"otis/bridge.wav",
	"otis/closet.wav",
	"otis/cousin.wav",
	"otis/mall.wav",
};

const char* COtis::pOkSounds[] =
{
	"otis/reputation.wav",
	"otis/live.wav",
	"otis/together.wav",
	"otis/letsgo.wav",
	"otis/diealone.wav",
	"otis/joinyou.wav",
	"otis/teamup.wav",
};

const char* COtis::pWaitSounds[] =
{
	"otis/help.wav",
	"otis/go_on.wav",
	"otis/slowingyoudown.wav",
	"otis/illwait.wav",
	"otis/seeya.wav",
	"otis/standguard.wav",
	"otis/notalone.wav",
};

const char* COtis::pScaredSounds[] =
{
	"otis/donthurtem.wav",
	"otis/leavealone.wav",
};

const char* COtis::pHelloSounds[] =
{
	"otis/hello.wav",
	"otis/hiya.wav",
	"otis/hey.wav",
	"otis/soldier.wav",
};

const char* COtis::pSmellSounds[] =
{
	"otis/ass.wav",
	"otis/chili.wav",
	"otis/janitor.wav",
};

const char* COtis::pWoundSounds[] =
{
	"otis/imhit.wav",
	"otis/virgin.wav",
};

const char* COtis::pMortalSounds[] =
{
	"otis/imdead.wav",
	"otis/hitbad.wav",
};

const char* COtis::pMadSounds[] =
{
	"otis/bully.wav",
	"otis/somuch.wav",
	"otis/dontmake.wav",
	"otis/tomb.wav",
};

const char* COtis::pShotSounds[] =
{
	"otis/watchit.wav",
	"otis/damn.wav",
	"otis/friends.wav",
	"otis/chump.wav",
	"otis/onry.wav",
};

const char* COtis::pKillSounds[] =
{
	"otis/gotone.wav",
	"otis/another.wav",
	"otis/buttugly.wav",
	"otis/seethat.wav",
	"otis/close.wav",
	"otis/firepl.wav",
	"otis/bring.wav",
};

LINK_ENTITY_TO_CLASS( monster_otis, COtis )

TYPEDESCRIPTION	COtis::m_SaveData[] = 
{
	DEFINE_FIELD( COtis, m_fGunDrawn, FIELD_BOOLEAN ),
	DEFINE_FIELD( COtis, m_painTime, FIELD_TIME ),
	DEFINE_FIELD( COtis, m_checkAttackTime, FIELD_TIME ),
	DEFINE_FIELD( COtis, m_lastAttackCheck, FIELD_BOOLEAN ),
	DEFINE_FIELD( COtis, m_flPlayerDamage, FIELD_FLOAT ),
	DEFINE_FIELD( COtis, m_iOtisBody, FIELD_INTEGER ),
	DEFINE_FIELD( COtis, m_iOtisHead, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( COtis, CTalkSquadMonster )

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Task_t	tlOtFollow[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE,(float)128		},	// Move within 128 of target ent (client)
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE },
};

Schedule_t	slOtFollow[] =
{
	{
		tlOtFollow,
		ARRAYSIZE ( tlOtFollow ),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"OTIS_FOLLOW"
	},
};

//=========================================================
// OtisDraw- much better looking draw schedule for when
// otis knows who he's gonna attack.
//=========================================================
Task_t	tlOtisEnemyDraw[] =
{
	{ TASK_STOP_MOVING,					0				},
	{ TASK_FACE_ENEMY,					0				},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float) ACT_ARM },
};

Schedule_t slOtisEnemyDraw[] = 
{
	{
		tlOtisEnemyDraw,
		ARRAYSIZE ( tlOtisEnemyDraw ),
		0,
		0,
		"OTIS_ENEMY_DRAW"
	}
};

Task_t	tlOtFaceTarget[] =
{
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_FACE_TARGET,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_CHASE },
};

Schedule_t	slOtFaceTarget[] =
{
	{
		tlOtFaceTarget,
		ARRAYSIZE ( tlOtFaceTarget ),
		bits_COND_CLIENT_PUSH	|
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"OTIS_FACE_TARGET"
	},
};


Task_t	tlIdleOtStand[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		}, // repick IDLESTAND every two seconds.
	{ TASK_TLK_HEADRESET,		(float)0		}, // reset head position
};

Schedule_t	slIdleOtStand[] =
{
	{ 
		tlIdleOtStand,
		ARRAYSIZE ( tlIdleOtStand ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND	|
		bits_COND_SMELL			|
		bits_COND_PROVOKED,

		bits_SOUND_COMBAT		|// sound flags - change these, and you'll break the talking code.
		//bits_SOUND_PLAYER		|
		//bits_SOUND_WORLD		|
		
		bits_SOUND_DANGER		|
		bits_SOUND_MEAT			|// scents
		bits_SOUND_CARCASS		|
		bits_SOUND_GARBAGE,
		"OTIS_IDLE_STAND"
	},
};

DEFINE_CUSTOM_SCHEDULES( COtis )
{
	slOtFollow,
	slOtisEnemyDraw,
	slOtFaceTarget,
	slIdleOtStand,
};


IMPLEMENT_CUSTOM_SCHEDULES( COtis, CTalkSquadMonster )

void COtis :: StartTask( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	
	case TASK_WAIT_FACE_ENEMY:
	{
		// need to override this to get the dynamic aiming time to work
		m_flWaitFinished = gpGlobals->time + reactiontim;
		break;
	}

	default:
		CTalkSquadMonster::StartTask( pTask );	
		break;
	
	}

}

void COtis :: RunTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		if (m_hEnemy != NULL && (m_hEnemy->IsPlayer()))
		{
			pev->framerate = 1.5;
		}
		CTalkSquadMonster::RunTask( pTask );
		break;
	default:
		CTalkSquadMonster::RunTask( pTask );
		break;
	}
}




//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
int COtis :: ISoundMask ( void) 
{
	//return	bits_SOUND_WORLD	|
			//bits_SOUND_COMBAT	|
			//bits_SOUND_CARCASS	|
			//bits_SOUND_MEAT		|
			//bits_SOUND_GARBAGE	|
			//bits_SOUND_DANGER	|
			//bits_SOUND_PLAYER;
	return bits_ALL_SOUNDS;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	COtis :: Classify ( void )
{
	// Is Player Ally? works inverted for friendly monsters
	if (!m_Classify && m_IsPlayerAlly)
		return CLASS_HUMAN_MILITARY;
	else
		return CBaseMonster::Classify(CLASS_PLAYER_ALLY);
}

const char* COtis::DisplayName() {
	return m_displayName ? CBaseMonster::DisplayName() : "Otis";
}

//=========================================================
// ALertSound - otis says "Freeze!"
//=========================================================
void COtis :: AlertSound( void )
{
	if ( m_hEnemy != NULL )
	{
		if ( FOkToSpeak() )
		{
			PlaySentence( "OT_ATTACK", RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE );
		}
	}

}
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void COtis :: SetYawSpeed ( void )
{
	int ys;

	ys = 0;

	switch ( m_Activity )
	{
	case ACT_IDLE:		
		ys = 70;
		break;
	case ACT_WALK:
		ys = 70;
		break;
	case ACT_RUN:
		ys = 90;
		break;
	default:
		ys = 70;
		break;
	}

	pev->yaw_speed = ys * gSkillData.sk_yawspeed_mult;
}


//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL COtis :: CheckRangeAttack1 ( float flDot, float flDist )
{
	distfactor = flDist / 2000;
	if (flDot >= 0.5 )
	{
		if ( gpGlobals->time > m_checkAttackTime )
		{
			TraceResult tr;
			
			Vector shootOrigin = pev->origin + Vector( 0, 0, 55 );
			CBaseEntity *pEnemy = m_hEnemy;
			Vector shootTarget = ((pEnemy->BodyTarget(shootOrigin) - pEnemy->pev->origin) + m_vecEnemyLKP);
			if (pEnemy->IsBSPModel()) {
				shootTarget = pEnemy->Center();
			}

			UTIL_TraceLine( shootOrigin, shootTarget, dont_ignore_monsters, ignore_glass, ENT(pev), &tr );
			m_checkAttackTime = gpGlobals->time + 1;
			if ( tr.flFraction == 1.0 || (tr.pHit != NULL && CBaseEntity::Instance(tr.pHit) == pEnemy) or CBaseEntity::Instance(tr.pHit)->pev->rendermode != kRenderNormal ) {
				m_lastAttackCheck = TRUE;
				m_timefinishcheck = gpGlobals->time;
			}
			else {
				m_lastAttackCheck = FALSE;
			}
			m_checkAttackTime = gpGlobals->time + 1.5;
		}
		return m_lastAttackCheck;
	}
	return FALSE;
}


//=========================================================
// OtisFirePistol - shoots one round from the pistol at
// the enemy otis is facing.
//=========================================================
void COtis :: OtisFirePistol ( void )
{
	reactiontim = RANDOM_FLOAT((distfactor*0.75), (distfactor*1.25));
	if (gpGlobals->time >= (m_timefinishcheck+reactiontim)) {
	Vector vecShootOrigin;

	UTIL_MakeVectors(pev->angles);
	vecShootOrigin = pev->origin + Vector( 0, 0, 55 );
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
	pev->effects = EF_MUZZLEFLASH;

	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_05DEGREES, 131072, gSkillData.sk_otis_bullet );
	
	int pitchShift = RANDOM_LONG( 0, 20 );
	
	// Only shift about half the time
	if ( pitchShift > 10 )
		pitchShift = 0;
	else
		pitchShift -= 5;
	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/de_shot1.wav", 1, ATTN_NORM, 0, 100 + pitchShift );

	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );

	// UNDONE: Reload?
	m_cAmmoLoaded--;// take away a bullet!
	}
}
		
//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void COtis :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case OTIS_AE_SHOOT:
		OtisFirePistol();
		break;

	case OTIS_AE_DRAW:
		// otis's bodygroup switches here so he can pull gun from holster
		if( GetBodygroup( OtisBodyGroup::Weapons ) == OtisWeapon::None )
		{
			SetBodygroup( OtisBodyGroup::Weapons, OtisWeapon::DesertEagle );
			m_iOtisBody = OtisWeapon::DesertEagle;
			m_fGunDrawn = true;
		}

		break;

	case OTIS_AE_HOLSTER:
		// change bodygroup to replace gun in holster
		if( GetBodygroup( OtisBodyGroup::Weapons ) == OtisWeapon::DesertEagle )
		{
			SetBodygroup( OtisBodyGroup::Weapons, OtisWeapon::None );
			m_iOtisBody = OtisWeapon::None;
			m_fGunDrawn = false;
		}
		break;

	default:
		CTalkSquadMonster::HandleAnimEvent( pEvent );
	}
}

//=========================================================
// Spawn
//=========================================================
void COtis :: Spawn()
{
	Precache( );

	m_friendlySkinFirst = true;
	InitModel();
	SetSize(VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->view_ofs		= Vector ( 0, 0, 50 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	//Note: This originally didn't use SetBodygroup
	if( m_iOtisHead == OtisHead::Random )
	{
		m_iOtisHead = RANDOM_LONG( 0, 1 );
	}

	if( m_iOtisBody == OtisWeapon::Random )
	{
		m_iOtisBody = OtisWeapon::None;
	}

	SetBodygroup( OtisBodyGroup::Weapons, m_iOtisBody );
	SetBodygroup( OtisBodyGroup::Heads, m_iOtisHead );

	m_fGunDrawn = m_iOtisBody == OtisWeapon::DesertEagle;

	MonsterInit();
	SetUse( &COtis::FollowerUse );
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void COtis :: Precache()
{
	// every new otis must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();

	CTalkSquadMonster::Precache();

	m_defaultModel = "models/otis.mdl";
	PRECACHE_MODEL(GetModel());

	ShuffleSoundArrays();

	PRECACHE_SOUND("weapons/de_shot1.wav" );

	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pDieSounds);

	int oldSoundVariety = soundvariety.value;
	soundvariety.value = soundvariety.value > 0 ? 1 : soundvariety.value;

	if (mp_npcidletalk.value) {
		PRECACHE_SOUND_ARRAY(pAnswerSounds);
		PRECACHE_SOUND_ARRAY(pQuestionSounds);
		PRECACHE_SOUND_ARRAY(pIdleSounds);
		PRECACHE_SOUND_ARRAY(pHelloSounds);
		PRECACHE_SOUND_ARRAY(pSmellSounds);
		PRECACHE_SOUND_ARRAY(pScaredSounds);
		PRECACHE_SOUND_ARRAY(pWoundSounds);
		PRECACHE_SOUND_ARRAY(pMortalSounds);
	}
	
	PRECACHE_SOUND_ARRAY(pMadSounds);
	PRECACHE_SOUND_ARRAY(pShotSounds);
	PRECACHE_SOUND_ARRAY(pKillSounds);

	// follow/unfollow sounds should have more variety because players activate them often
	soundvariety.value = (oldSoundVariety > 0 && oldSoundVariety > 3) ? 3 : soundvariety.value;
	PRECACHE_SOUND_ARRAY(pOkSounds);
	PRECACHE_SOUND_ARRAY(pWaitSounds);

	soundvariety.value = oldSoundVariety;

	canBeMadAtPlayer = true;
}	

// Init talk data
void COtis :: TalkInit()
{
	
	CTalkSquadMonster::TalkInit();

	// scientists speach group names (group names are in sentences.txt)

	m_szGrp[TLK_ANSWER]  =	"OT_ANSWER";
	m_szGrp[TLK_QUESTION] =	"OT_QUESTION";
	m_szGrp[TLK_IDLE] =		"OT_IDLE";
	m_szGrp[TLK_STARE] = NULL; // "OT_STARE";
	m_szGrp[TLK_USE] =		"OT_OK";
	m_szGrp[TLK_UNUSE] =	"OT_WAIT";
	m_szGrp[TLK_STOP] = NULL; // "OT_STOP";

	m_szGrp[TLK_NOSHOOT] =	"OT_SCARED";
	m_szGrp[TLK_HELLO] =	"OT_HELLO";

	m_szGrp[TLK_PLHURT1] = NULL; // "!BA_CUREA";
	m_szGrp[TLK_PLHURT2] = NULL; // "!BA_CUREB";
	m_szGrp[TLK_PLHURT3] = NULL; // "!BA_CUREC";

	m_szGrp[TLK_PHELLO] =	NULL;	//"OT_PHELLO";		// UNDONE
	m_szGrp[TLK_PIDLE] =	NULL;	//"OT_PIDLE";			// UNDONE
	m_szGrp[TLK_PQUESTION] = NULL; // "OT_PQUEST";		// UNDONE

	m_szGrp[TLK_SMELL] =	"OT_SMELL";
	
	m_szGrp[TLK_WOUND] =	"OT_WOUND";
	m_szGrp[TLK_MORTAL] =	"OT_MORTAL";

	// get voice for head - just one otis voice for now
	m_voicePitch = 100;

	AddPrecacheWeapon("weapon_357"); // weapon dropped on death
}

void COtis::ShuffleSoundArrays() {
	if (g_shuffledMonsterSounds.hasKey("otis")) {
		return;
	}
	g_shuffledMonsterSounds.put("otis");

	SHUFFLE_SOUND_ARRAY(pAnswerSounds);
	SHUFFLE_SOUND_ARRAY(pQuestionSounds);
	SHUFFLE_SOUND_ARRAY(pIdleSounds);
	SHUFFLE_SOUND_ARRAY(pOkSounds);
	SHUFFLE_SOUND_ARRAY(pWaitSounds);
	SHUFFLE_SOUND_ARRAY(pScaredSounds);
	SHUFFLE_SOUND_ARRAY(pHelloSounds);
	SHUFFLE_SOUND_ARRAY(pSmellSounds);
	SHUFFLE_SOUND_ARRAY(pWoundSounds);
	SHUFFLE_SOUND_ARRAY(pMortalSounds);
	SHUFFLE_SOUND_ARRAY(pMadSounds);
	SHUFFLE_SOUND_ARRAY(pShotSounds);
	SHUFFLE_SOUND_ARRAY(pKillSounds);
}

void COtis::PlaySentence(const char* pszSentence, float duration, float volume, float attenuation)
{
	if (!pszSentence)
		return;

	Talk(duration);

	CTalkSquadMonster::g_talkWaitTime = gpGlobals->time + duration + 2.0;
	
	const char* sample = "";

	int oldSoundVariety = soundvariety.value;
	soundvariety.value = soundvariety.value > 0 ? 1 : soundvariety.value;

	bool isIdleSound = false;

	PRECACHE_SOUND_ARRAY(pAnswerSounds);
	PRECACHE_SOUND_ARRAY(pQuestionSounds);
	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pHelloSounds);
	PRECACHE_SOUND_ARRAY(pSmellSounds);
	PRECACHE_SOUND_ARRAY(pScaredSounds);
	PRECACHE_SOUND_ARRAY(pWoundSounds);
	PRECACHE_SOUND_ARRAY(pMortalSounds);

	if (!strcmp(pszSentence, "OT_ANSWER")) {
		sample = RANDOM_SOUND_ARRAY(pAnswerSounds);
		isIdleSound = true;
	}
	else if (!strcmp(pszSentence, "OT_QUESTION")) {
		sample = RANDOM_SOUND_ARRAY(pQuestionSounds);
		isIdleSound = true;
	}
	else if (!strcmp(pszSentence, "OT_IDLE")) {
		sample = RANDOM_SOUND_ARRAY(pIdleSounds);
		isIdleSound = true;
	}
	else if (!strcmp(pszSentence, "OT_OK")) {
		soundvariety.value = (oldSoundVariety > 0 && oldSoundVariety > 2) ? 3 : soundvariety.value;
		sample = RANDOM_SOUND_ARRAY(pOkSounds);
	}
	else if (!strcmp(pszSentence, "OT_WAIT")) {
		soundvariety.value = (oldSoundVariety > 0 && oldSoundVariety > 2) ? 3 : soundvariety.value;
		sample = RANDOM_SOUND_ARRAY(pWaitSounds);
	}
	else if (!strcmp(pszSentence, "OT_SCARED")) {
		sample = RANDOM_SOUND_ARRAY(pScaredSounds);
		isIdleSound = true;
	}
	else if (!strcmp(pszSentence, "OT_HELLO")) {
		sample = RANDOM_SOUND_ARRAY(pHelloSounds);
		isIdleSound = true;
	}
	else if (!strcmp(pszSentence, "OT_SMELL")) {
		sample = RANDOM_SOUND_ARRAY(pSmellSounds);
		isIdleSound = true;
	}
	else if (!strcmp(pszSentence, "OT_WOUND")) {
		sample = RANDOM_SOUND_ARRAY(pWoundSounds);
		isIdleSound = true;
	}
	else if (!strcmp(pszSentence, "OT_MORTAL")) {
		sample = RANDOM_SOUND_ARRAY(pMortalSounds);
		isIdleSound = true;
	}
	else if (!strcmp(pszSentence, "OT_MAD")) {
		sample = RANDOM_SOUND_ARRAY(pMadSounds);
	}
	else if (!strcmp(pszSentence, "OT_SHOT")) {
		sample = RANDOM_SOUND_ARRAY(pShotSounds);
	}
	else if (!strcmp(pszSentence, "OT_KILL")) {
		sample = RANDOM_SOUND_ARRAY(pKillSounds);
	}
	else {
		ALERT(at_console, "Invalid sentence: %s\n", pszSentence);
		return;
	}

	soundvariety.value = oldSoundVariety;

	if (!isIdleSound || mp_npcidletalk.value)
		EMIT_SOUND_DYN(edict(), CHAN_VOICE, sample, volume, attenuation, 0, GetVoicePitch());

	// If you say anything, don't greet the player - you may have already spoken to them
	SetBits(m_bitsSaid, bit_saidHelloPlayer);
}

int COtis :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	if (IsImmune(pevAttacker, flDamage))
		return 0;

	// make sure friends talk about it if player hurts talkmonsters...
	int ret = CTalkSquadMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
	if ( !IsAlive() || pev->deadflag == DEAD_DYING )
		return ret;

	CBaseEntity* attacker = (CBaseEntity*)GET_PRIVATE(ENT(pevAttacker));

	if (m_MonsterState != MONSTERSTATE_PRONE && (pevAttacker->flags & FL_CLIENT) && attacker && IRelationship(attacker) == R_AL)
	{
		m_flPlayerDamage += flDamage;

		ALERT(at_console, "MY enemy is %s\n", m_hEnemy ? m_hEnemy->DisplayName() : "NULL");
		// This is a heurstic to determine if the player intended to harm me
		// If I have an enemy, we can't establish intent (may just be crossfire)
		if ( m_hEnemy == NULL )
		{
			// If the player was facing directly at me, or I'm already suspicious, get mad
			if ( (m_afMemory & bits_MEMORY_SUSPICIOUS) || IsFacing( pevAttacker, pev->origin ) )
			{
				// Alright, now I'm pissed!
				PlaySentence( "OT_MAD", 4, VOL_NORM, ATTN_NORM );

				Provoke(attacker);
			}
			else
			{
				// Hey, be careful with that
				PlaySentence( "OT_SHOT", 4, VOL_NORM, ATTN_NORM );
				Remember( bits_MEMORY_SUSPICIOUS );
			}
		}
		else if ( !(m_hEnemy->IsPlayer()) && pev->deadflag == DEAD_NO )
		{
			PlaySentence( "OT_SHOT", 4, VOL_NORM, ATTN_NORM );
		}
	}

	return ret;
}

	
//=========================================================
// PainSound
//=========================================================
void COtis :: PainSound ( void )
{
	if (gpGlobals->time < m_painTime)
		return;
	
	m_painTime = gpGlobals->time + RANDOM_FLOAT(0.5, 0.75);

	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pPainSounds), 1, ATTN_NORM, 0, GetVoicePitch());
}

//=========================================================
// DeathSound 
//=========================================================
void COtis :: DeathSound ( void )
{
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pDieSounds), 1, ATTN_NORM, 0, GetVoicePitch());
}


void COtis::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	switch( ptr->iHitgroup)
	{
	case HITGROUP_CHEST:
	case HITGROUP_STOMACH:
		if (bitsDamageType & (DMG_BULLET | DMG_SLASH))
		{
			flDamage = flDamage / 2;
		}
		break;
	default:
		break;
	}

	CTalkSquadMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


void COtis::Killed( entvars_t *pevAttacker, int iGib )
{
	if ( GetBodygroup( OtisBodyGroup::Weapons ) == OtisWeapon::DesertEagle && npc_dropweapons.value)
	{// drop the gun!
		Vector vecGunPos;
		Vector vecGunAngles;

		SetBodygroup( OtisBodyGroup::Weapons, OtisWeapon::None );
		m_iOtisBody = OtisWeapon::None;

		GetAttachment( 0, vecGunPos, vecGunAngles );
		
		DropItem( "weapon_eagle", vecGunPos, vecGunAngles );
	}

	SetUse( NULL );	
	CTalkSquadMonster::Killed( pevAttacker, iGib );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

Schedule_t* COtis :: GetScheduleOfType ( int Type )
{
	Schedule_t *psched;

	// react to player sounds when an enemy
	bool IsPlayerAlly = CBaseEntity::IRelationship(Classify(), CLASS_PLAYER) <= R_NO;

	switch( Type )
	{
	case SCHED_ARM_WEAPON:
		if ( m_hEnemy != NULL )
		{
			// face enemy, then draw.
			return slOtisEnemyDraw;
		}
		break;

	// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		// call base class default so that otis will talk
		// when 'used' 
		psched = CTalkSquadMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand && IsPlayerAlly)
			return slOtFaceTarget;	// override this for different target face behavior
		else
			return psched;

	case SCHED_TARGET_CHASE:
		return slOtFollow;

	case SCHED_IDLE_STAND:
		// call base class default so that scientist will talk
		// when standing during idle
		psched = CTalkSquadMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand && IsPlayerAlly)
		{
			// just look straight ahead.
			return slIdleOtStand;
		}
		else
			return psched;	
	case SCHED_RANGE_ATTACK1:
		reactiontim = RANDOM_FLOAT((distfactor*0.75), (distfactor*1.25));
		return &slRangeAttack1[0];
	case SCHED_RANGE_ATTACK2:
		reactiontim = RANDOM_FLOAT((distfactor*0.75), (distfactor*1.25));
		return &slRangeAttack2[0];
	}

	return CTalkSquadMonster::GetScheduleOfType( Type );
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *COtis :: GetSchedule ( void )
{
	if ( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );
		if ( pSound && (pSound->m_iType & bits_SOUND_DANGER) )
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
	}
	if ( HasConditions( bits_COND_ENEMY_DEAD ) && FOkToSpeak())
		PlaySentence( "OT_KILL", 4, VOL_NORM, ATTN_NORM );

	switch( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster :: GetSchedule();
			}

			// always act surprized with a new enemy
			if ( HasConditions( bits_COND_NEW_ENEMY ) && HasConditions( bits_COND_LIGHT_DAMAGE) )
				return GetScheduleOfType( SCHED_SMALL_FLINCH );
				
			// wait for one schedule to draw gun
			if (!m_fGunDrawn )
				return GetScheduleOfType( SCHED_ARM_WEAPON );

			if ( HasConditions( bits_COND_HEAVY_DAMAGE ) )
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
		}
		break;

	case MONSTERSTATE_ALERT:	
	case MONSTERSTATE_IDLE:
		if ( HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE))
		{
			// flinch if hurt
			return GetScheduleOfType( SCHED_SMALL_FLINCH );
		}

		if ( m_hEnemy == NULL && IsFollowing() )
		{
			if ( !m_hTargetEnt->IsAlive() )
			{
				// UNDONE: Comment about the recently dead player here?
				StopFollowing( FALSE );
				break;
			}
			else
			{
				if ( HasConditions( bits_COND_CLIENT_PUSH ) )
				{
					return GetScheduleOfType( SCHED_MOVE_AWAY_FOLLOW );
				}
				return GetScheduleOfType( SCHED_TARGET_FACE );
			}
		}

		if ( HasConditions( bits_COND_CLIENT_PUSH ) )
		{
			return GetScheduleOfType( SCHED_MOVE_AWAY );
		}

		// try to say something about smells
		TrySmellTalk();
		break;
	default:
		break;
	}
	
	return CBaseMonster::GetSchedule();
}

MONSTERSTATE COtis :: GetIdealState ( void )
{
	return CTalkSquadMonster::GetIdealState();
}



void COtis::DeclineFollowing( void )
{
	PlaySentence( "OT_POK", 2, VOL_NORM, ATTN_NORM );
}

void COtis::KeyValue( KeyValueData* pkvd )
{
	if( FStrEq( "head", pkvd->szKeyName ) )
	{
		m_iOtisHead = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( "bodystate", pkvd->szKeyName ) )
	{
		m_iOtisBody = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
	{
		CBaseMonster::KeyValue( pkvd );
	}
}



//=========================================================
// DEAD OTIS PROP
//
// Designer selects a pose in worldcraft, 0 through num_poses-1
// this value is added to what is selected as the 'first dead pose'
// among the monster's normal animations. All dead poses must
// appear sequentially in the model file. Be sure and set
// the m_iFirstPose properly!
//
//=========================================================
class CDeadOtis : public CBaseMonster
{
public:
	void Spawn( void );
	int	Classify ( void ) { return	CLASS_PLAYER_ALLY; }

	void KeyValue( KeyValueData *pkvd );

	int	m_iPose;// which sequence to display	-- temporary, don't need to save
	static const char *m_szPoses[5];
};

const char *CDeadOtis::m_szPoses[] = { "lying_on_back", "lying_on_side", "lying_on_stomach", "stuffed_in_vent", "dead_sitting" };

void CDeadOtis::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else 
		CBaseMonster::KeyValue( pkvd );
}

LINK_ENTITY_TO_CLASS( monster_otis_dead, CDeadOtis )

//=========================================================
// ********** DeadOtis SPAWN **********
//=========================================================
void CDeadOtis:: Spawn( )
{
	m_defaultModel = "models/otis.mdl";
	PRECACHE_MODEL(GetModel());
	InitModel();

	pev->effects		= 0;
	pev->yaw_speed		= 8;
	pev->sequence		= 0;
	m_bloodColor		= BLOOD_COLOR_RED;

	pev->sequence = LookupSequence( m_szPoses[m_iPose] );
	if (pev->sequence == -1)
	{
		ALERT ( at_console, "Dead otis with bad pose\n" );
	}
	// Corpses have less health
	pev->health			= 8;//gSkillData.otisHealth;

	MonsterInitDead();
}


