/*
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright (C) 2001-2003 David Olofson
 * Copyright (C) 2002 Jeremy Sheeley
 * Copyright (C) 2005-2007 David Olofson
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

#include "glSDL.h"
#include <math.h>
#ifndef M_PI
# define M_PI 3.14159265358979323846	/* pi */
#endif

#include "kobolog.h"
#include "config.h"
#include "states.h"
#include "kobo.h"
#include "screen.h"
#include "manage.h"
#include "options.h"
#include "sound.h"
#include "radar.h"
#include "random.h"


gamestatemanager_t gsm;
int run_intro = 0;
int last_level = -1;


kobo_basestate_t::kobo_basestate_t()
{
	name = "<unnamed>";
}


void kobo_basestate_t::frame()
{
}


void kobo_basestate_t::pre_render()
{
	screen.render_background(wmain);
}


void kobo_basestate_t::post_render()
{
	screen.render_fx(wmain);
	DBG(if(prefs->cmd_debug)
	{
		wmain->font(B_NORMAL_FONT);
		wmain->string(30, 5, name);
	})
}


/*----------------------------------------------------------
	st_introbase
----------------------------------------------------------*/

st_introbase_t::st_introbase_t()
{
	name = "intro";
	inext = NULL;
	duration = 0;
	timer = 0;
}


void st_introbase_t::enter()
{
	if(!run_intro)
	{
		manage.init_resources_title();
		if(prefs->use_music)
			sound.ui_music(SOUND_TITLE);
		run_intro = 1;
	}
	start_time = (int)SDL_GetTicks() + INTRO_BLANK_TIME;
	timer = 0;
}


void st_introbase_t::reenter()
{
	if(!run_intro)
	{
		manage.init_resources_title();
		if(prefs->use_music)
			sound.ui_music(SOUND_TITLE);
		run_intro = 1;
	}
	gsm.change(&st_intro_title);
}


void st_introbase_t::press(int button)
{
	switch (button)
	{
	  case BTN_EXIT:
	  case BTN_CLOSE:
		gsm.push(&st_main_menu);
		break;
	  case BTN_START:
	  case BTN_FIRE:
	  case BTN_SELECT:
		run_intro = 0;
		gsm.push(&st_main_menu);
	        if(scorefile.numProfiles <= 0)
			gsm.push(&st_new_player);
		break;
	  case BTN_BACK:
	  case BTN_UP:
	  case BTN_LEFT:
		gsm.change(&st_intro_title);
		break;
	  case BTN_DOWN:
	  case BTN_RIGHT:
		if(inext)
			gsm.change(inext);
		break;
#ifdef PROFILE_AUDIO
	  case BTN_F9:
		run_intro = 0;
		gsm.push(&st_profile_audio);
		break;
#endif
	  default:
		break;
	}
}


void st_introbase_t::frame()
{
	manage.run_intro();
	if((timer > duration) && inext)
		gsm.change(inext);
}


void st_introbase_t::pre_render()
{
	kobo_basestate_t::pre_render();
	timer = (int)SDL_GetTicks() - start_time;
}


void st_introbase_t::post_render()
{
	kobo_basestate_t::post_render();
	screen.scroller();
}



/*----------------------------------------------------------
	st_intro_title
----------------------------------------------------------*/

st_intro_title_t::st_intro_title_t()
{
	name = "intro_title";
}

void st_intro_title_t::enter()
{
	st_introbase_t::enter();
	if(!duration)
		duration = INTRO_TITLE_TIME + 2000 - INTRO_BLANK_TIME;
	if(!inext)
		inext = &st_intro_instructions;
	screen.set_highlight(0, 0);
}

void st_intro_title_t::post_render()
{
	if(exit_game)
		return;
	st_introbase_t::post_render();
	if((timer >= 0) && (timer < duration))
	{
		float nt = (float)timer / duration;
		float snt = 1.0f - sin(nt * M_PI);
		snt = 1.0f - snt * snt * snt;
		screen.title(timer, snt, mode);
	}
}

st_intro_title_t st_intro_title;


/*----------------------------------------------------------
	st_intro_instructions
----------------------------------------------------------*/

st_intro_instructions_t::st_intro_instructions_t()
{
	name = "intro_instructions";
}

void st_intro_instructions_t::enter()
{
	st_introbase_t::enter();
	duration = INTRO_INSTRUCTIONS_TIME;
	inext = &st_intro_title;
	st_intro_title.inext = &st_intro_highscores;
	st_intro_title.duration = INTRO_TITLE2_TIME - INTRO_BLANK_TIME;
	st_intro_title.mode = pubrand.get(1) + 1;
}

void st_intro_instructions_t::post_render()
{
	if(exit_game)
		return;
	st_introbase_t::post_render();
	if((timer >= 0) && (timer < duration))
		screen.help(timer);
	else
		screen.set_highlight(0, 0);
}

st_intro_instructions_t st_intro_instructions;


/*----------------------------------------------------------
	st_intro_highshores
----------------------------------------------------------*/

st_intro_highscores_t::st_intro_highscores_t()
{
	name = "intro_highscores";
}

void st_intro_highscores_t::enter()
{
	scorefile.gather_high_scores(1);
	screen.init_highscores();
	st_introbase_t::enter();
	duration = INTRO_HIGHSCORE_TIME;
	inext = &st_intro_title;
	st_intro_title.inext = &st_intro_credits;
	st_intro_title.duration = INTRO_TITLE2_TIME - INTRO_BLANK_TIME;
	st_intro_title.mode = pubrand.get(1) + 1;
	screen.set_highlight(0, 0);
}

void st_intro_highscores_t::post_render()
{
	if(exit_game)
		return;
	st_introbase_t::post_render();
	if((timer >= 0) && (timer < duration))
	{
		float nt = (float)timer / duration;
		float snt = 1.0f - sin(nt * M_PI);
		snt = 1.0f - snt * snt * snt;
		screen.highscores(timer, snt);
	}
}

st_intro_highscores_t st_intro_highscores;


/*----------------------------------------------------------
	st_intro_credits
----------------------------------------------------------*/

st_intro_credits_t::st_intro_credits_t()
{
	name = "intro_credits";
}

void st_intro_credits_t::enter()
{
	st_introbase_t::enter();
	duration = INTRO_CREDITS_TIME;
	inext = &st_intro_title;
	st_intro_title.inext = &st_intro_instructions;
	st_intro_title.duration = INTRO_TITLE_TIME - INTRO_BLANK_TIME;
	st_intro_title.mode = 0;
}

void st_intro_credits_t::post_render()
{
	if(exit_game)
		return;
	st_introbase_t::post_render();
	if((timer >= 0) && (timer < duration))
		screen.credits(timer);
	else
		screen.set_highlight(0, 0);
}

st_intro_credits_t st_intro_credits;


/*----------------------------------------------------------
	st_game
----------------------------------------------------------*/

st_game_t::st_game_t()
{
	name = "game";
}


void st_game_t::enter()
{
	audio_channel_stop(0, -1);	//Stop any music
	run_intro = 0;
	manage.game_start();
	if(exit_game || manage.game_stopped())
	{
		st_error.message("Could not start game!",
				"Please, check your installation.");
		gsm.change(&st_error);
	}
	if(prefs->mousecapture)
		if(SDL_WM_GrabInput(SDL_GRAB_QUERY) != SDL_GRAB_ON)
			SDL_WM_GrabInput(SDL_GRAB_ON);
}


void st_game_t::leave()
{
	if(SDL_WM_GrabInput(SDL_GRAB_QUERY) == SDL_GRAB_ON)
		SDL_WM_GrabInput(SDL_GRAB_OFF);
	st_intro_title.inext = &st_intro_instructions;
	st_intro_title.duration = INTRO_TITLE_TIME + 2000;
	st_intro_title.mode = 0;
}


void st_game_t::yield()
{
	if(SDL_WM_GrabInput(SDL_GRAB_QUERY) == SDL_GRAB_ON)
		SDL_WM_GrabInput(SDL_GRAB_OFF);
}


void st_game_t::reenter()
{
	if(prefs->mousecapture)
		if(SDL_WM_GrabInput(SDL_GRAB_QUERY) != SDL_GRAB_ON)
			SDL_WM_GrabInput(SDL_GRAB_ON);
}


void st_game_t::press(int button)
{
	switch (button)
	{
	  case BTN_EXIT:
		gsm.push(&st_main_menu);
		break;
	  case BTN_CLOSE:
		gsm.push(&st_ask_exit);
		break;
	  case BTN_SELECT:
	  case BTN_START:
	  case BTN_PAUSE:
		gsm.push(&st_pause_game);
		break;
	}
}


void st_game_t::frame()
{
	if(manage.get_ready())
	{
		gsm.push(&st_get_ready);
		return;
	}
	manage.run_game();
	last_level = manage.scene();
	if(manage.game_over())
		gsm.change(&st_game_over);
	else if(exit_game || manage.game_stopped())
		pop();
}


void st_game_t::post_render()
{
	kobo_basestate_t::post_render();
	wradar->frame();
}


st_game_t st_game;



/*----------------------------------------------------------
	st_pause_game
----------------------------------------------------------*/

st_pause_game_t::st_pause_game_t()
{
	name = "pause_game";
}


void st_pause_game_t::enter()
{
	sound.ui_pause();
}


void st_pause_game_t::press(int button)
{
	switch (button)
	{
	  case BTN_EXIT:
		gsm.change(&st_main_menu);
		break;
	  default:
		sound.ui_play();
		pop();
		break;
	}
}


void st_pause_game_t::frame()
{
	manage.run_pause();
}


void st_pause_game_t::post_render()
{
	kobo_basestate_t::post_render();

	float ft = SDL_GetTicks() * 0.001;
	wmain->font(B_BIG_FONT);
	int y = PIXEL2CS(75) + (int)floor(PIXEL2CS(15)*sin(ft * 6));
	wmain->center_fxp(y, "PAUSED");

	wradar->frame();
}

st_pause_game_t st_pause_game;



/*----------------------------------------------------------
	st_get_ready
----------------------------------------------------------*/

st_get_ready_t::st_get_ready_t()
{
	name = "get_ready";
}


void st_get_ready_t::enter()
{
	manage.update();
	sound.ui_ready();
	start_time = (int)SDL_GetTicks();
	frame_time = 0;
	countdown = prefs->countdown;
}


void st_get_ready_t::press(int button)
{
	if(frame_time < 500)
		return;

	switch (button)
	{
	  case BTN_EXIT:
	  case BTN_NO:
		gsm.change(&st_main_menu);
		break;
	  case BTN_LEFT:
	  case BTN_RIGHT:
	  case BTN_UP:
	  case BTN_DOWN:
	  case BTN_UL:
	  case BTN_UR:
	  case BTN_DL:
	  case BTN_DR:
	  case BTN_FIRE:
	  case BTN_YES:
		sound.ui_play();
		pop();
		break;
	  case BTN_SELECT:
	  case BTN_START:
	  case BTN_PAUSE:
		gsm.change(&st_pause_game);
		break;
	}
}


void st_get_ready_t::frame()
{
	manage.run_pause();

	if(exit_game || manage.game_stopped())
	{
		pop();
		return;
	}

	frame_time = (int)SDL_GetTicks() - start_time;
	if(0 == prefs->countdown)
	{
		if(frame_time > 700)
		{
			sound.ui_play();
			pop();
		}
	}
	else if(prefs->countdown <= 9)
	{
		int prevcount = countdown;
		countdown = prefs->countdown - frame_time/1000;
		if(prevcount != countdown)
			sound.ui_countdown(countdown);

		if(countdown < 1)
		{
			sound.ui_play();
			pop();
		}
	}
}


void st_get_ready_t::post_render()
{
	kobo_basestate_t::post_render();

	float ft = SDL_GetTicks() * 0.001;
	char counter[2] = "0";
	wmain->font(B_BIG_FONT);
	int y = PIXEL2CS(70) + (int)floor(PIXEL2CS(15)*sin(ft * 6));
	wmain->center_fxp(y, "GET READY!");

	float z = (float)((int)SDL_GetTicks() - start_time);
	if(10 == prefs->countdown)
		z = -1;
	else if(prefs->countdown)
		z = prefs->countdown - z * 0.001;
	else
		z = 1.0 - z / 700.0;
	if((z > 0.0) && (z < 1.0))
	{
		float x = wmain->width() / 2;
		wmain->foreground(wmain->map_rgb(
				255 - (int)(z * 255.0),
				(int)(z * 255.0),
				0));
		wmain->fillrect_fxp(PIXEL2CS((int)(x - z * 50.0)),
				y + PIXEL2CS(76),
				PIXEL2CS((int)(z * 100.0)),
				PIXEL2CS(10));
	}

	wmain->font(B_MEDIUM_FONT);
	if(10 == prefs->countdown)
		wmain->center_fxp(y + PIXEL2CS(70), "(Press FIRE)");
	else if(prefs->countdown)
	{
		wmain->center_fxp(y + PIXEL2CS(100), "(Press FIRE)");
		counter[0] = countdown + '0';
		wmain->font(B_COUNTER_FONT);
		wmain->center_fxp(y + PIXEL2CS(60), counter);
	}

	wradar->frame();
}

st_get_ready_t st_get_ready;


/*----------------------------------------------------------
	st_game_over
----------------------------------------------------------*/

st_game_over_t::st_game_over_t()
{
	name = "game_over";
}


void st_game_over_t::enter()
{
	sound.ui_gameover();
	manage.update();
	start_time = (int)SDL_GetTicks();
}


void st_game_over_t::press(int button)
{
	if(frame_time < 500)
		return;

	switch (button)
	{
	  case BTN_EXIT:
	  case BTN_NO:
	  case BTN_LEFT:
	  case BTN_RIGHT:
	  case BTN_UP:
	  case BTN_DOWN:
	  case BTN_UL:
	  case BTN_UR:
	  case BTN_DL:
	  case BTN_DR:
	  case BTN_FIRE:
	  case BTN_START:
	  case BTN_SELECT:
	  case BTN_YES:
		sound.ui_ok();
		pop();
		break;
	}
}


void st_game_over_t::frame()
{
	manage.run_game();

	frame_time = (int)SDL_GetTicks() - start_time;
	if(frame_time > 5000)
		pop();
}


void st_game_over_t::post_render()
{
	kobo_basestate_t::post_render();

	float ft = SDL_GetTicks() * 0.001;
	wmain->font(B_BIG_FONT);
	int y = PIXEL2CS(100) + (int)floor(PIXEL2CS(15)*sin(ft * 6));
	wmain->center_fxp(y, "GAME OVER");

	wradar->frame();
}

st_game_over_t st_game_over;



/*----------------------------------------------------------
	Menu Base
----------------------------------------------------------*/

/*
 * menu_base_t
 */
void menu_base_t::open()
{
	init(gengine);
	place(wmain->x(), wmain->y(), wmain->width(), wmain->height());
	font(B_NORMAL_FONT);
	foreground(wmain->map_rgb(0xffffff));
	background(wmain->map_rgb(0x000000));
	build_all();
}

void menu_base_t::close()
{
	clean();
}


/*
 * st_menu_base_t
 */

st_menu_base_t::st_menu_base_t()
{
	name = "(menu_base derivate)";
	sounds = 1;
	form = NULL;
}


st_menu_base_t::~st_menu_base_t()
{
	delete form;
}


void st_menu_base_t::enter()
{
	form = open();
	if(manage.game_stopped())
		run_intro = 1;
	if(sounds)
		sound.ui_ok();
}

// Because we may get back here after changing the configuration!
void st_menu_base_t::reenter()
{
	if(global_status & OS_RESTART_VIDEO)
		pop();
}

void st_menu_base_t::leave()
{
	screen.set_highlight(0, 0);
	close();
	delete form;
	form = NULL;
	if(manage.game_stopped())
	{
		manage.init_resources_title();
		st_intro_title.inext = &st_intro_instructions;
		st_intro_title.duration = INTRO_TITLE_TIME + 2000;
		st_intro_title.mode = 0;
	}
}

void st_menu_base_t::frame()
{
	if(manage.game_stopped())
		manage.run_intro();
	//(Game is paused when a menu is active.)
}

void st_menu_base_t::post_render()
{
	kobo_basestate_t::post_render();
	if(form)
		form->render();
	if(!manage.game_stopped())
		wradar->frame();
}

int st_menu_base_t::translate(int tag, int button)
{
	switch(button)
	{
	  case BTN_INC:
	  case BTN_RIGHT:
	  case BTN_DEC:
	  case BTN_LEFT:
		return -1;
	  default:
		return tag;
	}
}

void st_menu_base_t::press(int button)
{
	int selection;
	if(!form)
		return;

	do_default_action = 1;

	// Translate
	switch(button)
	{
	  case BTN_EXIT:
	  case BTN_CLOSE:
		selection = 0;
		break;
	  case BTN_UP:
	  case BTN_DOWN:
		selection = -1;
		break;
	  case BTN_INC:
	  case BTN_RIGHT:
	  case BTN_DEC:
	  case BTN_LEFT:
	  case BTN_FIRE:
	  case BTN_START:
	  case BTN_SELECT:
		if(form->selected())
			selection = translate(form->selected()->tag,
					button);
		else
			selection = -2;
		break;
	  default:
		selection = -1;
		break;
	}

	// Default action
	if(do_default_action)
		switch(button)
		{
		  case BTN_EXIT:
			escape();
			break;
		  case BTN_INC:
		  case BTN_RIGHT:
			form->change(1);
			break;
		  case BTN_DEC:
		  case BTN_LEFT:
			form->change(-1);
			break;
		  case BTN_FIRE:
		  case BTN_START:
		  case BTN_SELECT:
			form->change(0);
			break;
		  case BTN_UP:
			form->prev();
			break;
		  case BTN_DOWN:
			form->next();
			break;
#ifdef PROFILE_AUDIO
		  case BTN_F9:
			gsm.push(&st_profile_audio);
			break;
#endif
		}

	switch(selection)
	{
	  case -1:
		break;
	  case 0:
		if(sounds)
			sound.ui_cancel();
		select(0);
		pop();
		break;
	  default:
		select(selection);
		break;
	}
}



/*----------------------------------------------------------
	st_new_player
----------------------------------------------------------*/

void new_player_t::open()
{
	init(gengine);
	place(wmain->x(), wmain->y(), wmain->width(), wmain->height());
	font(B_NORMAL_FONT);
	foreground(wmain->map_rgb(255, 255, 255));
	background(wmain->map_rgb(0, 0, 0));
	memset(name, 0, sizeof(name));
	name[0] = 'A';
	currentIndex = 0;
	editing = 1;
	build_all();
	SDL_EnableUNICODE(1);
}

void new_player_t::close()
{
	SDL_EnableUNICODE(0);
	clean();
}

void new_player_t::change(int delta)
{
	kobo_form_t::change(delta);

	if(!selected())
		return;

	selection = selected()->tag;
}

void new_player_t::build()
{
	medium();
	space(2);
	label("Use arrows, joystick or keyboard");
	label("to enter name");

	big();
	space();
	button(name, 1);
	space();

	button("Ok", MENU_TAG_OK);
	button("Cancel", MENU_TAG_CANCEL);
}

void new_player_t::rebuild()
{
	int sel = selected_index();
	build_all();
	select(sel);
}


st_new_player_t::st_new_player_t()
{
	name = "new_player";
}

void st_new_player_t::frame()
{
	manage.run_intro();
}

void st_new_player_t::enter()
{
	menu.open();
	run_intro = 0;
	sound.ui_ok();
}

void st_new_player_t::leave()
{
	menu.close();
}

void st_new_player_t::post_render()
{
	kobo_basestate_t::post_render();
	menu.render();
}

void st_new_player_t::press(int button)
{
	if(menu.editing)
	{
		switch(button)
		{
		  case BTN_EXIT:
			sound.ui_ok();
			menu.editing = 0;
			menu.next();	// Select the CANCEL option.
			menu.next();
			break;

		  case BTN_FIRE:
			if(!prefs->use_joystick)
				break;
		  case BTN_START:
		  case BTN_SELECT:
			sound.ui_ok();
			menu.editing = 0;
			menu.next();	// Select the OK option.
			break;

		  case BTN_UP:
		  case BTN_INC:
			if(!menu.name[menu.currentIndex])
				menu.name[menu.currentIndex] = 'A';
			else if(menu.name[menu.currentIndex] == 'Z')
				menu.name[menu.currentIndex] = 'a';
			else if(menu.name[menu.currentIndex] == 'z')
				menu.name[menu.currentIndex] = 'A';
			else
				menu.name[menu.currentIndex]++;
			sound.ui_tick();
			break;

		  case BTN_DEC:
		  case BTN_DOWN:
			if(!menu.name[menu.currentIndex])
				menu.name[menu.currentIndex] = 'A';
			else if(menu.name[menu.currentIndex] == 'A')
				menu.name[menu.currentIndex] = 'z';
			else if(menu.name[menu.currentIndex] == 'a')
				menu.name[menu.currentIndex] = 'Z';
			else
				menu.name[menu.currentIndex]--;
			sound.ui_tick();
			break;

		  case BTN_RIGHT:
			if(menu.currentIndex < sizeof(menu.name) - 2)
			{
				menu.currentIndex++;
				sound.ui_tick();
			}
			else
			{
				sound.ui_error();
				break;
			}
			if(menu.name[menu.currentIndex] == '\0')
				menu.name[menu.currentIndex] = 'A';
			break;

		  case BTN_LEFT:
		  case BTN_BACK:
			if(menu.currentIndex > 0)
			{
				menu.name[menu.currentIndex] = '\0';
				menu.currentIndex--;
				sound.ui_tick();
			}
			else
				sound.ui_error();
			break;

		  default:
			if((unicode >= 'a') && (unicode <= 'z') ||
				(unicode >= 'A') && (unicode <= 'Z'))
			{
				menu.name[menu.currentIndex] = (char)unicode;
				if(menu.currentIndex < sizeof(menu.name) - 2)
				{
					menu.currentIndex++;
					sound.ui_tick();
				}
				else
					sound.ui_error();
			}
			else
				sound.ui_error();
			break;
		}
		menu.rebuild();
	}
	else
	{
		menu.selection = -1;

		switch(button)
		{
		  case BTN_EXIT:
			menu.selection = MENU_TAG_CANCEL;
			break;

		  case BTN_CLOSE:
			menu.selection = MENU_TAG_OK;
			break;

		  case BTN_FIRE:
		  case BTN_START:
		  case BTN_SELECT:
			menu.change(0);
			break;

		  case BTN_INC:
		  case BTN_UP:
			menu.prev();
			break;

		  case BTN_DEC:
		  case BTN_DOWN:
			menu.next();
			break;
		}

		switch(menu.selection)
		{
		  case 1:
			if(button == BTN_START
					|| button == BTN_SELECT
					|| button == BTN_FIRE)
			{
				sound.ui_ok();
				menu.editing = 1;
			}
			break;

		  case MENU_TAG_OK:
			switch(scorefile.addPlayer(menu.name))
			{
			  case 0:
				sound.ui_ok();
				prefs->last_profile = scorefile.current_profile();
				prefs->changed = 1;
				pop();
				break;
			  case -1:
				sound.ui_error();
				st_error.message("Cannot create Player Profile!",
						"Too many profiles!");
				gsm.change(&st_error);
				break;
			  case -2:
			  case -3:
				prefs->last_profile = scorefile.current_profile();
				sound.ui_error();
				st_error.message("Cannot save Player Profile!",
						"Please, check your installation.");
				gsm.change(&st_error);
				break;
			  default:
				sound.ui_error();
				st_error.message("Cannot create Player Profile!",
						"Bug or internal error.");
				gsm.change(&st_error);
				break;
			}
			break;

		  case MENU_TAG_CANCEL:
			sound.ui_cancel();
			strcpy(menu.name, "A");
			pop();
			break;
		}
	}
}

st_new_player_t st_new_player;


/*----------------------------------------------------------
	st_error
----------------------------------------------------------*/

void st_error_t::message(const char *error, const char *hint)
{
	msg[0] = error;
	msg[1] = hint;
}


st_error_t::st_error_t()
{
	name = "error";
	msg[0] = "No error!";
	msg[1] = "(Why am I here...?)";
}


void st_error_t::enter()
{
	sound.ui_error();
	manage.update();
	start_time = (int)SDL_GetTicks();
}


void st_error_t::press(int button)
{
	if(frame_time < 500)
		return;

	switch (button)
	{
	  case BTN_EXIT:
	  case BTN_NO:
	  case BTN_LEFT:
	  case BTN_RIGHT:
	  case BTN_UP:
	  case BTN_DOWN:
	  case BTN_UL:
	  case BTN_UR:
	  case BTN_DL:
	  case BTN_DR:
	  case BTN_FIRE:
	  case BTN_START:
	  case BTN_SELECT:
	  case BTN_YES:
		sound.ui_ok();
		pop();
		break;
	}
}


void st_error_t::frame()
{
	manage.run_intro();

	frame_time = (int)SDL_GetTicks() - start_time;

	if(frame_time % 1000 < 500)
		sound.ui_error();
}


void st_error_t::post_render()
{
	kobo_basestate_t::post_render();

	wmain->font(B_MEDIUM_FONT);
	wmain->center(95, msg[0]);

	wmain->font(B_NORMAL_FONT);
	wmain->center(123, msg[1]);

	if(frame_time % 1000 < 500)
	{
		int w = wmain->width();
		int h = wmain->height();
		wmain->foreground(wmain->map_rgb(0xff0000));
		wmain->rectangle(20, 80, w - 40, h - 160);
		wmain->rectangle(22, 82, w - 44, h - 164);
		wmain->rectangle(24, 84, w - 48, h - 168);
	}
}

st_error_t st_error;



/*----------------------------------------------------------
	st_main_menu
----------------------------------------------------------*/

void main_menu_t::buildStartLevel(int profNum)
{
	char buf[50];
	int MaxStartLevel = scorefile.last_scene(profNum);
	start_level = manage.scene();
	if(start_level > MaxStartLevel)
		start_level = MaxStartLevel;
	list("Start at Stage", &start_level, 5);
	for(int i = 0; i <= MaxStartLevel; ++i)
	{
		snprintf(buf, sizeof(buf), "%d", i + 1);
		item(buf, i);
	}
}

void main_menu_t::build()
{
	if(manage.game_stopped())
	{
		prefs->last_profile = scorefile.current_profile();
		if(last_level < 0)
			manage.select_scene(scorefile.last_scene());
		else
			manage.select_scene(last_level);
	}

	halign = ALIGN_CENTER;
	xoffs = 0.5;
	big();
	if(manage.game_stopped())
	{
		space();
	        if(scorefile.numProfiles > 0)
		{
			button("Start Game!", 1);
			space();
			list("Player", &prefs->last_profile, 4);
			for(int i = 0; i < scorefile.numProfiles; ++i)
				item(scorefile.name(i), i);
			small();
			buildStartLevel(prefs->last_profile);
			big();
		}
		else
			space(2);
		button("New Player...", 3);
	}
	else
	{
		space(2);
		button("Return to Game", 0);
	}
	space();
	button("Options", 2);
	space();
	if(manage.game_stopped())
		button("Return to Intro", 0);
	else
		button("Abort Current Game", 101);
	button("Quit Kobo Deluxe", MENU_TAG_CANCEL);
}

void main_menu_t::rebuild()
{
	int sel = selected_index();
	build_all();
	select(sel);
}


kobo_form_t *st_main_menu_t::open()
{
	if(manage.game_stopped())
	{
		// Moved here, as we want to do it as late as
		// possible, but *not* as a result of rebuild().
		if(prefs->last_profile >= scorefile.numProfiles)
		{
			prefs->last_profile = 0;
			prefs->changed = 1;
		}
		scorefile.select_profile(prefs->last_profile);
	}

	menu = new main_menu_t;
	menu->open();
	return menu;
}


void st_main_menu_t::reenter()
{
	menu->rebuild();
	st_menu_base_t::reenter();
}


// Custom translator to map inc/dec on certain widgets
int st_main_menu_t::translate(int tag, int button)
{
	switch(tag)
	{
	  case 4:
		// The default translate() filters out the
		// inc/dec events, and performs the default
		// action for fire/start/select...
		switch(button)
		{
		  case BTN_FIRE:
		  case BTN_START:
		  case BTN_SELECT:
			do_default_action = 0;
			return tag + 10;
		  default:
			return tag;
		}
	  case 5:
		return tag;
	  default:
		return st_menu_base_t::translate(tag, button);
	}
}

void st_main_menu_t::select(int tag)
{
	switch(tag)
	{
	  case 1:
		gsm.change(&st_skill_menu);
		break;
	  case 2:
		gsm.push(&st_options_main);
		break;
	  case 3:
		gsm.push(&st_new_player);
		break;
	  case 4:	// Player: Inc/Dec
		sound.ui_tick();
		prefs->changed = 1;
		scorefile.select_profile(prefs->last_profile);
		menu->rebuild();
		break;
	  case 14:	// Player: Select
		// Edit player profile!
//		menu->rebuild();
		break;
	  case 5:	// Start level: Inc/Dec
		sound.ui_tick();
		manage.select_scene(menu->start_level);
		break;
	  case MENU_TAG_CANCEL:
		gsm.change(&st_ask_exit);
		break;
	  case 101:
		gsm.change(&st_ask_abort_game);
		break;
	  case 0:
		if(!manage.game_stopped())
			gsm.change(&st_pause_game);
		break;
	}
}

st_main_menu_t st_main_menu;


/*----------------------------------------------------------
	st_skill_menu
----------------------------------------------------------*/

void skill_menu_t::build()
{
	halign = ALIGN_CENTER;
	xoffs = 0.5;
	medium();
	label("Select Skill Level");
	space(1);

	big();
	button("Classic", SKILL_CLASSIC + 10);
	small();
	space();
	big();
	button("Newbie", SKILL_NEWBIE + 10);
	button("Gamer", SKILL_GAMER + 10);
	button("Elite", SKILL_ELITE + 10);
	button("God", SKILL_GOD + 10);

	space();
	small();
	switch(skill)
	{
	  case SKILL_CLASSIC:
		label("\"I want the original XKobo, dammit!\"");
		break;
	  case SKILL_NEWBIE:
		label("\"Damn, this is hard...!\"");
		break;
	  case SKILL_GAMER:
		label("\"Classic is too retro for me!\"");
		break;
	  case SKILL_ELITE:
		label("\"Bah! Gimme some resistance here.\"");
		break;
	  case SKILL_GOD:
		label("\"The dark is afraid of me.\"");
		break;
	}
}

void skill_menu_t::rebuild()
{
	int sel = selected_index();
	build_all();
	select(sel);
}


kobo_form_t *st_skill_menu_t::open()
{
	menu = new skill_menu_t;
	menu->set_skill(scorefile.profile()->skill);
	menu->open();
	switch(scorefile.profile()->skill)
	{
	  case SKILL_CLASSIC:
		menu->select(1);
		break;
	  case SKILL_NEWBIE:
		menu->select(2);
		break;
	  case SKILL_GAMER:
		menu->select(3);
		break;
	  case SKILL_ELITE:
		menu->select(4);
		break;
	  case SKILL_GOD:
		menu->select(5);
		break;
	}
	return menu;
}


void st_skill_menu_t::press(int button)
{
	st_menu_base_t::press(button);
	switch (button)
	{
	  case BTN_UP:
	  case BTN_DOWN:
		menu->set_skill(menu->selected()->tag - 10);
		menu->rebuild();
		break;
	}
}

void st_skill_menu_t::select(int tag)
{
	if((tag >= 10) && (tag <= 20))
	{
		scorefile.profile()->skill = menu->selected()->tag - 10;
		gsm.change(&st_game);
	}
}

st_skill_menu_t st_skill_menu;


/*----------------------------------------------------------
	st_options_main
----------------------------------------------------------*/

void options_main_t::build()
{
	medium();
	label("Options");
	space();

	big();
	button("Game", 4);
	button("Controls", 3);
	button("Video", 1);
	button("Graphics", 6);
	button("Audio", 2);
	button("System", 5);
	space();

	button("DONE!", 0);
}

kobo_form_t *st_options_main_t::open()
{
	options_main_t *m = new options_main_t;
	m->open();
	return m;
}

void st_options_main_t::select(int tag)
{
	switch(tag)
	{
	  case 1:
		gsm.push(&st_options_video);
		break;
	  case 2:
		gsm.push(&st_options_audio);
		break;
	  case 3:
		gsm.push(&st_options_control);
		break;
	  case 4:
		gsm.push(&st_options_game);
		break;
	  case 5:
		gsm.push(&st_options_system);
		break;
	  case 6:
		gsm.push(&st_options_graphics);
		break;
	}
}

st_options_main_t st_options_main;


/*----------------------------------------------------------
	st_options_base
----------------------------------------------------------*/

kobo_form_t *st_options_base_t::open()
{
	sounds = 0;
	cfg_form = oopen();
	cfg_form->open(prefs);
	return cfg_form;
}

void st_options_base_t::close()
{
	cfg_form->close();
}

void st_options_base_t::enter()
{
	sound.ui_ok();
	st_menu_base_t::enter();
}

void st_options_base_t::select(int tag)
{
	if(cfg_form->status() & OS_CANCEL)
		cfg_form->undo();
	else if(cfg_form->status() & OS_CLOSE)
	{
		if(cfg_form->status() & (OS_RESTART | OS_RELOAD))
		{
			exit_game = 0;
			manage.freeze_abort();
		}
	}

	/* 
	 * Handle changes that require only an update...
	 */
	if(cfg_form->status() & OS_UPDATE_AUDIO)
		sound.prefschange();
	if(cfg_form->status() & OS_UPDATE_ENGINE)
	{
		gengine->timefilter(prefs->timefilter * 0.01f);
		gengine->interpolation(prefs->filter);
		gengine->vsync(prefs->vsync);
		gengine->pages(prefs->pages);
	}
	cfg_form->clearstatus(OS_UPDATE);

	if(cfg_form->status() & (OS_CANCEL | OS_CLOSE))
		pop();
}

void st_options_base_t::press(int button)
{
	st_menu_base_t::press(button);
	gengine->timefilter(prefs->timefilter * 0.01f);
	gengine->interpolation(prefs->filter);
}

void st_options_base_t::escape()
{
	sound.ui_cancel();
	cfg_form->undo();
}



/*----------------------------------------------------------
	Options...
----------------------------------------------------------*/
st_options_system_t st_options_system;
st_options_video_t st_options_video;
st_options_graphics_t st_options_graphics;
st_options_audio_t st_options_audio;
st_options_control_t st_options_control;
st_options_game_t st_options_game;



/*----------------------------------------------------------
	Requesters
----------------------------------------------------------*/
void yesno_menu_t::build()
{
	halign = ALIGN_CENTER;
	xoffs = 0.5;
	big();
	space(8);
	button("YES", MENU_TAG_OK);
	button("NO", MENU_TAG_CANCEL);
}

void yesno_menu_t::rebuild()
{
	int sel = selected_index();
	build_all();
	select(sel);
}


/*----------------------------------------------------------
	st_yesno_base_t
----------------------------------------------------------*/

kobo_form_t *st_yesno_base_t::open()
{
	menu = new yesno_menu_t;
	menu->open();
	menu->select(1);
	return menu;
}

void st_yesno_base_t::reenter()
{
	menu->rebuild();
	menu->select(1);
	st_menu_base_t::reenter();
}

void st_yesno_base_t::press(int button)
{
	switch (button)
	{
	  case BTN_NO:
		select(MENU_TAG_CANCEL);
		break;
	  case BTN_YES:
		select(MENU_TAG_OK);
		break;
	  default:
		st_menu_base_t::press(button);
	}
}

void st_yesno_base_t::frame()
{
	if(manage.game_stopped())
		manage.run_intro();
	else
		manage.run_pause();
}

void st_yesno_base_t::post_render()
{
	st_menu_base_t::post_render();

	float ft = SDL_GetTicks() * 0.001;
	wmain->font(B_BIG_FONT);
	int y = PIXEL2CS(100) + (int)floor(PIXEL2CS(15)*sin(ft * 6));
	wmain->center_fxp(y, msg);
}


/*----------------------------------------------------------
	st_ask_exit
----------------------------------------------------------*/

st_ask_exit_t::st_ask_exit_t()
{
	name = "ask_exit";
	msg = "Quit Kobo Deluxe?";
}

void st_ask_exit_t::select(int tag)
{
	switch(tag)
	{
	  case MENU_TAG_OK:
		audio_channel_stop(0, -1);	//Stop any music
		sound.ui_ok();
		exit_game = 1;
		pop();
		break;
	  case MENU_TAG_CANCEL:
		sound.ui_cancel();
		if(manage.game_stopped())
			pop();
		else
			gsm.change(&st_pause_game);
		break;
	}
}

st_ask_exit_t st_ask_exit;



/*----------------------------------------------------------
	st_ask_abort_game
----------------------------------------------------------*/

st_ask_abort_game_t::st_ask_abort_game_t()
{
	name = "ask_abort_game";
	msg = "Abort Game?";
}

void st_ask_abort_game_t::select(int tag)
{
	switch(tag)
	{
	  case MENU_TAG_OK:
		sound.ui_ok();
		manage.abort();
		pop();
		break;
	  case MENU_TAG_CANCEL:
		gsm.change(&st_pause_game);
		break;
	}
}

st_ask_abort_game_t st_ask_abort_game;



/*----------------------------------------------------------
	Debug: Audio Engine Profiling
----------------------------------------------------------*/
#ifdef PROFILE_AUDIO

#include "sound.h"
#include "a_midicon.h"

st_profile_audio_t::st_profile_audio_t()
{
	name = "profile_audio";
	pan = 0;
	pitch = 60;
	shift = 0;
}


void st_profile_audio_t::enter()
{
	audio_group_control(SOUND_GROUP_SFX, ACC_PAN, pan);
	audio_group_control(SOUND_GROUP_SFX, ACC_PITCH, pitch << 16);
	screen.set_highlight(0, 0);
	screen.noise(0);
	sound.sfx_volume(1.0f);
}


void st_profile_audio_t::press(int button)
{
	switch (button)
	{
	  case BTN_EXIT:
		pop();
		break;
	  case BTN_LEFT:
		pan -= 8192;
		if(pan < -65536)
			pan = -65536;
		audio_group_control(SOUND_GROUP_SFX, ACC_PAN, pan);
		break;
	  case BTN_RIGHT:
		pan += 8192;
		if(pan > 65536)
			pan = 65536;
		audio_group_control(SOUND_GROUP_SFX, ACC_PAN, pan);
		break;
	  case BTN_UP:
		++pitch;
		if(pitch > 127)
			pitch = 127;
		audio_group_control(SOUND_GROUP_SFX, ACC_PITCH, pitch << 16);
		break;
	  case BTN_DOWN:
		--pitch;
		if(pitch < 0)
			pitch = 0;
		audio_group_control(SOUND_GROUP_SFX, ACC_PITCH, pitch << 16);
		break;
	  case BTN_NO:
	  case BTN_UL:
	  case BTN_UR:
	  case BTN_DL:
	  case BTN_DR:
		break;
	  case BTN_INC:
		shift += 8;
		if(shift > AUDIO_MAX_WAVES-8)
			shift = 0;
		break;
	  case BTN_DEC:
		shift -= 8;
		if(shift < 0)
			shift = AUDIO_MAX_WAVES-8;
		break;
	  case BTN_FIRE:
	  {
		audio_channel_stop(-1, -1);
		int startt = SDL_GetTicks();
		audio_wave_load(0, "sfx.agw", 0);
		log_printf(VLOG, "(Loading + processing time: %d ms)\n",
				SDL_GetTicks() - startt);
		break;
	  }
	  case BTN_START:
	  case BTN_SELECT:
		audio_channel_stop(-1, -1);
		break;
	  case BTN_YES:
		break;
	  case BTN_F1:
		sound.g_play0(0 + shift);
		midicon_midisock.program_change(0, 0 + shift);
		break;
	  case BTN_F2:
		sound.g_play0(1 + shift);
		midicon_midisock.program_change(0, 1 + shift);
		break;
	  case BTN_F3:
		sound.g_play0(2 + shift);
		midicon_midisock.program_change(0, 2 + shift);
		break;
	  case BTN_F4:
		sound.g_play0(3 + shift);
		midicon_midisock.program_change(0, 3 + shift);
		break;
	  case BTN_F5:
		sound.g_play0(4 + shift);
		midicon_midisock.program_change(0, 4 + shift);
		break;
	  case BTN_F6:
		sound.g_play0(5 + shift);
		midicon_midisock.program_change(0, 5 + shift);
		break;
	  case BTN_F7:
		sound.g_play0(6 + shift);
		midicon_midisock.program_change(0, 6 + shift);
		break;
	  case BTN_F8:
		sound.g_play0(7 + shift);
		midicon_midisock.program_change(0, 7 + shift);
		break;
	  case BTN_F11:
		switch(audio_cpu_ticks)
		{
		  case 50:
			audio_cpu_ticks = 100;
			break;
		  case 100:
			audio_cpu_ticks = 250;
			break;
		  case 250:
			audio_cpu_ticks = 500;
			break;
		  case 500:
			audio_cpu_ticks = 1000;
			break;
		  default:
			audio_cpu_ticks = 50;
			break;
		}
		break;
	}
}

void st_profile_audio_t::pre_render()
{
	/*
	 * Heeelp! I just *can't* stay away from chances
	 * like this to play around... :-D
	 */
	static int dither = 0;
	int y = 0;
	int y2 = wmain->height();
	float t = SDL_GetTicks()/1000.0;
	while(y < y2)
	{
		float c1 = sin(y*0.11 + t*1.5)*30.0 + 30;
		float c2 = sin(y*0.07 + t*2.5)*25.0 + 25;
		float c3 = sin(y*0.03 - t)*40.0 + 40;
		//Wideband color dither - improves 15/16 bit modes.
		float c4 = (dither + y) & 1 ? 3.0 : 0.0;
		int r = (int)(c1 + c2 + c4);
		int g = (int)(c2 + 3.0 - c4);
		int b = (int)(c1 + c2 + c3 + c4);
		wmain->foreground(wmain->map_rgb(r, g, b));
		wmain->fillrect(0, y, wmain->width(), 1);
		++y;
	}
	dither = 1 - dither;
}

void st_profile_audio_t::post_render()
{
	kobo_basestate_t::post_render();

	wmain->font(B_BIG_FONT);
	wmain->center(20, "Audio CPU Load");

	wmain->font(B_NORMAL_FONT);
	Uint32 fgc = wmain->map_rgb(0xffcc00);
	Uint32 bgc = wmain->map_rgb(0x006600);
	char buf[40];
	for(int i = 0; i < AUDIO_CPU_FUNCTIONS; ++i)
	{
		int perc = (int)(audio_cpu_function[i] / audio_cpu_total * 100.0);
		wmain->foreground(fgc);
		wmain->fillrect(103, 50+i*12+9, (int)audio_cpu_function[i]/2, 2);
		wmain->fillrect(128+32, 50+i*12+9, (int)perc/2, 2);
		wmain->foreground(bgc);
		wmain->fillrect(103 + (int)audio_cpu_function[i]/2, 50+i*12+9,
				50 - (int)audio_cpu_function[i]/2, 2);
		wmain->fillrect(128+32 + (int)perc/2, 50+i*12+9, 50 - (int)perc/2, 2);
		snprintf(buf, sizeof(buf), "%s:%5.2f%% (%1.0f%%)",
				audio_cpu_funcname[i],
				audio_cpu_function[i],
				audio_cpu_function[i] / audio_cpu_total * 100.0);
		wmain->center_token(120, 50+i*12, buf, ':');
	}

	wmain->foreground(fgc);
	wmain->fillrect(80, 178, (int)audio_cpu_total, 4);
	wmain->foreground(bgc);
	wmain->fillrect(80 + (int)audio_cpu_total, 178, 100 - (int)audio_cpu_total, 4);
	wmain->font(B_BIG_FONT);
	snprintf(buf, sizeof(buf), "Total:%5.2f%%", audio_cpu_total);
	wmain->center_token(120, 180, buf, ':');

	wmain->font(B_NORMAL_FONT);
	snprintf(buf, sizeof(buf), "Pan [L/R]:%5.2f  "
			"Pitch [U/D]: %d  ",
			(float)pan/65536.0,
			pitch);
	wmain->center(200, buf);
	snprintf(buf, sizeof(buf), "F1..F8 [+/-]: %d..%d",
			shift, shift + 7);
	wmain->center(215, buf);
}

st_profile_audio_t st_profile_audio;

#endif /*PROFILE_AUDIO*/

