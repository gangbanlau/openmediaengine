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

#ifndef _SND_H_
#define _SND_H_

#include "pj_inc.h"

/* Close existing sound device */
void snd_close(pjmedia_snd_port *snd_port);

#endif // _SND_H_
