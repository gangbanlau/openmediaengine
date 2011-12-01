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
#ifndef _UTILS_H_
#define _UTILS_H_

#include "pj_inc.h"

/** Codec **/
pj_status_t codecs_init(pjmedia_endpt *med_endpt);

pj_status_t codecs_deinit(pjmedia_endpt *med_endpt);


/** Media Stream **/
pj_status_t stream_create( pj_pool_t *pool,
				  pjmedia_endpt *med_endpt,
				  const pjmedia_codec_info *codec_info,
				  unsigned int ptime,
				  unsigned int rtp_ssrc,
				  pjmedia_dir dir,
				  pjmedia_transport *transport,
				  const pj_sockaddr_in *rem_addr,
				  pjmedia_stream **p_stream);

/** Misc **/
void register_thread();				/* Register external thread */

pj_atomic_value_t getavailableid(pj_atomic_t * atomic_var);		/* Get available id */

#endif // _UTILS_H_
