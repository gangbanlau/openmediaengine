#ifndef _VOXVE_H_
#define _VOXVE_H_

#ifdef __cplusplus
extern "C"
{
#endif

/** Status code **/
typedef int voxve_status_t;

#define VOXVE_SUCCESS 0

/* Stream Direction */
typedef enum voxve_stream_dir 
{
	STREAM_DIR_NONE = 0,
	STREAM_DIR_ENCODING,
	STREAM_DIR_DECODING,
	STREAM_DIR_ENCODING_DECODING
} voxve_stream_dir;

/** Audio codec **/
typedef enum voxve_codec_id 
{
  CODEC_PCMU = 0,
  CODEC_GSM = 3,
  CODEC_G723 = 4,
  CODEC_PCMA = 8,
  CODEC_G729 = 18
} voxve_codec_id_t;

/** Sound device information **/
typedef struct voxve_snd_dev_info
{
    char	name[64];					/* Device name.		    */
    unsigned	input_count;	        /* Max number of input channels.  */
    unsigned	output_count;	        /* Max number of output channels. */
    unsigned	default_samples_per_sec;/* Default sampling rate.	    */
} voxve_snd_dev_info_t;

enum voxve_port_op
{
	PORT_NO_CHANGE, 	/* No change to the port TX or RX settings. */
	PORT_DISABLE, 		/* TX or RX is disabled from the port. It means get_frame() or put_frame() WILL NOT be called for this port. */
	PORT_MUTE, 			/* TX or RX is muted, which means that get_frame() or put_frame() will still be called, but the audio frame is discarded. */
	PORT_ENABLE 		/* Enable TX and RX to/from this port. */
};


/** ############# Initialization ################### **/

/** Init Voice Engine **/
/** You need provide the expiry information here if you get a time limited copy **/
voxve_status_t voxve_init(int month = 0, int day = 0, int year = 0);

/** Unlock this library after init **/
/** The password string should be embedded in the calling exe-file, and not stored in any resourcefile or registry key **/
voxve_status_t voxve_authenticate(char *auth_string, int len);



/** ############ Mic/Speaker functions #### **/

/** Get count of sound devices **/ 
int voxvx_snd_getcount();

/** Get sound device info, it is caller's duty to free this pointer */
voxve_snd_dev_info_t * voxve_snd_getinfo(int snd_dev_id);

/** Sets the sound device for channel capture and playback. If device is set to 每1 the default device is used. **/
int voxve_snd_set(int waveindevice, int waveoutdevice);

/** Sets the speaker volume level. **/
//voxve_status_t voxve_snd_setspeakervolume(int level);

/** Returns the current speaker volume or 每1 if an error occurred. **/
//int voxve_snd_getspeakervolume();

/** Sets the microphone volume level. **/
//voxve_status_t voxve_snd_setmicvolume(int level);

/** Returns the current microphone volume or 每1 if an error occurred. **/
//int voxve_snd_getmicvolume();

/** Set AEC setting before opening sound port, value zero will disable echo canceller. **/
/** By default, echo canceller is enabled. 200 ms **/
void voxve_snd_set_ec(unsigned ec_tail_ms);

/** Set clock rate before opening sound device **/
/** Default valus is 8000hz **/
/** Different clock rate between conference bridge, sound device and audio codec will cause resampling internal **/
void voxve_snd_set_clockrate(unsigned snd_clock_rate);



/** ############ Audio functions ################## **/

/** Enable VAD **/
void voxve_enable_vad();

/** Disable VAD **/
void voxve_disable_vad();

/* Stereo */
void voxve_enable_stereo();

/* Mono */
void voxve_disable_stereo();



/** ############ Channel functions ################ **/

/** Create new channel, return channel id or 每1 if an error occurred **/
int voxve_channel_create(unsigned short local_port);

voxve_status_t voxve_channel_delete(int channel);

/** Start streaming **/
voxve_status_t voxve_channel_startstream(int channel_id, voxve_codec_id_t codec, unsigned int ptime, char * remote_ip, unsigned short remote_port, voxve_stream_dir dir);
voxve_status_t voxve_channel_startstream2(int channel_id, voxve_codec_id_t codec, unsigned int ptime, unsigned int rtp_ssrc, char * remote_ip, unsigned short remote_port, voxve_stream_dir dir);

voxve_status_t voxve_channel_stopstream(int channel_id);

/** Modify current channel **/
voxve_status_t voxve_channel_update(int channel, voxve_codec_id_t codec, unsigned int ptime, unsigned short local_port, char * remote_ip, unsigned short remote_port, voxve_stream_dir dir);

/** Connected to the soundcard for that specific channel **/
voxve_status_t voxve_channel_startplayout(int channel);

/** Stops sending data from the specified channel to the soundcard. However, packets are still received as long as the **/
/** VoiceEnigne is listening to the port! **/
voxve_status_t voxve_channel_stopplayout(int channel);

/** When enable is TRUE this call will stop playout and transmission on a temporary basis. It will not shut down the sockets **/
/** and not release the ports. The call is resumed again by calling the same call with enable set to FALSE **/
voxve_status_t voxve_channel_putonhold(int channel, bool enable);

/** Get maximum number of channels supported in this particular version of VoiceEngine.**/
int voxve_channel_getlimit();



/** ############ DTMF functions #################### **/

/** Transmit DTMF to this stream. The DTMF will be transmitted uisng RTP telephone-events as described in RFC 2833. **/
/** Currently the maximum number of digits are 32. **/
voxve_status_t voxve_dtmf_dial(int channel, char *ascii_digit);



/** ############# Conference functions ############ **/

/** Set clock rate before opening conference bridge **/
/** Default valus is 8000hz **/
/** Different clock rate between conference bridge, sound device and audio codec will cause resampling internal **/
void voxve_conf_set_clockrate(unsigned conf_clock_rate);

/** Create conference bridge, return bridge id or 每1 if an error occurred **/
int voxve_conf_create();

voxve_status_t voxve_conf_destroy(int conf_id);

/** Make sure this channel not playing out, return channel slot or -1 if an error occurred **/
int voxve_conf_addchannel(int conf_id, int channel_id);

voxve_status_t voxve_conf_removechannel(int conf_id, int channel_id);

voxve_status_t voxve_conf_connect(int conf_id, unsigned source_slot, unsigned dst_slot);

voxve_status_t voxve_conf_disconnect(int conf_id, unsigned source_slot, unsigned dst_slot);

/** Select or change sound device **/
voxve_status_t voxve_conf_setsnddev(int conf_id, int cap_dev, int playback_dev);

/** Disconnect the main conference bridge from any sound devices **/
voxve_status_t voxve_conf_setnosnddev(int conf_id);

/** Get number of ports currently registered to the conference bridge. **/
int voxve_conf_getportcount(int conf_id);

/** Get total number of ports connections currently set up in the bridge **/
int voxve_conf_getconnectcount(int conf_id);

/** Get last signal level transmitted to or received from the specified port. This will retrieve the "real-time" signal level of the **/
/** audio as they are transmitted or received by the specified port. Application may call this function periodically to display the **/
/** signal level to a VU meter. **/
/** The signal level is an integer value in zero to 255, with zero indicates no signal, and 255 indicates the loudest signal level.**/
voxve_status_t voxve_conf_getsignallevel(int conf_id, unsigned slot, unsigned *tx_level, unsigned *rx_level);

/** Adjust the level of signal received from the specified port. Application may adjust the level to make signal received from the **/
/** port either louder or more quiet. The level adjustment is calculated with this formula: output = input * (adj_level+128) / 128. **/
/** Using this, zero indicates no adjustment, the value -128 will mute the signal, and the value of +128 will make the signal 100% **/
/** louder, +256 will make it 200% louder, etc. **/
/** The level adjustment value will stay with the port until the port is removed from the bridge or new adjustment value is set **/
voxve_status_t voxve_conf_adjustrxlevel(int conf_id, unsigned slot, int adj_level); 	

/** Adjust the level of signal to be transmitted to the specified port. Application may adjust the level to make signal transmitted **/
/** to the port either louder or more quiet. The level adjustment is calculated with this formula: output = input * (adj_level+128) / 128.**/
/** Using this, zero indicates no adjustment, the value -128 will mute the signal, and the value of +128 will make the signal 100% louder,**/
/** +256 will make it 200% louder, etc. **/
/** The level adjustment value will stay with the port until the port is removed from the bridge or new adjustment value is set. **/
voxve_status_t voxve_conf_adjusttxlevel(int conf_id, unsigned slot, int adj_level); 

/* Change TX and RX settings for the port. */
voxve_status_t voxve_conf_configureport(int conf_id, unsigned slot, voxve_port_op tx_op, voxve_port_op rx_op);



/** ############ Termination functions ############ **/

/** Shutdown Voice Engine **/
voxve_status_t voxve_shutdown();



/** ############ Engine Information ############### **/

voxve_status_t voxve_getversion(char *version, int buflen);



/** ############# Error handling ################## **/

/** Get the error message for the specified error code. The message string will be NULL terminated. **/
void voxve_strerror(voxve_status_t statcode, char *buf, int bufsize);




#ifdef __cplusplus
}
#endif

#endif // _VOXVE_H_
