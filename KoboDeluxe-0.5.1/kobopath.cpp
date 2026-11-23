/*
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Path helpers shared by the game and the graphics engine.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "kobopath.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void kobo_home_path(char *buf, size_t len, const char *suffix)
{
	const char *home = getenv("HOME");
	if(home && *home)
		snprintf(buf, len, "%s%s", home, suffix);
	else
		snprintf(buf, len, "%s", suffix);
}


void kobo_makedirs(const char *path)
{
	char buf[PATH_MAX];
	char *p;
	size_t len = strlen(path);
	if(len + 1 > sizeof(buf))
		return;
	strcpy(buf, path);
	for(p = buf + 1; *p; ++p)
		if(*p == '/')
		{
			*p = 0;
			::mkdir(buf, 0755);
			*p = '/';
		}
	::mkdir(buf, 0755);
}


void kobo_app_support_dir(char *buf, size_t len)
{
	kobo_home_path(buf, len,
			"/Library/Application Support/KoboDeluxe");
	kobo_makedirs(buf);
}
