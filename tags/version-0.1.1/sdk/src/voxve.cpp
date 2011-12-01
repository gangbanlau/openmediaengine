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
#include "voxve.h"

#include "pj_inc.h"

#include <map>

#include "global.h"
#include "config.h"
#include "utils.h"
#include "timer.h"
#include "snd.h"

#define THIS_FILE	"voxve.cpp"

/* voxve application instance. */
struct voxve_data voxve_var;

/* Init random seed */
static void init_random_seed(void)
{
    pj_sockaddr addr;
    const pj_str_t *hostname;
    pj_uint32_t pid;
    pj_time_val t;
    unsigned seed=0;

    /* Add hostname */
    hostname = pj_gethostname();
    seed = pj_hash_calc(seed, hostname->ptr, (int)hostname->slen);

    /* Add primary IP address */
    if (pj_gethostip(pj_AF_INET(), &addr)==PJ_SUCCESS)
	seed = pj_hash_calc(seed, &addr.ipv4.sin_addr, 4);

    /* Get timeofday */
    pj_gettimeofday(&t);
    seed = pj_hash_calc(seed, &t, sizeof(t));

    /* Add PID */
    pid = pj_getpid();
    seed = pj_hash_calc(seed, &pid, sizeof(pid));

    /* Init random seed */
    pj_srand(seed);
	return;
}

void voxve_enable_vad()
{
	voxve_var.is_no_vad = PJ_FALSE;
}

void voxve_disable_vad()
{
	voxve_var.is_no_vad = PJ_TRUE;
}

void voxve_enable_stereo()
{
	voxve_var.channel_count = 2;
}

void voxve_disable_stereo()
{
	voxve_var.channel_count = 1;
}

/** Init media engine global data **/
static void default_setting()
{
	/* STUN, be aware STUN has limitation */
	voxve_var.is_enable_stun = PJ_FALSE;
	voxve_var.stun_status = PJ_EUNKNOWN;
	pj_bzero(voxve_var.stun_server_addr, sizeof(voxve_var.stun_server_addr));

	/* Set default sound dev */
	voxve_var.sound_rec_id = -1;
	voxve_var.sound_play_id = -1;

	/* VAD */
	voxve_var.is_no_vad = PJ_TRUE;

	/* Echo canceller */
	voxve_var.ec_tail_len = 200;

	/* CLOCK RATE */
	voxve_var.clock_rate = DEFAULT_CLOCK_RATE;
	voxve_var.snd_clock_rate = SND_DEFAULT_CLOCK_RATE;

	voxve_var.audio_frame_ptime = DEFAULT_AUDIO_FRAME_PTIME;
	voxve_var.channel_count = NCHANNELS;
}

voxve_status_t voxve_init(int month, int day, int year)
{
	pj_status_t status;

    /* Init PJLIB : */
    status = pj_init();
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);	

	/* Init PJ random */
	init_random_seed();

	/* Engine default setting */
	default_setting();

    /* Must create a pool factory before we can allocate any memory. */
	pj_caching_pool_init(&voxve_var.cp, &pj_pool_factory_default_policy, 0);

    /* Create memory pool for application purpose */
	voxve_var.pool = pj_pool_create( &voxve_var.cp.factory,	    /* pool factory	    */
			   "openmediaengine",								/* pool name.	    */
			   1000,											/* init size	    */
			   1000,											/* increment size	    */
			   NULL												/* callback on error    */
			   );

    /* 
     * Initialize media endpoint.
     * This will implicitly initialize PJMEDIA too.
     */
	status = pjmedia_endpt_create(&voxve_var.cp.factory, NULL, 1, &voxve_var.med_endpt);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

	/* Timer heap */
	status = timer_heap_init();
	PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

    /* Register all supported codecs */
    status = codecs_init(voxve_var.med_endpt);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

	/* Atomic Variables, available id */
	status = pj_atomic_create(voxve_var.pool, 0, &voxve_var.atomic_var);
	PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

	status = pj_rwmutex_create(voxve_var.pool, "channel_rwmutex", &voxve_var.activechannels_rwmutex);
	PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

	status = pj_rwmutex_create(voxve_var.pool, "conf_rwmutex", &voxve_var.activeconfs_rwmutex);
	PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

	return (status == PJ_SUCCESS) ? 0 : status;
}

voxve_status_t voxve_authenticate(char *auth_string, int len)
{
	return 0;
}

voxve_status_t voxve_shutdown()
{
	register_thread();

	/* Atomic Variables, available id */
    pj_atomic_destroy(voxve_var.atomic_var);

	/* Destroy stream */
	pj_rwmutex_lock_write(voxve_var.activechannels_rwmutex);
	std::map<int, channel_t*>::iterator iterator = voxve_var.activechannels.begin();
	while (iterator != voxve_var.activechannels.end()) 
	{
		channel_t * channel = (*iterator).second;

		/* Destroy sound device */
		if (channel->snd_port != NULL) 
		{
			snd_close(channel->snd_port);
			channel->snd_port = NULL;
		}

		if (channel->stream)
		{
			pjmedia_transport *tp;

			tp = pjmedia_stream_get_transport(channel->stream);
			pjmedia_stream_destroy(channel->stream);
			pjmedia_transport_close(tp);

			channel->stream = NULL;
		}

		delete channel;

		iterator++;
	}
	voxve_var.activechannels.clear();
	pj_rwmutex_unlock_write(voxve_var.activechannels_rwmutex);

	pj_rwmutex_destroy(voxve_var.activechannels_rwmutex);

	/* Destroy Conf */
	pj_rwmutex_lock_write(voxve_var.activeconfs_rwmutex);
	std::map<int, conf_t*>::iterator iterator2 = voxve_var.activeconfs.begin();
	while (iterator2 != voxve_var.activeconfs.end()) 
	{
		conf_t * conf = (*iterator2).second;

		if (conf->p_conf != NULL)
		{
			pjmedia_conf_destroy(conf->p_conf);
			conf->p_conf = NULL;
		}

		delete conf;

		iterator2++;
	}
	voxve_var.activeconfs.clear();
	pj_rwmutex_unlock_write(voxve_var.activeconfs_rwmutex);

	pj_rwmutex_destroy(voxve_var.activeconfs_rwmutex);

	codecs_deinit(voxve_var.med_endpt);

	/* Timer heap */
	timer_heap_destroy();

	/* Release application pool */
	pj_pool_release(voxve_var.pool);

    /* Destroy media endpoint. */
    pjmedia_endpt_destroy(voxve_var.med_endpt);

    /* Destroy pool factory */
    pj_caching_pool_destroy( &voxve_var.cp );

    /* Shutdown PJLIB */
    pj_shutdown();

	return 0;
}

void voxve_strerror(voxve_status_t statuscode, char *buf, int bufsize)
{
	register_thread();
	pj_strerror(statuscode, buf, bufsize);
}

voxve_status_t voxve_dtmf_dial(int channel_id, char *ascii_digit)
{
	register_thread();

	channel_t * channel = channel_find(channel_id);

	if (channel != NULL)
	{
		pj_str_t digits = pj_str(ascii_digit);
		return pjmedia_stream_dial_dtmf(channel->stream, &digits);
	}
	else
		return -1;
}
