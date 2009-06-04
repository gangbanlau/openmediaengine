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

#include <pjlib.h>
#include <pjlib-util.h>
#include <pjmedia.h>
#include <pjmedia-codec.h>

#include <map>

#include "channel.h"
#include "conference.h"

/* Logging */
enum VOXVE_LOG_LEVEL 
{
	VOXVE_LOG_ERR = 1,
	VOXVE_LOG_WARN,
	VOXVE_LOG_INFO,
	VOXVE_LOG_DEBUG,
	VOXVE_LOG_TRACE
};

/**
 * Logging configuration, which can be (optionally) specified when calling
 * #voxve_init(). Application must call #logging_config_default() to
 * initialize this structure with the default values.
 */
typedef struct voxve_logging_config
{
    /**
     * Input verbosity level. Value 5 is reasonable.
     */
    unsigned	level;

    /**
     * Verbosity level for console. Value 4 is reasonable.
     */
    unsigned	console_level;

    /**
     * Log decoration.
     */
    unsigned	decor;

    /**
     * Optional log filename.
     */
    pj_str_t	log_filename;

    /**
     * Optional callback function to be called to write log to 
     * application specific device. This function will be called for
     * log messages on input verbosity level.
     *
     * \par Sample Python Syntax:
     * \code
     # level:	integer
     # data:	string
     # len:	integer

     def cb(level, data, len):
	    print data,
     * \endcode
     */
    void       (*cb)(int level, const char *data, pj_size_t len);
} voxve_logging_config_t;

/* Global Data Struct */
struct voxve_data
{
	pj_caching_pool cp;							/* Global pool factory */
	pj_pool_t *pool;							/* App private pool */

	pjmedia_endpt *med_endpt;					/* Media endpoint */

	pj_timer_heap_t *timer_heap;

	std::map<int, voxve_channel_t *> activechannels;	/* Active channels */
	pj_rwmutex_t * activechannels_rwmutex;

	pj_atomic_t *atomic_var;							/* Atomic variables, available id */

	std::map<int, voxve_conf_t *> activeconfs;			/* Active conference bridges */
	pj_rwmutex_t * activeconfs_rwmutex;

	/* Logging */
    voxve_logging_config log_cfg;				/* Current logging config */
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


/** Logging **/
void logging_config_default(voxve_logging_config *cfg);

pj_status_t logging_reconfigure(const voxve_logging_config_t *cfg);

void logging(const char *sender, VOXVE_LOG_LEVEL log_level, const char *title, pj_status_t status);


/** Misc **/
void register_thread();				/* Register external thread */

pj_atomic_value_t getavailableid(pj_atomic_t * atomic_var);		/* Get available id */

#endif // _UTILS_H_
