/*(LGPL)
----------------------------------------------------------------------
	sprite.c - Sprite engine for use with cs.h
----------------------------------------------------------------------
 * Copyright (C) 2001, 2003, 2007 David Olofson
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

/*
TODO: Automatic extension of the bank and sprite
TODO: tables as needed when loading banks.
*/

#define	DBG(x)	x

#include <stdlib.h>
#include <string.h>
#include "logger.h"
#include "glSDL.h"
#include "SDL_image.h"
#include "sprite.h"
#include "filters.h"


s_blitmodes_t s_blitmode = S_BLITMODE_AUTO;
pix_t s_colorkey = {0, 0, 0, 0};
pix_t s_clampcolor = {0, 0, 0, 0};
unsigned char s_alpha = SDL_ALPHA_OPAQUE;
int s_filter_flags = 0;


s_filter_t *filters = NULL;


/*
----------------------------------------------------------------------
	Filter Plugin Interface
----------------------------------------------------------------------
 */

s_filter_t *s_insert_filter(s_filter_cb_t callback)
{
	s_filter_t *nf = calloc(1, sizeof(s_filter_t));
	if(!nf)
		return NULL;

	nf->callback = callback;
	nf->next = filters;
	filters = nf;
	return nf;
}


s_filter_t *s_add_filter(s_filter_cb_t callback)
{
	s_filter_t *nf = calloc(1, sizeof(s_filter_t));
	if(!nf)
		return NULL;

	nf->callback = callback;
	if(!filters)
		filters = nf;
	else
	{
		s_filter_t *f = filters;
		while(f->next)
			f = f->next;
		f->next = nf;
	}
	return nf;
}


/* filter == NULL means "remove all plugins" */
void s_remove_filter(s_filter_t *filter)
{
	s_filter_t *f;
	if(!filters)
		return;

	f = filters;
	while(f->next)
	{
		if(f->next == filter || !filter)
		{
			s_filter_t *df = f->next;
			f->next = f->next->next;
			free(df);
		}
		else
			f = f->next;
	}
	if(filters == filter || !filter)
	{
		s_filter_t *df = filters;
		filters = filters->next;
		free(df);
	}
}


static void __run_plugins(s_bank_t *b, unsigned first, unsigned frames)
{
	s_filter_t *f = filters;
	while(f)
	{
		int oflags = f->args.flags;
		f->args.flags |= s_filter_flags;
		f->callback(b, first, frames, &f->args);
		f->args.flags = oflags;
		f = f->next;
	}	
}


/*
----------------------------------------------------------------------
	Basic Container Management
----------------------------------------------------------------------
 */

static int __alloc_bank_table(s_container_t *c, unsigned int count)
{
	c->max = count - 1;
	c->banks = (s_bank_t **)calloc((unsigned)count, sizeof(s_bank_t *));
	return -(c->banks == NULL);
}

s_container_t *s_new_container(unsigned banks)
{
	s_container_t *ret = (s_container_t *)calloc(1, sizeof(s_container_t));
	if(!ret)
		return NULL;
	if(__alloc_bank_table(ret, banks) < 0)
		return NULL;
	return ret;
}

void s_delete_container(s_container_t *c)
{
	s_delete_all_banks(c);
	free(c->banks);
	free(c);
}


/*
 * Allocates a new sprite.
 * If the sprite exists already, it's surface will be removed, so
 * that one can safely expect to get an *empty* sprite.
 */
s_sprite_t *s_new_sprite_b(s_bank_t *b, unsigned frame)
{
	s_sprite_t	*s = NULL;
	if(frame > b->max)
		return NULL;
	if(!b->sprites[frame])
	{
		s = calloc(1, sizeof(s_sprite_t));
		if(!s)
			return NULL;
		b->sprites[frame] = s;
	}
	else
	{
		if(s->surface)
			SDL_FreeSurface(s->surface);
		s->surface = NULL;
	}
	return s;
}


/*
 * Allocates a new sprite.
 * If the sprite exists already, it's surface will be removed, so
 * that one can safely expect to get an *empty* sprite.
 *
 * If the bank does not exist, or if the operation failed for
 * other reasons, this call returns NULL.
 */
s_sprite_t *s_new_sprite(s_container_t *c, unsigned bank, unsigned frame)
{
	if(bank > c->max)
		return NULL;
	if(!c->banks[bank])
		return NULL;

	return s_new_sprite_b(c->banks[bank], frame);
}


void s_delete_sprite_b(s_bank_t *b, unsigned frame)
{
	if(!b->sprites)	/* This can happen when failing to create a new bank */
		return;
	if(frame > b->max)
		return;
	if(!b->sprites[frame])
		return;
	if(b->sprites[frame]->surface)
		SDL_FreeSurface(b->sprites[frame]->surface);
	b->sprites[frame]->surface = NULL;
	free(b->sprites[frame]);
	b->sprites[frame] = NULL;
}


void s_delete_sprite(s_container_t *c, unsigned bank, unsigned frame)
{
	s_bank_t *b = s_get_bank(c, bank);
	if(b)
		s_delete_sprite_b(b, frame);
}


static int __alloc_sprite_table(s_bank_t *b, unsigned frames)
{
	b->max = frames - 1;
	b->sprites = (s_sprite_t **)calloc(frames, sizeof(s_sprite_t *));
	if(!b->sprites)
		return -1;
	return 0;
}


s_bank_t *s_new_bank(s_container_t *c, unsigned bank, unsigned frames,
				unsigned w, unsigned h)
{
	s_bank_t *b;
	DBG(log_printf(DLOG, "s_new_bank(%p, %d, %d, %d, %d)\n",
			c, bank, frames, w, h);)
	if(bank > c->max)
		return NULL;
	if(c->banks[bank])
		s_delete_bank(c, bank);

	c->banks[bank] = (s_bank_t *)calloc(1, sizeof(s_bank_t));
	if(!c->banks[bank])
		return NULL;
	if(__alloc_sprite_table(c->banks[bank], frames) < 0)
	{
		s_delete_bank(c, bank);
		return NULL;
	}
	b = c->banks[bank];
	b->max = frames - 1;
	b->w = w;
	b->h = h;
	return b;
}


void s_delete_bank(s_container_t *c, unsigned bank)
{
	unsigned i;
	s_bank_t *b;
	if(!c->banks)
		return;
	if(bank > c->max)
		return;
	b = c->banks[bank];
	if(b)
	{
		for(i = 0; i <= b->max; ++i)
			s_delete_sprite_b(b, i);
		free(b->sprites);
		free(b);
		c->banks[bank] = NULL;
	}
}


void s_delete_all_banks(s_container_t *c)
{
	unsigned i;
	for(i = 0; i <= c->max; ++i)
		s_delete_bank(c, i);
}



/*
----------------------------------------------------------------------
	Getting data
----------------------------------------------------------------------
 */

s_bank_t *s_get_bank(s_container_t *c, unsigned bank)
{
	if(bank > c->max)
		return NULL;
	if(!c->banks[bank])
		return NULL;
	return c->banks[bank];
}

s_sprite_t *s_get_sprite(s_container_t *c, unsigned bank, unsigned frame)
{
	s_bank_t *b = s_get_bank(c, bank);
	if(!b)
		return NULL;
	if(frame > b->max)
		return NULL;
	return b->sprites[frame];
}

s_sprite_t *s_get_sprite_b(s_bank_t *b, unsigned frame)
{
	if(!b)
		return NULL;
	if(frame > b->max)
		return NULL;
	return b->sprites[frame];
}

void s_detach_sprite(s_container_t *c, unsigned bank, unsigned frame)
{
	s_sprite_t *s = s_get_sprite(c, bank, frame);
	if(!s)
		return;
	s->surface = NULL;
}


/*
----------------------------------------------------------------------
	File tools
----------------------------------------------------------------------
 */

static int extract_sprite(s_bank_t *bank, unsigned frame,
				SDL_Surface *src, SDL_Rect *from)
{
	int y;
	SDL_Surface	*tmp;
	if(frame > bank->max)
	{
		log_printf(ELOG, "sprite: Too many frames!\n");
		return -1;
	}
	if(!s_new_sprite_b(bank, frame))
		return -2;

	tmp = SDL_CreateRGBSurface(src->flags,
			from->w, from->h,
			src->format->BitsPerPixel,
			src->format->Rmask,
			src->format->Gmask,
			src->format->Bmask,
			src->format->Amask	);
	if(!tmp)
		return -3;

	/* Copy the pixel data */
	if(SDL_LockSurface(src) < 0)
	{
		log_printf(ELOG, "sprite: extract_sprite() failed to lock surface!\n");
		SDL_FreeSurface(tmp);
		return -4;
	}
	for(y = 0; y < tmp->h; ++y)
	{
		char *s = (char *)src->pixels + src->pitch * (y + from->y) +
				from->x * src->format->BytesPerPixel;
		char *d = (char *)tmp->pixels + tmp->pitch * y;
		memcpy(d, s, src->format->BytesPerPixel * tmp->w);
	}
	SDL_UnlockSurface(src);

	/* Copy palette, if any */
	if(src->format->palette)
		SDL_SetColors(tmp, src->format->palette->colors, 0,
				src->format->palette->ncolors);

	/* Copy alpha and colorkey */
	if(src->flags & SDL_SRCALPHA)
		SDL_SetAlpha(tmp, src->flags & (SDL_SRCALPHA | SDL_RLEACCEL),
				src->format->alpha);
	if(src->flags & SDL_SRCCOLORKEY)
		SDL_SetColorKey(tmp, src->flags & (SDL_SRCCOLORKEY | SDL_RLEACCEL),
				src->format->colorkey);

	bank->sprites[frame]->surface = tmp;

	DBG(log_printf(DLOG, "image %d: (%d,%d)/%dx%d @ %p\n", frame,
			from->x, from->y, from->w, from->h,
			bank->sprites[frame]->surface);)
	return 0;
}


int s_copy_rect(s_container_t *c, unsigned bank,
		unsigned frombank, unsigned fromframe, SDL_Rect *from)
{
#if 0
	s_filter_args_t args;
#endif
	SDL_Surface	*src;
	s_sprite_t	*s;
	s_bank_t	*b;

	/* Get source image */
	s = s_get_sprite(c, frombank, fromframe);
	if(!s)
	{
		log_printf(ELOG, "sprite: Couldn't get source sprite %d:%d!\n",
				frombank, fromframe);
		return -1;
	}
	src = s->surface;
	if(!src)
	{
		log_printf(ELOG, "sprite: Sprite %d:%d"
				" does not have a surface!\n",
				frombank, fromframe);
		return -2;
	}

	/* Create destination bank */
	b = s_new_bank(c, bank, 1, from->w, from->h);
	if(!b)
	{
		log_printf(ELOG, "sprite: Failed to allocate bank %d!\n", bank);
		return -3;
	}

	if(extract_sprite(b, 0, src, from) < 0)
	{
		log_printf(ELOG, "sprite: Something went wrong while "
				"copying from sprite %d:%d.\n",
				frombank, fromframe);
		return -4;
	}
#if 0
	memset(&args, 0, sizeof(args));
	args.x = 0;
	s_filter_displayformat(b, 0, 1, &args);
#endif
	return 0;
}


int s_load_image(s_container_t *c, unsigned bank, const char *name)
{
	SDL_Surface	*src;
	s_bank_t	*b;

	src = IMG_Load(name);
	if(!src)
	{
		log_printf(ELOG, "sprite: Failed to load sprite \"%s\"!\n", name);
		return -1;
	}

	b = s_new_bank(c, bank, 1, src->clip_rect.w, src->clip_rect.h);
	if(!b)
	{
		log_printf(ELOG, "sprite: Failed to allocate bank for \"%s\"!\n", name);
		return -2;
	}

	if(extract_sprite(b, 0, src, &src->clip_rect) < 0)
	{
		log_printf(ELOG, "sprite: Something went wrong while"
				" extracting sprite \"%s\".\n", name);
		return -3;
	}

	SDL_FreeSurface(src);
	__run_plugins(b, 0, 1);
	return 0;
}


int s_load_sprite(s_container_t *c, unsigned bank, unsigned frame,
					const char *name)
{
	SDL_Surface	*src;
	SDL_Rect	from;
	s_bank_t	*b = s_get_bank(c, bank);
	if(!b)
	{
		log_printf(ELOG, "sprite: While loading \"%s\":"
				" Bank %d does not exist!\n", name, bank);
		return -2;
	}

	src = IMG_Load(name);
	if(!src)
	{
		log_printf(ELOG, "sprite: Failed to load sprite \"%s\"!\n", name);
		return -1;
	}

	from = src->clip_rect;
	if( (from.w != b->w) || (from.h != b->h) )
	{
		log_printf(ELOG, "sprite: Warning: Sprite \"%s\" cropped"
				" to fit in bank %d.\n", name, bank);
		from.w = b->w;
		from.h = b->h;
	}

	if(extract_sprite(b, frame, src, &from) < 0)
	{
		log_printf(ELOG, "sprite: Something went wrong while"
				" extracting sprite \"%s\".\n", name);
		return -3;
	}

	SDL_FreeSurface(src);
	__run_plugins(b, frame, 1);
	return 0;
}


int s_load_bank(s_container_t *c, unsigned bank, unsigned w, unsigned h,
					const char *name)
{
	SDL_Surface	*src;
	s_bank_t	*b;
	int		x, y;
	unsigned	frame = 0;
	unsigned	frames;

	DBG(log_printf(DLOG, "s_load_bank(%p, %d, %d, %d, %s)\n",
			c, bank, w, h, name);)

	src = IMG_Load(name);
	if(!src)
	{
		log_printf(ELOG, "sprite: Failed to load sprite palette %s!\n", name);
		return -1;
	}

	if(w > src->w)
	{
		log_printf(ELOG, "sprite: Source image %s not wide enough!\n", name);
		return -4;
	}

	if(h > src->h)
	{
		log_printf(ELOG, "sprite: Source image %s not high enough!\n", name);
		return -5;
	}

	frames = (src->w / w) * (src->h / h);
	b = s_new_bank(c, bank, frames, w, h);
	if(!b)
	{
		log_printf(ELOG, "sprite: Failed to allocate bank for \"%s\"!\n", name);
		return -2;
	}
	for(y = 0; y <= src->h - h; y += h)
		for(x = 0; x <= src->w - w; x += w)
		{
			SDL_Rect r;
			r.x = x;
			r.y = y;
			r.w = w;
			r.h = h;
			if(extract_sprite(b, frame, src, &r) < 0)
			{
				log_printf(ELOG, "sprite: Something went wrong while"
						" extracting sprites.\n");
				return -3;
			}
			++frame;
		}
	SDL_FreeSurface(src);
	__run_plugins(b, 0, frames);
	return 0;
}


int s_set_hotspot(s_container_t *c, unsigned bank, int frame, int x, int y)
{
	if(frame < 0)
	{
		frame = 0;
		while(1)
		{
			s_sprite_t *s = s_get_sprite(c, bank, frame++);
			if(!s)
				break;
			s->x = x;
			s->y = y;
		}
	}
	else
	{
		s_sprite_t *s = s_get_sprite(c, bank, frame);
		s->x = x;
		s->y = y;
	}
	return 0;
}
