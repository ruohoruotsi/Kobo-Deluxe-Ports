/*(LGPL)
----------------------------------------------------------------------
	gfxengine.h - Graphics Engine
----------------------------------------------------------------------
 * Copyright (C) 2001-2003, 2007 David Olofson
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef	_GFXENGINE_H_
#define	_GFXENGINE_H_

#define GFX_BANKS	256
#define	MAX_DIRTYRECTS	1024
#define	MAX_PAGES	3

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "glSDL.h"
#include "sprite.h"
#include "cs.h"

enum gfx_drivers_t
{
	GFX_DRIVER_SDL2D =	0,
	GFX_DRIVER_GLSDL =	1
};

enum gfx_scalemodes_t
{
	GFX_SCALE_NEAREST =	0,
	GFX_SCALE_BILINEAR =	1,
	GFX_SCALE_BILIN_OVER =	2,
	GFX_SCALE_SCALE2X =	3,
	GFX_SCALE_DIAMOND =	4
};

class window_t;
class SoFont;

class gfxengine_t
{
	friend class window_t;
  public:
	gfxengine_t();
	virtual ~gfxengine_t();

	void output(window_t *outwin);
	window_t *output()	{ return window; }

	void screen(window_t *fullwin);
	window_t *screen()	{ return fullwin; }

	/*
	 * Initialization
	 */
	void size(int w, int h);
	void centered(int c);
	void scale(float x, float y);
	void driver(gfx_drivers_t drv);
	void mode(int bits, int fullscreen);

	// 1: Use double buffering if possible
	void doublebuffer(int use);

	// -1: Use default for shadow() and doublebuffer() settings
	// 0: None; assume flipping gives you a garbage buffer
	// 1: Assume flipping leaves the back buffer intact
	// 2: Assume two buffers that are swapped when flipping
	// 3: Assume three buffers cycled when flipping
	void pages(int np);

	// 1: Enable vsync, if available
	void vsync(int use);

	// 1: Use a software shadow back buffer, if possible
	void shadow(int use);

	void autoinvalidate(int use);

	void interpolation(int inter);

	// 0 to reset internal timer
	void period(float frameduration);

	// 0 to disable timing, running one logic frame per rendered frame.
	// 1 to disable filtering, using raw delta times for timing.
	void timefilter(float coeff);

	void wrap(int x, int y);

	/* Info */
	int doublebuffer()	{ return _doublebuf; }
	int shadow()		{ return _shadow; }
	int autoinvalidate()	{ return _autoinvalidate; }

	/* Engine open/close */
	int open(int objects = 1024, int extraflags = 0);
	void close();

	/* Data management (use while engine is open) */
	void reset_filters();
	void filterflags(int fgs);
	void scalemode(gfx_scalemodes_t sm, int clamping = 0);
	void source_scale(float x, float y);
//	void colorkey(Uint8 r, Uint8 g, Uint8 b);
	void clampcolor(Uint8 r, Uint8 g, Uint8 b, Uint8 a);
	void dither(int type = 0, int _broken_rgba8 = 0);
	void noalpha(int threshold = 128);
	void brightness(float bright, float contr);

	int loadimage(int bank, const char *name);
	int loadtiles(int bank, int w, int h, const char *name);
	int loadfont(int bank, const char *name);
	int copyrect(int bank, int sbank, int sframe, SDL_Rect *r);

	int is_loaded(int bank);
	void reload();
	void unload(int bank = -1);

	/* Settings (use while engine is open) */
	void title(const char *win, const char *icon);

	/* Display show/hide */
	int show();
	void hide();

	/* Main loop take-over */
	void run();

	/*
	 * Override these;
	 *	frame() is called once per control system frame,
	 *		after the control system has executed.
	 *	pre_render() is called after the engine has advanced
	 *		to the state for the current video frame
	 *		(interpolated state is calculated), before
	 *		the engine renders all graphics.
	 *	post_render() is called after the engine have
	 *		rendered all sprites, but before video the
	 *		sync/flip/update operation.
	 */
	virtual void frame();
	virtual void pre_render();
	virtual void post_render();

	/*
	---------------------------------------------
	 * The members below this line can safely be
	 * called from within the frame() handler.
	 */

	/* Rendering */
	void clear(Uint32 _color = 0x000000);
	void invalidate(SDL_Rect *rect = NULL, window_t *window = NULL);

	/* Screenshots */
	void screenshot();

	/* Control */
	cs_engine_t *cs()	{ return csengine; }
	SDL_Surface *surface();
	void flip();		// Flip pages
	void update();		// Full update, making current draw page visible
	void stop();
	cs_obj_t *get_obj(int layer);
	void free_obj(cs_obj_t *obj);
	void cursor(int csr);

	s_sprite_t *get_sprite(unsigned bank, int _frame)
	{
		return s_get_sprite(gfx, bank, _frame);
	}
	SoFont *get_font(unsigned int f);
	int set_hotspot(unsigned bank, int _frame, int x, int y)
	{
		return s_set_hotspot(gfx, bank, _frame, x, y);
	}

	void scroll_ratio(int layer, float xr, float yr);
	void scroll(int xs, int ys);
	void force_scroll();

	int xoffs(int layer);
	int yoffs(int layer);

	/* Info */
	int objects_in_use();

	int width()		{ return _width; }
	int height()		{ return _height; }
	float xscale()		{ return xs * (1.f/256.f); }
	float yscale()		{ return ys * (1.f/256.f); }

  protected:
	gfx_drivers_t		_driver;
	gfx_scalemodes_t	_scalemode;
	int			_clamping;
	SDL_Surface	*screen_surface;
	SDL_Surface	*softbuf;
	int		backpage;
	int		frontpage;
	int		dirtyrects[MAX_PAGES];
	SDL_Rect	dirtytable[MAX_PAGES][MAX_DIRTYRECTS];
	window_t	*dirtywtable[MAX_PAGES][MAX_DIRTYRECTS];
	window_t	*fullwin;
	window_t	*window;
	window_t	*windows;	// Linked list
	int		wx, wy;
	int		xs, ys;		// fix 24:8
	int		sxs, sys;	// fix 24:8
	s_filter_t	*sf1, *sf2;	// Scaling filter plugins
	s_filter_t	*acf;		// Alpha cleaning plugin
	s_filter_t	*bcf;		// Brightness/contrast plugin
	s_filter_t	*df;		// Dither filter plugin
	s_filter_t	*dsf;		// Display format plugin
	int		xscroll, yscroll;
	float		xratio[CS_LAYERS];
	float		yratio[CS_LAYERS];
	s_container_t	*gfx;
	SoFont		*fonts[GFX_BANKS];	// Kludge.
	cs_engine_t	*csengine;
	int		xflags;
	int		_doublebuf;
	int		_pages;
	int		_vsync;
	int		_shadow;
	int		_fullscreen;
	int		_centered;
	int		_autoinvalidate;
	int		use_interpolation;
	int		_width, _height;
	int		_depth;
	const char	*_title;
	const char	*_icontitle;
	int		_cursor;
	int		_dither;
	int		_dither_type;
	int		broken_rgba8;	//Klugde for OpenGL (if RGBA8 ==> RGBA4)
	int		alpha_threshold;	//For noalpha()
	float		_brightness;
	float		_contrast;

	int		last_tick;
	float		ticks_per_frame;
	float		_timefilter;

	int		is_showing;
	int		is_running;
	int		is_open;

	int		screenshot_count;

	void __invalidate(int page, SDL_Rect *rect = NULL,
			 window_t *window = NULL);
	void refresh_rect(SDL_Rect *r);

	static void on_frame(cs_engine_t *e);
	void __frame();

	void start_engine();
	void stop_engine();
	static void render_sprite(cs_obj_t *o);
};


extern gfxengine_t *gfxengine;

#endif
