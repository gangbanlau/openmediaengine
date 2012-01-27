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
#include "channel.h"

#include "voxve.h"
#include "voxve_log.h"
#include "logging.h"
#include "global.h"

#include "config.h"
#include "utils.h"
#include "nat_helper.h"
#include "snd.h"

#define THIS_FILE "channel.cpp"

extern struct voxve_data voxve_var;

channel_t * channel_find(int channel_id)
{
	channel_t * channel = NULL;

	pj_rwmutex_lock_read(voxve_var.activechannels_rwmutex);
	std::map<int, channel_t*>::iterator iter = voxve_var.activechannels.find(channel_id);

	if (iter != voxve_var.activechannels.end())
	{
		channel = (*iter).second;
	}
	pj_rwmutex_unlock_read(voxve_var.activechannels_rwmutex);

	return channel;
}

int channel_internalcreate(channel_t * channel, const char *local_ip, unsigned short local_port)
{
	/* Create media transport */
	pjmedia_transport *transport = NULL;
	pj_status_t status;

	if (voxve_var.is_enable_stun && strlen(voxve_var.stun_server_addr) > 0)
	{
		/* Make sure STUN server resolution has completed */
		status = stun_resolve_server(PJ_TRUE);
		if (status != PJ_SUCCESS) 
		{
			logging(THIS_FILE, VOXVE_LOG_WARN, "Error resolving STUN server", status);
			return -1;
		}

		pjmedia_sock_info *skinfo = &(channel->skinfo);
		status = stun_create_rtp_rtcp_sock(skinfo, local_port);

		if (status != PJ_SUCCESS) 
		{
			logging(THIS_FILE, VOXVE_LOG_WARN, "Unable to create RTP/RTCP socket", status);
			return -1;
		}

		status = pjmedia_transport_udp_attach(voxve_var.med_endpt, NULL, skinfo, 0, &transport);
		if (status != PJ_SUCCESS) 
		{
			logging(THIS_FILE, VOXVE_LOG_WARN, "Unable to create media transport", status);
			/* FIXME free rtp/rtcp sock */
			return -1;
		}
	}
	else if (local_ip != NULL)
	{
		pj_str_t addr = pj_str((char *)local_ip);
		status = pjmedia_transport_udp_create2(voxve_var.med_endpt, NULL, &addr, local_port, 0, &transport);
	}
	else
		status = pjmedia_transport_udp_create(voxve_var.med_endpt, NULL, local_port, 0, &transport);

    if (status != PJ_SUCCESS)
	{
		logging(THIS_FILE, VOXVE_LOG_WARN, "Error create transport", status);
		return -1;
	}

	channel->snd_port = NULL;
	channel->null_snd = NULL;
	channel->master_port = NULL;
	channel->stream = NULL;
	channel->transport = transport;
	channel->is_conferencing = false;
	channel->conf_slot = -1;
	channel->conf_id = -1;

	pj_rwmutex_lock_write(voxve_var.activechannels_rwmutex);
	voxve_var.activechannels[channel->id] = channel;
	pj_rwmutex_unlock_write(voxve_var.activechannels_rwmutex);

	return 0;
}

voxve_status_t channel_connectnullsnd(channel_t * channel)
{
	/* Get the port interface of the stream */
	pjmedia_port *stream_port = NULL;
	pj_status_t status = pjmedia_stream_get_port(channel->stream, &stream_port);
	PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

	/* Create Null Snd */
	pjmedia_port *null_snd = NULL;
#define USEC_IN_SEC (pj_uint64_t)1000000
	unsigned samples_per_frame = stream_port->info.fmt.det.aud.frame_time_usec * stream_port->info.fmt.det.aud.clock_rate
				* stream_port->info.fmt.det.aud.channel_count / USEC_IN_SEC;
	status = pjmedia_null_port_create(channel->pool,
				stream_port->info.fmt.det.aud.clock_rate, stream_port->info.fmt.det.aud.channel_count,
				samples_per_frame, stream_port->info.fmt.det.aud.bits_per_sample,
				&null_snd);
	PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);
		
	pjmedia_master_port *master_port = NULL;
	status = pjmedia_master_port_create(channel->pool, null_snd, stream_port, 0, &master_port);
	if (status != PJ_SUCCESS)
	{
		pjmedia_port_destroy(null_snd);
		return status;
	}

	channel->null_snd = null_snd;
	channel->master_port = master_port;

	/* Start the media flow. */
	return pjmedia_master_port_start(master_port);
}

voxve_status_t channel_disconnectnullsnd(channel_t * channel)
{
	/* Destroy master port and null port */
	if (channel->master_port != NULL)
	{
		pjmedia_master_port_destroy(channel->master_port, PJ_FALSE);
		channel->master_port = NULL;
	}

	if (channel->null_snd != NULL)
	{
		pjmedia_port_destroy(channel->null_snd);
		channel->null_snd = NULL;
	}

	return 0;
}
