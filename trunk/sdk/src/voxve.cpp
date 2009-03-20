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
#include <pjlib.h>
#include <pjlib-util.h>
#include <pjmedia.h>
#include <pjmedia-codec.h>

#include <map>

#include "voxve.h"
#include "utils.h"

#define THIS_FILE	"voxve.cpp"


/* Conf bridge Constants */
#define DEFAULT_CLOCK_RATE	8000		/* The default clock rate to be used by the conference bridge */
#define DEFAULT_AUDIO_FRAME_PTIME	10	/* Default frame length in the conference bridge. */
#define NCHANNELS	1
#define NSAMPLES	(DEFAULT_CLOCK_RATE * DEFAULT_AUDIO_FRAME_PTIME / 1000 * NCHANNELS)
#define NBITS		16

#define CONF_MAX_SLOTS	10

/* Sound devices */
#define SND_DEFAULT_CLOCK_RATE	8000	/* The default clock rate to be used by sound devices */
#define SND_NSAMPLES	(SND_DEFAULT_CLOCK_RATE * DEFAULT_AUDIO_FRAME_PTIME / 1000 * NCHANNELS)

#define SND_DEFAULT_REC_LATENCY		100
#define SND_DEFAULT_PLAY_LATENCY	100

/* TODO Windows Vista Issue */
/* if SND_DEFAULT_REC_LATENCY 40 and SND_DEFAULT_PLAY_LATENCY 40, then Voice playback choppy */
/* if DEFAULT_CLOCK_RATE 8000 and SND_DEFAULT_CLOCK_RATE 44000, then Voice playback no sound */

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

void voxve_snd_set_ec(unsigned tail_ms)
{
	voxve_var.ec_tail_len = tail_ms;
}

void voxve_snd_set_clockrate(unsigned snd_clock_rate)
{
	if (snd_clock_rate < 8000 || snd_clock_rate > 192000) 
	{
		printf("Error: expecting value between 8000-192000 for sound device clock rate\r\n");
		return;
	}

	voxve_var.snd_clock_rate = snd_clock_rate;

	return;
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

voxve_status_t voxve_init(int month, int day, int year)
{
	pj_status_t status;

    /* init PJLIB : */
    status = pj_init();
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);	

	/* init PJ random */
	init_random_seed();

	/** init global data **/
	/* set default snd dev */
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

    /* Must create a pool factory before we can allocate any memory. */
	pj_caching_pool_init(&voxve_var.cp, &pj_pool_factory_default_policy, 0);

    /* 
     * Initialize media endpoint.
     * This will implicitly initialize PJMEDIA too.
     */
	status = pjmedia_endpt_create(&voxve_var.cp.factory, NULL, 1, &voxve_var.med_endpt);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

    /* Create memory pool for application purpose */
	voxve_var.pool = pj_pool_create( &voxve_var.cp.factory,	    /* pool factory	    */
			   "openmediaengine",	    /* pool name.	    */
			   4000,	    /* init size	    */
			   4000,	    /* increment size	    */
			   NULL		    /* callback on error    */
			   );

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

int voxve_snd_set(int waveindevice, int waveoutdevice)
{
	register_thread();

	if (waveindevice < 0 && waveindevice != -1)
		return -1;

	if (waveoutdevice < 0 && waveoutdevice != -1)
		return -1;

	const pjmedia_snd_dev_info *info_in;

	info_in = pjmedia_snd_get_dev_info(waveindevice);
	if (info_in == NULL)
		return -1;
	else {
		if (info_in->input_count <= 0)
			return -1;
	}

	const pjmedia_snd_dev_info *info_out;

	info_out = pjmedia_snd_get_dev_info(waveoutdevice);
	if (info_out == NULL)
		return -1;
	else {
		if (info_out->output_count <= 0)
			return -1;
	}

	voxve_var.sound_rec_id = waveindevice;
	voxve_var.sound_play_id = waveoutdevice;

	return 0;
}

/** Sets the speaker volume level. **/
voxve_status_t voxve_snd_setspeakervolume(int level) 
{
	return -1;
}

/** Returns the current speaker volume or ¨C1 if an error occurred. **/
int voxve_snd_getspeakervolume()
{
	return -1;
}

/** Sets the microphone volume level. **/
voxve_status_t voxve_snd_setmicvolume(int level)
{
	return -1;
}

/** Returns the current microphone volume or ¨C1 if an error occurred. **/
int voxve_snd_getmicvolume()
{
	return -1;
}

static int voxve_channel_internalcreate(voxve_channel_t * channel, unsigned short local_port)
{
	/* Create media transport */
	pjmedia_transport *transport = NULL;
    pj_status_t status = pjmedia_transport_udp_create(voxve_var.med_endpt, NULL, local_port, 0, &transport);
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

static voxve_status_t voxve_channel_connectnullsnd(voxve_channel_t * channel)
{
	/* Get the port interface of the stream */
	pjmedia_port *stream_port = NULL;
	pj_status_t status = pjmedia_stream_get_port(channel->stream, &stream_port);
	PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

	/* Create Null Snd */
	pjmedia_port *null_snd = NULL;
	status = pjmedia_null_port_create(voxve_var.pool, 
				stream_port->info.clock_rate, stream_port->info.channel_count, 
				stream_port->info.samples_per_frame, stream_port->info.bits_per_sample, 
				&null_snd);
	PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);
		
	pjmedia_master_port *master_port = NULL;
	status = pjmedia_master_port_create(voxve_var.pool, null_snd, stream_port, 0, &master_port);
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

static voxve_status_t voxve_channel_disconnectnullsnd(voxve_channel_t * channel)
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

int voxve_channel_create(unsigned short local_port)
{
	register_thread();

	int channel_id = getavailableid(voxve_var.atomic_var);

	voxve_channel_t * channel = new voxve_channel_t;
	channel->id = channel_id;

	int rtCode = voxve_channel_internalcreate(channel, local_port);

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

	voxve_channel_t * channel = channel_find(channel_id);

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
	else {
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
		return voxve_channel_connectnullsnd(channel);
	}

}

voxve_status_t voxve_channel_stopstream(int channel_id)
{
	register_thread();

	voxve_channel_t *channel = channel_find(channel_id);

	if (channel == NULL)
		return -1;

	/* Destroy Null Snd first, it will stop provide clocking timing */
	/* Otherwise may be crashed at stream put_frame() or get_freame() calling */
	voxve_channel_disconnectnullsnd(channel);

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
	voxve_channel_t * channel = NULL;

	pj_rwmutex_lock_write(voxve_var.activechannels_rwmutex);
	std::map<int, voxve_channel_t*>::iterator iter = voxve_var.activechannels.find(channelid);
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

		voxve_channel_disconnectnullsnd(channel);

		/* Destroy stream */
		if (channel->stream) {
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

	voxve_channel_t * channel = channel_find(channelid);
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

		status = pjmedia_snd_port_create(voxve_var.pool, voxve_var.sound_rec_id, voxve_var.sound_play_id, 
					stream_port->info.clock_rate,
					stream_port->info.channel_count,
					stream_port->info.samples_per_frame,
					stream_port->info.bits_per_sample,
					0, &snd_port);
		PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

		/* Set AEC */
		pjmedia_snd_port_set_ec(snd_port, voxve_var.pool, voxve_var.ec_tail_len, 0);

		voxve_channel_disconnectnullsnd(channel);

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

	voxve_channel_t * channel = channel_find(channelid);

	if (channel != NULL)
	{
		if (channel->snd_port != NULL)
		{
			snd_close(channel->snd_port);
			channel->snd_port = NULL;
		}

		return voxve_channel_connectnullsnd(channel);
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
	else {
		/* Unhold */
		status = voxve_channel_startplayout(channelid);
		PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

		return 0;
	}
}

voxve_status_t voxve_channel_update(int channel_id, voxve_codec_id_t codec, unsigned int ptime, unsigned short local_port, char * remote_ip, unsigned short remote_port, voxve_stream_dir dir)
{
	register_thread();

	voxve_channel_t * channel = channel_find(channel_id);
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

	voxve_channel_disconnectnullsnd(channel);

	/* Destroy stream */
	if (channel->stream) {
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

	int result = voxve_channel_internalcreate(channel, local_port);

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

voxve_status_t voxve_shutdown()
{
	register_thread();

	/* Atomic Variables, available id */
    pj_atomic_destroy(voxve_var.atomic_var);

	/* Destroy stream */
	pj_rwmutex_lock_write(voxve_var.activechannels_rwmutex);
	std::map<int, voxve_channel_t*>::iterator iterator = voxve_var.activechannels.begin();
	while (iterator != voxve_var.activechannels.end()) 
	{
		voxve_channel_t * channel = (*iterator).second;

		/* Destroy sound device */
		if (channel->snd_port != NULL) 
		{
			snd_close(channel->snd_port);
			channel->snd_port = NULL;
		}

		if (channel->stream) {
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
	std::map<int, voxve_conf_t*>::iterator iterator2 = voxve_var.activeconfs.begin();
	while (iterator2 != voxve_var.activeconfs.end()) 
	{
		voxve_conf_t * conf = (*iterator2).second;

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

int voxvx_snd_getcount()
{
	register_thread();
	return pjmedia_snd_get_dev_count();
}

voxve_snd_dev_info_t * voxve_snd_getinfo(int snd_dev_id)
{
	register_thread();

	if (snd_dev_id < 0 && snd_dev_id != -1)
		return NULL;

	const pjmedia_snd_dev_info *info;

	info = pjmedia_snd_get_dev_info(snd_dev_id);

	if (info == NULL)
		return NULL;

	voxve_snd_dev_info_t * sndinfo = (voxve_snd_dev_info_t *)malloc(sizeof(voxve_snd_dev_info_t));
	pj_bzero(sndinfo, sizeof(voxve_snd_dev_info_t));
	sndinfo->input_count = info->input_count;
	sndinfo->output_count = info->output_count;
	sndinfo->default_samples_per_sec = info->default_samples_per_sec;
	strcpy(sndinfo->name, info->name);

	return sndinfo;
}

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

	voxve_conf_t * conf = new voxve_conf_t;

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

	voxve_conf_t * conf = NULL;
//	pj_status_t status;

	pj_rwmutex_lock_write(voxve_var.activeconfs_rwmutex);
	std::map<int, voxve_conf_t*>::iterator iter = voxve_var.activeconfs.find(conf_id);
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

	voxve_channel_t * channel = channel_find(channel_id);

	if (channel == NULL)
	{
		PJ_LOG(3, (THIS_FILE, "Can't find channel, id %d", channel_id));
		return -1;
	}

	voxve_conf_t * conf = conf_find(conf_id);

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
	status = voxve_channel_disconnectnullsnd(channel);
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

	voxve_conf_t * conf = conf_find(conf_id);

	if (conf == NULL)
	{
		PJ_LOG(3, (THIS_FILE, "Can't find conference bridge, id %d", conf_id));
		return -1;
	}

	voxve_channel_t * channel = channel_find(channel_id);

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

	return voxve_channel_connectnullsnd(channel);
}

voxve_status_t voxve_conf_connect(int conf_id, unsigned source_slot, unsigned dst_slot)
{
	register_thread();

	voxve_conf_t * conf = conf_find(conf_id);

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

	voxve_conf_t * conf = conf_find(conf_id);

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

	voxve_conf_t * conf = conf_find(conf_id);

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
	else {
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

	voxve_conf_t * conf = conf_find(conf_id);

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

	voxve_conf_t * conf = conf_find(conf_id);

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

	voxve_conf_t * conf = conf_find(conf_id);

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

	voxve_conf_t * conf = conf_find(conf_id);

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

	voxve_conf_t * conf = conf_find(conf_id);

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

	voxve_conf_t * conf = conf_find(conf_id);

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

	voxve_conf_t * conf = conf_find(conf_id);

	if (conf != NULL)
	{
		return pjmedia_conf_configure_port(conf->p_conf, slot, t_op, r_op);
	}
	else
		return -1;
}

voxve_status_t voxve_dtmf_dial(int channel_id, char *ascii_digit)
{
	register_thread();

	voxve_channel_t * channel = channel_find(channel_id);

	if (channel != NULL)
	{
		pj_str_t digits = pj_str(ascii_digit);
		return pjmedia_stream_dial_dtmf(channel->stream, &digits);
	}
	else
		return -1;
}
