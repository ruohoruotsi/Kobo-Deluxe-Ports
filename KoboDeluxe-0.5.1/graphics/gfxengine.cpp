/*(LGPL)
----------------------------------------------------------------------
	gfxengine.cpp - Graphics Engine
----------------------------------------------------------------------
 * Copyright (C) 2001-2003, 2006-2007 David Olofson
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

#define	DBG(x)	x

#include "logger.h"

#include <string.h>
#include <math.h>

#include "gfxengine.h"
#include "filters.h"
#include "SDL_image.h"
#include "glSDL.h"
#include "sofont.h"
#include "window.h"

gfxengine_t *gfxengine;


gfxengine_t::gfxengine_t()
{
	gfxengine = this;	/* Uurgh! Kludge. */

	screen_surface = NULL;
	softbuf = NULL;
	fullwin = NULL;
	window = NULL;
	windows = NULL;
	wx = wy = 0;
	xs = ys = 256;		// 1.0
	sxs = sys = 256;	// 1.0
	sf1 = df = dsf = acf = NULL;
	gfx = NULL;
	csengine = NULL;
	_driver = GFX_DRIVER_SDL2D;
	_shadow = 1;
	_doublebuf = 1;
	_pages = -1;
	_vsync = 1;
	_fullscreen = 0;
	_centered = 0;
	use_interpolation = 1;
	_depth = 0;
	_title = "GfxEngine v0.4";
	_icontitle = "GfxEngine";
	_cursor = 1;
	_width = 320;
	_height = 240;
	_autoinvalidate = 1;
	_scalemode = GFX_SCALE_NEAREST;
	_clamping = 0;
	xflags = 0;

	_dither = 0;
	_dither_type = 0;
	broken_rgba8 = 0;
	alpha_threshold = 0;

	_brightness = 1.0;
	_contrast = 1.0;

	last_tick = -1000000;
	ticks_per_frame = 1000.0/60.0;

	is_running = 0;
	is_showing = 0;
	is_open = 0;

	memset(fonts, 0, sizeof(fonts));

	xscroll = yscroll = 0;
	for(int i = 0; i < CS_LAYERS ; ++i)
		xratio[i] = yratio[i] = 0.0;

	dirtyrects[0] = 0;
	dirtyrects[1] = 0;
	frontpage = 0;
	backpage = 1;
	screenshot_count = 0;
    
    
    // IOHAVOC - SDL2
    sdl2window = NULL;
}


gfxengine_t::~gfxengine_t()
{
	stop();
	hide();
	close();
	window_t *w = windows;
	while(w)
	{
		w->engine = NULL;
		w = w->next;
	}
}


void gfxengine_t::output(window_t *outwin)
{
	window = outwin;
}


/*----------------------------------------------------------
	Initialization
----------------------------------------------------------*/

void gfxengine_t::size(int w, int h)
{
	int was_showing = is_showing;
	hide();

	_width = w;
	_height = h;
	if(csengine)
		cs_engine_set_size(csengine, w, h);

	if(was_showing)
		show();
}

void gfxengine_t::centered(int c)
{
	int was_showing = is_showing;
	hide();

	_centered = c;

	if(was_showing)
		show();
}

void gfxengine_t::scale(float x, float y)
{
	xs = (int)(x * 256.f);
	ys = (int)(y * 256.f);
	log_printf(DLOG, "gfxengine: Setting scale to %d:256 x %d:256.\n", xs, ys);
}

void gfxengine_t::mode(int bits, int fullscreen)
{
	int was_showing = is_showing;
	hide();

	int olddepth = _depth;
	_depth = bits;
	if(_depth != olddepth)
		reload();
	_fullscreen = fullscreen;

	if(was_showing)
		show();
}

void gfxengine_t::driver(gfx_drivers_t drv)
{
	int was_showing = is_showing;
	hide();

	_driver = drv;

	if(was_showing)
		show();
}

void gfxengine_t::doublebuffer(int use)
{
	if(_doublebuf == use)
		return;

	int was_showing = is_showing;
	hide();

	_doublebuf = use;

	if(was_showing)
		show();
}

void gfxengine_t::pages(int np)
{
	_pages = np;
}

void gfxengine_t::vsync(int use)
{
	if(_vsync == use)
		return;

	int was_showing = is_showing;
	hide();

	_vsync = use;

	if(was_showing)
		show();
}

void gfxengine_t::shadow(int use)
{
	if(_shadow == use)
		return;

	int was_showing = is_showing;
	hide();

	_shadow = use;

	if(was_showing)
		show();
}

void gfxengine_t::autoinvalidate(int use)
{
	_autoinvalidate = use;
}

void gfxengine_t::period(float frameduration)
{
	if(frameduration > 0)
	{
		ticks_per_frame = frameduration;
	}
	else
		log_printf(DLOG, "gfxengine: Time line reset.\n");
	if(csengine)
		cs_engine_advance(csengine, 0);
}

void gfxengine_t::wrap(int x, int y)
{
	wx = x;
	wy = y;
	if(csengine)
		cs_engine_set_wrap(csengine, x, y);
}


/*----------------------------------------------------------
	Data management
----------------------------------------------------------*/

void gfxengine_t::reset_filters()
{
	/* Remove all filters */
	s_remove_filter(NULL);
	sf1 = df = dsf = acf = NULL;

	/* Set up filter pipeline */
	s_remove_filter(NULL);

	s_add_filter(s_filter_rgba8);
#if 0
	s_filter_t *fi;
	fi = s_add_filter(s_filter_key2alpha);
	fi->args.max = 1;
#endif
	sf1 = s_add_filter(s_filter_scale);
	sf2 = s_add_filter(s_filter_scale);
	acf = s_add_filter(s_filter_cleanalpha);
	bcf = s_add_filter(s_filter_brightness);
	df = s_add_filter(s_filter_dither);
	dsf = s_add_filter(s_filter_displayformat);

	/* Set default parameters */
//	colorkey(0, 0, 0);
	clampcolor(0, 0, 0, 0);
	scalemode(GFX_SCALE_NEAREST);
	dither(0, 0);
	noalpha(0);
	brightness(1.0, 1.0);
	filterflags(0);
}


void gfxengine_t::filterflags(int fgs)
{
	s_filter_flags = fgs;
}


void gfxengine_t::scalemode(gfx_scalemodes_t sm, int clamping)
{
	_scalemode = sm;
	_clamping = clamping;
	if(!sf1 || !df)
		return;

	int rxs = (xs * 256 + 128) / sxs;
	int rys = (ys * 256 + 128) / sys;
#if 1
	// Filter 2 is off in most cases
	sf2->args.x = 0;
	sf2->args.fx = 0.0f;
	sf2->args.fy = 0.0f;
#endif
	if((rxs == 256) && (rys == 256))
	{
		sf1->args.x = SF_SCALE_NEAREST;
		sf1->args.fx = 1.0f;
		sf1->args.fy = 1.0f;
		return;
	}

	switch(clamping)
	{
	  default:
	  case 0:
		filterflags(0);
		break;
	  case 1:
		filterflags(SF_CLAMP_EXTEND);
		break;
	  case 2:
		filterflags(SF_CLAMP_SFONT);
		break;
	}

	sf1->args.fx = rxs * (1.0f/256.0f);
	sf1->args.fy = rys * (1.0f/256.0f);

	switch(_scalemode)
	{
	  case GFX_SCALE_NEAREST:	//Nearest
		sf1->args.x = SF_SCALE_NEAREST;
		break;
	  case GFX_SCALE_BILINEAR:	//Bilinear
		sf1->args.x = SF_SCALE_BILINEAR;
		break;
	  case GFX_SCALE_BILIN_OVER:	//Bilinear + Oversampling
#if 0
		sf1->args.x = SF_SCALE_BILINEAR;
		sf2->args.x = SF_SCALE_BILINEAR;
		sf1->args.fx = rxs * (1.5f/256.f);
		sf1->args.fy = rys * (1.5f/256.f);
		sf2->args.fx = 1.0f/1.5f;
		sf2->args.fy = 1.0f/1.5f;
		break;
		/* Hack for better bilinear [+ oversampling] scaling... */
		switch(clamping)
		{
		  case 2:
			sf1->args.x = SF_SCALE_NEAREST;
			sf2->args.x = SF_SCALE_BILINEAR;
			switch((rxs + 128) / 256)
			{
			  case 2:
				sf1->args.fx = 4.0f;
				sf1->args.fy = 4.0f;
				sf2->args.fx = 0.5f;
				sf2->args.fy = 0.5f;
				break;
			  case 3:
				sf1->args.fx = 2.0f;
				sf1->args.fy = 2.0f;
				sf2->args.fx = 1.5f;
				sf2->args.fy = 1.5f;
				break;
			  case 4:
				sf1->args.fx = 2.0f;
				sf1->args.fy = 2.0f;
				sf2->args.fx = 2.0f;
				sf2->args.fy = 2.0f;
				break;
			  default:
				sf1->args.fx = rxs * (2.f/256.f);
				sf1->args.fy = rys * (2.f/256.f);
				sf2->args.fx = 0.5f;
				sf2->args.fy = 0.5f;
				break;
			}
			break;
		  default:
			sf1->args.x = SF_SCALE_BILINEAR;
			break;
		}
#endif
		sf1->args.x = SF_SCALE_BILINEAR;
		break;
	  case GFX_SCALE_SCALE2X:	//Scale2x
		sf1->args.x = SF_SCALE_SCALE2X;
		break;
	  case GFX_SCALE_DIAMOND:	//Diamond2x
		sf1->args.x = SF_SCALE_DIAMOND;
		break;
	}
}


void gfxengine_t::source_scale(float x, float y)
{
	sxs = (int)(x * 256.f);
	sys = (int)(y * 256.f);
	scalemode(_scalemode, _clamping);
}

#if 0
void gfxengine_t::colorkey(Uint8 r, Uint8 g, Uint8 b)
{
	s_colorkey.r = r;
	s_colorkey.g = g;
	s_colorkey.b = b;
}
#endif

void gfxengine_t::clampcolor(Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	s_clampcolor.r = r;
	s_clampcolor.g = g;
	s_clampcolor.b = b;
	s_clampcolor.a = b;
}


void gfxengine_t::dither(int type, int _broken_rgba8)
{
	_dither = type >= 0;
	_dither_type = type;
	broken_rgba8 = _broken_rgba8;

	if(!df)
		return;
	if(!screen_surface)
		return;

	if(_dither)
	{
		if(_driver == GFX_DRIVER_GLSDL)
		{
			//Klugde for glSDL, which doesn't give us a faked
			//screen surface - while we're interested only in
			//the *texture* depth; not the display depth!
			df->args.r = df->args.g = df->args.b = 0;
			//Another kludge, because some cards support RGB8
			//(24 bit) textures, but not RGBA8 (32 bit).
			df->args.x = broken_rgba8;
		}
		else
		{
			df->args.x = 0;
			df->args.r = 1<<(screen_surface->format->Rloss-1);
			df->args.g = 1<<(screen_surface->format->Gloss-1);
			df->args.b = 1<<(screen_surface->format->Bloss-1);
		}
	}
	else
		df->args.x = df->args.r = df->args.g = df->args.b = 0;
	df->args.y = type;
}


void gfxengine_t::noalpha(int threshold)
{
	alpha_threshold = threshold;
	if(!dsf || !acf)
		return;

	if(threshold > 0)
	{
		acf->args.min = 128;
		acf->args.max = 128;
		acf->args.fx = 1.0;
		acf->args.x = threshold;

		dsf->args.x = 1;
	}
	else
	{
		acf->args.min = 16;
		acf->args.max = 255-16;
		acf->args.fx = 1.3;
		acf->args.x = -16;

		dsf->args.x = 0;
	}
}


void gfxengine_t::brightness(float bright, float contr)
{
	_brightness = bright;
	_contrast = contr;
	if(!bcf)
		return;

	bcf->args.fx = _brightness;
	bcf->args.fy = _contrast;
	bcf->args.min = 0;
	bcf->args.max = 255;
}


int gfxengine_t::loadimage(int bank, const char *name)
{
	if(!csengine)
	{
		log_printf(ELOG, "loadimage: Engine must be open!\n");
		return -10;
	}
	s_blitmode = S_BLITMODE_AUTO;
	log_printf(DLOG, "Loading image %s (bank %d)...\n", name, bank);
	if(s_load_image(gfx, bank, name))
	{
		log_printf(ELOG, "  Failed to load %s!\n", name);
		return -1;
	}

	s_bank_t *b = s_get_bank(gfx, bank);
	if(!b)
	{ 
		log_printf(ELOG, "  gfxengine: Internal error 1!\n");
		return -15;
	}
	cs_engine_set_image_size(csengine, bank, b->w, b->h);

	log_printf(DLOG, "  Ok.\n");
	return 0;
}


int gfxengine_t::loadtiles(int bank, int w, int h, const char *name)
{
	if(!csengine)
	{
		log_printf(ELOG, "loadtiles: Engine must be open!\n");
		return -10;
	}
	s_blitmode = S_BLITMODE_AUTO;
	log_printf(DLOG, "Loading tiles %s (bank %d; %dx%d)...\n",
			name, bank, w, h);
	if(s_load_bank(gfx, bank, w, h, name))
	{
		log_printf(ELOG, "  Failed to load %s!\n", name);
		return -2;
	}

	cs_engine_set_image_size(csengine, bank, w, h);

	log_printf(DLOG, "  Ok. (%d frames)\n", s_get_bank(gfx, bank)->max+1);
	return 0;
}


int gfxengine_t::loadfont(int bank, const char *name)
{
	if(!csengine)
	{
		log_printf(ELOG, "loadfont: Engine must be open!\n");
		return -10;
	}
	s_blitmode = S_BLITMODE_AUTO;
	scalemode(_scalemode, 2);
	log_printf(DLOG, "Loading font %s (bank %d)...\n", name, bank);
	if(s_load_image(gfx, bank, name))
	{
		log_printf(ELOG, "  Failed to load %s!\n", name);
		return -2;
	}
	if(bank >= GFX_BANKS)
	{
		log_printf(ELOG, "  Too high bank #!\n");
		return -3;
	}

	if(!fonts[bank])
		fonts[bank] = new SoFont;
	if(!fonts[bank])
	{
		log_printf(ELOG, "  Failed to instantiate SoFont!\n");
		return -4;
	}

	fonts[bank]->ExtraSpace((xs + 127) / 256);
	if(fonts[bank]->load(s_get_sprite(gfx, bank, 0)->surface))
	{
		s_detach_sprite(gfx, bank, 0);
		log_printf(DLOG, "  Ok.\n");
		return 0;
	}
	else
	{
		log_printf(ELOG, "  SoFont::load() failed!\n");
		return -5;
	}
	return 0;
}


int gfxengine_t::copyrect(int bank, int sbank, int sframe, SDL_Rect *r)
{
	SDL_Rect sr = *r;
	if(!csengine)
	{
		log_printf(ELOG, "copyrect: Engine must be open!\n");
		return -10;
	}
	log_printf(DLOG, "Copying rect from %d:%d (bank %d)...\n",
			sbank, sframe, bank);
	int x2 = (int)((sr.x + sr.w) * xs + 128) >> 8;
	int y2 = (int)((sr.y + sr.h) * ys + 128) >> 8;
	sr.x = (int)(sr.x * xs + 128) >> 8;
	sr.y = (int)(sr.y * ys + 128) >> 8;
	sr.w = x2 - sr.x;
	sr.h = y2 - sr.y;
	if(s_copy_rect(gfx, bank, sbank, sframe, &sr) < 0)
	{
		log_printf(ELOG, "  s_copy_rect() failed!\n");
		return -1;
	}
	log_printf(DLOG, "  Ok.\n");
	return 0;
}


int gfxengine_t::is_loaded(int bank)
{
	if(!csengine)
		return 0;
	if(!s_get_bank(gfx, (unsigned)bank))
		return 0;
	return 1;
}


void gfxengine_t::reload()
{
	log_printf(DLOG, "Reloading all banks. (Not implemented!)\n");
//	s_reload_all_banks(gfx);
}


void gfxengine_t::unload(int bank)
{
	if(bank < 0)
	{
		log_printf(DLOG, "Unloading all banks.\n");
		for(int i = 0; i < GFX_BANKS; ++i)
		{
			delete fonts[i];
			fonts[i] = NULL;
		}
		if(gfx)
			s_delete_all_banks(gfx);
	}
	else
	{
		log_printf(DLOG, "Unloading bank %d.\n", bank);
		if(bank < GFX_BANKS)
		{
			delete fonts[bank];
			fonts[bank] = NULL;
		}
		if(gfx)
			s_delete_bank(gfx, bank);
	}
}


/*----------------------------------------------------------
	Engine open/close
----------------------------------------------------------*/

void gfxengine_t::on_frame(cs_engine_t *e)
{
	gfxengine->__frame();
	gfxengine->frame();
}


int gfxengine_t::open(int objects, int extraflags)
{
	xflags = extraflags;

	if(is_open)
		return show();

	log_printf(DLOG, "Opening engine...\n");
	csengine = cs_engine_create(_width, _height, objects);
	if(!csengine)
	{
		log_printf(ELOG, "Failed to set up control system engine!\n");
		return -1;
	}

	csengine->on_frame = on_frame;
	cs_engine_set_wrap(csengine, wx, wy);

	gfx = s_new_container(GFX_BANKS);
	if(!gfx)
	{
		log_printf(ELOG, "Failed to set up graphics container!\n");
		cs_engine_delete(csengine);
		return -2;
	}

	reset_filters();

	is_open = 1;
	return show();
}


void gfxengine_t::close()
{
	if(!is_open)
		return;

	log_printf(DLOG ,"Closing engine...\n");
	stop();
	hide();
	unload();
	cs_engine_delete(csengine);
	csengine = NULL;
	s_remove_filter(NULL);
	sf1 = df = dsf = acf = NULL;
	s_delete_container(gfx);
	gfx = NULL;
	is_open = 0;
}


/*----------------------------------------------------------
	Settings
----------------------------------------------------------*/

void gfxengine_t::title(const char *win, const char *icon)
{
	_title = win;
	_icontitle = icon;
	//if(screen_surface) {
		// SDL_WM_SetCaption(_title, _icontitle); // IOHAVOC -- update API - what about the _icontitle
        SDL_SetWindowTitle(SDL_GL_GetCurrentWindow(), _title);
   // }
}


/*----------------------------------------------------------
	Display show/hide
----------------------------------------------------------*/

int gfxengine_t::show()
{
	int flags = 0;

	if(!is_open)
		return -1;

	if(is_showing)
		return 0;

	if(_centered && !_fullscreen) {
		// SDL_putenv((char *)"SDL_VIDEO_CENTERED=1");  // IOHAVOC -- Update API and environment variable
        SDL_setenv((char *)"SDL_VIDEO_CENTERED", (char*)"1",1);
    }
    
	log_printf(DLOG, "Opening screen...\n");
	if(!SDL_WasInit(SDL_INIT_VIDEO))
		if(SDL_InitSubSystem(SDL_INIT_VIDEO) == -1)
		{
			log_printf(ELOG, "Failed to initialize SDL!\n");
			return -2;
		}

	switch(_driver)
	{
	  case GFX_DRIVER_SDL2D:
		break;
	  case GFX_DRIVER_GLSDL:
		if(!_doublebuf)
		{
			log_printf(WLOG, "Only double buffering is supported"
					" with OpenGL drivers!\n");
			doublebuffer(1);
		}
		if(_shadow)
		{
			log_printf(WLOG, "Shadow buffer not supported"
					" with OpenGL drivers!\n");
			shadow(0);
		}
		break;
	}

	switch(_driver)
	{
	  case GFX_DRIVER_SDL2D:
		/* Nothing extra */
		break;
	  case GFX_DRIVER_GLSDL:
	  	flags |= SDL_GLSDL;
		break;
	}

    /* IOHAVOC -- thes flags are deprecated
	if(_doublebuf)
		flags |= SDL_DOUBLEBUF | SDL_HWSURFACE;
	else
	{
		if(!_shadow)
		  	flags |= SDL_HWSURFACE;
	} */

	if(_fullscreen)
		flags |= SDL_WINDOW_FULLSCREEN; // SDL_FULLSCREEN; // IOHAVOC

	glSDL_VSync(_vsync);
	flags |= xflags;

    
    //////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////

    // SDL2 - re-create the legacy "screen" surface and when we're all done, we're write to texture and render it.
    // screen_surface = SDL_SetVideoMode(_width, _height, _depth, flags);

    Uint32 Rmask = 0x000000ff;
    Uint32 Gmask = 0x0000ff00;
    Uint32 Bmask = 0x00ff0000;
    Uint32 Amask = 0xff000000;;
    screen_surface = SDL_CreateRGBSurface(SDL_SWSURFACE, //flags,
                                          _width,
                                          _height,
                                          32,            // depth,
                                          Rmask,
                                          Gmask,
                                          Bmask,
                                          Amask);
    
    // SDL2 - one stop window and renderer setup
    SDL_CreateWindowAndRenderer(_width,
                                _height,
                                SDL_WINDOW_SHOWN, // SDL_WINDOW_FULLSCREEN_DESKTOP
                                &sdl2window,
                                &sdlRenderer);
    
    // SDL2 - make a texture -- with the pixelformat SDL_PIXELFORMAT_ABGR8888
    sdlTexture = SDL_CreateTexture(sdlRenderer,
                                   SDL_PIXELFORMAT_ABGR8888,    // SDL_PIXELFORMAT_ARGB8888, SDL_PIXELFORMAT_RGBA8888
                                   SDL_TEXTUREACCESS_STREAMING, // SDL_TEXTUREACCESS_STATIC,
                                   _width, _height);
    if(NULL == sdl2window  ||
       NULL == sdlRenderer ||
       NULL == sdlTexture)
	{
		log_printf(ELOG, "Failed to open display -- sdlwindow or sdlRenderer!\n");
		return -3;
	}
    
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");  // make the scaled rendering look smoother.
    SDL_RenderSetLogicalSize(sdlRenderer, 640, 480);
    
    //////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////

    
    
    
    if(!screen_surface)
	{
		log_printf(ELOG, "Failed to open display!\n");
		return -3;
	}
    /*
	if(_driver != GFX_DRIVER_GLSDL)
	{
		if((screen_surface->flags)) // & SDL_DOUBLEBUF) == SDL_DOUBLEBUF)   // IOHAVOC - depcrecated
		{
			if(!_doublebuf)
			{
				log_printf(WLOG, "Could not get"
						" single buffered display.\n");
				doublebuffer(1);
			}
		}
		else
		{
			if(_doublebuf)
			{
				log_printf(WLOG, "Could not get"
						" double buffered display.\n");
				doublebuffer(0);
			}
		}
	} */

	//if((screen_surface->flags)) // & SDL_HWSURFACE) == SDL_HWSURFACE)   // IOHAVOC - deprecated
	//{
		if(_shadow)
		{
			softbuf = SDL_CreateRGBSurface(SDL_SWSURFACE,
					_width, _height,
					screen_surface->format->BitsPerPixel,
					screen_surface->format->Rmask,
					screen_surface->format->Gmask,
					screen_surface->format->Bmask,
					screen_surface->format->Amask);
			if(!softbuf)
			{
				log_printf(WLOG, "Failed to create shadow buffer! "
						"Trying direct rendering.\n");
				shadow(0);
			}
		}
	//}
	// else
	{
		if(_shadow)
			log_printf(WLOG, "Shadow buffer requested; "
					"relying on SDL's shadow buffer.\n");
		else
		{
			log_printf(WLOG, "Could not get h/w display surface.\n");
			shadow(0);	//...which means we're using SDL's shadow.
		}
	}

	// SDL_WM_SetCaption(_title, _icontitle); // IOHAVOC -- update API -- what about the icontitle?
    SDL_SetWindowTitle(SDL_GL_GetCurrentWindow(), _title);
    
	SDL_SetRelativeMouseMode(SDL_TRUE); // SDL_ShowCursor(_cursor); // IOHAVOC -- update API
	cs_engine_set_size(csengine, _width, _height);
	csengine->filter = use_interpolation;

	dither(_dither);
	noalpha(alpha_threshold);
	is_showing = 1;

	fullwin = new window_t;
	fullwin->init(this);
	fullwin->place(0, 0, (int)(_width / xscale()),
			(int)(_height / yscale()));

	clear();

	return 0;
}


void gfxengine_t::clear(Uint32 _color)
{
	if(!fullwin)
		return;

	fullwin->background(fullwin->map_rgb(_color));
	fullwin->clear();
	flip();
	fullwin->clear();
	flip();
	fullwin->clear();
}


void gfxengine_t::hide(void)
{
	if(!is_showing)
		return;

	log_printf(DLOG, "Closing screen...\n");
	stop();
	delete fullwin;
	fullwin = NULL;

	// Make sure no windows keep old surface pointers!
	window_t *w = windows;
	while(w)
	{
		if((w->surface == screen_surface) ||
				(w->surface == softbuf))
			w->surface = NULL;
		w = w->next;
	}

	if(softbuf)
	{
		SDL_FreeSurface(softbuf);
		softbuf = NULL;
	}
	screen_surface = NULL;

	is_showing = 0;
}


void gfxengine_t::invalidate(SDL_Rect *rect, window_t *window)
{
	switch(_pages)
	{
	  case -1:
		if(_doublebuf)
			__invalidate(1, rect, window);
		__invalidate(0, rect, window);
		break;
	  case 0:
		__invalidate(0, NULL, NULL);
		break;
	  case 3:
		__invalidate(2, rect, window);
		// Fallthrough!
	  case 2:
		__invalidate(1, rect, window);
		// Fallthrough!
	  case 1:
		__invalidate(0, rect, window);
		break;
	}
}


void gfxengine_t::__invalidate(int page, SDL_Rect *rect, window_t *window)
{
	if(!screen_surface)
		return;

	if(!rect || (_pages == 0))
	{
		dirtyrects[page] = 1;
		dirtytable[page][0].x = 0;
		dirtytable[page][0].y = 0;
		dirtytable[page][0].w = screen_surface->w;
		dirtytable[page][0].h = screen_surface->h;
		dirtywtable[page][0] = NULL;
		return;
	}

	/* Clip to screen (stolen from SDL_surface.c) */
	SDL_Rect dr = *rect;
	int Amin, Amax, Bmin, Bmax;

	/* Horizontal intersection */
	Amin = dr.x;
	Amax = Amin + dr.w;
	Bmin = screen_surface->clip_rect.x;
	Bmax = Bmin + screen_surface->clip_rect.w;
	if(Bmin > Amin)
	        Amin = Bmin;
	dr.x = Amin;
	if(Bmax < Amax)
	        Amax = Bmax;
	dr.w = Amax - Amin > 0 ? Amax - Amin : 0;

	/* Vertical intersection */
	Amin = dr.y;
	Amax = Amin + dr.h;
	Bmin = screen_surface->clip_rect.y;
	Bmax = Bmin + screen_surface->clip_rect.h;
	if(Bmin > Amin)
	        Amin = Bmin;
	dr.y = Amin;
	if(Bmax < Amax)
	        Amax = Bmax;
	dr.h = Amax - Amin > 0 ? Amax - Amin : 0;

	if(!dr.w || !dr.h)
		return;

	for(int i = 0; i < dirtyrects[page]; ++i)
		if(memcmp(&dirtytable[page][i], &dr, sizeof(dr)) == 0)
			return;

	if(dirtyrects[page] < MAX_DIRTYRECTS - 1)
	{
		dirtytable[page][dirtyrects[page]] = dr;
		dirtywtable[page][dirtyrects[page]] = window;
		++dirtyrects[page];
	}
	else
		log_printf(ELOG, "gfxengine: Page %d out of dirtyrects!\n",
				page);
}


/*----------------------------------------------------------
	Engine start/stop
----------------------------------------------------------*/

void gfxengine_t::start_engine()
{
	last_tick = -1000000;
}


void gfxengine_t::stop_engine()
{
}


/*----------------------------------------------------------
	Control
----------------------------------------------------------*/

/*
 * Run engine until stop() is called.
 *
 * The virtual member frame() will be called once for
 * each control system frame. pre_render() and
 * post_render() will be called before/after the engine
 * renders each video frame.
 */
void gfxengine_t::run()
{
	open();
	show();
	start_engine();
	is_running = 1;
	double toframe = 0.0f;
	double fdt = 1.0f;
	cs_engine_advance(csengine, 0);
	while(is_running)
	{
		int t = (int)SDL_GetTicks();
		int dt = t - last_tick;
		last_tick = t;
		if(abs(dt) > 500)
			last_tick = t;
		if(dt > 250)
			dt = (int)ticks_per_frame;
		if(_timefilter)
			fdt += (dt - fdt) * _timefilter;
		else
			fdt = ticks_per_frame;
		toframe += fdt / ticks_per_frame;
		cs_engine_advance(csengine, toframe);
		pre_render();
		window->select();
		cs_engine_render(csengine);
		post_render();
		if(_autoinvalidate)
			window->invalidate();
		flip();
	}
	stop_engine();
}


void gfxengine_t::stop()
{
	if(!is_running)
		return;

	log_printf(DLOG, "Stopping engine...\n");
	is_running = 0;
}


void gfxengine_t::cursor(int csr)
{
	_cursor = csr;
	if(screen_surface)
		SDL_ShowCursor(csr);
}


void gfxengine_t::interpolation(int inter)
{
	use_interpolation = inter;
	if(!csengine)
		return;

	csengine->filter = use_interpolation;
}


void gfxengine_t::timefilter(float coeff)
{
	if(coeff < 0.001f)
		_timefilter = 0.0f;
	else if(coeff > 1.0f)
		_timefilter = 1.0f;
	else
		_timefilter = coeff;
}


void gfxengine_t::scroll_ratio(int layer, float xr, float yr)
{
	if(layer < 0)
		return;
	if(layer >= CS_LAYERS)
		return;

	xratio[layer] = xr;
	yratio[layer] = yr;
}


void gfxengine_t::scroll(int xscr, int yscr)
{
	xscroll = xscr;
	yscroll = yscr;

	/* Apply current scroll pos to layers */
	for(int i = 0; i < CS_LAYERS ; ++i)
	{
		csengine->offsets[i].v.x = (int)floor(xscroll * xratio[i]);
		csengine->offsets[i].v.y = (int)floor(yscroll * yratio[i]);
	}
}


void gfxengine_t::force_scroll()
{
	if(csengine)
		for(int i = 0; i < CS_LAYERS ; ++i)
			cs_point_force(&csengine->offsets[i]);
}


int gfxengine_t::xoffs(int layer)
{
	if(layer < 0)
		return 0;
	if(layer >= CS_LAYERS)
		return 0;
	return csengine->offsets[layer].gx;
}


int gfxengine_t::yoffs(int layer)
{
	if(layer < 0)
		return 0;
	if(layer >= CS_LAYERS)
		return 0;
	return csengine->offsets[layer].gy;
}


SDL_Surface *gfxengine_t::surface()
{
	if(softbuf)
		return softbuf;
	else
		return screen_surface;
}


void gfxengine_t::screenshot()
{
	char filename[1024];
	snprintf(filename, sizeof(filename), "screen%d.bmp", screenshot_count++);
	SDL_SaveBMP(screen_surface, filename);
}


/*----------------------------------------------------------
	Internal stuff
----------------------------------------------------------*/

/* Default frame handler */
void gfxengine_t::frame()
{
	SDL_Event ev;
	while(SDL_PollEvent(&ev))
	{
		if(ev.type == SDL_KEYDOWN)
			if(ev.key.keysym.sym == SDLK_ESCAPE)
				stop();
	}
}


/* Internal frame handler */
void gfxengine_t::__frame()
{
}


void gfxengine_t::pre_render()
{
}


void gfxengine_t::post_render()
{
}


void gfxengine_t::refresh_rect(SDL_Rect *r)
{
	window_t *w = windows;
	for(w = windows; w; w = w->next)
	{
		if(!w->visible())
			continue;

		SDL_Rect dr = *r;

		/* Clip to window */
		int Amin, Amax, Bmin, Bmax;

		/* Horizontal intersection */
		Amin = dr.x;
		Amax = Amin + dr.w;
		Bmin = w->phys_rect.x;
		Bmax = Bmin + w->phys_rect.w;
		if(Bmin > Amin)
			Amin = Bmin;
		dr.x = Amin;
		if(Bmax < Amax)
			Amax = Bmax;
		dr.w = Amax - Amin > 0 ? Amax - Amin : 0;

		/* Vertical intersection */
		Amin = dr.y;
		Amax = Amin + dr.h;
		Bmin = w->phys_rect.y;
		Bmax = Bmin + w->phys_rect.h;
		if(Bmin > Amin)
			Amin = Bmin;
		dr.y = Amin;
		if(Bmax < Amax)
			Amax = Bmax;
		dr.h = Amax - Amin > 0 ? Amax - Amin : 0;

		if(dr.w && dr.h)
			w->phys_refresh(&dr);
	}
}


void gfxengine_t::flip()
{
	if(!screen_surface)
		return;

	// Init dirtyrect table flipping, if necessary.
	switch(_pages)
	{
	  case -1:
		if(!_doublebuf)
			frontpage = backpage = 0;
		else if(frontpage == backpage)
		{
			frontpage = 0;
			backpage = 1;
		}
		break;
	  case 0:
		frontpage = backpage = 0;
		invalidate();
		break;
	  case 1:
		frontpage = backpage = 0;
		break;
	  case 2:
	  case 3:
		if(frontpage == backpage)
		{
			frontpage = 0;
			backpage = 1;
		}
		break;
	}

	// Process the dirtyrects.
	int i;
	for(i = 0; i < dirtyrects[backpage]; ++i)
	{
		if(dirtywtable[backpage][i])
		{
			if(!dirtywtable[backpage][i]->visible())
				continue;
			SDL_Rect dr = dirtytable[backpage][i];
			dirtywtable[backpage][i]->phys_refresh(&dr);
		}
		else
			refresh_rect(&dirtytable[backpage][i]);
	}

	// Perform the actual flip or update
	if(_shadow)
	{
		for(i = 0; i < dirtyrects[backpage]; ++i)
			SDL_BlitSurface(softbuf,
                            &dirtytable[backpage][i],
                            screen_surface,
                            &dirtytable[backpage][i]);
	}
	if(_doublebuf)
	{
		dirtyrects[backpage] = 0;
		if(_pages == -1)
		{
			backpage = !backpage;
			frontpage = !frontpage;
		}
		else if(_pages > 1)
		{
			backpage = (backpage + 1) % _pages;
			frontpage = (frontpage + 1) % _pages;
		}
		
        // IOHAVOC -- deprecated -- use SDL_RenderPresent(renderer) instead
        //SDL_Flip(screen_surface);
        
        
        // IOHAVOC -- SDL2 replacement.
        
        SDL_UpdateTexture(sdlTexture, NULL, screen_surface->pixels, screen_surface->pitch);
        
        SDL_RenderClear(sdlRenderer);
        SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, NULL);
        SDL_RenderPresent(sdlRenderer);
	}
	else
	{
        // IOHAVOC -- deprecated -- use SDL_RenderPresent(renderer) instead
		// SDL_UpdateRects(screen_surface, dirtyrects[0], dirtytable[0]);
		dirtyrects[0] = 0;
        
        // IOHAVOC -- replacement. Stuff is being collected into sdlRenderer
        SDL_RenderPresent(sdlRenderer);
	}
}


/*
 * Generic render() callback for sprites and tiles.
 */
void gfxengine_t::render_sprite(cs_obj_t *o)
{
	SDL_Rect dest_rect;
	s_sprite_t *s = gfxengine->get_sprite(o->anim.bank, o->anim.frame);
	if(!s || !s->surface)
		return;

	int x = o->point.gx - (s->x << 8);
	int y = o->point.gy - (s->y << 8);
	dest_rect.x = CS2PIXEL((x * gfxengine->xs + 128) >> 8);
	dest_rect.y = CS2PIXEL((y * gfxengine->ys + 128) >> 8);
	dest_rect.x += (gfxengine->window->x() * gfxengine->xs + 128) >> 8;
	dest_rect.y += (gfxengine->window->y() * gfxengine->xs + 128) >> 8;
	
    SDL_BlitSurface(s->surface, NULL, gfxengine->surface(), &dest_rect);

	if(!gfxengine->_autoinvalidate)
	{
		dest_rect.w = s->surface->w;
		dest_rect.h = s->surface->h;
		gfxengine->invalidate(&dest_rect);
	}
}


cs_obj_t *gfxengine_t::get_obj(int layer)
{
	cs_obj_t *o = cs_engine_get_obj(csengine);
	if(o)
	{
		o->render = render_sprite;
		cs_obj_layer(o, layer);
		cs_obj_activate(o);
	}
	return o;
}


void gfxengine_t::free_obj(cs_obj_t *obj)
{
	cs_obj_free(obj);
}


int gfxengine_t::objects_in_use()
{
	if(!csengine)
		return 0;

	return csengine->pool_total - csengine->pool_free;
}


SoFont *gfxengine_t::get_font(unsigned int f)
{
	if(f < GFX_BANKS)
		return fonts[f];
	else
		return NULL;
}
