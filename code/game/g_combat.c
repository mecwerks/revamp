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
// g_combat.c

#include "g_local.h"


/*
============
ScorePlum
============
*/
void ScorePlum( gentity_t *ent, vec3_t origin, int score ) {
	gentity_t *plum;

	plum = G_TempEntity( origin, EV_SCOREPLUM );
	// only send this temp entity to a single player
	plum->r.svFlags |= SVF_PLAYERMASK;
	Com_ClientListClear( &plum->r.sendPlayers );
	Com_ClientListAdd( &plum->r.sendPlayers, ent->s.number );
	//
	plum->s.otherEntityNum = ent->s.number;
	plum->s.time = score;
}

/*
============
AddScore

Adds score to both the player and his team
============
*/
void AddScore( gentity_t *ent, vec3_t origin, int score ) {
	if ( !ent->player ) {
		return;
	}
	// no scoring during pre-match warmup
	if ( level.warmupTime ) {
		return;
	}
	// show score plum
	ScorePlum(ent, origin, score);
	//
	ent->player->ps.persistant[PERS_SCORE] += score;
	if ( g_gametype.integer == GT_TEAM ) {
		AddTeamScore( origin, ent->player->ps.persistant[PERS_TEAM], score );
	}
	CalculateRanks();
}

/*
=================
TossPlayerItems

Toss the weapon and powerups for the killed player
=================
*/
void TossPlayerItems( gentity_t *self ) {
	gitem_t		*item;
	int			weapon;
	float		angle;
	int			i;
	gentity_t	*drop;

	// drop the weapon if not a gauntlet or machinegun
	weapon = self->s.weapon;

	// make a special check to see if they are changing to a new
	// weapon that isn't the mg or gauntlet.  Without this, a player
	// can pick up a weapon, be killed, and not drop the weapon because
	// their weapon change hasn't completed yet and they are still holding the MG.
	if ( weapon == WP_MACHINEGUN || weapon == WP_GRAPPLING_HOOK ) {
		if ( self->player->ps.weaponstate == WEAPON_DROPPING ) {
			BG_DecomposeUserCmdValue( self->player->pers.cmd.stateValue, &weapon );
		}
		if ( !( self->player->ps.stats[STAT_WEAPONS] & ( 1 << weapon ) ) ) {
			weapon = WP_NONE;
		}
	}

	if ( weapon > WP_MACHINEGUN && weapon != WP_GRAPPLING_HOOK && 
		self->player->ps.ammo[ weapon ] ) {
		// find the item type for this weapon
		item = BG_FindItemForWeapon( weapon );

		// spawn the item
		Drop_Item( self, item, 0 );
	}

	// drop all the powerups if not in teamplay
	if ( g_gametype.integer != GT_TEAM ) {
		angle = 45;
		for ( i = 1 ; i < PW_NUM_POWERUPS ; i++ ) {
			if ( self->player->ps.powerups[ i ] > level.time ) {
				item = BG_FindItemForPowerup( i );
				if ( !item ) {
					continue;
				}
				drop = Drop_Item( self, item, angle );
				// decide how many seconds it has left
				drop->count = ( self->player->ps.powerups[ i ] - level.time ) / 1000;
				if ( drop->count < 1 ) {
					drop->count = 1;
				}
				angle += 45;
			}
		}
	}
}

/*
===========
TossPlayerGametypeItems

Drop CTF flag and Harvester cubes
===========
*/
void TossPlayerGametypeItems(gentity_t *ent) {
	int j;
	gitem_t *item;
	gentity_t *drop;
	int angle = 0;

	// drop flags in CTF
	item = NULL;
	j = 0;

	if ( ent->player->ps.powerups[ PW_REDFLAG ] ) {
		item = BG_FindItemForPowerup( PW_REDFLAG );
		j = PW_REDFLAG;
	} else if ( ent->player->ps.powerups[ PW_BLUEFLAG ] ) {
		item = BG_FindItemForPowerup( PW_BLUEFLAG );
		j = PW_BLUEFLAG;
	} else if ( ent->player->ps.powerups[ PW_NEUTRALFLAG ] ) {
		item = BG_FindItemForPowerup( PW_NEUTRALFLAG );
		j = PW_NEUTRALFLAG;
	}

	if ( item ) {
		drop = Drop_Item( ent, item, angle );
		angle += 45;
		// decide how many seconds it has left
		drop->count = ( ent->player->ps.powerups[ j ] - level.time ) / 1000;
		if ( drop->count < 1 ) {
			drop->count = 1;
		}
		ent->player->ps.powerups[ j ] = 0;
	}

#ifdef MISSIONPACK
	if ( g_gametype.integer == GT_HARVESTER ) {
		if ( ent->player->ps.tokens > 0 ) {
			if ( ent->player->sess.sessionTeam == TEAM_RED ) {
				item = BG_FindItem( "Blue Cube" );
			} else {
				item = BG_FindItem( "Red Cube" );
			}
			if ( item ) {
				for ( j = 0; j < ent->player->ps.tokens; j++ ) {
					drop = Drop_Item( ent, item, angle );
					if ( ent->player->sess.sessionTeam == TEAM_RED ) {
						drop->s.team = TEAM_BLUE;
					} else {
						drop->s.team = TEAM_RED;
					}
					angle += 45;
				}
			}
			ent->player->ps.tokens = 0;
		}
	}
#endif
}

#ifdef MISSIONPACK

/*
=================
TossPlayerCubes

Spawn cube at neutral obelisk
=================
*/
extern gentity_t	*neutralObelisk;

void TossPlayerCubes( gentity_t *self ) {
	gitem_t		*item;
	gentity_t	*drop;
	vec3_t		velocity;
	vec3_t		angles;
	vec3_t		origin;

	self->player->ps.tokens = 0;

	// this should never happen but we should never
	// get the server to crash due to skull being spawned in
	if (!G_EntitiesFree()) {
		return;
	}

	if( self->player->sess.sessionTeam == TEAM_RED ) {
		item = BG_FindItem( "Red Cube" );
	}
	else {
		item = BG_FindItem( "Blue Cube" );
	}

	angles[YAW] = (float)(level.time % 360);
	angles[PITCH] = 0;	// always forward
	angles[ROLL] = 0;

	AngleVectors( angles, velocity, NULL, NULL );
	VectorScale( velocity, 150, velocity );
	velocity[2] += 200 + crandom() * 50;

	if( neutralObelisk ) {
		VectorCopy( neutralObelisk->s.pos.trBase, origin );
		origin[2] += 44;
	} else {
		VectorClear( origin ) ;
	}

	drop = LaunchItem( item, origin, velocity );

	drop->nextthink = level.time + g_cubeTimeout.integer * 1000;
	drop->think = G_FreeEntity;
	drop->s.team = self->player->sess.sessionTeam;
}


/*
=================
TossPlayerPersistantPowerups
=================
*/
void TossPlayerPersistantPowerups( gentity_t *ent ) {
	gentity_t	*powerup;

	if( !ent->player ) {
		return;
	}

	if( !ent->player->persistantPowerup ) {
		return;
	}

	powerup = ent->player->persistantPowerup;

	powerup->r.svFlags &= ~SVF_NOCLIENT;
	powerup->s.eFlags &= ~EF_NODRAW;
	powerup->s.contents = CONTENTS_TRIGGER;
	trap_LinkEntity( powerup );

	ent->player->ps.stats[STAT_PERSISTANT_POWERUP] = 0;
	ent->player->persistantPowerup = NULL;
}
#endif


/*
==================
LookAtKiller
==================
*/
void LookAtKiller( gentity_t *self, gentity_t *inflictor, gentity_t *attacker ) {
	vec3_t		dir;

	if ( attacker && attacker != self ) {
		VectorSubtract (attacker->s.pos.trBase, self->s.pos.trBase, dir);
	} else if ( inflictor && inflictor != self ) {
		VectorSubtract (inflictor->s.pos.trBase, self->s.pos.trBase, dir);
	} else {
		self->player->ps.stats[STAT_DEAD_YAW] = self->s.angles[YAW];
		return;
	}

	self->player->ps.stats[STAT_DEAD_YAW] = vectoyaw ( dir );
}

/*
==================
GibEntity
==================
*/
void GibEntity( gentity_t *self, qboolean headshot ) {
	gentity_t *ent;
	int i;

	//if this entity still has kamikaze
	if (self->s.eFlags & EF_KAMIKAZE) {
		self->s.eFlags &= ~EF_KAMIKAZE;
		if (self->player) {
			self->player->ps.eFlags &= ~EF_KAMIKAZE;
		}

		// check if there is a kamikaze timer around for this owner
		for (i = 0; i < level.num_entities; i++) {
			ent = &g_entities[i];
			if (!ent->inuse)
				continue;
			if (ent->activator != self)
				continue;
			if (strcmp(ent->classname, "kamikaze timer"))
				continue;
			G_FreeEntity(ent);
			break;
		}
	}

	if (headshot) {
		self->s.eFlags |= EF_GIBBED_HEADSHOT;

		if (self->player)
			self->player->ps.eFlags |= EF_GIBBED_HEADSHOT;

		self->player->headless = qtrue;
		self->takedamage = qtrue;
	} else {
		self->s.eFlags |= EF_GIBBED;
		self->s.contents = 0;

		if (self->player) {
			self->player->ps.eFlags |= EF_GIBBED;
			self->player->ps.contents = 0;
		}
		self->player->headless = qfalse;
		self->takedamage = qfalse;
	}
}

/*
==================
body_die
==================
*/
void body_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath ) {
	if ( self->health > GIB_HEALTH ) {
		return;
	}

	GibEntity( self, qfalse );

	// add corpse gibbed event
	G_AddEvent( self, EV_DEATH1, 2 );
}


// these are just for logging, the client prints its own messages
char	*modNames[] = {
	"MOD_UNKNOWN",
	"MOD_SHOTGUN",
	"MOD_GAUNTLET",
	"MOD_MACHINEGUN",
	"MOD_GRENADE",
	"MOD_GRENADE_SPLASH",
	"MOD_ROCKET",
	"MOD_ROCKET_SPLASH",
	"MOD_PLASMA",
	"MOD_PLASMA_SPLASH",
	"MOD_RAILGUN",
	"MOD_LIGHTNING",
	"MOD_BFG",
	"MOD_BFG_SPLASH",
	"MOD_WATER",
	"MOD_SLIME",
	"MOD_LAVA",
	"MOD_CRUSH",
	"MOD_TELEFRAG",
	"MOD_FALLING",
	"MOD_SUICIDE",
	"MOD_ASSISTED_SUICIDE",
	"MOD_TARGET_LASER",
	"MOD_TRIGGER_HURT",
#ifdef MISSIONPACK
	"MOD_NAIL",
	"MOD_CHAINGUN",
	"MOD_PROXIMITY_MINE",
	"MOD_KAMIKAZE",
	"MOD_JUICED",
#endif
	"MOD_GRAPPLE",
	"MOD_SUICIDE_TEAM_CHANGE"
};

#ifdef MISSIONPACK
/*
==================
Kamikaze_DeathActivate
==================
*/
void Kamikaze_DeathActivate( gentity_t *ent ) {
	G_StartKamikaze(ent);
	G_FreeEntity(ent);
}

/*
==================
Kamikaze_DeathTimer
==================
*/
void Kamikaze_DeathTimer( gentity_t *self ) {
	gentity_t *ent;

	ent = G_Spawn();
	ent->classname = "kamikaze timer";
	VectorCopy(self->s.pos.trBase, ent->s.pos.trBase);
	ent->r.svFlags |= SVF_NOCLIENT;
	ent->think = Kamikaze_DeathActivate;
	ent->nextthink = level.time + 5 * 1000;

	ent->activator = self;
}

#endif

/*
==================
CheckAlmostCapture
==================
*/
void CheckAlmostCapture( gentity_t *self, gentity_t *attacker ) {
	gentity_t	*ent;
	vec3_t		dir;
	char		*classname;

	// if this player was carrying a flag
	if ( self->player->ps.powerups[PW_REDFLAG] ||
		self->player->ps.powerups[PW_BLUEFLAG] ||
		self->player->ps.powerups[PW_NEUTRALFLAG] ) {
		// get the goal flag this player should have been going for
		if ( g_gametype.integer == GT_CTF ) {
			if ( self->player->sess.sessionTeam == TEAM_BLUE ) {
				classname = "team_CTF_blueflag";
			}
			else {
				classname = "team_CTF_redflag";
			}
		}
		else {
			if ( self->player->sess.sessionTeam == TEAM_BLUE ) {
				classname = "team_CTF_redflag";
			}
			else {
				classname = "team_CTF_blueflag";
			}
		}
		ent = NULL;
		do
		{
			ent = G_Find(ent, FOFS(classname), classname);
		} while (ent && (ent->flags & FL_DROPPED_ITEM));
		// if we found the destination flag and it's not picked up
		if (ent && !(ent->r.svFlags & SVF_NOCLIENT) ) {
			// if the player was *very* close
			VectorSubtract( self->player->ps.origin, ent->s.origin, dir );
			if ( VectorLength(dir) < 200 ) {
				self->player->ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_HOLYSHIT;
				if ( attacker->player ) {
					attacker->player->ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_HOLYSHIT;
				}
			}
		}
	}
}

/*
==================
CheckAlmostScored
==================
*/
void CheckAlmostScored( gentity_t *self, gentity_t *attacker ) {
	gentity_t	*ent;
	vec3_t		dir;
	char		*classname;

	// if the player was carrying cubes
	if ( self->player->ps.tokens ) {
		if ( self->player->sess.sessionTeam == TEAM_BLUE ) {
			classname = "team_redobelisk";
		}
		else {
			classname = "team_blueobelisk";
		}
		ent = G_Find(NULL, FOFS(classname), classname);
		// if we found the destination obelisk
		if ( ent ) {
			// if the player was *very* close
			VectorSubtract( self->player->ps.origin, ent->s.origin, dir );
			if ( VectorLength(dir) < 200 ) {
				self->player->ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_HOLYSHIT;
				if ( attacker->player ) {
					attacker->player->ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_HOLYSHIT;
				}
			}
		}
	}
}

/*
==================
player_die
==================
*/
void player_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath ) {
	static int	rndAnim;
	gentity_t	*ent;
	int			anim;
	int			contents;
	int			killer;
	int			i;
	char		*killerName, *obit;
	qboolean	gibPlayer;
	qboolean	headshot = qfalse;

	if ( self->player->ps.pm_type == PM_DEAD ) {
		return;
	}

	if ( level.intermissiontime ) {
		return;
	}

	// make sure the body shows up in the player's current position
	G_UnTimeShiftClient( self );

	// check for an almost capture
	CheckAlmostCapture( self, attacker );
	// check for a player that almost brought in cubes
	CheckAlmostScored( self, attacker );

	if (self->player && self->player->hook) {
		Weapon_HookFree(self->player->hook);
	}
#ifdef MISSIONPACK
	if ((self->player->ps.eFlags & EF_TICKING) && self->activator) {
		self->player->ps.eFlags &= ~EF_TICKING;
		self->activator->think = G_FreeEntity;
		self->activator->nextthink = level.time;
	}
#endif
	self->player->ps.pm_type = PM_DEAD;

	if ( attacker ) {
		killer = attacker->s.number;
		if ( attacker->player ) {
			killerName = attacker->player->pers.netname;
		} else {
			killerName = "<non-player>";
		}
	} else {
		killer = ENTITYNUM_WORLD;
		killerName = "<world>";
	}

	if ( killer < 0 || killer >= MAX_CLIENTS ) {
		killer = ENTITYNUM_WORLD;
		killerName = "<world>";
	}

	if ( meansOfDeath < 0 || meansOfDeath >= ARRAY_LEN( modNames ) ) {
		obit = "<bad obituary>";
	} else {
		obit = modNames[meansOfDeath];
	}

	G_LogPrintf("Kill: %i %i %i: %s killed %s by %s\n", 
		killer, self->s.number, meansOfDeath, killerName, 
		self->player->pers.netname, obit );

	self->enemy = attacker;

	self->player->ps.persistant[PERS_KILLED]++;

	if (attacker && attacker->player) {
		attacker->player->lastkilled_player = self->s.number;

		if ( attacker == self || OnSameTeam (self, attacker ) ) {
			if ( ((level.time - self->player->lasthurt_time) <= ASSISTED_SUICIDE_TIME) && 
				g_entities[self->player->lasthurt_player2].player && (self->player->lasthurt_player2 != self->s.number) ) 
			{
				AddScore( &g_entities[self->player->lasthurt_player2], self->r.currentOrigin, 1 );
				meansOfDeath = MOD_ASSISTED_SUICIDE;
			} else AddScore( attacker, self->r.currentOrigin, -1 );
		} else {
			AddScore( attacker, self->r.currentOrigin, 1 );

			if( meansOfDeath == MOD_GAUNTLET ) {
				
				// play humiliation on player
				attacker->player->ps.persistant[PERS_GAUNTLET_FRAG_COUNT]++;

				// add the sprite over the player's head
				attacker->player->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
				attacker->player->ps.eFlags |= EF_AWARD_GAUNTLET;
				attacker->player->rewardTime = level.time + REWARD_SPRITE_TIME;

				// also play humiliation on target
				self->player->ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_GAUNTLETREWARD;
			}

			// check for two kills in a short amount of time
			// if this is close enough to the last kill, give a reward sound
			if ( level.time - attacker->player->lastKillTime < CARNAGE_REWARD_TIME ) {
				// play excellent on player
				attacker->player->ps.persistant[PERS_EXCELLENT_COUNT]++;

				// add the sprite over the player's head
				attacker->player->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
				attacker->player->ps.eFlags |= EF_AWARD_EXCELLENT;
				attacker->player->rewardTime = level.time + REWARD_SPRITE_TIME;
			}
			attacker->player->lastKillTime = level.time;

		}
	} else {
		if ( ((level.time - self->player->lasthurt_time) <= ASSISTED_SUICIDE_TIME) &&
			g_entities[self->player->lasthurt_player2].player && (self->player->lasthurt_player2 != self->s.number) ) 
		{
			AddScore( &g_entities[self->player->lasthurt_player2], self->r.currentOrigin, 1 );
			meansOfDeath = MOD_ASSISTED_SUICIDE;
		} else AddScore( self, self->r.currentOrigin, -1 );
	}

	// don't send death obituary when swiching teams
	// allow mod to be changed by death type
	if (meansOfDeath != MOD_SUICIDE_TEAM_CHANGE) {
		// broadcast the death event to everyone
		ent = G_TempEntity( self->r.currentOrigin, EV_OBITUARY );
		ent->s.eventParm = meansOfDeath;
		ent->s.otherEntityNum = self->s.number;
		ent->s.otherEntityNum2 = killer;
		ent->r.svFlags = SVF_BROADCAST;	// send to everyone
		if (self->player && attacker->player && ((self->player->lasthurt_location & LOCATION_HEAD) ||
			(self->player->lasthurt_location & LOCATION_FACE)))
		{
			ent->s.eFlags |= EF_HEADSHOT;
			headshot = qtrue;
		}
	}

	// Add team bonuses
	Team_FragBonuses(self, inflictor, attacker);

	// if I committed suicide, the flag does not fall, it returns.
	if (meansOfDeath == MOD_SUICIDE || meansOfDeath == MOD_SUICIDE_TEAM_CHANGE) {
		if ( self->player->ps.powerups[PW_NEUTRALFLAG] ) {		// only happens in One Flag CTF
			Team_ReturnFlag( TEAM_FREE );
			self->player->ps.powerups[PW_NEUTRALFLAG] = 0;
		}
		else if ( self->player->ps.powerups[PW_REDFLAG] ) {		// only happens in standard CTF
			Team_ReturnFlag( TEAM_RED );
			self->player->ps.powerups[PW_REDFLAG] = 0;
		}
		else if ( self->player->ps.powerups[PW_BLUEFLAG] ) {	// only happens in standard CTF
			Team_ReturnFlag( TEAM_BLUE );
			self->player->ps.powerups[PW_BLUEFLAG] = 0;
		}
	}

	TossPlayerItems( self );
#ifdef MISSIONPACK
	TossPlayerPersistantPowerups( self );
	if( g_gametype.integer == GT_HARVESTER ) {
		TossPlayerCubes( self );
	}
#endif

	Cmd_Score_f( self );		// show scores
	// send updated scores to any clients that are following this one,
	// or they would get stale scoreboards
	for ( i = 0 ; i < level.maxplayers ; i++ ) {
		gplayer_t	*player;

		player = &level.players[i];
		if ( player->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( player->sess.sessionTeam != TEAM_SPECTATOR ) {
			continue;
		}
		if ( player->sess.spectatorPlayer == self->s.number ) {
			Cmd_Score_f( g_entities + i );
		}
	}

	self->takedamage = qtrue;	// can still be gibbed

	self->s.weapon = WP_NONE;
	self->s.powerups = 0;
	self->player->ps.contents = self->s.contents = CONTENTS_CORPSE;

	self->s.angles[0] = 0;
	self->s.angles[2] = 0;
	LookAtKiller (self, inflictor, attacker);

	VectorCopy( self->s.angles, self->player->ps.viewangles );

	self->s.loopSound = 0;

	self->player->ps.maxs[2] = self->s.maxs[2] = -8;

	// don't allow respawn until the death anim is done
	// g_forcerespawn may force spawning at some later time
	self->player->respawnTime = level.time + 1700;

	// remove powerups
	memset( self->player->ps.powerups, 0, sizeof(self->player->ps.powerups) );

	// never gib in a nodrop
	contents = trap_PointContents( self->r.currentOrigin, -1 );

	gibPlayer = ( (self->health <= GIB_HEALTH && !(contents & CONTENTS_NODROP)) || meansOfDeath == MOD_SUICIDE );

	if ( gibPlayer ) {
		// gib death
		GibEntity( self, qfalse );

		// do normal death for clients with gibs disable
	} else {
		if (headshot)
			gibPlayer = 3;

		// the body can still be gibbed
		self->die = body_die;

#ifdef MISSIONPACK
		if (self->s.eFlags & EF_KAMIKAZE) {
			Kamikaze_DeathTimer( self );
		}
#endif
	}

	// normal death
	switch ( rndAnim ) {
	case 0:
		anim = BOTH_DEATH1;
		break;
	case 1:
		anim = BOTH_DEATH2;
		break;
	case 2:
	default:
		anim = BOTH_DEATH3;
		break;
	}

	self->player->ps.legsAnim = 
		( ( self->player->ps.legsAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | anim;
	self->player->ps.torsoAnim = 
		( ( self->player->ps.torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | anim;

	G_AddEvent( self, EV_DEATH1 + rndAnim, gibPlayer );

	if ( headshot )
		GibEntity( self, qtrue );
	else self->player->headless = qfalse;

	// globally cycle through the different death animations
	rndAnim = ( rndAnim + 1 ) % 3;

	trap_LinkEntity (self);

}


/*
================
CheckArmor
================
*/
int CheckArmor (gentity_t *ent, int damage, int dflags)
{
	gplayer_t	*player;
	int			save;
	int			count;

	if (!damage)
		return 0;

	player = ent->player;

	if (!player)
		return 0;

	if (dflags & DAMAGE_NO_ARMOR)
		return 0;

	// armor
	count = player->ps.stats[STAT_ARMOR];
	save = ceil( damage * ARMOR_PROTECTION );
	if (save >= count)
		save = count;

	if (!save)
		return 0;

	player->ps.stats[STAT_ARMOR] -= save;

	return save;
}

/*
================
RaySphereIntersections
================
*/
int RaySphereIntersections( vec3_t origin, float radius, vec3_t point, vec3_t dir, vec3_t intersections[2] ) {
	float b, c, d, t;

	//	| origin - (point + t * dir) | = radius
	//	a = dir[0]^2 + dir[1]^2 + dir[2]^2;
	//	b = 2 * (dir[0] * (point[0] - origin[0]) + dir[1] * (point[1] - origin[1]) + dir[2] * (point[2] - origin[2]));
	//	c = (point[0] - origin[0])^2 + (point[1] - origin[1])^2 + (point[2] - origin[2])^2 - radius^2;

	// normalize dir so a = 1
	VectorNormalize(dir);
	b = 2 * (dir[0] * (point[0] - origin[0]) + dir[1] * (point[1] - origin[1]) + dir[2] * (point[2] - origin[2]));
	c = (point[0] - origin[0]) * (point[0] - origin[0]) +
		(point[1] - origin[1]) * (point[1] - origin[1]) +
		(point[2] - origin[2]) * (point[2] - origin[2]) -
		radius * radius;

	d = b * b - 4 * c;
	if (d > 0) {
		t = (- b + sqrt(d)) / 2;
		VectorMA(point, t, dir, intersections[0]);
		t = (- b - sqrt(d)) / 2;
		VectorMA(point, t, dir, intersections[1]);
		return 2;
	}
	else if (d == 0) {
		t = (- b ) / 2;
		VectorMA(point, t, dir, intersections[0]);
		return 1;
	}
	return 0;
}

#ifdef MISSIONPACK
/*
================
G_InvulnerabilityEffect
================
*/
int G_InvulnerabilityEffect( gentity_t *targ, vec3_t dir, vec3_t point, vec3_t impactpoint, vec3_t bouncedir ) {
	gentity_t	*impact;
	vec3_t		intersections[2], vec;
	int			n;

	if ( !targ->player ) {
		return qfalse;
	}
	VectorCopy(dir, vec);
	VectorInverse(vec);
	// sphere model radius = 42 units
	n = RaySphereIntersections( targ->player->ps.origin, 42, point, vec, intersections);
	if (n > 0) {
		impact = G_TempEntity( targ->player->ps.origin, EV_INVUL_IMPACT );
		VectorSubtract(intersections[0], targ->player->ps.origin, vec);
		vectoangles(vec, impact->s.angles);
		impact->s.angles[0] += 90;
		if (impact->s.angles[0] > 360)
			impact->s.angles[0] -= 360;
		if ( impactpoint ) {
			VectorCopy( intersections[0], impactpoint );
		}
		if ( bouncedir ) {
			VectorCopy( vec, bouncedir );
			VectorNormalize( bouncedir );
		}
		return qtrue;
	}
	else {
		return qfalse;
	}
}
#endif

typedef struct {
	int weapon;
	float damageMod[3];
} weaponDamageMod_t;

weaponDamageMod_t weaponMods[WP_NUM_WEAPONS] = {
	{ WP_NONE,
		// head, torso, legs
		{0, 0, 0} },

	{ WP_GAUNTLET,
		{1, 1, 1} },

	{ WP_MACHINEGUN,
		{1.2, 1.1, 1} },

	{ WP_SHOTGUN,
		{1.1, 1, 1} },
	
	{ WP_GRENADE_LAUNCHER,
		{1.2, 1.2, 1.1} },

	{ WP_ROCKET_LAUNCHER,
		{1.2, 1.2, 1.2} },

	{ WP_LIGHTNING,
		{1.1, 1, 1} },

	{ WP_RAILGUN,
		{1.4, 1.2, 0.9} },

	{ WP_PLASMAGUN,
		{1.1, 1, 1} },

	{ WP_BFG,
		{1.2, 1.1, 1} },

	{ WP_GRAPPLING_HOOK,
		{0, 0, 0} }
};

/*
============
G_LocationString
============
*/
char *G_LocationString(int location) {
	// Check the location ignoring the rotation info
	switch (location &  ~(LOCATION_BACK | LOCATION_LEFT | LOCATION_RIGHT | LOCATION_FRONT)) {
		case LOCATION_HEAD:
		case LOCATION_FACE:
			return "Head";
		
		case LOCATION_SHOULDER:
			if (location & (LOCATION_FRONT | LOCATION_BACK))
				return "Neck";
			else return "Shoulders";

		case LOCATION_CHEST:
			if (location & (LOCATION_FRONT | LOCATION_BACK))
				return "Chest";
			else return "Arms";
		
		case LOCATION_STOMACH:
			return "Belly";
		
		case LOCATION_GROIN:
			return "Groin";
		
		case LOCATION_LEG:
			return "Leg";
		
		case LOCATION_FOOT:
			return "Foot";
		
		default:
			return "NONE";
	}
}
/*
============
G_WeaponDamageModifier
============
*/
int G_WeaponDamageModifier(int location, int otake, int weapon) {
	int i;
	float take = otake;
	weaponDamageMod_t *wpMod = NULL;

	for (i = WP_NONE+1; i < WP_NUM_WEAPONS; i++) {
		if (weaponMods[i].weapon == weapon)
			wpMod = &weaponMods[i];
	}

	if (!wpMod) {
		return take;
	}

	// Check the location ignoring the rotation info
	switch (location &  ~(LOCATION_BACK | LOCATION_LEFT | LOCATION_RIGHT | LOCATION_FRONT)) {
		case LOCATION_HEAD:
		case LOCATION_FACE:
			if ( g_instaGib.integer ) take = 130;
			else take *= wpMod->damageMod[0]; // head
			break;
		
		case LOCATION_SHOULDER:
		case LOCATION_CHEST:
		case LOCATION_STOMACH:
			if ( g_instaGib.integer ) take = 9999;
			else take *= wpMod->damageMod[1]; // torso
			break;
		
		case LOCATION_GROIN:
		case LOCATION_LEG:
		case LOCATION_FOOT:
			if ( g_instaGib.integer ) take = 9999;
			else take *= wpMod->damageMod[2]; // legs
			break;
	}

	return (int)take;
}

/* 
============
G_LocationDamage
============
*/
int G_LocationDamage(vec3_t point, gentity_t* targ, gentity_t* attacker, int take) {
	vec3_t bulletPath;
	vec3_t bulletAngle;

	int clientHeight;
	int clientFeetZ;
	int clientRotation;
	int bulletHeight;
	int bulletRotation;	// Degrees rotation around client.
				// used to check Back of head vs. Face
	int impactRotation;


	// First things first.  If we're not damaging them, why are we here? 
	if (!take) 
		return 0;

	// Point[2] is the REAL world Z. We want Z relative to the clients feet
	
	// Where the feet are at [real Z]
	clientFeetZ  = targ->r.currentOrigin[2] + targ->s.mins[2];	
	// How tall the client is [Relative Z]
	clientHeight = targ->s.maxs[2] - targ->s.mins[2];
	// Where the bullet struck [Relative Z]
	bulletHeight = point[2] - clientFeetZ;

	// Get a vector aiming from the client to the bullet hit 
	VectorSubtract(targ->r.currentOrigin, point, bulletPath); 
	// Convert it into PITCH, ROLL, YAW
	vectoangles(bulletPath, bulletAngle);

	clientRotation = targ->player->ps.viewangles[YAW];
	bulletRotation = bulletAngle[YAW];

	impactRotation = abs(clientRotation-bulletRotation);
	
	impactRotation += 45; // just to make it easier to work with
	impactRotation = impactRotation % 360; // Keep it in the 0-359 range

	if (impactRotation < 90)
		targ->player->lasthurt_location = LOCATION_BACK;
	else if (impactRotation < 180)
		targ->player->lasthurt_location = LOCATION_RIGHT;
	else if (impactRotation < 270)
		targ->player->lasthurt_location = LOCATION_FRONT;
	else if (impactRotation < 360)
		targ->player->lasthurt_location = LOCATION_LEFT;
	else
		targ->player->lasthurt_location = LOCATION_NONE;

	// The upper body never changes height, just distance from the feet
	if (bulletHeight > clientHeight - 2)
		targ->player->lasthurt_location |= LOCATION_HEAD;
	else if (bulletHeight > clientHeight - 8)
		targ->player->lasthurt_location |= LOCATION_FACE;
	else if (bulletHeight > clientHeight - 10)
		targ->player->lasthurt_location |= LOCATION_SHOULDER;
	else if (bulletHeight > clientHeight - 16)
		targ->player->lasthurt_location |= LOCATION_CHEST;
	else if (bulletHeight > clientHeight - 26)
		targ->player->lasthurt_location |= LOCATION_STOMACH;
	else if (bulletHeight > clientHeight - 29)
		targ->player->lasthurt_location |= LOCATION_GROIN;
	else if (bulletHeight < 4)
		targ->player->lasthurt_location |= LOCATION_FOOT;
	else
		// The leg is the only thing that changes size when you duck,
		// so we check for every other parts RELATIVE location, and
		// whats left over must be the leg. 
		targ->player->lasthurt_location |= LOCATION_LEG; 

	return G_WeaponDamageModifier(targ->player->lasthurt_location, take, attacker->player->ps.weapon);
}

/*
============
G_Damage

targ		entity that is being damaged
inflictor	entity that is causing the damage
attacker	entity that caused the inflictor to damage targ
	example: targ=monster, inflictor=rocket, attacker=player

dir			direction of the attack for knockback
point		point at which the damage is being inflicted, used for headshots
damage		amount of damage being inflicted
knockback	force to be applied against targ as a result of the damage

inflictor, attacker, dir, and point can be NULL for environmental effects

dflags		these flags are used to control how T_Damage works
	DAMAGE_RADIUS			damage was indirect (from a nearby explosion)
	DAMAGE_NO_ARMOR			armor does not protect from this damage
	DAMAGE_NO_KNOCKBACK		do not affect velocity, just view angles
	DAMAGE_NO_PROTECTION	kills godmode, armor, everything
============
*/

void G_Damage( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
			   vec3_t dir, vec3_t point, int damage, int dflags, int mod ) {
	gplayer_t	*player;
	int			take;
	int			asave = 0;
	int			knockback;
	int			max;
#ifdef MISSIONPACK
	vec3_t		bouncedir, impactpoint;
#endif

	if (!targ->takedamage) {
		return;
	}

	// the intermission has allready been qualified for, so don't
	// allow any extra scoring
	if ( level.intermissionQueued ) {
		return;
	}
#ifdef MISSIONPACK
	if ( targ->player && mod != MOD_JUICED) {
		if ( targ->player->invulnerabilityTime > level.time) {
			if ( dir && point ) {
				G_InvulnerabilityEffect( targ, dir, point, impactpoint, bouncedir );
			}
			return;
		}
	}
#endif
	if ( !inflictor ) {
		inflictor = &g_entities[ENTITYNUM_WORLD];
	}
	if ( !attacker ) {
		attacker = &g_entities[ENTITYNUM_WORLD];
	}

	// shootable doors / buttons don't actually have any health
	if ( targ->s.eType == ET_MOVER ) {
		if ( targ->use && targ->moverState == MOVER_POS1 ) {
			targ->use( targ, inflictor, attacker );
		}
		return;
	}
#ifdef MISSIONPACK
	if( g_gametype.integer == GT_OBELISK && CheckObeliskAttack( targ, attacker ) ) {
		return;
	}
#endif

	player = targ->player;

	if ( player ) {
		if ( player->noclip ) {
			return;
		}
	}

	if ( !dir ) {
		dflags |= DAMAGE_NO_KNOCKBACK;
	} else {
		VectorNormalize(dir);
	}

	knockback = damage;
	if ( knockback > 200 ) {
		knockback = 200;
	}
	if ( targ->flags & FL_NO_KNOCKBACK ) {
		knockback = 0;
	}
	if ( dflags & DAMAGE_NO_KNOCKBACK ) {
		knockback = 0;
	}

	// figure momentum add, even if the damage won't be taken
	if ( knockback && targ->player ) {
		vec3_t	kvel;
		float	mass;

		mass = 200;

		VectorScale (dir, g_knockback.value * (float)knockback / mass, kvel);
		VectorAdd (targ->player->ps.velocity, kvel, targ->player->ps.velocity);

		// set the timer so that the other player can't cancel
		// out the movement immediately
		if ( !targ->player->ps.pm_time ) {
			int		t;

			t = knockback * 2;
			if ( t < 50 ) {
				t = 50;
			}
			if ( t > 200 ) {
				t = 200;
			}
			targ->player->ps.pm_time = t;
			targ->player->ps.pm_flags |= PMF_TIME_KNOCKBACK;
		}
	}

	// check for completely getting out of the damage
	if ( !(dflags & DAMAGE_NO_PROTECTION) ) {

		// if TF_NO_FRIENDLY_FIRE is set, don't do damage to the target
		// if the attacker was on the same team
#ifdef MISSIONPACK
		if ( mod != MOD_JUICED && targ != attacker && !(dflags & DAMAGE_NO_TEAM_PROTECTION) && OnSameTeam (targ, attacker)  ) {
#else	
		if ( targ != attacker && OnSameTeam (targ, attacker)  ) {
#endif
			if ( !g_friendlyFire.integer ) {
				return;
			}
		}
#ifdef MISSIONPACK
		if (mod == MOD_PROXIMITY_MINE) {
			if (inflictor && inflictor->parent && OnSameTeam(targ, inflictor->parent)) {
				return;
			}
			if (targ == attacker) {
				return;
			}
		}
#endif

		// check for godmode
		if ( targ->flags & FL_GODMODE ) {
			return;
		}
	}

	// battlesuit protects from all radius damage (but takes knockback)
	// and protects 50% against all damage
	if ( player && player->ps.powerups[PW_BATTLESUIT] ) {
		G_AddEvent( targ, EV_POWERUP_BATTLESUIT, 0 );
		if ( ( dflags & DAMAGE_RADIUS ) || ( mod == MOD_FALLING ) ) {
			return;
		}
		damage *= 0.5;
	}

	// Mega Health reduces damage by 50%
	if ( targ->player && targ->player->ps.powerups[PW_MEGA] )
		damage *= 0.5;

	// add to the attacker's hit counter (if the target isn't a general entity like a prox mine)
	if ( attacker->player && player
			&& targ != attacker && targ->health > 0
			&& targ->s.eType != ET_MISSILE
			&& targ->s.eType != ET_GENERAL) {
		if ( OnSameTeam( targ, attacker ) ) {
			attacker->player->ps.persistant[PERS_HITS]--;
		} else {
			attacker->player->ps.persistant[PERS_HITS]++;
		}
		attacker->player->ps.persistant[PERS_ATTACKEE_ARMOR] = (targ->health<<8)|(player->ps.stats[STAT_ARMOR]);
	}

	// always give half damage if hurting self
	// calculated after knockback, so rocket jumping works
	if ( targ == attacker) {
		damage *= 0.5;
	}

	if ( damage < 1 ) {
		damage = 1;
	}
	take = damage;

	if (targ->player) {
		if (point && targ && targ->health && attacker && take)
			take = G_LocationDamage(point, targ, attacker, take);
		else 
			targ->player->lasthurt_location = LOCATION_NONE;
	}

	// save some from armor
	if ( !g_instaGib.integer && !(!g_selfDamage.integer && (targ == attacker))) {
		asave = CheckArmor (targ, take, dflags);
		take -= asave;
	}

	if ((targ == attacker) && !g_selfDamage.integer)
		take = 0;
	
	// add to the damage inflicted on a player this frame
	// the total will be turned into screen blends and view angle kicks
	// at the end of the frame
	if ( player ) {
		if ( attacker ) {
			player->ps.persistant[PERS_ATTACKER] = attacker->s.number;
		} else {
			player->ps.persistant[PERS_ATTACKER] = ENTITYNUM_WORLD;
		}
		player->damage_armor += asave;
		player->damage_blood += take;
		player->damage_knockback += knockback;
		if ( dir ) {
			VectorCopy ( dir, player->damage_from );
			player->damage_fromWorld = qfalse;
		} else {
			VectorCopy ( targ->r.currentOrigin, player->damage_from );
			player->damage_fromWorld = qtrue;
		}
	}

	// See if it's the player hurting the emeny flag carrier
#ifdef MISSIONPACK
	if( g_gametype.integer == GT_CTF || g_gametype.integer == GT_1FCTF ) {
#else	
	if( g_gametype.integer == GT_CTF) {
#endif
		Team_CheckHurtCarrier(targ, attacker);
	}

	if (targ->player) {
		// set the last player who damaged the target
		targ->player->lasthurt_player = attacker->s.number;
		targ->player->lasthurt_mod = mod;

		if ( attacker->player ) {
			targ->player->lasthurt_player2 = attacker->s.number;
			targ->player->lasthurt_time = level.time;
		}
	}

	if ( g_debugDamage.integer ) {
		G_Printf( "%i: player:%i health:%i damage:%i armor:%i location: %s\n", level.time, targ->s.number,
			targ->health, take, asave, G_LocationString(targ->player->lasthurt_location) );
	}

	// do the damage
	if (take) {
		targ->health = targ->health - take;
		if ( targ->player ) {
			targ->player->ps.stats[STAT_HEALTH] = targ->health;
		}
			
		if ( targ->health <= 0 ) {
			if ( player )
				targ->flags |= FL_NO_KNOCKBACK;

			if (targ->health < -999)
				targ->health = -999;

			targ->enemy = attacker;
			targ->die (targ, inflictor, attacker, take, mod);
			return;
		} else if ( targ->pain ) {
			targ->pain (targ, attacker, take);
		}
	}
}


/*
============
CanDamage

Returns qtrue if the inflictor can directly damage the target.  Used for
explosions and melee attacks.
============
*/
qboolean CanDamage (gentity_t *targ, vec3_t origin) {
	vec3_t	dest;
	trace_t	tr;
	vec3_t	midpoint;
	vec3_t	offsetmins = {-15, -15, -15};
	vec3_t	offsetmaxs = {15, 15, 15};

	// use the midpoint of the bounds instead of the origin, because
	// bmodels may have their origin is 0,0,0
	VectorAdd (targ->r.absmin, targ->r.absmax, midpoint);
	VectorScale (midpoint, 0.5, midpoint);

	VectorCopy(midpoint, dest);
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if (tr.fraction == 1.0 || tr.entityNum == targ->s.number)
		return qtrue;

	// this should probably check in the plane of projection, 
	// rather than in world coordinate
	VectorCopy(midpoint, dest);
	dest[0] += offsetmaxs[0];
	dest[1] += offsetmaxs[1];
	dest[2] += offsetmaxs[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if (tr.fraction == 1.0)
		return qtrue;

	VectorCopy(midpoint, dest);
	dest[0] += offsetmaxs[0];
	dest[1] += offsetmins[1];
	dest[2] += offsetmaxs[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if (tr.fraction == 1.0)
		return qtrue;

	VectorCopy(midpoint, dest);
	dest[0] += offsetmins[0];
	dest[1] += offsetmaxs[1];
	dest[2] += offsetmaxs[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if (tr.fraction == 1.0)
		return qtrue;

	VectorCopy(midpoint, dest);
	dest[0] += offsetmins[0];
	dest[1] += offsetmins[1];
	dest[2] += offsetmaxs[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if (tr.fraction == 1.0)
		return qtrue;

	VectorCopy(midpoint, dest);
	dest[0] += offsetmaxs[0];
	dest[1] += offsetmaxs[1];
	dest[2] += offsetmins[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if (tr.fraction == 1.0)
		return qtrue;

	VectorCopy(midpoint, dest);
	dest[0] += offsetmaxs[0];
	dest[1] += offsetmins[1];
	dest[2] += offsetmins[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if (tr.fraction == 1.0)
		return qtrue;

	VectorCopy(midpoint, dest);
	dest[0] += offsetmins[0];
	dest[1] += offsetmaxs[1];
	dest[2] += offsetmins[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if (tr.fraction == 1.0)
		return qtrue;

	VectorCopy(midpoint, dest);
	dest[0] += offsetmins[0];
	dest[1] += offsetmins[2];
	dest[2] += offsetmins[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if (tr.fraction == 1.0)
		return qtrue;

	return qfalse;
}


/*
============
G_RadiusDamage
============
*/
qboolean G_RadiusDamage ( vec3_t origin, gentity_t *attacker, float damage, float radius,
					 gentity_t *ignore, int mod) {
	float		points, dist;
	gentity_t	*ent;
	int			entityList[MAX_GENTITIES];
	int			numListedEntities;
	vec3_t		mins, maxs;
	vec3_t		v;
	vec3_t		dir;
	int			i, e;
	qboolean	hitPlayer = qfalse;

	if ( radius < 1 ) {
		radius = 1;
	}

	for ( i = 0 ; i < 3 ; i++ ) {
		mins[i] = origin[i] - radius;
		maxs[i] = origin[i] + radius;
	}

	numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( e = 0 ; e < numListedEntities ; e++ ) {
		ent = &g_entities[entityList[ e ]];

		if (ent == ignore)
			continue;
		if (!ent->takedamage)
			continue;

		// find the distance from the edge of the bounding box
		for ( i = 0 ; i < 3 ; i++ ) {
			if ( origin[i] < ent->r.absmin[i] ) {
				v[i] = ent->r.absmin[i] - origin[i];
			} else if ( origin[i] > ent->r.absmax[i] ) {
				v[i] = origin[i] - ent->r.absmax[i];
			} else {
				v[i] = 0;
			}
		}

		dist = VectorLength( v );
		if ( dist >= radius ) {
			continue;
		}

		points = damage * ( 1.0 - dist / radius );

		if( CanDamage (ent, origin) ) {
			if( LogAccuracyHit( ent, attacker ) ) {
				hitPlayer = qtrue;
			}
			VectorSubtract (ent->r.currentOrigin, origin, dir);
			// push the center of mass higher than the origin so players
			// get knocked into the air more
			dir[2] += 24;
			G_Damage (ent, NULL, attacker, dir, origin, (int)points, DAMAGE_RADIUS, mod);
		}
	}

	return hitPlayer;
}
