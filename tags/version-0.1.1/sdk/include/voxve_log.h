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
#ifndef VOXVE_LOG_H_
#define VOXVE_LOG_H_

#include "voxve.h"

/* Logging Level */
enum VOXVE_LOG_LEVEL
{
	VOXVE_LOG_ERR = 1,
	VOXVE_LOG_WARN,
	VOXVE_LOG_INFO,
	VOXVE_LOG_DEBUG,
	VOXVE_LOG_TRACE
};

/* Logging configuration */
OPENMEDIAENGINE_DLL_API voxve_status_t voxve_logging_reconfigure(int log_level, int console_log_level, const char * log_filename);

#endif // _VOXVE_LOG_H_
