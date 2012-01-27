/* 
 * Copyright (C) 2009-2012 Gang Liu <gangban.lau@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */
#ifndef __OPENMEDIAENGINE__
#error only libopenmediaengine should #include this header
#endif

#ifndef _CONFIG_H_
#define _CONFIG_H_

/* Conf bridge Constants */
#define DEFAULT_CLOCK_RATE	8000		/* The default clock rate to be used by the conference bridge */
#define DEFAULT_AUDIO_FRAME_PTIME	20	/* Default frame length in the conference bridge. */
#define NCHANNELS	1
#define NSAMPLES	(DEFAULT_CLOCK_RATE * DEFAULT_AUDIO_FRAME_PTIME / 1000 * NCHANNELS)
#define NBITS		16

#define CONF_MAX_SLOTS	10

/* Sound devices */
#define SND_DEFAULT_CLOCK_RATE	8000	/* The default clock rate to be used by sound devices */
#define SND_NSAMPLES	(SND_DEFAULT_CLOCK_RATE * DEFAULT_AUDIO_FRAME_PTIME / 1000 * NCHANNELS)

#define SND_DEFAULT_REC_LATENCY		100
#define SND_DEFAULT_PLAY_LATENCY	100

/* TODO Windows Vista Issue */
/* if SND_DEFAULT_REC_LATENCY 40 and SND_DEFAULT_PLAY_LATENCY 40, then sound playback choppy */
/* if DEFAULT_CLOCK_RATE 8000 and SND_DEFAULT_CLOCK_RATE 44000, then sound playback no voice */

#endif	// _CONFIG_H_
