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
#include "stdlib.h"

#include "voxve.h"

#include <iostream>

using namespace std;

int main(int argc, char ** argv)
{
	voxve_status_t status;

	if (argc != 10)
	{
		cout << "Usage: " << argv[0] << " is_enable_stereo conf_clock_rate snd_clock_rate ec_tail_ms is_enable_vad codec_pt local_port remote_ip remote_port" << endl;
		return -1;
	}

	int stereo = atoi(argv[1]);
	int clock_rate = atoi(argv[2]);
	int snd_clock_rate = atoi(argv[3]);
	int ec_tail_ms = atoi(argv[4]);
	int vad = atoi(argv[5]);
	int codec_pt = atoi(argv[6]);
	int local_port = atoi(argv[7]);
	char * remote_ip = argv[8];
	int remote_port = atoi(argv[9]);

	voxve_codec_id_t codec = CODEC_PCMA;
	
	switch (codec_pt)
	{
	case 0:
		codec = CODEC_PCMU;
		break;
/*
	case 18:
		codec = CODEC_G729;
		break;
*/
	default:
		codec = CODEC_PCMA;
	}

	cout << "clock_rate " << clock_rate << endl;
	cout << "snd_clock_rate " << snd_clock_rate << endl;
	cout << "codec " << codec_pt << endl;
	cout << "local_port " << local_port << endl;
	cout << "remote_ip " << remote_ip << endl;
	cout << "remote_port " << remote_port << endl;

	status = voxve_init(0, 0, 0);
	if (status != 0)	return status;

	if (stereo)
	{
		voxve_enable_stereo();
		cout << "Stereo " << endl;
	}
	else {
		voxve_disable_stereo();
		cout << "Mono " << endl;
	}

	if (vad)
	{
		voxve_enable_vad();
		cout << "VAD enabled " << endl;
	}
	else
	{
		voxve_disable_vad();
		cout << "VAD disabled " << endl;
	}

	if (ec_tail_ms > 0)
	{
		voxve_snd_set_ec(ec_tail_ms);
		cout << "AEC enabled " << endl;
	}
	else {
		voxve_snd_set_ec(0);
		cout << "AEC disabled " << endl;
	}

	voxve_snd_set_clockrate(snd_clock_rate);
	
	voxve_conf_set_clockrate(clock_rate);

	cout << "Case: Create channel and delete channel" << endl;

	int channel_id = voxve_channel_create(local_port);

	char go;

	if (channel_id == -1)
		cout << "Create channel error" << endl;
	else {
		cin >> go;

		status = voxve_channel_delete(channel_id);
		if (status != 0)	return status;

		cin >> go;

		channel_id = voxve_channel_create(local_port);
		
		status = voxve_channel_startstream(channel_id, codec, 20, remote_ip, remote_port, STREAM_DIR_ENCODING_DECODING);
		if (status != 0)	return status;

		cin >> go;

		status = voxve_channel_stopstream(channel_id);
		if (status != 0)	return status;

		status = voxve_channel_startstream(channel_id, codec, 20, remote_ip, remote_port, STREAM_DIR_ENCODING_DECODING);
		if (status != 0)	return status;
		
		status = voxve_channel_startplayout(channel_id);
		if (status != 0)	return status;
		
		cin >> go;

		status = voxve_dtmf_dial(channel_id, "1234567890*#");
		if (status != 0)
		{
			char buf[256];
			voxve_strerror(status, buf, sizeof(buf) - 1);

			cout << buf << endl;

			return status;
		}

		cin >> go;

		status = voxve_channel_stopplayout(channel_id);
		if (status != 0)	return status;

		status = voxve_channel_startplayout(channel_id);
		if (status != 0)	return status;
		
		status = voxve_channel_putonhold(channel_id, true);
		if (status != 0)	return status;
		
		cin >> go;

		status = voxve_channel_putonhold(channel_id, false);
		if (status != 0)	return status;
		
		cin >> go;

		status = voxve_channel_update(channel_id, CODEC_PCMA, 20, 5002, "192.168.0.180", 40002, STREAM_DIR_ENCODING_DECODING);
		if (status != 0)	return status;

		cin >> go;

		status = voxve_channel_update(channel_id, CODEC_PCMU, 20, 5000, "192.168.0.180", 40000, STREAM_DIR_ENCODING_DECODING);
		if (status != 0)	return status;

		cin >> go;

		status = voxve_channel_stopplayout(channel_id);
		if (status != 0)	return status;

		cin >> go;

		status = voxve_channel_stopstream(channel_id);
		if (status != 0)	return status;

		cin >> go;

		status = voxve_channel_delete(channel_id);
		if (status != 0)	return status;

		cin >> go;
	}

	status = voxve_shutdown();
	if (status != 0)	return status;
	
	cout << "Done" << endl;

	return 0;
}
