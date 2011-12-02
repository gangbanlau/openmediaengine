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
#include "conference.h"

#include "voxve.h"

#include "global.h"
#include "config.h"
#include "utils.h"
#include "snd.h"

#define THIS_FILE "voxve_conference.cpp"

extern struct voxve_data voxve_var;

int voxve_conf_create()
{
	register_thread();

	pj_status_t status;
	pjmedia_conf * p_conf = NULL;

	status = pjmedia_conf_create(voxve_var.pool, CONF_MAX_SLOTS, voxve_var.clock_rate, voxve_var.channel_count,
			(voxve_var.clock_rate * voxve_var.audio_frame_ptime / 1000 * voxve_var.channel_count), NBITS,
			PJMEDIA_CONF_NO_DEVICE, &p_conf
			);
	PJ_ASSERT_RETURN(status == PJ_SUCCESS, -1);

	pjmedia_port * null_snd = NULL;
	status = pjmedia_null_port_create(voxve_var.pool, voxve_var.snd_clock_rate, voxve_var.channel_count,
		(voxve_var.snd_clock_rate * voxve_var.audio_frame_ptime / 1000 * voxve_var.channel_count), NBITS, &null_snd);
	if (status != PJ_SUCCESS)
	{
		pjmedia_conf_destroy(p_conf);
		return -1;
	}

	/* Port zero's port. */
	pjmedia_port* port0 = pjmedia_conf_get_master_port(p_conf);

	pjmedia_master_port * master_port = NULL;
	status = pjmedia_master_port_create(voxve_var.pool, null_snd, port0, 0, &master_port);
	if (status != PJ_SUCCESS)
	{
		pjmedia_conf_destroy(p_conf);
		pjmedia_port_destroy(null_snd);
		return -1;
	}

	status = pjmedia_master_port_start(master_port);
	if (status != PJ_SUCCESS)
	{
		pjmedia_conf_destroy(p_conf);
		pjmedia_master_port_destroy(master_port, PJ_FALSE);
		pjmedia_port_destroy(null_snd);
		return -1;
	}

	conf_t * conf = new conf_t;

	conf->bits_per_sample = NBITS;
	conf->channel_count = voxve_var.channel_count;
	conf->id = getavailableid(voxve_var.atomic_var);
	conf->max_slots = CONF_MAX_SLOTS;
	conf->p_conf = p_conf;
	conf->samples_per_frame = (voxve_var.clock_rate * voxve_var.audio_frame_ptime / 1000 * voxve_var.channel_count);
	conf->sampling_rate = voxve_var.clock_rate;
	conf->null_snd = null_snd;
	conf->master_port = master_port;
	conf->snd_port = NULL;

	pj_rwmutex_lock_write(voxve_var.activeconfs_rwmutex);
	voxve_var.activeconfs[conf->id] = conf;
	pj_rwmutex_unlock_write(voxve_var.activeconfs_rwmutex);

	return conf->id;
}

voxve_status_t voxve_conf_destroy(int conf_id)
{
	register_thread();

	conf_t * conf = NULL;
//	pj_status_t status;

	pj_rwmutex_lock_write(voxve_var.activeconfs_rwmutex);
	std::map<int, conf_t*>::iterator iter = voxve_var.activeconfs.find(conf_id);
	if (iter != voxve_var.activeconfs.end())
	{
		conf = (*iter).second;

		voxve_var.activeconfs.erase(iter);
	}
	pj_rwmutex_unlock_write(voxve_var.activeconfs_rwmutex);

	if (conf != NULL)
	{
		if (conf->master_port != NULL)
		{
			pjmedia_master_port_destroy(conf->master_port, PJ_FALSE);
			conf->master_port = NULL;
		}

		if (conf->null_snd != NULL)
		{
			pjmedia_port_destroy(conf->null_snd);
			conf->null_snd = NULL;
		}

		if (conf->snd_port != NULL)
		{
			snd_close(conf->snd_port);
			conf->snd_port = NULL;
		}

		if (conf->p_conf != NULL)
		{
			pjmedia_conf_destroy(conf->p_conf);
			conf->p_conf = NULL;
		}

		delete conf;

		return 0;
	}
	else
		return -1;
}

int voxve_conf_addchannel(int conf_id, int channel_id)
{
	register_thread();

	channel_t * channel = channel_find(channel_id);

	if (channel == NULL)
	{
		PJ_LOG(3, (THIS_FILE, "Can't find channel, id %d", channel_id));
		return -1;
	}

	conf_t * conf = conf_find(conf_id);

	if (conf == NULL)
	{
		PJ_LOG(3, (THIS_FILE, "Can't find conference bridge, id %d", conf_id));
		return -1;
	}

	if (channel->is_conferencing)
	{
		PJ_LOG(2, (THIS_FILE, "This channel already inside bridge: %d", conf_id));
		return -1;
	}

	pjmedia_port *stream_port = NULL;
	pj_status_t status = pjmedia_stream_get_port(channel->stream, &stream_port);
	PJ_ASSERT_RETURN(status == PJ_SUCCESS, -1);

	/* Make sure disconnect from snd device */
//	status = voxve_channel_stopplayout(channel->id);
//	PJ_ASSERT_RETURN(status == PJ_SUCCESS, -1);
	status = channel_disconnectnullsnd(channel);
	PJ_ASSERT_RETURN(status == PJ_SUCCESS, -1);

	unsigned slot = 0;

	status = pjmedia_conf_add_port(conf->p_conf, voxve_var.pool, stream_port, NULL, &slot);
	PJ_ASSERT_RETURN(status == PJ_SUCCESS, -1);

	channel->is_conferencing = true;
	channel->conf_slot = slot;
	channel->conf_id = conf_id;

	return slot;
}

voxve_status_t voxve_conf_removechannel(int conf_id, int channel_id)
{
	register_thread();

	conf_t * conf = conf_find(conf_id);

	if (conf == NULL)
	{
		PJ_LOG(3, (THIS_FILE, "Can't find conference bridge, id %d", conf_id));
		return -1;
	}

	channel_t * channel = channel_find(channel_id);

	if (channel == NULL)
	{
		PJ_LOG(3, (THIS_FILE, "Can't find channel, id %d", channel_id));
		return -1;
	}

	pj_status_t status = pjmedia_conf_remove_port(conf->p_conf, channel->conf_slot);
	PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

	channel->is_conferencing = false;
	channel->conf_slot = -1;
	channel->conf_id = -1;

	return channel_connectnullsnd(channel);
}

voxve_status_t voxve_conf_connect(int conf_id, unsigned source_slot, unsigned dst_slot)
{
	register_thread();

	conf_t * conf = conf_find(conf_id);

	if (conf == NULL)
	{
		PJ_LOG(3, (THIS_FILE, "Can't find conference bridge, id %d", conf_id));
		return -1;
	}

	return pjmedia_conf_connect_port(conf->p_conf, source_slot, dst_slot, 0);
}

voxve_status_t voxve_conf_disconnect(int conf_id, unsigned source_slot, unsigned dst_slot)
{
	register_thread();

	conf_t * conf = conf_find(conf_id);

	if (conf == NULL)
	{
		PJ_LOG(3, (THIS_FILE, "Can't find conference bridge, id %d", conf_id));
		return -1;
	}

	return pjmedia_conf_disconnect_port(conf->p_conf, source_slot, dst_slot);
}

/* Select or change sound device */
voxve_status_t voxve_conf_setsnddev(int conf_id, int cap_dev, int playback_dev)
{
	register_thread();

	pj_status_t status;

	conf_t * conf = conf_find(conf_id);

	if (conf == NULL)
	{
		PJ_LOG(3, (THIS_FILE, "Can't find conference bridge, id %d", conf_id));
		return -1;
	}

	if (conf->snd_port != NULL)
	{
		if (cap_dev == conf->rec_dev_id && playback_dev == conf->playback_dev_id)
		{
			return 0;
		}

		snd_close(conf->snd_port);
		conf->snd_port = NULL;
	}
	else
	{
		/* snd not add to bridge */
		if (conf->master_port != NULL)
		{
			pjmedia_master_port_destroy(conf->master_port, PJ_FALSE);
			conf->master_port = NULL;
		}

		if (conf->null_snd != NULL)
		{
			pjmedia_port_destroy(conf->null_snd);
			conf->null_snd = NULL;
		}
	}

	// FIXME resample if necessary if the bridge's clock rate is different than the sound device's
	// clock rate.

	pjmedia_snd_port * snd_port = NULL;
	status = pjmedia_snd_set_latency(SND_DEFAULT_REC_LATENCY, SND_DEFAULT_PLAY_LATENCY);

	status = pjmedia_snd_port_create(voxve_var.pool, cap_dev, playback_dev, voxve_var.snd_clock_rate, voxve_var.channel_count,
		(voxve_var.snd_clock_rate * voxve_var.audio_frame_ptime / 1000 * voxve_var.channel_count), NBITS, 0, &snd_port);
	PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

	/* Set AEC */
	pjmedia_snd_port_set_ec(snd_port, voxve_var.pool, voxve_var.ec_tail_len, 0);

	conf->snd_port = snd_port;

	/* Port zero's port. */
	pjmedia_port* port0 = pjmedia_conf_get_master_port(conf->p_conf);

	status = pjmedia_snd_port_connect(snd_port, port0);
	PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

	return 0;
}

voxve_status_t voxve_conf_setnosnddev(int conf_id)
{
	register_thread();

	pj_status_t status;

	conf_t * conf = conf_find(conf_id);

	if (conf == NULL)
	{
		PJ_LOG(3, (THIS_FILE, "Can't find conference bridge, id %d", conf_id));
		return -1;
	}

	if (conf->snd_port != NULL)
	{
		snd_close(conf->snd_port);
		conf->snd_port = NULL;
	}

	if (conf->null_snd == NULL)
	{
		pjmedia_port * null_snd = NULL;
		status = pjmedia_null_port_create(voxve_var.pool, voxve_var.snd_clock_rate, voxve_var.channel_count,
			(voxve_var.snd_clock_rate * voxve_var.audio_frame_ptime / 1000 * voxve_var.channel_count), NBITS, &null_snd);
		if (status != PJ_SUCCESS)
		{
			return status;
		}

		/* Port zero's port. */
		pjmedia_port* port0 = pjmedia_conf_get_master_port(conf->p_conf);

		pjmedia_master_port * master_port = NULL;
		status = pjmedia_master_port_create(voxve_var.pool, null_snd, port0, 0, &master_port);
		if (status != PJ_SUCCESS)
		{
			pjmedia_port_destroy(null_snd);
			return status;
		}

		status = pjmedia_master_port_start(master_port);
		if (status != PJ_SUCCESS)
		{
			pjmedia_master_port_destroy(master_port, PJ_FALSE);
			pjmedia_port_destroy(null_snd);
			return status;
		}

		conf->null_snd = null_snd;
		conf->master_port = master_port;
	}

	return 0;
}

int voxve_conf_getportcount(int conf_id)
{
	register_thread();

	conf_t * conf = conf_find(conf_id);

	if (conf != NULL)
	{
		return pjmedia_conf_get_port_count(conf->p_conf);
	}
	else
		return -1;
}

int voxve_conf_getconnectcount(int conf_id)
{
	register_thread();

	conf_t * conf = conf_find(conf_id);

	if (conf != NULL)
	{
		return pjmedia_conf_get_connect_count(conf->p_conf);
	}
	else
		return -1;

}

voxve_status_t voxve_conf_getsignallevel(int conf_id, unsigned slot, unsigned *tx_level, unsigned *rx_level)
{
	register_thread();

	conf_t * conf = conf_find(conf_id);

	if (conf != NULL)
	{
		return pjmedia_conf_get_signal_level(conf->p_conf, slot, tx_level, rx_level);
	}
	else
		return -1;
}

voxve_status_t voxve_conf_adjustrxlevel(int conf_id, unsigned slot, int adj_level)
{
	register_thread();

	conf_t * conf = conf_find(conf_id);

	if (conf != NULL)
	{
		return pjmedia_conf_adjust_rx_level(conf->p_conf, slot, adj_level);
	}
	else
		return -1;
}

voxve_status_t voxve_conf_adjusttxlevel(int conf_id, unsigned slot, int adj_level)
{
	register_thread();

	conf_t * conf = conf_find(conf_id);

	if (conf != NULL)
	{
		return pjmedia_conf_adjust_tx_level(conf->p_conf, slot, adj_level);
	}
	else
		return -1;

}

voxve_status_t voxve_conf_configureport(int conf_id, unsigned slot, voxve_port_op tx_op, voxve_port_op rx_op)
{
	register_thread();

	pjmedia_port_op t_op, r_op;

	switch (tx_op)
	{
	case PORT_NO_CHANGE:
		t_op = PJMEDIA_PORT_NO_CHANGE;
		break;
	case PORT_DISABLE:
		t_op = PJMEDIA_PORT_DISABLE;
		break;
	case PORT_MUTE:
		t_op = PJMEDIA_PORT_MUTE;
		break;
	case PORT_ENABLE:
		t_op = PJMEDIA_PORT_ENABLE;
		break;
	default:
		t_op = PJMEDIA_PORT_NO_CHANGE;
	}

	switch (rx_op)
	{
	case PORT_NO_CHANGE:
		r_op = PJMEDIA_PORT_NO_CHANGE;
		break;
	case PORT_DISABLE:
		r_op = PJMEDIA_PORT_DISABLE;
		break;
	case PORT_MUTE:
		r_op = PJMEDIA_PORT_MUTE;
		break;
	case PORT_ENABLE:
		r_op = PJMEDIA_PORT_ENABLE;
		break;
	default:
		r_op = PJMEDIA_PORT_NO_CHANGE;
	}

	conf_t * conf = conf_find(conf_id);

	if (conf != NULL)
	{
		return pjmedia_conf_configure_port(conf->p_conf, slot, t_op, r_op);
	}
	else
		return -1;
}

void voxve_conf_set_clockrate(unsigned clock_rate)
{
	if (clock_rate < 8000 || clock_rate > 192000)
	{
		printf("Error: expecting value between 8000-192000 for conference clock rate\r\n");
		return;
	}

	voxve_var.clock_rate = clock_rate;

	return;
}
