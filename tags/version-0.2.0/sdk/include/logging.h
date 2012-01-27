/*
 * Copyright (C) 2009-2012 Gang Liu <gangban.lau@gmail.com>
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
#ifndef __OPENMEDIAENGINE__
#error only libopenmediaengine should #include this header
#endif

#ifndef LOGGING_H_
#define LOGGING_H_

#include "pj_inc.h"

#include "voxve_log.h"

/**
 * Logging configuration.
 * Application must call #logging_config_default() to
 * initialize this structure with the default values.
 */
typedef struct
{
    /**
     * Input verbosity level. Value 5 is reasonable.
     */
    unsigned	level;

    /**
     * Verbosity level for console. Value 4 is reasonable.
     */
    unsigned	console_level;

    /**
     * Log decoration.
     */
    unsigned	decor;

    /**
     * Optional log filename.
     */
    pj_str_t	log_filename;

    /**
     * Optional callback function to be called to write log to
     * application specific device. This function will be called for
     * log messages on input verbosity level.
     */
    void       (*cb)(int level, const char *data, pj_size_t len);
} logging_config_t;

/*
 * Init logging default setting
 */
void logging_config_default(logging_config_t *cfg);

/*
 * Use new setting to reconfigure logging
 */
pj_status_t logging_reconfigure(const logging_config_t *cfg);

/**
 * Output log message
 */
void logging(const char *sender, VOXVE_LOG_LEVEL log_level, const char *title, pj_status_t status);

#endif /* LOGGING_H_ */
