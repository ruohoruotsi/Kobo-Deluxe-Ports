/*(LGPL)
---------------------------------------------------------------------------
	a_struct.h - Audio Engine internal structures
---------------------------------------------------------------------------
 * Copyright (C) 2001, 2002, David Olofson
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

#ifndef _A_STRUCT_H_
#define _A_STRUCT_H_

#include "a_wave.h"
#include "a_patch.h"
#include "a_channel.h"
#include "a_voice.h"
#include "a_bus.h"

/*----------------------------------------------------------
	Engine Global Data
----------------------------------------------------------*/
audio_wave_t wavetab[AUDIO_MAX_WAVES];
audio_patch_t patchtab[AUDIO_MAX_PATCHES];
audio_group_t grouptab[AUDIO_MAX_GROUPS];
audio_channel_t channeltab[AUDIO_MAX_CHANNELS];
audio_voice_t voicetab[AUDIO_MAX_VOICES];
audio_bus_t bustab[AUDIO_MAX_BUSSES];

void _audio_init(void);

#endif /*_A_STRUCT_H_*/
