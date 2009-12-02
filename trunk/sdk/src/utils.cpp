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

#include "utils.h"

#include "global.h"

#include <list>

#define THIS_FILE	"utils.cpp"

extern struct voxve_data voxve_var;

/* 
 * Register all codecs. 
 */
pj_status_t codecs_init(pjmedia_endpt *med_endpt)
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

    return PJ_SUCCESS;
}

pj_status_t codecs_deinit(pjmedia_endpt *med_endpt)
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
pj_status_t stream_create( pj_pool_t *pool,
				  pjmedia_endpt *med_endpt,
				  const pjmedia_codec_info *codec_info,
				  unsigned int ptime,
				  unsigned int rtp_ssrc,
				  pjmedia_dir dir,
				  pjmedia_transport *transport,
				  const pj_sockaddr_in *rem_addr,
				  pjmedia_stream **p_stream)
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
	info.tx_event_pt = 101;											/* Remote support telephone-events RFC 2833, otherwise we can't use pjmedia_stream_dial_dtmf(), check PJSIP FAQ about DTMF */

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
    status = pjmedia_stream_create(med_endpt, pool, &info, transport, NULL, p_stream);

    if (status != PJ_SUCCESS) 
	{
		return status;
    }

    return PJ_SUCCESS;
}

typedef struct
{
	pj_thread_t *thread;
	pj_thread_desc desc;
} external_thread_t;

static std::list<external_thread_t *> registered_threads;

void register_thread()
{
	external_thread_t * e_thread = new external_thread_t;

	if(!pj_thread_is_registered()) 
	{

		pj_thread_register(NULL, e_thread->desc, &(e_thread->thread));

		PJ_LOG(4, (THIS_FILE, "Register external thread"));

		registered_threads.push_back(e_thread);
	}
	else
		delete e_thread;
}
