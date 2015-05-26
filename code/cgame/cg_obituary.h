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

#include "cg_local.h"

#define OBIT_MAX_VISABLE 5

#define OBIT_MAX_NAME_LENGTH 28

#define OBIT_FADE_TIME 5000

#define OBIT_POS_X 10
#define OBIT_POS_Y 400

#define OBIT_ICON_HEIGHT 32
#define OBIT_ICON_WIDTH 64

#define OBIT_SPACING 25
#define OBIT_GAP_WIDTH 10

static int fontFlags = (UI_TINYFONT);

#define MAX_MODEL_INFO 32

typedef struct {
    int mod;
    qhandle_t handles[2];
    vec3_t spacing;    // before, after, barrelSpacing
    vec3_t origin;
    vec3_t angles;
} modelInfo_t;

#define MI_NONE -1
#define MI_HEADSHOT -2

typedef struct {
    char    attacker[OBIT_MAX_NAME_LENGTH];
    char    target[OBIT_MAX_NAME_LENGTH];
    int     time;
    modelInfo_t *mi;
} obituary_t;
