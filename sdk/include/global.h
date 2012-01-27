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

#ifndef GLOBAL_H_
#define GLOBAL_H_

#include <map>

#include "pj_inc.h"

#include "logging.h"
#include "conference.h"
#include "channel.h"

/* Global Data Struct */
struct voxve_data
{
	pj_caching_pool cp;							/* Global pool factory */
	pj_pool_t *pool;							/* App private pool */

	pjmedia_endpt *med_endpt;					/* Media endpoint */

	pj_timer_heap_t *timer_heap;

	std::map<int, channel_t *> activechannels;	/* Active channels */
	pj_rwmutex_t * activechannels_rwmutex;

	pj_atomic_t *atomic_var;					/* Atomic variables, available id */

	std::map<int, conf_t *> activeconfs;		/* Active conference bridges */
	pj_rwmutex_t * activeconfs_rwmutex;

	/* Logging */
    logging_config_t log_cfg;					/* Current logging config */
    pj_oshandle_t	 log_file;					/* Output log file handle */

	/* Sound device */
	int sound_rec_id;							/* Capture device ID */
	int sound_play_id;							/* Playback device ID */

	pj_bool_t	is_no_vad;						/* VAD */

    unsigned	ec_tail_len;					/* Echo canceller tail length, in miliseconds */

    /**
     * Clock rate to be applied to the conference bridge.
     */
    unsigned		clock_rate;

    /**
     * Clock rate to be applied when opening the sound device.
     */
    unsigned		snd_clock_rate;

    /**
     * Channel count be applied when opening the sound device and
     * conference bridge.
     */
    unsigned		channel_count;

    /**
     * Specify audio frame ptime. The value here will affect the
     * samples per frame of both the conference bridge and sound
	 * device. Specifying lower ptime will normally reduce the
	 * latency.
     */
    unsigned		audio_frame_ptime;

    /**
     * Specify maximum number of media ports to be created in the
     * conference bridge. Since all media terminate in the bridge
     * (calls, file player, file recorder, etc), the value must be
     * large enough to support all of them. However, the larger
     * the value, the more computations are performed.
     */
    unsigned		max_media_ports;

	pj_bool_t is_enable_stun;		/* STUN */
	pj_stun_config stun_cfg;		/* STUN global setting */
	pj_status_t	stun_status;		/* STUN server status */
	char stun_server_addr[128];
	pj_sockaddr	stun_srv;
};

#endif /* GLOBAL_H_ */
