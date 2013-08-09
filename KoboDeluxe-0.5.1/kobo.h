/*
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright (C) 1995, 1996 Akira Higuchi
 * Copyright (C) 2001-2003, 2005, 2007 David Olofson
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _KOBO_H_
#define _KOBO_H_

#include "gfxengine.h"
#include "window.h"
#include "display.h"
#include "filemap.h"
#include "prefs.h"
#include "radar.h"
#include "dashboard.h"
#include "sound.h"
#include "region.h"

/*----------------------------------------------------------
	Singletons
----------------------------------------------------------*/
extern KOBO_sound	sound;


/*----------------------------------------------------------
	Globals
----------------------------------------------------------*/

class kobo_gfxengine_t : public gfxengine_t
{
	void frame();
	void pre_render();
	void post_render();
  public:
	kobo_gfxengine_t();
};

extern kobo_gfxengine_t		*gengine;
extern filemapper_t		*fmap;
extern prefs_t			*prefs;

extern screen_window_t		*wscreen;
extern dashboard_window_t	*wdash;
extern bargraph_t		*whealth;
extern bargraph_t		*wtemp;
extern bargraph_t		*wttemp;
extern radar_map_t		*wmap;
extern radar_window_t		*wradar;
extern window_t			*wmain;
extern display_t		*dhigh;
extern display_t		*dscore;
extern display_t		*dstage;
extern display_t		*dships;

extern RGN_region		*logo_region;

extern int mouse_x, mouse_y;
extern int mouse_left, mouse_middle, mouse_right;

extern int exit_game;


/* Sprite priority levels */
#define	LAYER_OVERLAY	0	// Mouse crosshair
#define	LAYER_BULLETS	1	// Bullets - most important!
#define	LAYER_FX	2	// Explosions and similar effects
#define	LAYER_PLAYER	3	// Player and fire bolts
#define	LAYER_ENEMIES	4	// Enemies
#define	LAYER_BASES	5	// Bases and stationary enemies

/* Graphics banks */
typedef enum
{
	B_TILES1 =	0,
	B_TILES2,
	B_TILES3,
	B_TILES4,
	B_TILES5,

	B_OLDSTARS,
	B_CROSSHAIR,
	B_PLAYER,
	B_BULLETS,
	B_BULLETEXPL,
	B_RING,
	B_RINGEXPL,
	B_BOMB,
	B_BOMBDETO,
	B_BOLT,
	B_BOLTEXPL,
	B_EXPLO1,
	B_EXPLO3,
	B_EXPLO4,
	B_EXPLO5,
	B_ROCK1,
	B_ROCK2,
	B_ROCK3,
	B_ROCKEXPL,
	B_BMR_GREEN,
	B_BMR_PURPLE,
	B_BMR_PINK,
	B_FIGHTER,
	B_MISSILE1,
	B_MISSILE2,
	B_MISSILE3,
	B_BIGSHIP,

	B_NOISE,
	B_HITNOISE,
	B_FOCUSFX,

	B_SCREEN,
	B_FRAME_TL,
	B_FRAME_TR,
	B_FRAME_BL,
	B_FRAME_BR,
	B_LOGO,
	B_LOGOMASK,
	B_LOGODELUXE,
	B_BRUSHES,

	B_HIGH_BACK,
	B_SCORE_BACK,
	B_RADAR_BACK,
	B_SHIPS_BACK,
	B_STAGE_BACK,

	B_HEALTH_LID,
	B_TEMP_LID,
	B_TTEMP_LID,

	B_LOADING,
	B_NORMAL_FONT,
	B_MEDIUM_FONT,
	B_BIG_FONT,
	B_COUNTER_FONT
} KOBO_Banks;

#define	NOALPHA_THRESHOLD	64

#define	INTRO_SCENE	-100000

typedef enum
{
	STARFIELD_NONE = 0,
	STARFIELD_OLD,
	STARFIELD_PARALLAX
} KOBO_StarfieldModes;

#endif // _KOBO_H_
