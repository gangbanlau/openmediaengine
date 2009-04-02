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
#include "voxve_log.h"

#include "utils.h"

voxve_status_t voxve_logging_reconfigure(int log_level, int console_log_level, char * filename)
{
	voxve_logging_config_t config;

	// set default
	logging_config_default(&config);

	config.level = log_level;
	config.console_level = console_log_level;

	if (filename != NULL)
	{
		config.log_filename = pj_str(filename);
	}

	return logging_reconfigure(&config);
}
