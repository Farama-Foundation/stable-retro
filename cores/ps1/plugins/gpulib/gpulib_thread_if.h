/**************************************************************************
*   Copyright (C) 2020 The RetroArch Team                                 *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   51 Franklin Street, Fifth Floor, Boston, MA 02111-1307 USA.           *
***************************************************************************/

#ifndef __GPULIB_THREAD_H__
#define __GPULIB_THREAD_H__

#ifdef __cplusplus
extern "C" {
#endif

int  real_do_cmd_list(uint32_t *list, int count,
	       int *cycles_sum_out, int *cycles_last, int *last_cmd);
int  real_renderer_init(void);
void real_renderer_finish(void);
void real_renderer_sync_ecmds(uint32_t * ecmds);
void real_renderer_update_caches(int x, int y, int w, int h, int state_changed);
void real_renderer_flush_queues(void);
void real_renderer_set_interlace(int enable, int is_odd);
void real_renderer_set_config(const struct rearmed_cbs *config);
void real_renderer_notify_res_change(void);

#ifdef __cplusplus
}
#endif

#endif /* __GPULIB_THREAD_H__ */
