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

#define OBIT_ICON_HEIGHT 35
#define OBIT_ICON_WIDTH 40

#define OBIT_SPACING 45
#define OBIT_GAP_WIDTH 15

typedef struct {
	char	attacker[OBIT_MAX_NAME_LENGTH];
	char	target[OBIT_MAX_NAME_LENGTH];
	qhandle_t icon;
	int		time;
} obituary_t;

static int numObits;
static obituary_t obitStack[OBIT_MAX_VISABLE];

/*
===================
CG_MoveObitInStack
===================
*/
static void CG_MoveObitInStack( int obit, int move ) {
	if ( obit + move < OBIT_MAX_VISABLE )
		memcpy( &obitStack[obit+move], &obitStack[obit], sizeof(obituary_t) );
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
CG_ClearObit
===================
*/
static void CG_ClearObit( int obit ) {
	if ( obit >= 0 && obit < OBIT_MAX_VISABLE )
		memset( &obitStack[obit], 0, sizeof(obituary_t) );
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
			CG_DrawString( x, y, obitStack[i].attacker, UI_DROPSHADOW | UI_SMALLFONT, color );
			x += CG_DrawStrlen( obitStack[i].attacker, UI_SMALLFONT );
			x += OBIT_GAP_WIDTH;
		}

		CG_DrawPic( x, y, OBIT_ICON_WIDTH, OBIT_ICON_HEIGHT, obitStack[i].icon );

		x += OBIT_ICON_WIDTH;
		x += OBIT_GAP_WIDTH;

		CG_DrawString( x, y, obitStack[i].target, UI_DROPSHADOW | UI_SMALLFONT, color );

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

	for (i = OBIT_MAX_VISABLE; i >= 0; i-- ) {
		if ( obitStack[i].target[0] != '\0' ) {
			if ( i == 5 ) {
				CG_ClearObit( i );
				continue;
			}

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
static void CG_AddObituary( char *attackerName, char *targetName, char *icon, qboolean weapon ) {
	if ( numObits >= OBIT_MAX_VISABLE ) {
		CG_ShiftObituaryStack();
		CG_CountObits();
	}

	if ( !targetName ) {
		Com_Printf( "CG_AddObituary: called with NULL targetName!\n");
		return;
	}

	CG_ShiftObituaryStack();

	obitStack[0].icon = trap_R_RegisterShader( weapon ? icon : va("icons/obituary/%s", icon) );

	if ( !obitStack[0].icon )
		obitStack[0].icon = trap_R_RegisterShader( "icons/obituary/test" );

	if ( attackerName )
		Q_strncpyz( obitStack[0].attacker, attackerName, sizeof(obitStack[0].attacker) );

	Q_strncpyz( obitStack[0].target, targetName, sizeof(obitStack[0].target) );

	obitStack[0].time = cg.time;

	CG_CountObits();
}

typedef struct {
	int mod;
	char *icon;
} obitIcon_t;

static obitIcon_t obitIcons[] = {
	{ -1, "headshot" },
	{ MOD_SUICIDE, "suicide" },
	{ MOD_ASSISTED_SUICIDE, "assist_suicide" },
	{ MOD_FALLING, "fall" },
	{ MOD_CRUSH, "crush" },
	{ MOD_WATER, "water" },
	{ MOD_SLIME, "slime" },
	{ MOD_LAVA, "lava" },
	{ MOD_TARGET_LASER, "target_laser" },
	{ MOD_TRIGGER_HURT, "trigger_hurt" },
	{ MOD_TELEFRAG, "telefrag" },
	// weapons
	{ MOD_GAUNTLET, "gauntlet" },
	{ MOD_MACHINEGUN, "machinegun" },
	{ MOD_SHOTGUN, "shotgun" },
	{ MOD_GRENADE, "grenade" },
	{ MOD_GRENADE_SPLASH, "grenade_splash" },
	{ MOD_ROCKET, "rocket" },
	{ MOD_ROCKET_SPLASH, "rocket_splash" },
	{ MOD_PLASMA, "plasma" },
	{ MOD_PLASMA_SPLASH, "plasma_splash" },
	{ MOD_LIGHTNING, "lightning" },
	{ MOD_RAILGUN, "railgun" },
	{ MOD_BFG, "bfg" },
	{ MOD_BFG_SPLASH, "bfg_splash" }
};

static int obitIconLen = ARRAY_LEN(obitIcons);

/*
===================
CG_ObitIcon
===================
*/
static char *CG_ObitIcon( int mod ) {
	int i;

	for ( i = 0; i < obitIconLen; i++ ) {
		if ( obitIcons[i].mod == mod )
			return obitIcons[i].icon;
	}

	return NULL;
}

/*
===================
CG_ParseObituary
===================
*/
void CG_ParseObituary( entityState_t *ent ) {
	int 	mod;
	int 	target, attacker;
	char 	*icon;
	const char 	*targetInfo;
	const char 	*attackerInfo;
	char 	targetName[OBIT_MAX_NAME_LENGTH];
	char 	attackerName[OBIT_MAX_NAME_LENGTH];
//	playerInfo_t	*pi;
	int i;
	qboolean headshot;

	target = ent->otherEntityNum;
	attacker = ent->otherEntityNum2;
	mod = ent->eventParm;
	headshot = ent->eFlags & EF_HEADSHOT;

	if ( target < 0 || target >= MAX_CLIENTS ) {
		Com_Printf( "CG_ParseObituary: target out of range!\n" );
		return;
	}

//	pi = &cgs.playerinfo[target];

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

	if ( headshot ) icon = CG_ObitIcon( -1 );
	else icon = CG_ObitIcon( mod );

	if ( (attacker == ENTITYNUM_WORLD) || (attacker == target) ) {
		CG_AddObituary( NULL, targetName, icon, qfalse );
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
		CG_AddObituary( attackerName, targetName, icon, qtrue );
		return;
	}

	CG_AddObituary( NULL, targetName, "died", qfalse );
}
