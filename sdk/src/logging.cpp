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
#include "logging.h"

#include "global.h"

#define THIS_FILE "logging.cpp"

extern struct voxve_data voxve_var;

/* Set default logging cfg */
void logging_config_default(logging_config_t *cfg)
{
    pj_bzero(cfg, sizeof(*cfg));

    cfg->level = 3;
    cfg->console_level = 3;
    cfg->decor = PJ_LOG_HAS_SENDER | PJ_LOG_HAS_TIME | PJ_LOG_HAS_MICRO_SEC | PJ_LOG_HAS_NEWLINE;
}

static void logging_config_dup(pj_pool_t *pool, logging_config_t *dst, const logging_config_t *src)
{
    pj_memcpy(dst, src, sizeof(*src));
    pj_strdup_with_null(voxve_var.pool, &dst->log_filename, &src->log_filename);
}

/* Log callback */
static void log_writer(int level, const char *buffer, int len)
{
    /* Write to file, stdout or application callback. */

    if (voxve_var.log_file)
	{
		pj_ssize_t size = len;
		pj_file_write(voxve_var.log_file, buffer, &size);
		/* This will slow things down considerably! Don't do it! */
		pj_file_flush(voxve_var.log_file);
    }

    if (level <= (int)voxve_var.log_cfg.console_level)
	{
		if (voxve_var.log_cfg.cb)
			(*voxve_var.log_cfg.cb)(level, buffer, len);
		else
			pj_log_write(level, buffer, len);
    }
}

void logging(const char *sender, VOXVE_LOG_LEVEL log_level, const char *title, pj_status_t status)
{
    char msg[PJ_ERR_MSG_SIZE];

    pj_strerror(status, msg, sizeof(msg));

	switch(log_level)
	{
#if PJ_LOG_MAX_LEVEL >= 1
	case VOXVE_LOG_ERR:
		PJ_LOG(1,(sender, "%s: %s [status=%d]", title, msg, status));
		break;
#endif
#if PJ_LOG_MAX_LEVEL >= 2
	case VOXVE_LOG_WARN:
		PJ_LOG(2,(sender, "%s: %s [status=%d]", title, msg, status));
		break;
#endif
#if PJ_LOG_MAX_LEVEL >= 3
	case VOXVE_LOG_INFO:
		PJ_LOG(3,(sender, "%s: %s [status=%d]", title, msg, status));
		break;
#endif
#if PJ_LOG_MAX_LEVEL >= 4
	case VOXVE_LOG_DEBUG:
		PJ_LOG(4,(sender, "%s: %s [status=%d]", title, msg, status));
		break;
#endif
#if PJ_LOG_MAX_LEVEL >= 5
	case VOXVE_LOG_TRACE:
		PJ_LOG(5,(sender, "%s: %s [status=%d]", title, msg, status));
		break;
	default:
		PJ_LOG(5,(sender, "%s: %s [status=%d]", title, msg, status));
#endif
	}
}

/*
 * Application can call this function at any time to change logging settings.
 */
pj_status_t logging_reconfigure(const logging_config_t *cfg)
{
    pj_status_t status;

    /* Save config. */
    logging_config_dup(voxve_var.pool, &voxve_var.log_cfg, cfg);

    /* Redirect log function to ours */
    pj_log_set_log_func(&log_writer);

    /* Set decor */
    pj_log_set_decor(voxve_var.log_cfg.decor);

    /* Close existing file, if any */
    if (voxve_var.log_file)
	{
		pj_file_close(voxve_var.log_file);
		voxve_var.log_file = NULL;
    }

    /* If output log file is desired, create the file: */
    if (voxve_var.log_cfg.log_filename.slen)
	{
		status = pj_file_open(voxve_var.pool,
			      voxve_var.log_cfg.log_filename.ptr,
			      PJ_O_WRONLY,
			      &voxve_var.log_file);

		if (status != PJ_SUCCESS)
		{
			logging(THIS_FILE, VOXVE_LOG_WARN, "Create log file", status);
			return status;
		}
    }

    return PJ_SUCCESS;
}
