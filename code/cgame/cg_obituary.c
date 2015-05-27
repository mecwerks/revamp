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

#include "cg_obituary.h"

static obituary_t obitStack[OBIT_MAX_VISABLE];
static int numObits;

static modelInfo_t modelInfo[MAX_MODEL_INFO];
static int numModelInfo;

static qboolean obitInit = qfalse;

/*
===================
CG_RegisterModelInfo
===================
*/
static modelInfo_t *CG_RegisterModelInfo( int mod, qhandle_t models[2], vec3_t spacing, vec3_t origin, vec3_t angles ) {
	modelInfo_t *mi;

	if ( numModelInfo >= MAX_MODEL_INFO )
		return NULL;

	mi = &modelInfo[numModelInfo];
	mi->mod = mod;
	mi->handles[0] = models[0];
	mi->handles[1] = models[1];
	mi->spacing[0] = spacing[0];
	mi->spacing[1] = spacing[1];
	mi->spacing[2] = spacing[2];
	mi->origin[0] = origin[0];
	mi->origin[1] = origin[1];
	mi->origin[2] = origin[2];
	mi->angles[0] = angles[0];
	mi->angles[1] = angles[1];
	mi->angles[2] = angles[2];

	numModelInfo++;

	//CG_Printf("Registered mod %d\n", mod);
	return mi;
}

/*
===================
CG_GetModelInfo
===================
*/
static modelInfo_t *CG_GetModelInfo ( int mod ) {
	int i;

	for ( i = 0; i < numModelInfo; i++ ) {
		if ( modelInfo[i].mod == mod ) {
			//CG_Printf("Found index %d for mod %d\n", i, mod);
			return &modelInfo[i];
		}
	}

	return NULL;
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

	for (i = 0; i < OBIT_MAX_VISABLE; i++) {
		if (obitStack[i].target[0] != '\0')
			count++;
	}

	numObits = count;
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
static void CG_AddObituary( char *attackerName, char *targetName, team_t attackerTeam, team_t targetTeam, int mod ) {
	modelInfo_t *mi = NULL;

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

	mi = CG_GetModelInfo( mod );

	if ( mi != NULL ) {
		obitStack[0].mi = mi;
	} else {
		int handles[2] = { 0, 0 };
		vec3_t origin = { 70, 0, 0 };
		vec3_t angles = { 0, 0, 0 };
		vec3_t spacing = { 0, 0, 0 };
		qboolean notFound = qfalse;

		switch ( mod ) {
			case MOD_GAUNTLET:
				handles[0] = cg_weapons[WP_GAUNTLET].weaponModel;
				handles[1] = cg_weapons[WP_GAUNTLET].barrelModel;
				angles[YAW] = 270;
				spacing[0] = -4;
				spacing[1] = -14;
				break;
			
			case MOD_MACHINEGUN:
				handles[0] = cg_weapons[WP_MACHINEGUN].weaponModel;
				handles[1] = cg_weapons[WP_MACHINEGUN].barrelModel;
				angles[YAW] = 270;
				spacing[0] = -1;
				spacing[1] = 14;
				spacing[2] = -13;
				break;
			
			case MOD_SHOTGUN:
				handles[0] = cg_weapons[WP_SHOTGUN].weaponModel;
				angles[YAW] = 270;
				spacing[0] = -6;
				spacing[1] = 6;
				break;
			
			case MOD_GRENADE:
			case MOD_GRENADE_SPLASH:
				handles[0] = cg_weapons[WP_GRENADE_LAUNCHER].weaponModel;
				angles[YAW] = 270;
				spacing[0] = -15;
				break;
			
			case MOD_ROCKET:
			case MOD_ROCKET_SPLASH:
				handles[0] = cg_weapons[WP_ROCKET_LAUNCHER].weaponModel;
				angles[YAW] = 270;
				spacing[0] = -13;
				spacing[1] = 6;
				break;
			
			case MOD_LIGHTNING:
				handles[0] = cg_weapons[WP_LIGHTNING].weaponModel;
				angles[YAW] = 270;
				spacing[0] = -15;
				spacing[1] = 5;
				break;
			
			case MOD_PLASMA:
			case MOD_PLASMA_SPLASH:
				handles[0] = cg_weapons[WP_PLASMAGUN].weaponModel;
				angles[YAW] = 270;
				spacing[0] = -15;
				spacing[1] = 5;
				break;
			
			case MOD_RAILGUN:
				handles[0] = cg_weapons[WP_RAILGUN].weaponModel;
				angles[YAW] = 270;
				break;

			case MOD_BFG:
			case MOD_BFG_SPLASH:
				handles[0] = cg_weapons[WP_BFG].weaponModel;
				handles[1] = cg_weapons[WP_BFG].barrelModel;
				angles[YAW] = 270;
				spacing[0] = -15;
				spacing[1] = 5;
				break;
			
			case MI_HEADSHOT:
				handles[0] = cgs.media.gibBrain;
				angles[YAW] = 270;
				spacing[0] = -20;
				spacing[1] = -20;
				break;

			case MI_SAMETEAM:
			case MI_NONE:
			case MOD_UNKNOWN:
			default:
				notFound = qtrue;
				break;
		}

		if ( notFound )
			obitStack[0].mi = CG_GetModelInfo( MI_NONE );
		else
			obitStack[0].mi = CG_RegisterModelInfo( mod, handles, spacing, origin, angles );
	}

	if ( attackerName )
		Q_strncpyz( obitStack[0].attacker, attackerName, sizeof(obitStack[0].attacker) );

	Q_strncpyz( obitStack[0].target, targetName, sizeof(obitStack[0].target) );

	obitStack[0].attackerTeam = attackerTeam;
	obitStack[0].targetTeam = targetTeam;

	obitStack[0].time = cg.time;
}

/*
===================
CG_InitObituary
===================
*/
static void CG_InitObituary( void ) {
	qhandle_t handles[2];
	vec3_t origin = {70, 0, 0};
	vec3_t angles = {0, 180, 0};
	vec3_t spacing = {-10, -10, 0};

	handles[0] = cgs.media.gibSkull;
	handles[1] = 0;

	CG_RegisterModelInfo( MI_NONE, handles, spacing, origin, angles );
	obitInit = qtrue;
}

/*
===================
CG_ParseObituary
===================
*/
void CG_ParseObituary( entityState_t *ent ) {
	int 	mod;
	int 	target, attacker;
	playerInfo_t	*targetpi, *attackerpi;
	const char 	*targetInfo;
	const char 	*attackerInfo;
	char 	targetName[OBIT_MAX_NAME_LENGTH];
	char 	attackerName[OBIT_MAX_NAME_LENGTH];
	int i;

	if ( !obitInit )
		CG_InitObituary();

	target = ent->otherEntityNum;
	attacker = ent->otherEntityNum2;
	targetpi = &cgs.playerinfo[ target ];
	attackerpi = &cgs.playerinfo[ attacker ];
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
	strcat(targetName, S_COLOR_WHITE);

	if ( (attacker == ENTITYNUM_WORLD) || (attacker == target) ) {
		CG_AddObituary( NULL, targetName, -1, targetpi->team, mod );
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

			CG_CenterPrint( i, s, SCREEN_HEIGHT * 0.3, 0.35 );
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
		if ( cgs.gametype >= GT_TEAM && targetpi->team == attackerpi->team )
			mod = MI_SAMETEAM;
		else if ( ent->eFlags & EF_HEADSHOT )
			mod = MI_HEADSHOT;
		
		CG_AddObituary( attackerName, targetName, attackerpi->team, targetpi->team, mod );
		return;
	}

	CG_AddObituary( NULL, targetName, -1, targetpi->team, MI_NONE );
}

static float obitRedColor[4] = { 1.0f, 0.1f, 0.1f, 1.0f };
static float obitBlueColor[4] = { 0.1f, 0.1f, 1.0f, 1.0f };
static float obitNormalColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

/*
===================
CG_DrawObituary
===================
*/
void CG_DrawObituary( void ) {
	int i;
	float *color;
	float drawColor[4];
	float *teamColor;
	float x, y;
	modelInfo_t *mi;

	if ( !obitInit )
		CG_InitObituary();

	CG_SetScreenPlacement( PLACE_LEFT, PLACE_BOTTOM );

	for ( i = 0; i < OBIT_MAX_VISABLE; i++ ) {
		if ( obitStack[i].target[0] == '\0' )
			continue;

		mi = obitStack[i].mi;

		color = CG_FadeColor( obitStack[i].time, OBIT_FADE_TIME );

		if ( !color ) {
			CG_ClearObit( i );
			CG_CountObits();
			continue;
		}

		y = OBIT_POS_Y - ( i * OBIT_SPACING );
		x = OBIT_POS_X;

		if ( obitStack[i].attacker[0] != '\0' ) {
			if ( cgs.gametype >= GT_TEAM ) {
				teamColor = (obitStack[i].attackerTeam == TEAM_RED) ? obitRedColor : obitBlueColor;
				RemoveColorEscapeSequences(obitStack[i].attacker);
			} else {
				teamColor = obitNormalColor;
			}

			drawColor[0] = color[0] * teamColor[0];
			drawColor[1] = color[1] * teamColor[1];
			drawColor[2] = color[2] * teamColor[2];
			drawColor[3] = color[3] * teamColor[3];

			trap_R_SetColor( drawColor );
			CG_DrawString( x, y, obitStack[i].attacker, fontFlags, drawColor );
			x += CG_DrawStrlen( obitStack[i].attacker, fontFlags );
			x += mi->spacing[0];
		} else {
			x -= 20;
		}

		if (mi->handles[1]) {
			CG_Draw3DModel( x, y-10, OBIT_ICON_WIDTH, OBIT_ICON_HEIGHT, mi->handles[1], NULL, mi->origin, mi->angles );
			x += mi->spacing[2];
		}

		if (mi->handles[0]) {
			CG_Draw3DModel( x, y-10, OBIT_ICON_WIDTH, OBIT_ICON_HEIGHT, mi->handles[0], NULL, mi->origin, mi->angles );
		}

		x += OBIT_ICON_WIDTH;
		x += mi->spacing[1];

		if ( cgs.gametype >= GT_TEAM ) {
			teamColor = (obitStack[i].targetTeam == TEAM_RED) ? obitRedColor : obitBlueColor;
			RemoveColorEscapeSequences(obitStack[i].target);
		} else {
			teamColor = obitNormalColor;
		}

		drawColor[0] = color[0] * teamColor[0];
		drawColor[1] = color[1] * teamColor[1];
		drawColor[2] = color[2] * teamColor[2];
		drawColor[3] = color[3] * teamColor[3];

		trap_R_SetColor( drawColor );
		CG_DrawString( x, y, obitStack[i].target, fontFlags, drawColor );

		trap_R_SetColor( NULL );
	}
}
