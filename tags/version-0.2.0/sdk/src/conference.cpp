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

#include "conference.h"

#include "voxve.h"

#include "global.h"
#include "config.h"
#include "utils.h"
#include "snd.h"

#define THIS_FILE "conference.cpp"

extern struct voxve_data voxve_var;

conf_t * conf_find(int conf_id)
{
	conf_t * conf = NULL;
	pj_rwmutex_lock_read(voxve_var.activeconfs_rwmutex);
	std::map<int, conf_t*>::iterator iter2 = voxve_var.activeconfs.find(conf_id);

	if (iter2 != voxve_var.activeconfs.end())
	{
		conf = (*iter2).second;
	}
	pj_rwmutex_unlock_read(voxve_var.activeconfs_rwmutex);

	return conf;
}
