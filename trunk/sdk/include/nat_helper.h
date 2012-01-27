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

#ifndef _NAT_HELPER_H_
#define _NAT_HELPER_H_

#include "pj_inc.h"

/*
 * Resolve STUN server.
 */
pj_status_t stun_resolve_server(pj_bool_t wait);

/* 
 * Create RTP and RTCP socket pair, and possibly resolve their public
 * address via STUN.
 */
pj_status_t stun_create_rtp_rtcp_sock(pjmedia_sock_info *skinfo, unsigned short local_port);

#endif	// _NAT_HELPER_H_
