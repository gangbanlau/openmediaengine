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
#include "voxve.h"

#include "global.h"
#include "utils.h"
#include "snd.h"
#include "config.h"

#define THIS_FILE "voxve_channel.cpp"

extern struct voxve_data voxve_var;

int voxve_channel_create(unsigned short local_port)
{
	register_thread();

	int channel_id = getavailableid(voxve_var.atomic_var);

	channel_t * channel = new channel_t;
	channel->id = channel_id;

	int rtCode = channel_internalcreate(channel, local_port);

	if (rtCode != 0)
	{
		delete channel;
		return -1;
	}
	else
		return channel_id;
}

voxve_status_t voxve_channel_startstream(int channel_id, voxve_codec_id_t codec, unsigned int ptime, char * remote_ip, unsigned short remote_port, voxve_stream_dir dir)
{
	return voxve_channel_startstream2(channel_id, codec, ptime, 0, remote_ip, remote_port, dir);
}

voxve_status_t voxve_channel_startstream2(int channel_id, voxve_codec_id_t codec, unsigned int ptime, unsigned int rtp_ssrc, char * remote_ip, unsigned short remote_port, voxve_stream_dir dir)
{
	register_thread();

	/* Find which codec to use. */
	pj_str_t str_codec_id;

	switch(codec)
	{
	case CODEC_G729:
		str_codec_id = pj_str("g729");
		break;
	case CODEC_PCMU:
		str_codec_id = pj_str("pcmu");
		break;
	case CODEC_PCMA:
		str_codec_id = pj_str("pcma");
		break;
	case CODEC_GSM:
		str_codec_id = pj_str("gsm");
		break;
	default:
		str_codec_id = pj_str("pcmu");
	}

	pjmedia_dir pj_dir;

	switch (dir)
	{
	case STREAM_DIR_NONE:
		pj_dir = PJMEDIA_DIR_NONE;
		break;
	case STREAM_DIR_ENCODING:
		pj_dir = PJMEDIA_DIR_ENCODING;
		break;
	case STREAM_DIR_DECODING:
		pj_dir = PJMEDIA_DIR_DECODING;
		break;
	case STREAM_DIR_ENCODING_DECODING:
		pj_dir = PJMEDIA_DIR_ENCODING_DECODING;
		break;
	default:
		pj_dir = PJMEDIA_DIR_ENCODING_DECODING;
	}

	const pjmedia_codec_info *codec_info;
	unsigned count = 1;

	pjmedia_codec_mgr *codec_mgr = pjmedia_endpt_get_codec_mgr(voxve_var.med_endpt);
	pj_status_t status = pjmedia_codec_mgr_find_codecs_by_id(codec_mgr, &str_codec_id, &count, &codec_info, NULL);
	if (status != PJ_SUCCESS)
	{
//	    printf("Error: unable to find codec %s\n", codec_id);
	    return -1;
	}

	/* remote media addr */
	pj_sockaddr_in remote_addr;
	pj_bzero(&remote_addr, sizeof(remote_addr));
	pj_str_t ip = pj_str(remote_ip);
	pj_uint16_t port = (pj_uint16_t)remote_port;

	status = pj_sockaddr_in_init(&remote_addr, &ip, port);
	if (status != PJ_SUCCESS)
	{
		    return -1;
	}

	channel_t * channel = channel_find(channel_id);

	if (channel == NULL)
	{
		return -1;
	}

    /* Create stream based on arguments */
	pjmedia_stream *stream = NULL;
	status = stream_create(voxve_var.pool, voxve_var.med_endpt, codec_info, ptime, rtp_ssrc, pj_dir, channel->transport,
			   &remote_addr, &stream);

	if (status != PJ_SUCCESS)
		return -1;
	else
	{
		/* Start streaming */
		status = pjmedia_stream_start(stream);

		if (status != PJ_SUCCESS)
		{
			pjmedia_stream_destroy(stream);
			return status;
		}
		else
			channel->stream = stream;

		/* Null Snd provide clock timing */
		return channel_connectnullsnd(channel);
	}

}

voxve_status_t voxve_channel_stopstream(int channel_id)
{
	register_thread();

	channel_t *channel = channel_find(channel_id);

	if (channel == NULL)
		return -1;

	/* Destroy Null Snd first, it will stop provide clocking timing */
	/* Otherwise may be crashed at stream put_frame() or get_freame() calling */
	channel_disconnectnullsnd(channel);

	/* Destroy Stream */
	if (channel->stream != NULL)
	{
		pjmedia_stream_destroy(channel->stream);
		channel->stream = NULL;
	}

	return 0;
}

voxve_status_t voxve_channel_delete(int channelid)
{
	register_thread();

	pj_status_t status;
	bool hasError = false;
	channel_t * channel = NULL;

	pj_rwmutex_lock_write(voxve_var.activechannels_rwmutex);
	std::map<int, channel_t*>::iterator iter = voxve_var.activechannels.find(channelid);
	if (iter != voxve_var.activechannels.end())
	{
		channel = (*iter).second;

		voxve_var.activechannels.erase(iter);
	}
	pj_rwmutex_unlock_write(voxve_var.activechannels_rwmutex);

	if (channel != NULL)
	{
		/* Destroy sound device first before stream */
		if (channel->snd_port != NULL)
		{
			snd_close(channel->snd_port);
			channel->snd_port = NULL;
		}

		channel_disconnectnullsnd(channel);

		/* Destroy stream */
		if (channel->stream)
		{
			pjmedia_stream_destroy(channel->stream);
			channel->stream = NULL;
		}

		/* Destroy transport */
		if (channel->transport != NULL)
		{
//			status = pjmedia_transport_udp_close(channel->transport);
			status = pjmedia_transport_media_stop(channel->transport);
			status = pjmedia_transport_close(channel->transport);
			channel->transport = NULL;
			if (status != PJ_SUCCESS)
			{
				logging(THIS_FILE, VOXVE_LOG_WARN, "Error close transport", status);
				hasError = true;
			}

		}

		delete channel;

		if (hasError)
			return -1;
		else
			return 0;
	}
	else
		return -1;
}

voxve_status_t voxve_channel_startplayout(int channelid)
{
	register_thread();

	channel_t * channel = channel_find(channelid);
	pj_status_t status;

	if (channel != NULL)
	{
		if (channel->snd_port != NULL)
			return 0;

		/* Get the port interface of the stream */
		pjmedia_port *stream_port = NULL;
		status = pjmedia_stream_get_port(channel->stream, &stream_port);
		PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

		/* Create sound device port. */
		pjmedia_snd_port *snd_port = NULL;
		status = pjmedia_snd_set_latency(SND_DEFAULT_REC_LATENCY, SND_DEFAULT_PLAY_LATENCY);
		PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

#define USEC_IN_SEC (pj_uint64_t)1000000
		unsigned samples_per_frame = stream_port->info.fmt.det.aud.frame_time_usec * stream_port->info.fmt.det.aud.clock_rate
					* stream_port->info.fmt.det.aud.channel_count / USEC_IN_SEC;
		status = pjmedia_snd_port_create(voxve_var.pool, voxve_var.sound_rec_id, voxve_var.sound_play_id,
					stream_port->info.fmt.det.aud.clock_rate,
					stream_port->info.fmt.det.aud.channel_count,
					samples_per_frame,
					stream_port->info.fmt.det.aud.bits_per_sample,
					0, &snd_port);
		PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

		/* Set AEC */
		pjmedia_snd_port_set_ec(snd_port, voxve_var.pool, voxve_var.ec_tail_len, 0);

		channel_disconnectnullsnd(channel);

		channel->snd_port = snd_port;

		/* Connect sound port to stream */
		status = pjmedia_snd_port_connect(snd_port, stream_port);
		PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

		return 0;
	}
	else
		return -1;
}

voxve_status_t voxve_channel_stopplayout(int channelid)
{
	register_thread();

	channel_t * channel = channel_find(channelid);

	if (channel != NULL)
	{
		if (channel->snd_port != NULL)
		{
			snd_close(channel->snd_port);
			channel->snd_port = NULL;
		}

		return channel_connectnullsnd(channel);
	}
	else
		return -1;
}

voxve_status_t voxve_channel_putonhold(int channelid, bool enable)
{
	register_thread();

	pj_status_t status;

	if (enable)
	{
		/* Hold */
		status = voxve_channel_stopplayout(channelid);
		PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

		return 0;
	}
	else
	{
		/* Unhold */
		status = voxve_channel_startplayout(channelid);
		PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

		return 0;
	}
}

voxve_status_t voxve_channel_update(int channel_id, voxve_codec_id_t codec, unsigned int ptime, unsigned short local_port, char * remote_ip, unsigned short remote_port, voxve_stream_dir dir)
{
	register_thread();

	channel_t * channel = channel_find(channel_id);
	pj_status_t status;
	bool connected_snd = false;

	if (channel == NULL)
		return -1;

	/* Destroy sound device first before stream */
	if (channel->snd_port != NULL)
	{
		snd_close(channel->snd_port);
		channel->snd_port = NULL;

		connected_snd = true;
	}

	channel_disconnectnullsnd(channel);

	/* Destroy stream */
	if (channel->stream)
	{
		pjmedia_stream_destroy(channel->stream);
		channel->stream = NULL;
	}

	/* Destroy transport */
	if (channel->transport != NULL)
	{
//		status = pjmedia_transport_udp_close(channel->transport);
		status = pjmedia_transport_close(channel->transport);
		channel->transport = NULL;
		if (status != PJ_SUCCESS)
		{
			logging(THIS_FILE, VOXVE_LOG_WARN, "Error close transport", status);
			PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);
		}
	}

	int result = channel_internalcreate(channel, local_port);

	if (result == -1)
		return -1;
	else
		status = voxve_channel_startstream(channel->id, codec, ptime, remote_ip, remote_port, dir);

	PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

	if (connected_snd)
	{
		status = voxve_channel_startplayout(channel->id);
		PJ_ASSERT_RETURN(status == PJ_SUCCESS, -1);
	}

	return 0;
}

