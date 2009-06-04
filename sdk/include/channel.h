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

#ifndef _CHANNEL_H_
#define _CHANNEL_H_

#include <pjlib.h>
#include <pjlib-util.h>
#include <pjmedia.h>
#include <pjmedia-codec.h>

#include "voxve.h"

/* Channel */
typedef struct voxve_channel
{
	int id;											/* unique id */

	pjmedia_stream *stream;

	pjmedia_transport *transport;

	pjmedia_snd_port *snd_port;

	pjmedia_port *null_snd;
	pjmedia_master_port *master_port;

	bool is_conferencing;	
	int conf_slot;									/* slot id if added into conf bridge */ 
	int conf_id;									/* conference bridge id */

} voxve_channel_t;

voxve_channel_t * channel_find(int channel_id);

int channel_internalcreate(voxve_channel_t * channel, unsigned short local_port);

voxve_status_t channel_connectnullsnd(voxve_channel_t * channel);

voxve_status_t channel_disconnectnullsnd(voxve_channel_t * channel);

#endif	// _CHANNEL_H_
