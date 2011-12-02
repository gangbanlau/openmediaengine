/*
 * Copyright (C) 2011 Gang Liu <gangban.lau@gmail.com>
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
#include "nat_helper.h"

#include "voxve.h"

#include "global.h"
#include "utils.h"
#include "channel.h"

#define THIS_FILE "voxve_nat.cpp"

extern struct voxve_data voxve_var;

/** Enable STUN support **/
voxve_status_t voxve_stun_enable(const char * stun_server_addr)
{
	voxve_var.is_enable_stun = PJ_TRUE;
	pj_ansi_snprintf(voxve_var.stun_server_addr, sizeof(voxve_var.stun_server_addr), "%s", stun_server_addr);

	return 0;
}

/** Get resolved public address via STUN **/
voxve_status_t voxve_stun_get_public_addr(int channel_id, char * addr_buf, unsigned buf_len, unsigned &port)
{
	PJ_ASSERT_RETURN(voxve_var.is_enable_stun, PJ_FALSE);

	channel_t * channel = channel_find(channel_id);
	if (channel != NULL)
	{
		pj_sockaddr_print(&channel->skinfo.rtp_addr_name, addr_buf, buf_len, 0);
		port = pj_sockaddr_get_port(&channel->skinfo.rtp_addr_name);

		return PJ_SUCCESS;
	}
	else
		return PJ_FALSE;
}

