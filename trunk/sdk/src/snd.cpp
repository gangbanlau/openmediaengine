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
#include "snd.h"

#include "utils.h"

#define THIS_FILE "snd.cpp"

extern struct voxve_data voxve_var;

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

int voxve_snd_getcount()
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

/* Close existing sound device */
void snd_close(pjmedia_snd_port *snd_port)
{
    /* Close sound device */
    if (snd_port != NULL) 
	{
		pjmedia_snd_port_disconnect(snd_port);
		pjmedia_snd_port_destroy(snd_port);
    }
}
