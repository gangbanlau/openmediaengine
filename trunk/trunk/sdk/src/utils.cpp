#include "utils.h"

#include "g729.h"

#include <list>

#define THIS_FILE	"utils.cpp"

extern struct voxve_data voxve_var;

/* 
 * Register all codecs. 
 */
pj_status_t init_codecs(pjmedia_endpt *med_endpt)
{
    pj_status_t status;

    /* To suppress warning about unused var when all codecs are disabled */
    PJ_UNUSED_ARG(status);

#if defined(PJMEDIA_HAS_G711_CODEC) && PJMEDIA_HAS_G711_CODEC!=0
    status = pjmedia_codec_g711_init(med_endpt);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);
#endif

#if defined(PJMEDIA_HAS_GSM_CODEC) && PJMEDIA_HAS_GSM_CODEC!=0
    status = pjmedia_codec_gsm_init(med_endpt);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);
#endif

#if defined(PJMEDIA_HAS_SPEEX_CODEC) && PJMEDIA_HAS_SPEEX_CODEC!=0
    status = pjmedia_codec_speex_init(med_endpt, 0, -1, -1);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);
#endif

#if defined(PJMEDIA_HAS_L16_CODEC) && PJMEDIA_HAS_L16_CODEC!=0
    status = pjmedia_codec_l16_init(med_endpt, 0);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);
#endif

	/* IPP G729 */
#ifdef HAS_IPP 
	status = pjmedia_codec_ipp_g729_init(med_endpt);
	PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);
#endif

    return PJ_SUCCESS;
}

pj_status_t deinit_codecs(pjmedia_endpt *med_endpt)
{
    pj_status_t status;

    /* To suppress warning about unused var when all codecs are disabled */
    PJ_UNUSED_ARG(status);

#if defined(PJMEDIA_HAS_G711_CODEC) && PJMEDIA_HAS_G711_CODEC!=0
    status = pjmedia_codec_g711_deinit();
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);
#endif

#if defined(PJMEDIA_HAS_GSM_CODEC) && PJMEDIA_HAS_GSM_CODEC!=0
    status = pjmedia_codec_gsm_deinit();
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);
#endif

#if defined(PJMEDIA_HAS_SPEEX_CODEC) && PJMEDIA_HAS_SPEEX_CODEC!=0
    status = pjmedia_codec_speex_deinit();
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);
#endif

#if defined(PJMEDIA_HAS_L16_CODEC) && PJMEDIA_HAS_L16_CODEC!=0
    status = pjmedia_codec_l16_deinit();
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);
#endif

	/* IPP G729 */
#ifdef HAS_IPP
	status = pjmedia_codec_ipp_g729_deinit();
	PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);
#endif

    return PJ_SUCCESS;
}

/* Get available id */
pj_atomic_value_t getavailableid(pj_atomic_t * atomic_var)
{
	return pj_atomic_inc_and_get(atomic_var);
}

/* 
 * Create stream based on the codec, dir, remote address, etc. 
 */
pj_status_t create_stream( pj_pool_t *pool,
				  pjmedia_endpt *med_endpt,
				  const pjmedia_codec_info *codec_info,
				  unsigned int ptime,
				  unsigned int rtp_ssrc,
				  pjmedia_dir dir,
				  pjmedia_transport *transport,
				  const pj_sockaddr_in *rem_addr,
				  pjmedia_stream **p_stream )
{
    pjmedia_stream_info info;
    pj_status_t status;

    /* Reset stream info. */
    pj_bzero(&info, sizeof(info));

    /* Initialize stream info formats */
    info.type = PJMEDIA_TYPE_AUDIO;									/* Media type */
    info.dir = dir;													/* Media direction. */
    pj_memcpy(&info.fmt, codec_info, sizeof(pjmedia_codec_info));	/* Incoming codec format info. */
    info.tx_pt = codec_info->pt;									/* Outgoing codec paylaod type. */
	if (rtp_ssrc == 0)
		info.ssrc = pj_rand();										/* RTP SSRC. */
	else
		info.ssrc = rtp_ssrc;
	info.tx_event_pt = 101;											/* Remote support RFC 2833, otherwise we can't use pjmedia_stream_dial_dtmf(), check PJSIP FAQ about DTMF */

	pjmedia_codec_param param;
	pjmedia_codec_mgr *codec_mgr = pjmedia_endpt_get_codec_mgr(voxve_var.med_endpt);
	status = pjmedia_codec_mgr_get_default_param(codec_mgr, codec_info, &param);
	if (status != PJ_SUCCESS)
	{
		return status;
	}

	if (voxve_var.is_no_vad)
		param.setting.vad = 0;
	else
		param.setting.vad = 1;

	param.setting.frm_per_pkt = (pj_uint8_t)(ptime / param.info.frm_ptime);

	info.param = &param;

	/*
	si->jb_init = pjsua_var.media_cfg.jb_init;
	si->jb_min_pre = pjsua_var.media_cfg.jb_min_pre;
	si->jb_max_pre = pjsua_var.media_cfg.jb_max_pre;
	si->jb_max = pjsua_var.media_cfg.jb_max;
	*/

    /* Copy remote address */
    pj_memcpy(&info.rem_addr, rem_addr, sizeof(pj_sockaddr_in));	/* Remote RTP address  */

    /* Now that the stream info is initialized, we can create the 
     * stream.
     */
    status = pjmedia_stream_create( med_endpt, pool, &info, transport, NULL, p_stream);

    if (status != PJ_SUCCESS) {
		return status;
    }

    return PJ_SUCCESS;
}

/* Set default logging cfg */
void logging_config_default(voxve_logging_config *cfg)
{
    pj_bzero(cfg, sizeof(*cfg));

    cfg->level = 5;
    cfg->console_level = 4;
    cfg->decor = PJ_LOG_HAS_SENDER | PJ_LOG_HAS_TIME | PJ_LOG_HAS_MICRO_SEC | PJ_LOG_HAS_NEWLINE;
}

static void logging_config_dup(pj_pool_t *pool, voxve_logging_config *dst, const voxve_logging_config *src)
{
    pj_memcpy(dst, src, sizeof(*src));
    pj_strdup_with_null(voxve_var.pool, &dst->log_filename, &src->log_filename);
}

/* Log callback */
static void log_writer(int level, const char *buffer, int len)
{
    /* Write to file, stdout or application callback. */

    if (voxve_var.log_file) {
		pj_ssize_t size = len;
		pj_file_write(voxve_var.log_file, buffer, &size);
		/* This will slow things down considerably! Don't do it!
		pj_file_flush(pjsua_var.log_file);
		*/
    }

    if (level <= (int)voxve_var.log_cfg.console_level) {
		if (voxve_var.log_cfg.cb)
			(*voxve_var.log_cfg.cb)(level, buffer, len);
		else
			pj_log_write(level, buffer, len);
    }
}

void logging(const char *sender, VOXVE_LOG_LEVEL log_level, const char *title, pj_status_t status)
{
    char msg[PJ_ERR_MSG_SIZE];

    pj_strerror(status, msg, sizeof(msg));

	switch(log_level)
	{
	case VOXVE_LOG_ERR:
		PJ_LOG(1,(sender, "%s: %s [status=%d]", title, msg, status));
		break;
	case VOXVE_LOG_WARN:
		PJ_LOG(2,(sender, "%s: %s [status=%d]", title, msg, status));
		break;
	case VOXVE_LOG_INFO:
		PJ_LOG(3,(sender, "%s: %s [status=%d]", title, msg, status));
		break;
	case VOXVE_LOG_DEBUG:
		PJ_LOG(4,(sender, "%s: %s [status=%d]", title, msg, status));
		break;
	case VOXVE_LOG_TRACE:
		PJ_LOG(5,(sender, "%s: %s [status=%d]", title, msg, status));
		break;
	default:
		PJ_LOG(5,(sender, "%s: %s [status=%d]", title, msg, status));
	}
}

/*
 * Application can call this function at any time to change logging settings.
 */
pj_status_t logging_reconfigure(const voxve_logging_config_t *cfg)
{
    pj_status_t status;

    /* Save config. */
    logging_config_dup(voxve_var.pool, &voxve_var.log_cfg, cfg);

    /* Redirect log function to ours */
    pj_log_set_log_func(&log_writer);

    /* Set decor */
    pj_log_set_decor(voxve_var.log_cfg.decor);

    /* Close existing file, if any */
    if (voxve_var.log_file) {
		pj_file_close(voxve_var.log_file);
		voxve_var.log_file = NULL;
    }

    /* If output log file is desired, create the file: */
    if (voxve_var.log_cfg.log_filename.slen) {
		status = pj_file_open(voxve_var.pool, 
			      voxve_var.log_cfg.log_filename.ptr,
			      PJ_O_WRONLY, 
			      &voxve_var.log_file);

		if (status != PJ_SUCCESS) {
			logging(THIS_FILE, VOXVE_LOG_WARN, "Create log file", status);
			return status;
		}
    }

    return PJ_SUCCESS;
}

/* Close existing sound device */
void close_snd_dev(pjmedia_snd_port *snd_port)
{
    /* Close sound device */
    if (snd_port != NULL) {
		pjmedia_snd_port_disconnect(snd_port);
		pjmedia_snd_port_destroy(snd_port);
    }
}

voxve_conf_t * find_conf_bridge(int conf_id)
{
	voxve_conf_t * conf = NULL;
	pj_rwmutex_lock_read(voxve_var.activeconfs_rwmutex);
	std::map<int, voxve_conf_t*>::iterator iter2 = voxve_var.activeconfs.find(conf_id);

	if (iter2 != voxve_var.activeconfs.end())
	{
		conf = (*iter2).second;
	}
	pj_rwmutex_unlock_read(voxve_var.activeconfs_rwmutex);

	return conf;
}

voxve_channel_t * find_channel(int channel_id)
{
	voxve_channel_t * channel = NULL;
//	pj_status_t status; 

	pj_rwmutex_lock_read(voxve_var.activechannels_rwmutex);
	std::map<int, voxve_channel_t*>::iterator iter = voxve_var.activechannels.find(channel_id);

	if (iter != voxve_var.activechannels.end())
	{
		channel = (*iter).second;
	}
	pj_rwmutex_unlock_read(voxve_var.activechannels_rwmutex);

	return channel;
}

typedef struct voxve_external_thread
{
	pj_thread_t *thread;
	pj_thread_desc desc;
} voxve_external_thread;

static std::list<voxve_external_thread *> registered_threads;

//static pj_thread_desc desc;
//static pj_thread_t *  thread;

void register_thread()
{
	voxve_external_thread * e_thread = new voxve_external_thread;

	if(!pj_thread_is_registered()) {

		pj_thread_register(NULL, e_thread->desc, &(e_thread->thread));

//		pj_thread_register(NULL, desc, &thread);

		PJ_LOG(4, (THIS_FILE, "Register external thread into VE"));

		registered_threads.push_back(e_thread);
	}
	else
		delete e_thread;
}