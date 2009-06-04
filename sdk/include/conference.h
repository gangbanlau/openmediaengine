/* 
 * Copyright (C) 2009 Gang Liu <gangban.lau@gmail.com>
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

#ifndef _CONFERENCE_H_
#define _CONFERENCE_H_

#include <pjlib.h>
#include <pjlib-util.h>
#include <pjmedia.h>
#include <pjmedia-codec.h>

/* Conference bridge */
typedef struct voxve_conf
{
	int id;											/* unique id */

	pjmedia_conf *p_conf;
	
	unsigned max_slots;
	unsigned sampling_rate;
	unsigned channel_count;
	unsigned samples_per_frame;
	unsigned bits_per_sample;

	pjmedia_port *null_snd;
	pjmedia_master_port *master_port;				/* Provide clock timing */

	pjmedia_snd_port *snd_port;
	int rec_dev_id;
	int playback_dev_id;

} voxve_conf_t;

voxve_conf_t * conf_find(int conf_id);

#endif	// _CONFERENCE_H_
