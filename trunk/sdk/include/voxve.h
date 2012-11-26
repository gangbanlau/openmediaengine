/* 
 * Copyright (C) 2009-2011 Gang Liu <gangban.lau@gmail.com>
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

#ifndef VOXVE_H_
#define VOXVE_H_

#ifdef WIN32
#include <pstdint.h>
#else
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef WIN32
#ifdef OPENMEDIAENGINE_EXPORTS
	#define OPENMEDIAENGINE_DLL_API __declspec(dllexport)
#else
	#define OPENMEDIAENGINE_DLL_API __declspec(dllimport)
#endif
#else
	#define OPENMEDIAENGINE_DLL_API
#endif

typedef enum
{
	VOXVE_FALSE = 0,
	VOXVE_TRUE = 1
} voxve_bool_t;

/** Status code **/
typedef int voxve_status_t;

#define VOXVE_SUCCESS 	0

/**
 * Top most media type.
 */
typedef enum
{
    /** No type. */
    MEDIA_TYPE_NONE = 0,

    /** The media is audio */
    MEDIA_TYPE_AUDIO = 1,

    /** The media is video. */
    MEDIA_TYPE_VIDEO = 2,

	/** The media is image */
	MEDIA_TYPE_IMAGE = 3,

    /** Unknown media type, in this case the name will be specified in
     *  encoding_name.
     */
    MEDIA_TYPE_UNKNOWN = 4,

    /** The media is application. */
    MEDIA_TYPE_APPLICATION = 5

} voxve_media_type_t;

typedef struct
{
	voxve_media_type_t type;

	unsigned pt;
	char encoding_name[16];

	unsigned clock_rate;
	unsigned channel_cnt;
} voxve_codec_info_t;

/* Stream Direction */
typedef enum
{
	STREAM_DIR_NONE = 0,
	STREAM_DIR_ENCODING,
	STREAM_DIR_DECODING,
	STREAM_DIR_ENCODING_DECODING
} voxve_stream_dir_t;

typedef struct
{
	voxve_media_type_t type;

	voxve_stream_dir_t dir;

	char remote_ip[16];
	uint16_t remote_port;

	voxve_codec_info_t fmt;		/* Incoming codec format info. */
	uint16_t frm_ptime;
	uint16_t enc_ptime;			/* Encoder ptime, or zero if it's equal to decoder ptime */
	int tx_pt;				/* Outgoing codec paylaod type. */
	int rx_pt;				/* Incoming codec paylaod type. */
	int tx_event_pt;
	int rx_event_pt;
	uint32_t ssrc;
} voxve_stream_info_t;

typedef struct
{
	unsigned 	frame_size;
	unsigned 	min_prefetch;
	unsigned 	max_prefetch;
	unsigned 	burst;
	unsigned 	prefetch;
	unsigned 	size;
	unsigned 	avg_delay;
	unsigned 	min_delay;
	unsigned 	max_delay;
	unsigned 	dev_delay;
	unsigned 	avg_burst;
	unsigned 	lost;
	unsigned 	discard;
	unsigned 	empty;
} voxve_stream_stat_jbuf_t;

typedef struct
{
	char codec_info[64];

	char remote_addr[80];

	unsigned 	tx_pt;
	unsigned 	rx_pt;

	unsigned 	tx_ptime;

	voxve_stream_stat_jbuf_t jb_state;
} voxve_stream_stat_t;

/** Audio codec **/
typedef enum
{
  CODEC_PCMU = 0,
  CODEC_GSM = 3,
  CODEC_G723 = 4,
  CODEC_PCMA = 8,
  CODEC_G729 = 18
} voxve_codec_id_t;

/** Sound device information **/
typedef struct
{
    char	name[64];					/* Device name.		    */
    unsigned	input_count;	        /* Max number of input channels.  */
    unsigned	output_count;	        /* Max number of output channels. */
    unsigned	default_samples_per_sec;/* Default sampling rate.	    */
} voxve_snd_dev_info_t;

typedef enum
{
	PORT_NO_CHANGE, 	/* No change to the port TX or RX settings. */
	PORT_DISABLE, 		/* TX or RX is disabled from the port. It means get_frame() or put_frame() WILL NOT be called for this port. */
	PORT_MUTE, 			/* TX or RX is muted, which means that get_frame() or put_frame() will still be called, but the audio frame is discarded. */
	PORT_ENABLE 		/* Enable TX and RX to/from this port. */
} voxve_port_op_t;


/** ############# Initialization ################### **/

/** Init Media Engine **/
/** You need provide the expiry information here if you get a time limited copy **/
OPENMEDIAENGINE_DLL_API voxve_status_t voxve_init(int month, int day, int year);

/** Unlock this library after init **/
/** The password string should be embedded in the calling exe-file, and not stored in any resourcefile or registry key **/
OPENMEDIAENGINE_DLL_API voxve_status_t voxve_authenticate(char *auth_string, int len);



/** ############ NAT/TUNNEL/P2P Helpers ################# **/

/** Enable STUN support **/
OPENMEDIAENGINE_DLL_API voxve_status_t voxve_stun_enable(const char * stun_server_addr);

/** Get resolved public address via STUN after channel created **/
OPENMEDIAENGINE_DLL_API voxve_status_t voxve_stun_get_public_addr(int channel_id, char * addr_buf, unsigned buf_len, unsigned *port);



/** ############ Microphone/Speaker functions #### **/

/** Get count of sound devices **/ 
OPENMEDIAENGINE_DLL_API int voxve_snd_getcount();

/** Get sound device info, it is caller's duty to free this pointer */
OPENMEDIAENGINE_DLL_API voxve_snd_dev_info_t * voxve_snd_getinfo(int snd_dev_id);

/** Sets the sound device for channel capture and playback. If device is set to -1 the default device is used. **/
OPENMEDIAENGINE_DLL_API voxve_status_t voxve_snd_set(int waveindevice, int waveoutdevice);

/** Sets the speaker volume level. **/
//voxve_status_t voxve_snd_setspeakervolume(int level);

/** Returns the current speaker volume or -1 if an error occurred. **/
//int voxve_snd_getspeakervolume();

/** Sets the microphone volume level. **/
//voxve_status_t voxve_snd_setmicvolume(int level);

/** Returns the current microphone volume or -1 if an error occurred. **/
//int voxve_snd_getmicvolume();

/** Set AEC setting before opening sound port, value zero will disable echo canceller. **/
/** By default, echo canceller is enabled at 200 ms **/
OPENMEDIAENGINE_DLL_API void voxve_snd_set_ec(unsigned ec_tail_ms);

/** Set clock rate before opening sound device **/
/** Default values is 8000hz **/
/** Different clock rate between conference bridge, sound device and audio codec will cause resampling internal **/
OPENMEDIAENGINE_DLL_API void voxve_snd_set_clockrate(unsigned snd_clock_rate);



/** ############ Audio advanced setting ################## **/

/** Enable VAD **/
OPENMEDIAENGINE_DLL_API void voxve_enable_vad();

/** Disable VAD **/
OPENMEDIAENGINE_DLL_API void voxve_disable_vad();

/* Enable Stereo */
OPENMEDIAENGINE_DLL_API void voxve_enable_stereo();

/* Disable Stereo */
OPENMEDIAENGINE_DLL_API void voxve_disable_stereo();



/** ############ Channel functions ################ **/

/** Create new channel, return channel id or -1 if an error occurred **/
OPENMEDIAENGINE_DLL_API int voxve_channel_create(unsigned short local_port);
OPENMEDIAENGINE_DLL_API int voxve_channel_create2(const char* local_ip, unsigned short local_port);

/** Release a channel **/
OPENMEDIAENGINE_DLL_API voxve_status_t voxve_channel_delete(int channel_id);

/** Start streaming **/
OPENMEDIAENGINE_DLL_API voxve_status_t voxve_channel_startstream(int channel_id, voxve_codec_id_t codec,
		unsigned int ptime, const char * remote_ip, unsigned short remote_port, voxve_stream_dir_t dir);
OPENMEDIAENGINE_DLL_API voxve_status_t voxve_channel_startstream2(int channel_id, voxve_codec_id_t codec,
		unsigned int ptime, unsigned int rtp_ssrc, const char * remote_ip, unsigned short remote_port, voxve_stream_dir_t dir);
OPENMEDIAENGINE_DLL_API voxve_status_t voxve_channel_startstream3(int channel_id, const char* codec, int rtp_dynamic_payload,
		unsigned int ptime, int telephone_event_payload, unsigned int rtp_ssrc, const char * remote_ip, unsigned short remote_port, voxve_stream_dir_t dir);

/** Stop streaming **/
OPENMEDIAENGINE_DLL_API voxve_status_t voxve_channel_stopstream(int channel_id);
OPENMEDIAENGINE_DLL_API voxve_status_t voxve_channel_stopstream2(int channel_id, voxve_stream_stat_t *stat);

/** Modify current channel **/
OPENMEDIAENGINE_DLL_API voxve_status_t voxve_channel_update(int channel_id, voxve_codec_id_t codec,
		unsigned int ptime, unsigned short local_port, const char * remote_ip, unsigned short remote_port, voxve_stream_dir_t dir);

/** Connected to the sound device for that specific channel **/
OPENMEDIAENGINE_DLL_API voxve_status_t voxve_channel_startplayout(int channel_id);

/** Stops sending data from the specified channel to the sound device. However, packets are still received as long as the **/
/** Media Enigne is listening to the port! **/
OPENMEDIAENGINE_DLL_API voxve_status_t voxve_channel_stopplayout(int channel);

/** When enable is TRUE this call will stop playout and transmission on a temporary basis. It will not shut down the sockets **/
/** and not release the ports. The call is resumed again by calling the same call with enable set to FALSE **/
OPENMEDIAENGINE_DLL_API voxve_status_t voxve_channel_putonhold(int channel, voxve_bool_t enable);

/** Get maximum number of channels supported in this particular version of VoiceEngine.**/
OPENMEDIAENGINE_DLL_API int voxve_channel_getlimit();



/** ############ DTMF functions #################### **/

/** Transmit DTMF to this stream. The DTMF will be transmitted uisng RTP telephone-events as described in RFC 2833. **/
/** Currently the maximum number of digits are 32. **/
OPENMEDIAENGINE_DLL_API voxve_status_t voxve_dtmf_dial(int channel, const char *ascii_digit);



/** ############# Conference functions ############ **/

/** Set clock rate before opening conference bridge **/
/** Default values is 8000hz **/
/** Different clock rate between conference bridge, sound device and audio codec will cause resampling internal **/
OPENMEDIAENGINE_DLL_API void voxve_conf_set_clockrate(unsigned conf_clock_rate);

/** Create conference bridge, return bridge id or -1 if an error occurred **/
OPENMEDIAENGINE_DLL_API int voxve_conf_create();
OPENMEDIAENGINE_DLL_API int voxve_conf_create2(unsigned max_slots);

/** Release a bridge **/
OPENMEDIAENGINE_DLL_API voxve_status_t voxve_conf_destroy(int conf_id);

/** Make sure this channel not playing out, return channel slot or -1 if an error occurred **/
OPENMEDIAENGINE_DLL_API int voxve_conf_addchannel(int conf_id, int channel_id);

OPENMEDIAENGINE_DLL_API voxve_status_t voxve_conf_removechannel(int conf_id, int channel_id);

OPENMEDIAENGINE_DLL_API voxve_status_t voxve_conf_connect(int conf_id, unsigned source_slot, unsigned dst_slot);

OPENMEDIAENGINE_DLL_API voxve_status_t voxve_conf_disconnect(int conf_id, unsigned source_slot, unsigned dst_slot);

/** Select or change sound device **/
OPENMEDIAENGINE_DLL_API voxve_status_t voxve_conf_setsnddev(int conf_id, int cap_dev, int playback_dev);

/** Disconnect the main conference bridge from any sound devices **/
OPENMEDIAENGINE_DLL_API voxve_status_t voxve_conf_setnosnddev(int conf_id);

/** Get number of ports currently registered to the conference bridge. **/
OPENMEDIAENGINE_DLL_API int voxve_conf_getportcount(int conf_id);

/** Get total number of ports connections currently set up in the bridge **/
OPENMEDIAENGINE_DLL_API int voxve_conf_getconnectcount(int conf_id);

/** Get last signal level transmitted to or received from the specified port. This will retrieve the "real-time" signal level of the **/
/** audio as they are transmitted or received by the specified port. Application may call this function periodically to display the **/
/** signal level to a VU meter. **/
/** The signal level is an integer value in zero to 255, with zero indicates no signal, and 255 indicates the loudest signal level.**/
OPENMEDIAENGINE_DLL_API voxve_status_t voxve_conf_getsignallevel(int conf_id, unsigned slot, unsigned *tx_level, unsigned *rx_level);

/** Adjust the level of signal received from the specified port. Application may adjust the level to make signal received from the **/
/** port either louder or more quiet. The level adjustment is calculated with this formula: output = input * (adj_level+128) / 128. **/
/** Using this, zero indicates no adjustment, the value -128 will mute the signal, and the value of +128 will make the signal 100% **/
/** louder, +256 will make it 200% louder, etc. **/
/** The level adjustment value will stay with the port until the port is removed from the bridge or new adjustment value is set **/
OPENMEDIAENGINE_DLL_API voxve_status_t voxve_conf_adjustrxlevel(int conf_id, unsigned slot, int adj_level); 	

/** Adjust the level of signal to be transmitted to the specified port. Application may adjust the level to make signal transmitted **/
/** to the port either louder or more quiet. The level adjustment is calculated with this formula: output = input * (adj_level+128) / 128.**/
/** Using this, zero indicates no adjustment, the value -128 will mute the signal, and the value of +128 will make the signal 100% louder,**/
/** +256 will make it 200% louder, etc. **/
/** The level adjustment value will stay with the port until the port is removed from the bridge or new adjustment value is set. **/
OPENMEDIAENGINE_DLL_API voxve_status_t voxve_conf_adjusttxlevel(int conf_id, unsigned slot, int adj_level); 

/* Change TX and RX settings for the port. */
OPENMEDIAENGINE_DLL_API voxve_status_t voxve_conf_configureport(int conf_id, unsigned slot, voxve_port_op_t tx_op, voxve_port_op_t rx_op);



/** ############ Termination functions ############ **/

/** Shutdown Media Engine **/
OPENMEDIAENGINE_DLL_API voxve_status_t voxve_shutdown();



/** ############ Engine Information ############### **/

OPENMEDIAENGINE_DLL_API voxve_status_t voxve_getversion(char *version, int buflen);



/** ############# Error handling ################## **/

/** Get the error message for the specified error code. The message string will be NULL terminated. **/
OPENMEDIAENGINE_DLL_API void voxve_strerror(voxve_status_t statcode, char *buf, int bufsize);



/** ############# Debug Helpers ########################### **/

OPENMEDIAENGINE_DLL_API void voxve_dump();



#ifdef __cplusplus
}
#endif

#endif // _VOXVE_H_
