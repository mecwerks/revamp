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
//
// cg_hud.c -- draw heads up display

#include "cg_local.h"

extern void CG_DrawStatusBarHead( float x );
extern void CG_DrawStatusBarFlag( float x, int team );
extern void CG_DrawField (int x, int y, int width, int value);
extern void CG_DrawFieldEX (int x, int y, int width, int value, int cw, int ch);

#define HUD_ICON_SIZE 50

#define HUD_TEXT_HEIGHT (HUD_ICON_SIZE-10)
#define HUD_TEXT_WIDTH (HUD_TEXT_HEIGHT/2)

/*
===================
CG_DrawHUD
===================
*/
void CG_DrawHUD( void ) {
    int         color;
    centity_t   *cent;
    playerState_t   *ps;
    int         value;
    vec3_t      angles;
    vec3_t      origin;
    float x;

    static float colors[4][4] = { 
//      { 0.2, 1.0, 0.2, 1.0 } , { 1.0, 0.2, 0.2, 1.0 }, {0.5, 0.5, 0.5, 1} };
        { 1.0f, 0.69f, 0.0f, 1.0f },    // normal
        { 1.0f, 0.2f, 0.2f, 1.0f },     // low health
        { 0.5f, 0.5f, 0.5f, 1.0f },     // weapon firing
        { 1.0f, 1.0f, 1.0f, 1.0f } };   // health > 100

    if ( cg_drawStatus.integer == 0 ) {
        return;
    }

    CG_SetScreenPlacement(PLACE_RIGHT, PLACE_BOTTOM);

    ps = cg.cur_ps;
    cent = &cg_entities[ps->playerNum];

    // draw the team background
//    CG_DrawTeamBackground( 0, 420, 640, 60, 0.33f, ps->persistant[PERS_TEAM] );

    x = 625;

    //
    // head
    //
    CG_DrawStatusBarHead( x - HUD_ICON_SIZE );
    x -= HUD_ICON_SIZE;

    //
    // health
    //
    value = ps->stats[STAT_HEALTH];
    if ( value > 100 ) {
        trap_R_SetColor( colors[3] );       // white
    } else if (value > 25) {
        trap_R_SetColor( colors[0] );   // green
    } else if (value > 0) {
        color = (cg.time >> 8) & 1; // flash
        trap_R_SetColor( colors[color] );
    } else {
        trap_R_SetColor( colors[1] );   // red
    }

    // stretch the health up when taking damage
    CG_DrawFieldEX( x - (HUD_TEXT_WIDTH*3), 470 - HUD_TEXT_HEIGHT, 3, value, HUD_TEXT_WIDTH, HUD_TEXT_HEIGHT );
    trap_R_SetColor( NULL );
    x -= HUD_TEXT_WIDTH*3;

    //
    // ammo box
    //
    VectorClear( angles );
    // draw any 3D icons first, so the changes back to 2D are minimized
    if ( cent->currentState.weapon && cg_weapons[ cent->currentState.weapon ].ammoModel ) {
        origin[0] = 70;
        origin[1] = 0;
        origin[2] = 0;
        angles[YAW] = 90;
        CG_Draw3DModel( x - HUD_ICON_SIZE, 478 - HUD_ICON_SIZE, HUD_ICON_SIZE, HUD_ICON_SIZE,
                       cg_weapons[ cent->currentState.weapon ].ammoModel, NULL, origin, angles );
        x -= HUD_ICON_SIZE;  
    }

    //
    // ammo
    //
    if ( cent->currentState.weapon ) {
        value = ps->ammo[cent->currentState.weapon];
        if ( value > -1 ) {
            if ( cg.cur_lc->predictedPlayerState.weaponstate == WEAPON_FIRING
                && cg.cur_lc->predictedPlayerState.weaponTime > 100 ) {
                // draw as dark grey when reloading
                color = 2;  // dark grey
            } else {
                if ( value >= 0 ) {
                    color = 0;  // green
                } else {
                    color = 1;  // red
                }
            }
            trap_R_SetColor( colors[color] );
            
            CG_DrawFieldEX (x - (HUD_TEXT_WIDTH*3), 470 - HUD_TEXT_HEIGHT, 3, value, HUD_TEXT_WIDTH, HUD_TEXT_HEIGHT);
            trap_R_SetColor( NULL );

            x -= HUD_TEXT_WIDTH*3;
        }
    }

    if( cg.cur_lc->predictedPlayerState.powerups[PW_REDFLAG] ) {
        CG_DrawStatusBarFlag( 185 + CHAR_WIDTH*3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_RED );
    } else if( cg.cur_lc->predictedPlayerState.powerups[PW_BLUEFLAG] ) {
        CG_DrawStatusBarFlag( 185 + CHAR_WIDTH*3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_BLUE );
    } else if( cg.cur_lc->predictedPlayerState.powerups[PW_NEUTRALFLAG] ) {
        CG_DrawStatusBarFlag( 185 + CHAR_WIDTH*3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_FREE );
    }

    x = 625;

    //
    // armor model
    //
    if ( ps->stats[ STAT_ARMOR ] ) {
        origin[0] = 90;
        origin[1] = 0;
        origin[2] = -10;
        angles[YAW] = 180;
        CG_Draw3DModel( x - HUD_ICON_SIZE + 3, 470 - (HUD_ICON_SIZE*2), HUD_ICON_SIZE, HUD_ICON_SIZE, cgs.media.armorModel, NULL, origin, angles );
        x -= HUD_ICON_SIZE;
    }

    //
    // armor
    //
    value = ps->stats[STAT_ARMOR];
    if (value > 0 ) {
        trap_R_SetColor( colors[0] );
        CG_DrawFieldEX (x - (HUD_TEXT_WIDTH*3), 463 - HUD_TEXT_HEIGHT - HUD_ICON_SIZE, 3, value, HUD_TEXT_WIDTH, HUD_TEXT_HEIGHT);
        trap_R_SetColor( NULL );
        // if we didn't draw a 3D icon, draw a 2D icon for armor
    }
}

/*
================
CG_DrawStatusBarHead
================
*/
void CG_DrawStatusBarHead( float x ) {
    vec3_t      angles;
    float       size, stretch;
    float       frac;

    VectorClear( angles );

    if ( cg.cur_lc->damageTime && cg.time - cg.cur_lc->damageTime < DAMAGE_TIME ) {
        frac = (float)(cg.time - cg.cur_lc->damageTime ) / DAMAGE_TIME;
        size = HUD_ICON_SIZE * 1.25 * ( 1.5 - frac * 0.5 );

        stretch = size - HUD_ICON_SIZE * 1.25;
        // kick in the direction of damage
        x -= stretch * 0.5 + cg.cur_lc->damageX * stretch * 0.5;

#if 0
        cg.cur_lc->headStartYaw = 180 + cg.cur_lc->damageX * 45;

        cg.cur_lc->headEndYaw = 180 + 20 * cos( crandom()*M_PI );
        cg.cur_lc->headEndPitch = 5 * cos( crandom()*M_PI );

        cg.cur_lc->headStartTime = cg.time;
        cg.cur_lc->headEndTime = cg.time + 100 + random() * 2000;
#endif
    } else {
    #if 0
        if ( cg.time >= cg.cur_lc->headEndTime ) {
            // select a new head angle
            cg.cur_lc->headStartYaw = cg.cur_lc->headEndYaw;
            cg.cur_lc->headStartPitch = cg.cur_lc->headEndPitch;
            cg.cur_lc->headStartTime = cg.cur_lc->headEndTime;
            cg.cur_lc->headEndTime = cg.time + 100 + random() * 2000;

            cg.cur_lc->headEndYaw = 180 + 20 * cos( crandom()*M_PI );
            cg.cur_lc->headEndPitch = 5 * cos( crandom()*M_PI );
        }
    #endif
        size = HUD_ICON_SIZE * 1.25;
    }

    // if the server was frozen for a while we may have a bad head start time
    if ( cg.cur_lc->headStartTime > cg.time ) {
        cg.cur_lc->headStartTime = cg.time;
    }

    angles[YAW] = 180;
    angles[PITCH] = 0;

    CG_DrawHead( x, 480 - size, size, size, cg.cur_ps->playerNum, angles );
}
