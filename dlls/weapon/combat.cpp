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
/*

===== combat.cpp ========================================================

  functions dealing with damage infliction & death

*/

#include "extdll.h"
#include "util.h"
#include "env/CSoundEnt.h"
#include "decals.h"
#include "animation.h"
#include "weapons.h"
#include "CBreakable.h"

extern Vector VecBModelOrigin( entvars_t* pevBModel );
//
// RadiusDamage - this entity is exploding, or otherwise needs to inflict damage upon entities within a certain range.
// 
// only damage ents that can clearly be seen by the explosion!
void RadiusDamage( Vector vecSrc, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, float flRadius, int iClassIgnore, int bitsDamageType )
{
	CBaseEntity *pEntity = NULL;
	TraceResult	tr;
	float		flAdjustedDamage, falloff;
	Vector		vecSpot;

	if ( flRadius )
		falloff = flDamage / flRadius;
	else
		falloff = 1.0;

	int bInWater = (UTIL_PointContents ( vecSrc ) == CONTENTS_WATER);

	vecSrc.z += 1;// in case grenade is lying on the ground

	if ( !pevAttacker )
		pevAttacker = pevInflictor;

	// iterate on all entities in the vicinity.
	while ((pEntity = UTIL_FindEntityInSphere( pEntity, vecSrc, 3850 )) != NULL)
	{
		if ( pEntity->pev->takedamage != DAMAGE_NO )
		{
			// UNDONE: this should check a damage mask, not an ignore
			if ( iClassIgnore != CLASS_NONE && pEntity->Classify() == iClassIgnore )
			{// houndeyes don't hurt other houndeyes with their attack
				continue;
			}

			// blast's don't tavel into or out of water
			if (bInWater && pEntity->pev->waterlevel == 0)
				continue;
			if (!bInWater && pEntity->pev->waterlevel == 3)
				continue;

			vecSpot = pEntity->BodyTarget( vecSrc );
			
			UTIL_TraceLine ( vecSrc, vecSpot, dont_ignore_monsters, ENT(pevInflictor), &tr );

			if ( tr.flFraction == 1.0 || tr.pHit == pEntity->edict() )
			{// the explosion can 'see' this entity, so hurt them!
				if (tr.fStartSolid)
				{
					// if we're stuck inside them, fixup the position and distance
					tr.vecEndPos = vecSrc;
					tr.flFraction = 0.0;
				}
				
				// decrease damage for an ent that's farther from the bomb.
				float distance = ( vecSrc - tr.vecEndPos ).Length();
				if (distance < 1) {
					distance = 1;
				}
				float drf = RANDOM_FLOAT(0.01, 199.99);
				if (drf >= 100) {
					distance = (((((-100) * distance) / drf) / distance) + 1) + distance;
				}
				else {
					distance = (((((-1 * distance) / 10) / (drf - 200)) * distance) - ((pow(distance, 2))/1000)) + distance;
				}
				if (distance < 1) {
					distance = 1;
				}
				if (std::isnan(distance)) {
					distance = 1;
				}
				flAdjustedDamage = (flDamage/(distance * 0.00318198051534)) - (distance * 0.00795495128835);
				//flAdjustedDamage = flDamage - flAdjustedDamage;
			
				if ( flAdjustedDamage < 0 )
				{
					flAdjustedDamage = 0;
				}
				if (flAdjustedDamage > 2*flDamage) {
					flAdjustedDamage = 2*flDamage;
				}
				if (std::isnan(flAdjustedDamage)) {
					flAdjustedDamage = 2*flDamage;
				}
			
				// ALERT( at_console, "hit %s\n", STRING( pEntity->pev->classname ) );
				if (flAdjustedDamage != 0) {
				if (tr.flFraction != 1.0)
				{
					ClearMultiDamage( );
					pEntity->TraceAttack( pevInflictor, flAdjustedDamage, (tr.vecEndPos - vecSrc).Normalize( ), &tr, bitsDamageType );
					ApplyMultiDamage( pevInflictor, pevAttacker );
				}
				else
				{
					pEntity->TakeDamage ( pevInflictor, pevAttacker, flAdjustedDamage, bitsDamageType );
				}
				}
			}
		}
	}
}

