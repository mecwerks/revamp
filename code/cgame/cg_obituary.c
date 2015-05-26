/*
===========================================================================
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of Spearmint Source Code.

Spearmint Source Code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

Spearmint Source Code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Spearmint Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, Spearmint Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following
the terms and conditions of the GNU General Public License.  If not, please
request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional
terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc.,
Suite 120, Rockville, Maryland 20850 USA.
===========================================================================
*/

// cg_obituary.c -- new obituary feed

#include "cg_local.h"

#define OBIT_MAX_NAME_LENGTH 28

#define OBIT_MAX_VISABLE 5

#define OBIT_FADE_TIME 5000

#define OBIT_POS_X 80
#define OBIT_POS_Y 320

#define OBIT_ICON_HEIGHT 32
#define OBIT_ICON_WIDTH 64

#define OBIT_SPACING 30
#define OBIT_GAP_WIDTH 10

static int fontFlags = (UI_DROPSHADOW | UI_TINYFONT);

typedef struct {
	char	attacker[OBIT_MAX_NAME_LENGTH];
	char	target[OBIT_MAX_NAME_LENGTH];
	int 	modelID;
	int		time;
} obituary_t;

static int numObits;
static obituary_t obitStack[OBIT_MAX_VISABLE];

typedef enum {
	MID_DEFAULT,
	MID_GAUNTLET,
	MID_MACHINEGUN,
	MID_SHOTGUN,
	MID_LIGHTNING,
	MID_PLASMAGUN,
	MID_GRENADEL,
	MID_ROCKETL,
	MID_RAILGUN,
	MID_BFG
} modelID_t;

typedef qhandle_t modelHandles_t[2];

/*
===================
CG_ModelNames
===================
*/
static void CG_ModelHandles( modelHandles_t *handles, modelID_t modelID ) {
	weaponInfo_t *weapon = NULL;
	int weaponNum = WP_NONE;

	handles[0][0] = 0;
	handles[0][1] = 0;

	switch ( modelID ) {
		case MID_GAUNTLET:
			weaponNum = WP_GAUNTLET;
			break;

		case MID_MACHINEGUN:
			weaponNum = WP_MACHINEGUN;
			break;

		case MID_SHOTGUN:
			weaponNum = WP_SHOTGUN;
			break;

		case MID_LIGHTNING:
			weaponNum = WP_LIGHTNING;
			break;

		case MID_PLASMAGUN:
			weaponNum = WP_PLASMAGUN;
			break;

		case MID_GRENADEL:
			weaponNum = WP_GRENADE_LAUNCHER;
			break;

		case MID_ROCKETL:
			weaponNum = WP_ROCKET_LAUNCHER;
			break;

		case MID_RAILGUN:
			weaponNum = WP_RAILGUN;
			break;

		case MID_BFG:
			weaponNum = WP_BFG;
			break;

		case MID_DEFAULT:
		default:
			break;
	}

	if ( weaponNum != WP_NONE ) {
		weapon = &cg_weapons[weaponNum];

		handles[0][0] = weapon->weaponModel;
		if ( weapon->barrelModel )
			handles[0][1] = weapon->barrelModel;
	}
}

/*
===================
CG_ClearObit
===================
*/
static void CG_ClearObit( int obit ) {
	if ( obit >= 0 && obit < OBIT_MAX_VISABLE )
		memset( &obitStack[obit], 0, sizeof(obituary_t) );
}

/*
===================
CG_MoveObitInStack
===================
*/
static void CG_MoveObitInStack( int obit, int move ) {
	if ( obit + move < OBIT_MAX_VISABLE )
		memcpy( &obitStack[obit+move], &obitStack[obit], sizeof(obituary_t) );

	CG_ClearObit( obit );
}

/*
===================
CG_CountObits
===================
*/
static void CG_CountObits( void ) {
	int i, count = 0;

#if 1
	for (i = 0; i < OBIT_MAX_VISABLE; i++) {
		if (obitStack[i].target[0] != '\0')
			count++;
	}

	numObits = count;
#else
	int numEmpty = 0, lastFound = -1;

	for (i = OBIT_MAX_VISABLE; i >= 0; i--) {
		if (obitStack[i].target) {
			if (numEmpty)
				CG_MoveObitInStack( lastFound, -(numEmpty) );
			else
				lastFound = i;
		} else {
			numEmpty++;
		}
	}
#endif
}

/*
===================
CG_DrawObituary
===================
*/
void CG_DrawObituary( void ) {
	int i;
	float *color;
	float x, y;
	modelHandles_t modelHandles = {0, 0};
	vec3_t angles, origin;

	CG_SetScreenPlacement( PLACE_LEFT, PLACE_BOTTOM );

	for ( i = 0; i < OBIT_MAX_VISABLE; i++ ) {
		if ( obitStack[i].target[0] == '\0' )
			continue;

		color = CG_FadeColor( obitStack[i].time, OBIT_FADE_TIME );

		if ( !color ) {
			CG_ClearObit( i );
			CG_CountObits();
			continue;
		}

		trap_R_SetColor( color );

		y = OBIT_POS_Y - ( i * OBIT_SPACING );
		x = OBIT_POS_X;

		if ( obitStack[i].attacker[0] != '\0' ) {
			CG_DrawString( x, y, obitStack[i].attacker, fontFlags, color );
			x += CG_DrawStrlen( obitStack[i].attacker, fontFlags );
			x += OBIT_GAP_WIDTH;
		}

		CG_ModelHandles( &modelHandles, obitStack[i].modelID );

		angles[YAW] = 90;
		origin[0] = 70;
		origin[1] = 0;
		origin[2] = 0;

		if (modelHandles[1]) {
			CG_Draw3DModel( x, y-10, OBIT_ICON_WIDTH, OBIT_ICON_HEIGHT, modelHandles[1], NULL, origin, angles );
			if ( modelHandles[1] == cg_weapons[WP_MACHINEGUN].barrelModel )
				x += 15;
		}

		if (modelHandles[0]) {
			CG_Draw3DModel( x, y-10, OBIT_ICON_WIDTH, OBIT_ICON_HEIGHT, modelHandles[0], NULL, origin, angles );
		}

		x += OBIT_ICON_WIDTH-15;
		x += OBIT_GAP_WIDTH;

		CG_DrawString( x, y, obitStack[i].target, fontFlags, color );

		trap_R_SetColor( NULL );
	}
}

/*
===================
CG_ShiftObituaryStack
===================
*/
static void CG_ShiftObituaryStack( void ) {
	int i;

	for (i = OBIT_MAX_VISABLE-1; i >= 0; i-- ) {
		if ( obitStack[i].target[0] != '\0' ) {
			CG_MoveObitInStack( i, 1 );
		}
	}

	CG_ClearObit( 0 );
}

/*
===================
CG_AddObituary
===================
*/
static void CG_AddObituary( char *attackerName, char *targetName, int modelID ) {
	if ( numObits >= OBIT_MAX_VISABLE ) {
		CG_ShiftObituaryStack();
		CG_CountObits();
	}

	if ( !targetName ) {
		Com_Printf( "CG_AddObituary: called with NULL targetName!\n");
		return;
	}

	CG_ShiftObituaryStack();
	CG_CountObits();

	obitStack[0].modelID = modelID;

	if ( attackerName )
		Q_strncpyz( obitStack[0].attacker, attackerName, sizeof(obitStack[0].attacker) );

	Q_strncpyz( obitStack[0].target, targetName, sizeof(obitStack[0].target) );

	obitStack[0].time = cg.time;
}

/*
===================
CG_ObitModelID
===================
*/
static int CG_ObitModelID( int meansOfDeath ) {
	switch ( meansOfDeath ) {
		case MOD_SUICIDE:
		case MOD_ASSISTED_SUICIDE:
		case MOD_TRIGGER_HURT:
		case MOD_TARGET_LASER:
		case MOD_CRUSH:
		case MOD_TELEFRAG:
		case MOD_WATER:
		case MOD_SLIME:
		case MOD_LAVA:
		case MOD_FALLING:
		case MOD_GRAPPLE:
		default:
			return MID_DEFAULT;

		case MOD_GAUNTLET:
			return MID_GAUNTLET;

		case MOD_MACHINEGUN:
			return MID_MACHINEGUN;
		
		case MOD_SHOTGUN:
			return MID_SHOTGUN;

		case MOD_GRENADE:
		case MOD_GRENADE_SPLASH:
			return MID_GRENADEL;

		case MOD_ROCKET:
		case MOD_ROCKET_SPLASH:
			return MID_ROCKETL;

		case MOD_LIGHTNING:
			return MID_LIGHTNING;

		case MOD_PLASMA:
		case MOD_PLASMA_SPLASH:
			return MID_PLASMAGUN;

		case MOD_RAILGUN:
			return MID_RAILGUN;

		case MOD_BFG:
		case MOD_BFG_SPLASH:
			return MID_BFG;
	}
}

/*
===================
CG_ParseObituary
===================
*/
void CG_ParseObituary( entityState_t *ent ) {
	int 	mod;
	int 	target, attacker;
	const char 	*targetInfo;
	const char 	*attackerInfo;
	char 	targetName[OBIT_MAX_NAME_LENGTH];
	char 	attackerName[OBIT_MAX_NAME_LENGTH];
	int i;

	target = ent->otherEntityNum;
	attacker = ent->otherEntityNum2;
	mod = ent->eventParm;

	if ( target < 0 || target >= MAX_CLIENTS ) {
		Com_Printf( "CG_ParseObituary: target out of range!\n" );
		return;
	}

	if ( attacker < 0 || attacker >= MAX_CLIENTS ) {
		attacker = ENTITYNUM_WORLD;
		attackerInfo = NULL;
	} else {
		attackerInfo = CG_ConfigString( CS_PLAYERS + attacker );
	}

	targetInfo = CG_ConfigString( CS_PLAYERS + target );

	if ( !targetInfo ) {
		Com_Printf( "CG_ParseObituary: could not get target info!\n" );
		return;
	}

	Q_strncpyz( targetName, Info_ValueForKey( targetInfo, "n" ), sizeof(targetName) );
	strcat( targetName, S_COLOR_WHITE );

	if ( (attacker == ENTITYNUM_WORLD) || (attacker == target) ) {
		CG_AddObituary( NULL, targetName, MID_DEFAULT );
		return;
	}

	if ( CG_LocalPlayerState(attacker) ) {
		char *s;
		playerState_t *ps;

		for ( i = 0; i < CG_MaxSplitView(); i++ ) {
			if ( attacker != cg.snap->pss[i].playerNum )
				continue;

			ps = &cg.snap->pss[i];

			if ( cgs.gametype < GT_TEAM ) {
				s = va("You killed %s\n%s place with %i", targetName,
						CG_PlaceString( ps->persistant[PERS_RANK] + 1 ),
						ps->persistant[PERS_SCORE]);
			} else {
				s = va("You killed %s", targetName);
			}

			CG_CenterPrint( i, s, SCREEN_HEIGHT * 0.30, 0.5 );
		}
	}

	if ( !attackerInfo ) {
		attacker = ENTITYNUM_WORLD;
		strcpy( attackerName, "noname" );
	} else {
		Q_strncpyz( attackerName, Info_ValueForKey( attackerInfo, "n" ), sizeof(attackerName) );
		strcat( attackerName, S_COLOR_WHITE );

		for ( i = 0; i < CG_MaxSplitView(); i++ ) {
			if ( target == cg.snap->pss[i].playerNum )
				Q_strncpyz( cg.localPlayers[i].killerName, attackerName, sizeof(cg.localPlayers[i].killerName) );
		}
	}

	if ( attacker != ENTITYNUM_WORLD ) {
		CG_AddObituary( attackerName, targetName, CG_ObitModelID(mod) );
		return;
	}

	CG_AddObituary( NULL, targetName, MID_DEFAULT );
}
