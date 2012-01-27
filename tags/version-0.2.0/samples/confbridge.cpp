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

	if (argc != 8)
	{
		cout << "Usage: " << argv[0] << " conf_clock_rate snd_clock_rate local_port remote_ip remote_port remote_ip_2 remote_port_2" << endl;
		return -1;
	}

	int clock_rate = atoi(argv[1]);
	int snd_clock_rate = atoi(argv[2]);

	int local_port = atoi(argv[3]);
	/* Channel 1, will use CODEC_PCMA */
	char * remote_ip_1 = argv[4];
	int remote_port_1 = atoi(argv[5]);
	/* Channel 2, will use CODEC_PCMU */
	char * remote_ip_2 = argv[6];
	int remote_port_2 = atoi(argv[7]);

	status = voxve_init(0, 0, 0);
	if (status != 0)	return status;

	voxve_snd_set_clockrate(snd_clock_rate);

	voxve_conf_set_clockrate(clock_rate);

	int channel_1, channel_2;
	int channel_1_slot, channel_2_slot;

	cout << "Create conference bridge" << endl;
	int conf_id = voxve_conf_create();
	if (conf_id == -1)
	{
		cout << "Create bridge fail" << endl;
		goto on_error;
	}

	cout << "Attach default sound device into bridge" << endl;
	status = voxve_conf_setsnddev(conf_id, -1, -1);
	if (status != 0) goto on_error;

	cout << "Create media channels" << endl;
	channel_1 = voxve_channel_create(local_port);
	channel_2 = voxve_channel_create(local_port + 2);
	if (channel_1 < 0 || channel_2 < 0)
	{
		cout << "Create channel fail" << endl;
		goto on_error;
	}

	cout << "Start media streams" << endl;
	status = voxve_channel_startstream(channel_1, CODEC_PCMA, 20, remote_ip_1, remote_port_1, STREAM_DIR_ENCODING_DECODING);
	if (status != 0) goto on_error;
	status = voxve_channel_startstream(channel_2, CODEC_PCMU, 30, remote_ip_2, remote_port_2, STREAM_DIR_ENCODING_DECODING);
	if (status != 0) goto on_error;

	cout << "Add channels into bridge" << endl;
	channel_1_slot = voxve_conf_addchannel(conf_id, channel_1);
	channel_2_slot = voxve_conf_addchannel(conf_id, channel_2);
	
	cout << "Connect channels each other" << endl;
	status = voxve_conf_connect(conf_id, 0, channel_1_slot);
	status = voxve_conf_connect(conf_id, 0, channel_2_slot);
	status = voxve_conf_connect(conf_id, channel_1_slot, 0);
	status = voxve_conf_connect(conf_id, channel_1_slot, channel_2_slot);
	status = voxve_conf_connect(conf_id, channel_2_slot, 0);
	status = voxve_conf_connect(conf_id, channel_2_slot, channel_1_slot);

	cin.get();

	status = voxve_conf_setnosnddev(conf_id);

	status = voxve_conf_disconnect(conf_id, 0, channel_1_slot);
	status = voxve_conf_disconnect(conf_id, 0, channel_2_slot);
	status = voxve_conf_disconnect(conf_id, channel_1_slot, 0);
	status = voxve_conf_disconnect(conf_id, channel_1_slot, channel_2_slot);
	status = voxve_conf_disconnect(conf_id, channel_2_slot, 0);
	status = voxve_conf_disconnect(conf_id, channel_2_slot, channel_1_slot);

	status = voxve_conf_removechannel(conf_id, channel_1);
	status = voxve_conf_removechannel(conf_id, channel_2);

	status = voxve_conf_destroy(conf_id);
	if (status != 0)
	{
		cout << "Destroy bridge fail" << endl;
		goto on_error;
	}

	status = voxve_channel_stopstream(channel_1);
	if (status != 0)	goto on_error;
	status = voxve_channel_stopstream(channel_2);
	if (status != 0)	goto on_error;

	status = voxve_channel_delete(channel_1);
	if (status != 0)	goto on_error;
	status = voxve_channel_delete(channel_2);
	if (status != 0)	goto on_error;

	cout << "Done" << endl;
	return 0;

on_error:

	status = voxve_shutdown();
	if (status != 0)	return status;

	return -1;
}
