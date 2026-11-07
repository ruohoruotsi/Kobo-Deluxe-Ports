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

#ifndef	_KOBO_PATH_H_
#define	_KOBO_PATH_H_

#include <stddef.h>

/* Expand a $HOME-relative suffix into an absolute path in buf.
 * Falls back to the bare suffix when $HOME is unset. */
void kobo_home_path(char *buf, size_t len, const char *suffix);

/* Create all missing parent directories of the given absolute path. */
void kobo_makedirs(const char *path);

/* Writable per-user app dir: $HOME/Library/Application Support/KoboDeluxe
 * (under the App Sandbox $HOME is the app container). Created. Used for
 * log.txt and screen*.bmp, since the bundle Resources dir is read-only. */
void kobo_app_support_dir(char *buf, size_t len);

#endif	/* _KOBO_PATH_H_ */
