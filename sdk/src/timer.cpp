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
#include "timer.h"

#include "global.h"
#include "utils.h"

#define THIS_FILE "timer.cpp"

/* Timer Heap */
#define MAX_TIMER_COUNT				1000
#define MAX_TIMED_OUT_ENTRIES		10

extern struct voxve_data voxve_var;

int timer_heap_handle_events(unsigned msec_timeout)
{
    unsigned count = 0;
    pj_time_val tv;

    tv.sec = 0;
    tv.msec = msec_timeout;
    pj_time_val_normalize(&tv);

	pj_time_val next_delay = {0, 0};
	int c = pj_timer_heap_poll(voxve_var.timer_heap, &next_delay);
	if (c > 0)
		count += c;

	if (msec_timeout > 0)
	{
		if (PJ_TIME_VAL_GT(next_delay, tv))
		{
			pj_thread_sleep(PJ_TIME_VAL_MSEC(tv));
		}
		else
		{
			 pj_thread_sleep(PJ_TIME_VAL_MSEC(next_delay));
			 c = pj_timer_heap_poll(voxve_var.timer_heap, &next_delay);
			 if (c > 0)
				 count += c;
		}
	}

    return count;
}

pj_status_t timer_heap_init()
{
    /* Create timer heap to manage all timers within this engine */
	pj_status_t status = pj_timer_heap_create(voxve_var.pool, MAX_TIMER_COUNT, &voxve_var.timer_heap);
	PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

    /* Set recursive lock for the timer heap. */
	pj_lock_t * lock;
    status = pj_lock_create_recursive_mutex(voxve_var.pool, "t%p", &lock);
	PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);
    pj_timer_heap_set_lock(voxve_var.timer_heap, lock, PJ_TRUE);

    /* Set maximum timed out entries to process in a single poll. */
    pj_timer_heap_set_max_timed_out_per_poll(voxve_var.timer_heap, MAX_TIMED_OUT_ENTRIES);

	return PJ_SUCCESS;
}

pj_status_t timer_heap_destroy()
{
	if (voxve_var.timer_heap)
	{
		pj_timer_heap_destroy(voxve_var.timer_heap);
		voxve_var.timer_heap = NULL;
	}

	return PJ_SUCCESS;
}
