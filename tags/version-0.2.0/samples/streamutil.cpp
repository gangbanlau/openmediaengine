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
#include <iostream>
using namespace std;

#include <gflags/gflags.h>

DEFINE_bool(stereo, false, "Audio device and conference bridge opened in stereo mode");
DEFINE_int32(clock_rate, 16000, "Override conference bridge clock rate");
DEFINE_int32(snd_clock_rate, 16000, "Override sound device clock rate");
DEFINE_int32(ec_tail, 200, "Set echo canceller tail length");
DEFINE_bool(vad, false, "Enable VAD/silence detector");
DEFINE_string(codec, "PCMA/8000/1", "Codec in \"ENCODINGNAME/CLOCKRATE/CHANNELCOUNT\" format");
DEFINE_int32(rtp_port, 5000, "Local RTP Port");
DEFINE_string(remote_rtp_addr, "127.0.0.1", "Remote RTP IP Address");
DEFINE_int32(remote_rtp_port, 5000, "Remote RTP Port");
DEFINE_string(stun_srv, "", "Set STUN server host or domain");

#include "voxve.h"

int main(int argc, char ** argv)
{
	google::ParseCommandLineFlags(&argc, &argv, true);

	voxve_status_t status;
	char errMsg[256];

	status = voxve_init(0, 0, 0);
	if (status != 0)
	{
		voxve_strerror(status, errMsg, sizeof(errMsg));
		cout << "Err: " << errMsg << endl;
		return status;
	}

	/* STUN */
	if (!FLAGS_stun_srv.empty())
		voxve_stun_enable(FLAGS_stun_srv.c_str());		// stun.ekiga.net

	if (FLAGS_stereo)
	{
		cout << "Stereo enabled" << endl;
		voxve_enable_stereo();
	}
	else {
		cout << "Mono enabled" << endl;
		voxve_disable_stereo();
	}

	if (FLAGS_vad)
	{
		cout << "VAD enabled " << endl;
		voxve_enable_vad();
	}
	else
	{
		cout << "VAD disabled " << endl;
		voxve_disable_vad();
	}

	if (FLAGS_ec_tail > 0)
	{
		cout << "AEC enabled " << endl;
		voxve_snd_set_ec(FLAGS_ec_tail);
	}
	else {
		cout << "AEC disabled " << endl;
		voxve_snd_set_ec(0);
	}

	voxve_snd_set_clockrate(FLAGS_snd_clock_rate);
	
	voxve_conf_set_clockrate(FLAGS_clock_rate);

	int channel_id = voxve_channel_create(FLAGS_rtp_port);

	if (channel_id == -1)
		cout << "Create channel error" << endl;
	else
	{
		if (!FLAGS_stun_srv.empty())
		{
			/* Get resolved public addr via STUN */
			char addr_buf[128];
			unsigned port;
			status = voxve_stun_get_public_addr(channel_id, addr_buf, sizeof(addr_buf) - 1, port);
			if (status == 0)
				cout << "Resolved public addr " << addr_buf << ":" << port << endl;
			else
				goto on_error;
		}

		status = voxve_channel_startstream3(channel_id, FLAGS_codec.c_str(), -1, 20, 101, 0,
				FLAGS_remote_rtp_addr.c_str(), FLAGS_remote_rtp_port, STREAM_DIR_ENCODING_DECODING);
		if (status != 0)	goto on_error;
		
		status = voxve_channel_startplayout(channel_id);
		if (status != 0)	goto on_error;
		
		cin.get();

		status = voxve_channel_stopplayout(channel_id);
		if (status != 0)	goto on_error;

		status = voxve_channel_stopstream(channel_id);
		if (status != 0)	goto on_error;

		status = voxve_channel_delete(channel_id);
		if (status != 0)	goto on_error;
	}

	status = voxve_shutdown();
	if (status != 0)
	{
		voxve_strerror(status, errMsg, sizeof(errMsg));
		cout << "Err: " << errMsg << endl;
		return status;
	}
	else {
		cout << "Done" << endl;

		return 0;
	}

on_error:
	voxve_strerror(status, errMsg, sizeof(errMsg));

	cout << "Err: " << errMsg << endl;
	
	voxve_shutdown();

	return status;
}
