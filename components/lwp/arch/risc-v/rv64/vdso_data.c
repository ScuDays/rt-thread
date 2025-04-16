/*
 * Copyright (c) 2006-2024 RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author         Notes
 * 2024-07-04     rcitach        init ver.
 */

#include <rtthread.h>
//#include <gtimer.h>
#include <ktime.h>
#include <time.h>
#include <vdso_datapage.h>
#include <vdso_data.h>

/**
 * @brief 更新vDSO全局时间
 * 
 * 该函数用于更新vDSO数据页中的时间信息:
 * - 更新实时时钟(CLOCK_REALTIME)时间,加上初始时间偏移
 * - 更新单调时钟(CLOCK_MONOTONIC)时间
 * - 更新最后一次计数器值
 */
void rt_vdso_update_glob_time(void)
{
    struct vdso_data *vdata = get_k_vdso_data();
    struct timespec *vdso_ts;
    uint64_t initdata = vdata->realtime_initdata;
    rt_vdso_write_begin(vdata);

    /* 更新实时时钟时间 */
    vdso_ts = &vdata[CS_HRES_COARSE].basetime[CLOCK_REALTIME];
    rt_ktime_boottime_get_ns(vdso_ts);
    vdso_ts->tv_sec = initdata + vdso_ts->tv_sec;

    /* 更新单调时钟时间 */
    vdso_ts = &vdata[CS_HRES_COARSE].basetime[CLOCK_MONOTONIC];
    rt_ktime_boottime_get_ns(vdso_ts);

    /* 更新最后一次计数器值 */
    // vdata->cycle_last = rt_hw_get_cntpct_val();
   // vdata->cycle_last = riscv_cputime_gettime();
    rt_vdso_write_end(vdata);
}
