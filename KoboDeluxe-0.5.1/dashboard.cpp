/*(GPL)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright (C) 2003, 2007 David Olofson
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

#include "config.h"
#include "kobo.h"
#include "random.h"
#include <stdlib.h>
#include <string.h>

#define MAX_PROGRESS	1000


/*----------------------------------------------------------
	Screen
----------------------------------------------------------*/

void screen_window_t::border(int top, int left, int right, int bottom)
{
	_top = top;
	_left = left;
	_right = right;
	_bottom = bottom;
}


void screen_window_t::refresh(SDL_Rect *r)
{
	int x, y, w, h;
	foreground(map_rgb(0x000000));
	x = 0;
	y = 0;
	w = width();
	h = _top;
	fillrect(x, y, w, h);

	y = height() - _bottom;
	h = _bottom;
	fillrect(x, y, w, h);

	y = _top;
	w = _left;
	h = height() - _top - _bottom;
	fillrect(x, y, w, h);

	x = width() - _right;
	w = _right;
	fillrect(x, y, w, h);
}


/*----------------------------------------------------------
	Dashboard
----------------------------------------------------------*/

dashboard_window_t::dashboard_window_t()
{
	_mode = DASHBOARD_BLACK;
	_percent = 0.0f;
	_msg = NULL;
}


dashboard_window_t::~dashboard_window_t()
{
	free(_msg);
}


void dashboard_window_t::mode(dashboard_modes_t m)
{
	int vis;
	_mode = m;
	switch(_mode)
	{
	  case DASHBOARD_GAME:
		vis = 1;
		break;
	  default:
		vis = 0;
		break;
	}
	wmain->visible(vis);
	whealth->visible(vis);
	wtemp->visible(vis);
	wttemp->visible(vis);
	wradar->visible(vis);
	dhigh->visible(vis);
	dscore->visible(vis);
	dstage->visible(vis);
	dships->visible(vis);
	switch(_mode)
	{
	  case DASHBOARD_OFF:
		break;
	  default:
		gengine->invalidate();
		gengine->flip();
		gengine->invalidate();
		gengine->flip();
		gengine->invalidate();
		break;
	}
}


void dashboard_window_t::doing(const char *msg)
{
	free(_msg);
	_msg = strdup(msg);
	invalidate();
	gengine->flip();
}


void dashboard_window_t::progress_init(float *progtab)
{
	if(!progtab)
	{
		progress_table = (float *)malloc(MAX_PROGRESS * sizeof(float));
		progress_bench = (progress_table != NULL);
	}
	else
	{
		progress_table = progtab;
		progress_bench = 0;
	}
	progress_index = 0;
}


void dashboard_window_t::progress()
{
	if(progress_table)
	{
		if(progress_bench)
		{
			progress_table[progress_index++] = (float)SDL_GetTicks();
			_percent = 0.0f;
		}
		else
			_percent = progress_table[progress_index++];
	}
	else
		_percent = 50.0f;
	invalidate();
	gengine->flip();
}


void dashboard_window_t::progress_done()
{
	if(progress_bench)
	{
		int i;
		int total = SDL_GetTicks() - (int)progress_table[0];
		printf("Progress percentages:\n");
		printf("---------------------\n");
		for(i = 0; i < progress_index; ++i)
			printf("\t%f,\n", (progress_table[i] -
					progress_table[0]) * 100.0f / total);
		printf("---------------------\n");
		free(progress_table);
		progress_bench = 0;
	}
	progress_table = NULL;
}


void dashboard_window_t::render_progress()
{
	SDL_Rect r;
	int x, y, w, h;

	x = 0;
	w = (int)(_percent * 0.01f * width() + 0.5f);
	if(w < 4)
		w = 4;
	else if(w > width())
		w = width();

	h = 16;
	y = height() - h;

	r.x = x;
	r.y = y;
	r.w = w;
	r.h = h;

	foreground(map_rgb(0x000099));
	rectangle(x, y, w, h);

	++x;
	++y;
	w -= 2;
	h -= 2;
	foreground(map_rgb(0x0000cc));
	rectangle(x, y, w, h);

	++x;
	++y;
	w -= 2;
	h -= 2;
	foreground(map_rgb(0x0000ff));
	fillrect(x, y, w, h);
	
	r.x = 0;
	r.y = height() - 40;
	r.w = width();
	r.h = 12;
	foreground(map_rgb(0x000000));
	fillrect(r.x, r.y, r.w, r.h);
	if(_msg)
	{
		font(B_NORMAL_FONT);
		center(height() - 40, _msg);
	}
}


#define	NIBBLE_W	4
#define	NIBBLE_H	4
#define	NIBBLE_TILES	((SCREEN_WIDTH/NIBBLE_W+1)*(SCREEN_HEIGHT/NIBBLE_H+1))

void dashboard_window_t::nibble(int tool)
{
	int i;
	int x[NIBBLE_TILES];
	int y[NIBBLE_TILES];

	mode(DASHBOARD_OFF);

	if(tool < 0)
		tool = (pubrand.get(20) + SDL_GetTicks()) % 5;

	/* Clear */
	for(i = 0; i < NIBBLE_TILES; ++i)
		x[i] = y[i] = -1;

	/* Fill in */
	int ind = 0;
	int xx = -NIBBLE_W / 2;
	int yy = -NIBBLE_H / 2;
	for(i = 0; i < NIBBLE_TILES; ++i)
	{
		ind = pubrand.get() % NIBBLE_TILES;
		if(ind >= NIBBLE_TILES)
			ind = 0;
		while(x[ind] != -1)
			if(++ind >= NIBBLE_TILES)
				ind = 0;
		x[ind] = xx;
		y[ind] = yy;
		xx += NIBBLE_W;
		if(xx >= SCREEN_WIDTH + NIBBLE_W / 2)
		{
			xx = -NIBBLE_W / 2;
			yy += NIBBLE_H;
		}
	}

	/* Clear 8000 tiles/second, until all are done. */
	foreground(map_rgb(0x000000));
	ind = 0;
	int last_index;
	int t = SDL_GetTicks();
	while(ind < NIBBLE_TILES)
	{
		int nt = SDL_GetTicks();
		int dt = nt - t;
		t = nt;
		last_index = ind;	/* For double buffer mode */
		for(i = 0; i < dt * 8; ++i)
		{
			if(ind >= NIBBLE_TILES)
				break;
			switch (tool)
			{
			  case 0:
				fillrect(x[ind] + NIBBLE_W / 2,
						y[ind] + NIBBLE_H / 2,
						NIBBLE_W, NIBBLE_H);
				break;
			  default:
				sprite(x[ind] - 8 + NIBBLE_W / 2,
						y[ind] - 8 +
						NIBBLE_H / 2, B_BRUSHES,
						tool - 1);
				break;
			}
			++ind;
		}
		gengine->invalidate();
		gengine->flip();
		if(gengine->doublebuffer())
		{
			for(i = 0; i < dt * 4; ++i)
			{
				if(last_index >= NIBBLE_TILES)
					break;
				switch (tool)
				{
				  case 0:
					fillrect(x[last_index] +
							NIBBLE_W / 2,
							y[last_index] +
							NIBBLE_H / 2,
							NIBBLE_W,
							NIBBLE_H);
					break;
				  default:
					sprite(x[last_index] - 8 +
							NIBBLE_W / 2,
							y[last_index] - 8 +
							NIBBLE_H / 2,
							B_BRUSHES,
							tool - 1);
					break;
				}
				++last_index;
			}
		}
	}
	mode(DASHBOARD_BLACK);
	gengine->flip();
}


void dashboard_window_t::refresh(SDL_Rect *r)
{
	switch(_mode)
	{
	  case DASHBOARD_OFF:
		break;
	  case DASHBOARD_BLACK:
		background(map_rgb(0x000000));
		clear();
		break;
	  case DASHBOARD_GAME:
		sprite(0, 0, B_SCREEN, 0);
		break;
	  case DASHBOARD_LOADING:
		background(map_rgb(0x000000));
		clear();
		sprite((width() - 250) / 2, 55, B_LOADING, 0);
		render_progress();
		break;
	}
}


/*----------------------------------------------------------
	Bar graph display
----------------------------------------------------------*/

bargraph_t::bargraph_t()
{
	_value = 0.0f;
	_redmax = 1;
	_y = -1000;
	_enabled = 1;
}


void bargraph_t::value(float val)
{
	if(val < 0.0f)
		_value = 0.0f;
	else
		_value = val;
	if(!_enabled)
		return;
	int y = (int)((height() - 2) * _value);
	if(y != _y)
		invalidate();
}


void bargraph_t::enable(int ena)
{
	_enabled = ena;
	invalidate();
}


void bargraph_t::refresh(SDL_Rect *r)
{
	if(!_enabled)
	{
		clear();
		return;
	}
	float v = _value;
	int red, green, blue;
	if(v > 1.0f)
	{
		blue = 50 + (v - 1.0f) * 512;
		if(_redmax)
		{
			red = 255;
			green = blue / 2;
		}
		else
		{
			green = 255;
			red = blue / 2;
		}
		v = 1.0f;
	}
	else
	{
		if(_redmax)
		{
			red = (int)(v * 300.0);
			green = (int)((1.0 - v) * 400.0);
		}
		else
		{
			red = (int)((1.0 - v) * 300.0);
			green = (int)(v * 400.0);
		}
		blue = 50;
	}
	if(green > 180)
		green = 180;
	if(red > 230)
		red = 230;
	if(blue > 255)
		blue = 255;
	_y = (int)((height() - 2) * (1.0f - v));
	foreground(bgcolor);
	fillrect(0, 0, width(), height());
	foreground(map_rgb(red, green, blue));
	fillrect(1, _y + 1, width() - 2, height() - _y - 2);
}
