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
#ifndef MONSTERS_H
#include "skill.h"
#define MONSTERS_H

#include "monster/CBaseMonster.h"

/*

===== monsters.h ========================================================

  Header file for monster-related utility code

*/

// CHECKLOCALMOVE result types 
#define	LOCALMOVE_INVALID					0 // move is not possible
#define LOCALMOVE_INVALID_DONT_TRIANGULATE	1 // move is not possible, don't try to triangulate
#define LOCALMOVE_VALID						2 // move is possible

// Hit Group standards
#define	HITGROUP_GENERIC	0
#define	HITGROUP_HEAD		1
#define	HITGROUP_CHEST		2
#define	HITGROUP_STOMACH	3
#define HITGROUP_LEFTARM	4	
#define HITGROUP_RIGHTARM	5
#define HITGROUP_LEFTLEG	6
#define HITGROUP_RIGHTLEG	7


// Monster Spawnflags
#define	SF_MONSTER_WAIT_TILL_SEEN		1// spawnflag that makes monsters wait until player can see them before attacking.
#define	SF_MONSTER_GAG					2 // no idle noises from this monster
#define SF_MONSTER_HITMONSTERCLIP		4
//										8
#define SF_MONSTER_PRISONER				16 // monster won't attack anyone, no one will attacke him.
//										32
//										64
#define	SF_MONSTER_WAIT_FOR_SCRIPT		128 //spawnflag that makes monsters wait to check for attacking until the script is done or they've been attacked
#define SF_MONSTER_PREDISASTER			256	//this is a predisaster scientist or barney. Influences how they speak.
#define SF_MONSTER_FADECORPSE			512 // Fade out corpse after death
#define SF_MONSTER_FALL_TO_GROUND		0x80000000

// specialty spawnflags
#define SF_MONSTER_TURRET_AUTOACTIVATE	32
#define SF_MONSTER_TURRET_STARTINACTIVE	64
#define SF_MONSTER_WAIT_UNTIL_PROVOKED	64 // don't attack the player unless provoked



// MoveToOrigin stuff
#define		MOVE_START_TURN_DIST	64 // when this far away from moveGoal, start turning to face next goal
#define		MOVE_STUCK_DIST			32 // if a monster can't step this far, it is stuck.


// MoveToOrigin stuff
#define		MOVE_NORMAL				0// normal move in the direction monster is facing
#define		MOVE_STRAFE				1// moves in direction specified, no matter which way monster is facing

// spawn flags 256 and above are already taken by the engine
EXPORT void UTIL_MoveToOrigin( edict_t* pent, const Vector &vecGoal, float flDist, int iMoveType );

EXPORT Vector VecCheckToss ( entvars_t *pev, const Vector &vecSpot1, Vector vecSpot2, float flGravityAdj = 1.0 );
EXPORT Vector VecCheckThrow ( entvars_t *pev, const Vector &vecSpot1, Vector vecSpot2, float flSpeed, float flGravityAdj = 1.0 );
EXPORT extern DLL_GLOBAL Vector		g_vecAttackDir;
EXPORT extern DLL_GLOBAL CONSTANT float g_flMeleeRange;
EXPORT extern DLL_GLOBAL CONSTANT float g_flMediumRange;
EXPORT extern DLL_GLOBAL CONSTANT float g_flLongRange;

EXPORT BOOL FBoxVisible ( entvars_t *pevLooker, entvars_t *pevTarget );
EXPORT BOOL FBoxVisible ( entvars_t *pevLooker, entvars_t *pevTarget, Vector &vecTargetOrigin, float flSize = 0.0 );

EXPORT BOOL IsFacing(entvars_t* pevTest, const Vector& reference);

// monster to monster relationship types
#define R_AL	-2 // (ALLY) pals. Good alternative to R_NO when applicable.
#define R_FR	-1// (FEAR)will run
#define	R_NO	0// (NO RELATIONSHIP) disregard
#define R_DL	1// (DISLIKE) will attack
#define R_HT	2// (HATE)will attack this character instead of any visible DISLIKEd characters
#define R_NM	3// (NEMESIS)  A monster Will ALWAYS attack its nemsis, no matter what


// these bits represent the monster's memory
#define MEMORY_CLEAR					0
#define bits_MEMORY_PROVOKED			( 1 << 0 )// right now only used for houndeyes.
#define bits_MEMORY_INCOVER				( 1 << 1 )// monster knows it is in a covered position.
#define bits_MEMORY_SUSPICIOUS			( 1 << 2 )// Ally is suspicious of the player, and will move to provoked more easily
#define bits_MEMORY_PATH_FINISHED		( 1 << 3 )// Finished monster path (just used by big momma for now)
#define bits_MEMORY_ON_PATH				( 1 << 4 )// Moving on a path
#define bits_MEMORY_MOVE_FAILED			( 1 << 5 )// Movement has already failed
#define bits_MEMORY_FLINCHED			( 1 << 6 )// Has already flinched
#define bits_MEMORY_KILLED				( 1 << 7 )// HACKHACK -- remember that I've already called my Killed()
#define bits_MEMORY_CUSTOM4				( 1 << 8 )	// Monster-specific memory
#define bits_MEMORY_CUSTOM3				( 1 << 9 )	// Monster-specific memory
#define bits_MEMORY_CUSTOM2				( 1 << 10 )	// Monster-specific memory
#define bits_MEMORY_CUSTOM1				( 1 << 11 )	// Monster-specific memory

// trigger conditions for scripted AI
// these MUST match the CHOICES interface in halflife.fgd for the base monster
enum 
{
	AITRIGGER_NONE = 0,
	AITRIGGER_SEEPLAYER_ANGRY_AT_PLAYER,
	AITRIGGER_TAKEDAMAGE,
	AITRIGGER_HALFHEALTH,
	AITRIGGER_DEATH,
	AITRIGGER_SQUADMEMBERDIE,
	AITRIGGER_SQUADLEADERDIE,
	AITRIGGER_HEARWORLD,
	AITRIGGER_HEARPLAYER,
	AITRIGGER_HEARCOMBAT,
	AITRIGGER_SEEPLAYER_UNCONDITIONAL,
	AITRIGGER_SEEPLAYER_NOT_IN_COMBAT,
};
/*
		0 : "No Trigger"
		1 : "See Player"
		2 : "Take Damage"
		3 : "50% Health Remaining"
		4 : "Death"
		5 : "Squad Member Dead"
		6 : "Squad Leader Dead"
		7 : "Hear World"
		8 : "Hear Player"
		9 : "Hear Combat"
*/

#define CUSTOM_SCHEDULES\
		virtual Schedule_t *ScheduleFromName( const char *pName );\
		virtual Schedule_t* ScheduleFromTableIdx(uint32_t idx); \
		virtual int GetScheduleTableSize(); \
		virtual int GetScheduleTableIdx(); \
		virtual void GetAllSchedules( std::unordered_set<Schedule_t*>& schedulesOut ); \
		static Schedule_t *m_scheduleList[]

#define DEFINE_CUSTOM_SCHEDULES(derivedClass)\
	Schedule_t *derivedClass::m_scheduleList[] =

#define IMPLEMENT_CUSTOM_SCHEDULES(derivedClass, baseClass)\
		Schedule_t *derivedClass::ScheduleFromName( const char *pName )\
		{\
			Schedule_t *pSchedule = ScheduleInList( pName, m_scheduleList, ARRAYSIZE(m_scheduleList) );\
			if ( !pSchedule )\
				return baseClass::ScheduleFromName(pName);\
			return pSchedule;\
		} \
		Schedule_t* derivedClass::ScheduleFromTableIdx(uint32_t idx) { \
			Schedule_t* baseSched = baseClass::ScheduleFromTableIdx(idx); \
			idx -= baseClass::GetScheduleTableSize(); \
			return (!baseSched && idx < ARRAYSIZE(m_scheduleList)) ? m_scheduleList[idx] : baseSched; \
		} \
		int derivedClass::GetScheduleTableSize() { \
			return baseClass::GetScheduleTableSize() + ARRAYSIZE(m_scheduleList); \
		} \
		int derivedClass::GetScheduleTableIdx() { \
			int baseIdx = baseClass::GetScheduleTableIdx(); \
			if (baseIdx != -1) { \
				return baseIdx; \
			} \
			for (int i = 0; i < (int)ARRAYSIZE(m_scheduleList); i++) { \
				if (m_scheduleList[i] == m_pSchedule) { \
					return baseClass::GetScheduleTableSize() + i; \
				} \
			} \
			return -1; \
		} \
		void derivedClass::GetAllSchedules(std::unordered_set<Schedule_t*>& schedulesOut) { \
			baseClass::GetAllSchedules(schedulesOut); \
			for (int i = 0; i < (int)ARRAYSIZE(m_scheduleList); i++) { \
				schedulesOut.insert(m_scheduleList[i]); \
			} \
		} \



#endif	//MONSTERS_H
