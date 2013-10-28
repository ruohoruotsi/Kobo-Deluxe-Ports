/*(GPL)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright (C) 1995, 1996 Akira Higuchi
 * Copyright (C) 2001-2003, 2005-2007 David Olofson
 * Copyright (C) 2005 Erik Auerswald
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

#define	DBG(x)	x

#undef	DEBUG_OUT

// Use this to benchmark and create a new percentage table!
#undef	TIME_PROGRESS

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/param.h> /* for MAXPATHLEN */
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>

#ifdef DEBUG
#include "audio.h"
extern "C" {
/*For the oscilloscope*/
#include "a_globals.h"
/*For VU and voice info*/
#include "a_struct.h"
}
#endif

#include "kobolog.h"
#include "config.h"
#include "kobo.h"
#include "states.h"
#include "screen.h"
#include "manage.h"
#include "score.h"
#include "gamectl.h"
#include "random.h"
#include "version.h"
#include "options.h"
#include "myship.h"
#include "enemies.h"


#include <CoreFoundation/CFBundle.h>


#define	MAX_FPS_RESULTS	64

/* Joystick support */
#define DEFAULT_JOY_LR		0	// Joystick axis left-right default
#define DEFAULT_JOY_UD		1	// Joystick axis up-down default
#define DEFAULT_JOY_FIRE	0	// Default fire button on joystick
#define DEFAULT_JOY_START	1

/* IOHAVOC: PATHS */
#define KOBO_DATA_DIR "$HOME/Library/Preferences"
#define KOBO_SCORE_DIR "/Library/Preferences/KoboDeluxe/scores"
#define KOBO_CONFIG_DIR "$HOME/Library/Preferences"
#define SYSCONF_DIR "$HOME/Library/Preferences"
#define KOBO_CONFIG_FILE "KoboDeluxe Preferences"

/*----------------------------------------------------------
	Singletons
----------------------------------------------------------*/
KOBO_sound		sound;


/*----------------------------------------------------------
	Globals
----------------------------------------------------------*/
filemapper_t		*fmap = NULL;
prefs_t			*prefs = NULL;
kobo_gfxengine_t	*gengine = NULL;

screen_window_t		*wscreen = NULL;
dashboard_window_t	*wdash = NULL;
bargraph_t		*whealth = NULL;
bargraph_t		*wtemp = NULL;
bargraph_t		*wttemp = NULL;
radar_map_t		*wmap = NULL;
radar_window_t		*wradar = NULL;
window_t		*wmain = NULL;
display_t		*dhigh = NULL;
display_t		*dscore = NULL;
display_t		*dstage = NULL;
display_t		*dships = NULL;
RGN_region		*logo_region = NULL;

int mouse_x = 0;
int mouse_y = 0;
int mouse_left = 0;
int mouse_middle = 0;
int mouse_right = 0;

int exit_game = 0;


static int main_init()
{
	prefs = new prefs_t;
	fmap = new filemapper_t;
	gengine = new kobo_gfxengine_t;
	return 0;
}


static void main_cleanup()
{
	delete gengine;
	gengine = NULL;
	delete fmap;
	fmap = NULL;
	delete prefs;
	prefs = NULL;
}


/*----------------------------------------------------------
	Various functions
----------------------------------------------------------*/

static void setup_dirs(char *xpath)
{
	fmap->exepath(xpath);

	fmap->addpath("DATA", KOBO_DATA_DIR);

	/*
	 * Graphics data
	 */
	/* Current dir; from within the build tree */
	fmap->addpath("GFX", "./data/gfx");
	/* Real data dir */
	fmap->addpath("GFX", "DATA>>gfx");
	/* Current dir */
	fmap->addpath("GFX", "./gfx");
    fmap->addpath("GFX", "./");

	/*
	 * Sound data
	 */
	/* Current dir; from within the build tree */
	fmap->addpath("SFX", "./data/sfx");
	/* Real data dir */
	fmap->addpath("SFX", "DATA>>sfx");
	/* Current dir */
	fmap->addpath("SFX", "./sfx");
    
    // When running as a standalone .app bundle, the Resources directory is our
    // current location (per SDLMain.m)  Add it to the SFX/GFX path
#if USE_RESOURCE_WORKING_DIR
    fmap->addpath("GFX", "./");
    fmap->addpath("SFX", "./");
#endif

	/*
	 * Score files (user and global)
	 */
	fmap->addpath("SCORES", KOBO_SCORE_DIR);
	/* 'scores' in current dir (For importing scores, perhaps...) */
// (Disabled for now, since filemapper_t can't tell
// when it hits the same dir more than once...)
//	fmap->addpath("SCORES", "./scores");

	/*
	 * Configuration files
	 */
	fmap->addpath("CONFIG", KOBO_CONFIG_DIR);
	/* System local */
	fmap->addpath("CONFIG", SYSCONF_DIR);
	/* In current dir (last resort) */
	fmap->addpath("CONFIG", "./");
}


static void add_dirs(prefs_t *p)
{
	char buf[300];
	if(p->dir[0])
	{
		char *upath = fmap->sys2unix(p->dir);
		snprintf(buf, 300, "%s/sfx", upath);
		fmap->addpath("SFX", buf, 1);
		snprintf(buf, 300, "%s/gfx", upath);
		fmap->addpath("GFX", buf, 1);
		snprintf(buf, 300, "%s/scores", upath);
		fmap->addpath("SCORES", buf, 1);
		snprintf(buf, 300, "%s", upath);
		fmap->addpath("CONFIG", buf, 0);
	}

	if(p->sfxdir[0])
		fmap->addpath("SFX", fmap->sys2unix(p->sfxdir), 1);

	if(p->gfxdir[0])
		fmap->addpath("GFX", fmap->sys2unix(p->gfxdir), 1);

	if(p->scoredir[0])
		fmap->addpath("SCORES", fmap->sys2unix(p->scoredir), 1);
}


#ifdef DEBUG
static void draw_osc(int mode)
{
	int mx = oscframes;
	if(mx > wmain->width())
		mx = wmain->width();
	int yo = wmain->height() - 40;
	wmain->foreground(wmain->map_rgb(0x000099));
	wmain->fillrect(0, yo, wmain->width(), 1);
	wmain->foreground(wmain->map_rgb(0x990000));
	wmain->fillrect(0, yo - 32, wmain->width(), 1);
	wmain->fillrect(0, yo + 31, wmain->width(), 1);

	switch(mode)
	{
	  case 0:
		wmain->foreground(wmain->map_rgb(0x009900));
		for(int s = 0; s < mx; ++s)
			wmain->point(s, ((oscbufl[s]+oscbufr[s]) >> 11) + yo);
		break;
	  case 1:
		wmain->foreground(wmain->map_rgb(0x009900));
		for(int s = 0; s < mx; ++s)
			wmain->point(s, (oscbufl[s] >> 10) + yo);
		wmain->foreground(wmain->map_rgb(0xcc0000));
		for(int s = 0; s < mx; ++s)
			wmain->point(s, (oscbufr[s] >> 10) + yo);
		break;
	  case 2:
		wmain->foreground(wmain->map_rgb(0x009900));
		for(int s = 0; s < mx; ++s)
			wmain->point(s>>1, (oscbufl[s] >> 10) + yo);
		wmain->foreground(wmain->map_rgb(0xcc0000));
		for(int s = 0; s < mx; ++s)
			wmain->point((s>>1) + (mx>>1), (oscbufr[s] >> 10) + yo);
		wmain->fillrect(mx/2, yo-34, 1, 68);
		break;
	}

	wmain->foreground(wmain->map_rgb(0xcc0000));
	wmain->fillrect(0, yo - 32, 6, limiter.attenuation >> 11);
}


static void draw_vu(void)
{
	int xo = (wmain->width() - 5 * AUDIO_MAX_VOICES) / 2;
	int yo = wmain->height() - 50;
	wmain->foreground(wmain->map_rgb(0x000000));
	for(int s = 4; s < 40; s += 4)
		wmain->fillrect(xo, yo+s, 5 * AUDIO_MAX_VOICES-1, 1);
	wmain->foreground(wmain->map_rgb(0x333333));
	wmain->fillrect(xo-1, yo, 5 * AUDIO_MAX_VOICES+1, 1);
	wmain->fillrect(xo-1, yo+40, 5 * AUDIO_MAX_VOICES+1, 4);
	for(int s = 0; s <= AUDIO_MAX_VOICES; ++s)
		wmain->fillrect(xo + s*5 - 1, yo+1, 1, 39);
	for(int s = 0; s < AUDIO_MAX_VOICES; ++s)
	{
		int vu, vu2, vumin;
		if(VS_STOPPED != voicetab[s].state)
		{
			wmain->foreground(wmain->map_rgb(0x009900));
			wmain->fillrect(xo + 1, yo + 41, 2, 2);

			vu = labs((voicetab[s].ic[VIC_LVOL].v>>1) +
					(voicetab[s].ic[VIC_RVOL].v>>1)) >>
					RAMP_BITS;
#ifdef AUDIO_USE_VU
			vu2 = voicetab[s].vu;
			voicetab[s].vu = 0;
#else
			vu2 = 0;
#endif
			vu2 = (vu2>>4) * (vu>>4) >> 8;
			vu2 >>= 11;
			if(vu2 > 40)
				vu2 = 40;
			vu >>= 11;
			if(vu > 40)
				vu = 40;
			vumin = vu < vu2 ? vu : vu2;
		}
		else
			vu = vu2 = vumin = 0;
		wmain->foreground(wmain->map_rgb(0x006600));
		wmain->fillrect(xo, yo + 40 - vu, 4, vu - vu2);
		wmain->foreground(wmain->map_rgb(0xffcc00));
		wmain->fillrect(xo, yo + 40 - vu2, 4, vu2);
		xo += 5;
	}
}
#endif


/*----------------------------------------------------------
	The main object
----------------------------------------------------------*/
class KOBO_main
{
  public:
#ifdef DEBUG
	static int		audio_vismode;
#endif
	static SDL_Joystick	*joystick;
	static int		js_lr;
	static int		js_ud;
	static int		js_fire;
	static int		js_start;

	static FILE		*logfile;

	static Uint32		esc_tick;
	static int		esc_count;
	static int		exit_game_fast;

	// Frame rate counter
	static int		fps_count;
	static int		fps_starttime;
	static int		fps_nextresult;
	static int		fps_lastresult;
	static float		*fps_results;
	static display_t	*dfps;

	// Frame rate limiter
	static float		max_fps_filter;
	static int		max_fps_begin;

	static int		xoffs;
	static int		yoffs;

	// Backup in case we screw up we can't get back up
	static prefs_t		safe_prefs;

	static int open();
	static void close();
	static int run();

	static int open_logging(prefs_t *p);
	static void close_logging();
	static void load_config(prefs_t *p);
	static void save_config(prefs_t *p);

	static void build_screen();
	static int init_display(prefs_t *p);
	static void close_display();

	static void show_progress(prefs_t *p);
	static void progress();
	static void doing(const char *msg);
	static int load_graphics(prefs_t *p);
	static int load_sounds(prefs_t *p, int render_all = 0);

	static int init_js(prefs_t *p);
	static void close_js();

	static int escape_hammering();
	static int quit_requested();
	static void brutal_quit();
	static void pause_game();

	static void print_fps_results();
};

#ifdef DEBUG
int		KOBO_main::audio_vismode = 0;
#endif

SDL_Joystick	*KOBO_main::joystick = NULL;
int		KOBO_main::js_lr = DEFAULT_JOY_LR;
int		KOBO_main::js_ud = DEFAULT_JOY_UD;
int		KOBO_main::js_fire = DEFAULT_JOY_FIRE;
int		KOBO_main::js_start = DEFAULT_JOY_START;

FILE		*KOBO_main::logfile = NULL;

Uint32		KOBO_main::esc_tick = 0;
int		KOBO_main::esc_count = 0;
int		KOBO_main::exit_game_fast = 0;

int		KOBO_main::fps_count = 0;
int		KOBO_main::fps_starttime = 0;
int		KOBO_main::fps_nextresult = 0;
int		KOBO_main::fps_lastresult = 0;
float		*KOBO_main::fps_results = NULL;
display_t	*KOBO_main::dfps = NULL;

float		KOBO_main::max_fps_filter = 0.0f;
int		KOBO_main::max_fps_begin = 0;

int		KOBO_main::xoffs = 0;
int		KOBO_main::yoffs = 0;

prefs_t		KOBO_main::safe_prefs;


static KOBO_main km;


void KOBO_main::print_fps_results()
{
	int i, r = fps_nextresult;
	if(fps_lastresult != MAX_FPS_RESULTS-1)
		r = 0;
	for(i = 0; i < fps_lastresult; ++i)
	{
		log_printf(ULOG, "%.1f fps\n", fps_results[r++]);
		if(r >= MAX_FPS_RESULTS)
			r = 0;
	}

	free(fps_results);
	fps_nextresult = 0;
	fps_lastresult = 0;
	fps_results = NULL;
}


int KOBO_main::escape_hammering()
{
	Uint32 nt = SDL_GetTicks();
	if(nt - esc_tick > 300)
		esc_count = 1;
	else
		++esc_count;
	esc_tick = nt;
	return esc_count >= 5;
}


int KOBO_main::quit_requested()
{
	SDL_Event e;
	while(SDL_PollEvent(&e))
	{
		switch(e.type)
		{
		  case SDL_QUIT:
			exit_game_fast = 1;
			break;
        // SDL_VIDEOEXPOSE generated by wake from sleep? IOHAVOC removing. Is SDL_APP_WILLENTERFOREGROUND a replacement?
        //    case SDL_VIDEOEXPOSE
		//	gengine->invalidate();
		//	break;
		  case SDL_KEYUP:
			switch(e.key.keysym.sym)
			{
			  case SDLK_ESCAPE:
				if(escape_hammering())
					exit_game_fast = 1;
				break;
			  default:
				break;
			}
			break;
		}
	}
	if(exit_game_fast)
		return 1;
	return SDL_QuitRequested();
}


void KOBO_main::close_logging()
{
	/* Flush logs to disk, close log files etc. */
	log_close();

	if(logfile)
	{
		fclose(logfile);
		logfile = NULL;
	}
}


int KOBO_main::open_logging(prefs_t *p)
{
	close_logging();

	if(log_open() < 0)
		return -1;

	if(p && p->logfile)
		switch (p->logformat)
		{
		  case 2:
			logfile = fopen("log.html", "wb");
			break;
		  default:
			logfile = fopen("log.txt", "wb");
			break;
		}

	if(logfile)
	{
		log_set_target_stream(0, logfile);
		log_set_target_stream(1, logfile);
	}
	else
	{
		log_set_target_stream(0, stdout);
		log_set_target_stream(1, stderr);
	}

    // IOHAVOC
	// log_set_target_stream(2, NULL);

	if(p)
		switch(p->logformat)
		{
		  default:
			log_set_target_flags(-1, LOG_TIMESTAMP);
			break;
		  case 1:
			log_set_target_flags(-1, LOG_ANSI | LOG_TIMESTAMP);
			break;
		  case 2:
			log_set_target_flags(-1, LOG_HTML | LOG_TIMESTAMP);
			break;
		}

	/* All levels output to stdout... */
	log_set_level_target(-1, 0);

	/* ...except these, that output to stderr. */
	log_set_level_target(ELOG, 1);
	log_set_level_target(CELOG, 1);

	/* Some fancy colors... */
	log_set_level_attr(ULOG, LOG_YELLOW);
	log_set_level_attr(WLOG, LOG_YELLOW | LOG_BRIGHT);
	log_set_level_attr(ELOG, LOG_RED | LOG_BRIGHT);
	log_set_level_attr(CELOG, LOG_RED | LOG_BRIGHT | LOG_BLINK);
	log_set_level_attr(DLOG, LOG_CYAN);
	log_set_level_attr(D2LOG, LOG_BLUE | LOG_BRIGHT);
	log_set_level_attr(D3LOG, LOG_BLUE);

	/* Disable levels as desired */
	if(p)
		switch(p->logverbosity)
		{
		  case 0:
			log_set_level_target(ELOG, 2);
		  case 1:
			log_set_level_target(WLOG, 2);
		  case 2:
			log_set_level_target(DLOG, 2);
		  case 3:
			log_set_level_target(D2LOG, 2);
		  case 4:
			log_set_level_target(D3LOG, 2);
		  case 5:
			break;
		}

	if(p && p->logfile && !logfile)
		log_printf(ELOG, "Couldn't open log file!\n");

	return 0;
}


void KOBO_main::build_screen()
{
	gengine->clear(0x000000);

	wdash->place(xoffs, yoffs, SCREEN_WIDTH, SCREEN_HEIGHT);

	whealth->place(xoffs + 4, yoffs + 92, 8, 128);
	whealth->bgimage(B_HEALTH_LID, 0);
	whealth->background(whealth->map_rgb(0x182838));
	whealth->redmax(0);

	wmain->place(xoffs + 8 + MARGIN, yoffs + MARGIN, WSIZE, WSIZE);
	gengine->output(wmain);

	dhigh->place(xoffs + 252, yoffs + 4, 64, 18);
	dhigh->font(B_NORMAL_FONT);
	dhigh->bgimage(B_HIGH_BACK, 0);
	dhigh->caption("HIGHSCORE");
	dhigh->text("000000000");

	dscore->place(dhigh->x(), dhigh->y2() + 4, 64, 18);
	dscore->font(B_NORMAL_FONT);
	dscore->bgimage(B_SCORE_BACK, 0);
	dscore->caption("SCORE");
	dscore->text("000000000");

	wmap->place(0, 0, MAP_SIZEX, MAP_SIZEY);
	wmap->offscreen();

	wradar->place(xoffs + 244,
			yoffs + (SCREEN_HEIGHT - MAP_SIZEY) / 2,
			MAP_SIZEX, MAP_SIZEY);
	wradar->bgimage(B_RADAR_BACK, 0);

	wtemp->place(xoffs + 244, yoffs + 188, 4, 32);
	wtemp->bgimage(B_TEMP_LID, 0);
	wtemp->background(wtemp->map_rgb(0x182838));
	wtemp->redmax(1);

	wttemp->place(xoffs + 248, yoffs + 188, 4, 32);
	wttemp->bgimage(B_TTEMP_LID, 0);
	wttemp->background(wttemp->map_rgb(0x182838));
	wttemp->redmax(1);

	dships->place(xoffs + 264, yoffs + 196, 38, 18);
	dships->font(B_NORMAL_FONT);
	dships->bgimage(B_SHIPS_BACK, 0);
	dships->caption("SHIPS");
	dships->text("000");

	dstage->place(dships->x(), dships->y2() + 4, 38, 18);
	dstage->font(B_NORMAL_FONT);
	dstage->bgimage(B_STAGE_BACK, 0);
	dstage->caption("STAGE");
	dstage->text("000");

	if(prefs->cmd_fps)
	{
		dfps = new display_t;
		dfps->init(gengine);
		dfps->place(0, -9, 48, 18);
		dfps->color(wdash->map_rgb(0, 0, 0));
		dfps->font(B_NORMAL_FONT);
		dfps->caption("FPS");
	}
}


int KOBO_main::init_display(prefs_t *p)
{
	int dw, dh;		// Display size
	int gw, gh;		// Game "window" size
	gengine->title("Kobo Deluxe " VERSION, "kobodl");
	gengine->driver((gfx_drivers_t)p->videodriver);

	dw = p->width;
	dh = p->height;
	if(p->fullscreen)
	{
		// This game assumes 1:1 pixel aspect ratio, or 4:3
		// width:height ratio, so we need to adjust accordingly.
		// Note:
		//	This code assumes 1:1 pixels for all resolutions!
		//	This does not hold true for 1280x1024 or a CRT
		//	for example, but that's an incorrect (although
		//	all too common) setup anyway. (1280x1024 is for
		//	5:4 TFT displays only!)
		if(dw * 3 >= dh * 4)
		{
			// 4:3 or widescreen; Height defines size
			gw = dh * 4 / 3;
			gh = dh;
		}
		else
		{
			// "tallscreen" (probably 5:4 TFT or rotated display)
			// Width defines size
			gw = dw;
			gh = dw * 3 / 4;
		}
	}
	else
	{
		gw = dw;
		gh = dh;
	}

	// Scaling has 16ths granularity, so tiles scale properly!
	gengine->scale((int)((gw * 16 + 8) / SCREEN_WIDTH) / 16.f,
			(int)((gh * 16 + 8) / SCREEN_HEIGHT) / 16.f);

	// Read back and recalculate, in case the engine has some ideas...
	gw = (int)(SCREEN_WIDTH * gengine->xscale() + 0.5f);
	gh = (int)(SCREEN_HEIGHT * gengine->yscale() + 0.5f);

	if(!p->fullscreen)
	{
		//Add thin black border around the game "screen" in windowed mode.
		dw = gw + 8;
		dh = gh + 8;
	}

	xoffs = (int)((dw - gw) / 2 / gengine->xscale());
	yoffs = (int)((dh - gh) / 2 / gengine->yscale());
	gengine->size(dw, dh);

	gengine->mode(0, p->fullscreen);
	gengine->doublebuffer(p->doublebuf);
	gengine->pages(p->pages);
	gengine->vsync(p->vsync);
	gengine->shadow(p->shadow);
	gengine->cursor(0);

	gengine->period(game.speed);
	sound.period(game.speed);
	gengine->timefilter(p->timefilter * 0.01f);
	gengine->interpolation(p->filter);

	gengine->scroll_ratio(LAYER_OVERLAY, 0.0, 0.0);
	gengine->scroll_ratio(LAYER_BULLETS, 1.0, 1.0);
	gengine->scroll_ratio(LAYER_FX, 1.0, 1.0);
	gengine->scroll_ratio(LAYER_PLAYER, 1.0, 1.0);
	gengine->scroll_ratio(LAYER_ENEMIES, 1.0, 1.0);
	gengine->scroll_ratio(LAYER_BASES, 1.0, 1.0);
	gengine->wrap(MAP_SIZEX * CHIP_SIZEX, MAP_SIZEY * CHIP_SIZEY);

	if(gengine->open(ENEMY_MAX) < 0)
		return -1;

	gengine->clear();

	wscreen = new screen_window_t;
	wscreen->init(gengine);
	wscreen->place(0, 0,
			(int)(gengine->width() / gengine->xscale() + 0.5f),
			(int)(gengine->height() / gengine->yscale() + 0.5f));
	wscreen->border((int)(yoffs * gengine->yscale() + 0.5f),
			(int)(xoffs * gengine->xscale() + 0.5f),
			dw - gw - (int)(xoffs * gengine->xscale() + 0.5f),
			dh - gh - (int)(yoffs * gengine->yscale() + 0.5f));

	wdash = new dashboard_window_t;
	wdash->init(gengine);
	whealth = new bargraph_t;
	whealth->init(gengine);
	wmain = new window_t;
	wmain->init(gengine);
	dhigh = new display_t;
	dhigh->init(gengine);
	dscore = new display_t;
	dscore->init(gengine);
	wmap = new radar_map_t;
	wmap->init(gengine);
	wradar = new radar_window_t;
	wradar->init(gengine);
	wtemp = new bargraph_t;
	wtemp->init(gengine);
	wttemp = new bargraph_t;
	wttemp->init(gengine);
	dships = new display_t;
	dships->init(gengine);
	dstage = new display_t;
	dstage->init(gengine);

	build_screen();

	wdash->mode(DASHBOARD_BLACK);

	return 0;
}


void KOBO_main::close_display()
{
	delete dfps;
	dfps = NULL;
	delete dstage;
	dstage = NULL;
	delete dships;
	dships = NULL;
	delete wttemp;
	wttemp = NULL;
	delete wtemp;
	wtemp = NULL;
	delete wradar;
	wradar = NULL;
	delete wmap;
	wmap = NULL;
	delete dscore;
	dscore = NULL;
	delete dhigh;
	dhigh = NULL;
	delete wmain;
	wmain = NULL;
	delete whealth;
	whealth = NULL;
	delete wdash;
	wdash = NULL;
	delete wscreen;
	wscreen = NULL;
}


void KOBO_main::show_progress(prefs_t *p)
{
	wdash->mode(DASHBOARD_LOADING);
	wdash->doing("");
}


void KOBO_main::progress()
{
	wdash->progress();
}


void KOBO_main::doing(const char *msg)
{
	wdash->doing(msg);
}


#ifndef TIME_PROGRESS
static float progtab_graphics[] = {
	0.000000,
	1.167315,
	4.085603,
	5.836576,
	7.490273,
	9.046693,
	10.992218,
	12.937743,
	13.813230,
	15.564202,
	17.217899,
	18.774319,
	20.428015,
	22.081713,
	23.540855,
	25.291828,
	26.848249,
	28.404669,
	29.961090,
	32.003891,
	33.560310,
	35.311283,
	36.770428,
	38.521400,
	40.369648,
	41.828793,
	43.190662,
	44.649807,
	46.595329,
	48.151752,
	49.610893,
	51.361866,
	53.501945,
	58.852139,
	60.116730,
	61.089493,
	63.521400,
	66.050583,
	68.287941,
	69.844360,
	72.665367,
	74.124512,
	76.653694,
	77.334633,
	79.182877,
	80.642021,
	82.295723,
	83.852142,
	85.505836,
	87.159531,
	88.813232,
	90.369652,
	92.023346,
	93.677040,
	95.233459,
	96.984436,
	98.540855,
	100.0,
	0
};

static float progtab_sounds[] = {
	0.000000,
	58.709679,
	59.516129,
	98.790321,
	98.790321,
	100.0,
	0
};

static float progtab_all[] = {
	0.000000,
	1.556684,
	2.639594,
	3.282572,
	4.060914,
	4.737733,
	5.380711,
	8.697124,
	8.764806,
	10.016920,
	10.761421,
	11.472081,
	12.216582,
	12.893401,
	13.604061,
	14.416244,
	15.093062,
	15.262267,
	16.074450,
	17.428087,
	18.680202,
	19.932318,
	20.879864,
	21.759729,
	23.011845,
	24.365482,
	24.534687,
	24.839256,
	25.651438,
	26.429781,
	26.700508,
	27.241962,
	29.678511,
	38.477158,
	40.203045,
	40.372250,
	40.913704,
	42.571911,
	45.211506,
	45.922165,
	46.565144,
	46.903553,
	48.866329,
	48.967850,
	49.306259,
	49.847717,
	50.389172,
	50.964466,
	51.641285,
	52.115059,
	52.690357,
	53.231812,
	53.773266,
	54.382404,
	54.991539,
	55.499153,
	56.006767,
	57.495770,
	82.673431,
	82.978004,
	99.763115,
	99.796951,
	100.0,
	0
};
#endif


typedef enum
{
	KOBO_CLAMP =		0x0001,	// Clamp to frame edge pixels
	KOBO_CLAMP_OPAQUE =	0x0002,	// Clamp to black instead of transparent
	KOBO_NOALPHA =		0x0004,	// Disable alpha channel
	KOBO_NODITHER =		0x0008,	// Disable dithering
	KOBO_NEAREST =		0x0010,	// Force NEAREST scale mode
	KOBO_CENTER =		0x0020,	// Center hotspot in frames
	KOBO_NOBRIGHT =		0x0040,	// Disable brightness/contrast filter
	KOBO_FONT =		0x0080,	// Load as "SFont" rather than tiles!
	KOBO_MESSAGE =		0x1000	// Not a file! 'path' is a message.
} KOBO_GfxDescFlags;

typedef struct KOBO_GfxDesc
{
	const char	*path;		// Path + name (for filemapper)
	int		bank;		// Sprite bank ID
	int		w, h;		// Frame size (original image pixels)
	float		scale;		// Scale of source data rel. 320x240
	int		flags;		// KOBO_GfxDescFlags
} KOBO_GfxDesc;

static KOBO_GfxDesc gfxdesc[] = {
	// Loading screen
	{ "Loading loading screen graphics", 0, 0,0, 0.0f, KOBO_MESSAGE },
	{ "GFX>>loading3.png", B_LOADING,	0, 0,	2.0f,
			KOBO_CLAMP | KOBO_NOALPHA },
	{ "GFX>>icefont2.png", B_NORMAL_FONT,	0, 0,	2.0f,	KOBO_FONT },

	// In-game
	{ "Loading in-game graphics", 0, 0,0, 0.0f, KOBO_MESSAGE },
	{ "GFX>>tiles-green.png", B_TILES1,	32, 32,	2.0f,	KOBO_CLAMP },
	{ "GFX>>tiles-metal.png", B_TILES2,	32, 32,	2.0f,	KOBO_CLAMP },
	{ "GFX>>tiles-blood.png", B_TILES3,	32, 32,	2.0f,	KOBO_CLAMP },
	{ "GFX>>tiles-double.png", B_TILES4,	32, 32,	2.0f,	KOBO_CLAMP },
	{ "GFX>>tiles-chrome.png", B_TILES5,	32, 32,	2.0f,	KOBO_CLAMP },
	{ "GFX>>flatstars1.png", B_OLDSTARS,	32, 32,	2.0f,	KOBO_CLAMP },
	{ "GFX>>crosshair.png", B_CROSSHAIR,	32, 32,	2.0f,	KOBO_CENTER },
	{ "GFX>>player.png", B_PLAYER,		40, 40,	2.0f,	KOBO_CENTER },
	{ "GFX>>bmr-green.png", B_BMR_GREEN,	40, 40,	2.0f,	KOBO_CENTER },
	{ "GFX>>bmr-purple.png", B_BMR_PURPLE,	40, 40,	2.0f,	KOBO_CENTER },
	{ "GFX>>bmr-pink.png", B_BMR_PINK,	40, 40,	2.0f,	KOBO_CENTER },
	{ "GFX>>fighter.png", B_FIGHTER,	40, 40,	2.0f,	KOBO_CENTER },
	{ "GFX>>missile.png", B_MISSILE1,	40, 40,	2.0f,	KOBO_CENTER },
	{ "GFX>>missile2.png", B_MISSILE2,	40, 40,	2.0f,	KOBO_CENTER },
	{ "GFX>>missile3.png", B_MISSILE3,	40, 40,	2.0f,	KOBO_CENTER },
	{ "GFX>>bolt.png", B_BOLT,		16, 16,	2.0f,	KOBO_CENTER },
	{ "GFX>>boltexpl.png", B_BOLTEXPL,	32, 32,	2.0f,	KOBO_CENTER },
	{ "GFX>>explo1e.png", B_EXPLO1,		48, 48,	2.0f,	KOBO_CENTER },
	{ "GFX>>explo3e.png", B_EXPLO3,		64, 64,	2.0f,	KOBO_CENTER },
	{ "GFX>>explo4e.png", B_EXPLO4,		64, 64,	2.0f,	KOBO_CENTER },
	{ "GFX>>explo5e.png", B_EXPLO5,		64, 64,	2.0f,	KOBO_CENTER },
	{ "GFX>>rock1c.png", B_ROCK1,		32, 32,	2.0f,	KOBO_CENTER },
	{ "GFX>>rock2.png", B_ROCK2,		32, 32,	2.0f,	KOBO_CENTER },
	{ "GFX>>shinyrock.png", B_ROCK3,	32, 32,	2.0f,	KOBO_CENTER },
	{ "GFX>>rockexpl.png", B_ROCKEXPL,	64, 64,	2.0f,	KOBO_CENTER },
	{ "GFX>>bullet5b.png", B_BULLETS,	16, 16,	2.0f,	KOBO_CENTER },
	{ "GFX>>bulletexpl2.png", B_BULLETEXPL,	32, 32,	2.0f,	KOBO_CENTER },
	{ "GFX>>ring.png", B_RING,		32, 32,	2.0f,	KOBO_CENTER },
	{ "GFX>>ringexpl2b.png", B_RINGEXPL,	40, 40,	2.0f,	KOBO_CENTER },
	{ "GFX>>bomb.png", B_BOMB,		24, 24,	2.0f,	KOBO_CENTER },
	{ "GFX>>bombdeto.png", B_BOMBDETO,	40, 40,	2.0f,	KOBO_CENTER },
	{ "GFX>>bigship.png", B_BIGSHIP,	72, 72,	2.0f,	KOBO_CENTER },

	// Framework
	{ "Loading framework graphics", 0, 0,0, 0.0f, KOBO_MESSAGE },
	{ "GFX>>screen2.png", B_SCREEN,	0, 0,	2.0f,	KOBO_CLAMP_OPAQUE },

	// Logo
	{ "Loading logo", 0, 0,0, 0.0f, KOBO_MESSAGE },
	{ "GFX>>logo-outline.png", B_LOGO,	0, 0,	2.0f,	0 },
	{ "GFX>>deluxe.png", B_LOGODELUXE,	0, 0,	2.0f,	0 },
	{ "GFX>>logomask3.png", B_LOGOMASK,	0, 0,	4.0f,
			KOBO_NEAREST | KOBO_NODITHER },

	// Fonts
	{ "Loading fonts", 0, 0,0, 0.0f, KOBO_MESSAGE },
	{ "GFX>>goldfont.png", B_MEDIUM_FONT,	0, 0,	2.0f,	KOBO_FONT },
	{ "GFX>>bigfont3.png", B_BIG_FONT,	0, 0,	2.0f,	KOBO_FONT },
	{ "GFX>>counterfont.png", B_COUNTER_FONT, 0, 0,	2.0f,	KOBO_FONT },

	// Special FX
	{ "Loading special FX graphics", 0, 0,0, 0.0f, KOBO_MESSAGE },
	{ "GFX>>noise.png", B_NOISE,		NOISE_SIZEX, 1,	1.0f,
			KOBO_CLAMP },
	{ "GFX>>hitnoise.png", B_HITNOISE,	NOISE_SIZEX, 1,	1.0f,
			KOBO_CLAMP | KOBO_NEAREST },
	{ "GFX>>focusfx.png", B_FOCUSFX,	0, 0,	1.0f, KOBO_CLAMP },
	{ "GFX>>brushes.png", B_BRUSHES,	16, 16,	1.0f,
			KOBO_CLAMP | KOBO_NEAREST | KOBO_NOALPHA },

	{ NULL, 0,	0, 0,	0.0f,	0 }	// Terminator
};


int KOBO_main::load_graphics(prefs_t *p)
{
	KOBO_GfxDesc *gd;
	gengine->reset_filters();
	show_progress(p);
	for(gd = gfxdesc; gd->path; ++gd)
	{
		if(gd->flags & KOBO_MESSAGE)
		{
			doing(gd->path);
			continue;
		}

		// Set scale (filter) mode and clamping
		int clamping;
		gfx_scalemodes_t sm;
		if(gd->flags & KOBO_CLAMP)
		{
			gengine->clampcolor(0, 0, 0, 0);
			clamping = 1;
		}
		else if(gd->flags & KOBO_CLAMP_OPAQUE)
		{
			gengine->clampcolor(0, 0, 0, 255);
			clamping = 1;
		}
		else
			clamping = 0;
		if(gd->flags & KOBO_NEAREST)
			sm = GFX_SCALE_NEAREST;
		else
			sm = (gfx_scalemodes_t) p->scalemode;
		gengine->scalemode(sm, clamping);

		// Disable brightness filter?
		if(gd->flags & KOBO_NOBRIGHT)
			gengine->brightness(1.0f, 1.0f);
		else
			gengine->brightness(0.01f * p->brightness,
					0.01f * p->contrast);

		// Dithering
		if(gd->flags & KOBO_NODITHER || !p->use_dither)
			gengine->dither(-1);
		else
			gengine->dither(p->dither_type, p->broken_rgba8);

		// Alpha channels
		if(gd->flags & KOBO_NOALPHA || !p->alpha)
			gengine->noalpha(NOALPHA_THRESHOLD);
		else
			gengine->noalpha(0);

		// Source image scale
		gengine->source_scale(gd->scale, gd->scale);

		// Load!
		const char *fn = fmap->get(gd->path);
		if(!fn)
		{
			log_printf(ELOG, "Couldn't get path to \"%s\"!\n",
					gd->path);
			return -1;
		}
		int res;
		if(gd->flags & KOBO_FONT)
			res = gengine->loadfont(gd->bank, fn);
		else if(!gd->w || !gd->h)
			res = gengine->loadimage(gd->bank, fn);
		else
			res = gengine->loadtiles(gd->bank, gd->w, gd->h, fn);
		if(res < 0)
		{
			log_printf(ELOG, "Couldn't load \"%s\"!\n", fn);
			return -1;
		}

		// Hotspot
		if(gd->flags & KOBO_CENTER)
			gengine->set_hotspot(gd->bank, -1,
					(int)(gd->w / gd->scale / 2),
					(int)(gd->h / gd->scale / 2));

		// Update progress bar
		progress();
	}

	// Chop up the dashboard graphics as needed
	SDL_Rect r;
	r.w = r.h = 16;
	r.x = (8 + MARGIN);
	r.y = MARGIN;
	if(gengine->copyrect(B_FRAME_TL, B_SCREEN, 0, &r) < 0)
		return -6;
	progress();
	r.x += WSIZE - 16;
	if(gengine->copyrect(B_FRAME_TR, B_SCREEN, 0, &r) < 0)
		return -7;
	progress();
	r.x = (8 + MARGIN);
	r.y = MARGIN + WSIZE - 16;
	if(gengine->copyrect(B_FRAME_BL, B_SCREEN, 0, &r) < 0)
		return -8;
	progress();
	r.x += WSIZE - 16;
	if(gengine->copyrect(B_FRAME_BR, B_SCREEN, 0, &r) < 0)
		return -9;
	progress();
	r.x = 252;
	r.y = 4;
	r.w = 64;
	r.h = 18;
	if(gengine->copyrect(B_HIGH_BACK, B_SCREEN, 0, &r) < 0)
		return -26;
	progress();
	r.y += 18 + 4;
	if(gengine->copyrect(B_SCORE_BACK, B_SCREEN, 0, &r) < 0)
		return -27;
	progress();
	r.x = 244;
	r.y = (SCREEN_HEIGHT - MAP_SIZEY) / 2;
	r.w = MAP_SIZEX;
	r.h = MAP_SIZEY;
	if(gengine->copyrect(B_RADAR_BACK, B_SCREEN, 0, &r) < 0)
		return -28;
	progress();
	r.x = 264;
	r.y = 196;
	r.w = 38;
	r.h = 18;
	if(gengine->copyrect(B_SHIPS_BACK, B_SCREEN, 0, &r) < 0)
		return -29;
	progress();
	r.y += 18 + 4;
	if(gengine->copyrect(B_STAGE_BACK, B_SCREEN, 0, &r) < 0)
		return -30;
	progress();
	r.x = 4;
	r.y = 92;
	r.w = 8;
	r.h = 128;
	if(gengine->copyrect(B_HEALTH_LID, B_SCREEN, 0, &r) < 0)
		return -31;
	progress();
	r.x = 244;
	r.y = 188;
	r.w = 4;
	r.h = 32;
	if(gengine->copyrect(B_TEMP_LID, B_SCREEN, 0, &r) < 0)
		return -32;
	progress();
	r.x += 4;
	if(gengine->copyrect(B_TTEMP_LID, B_SCREEN, 0, &r) < 0)
		return -33;
	progress();

	// Prepare the logo region
	s_sprite_t *s = gengine->get_sprite(B_LOGOMASK, 0);
	if(!s || !s->surface)
		return -10;
	RGN_FreeRegion(logo_region);
	logo_region = RGN_ScanMask(s->surface,
			SDL_MapRGB(s->surface->format, 255, 255, 255));
	if(!logo_region)
	{
		log_printf(ELOG, "Could not create logo region!\n");
		return -92;
	}
	progress();

	return 0;
}


static int progress_cb(const char *msg)
{
	if(msg)
		km.doing(msg);
	km.progress();
	if(km.quit_requested())
		return -999;
	return 0;
}


int KOBO_main::load_sounds(prefs_t *p, int render_all)
{
	if(!p->use_sound)
		return 0;
	show_progress(p);
	return sound.load(progress_cb, render_all);
}


int KOBO_main::init_js(prefs_t *p)
{
	/* Activate Joystick sub-sys if we are using it */
	if(p->use_joystick)
	{
		if(SDL_InitSubSystem(SDL_INIT_JOYSTICK) < 0)
		{
			log_printf(ELOG, "Error setting up joystick!\n");
			return -1;
		}
		p->number_of_joysticks = SDL_NumJoysticks();
		if(p->number_of_joysticks > 0)
		{
			SDL_JoystickEventState(SDL_ENABLE);
			if(p->joystick_no >= p->number_of_joysticks)
				p->joystick_no = 0;
			joystick = SDL_JoystickOpen(p->joystick_no);
			if(!joystick)
			{
				SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
				return -2;
			}
		}
		else
		{
			log_printf(ELOG, "No joysticks found!\n");
			joystick = NULL;
			return -3;
		}

	}
	return 0;
}


void KOBO_main::close_js()
{
	if(!SDL_WasInit(SDL_INIT_JOYSTICK))
		return;

	if(!joystick)
		return;

	//if(SDL_JoystickOpened(0))     // IOHAVOC
	if(SDL_JoystickGetAttached(joystick))
        SDL_JoystickClose(joystick);
	joystick = NULL;

	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
}


void KOBO_main::load_config(prefs_t *p)
{
	FILE *f = fmap->fopen(KOBO_CONFIG_DIR "/" KOBO_CONFIG_FILE, "r");
	if(f)
	{
		p->read(f);
		fclose(f);
	}
}


void KOBO_main::save_config(prefs_t *p)
{
	FILE *f;
#if defined(HAVE_GETEGID) && defined(HAVE_SETGID)
	gid_t oldgid = getegid();
	if(setgid(getgid()) != 0)
	{
		log_printf(ELOG, "Cannot save config! (%s)\n",
				strerror(errno));
		return;
	}
#endif
	f = fmap->fopen(KOBO_CONFIG_DIR "/" KOBO_CONFIG_FILE, "w");
	if(f)
	{
		p->write(f);
		fclose(f);
	}
#if defined(HAVE_GETEGID) && defined(HAVE_SETGID)
	if(setgid(oldgid) != 0)
	{
		log_printf(ELOG, "Cannot restore GID! (%s)\n",
				strerror(errno));
		return;
	}
#endif
}


void KOBO_main::brutal_quit()
{
	if(exit_game_fast)
	{
		log_printf(ULOG, "Second try quitting; using brutal method!\n");
		atexit(SDL_Quit);
		close_logging();
		exit(1);
	}

	exit_game_fast = 1;
	if(gengine)
		gengine->stop();
}


void KOBO_main::pause_game()
{
	if(gsm.current() != &st_pause_game)
		gsm.press(BTN_PAUSE);
}


static void kobo_render_highlight(ct_widget_t *wg)
{
	screen.set_highlight(wg->y() + wg->height() / 2 - wmain->y(),
			wg->height());
}


int KOBO_main::open()
{
	if(init_display(prefs) < 0)
		return -1;

#ifdef TIME_PROGRESS
	wdash->progress_init(NULL);
#else
	wdash->progress_init(progtab_all);
#endif

	if(load_graphics(prefs) < 0)
		return -2;

#ifdef TIME_PROGRESS
	// Use these two lines to time graphics and sounds separately!
	wdash->progress_done();
	wdash->progress_init(NULL);
#endif

	sound.open();

	if(load_sounds(prefs) < 0)
		return -3;

	wdash->progress_done();

	wdash->nibble();

	ct_engine.render_highlight = kobo_render_highlight;
	wdash->mode(DASHBOARD_GAME);
	wradar->mode(RM_NOISE);
	pubrand.init();
	init_js(prefs);
	gamecontrol.init(prefs->always_fire);
	manage.init();

	gsm.push(&st_intro_title);

	return 0;
}


void KOBO_main::close()
{
	close_js();
	RGN_FreeRegion(logo_region);
	logo_region = NULL;
	close_display();
	if(gengine)
	{
		delete gengine;
		gengine = NULL;
	}
	sound.close();
	SDL_Quit();
}


int KOBO_main::run()
{
	int retry_status = 0;
	int dont_retry = 0;
	while(1)
	{
		if(!retry_status)
		{
			safe_prefs = *prefs;
			gengine->run();

			if(!manage.game_stopped())
				manage.abort();

			if(exit_game_fast)
				break;

			if(exit_game)
			{
				if(wdash)
					wdash->nibble();
				break;
			}
			dont_retry = 0;
		}

		retry_status = 0;
		if(global_status & OS_RESTART_AUDIO)
		{
			if(prefs->use_sound)
			{
				log_printf(ULOG, "--- Restarting audio...\n");
				sound.stop();
#ifdef TIME_PROGRESS
				wdash->progress_init(NULL);
#else
				wdash->progress_init(progtab_sounds);
#endif
				if(load_sounds(prefs) < 0)
					return 5;
				wdash->progress_done();
				sound.open();
				wdash->nibble();
				log_printf(ULOG, "--- Audio restarted.\n");
				wdash->mode(DASHBOARD_GAME);
				wradar->mode(screen.radar_mode);
				manage.set_bars();
			}
			else
			{
				log_printf(ULOG, "--- Stopping audio...\n");
				sound.close();
				log_printf(ULOG, "--- Audio stopped.\n");
			}
		}
		if((global_status & OS_RELOAD_AUDIO_CACHE) && prefs->cached_sounds)
		{
			log_printf(ULOG, "--- Rendering sounds to disk...\n");
#ifdef TIME_PROGRESS
			wdash->progress_init(NULL);
#else
			wdash->progress_init(progtab_sounds);
#endif
			if(load_sounds(prefs) < 0)
			{
				log_printf(ELOG, "--- Could not render sounds to disk!\n");
				st_error.message("Could not render to disk!",
						"Please, check your installation.");
				gsm.push(&st_error);
			}
			wdash->progress_done();
			wdash->mode(DASHBOARD_GAME);
			wradar->mode(screen.radar_mode);
			manage.set_bars();
		}
		if(global_status & OS_RESTART_VIDEO)
		{
			log_printf(ULOG, "--- Restarting video...\n");
			wdash->nibble();
			gengine->hide();
			close_display();
			gengine->unload();
			if(init_display(prefs) < 0)
			{
				log_printf(ELOG, "--- Video init failed!\n");
				st_error.message("Video initialization failed!",
						"Try different settings.");
				gsm.push(&st_error);
				if(dont_retry)
					return 6;
				*prefs = safe_prefs;
				dont_retry = 1;
				retry_status |= OS_RESTART_VIDEO;
			}
			else
			{
				gamecontrol.init(prefs->always_fire);
				log_printf(ULOG, "--- Video restarted.\n");
			}
		}
		if(global_status & (OS_RELOAD_GRAPHICS |
					OS_RESTART_VIDEO))
		{
			if(!(global_status & OS_RESTART_VIDEO))
				wdash->nibble();
			gengine->unload();
			RGN_FreeRegion(logo_region);
			logo_region = NULL;
			if(!retry_status)
			{
				log_printf(ULOG, "--- Reloading graphics...\n");
#ifdef TIME_PROGRESS
				wdash->progress_init(NULL);
#else
				wdash->progress_init(progtab_graphics);
#endif
				if(load_graphics(prefs) < 0)
					return 7;
				wdash->progress_done();
				wdash->nibble();
				wdash->mode(DASHBOARD_GAME);
				wradar->mode(screen.radar_mode);
				manage.set_bars();
				log_printf(ULOG, "--- Graphics reloaded.\n");
			}
		}
		if(global_status & OS_RESTART_ENGINE)
			log_printf(ELOG, "OS_RESTART_ENGINE not implemented!\n");
		if(global_status & OS_RESTART_INPUT)
		{
			close_js();
			init_js(prefs);
			gamecontrol.init(prefs->always_fire);
		}
		if(global_status & OS_RESTART_LOGGER)
			open_logging(prefs);

		global_status = retry_status;
		km.pause_game();
		manage.reenter();
	}
	return 0;
}


/*----------------------------------------------------------
	Kobo Graphics Engine
----------------------------------------------------------*/

kobo_gfxengine_t::kobo_gfxengine_t()
{
}


void kobo_gfxengine_t::frame()
{
	sound.frame();

	if(!gsm.current())
	{
		log_printf(CELOG, "INTERNAL ERROR: No gamestate!\n");
		exit_game = 1;
		stop();
		return;
	}
	if(exit_game || manage.aborted())
	{
		stop();
		return;
	}

	/*
	 * Process input
	 */
	SDL_Event ev;
	while(SDL_PollEvent(&ev))
	{
		int k, ms;
		switch (ev.type)
		{
		  case SDL_KEYDOWN:
			switch(ev.key.keysym.sym)
			{
#ifdef PROFILE_AUDIO
			  case SDLK_F10:
				++km.audio_vismode;
				break;
			  case SDLK_F12:
				audio_print_info();
				break;
			  case SDLK_r:
			  {
				audio_channel_stop(-1, -1);
				int startt = SDL_GetTicks();
				audio_wave_load(0, "sfx.agw", 0);
				log_printf(VLOG, "(Loading + processing time: %d ms)\n",
						SDL_GetTicks() - startt);
				break;
			  }
#endif
			  case SDLK_DELETE:
				if(prefs->cmd_debug)
				{
					manage.ships = 1;
					myship.hit(1000);
				}
				break;
			  case SDLK_RETURN:
				ms = SDL_GetModState();
				if(ms & (KMOD_CTRL | KMOD_SHIFT | KMOD_GUI)) // IOHAVOC KMOD_META == (KMOD_LMETA|KMOD_RMETA) i.e. (0x0400 | 0x0800)
					break;
				if(!(ms & KMOD_ALT))
					break;
				km.pause_game();
				prefs->fullscreen = !prefs->fullscreen;
				prefs->changed = 1;
				global_status |= OS_RELOAD_GRAPHICS |
						OS_RESTART_VIDEO;
				stop();
				return;
                case SDLK_PRINTSCREEN: // SDLK_PRINT: IOHAVOC -- making an educated guess
			  case SDLK_SYSREQ:
// FIXME: Doesn't this trigger when entering names and stuff...?
			  case SDLK_s:
				gengine->screenshot();
				break;
			  default:
				break;
			}
			k = gamecontrol.map(ev.key.keysym.sym);
			gamecontrol.press(k);
			
            // IOHAVOC -- SDL_Keysym no longer has a unicode, only an used by sizeof stuff in struct has changed.
            // unicode isn't used so just pass in 1 for now, as that seems to be working.
            // gsm.press(k, ev.key.keysym.unicode);
            gsm.press(k, 1);

			break;
		  case SDL_KEYUP:
			if((ev.key.keysym.sym == SDLK_ESCAPE) && km.escape_hammering())
			{
				km.pause_game();
				prefs->fullscreen = 0;
				prefs->videodriver = (int)GFX_DRIVER_SDL2D;
				prefs->width = 640;
				prefs->height = 480;
				prefs->aspect = 1000;
				prefs->depth = 0;
				prefs->doublebuf = 0;
				prefs->pages = -1;
				prefs->shadow = 1;
				prefs->scalemode = (int)GFX_SCALE_NEAREST;
				prefs->brightness = 100;
				prefs->contrast = 100;
				global_status |= OS_RELOAD_GRAPHICS |
						OS_RESTART_VIDEO;
				stop();
				st_error.message("Safe video settings applied!",
					"Enter Options menu to save.");
				gsm.push(&st_error);
				return;
			}
			k = gamecontrol.map(ev.key.keysym.sym);
			if(k == SDLK_PAUSE)
			{
				gamecontrol.press(BTN_PAUSE);
				gsm.press(BTN_PAUSE);
			}
			else
			{
				gamecontrol.release(k);
				gsm.release(k);
			}
			break;
        // SDL_VIDEOEXPOSE generated by wake from sleep? IOHAVOC removing. Is SDL_APP_WILLENTERFOREGROUND a replacement?
        // case SDL_VIDEOEXPOSE:
		//	gengine->invalidate();
		//	break;
            case SDL_APP_WILLENTERBACKGROUND:   // IOHAVOC -- SDL_ACTIVEEVENT:
			// Any type of focus loss should activate pause mode!
			// if(!ev.active.gain)              // IOHAVOC -- entering bg obviates need for this conditional
				km.pause_game();
			break;
		  case SDL_QUIT:
			/*gsm.press(BTN_CLOSE);*/
			km.brutal_quit();
			break;
		  case SDL_JOYBUTTONDOWN:
			if(ev.jbutton.button == km.js_fire)
			{
				gamecontrol.press(BTN_FIRE);
				gsm.press(BTN_FIRE);
			}
			else if(ev.jbutton.button == km.js_start)
			{
				gamecontrol.press(BTN_START);
				gsm.press(BTN_START);
			}
			break;
		  case SDL_JOYBUTTONUP:
			if(ev.jbutton.button == km.js_fire)
			{
				gamecontrol.release(BTN_FIRE);
				gsm.release(BTN_FIRE);
			}
			break;
		  case SDL_JOYAXISMOTION:
			// FIXME: We will want to allow these to be
			// redefined, but for now, this works ;-)
			if(ev.jaxis.axis == km.js_lr)
			{
				if(ev.jaxis.value < -3200)
				{
					gamecontrol.press(BTN_LEFT);
					gsm.press(BTN_LEFT);
				}
				else if(ev.jaxis.value > 3200)
				{
					gamecontrol.press(BTN_RIGHT);
					gsm.press(BTN_RIGHT);
				}
				else
				{
					gamecontrol.release(BTN_LEFT);
					gamecontrol.release(BTN_RIGHT);
					gsm.release(BTN_LEFT);
					gsm.release(BTN_RIGHT);
				}

			}
			else if(ev.jaxis.axis == km.js_ud)
			{
				if(ev.jaxis.value < -3200)
				{
					gamecontrol.press(BTN_UP);
					gsm.press(BTN_UP);
				}
				else if(ev.jaxis.value > 3200)
				{
					gamecontrol.press(BTN_DOWN);
					gsm.press(BTN_DOWN);
				}
				else
				{
					gamecontrol.release(BTN_UP);
					gamecontrol.release(BTN_DOWN);
					gsm.release(BTN_UP);
					gsm.release(BTN_DOWN);
				}

			}
		  case SDL_MOUSEMOTION:
			mouse_x = (int)(ev.motion.x / gengine->xscale()) - km.xoffs;
			mouse_y = (int)(ev.motion.y / gengine->yscale()) - km.yoffs;
			if(prefs->use_mouse)
				gamecontrol.mouse_position(
						mouse_x - 8 - MARGIN - WSIZE/2,
						mouse_y - MARGIN - WSIZE/2);
			break;
		  case SDL_MOUSEBUTTONDOWN:
			mouse_x = (int)(ev.motion.x / gengine->xscale()) - km.xoffs;
			mouse_y = (int)(ev.motion.y / gengine->yscale()) - km.yoffs;
			gsm.press(BTN_FIRE);
			if(prefs->use_mouse)
			{
				gamecontrol.mouse_position(
						mouse_x - 8 - MARGIN - WSIZE/2,
						mouse_y - MARGIN - WSIZE/2);
				switch(ev.button.button)
				{
				  case SDL_BUTTON_LEFT:
					mouse_left = 1;
					break;
				  case SDL_BUTTON_MIDDLE:
					mouse_middle = 1;
					break;
				  case SDL_BUTTON_RIGHT:
					mouse_right = 1;
					break;
				}
				gamecontrol.press(BTN_FIRE);
			}
			break;
		  case SDL_MOUSEBUTTONUP:
			mouse_x = (int)(ev.motion.x / gengine->xscale()) - km.xoffs;
			mouse_y = (int)(ev.motion.y / gengine->yscale()) - km.yoffs;
			if(prefs->use_mouse)
			{
				gamecontrol.mouse_position(
						mouse_x - 8 - MARGIN - WSIZE/2,
						mouse_y - MARGIN - WSIZE/2);
				switch(ev.button.button)
				{
				  case SDL_BUTTON_LEFT:
					mouse_left = 0;
					break;
				  case SDL_BUTTON_MIDDLE:
					mouse_middle = 0;
					break;
				  case SDL_BUTTON_RIGHT:
					mouse_right = 0;
					break;
				}
			}
			if(!mouse_left && !mouse_middle && !mouse_right)
			{
				if(prefs->use_mouse)
					gamecontrol.release(BTN_FIRE);
				gsm.release(BTN_FIRE);
			}
			break;
		}
	}
	gamecontrol.process();

	/*
	 * Run the current gamestate for one frame
	 */
	gsm.frame();

	if(prefs->cmd_autoshot && !manage.game_stopped())
	{
		static int c = 0;
		++c;
		if(c >= 3)
		{
			gengine->screenshot();
			c = 0;
		}
	}
}


void kobo_gfxengine_t::pre_render()
{
	sound.run();
	gsm.pre_render();
}


void kobo_gfxengine_t::post_render()
{
	gsm.post_render();

	if(!prefs->cmd_noframe)
	{
		wmain->sprite(0, 0, B_FRAME_TL, 0, 0);
		wmain->sprite(WSIZE - 16, 0, B_FRAME_TR, 0, 0);
		wmain->sprite(0, WSIZE - 16, B_FRAME_BL, 0, 0);
		wmain->sprite(WSIZE - 16, WSIZE - 16, B_FRAME_BR, 0, 0);
	}

#ifdef DEBUG
	if(prefs->cmd_debug)
	{
		char buf[20];
		snprintf(buf, sizeof(buf), "Obj: %d",
				gengine->objects_in_use());
		wmain->font(B_NORMAL_FONT);
		wmain->string(160, 5, buf);
	}

	switch (km.audio_vismode % 5)
	{
	  case 0:
		break;
	  case 1:
		draw_osc(0);
		break;
	  case 2:
		draw_vu();
		break;
	  case 3:
		draw_osc(1);
		break;
	  case 4:
		draw_osc(2);
		break;
	}
#endif

	// Frame rate counter
	int nt = (int)SDL_GetTicks();
	int tt = nt - km.fps_starttime;
	if((tt > 1000) && km.fps_count)
	{
		float f = km.fps_count * 1000.0 / tt;
		::screen.fps(f);
		km.fps_count = 0;
		km.fps_starttime = nt;
		if(prefs->cmd_fps)
		{
			char buf[20];
			snprintf(buf, sizeof(buf), "%.1f", f);
			km.dfps->text(buf);
			if(!km.fps_results)
				km.fps_results = (float *)
						calloc(MAX_FPS_RESULTS,
						sizeof(float));
			if(km.fps_results)
			{
				km.fps_results[km.fps_nextresult++] = f;
				if(km.fps_nextresult >= MAX_FPS_RESULTS)
					km.fps_nextresult = 0;
				if(km.fps_nextresult > km.fps_lastresult)
					km.fps_lastresult = km.fps_nextresult;
			}
		}
	}
	++km.fps_count;

	// Frame rate limiter
	if(prefs->max_fps)
	{
		if(prefs->max_fps_strict)
		{
			static double nextframe = -1000000.0f;
			while(1)
			{
				double t = (double)SDL_GetTicks();
				if(fabs(nextframe - t) > 1000.0f)
					nextframe = t;
				double d = nextframe - t;
				if(d > 10.0f)
					SDL_Delay(10);
				else if(d > 1.0f)
					SDL_Delay(1);
				else
					break;
			}
			nextframe += 1000.0f / prefs->max_fps;
		}
		else
		{
			int rtime = nt - km.max_fps_begin;
			km.max_fps_begin = nt;
			km.max_fps_filter += (float)(rtime - km.max_fps_filter) * 0.3;
			int delay = (int)(1000.0 / prefs->max_fps -
					km.max_fps_filter + 0.5);
			if((delay > 0) && (delay < 1100))
				SDL_Delay(delay);
			km.max_fps_begin = (int)SDL_GetTicks();
		}
	}
}


/*----------------------------------------------------------
	main() and related stuff
----------------------------------------------------------*/

static void put_usage()
{
	printf("\nKobo Deluxe %s\n", KOBO_VERSION);
	printf("Usage: kobodl [<options>]\n");
	printf("Recognized options:\n");
	int s = -1;
	while(1)
	{
		const char *fs;
		s = prefs->find_next(s);
		if(s < 0)
			break;
		switch(prefs->type(s))
		{
		  case CFG_BOOL:
			fs = "	-[no]%-16.16s[%s]\t%s%s\n";
			break;
		  case CFG_INT:
		  case CFG_FLOAT:
			fs = "	-%-20.20s[%s]\t%s%s\n";
			break;
		  case CFG_STRING:
			fs = "	-%-20.20s[\"%s\"]\t%s%s\n";
			break;
		  default:
			continue;
		}
		printf(fs, prefs->name(s), prefs->get_default_s(s),
				prefs->do_save(s) ? "" : "(Not saved!) ",
				prefs->description(s));
	}
}


static void put_options_man()
{
	int s = -1;
	while(1)
	{
		const char *fs;
		s = prefs->find_next(s);
		if(s < 0)
			break;
		switch(prefs->type(s))
		{
		  case CFG_BOOL:
			fs = ".TP\n.B \\-[no]%s\n%s%s. Default: %s.\n";
			break;
		  case CFG_INT:
		  case CFG_FLOAT:
			fs = ".TP\n.B \\-%s\n%s%s. Default: %s.\n";
			break;
		  case CFG_STRING:
			fs = ".TP\n.B \\-%s\n%s%s. Default: \"%s\"\n";
			break;
		  default:
			continue;
		}
		printf(fs, prefs->name(s),
				prefs->do_save(s) ? "" : "(Not saved!) ",
				prefs->description(s),
				prefs->get_default_s(s));
	}
}


extern "C" void emergency_close(void)
{
	km.close();
}


extern "C" RETSIGTYPE breakhandler(int dummy)
{
	/* For platforms that drop the handlers on the first signal... */
	signal(SIGTERM, breakhandler);
	signal(SIGINT, breakhandler);
	km.brutal_quit();
#if (RETSIGTYPE != void)
	return 0;
#endif
}

// From SDLMain - Set the working directory to the .app's parent directory
static void setupWorkingDirectory(bool shouldChdir)
{
    
#if USE_RESOURCE_WORKING_DIR // For an app bundle, we want to be located in the resource folder
    
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
    char path[PATH_MAX];
    if (!CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8 *)path, PATH_MAX))
    {
        // error!
        log_printf(ELOG, "Failed to setupWorkingDirectory \n");
    }
    CFRelease(resourcesURL);
    chdir(path);
    
#else
    
    if (shouldChdir)
    {
        char parentdir[MAXPATHLEN];
        CFURLRef url = CFBundleCopyBundleURL(CFBundleGetMainBundle());
        CFURLRef url2 = CFURLCreateCopyDeletingLastPathComponent(0, url);
        
        if (CFURLGetFileSystemRepresentation(url2, 1, (UInt8 *)parentdir, MAXPATHLEN)) {
            chdir(parentdir);   /* chdir to the binary app's parent */
        }
        CFRelease(url);
        CFRelease(url2);
    }
    
#endif
    
}

int main(int argc, char *argv[])
{
	int cmd_exit = 0;
    
	atexit(emergency_close);
	signal(SIGTERM, breakhandler);
	signal(SIGINT, breakhandler);

    // gFinderLaunch
    setupWorkingDirectory(false);

    // Initialize SDL
    if (SDL_Init(0) < 0) {
        fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
        return (1);
    }

	if(main_init())
	{
		fprintf(stderr, "INTERNAL ERROR\n");
		return 1;
	};

	km.open_logging(NULL);

	setup_dirs(argv[0]);
	--argc;
	++argv;

	if((argc < 1) || (strcmp("-override", argv[0]) != 0))
		km.load_config(prefs);

	if((prefs->parse(argc, argv) < 0) || prefs->cmd_help)
	{
		put_usage();
		main_cleanup();
		return 1;
	}

	if(prefs->cmd_options_man)
	{
		put_options_man();
		main_cleanup();
		return 1;
	}

	if(prefs->cmd_noparachute)
	{
		SDL_Quit();
		SDL_Init(SDL_INIT_NOPARACHUTE);  // IOHAVOC: According to http://wiki.libsdl.org/MigrationGuide There's no SDL
                                         // parachute anymore. What 1.2 called SDL_INIT_NOPARACHUTE is a default and only state now.
	}

	km.open_logging(prefs);

	for(int a = 0; a < argc; ++a)
		log_printf(DLOG, "argv[%d] = \"%s\"\n", a, argv[a]);

	int k = -1;
	while((k = prefs->find_next(k)) >= 0)
	{
		log_printf(D3LOG, "key %d: \"%s\"\ttype=%d\t\"%s\"\n",
				k,
				prefs->name(k),
				prefs->type(k),
				prefs->description(k)
			);
	}

	add_dirs(prefs);

	if(prefs->cmd_hiscores)
	{
		scorefile.gather_high_scores();
		scorefile.print_high_scores();
		cmd_exit = 1;
	}

	if(prefs->cmd_showcfg)
	{
		printf("Configuration:\n");
		printf("----------------------------------------\n");
		prefs->write(stdout);
		printf("\nPaths:\n");
		printf("----------------------------------------\n");
		fmap->print(stdout, "*");
		printf("----------------------------------------\n");
		cmd_exit = 1;
	}

	if(cmd_exit)
	{
		km.close_logging();
		main_cleanup();
		return 0;
	}

	if(km.open() < 0)
	{
		km.close_logging();
		main_cleanup();
		return 1;
	}

	km.run();

	km.close();

	/* 
	 * Seems like we got all the way here without crashing,
	 * so let's save the current configuration! :-)
	 */
	if(prefs->changed)
	{
		km.save_config(prefs);
		prefs->changed = 0;
	}

	if(prefs->cmd_fps)
		km.print_fps_results();

	km.close_logging();
	main_cleanup();
	return 0;
}
