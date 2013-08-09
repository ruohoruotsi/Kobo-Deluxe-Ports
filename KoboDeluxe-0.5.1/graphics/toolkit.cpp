/*(LGPL)
---------------------------------------------------------------------------
	toolkit.cpp - Simple "GUI" toolkit for config screens.
---------------------------------------------------------------------------
 * Copyright (C) 2001 David Olofson
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

#include "config.h"

#include <string.h>
#include <stdlib.h>

#include "window.h"
#include "toolkit.h"



/*----------------------------------------------------------
	ct_widget_t::
----------------------------------------------------------*/

ct_widget_t::ct_widget_t()
{
	interactive = 0;
	transparent = 1;
	highlighted = 0;
	user = NULL;
	user2 = 0;
	tag = 0;
	_color = 0;
	next = prev = NULL;
	_widget_index = -1;
	halign(ALIGN_DEFAULT);
	valign(ALIGN_DEFAULT);
	_token = 0;
	xo = yo = 0.0;
}


void ct_widget_t::halign(ct_align_t ha)
{
	if(ALIGN_DEFAULT == ha)
		_halign = ALIGN_RIGHT;
	else
		_halign = ha;
}


void ct_widget_t::valign(ct_align_t va)
{
	if(ALIGN_DEFAULT == va)
		_valign = ALIGN_TOP;
	else
		_valign = va;
}


void ct_widget_t::init(gfxengine_t *e)
{
	window_t::init(e);
	_color = map_rgb(0,0,0);
}


void ct_widget_t::transparency(int t)
{
	if(t == transparent)
		return;

	transparent = t;
	if(!transparent)
		render();
}


void ct_widget_t::highlight(int hl)
{
	if(hl == highlighted)
		return;

	highlighted = hl;
	if(!transparent)
		render();
}


void ct_widget_t::color(Uint32 _cl)
{
	if(_cl == _color)
		return;

	_color = _cl;
	if(!transparent)
		render();
}


void ct_widget_t::render()
{
	if(!transparent)
	{
		foreground(_color);
		fillrect(0, 0, width(), height());
	}
	if(highlighted)
		ct_engine.render_highlight(this);
}


/* Handy tool function used by some descendents. */
void ct_widget_t::render_text_aligned(const char *buf)
{
	int yy = 0;
	switch(_valign)
	{
	  case ALIGN_DEFAULT:	//dummy
	  case ALIGN_NONE:
	  case ALIGN_TOP:
		yy = yoffs();
		break;
	  case ALIGN_BOTTOM:
		yy = height() - fontheight() - yoffs();
		break;
	  case ALIGN_CENTER:
	  case ALIGN_CENTER_TOKEN:
		yy = (height() - fontheight()) / 2 + yoffs();
		break;
	}
	switch(_halign)
	{
	  case ALIGN_DEFAULT:	//dummy
	  case ALIGN_NONE:
	  case ALIGN_LEFT:
		string(xoffs(), yy, buf);
		break;
	  case ALIGN_RIGHT:
		center_token(width() - xoffs(), yy, buf);
		break;
	  case ALIGN_CENTER:
		center_token(xoffs(), yy, buf, -1);
		break;
	  case ALIGN_CENTER_TOKEN:
		center_token(xoffs(), yy, buf, _token);
		break;
	}
}


void ct_widget_t::change(int delta)
{
	_value += delta;
}


void ct_widget_t::value(int val)
{
	_value = val;
}


int ct_widget_t::value()
{
	return _value;
}



/*----------------------------------------------------------
	ct_engine_t::
----------------------------------------------------------*/

void default_render_highlight(ct_widget_t *wg)
{
	Uint32 hlc[6];
	hlc[0] = wg->map_rgb(0x440000);
	hlc[1] = wg->map_rgb(0x660000);
	hlc[2] = wg->map_rgb(0x880000);
	hlc[3] = wg->map_rgb(0x990000);
	hlc[4] = wg->map_rgb(0xaa0000);
	hlc[5] = wg->map_rgb(0xbb0000);
	wg->foreground(hlc[5]);
	wg->fillrect(0, 6, wg->width(), wg->height()-6-1);
	for(int i = 0; i < 6; ++i)
	{
		wg->foreground(hlc[i]);
		wg->fillrect(0, i, wg->width(), 1);
		wg->fillrect(0, wg->height()-1-i, wg->width(), 1);
	}
}


ct_engine_t::ct_engine_t()
{
	ct_engine.render_highlight = default_render_highlight;
}


ct_engine_t ct_engine;



/*----------------------------------------------------------
	ct_label_t::
----------------------------------------------------------*/

ct_label_t::ct_label_t(const char *cap)
{
	xo = 0.5;
	if(cap)
		caption(cap);
	else
		caption("Label");
}


void ct_label_t::halign(ct_align_t ha)
{
	if(ALIGN_DEFAULT == ha)
		_halign = ALIGN_CENTER;
	else
		_halign = ha;
}


void ct_label_t::caption(const char *cap)
{
	strncpy(_caption, cap, sizeof(_caption));
	_caption[sizeof(_caption)-1] = 0;
	if(!transparent)
		render();
}


void ct_label_t::render()
{
	ct_widget_t::render();
	render_text_aligned(_caption);
}


/*----------------------------------------------------------
	ct_item_t::
----------------------------------------------------------*/

ct_item_t::ct_item_t(const char *cap, int val)
{
	if(cap)
		caption(cap);
	else
		caption("Item");
	_value = val;
}


void ct_item_t::caption(const char *cap)
{
	strncpy(_caption, cap, sizeof(_caption));
	_caption[sizeof(_caption)-1] = 0;
}


static ct_item_t dummy_item("<error>", -1);


/*----------------------------------------------------------
	ct_list_t::
----------------------------------------------------------*/

ct_list_t::ct_list_t(const char *cap) : ct_label_t(cap)
{
	interactive = 1;
	if(cap)
		caption(cap);
	else
		caption("List");
	items = NULL;
	_selected = NULL;

	_token = ':';
	xo = 0.55;
}


ct_list_t::~ct_list_t()
{
	clean();
}


void ct_list_t::halign(ct_align_t ha)
{
	if(ALIGN_DEFAULT == ha)
		_halign = ALIGN_CENTER_TOKEN;
	else
		_halign = ha;
}


void ct_list_t::clean()
{
	if(!items)
		return;

	/*
	 * Break the circular list first, or we'll
	 * blow up the memory manager...! :-)
	 */
	items->prev->next = NULL;

	while(items)
	{
		ct_item_t *i = items;
		items = i->next;
		delete i;
	}

	items = NULL;
	_selected = NULL;
}


void ct_list_t::add(ct_item_t *item)
{
	if(items)
	{
		item->next = items;
		item->prev = items->prev;
		items->prev->next = item;
		items->prev = item;
	}
	else
	{
		item->next = item->prev = item;
		items = item;
		_selected = item;
		if(!transparent)
			render();
	}
}


void ct_list_t::add(const char *cap, int val)
{
	ct_item_t *it = new ct_item_t(cap, val);
	add(it);
}


void ct_list_t::select(ct_item_t *item)
{
	_selected = item;
#if 0
	ct_item_t *i = items;
	if(i)
		for(int n = 0; n < 10000; ++n)	//Just in case...
		{
			if(i == _selected)
			{
				_value = n;
				break;
			}
			i = i->next;
		}
#endif
	if(_selected)
		_value = _selected->value();
	if(!transparent)
		render();
}


void ct_list_t::select(int ind)
{
	if(!items)
		return;

	_selected = items;
	for(int i = 0; i < ind; ++i)
		_selected = _selected->next;
	select(_selected);	//To update _value, render etc...
}


ct_item_t *ct_list_t::selected()
{
	if(!items)
		return &dummy_item;

	return _selected;
}


void ct_list_t::value(int val)
{
	if(!items)
		return;

	ct_item_t *best = items;
	_selected = items;
	while(_selected->value() != val)
	{
		if(labs(_selected->value()-val) < labs(best->value()-val))
			best = _selected;
		if(_selected->next == items)
			break;
		_selected = _selected->next;
	}
	if(_selected->value() != val)
		_selected = best;
	select(_selected);	//To update _value, render etc...
}


int ct_list_t::value()
{
	if(!items)
		return dummy_item.value();
	if(_selected)
		return _selected->value();
	else
		return dummy_item.value();
}


void ct_list_t::change(int delta)
{
	if(_selected)
	{
		if(0 == delta)
			delta = 1;
		if(delta > 0)
			for(int i = 0; i < delta; ++i)
				_selected = _selected->next;
		else if(delta < 0)
			for(int i = 0; i > delta; --i)
				_selected = _selected->prev;
		select(_selected);	//To update _value, render etc...
	}
	ct_widget_t::change(delta);
}


void ct_list_t::render()
{
	char buf[128];
	const char *ic;
	if(_selected)
		ic = _selected->caption();
	else
		ic = dummy_item.caption();
	ct_widget_t::render();
	snprintf(buf, sizeof(buf), "%s: %s", caption(), ic);
	//center(0, buf);
	//center_token(width()*3/5, 0, buf, ':');
	render_text_aligned(buf);
}



/*----------------------------------------------------------
	ct_spin_t::
----------------------------------------------------------*/

ct_spin_t::ct_spin_t(const char *cap, int _min, int _max,
		const char *__unit) : ct_label_t(cap)
{
	interactive = 1;
	min = _min;
	max = _max;
	_token = ':';
	xo = 0.55;
	if(__unit)
		unit(__unit);
	else
		unit("");
}


void ct_spin_t::unit(const char *txt)
{
	strncpy(_unit, txt, sizeof(_unit));
	_unit[sizeof(_unit)-1] = 0;
	if(!transparent)
		render();
}


void ct_spin_t::value(int val)
{
	val -= min;
	while(val < 0)
		val += max - min + 1;
	val %= max - min + 1;
	val += min;
	ct_label_t::value(val);
}


void ct_spin_t::change(int delta)
{
	value(_value + delta);
}


void ct_spin_t::render()
{
	char buf[128];
	ct_widget_t::render();
	snprintf(buf, sizeof(buf), "%s: %d %s", caption(), _value, unit());
	//center(0, buf);
	//center_token(width()*3/5, 0, buf, ':');
	render_text_aligned(buf);
}



/*----------------------------------------------------------
	ct_button_t::
----------------------------------------------------------*/

ct_button_t::ct_button_t(const char *cap) : ct_label_t(cap)
{
	interactive = 1;
}


void ct_button_t::render()
{
	ct_label_t::render();
}


void ct_button_t::change(int delta)
{
	ct_label_t::change(delta);
	if(!transparent)
		render();
}



/*----------------------------------------------------------
	ct_form_t::
----------------------------------------------------------*/

ct_form_t::ct_form_t()
{
	widgets = NULL;
	_selected = NULL;
}


ct_form_t::~ct_form_t()
{
	if(!widgets)
		return;

	/*
	 * Break the circular list first, or we'll
	 * blow up the memory manager...! :-)
	 */
	widgets->prev->next = NULL;

	while(widgets)
	{
		ct_widget_t *i = widgets;
		widgets = i->next;
		delete i;
	}
}


void ct_form_t::add(ct_widget_t *widget)
{
	if(widgets)
	{
		widget->next = widgets;
		widget->prev = widgets->prev;
		widgets->prev->next = widget;
		widgets->prev = widget;
		widget->_widget_index = widget->prev->_widget_index + 1;
		if(!_selected->interactive)
			select(widget);
	}
	else
	{
		widget->_widget_index = 0;
		widget->next = widget->prev = widget;
		widgets = widget;
		select(widget);
	}
	render_nontransparent();
}


void ct_form_t::clean()
{
	if(!widgets)
		return;

	/*
	 * Break the circular list first, or we'll
	 * blow up the memory manager...! :-)
	 */
	widgets->prev->next = NULL;

	while(widgets)
	{
		ct_widget_t *w = widgets;
		widgets = w->next;
		delete w;
	}

	widgets = NULL;
	_selected = NULL;
}


void ct_form_t::select(ct_widget_t *widget)
{
	if(_selected)
		_selected->highlight(0);
	_selected = widget;
	if(_selected)
	{
		_selected->highlight(1);
		if(!_selected->transparent)
			_selected->render();
	}
}


void ct_form_t::select(int ind)
{
	if(!widgets)
		return;

	if(!widgets)
		return;

	ct_widget_t *sel = widgets;
	for(int i = 0; i < ind; ++i)
		sel = sel->next;

	int lim = 10000;
	while(!sel->interactive)
	{
		if(!lim--)
			break;
		sel = sel->next;
	}
	select(sel);
}


void ct_form_t::next()
{
	if(!widgets)
		return;

	if(!_selected)
	{
		select(0);
		return;
	}

	ct_widget_t *sel = _selected->next;

	int lim = 1000;
	while(!sel->interactive)
	{
		if(!lim--)
			break;
		sel = sel->next;
	}
	select(sel);
}


void ct_form_t::prev()
{
	if(!widgets)
		return;

	if(!_selected)
	{
		select(0);
		return;
	}

	ct_widget_t *sel = _selected->prev;

	int lim = 1000;
	while(!sel->interactive)
	{
		if(!lim--)
			break;
		sel = sel->prev;
	}
	select(sel);
}


ct_widget_t *ct_form_t::selected()
{
	return _selected;
}


int ct_form_t::selected_index()
{
	if(_selected)
		return _selected->_widget_index;
	else
		return -1;
}


void ct_form_t::change(int delta)
{
	if(_selected)
		_selected->change(delta);
}


void ct_form_t::render()
{
	ct_widget_t *w = widgets;
	while(w)
	{
		w->render();
		w = w->next;
		if(w == widgets)
			break;	//Done!
	}
}


void ct_form_t::render_nontransparent()
{
	ct_widget_t *w = widgets;
	while(w)
	{
		if(!w->transparent)
			w->render();
		w = w->next;
		if(w == widgets)
			break;	//Done!
	}
}
