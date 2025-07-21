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
	float		flAdjustedDamage;
	Vector		vecSpot;


	int pointContents = UTIL_PointContents(vecSrc);
	int bInWater = (pointContents == CONTENTS_WATER);

	vecSrc.z += 1;// in case grenade is lying on the ground

	if (mp_explosionbug.value == 0 && UTIL_PointContents(vecSrc) != pointContents) {
		vecSrc.z -= 1; // must have hit a ceiling
	}

	if ( !pevAttacker )
		pevAttacker = pevInflictor;

	// iterate on all entities in the vicinity.
	while ((pEntity = UTIL_FindEntityInSphere( pEntity, vecSrc, 99999 )) != NULL)
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
					//tr.vecEndPos = vecSrc;
					tr.flFraction = 0.0;
				}
				
				// decrease damage for an ent that's farther from the bomb.
				float distance = ( vecSrc - tr.vecEndPos ).Length();
				if (distance < 0.001) {
					continue;
				}
				float odistance = distance;
				float drf = RANDOM_FLOAT(0.01, 99.99);

				distance = (((((-100) * odistance) / drf) / odistance) + 1) + odistance;

				if (distance < pow(odistance, 0.5)) {
					distance = pow(odistance, 0.5);
					
				}
				if (std::isnan(distance)) {
					continue;
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
					continue;
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

