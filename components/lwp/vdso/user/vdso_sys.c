/*
 * Copyright (c) 2006-2024 RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author         Notes
 * 2024-07-04     rcitach        init ver.
 */

#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <stdbool.h>
#include <vdso_sys.h>

/* 检查硬件计数器是否就绪 */
#ifndef rt_vdso_cycles_ready
static inline bool rt_vdso_cycles_ready(uint64_t cycles)
{
    return true;
}
#endif

/* 根据硬件计数器计算纳秒时间 */
#ifndef rt_vdso_get_ns
static inline
uint64_t rt_vdso_get_ns(uint64_t cycles, uint64_t last)
{
    return (cycles - last) * NSEC_PER_SEC / __arch_get_hw_frq();
}
#endif

/* 获取粗略时间 */
static int
__rt_vdso_getcoarse(struct timespec *ts, clockid_t clock, const struct vdso_data *vdns)
{
    const struct vdso_data *vd;
    const struct timespec *vdso_ts;
    uint32_t seq;
    uint64_t sec, last, ns, cycles;

    /* 根据时钟类型选择数据源 */
    if (clock != CLOCK_MONOTONIC_RAW)
        vd = &vdns[CS_HRES_COARSE];
    else
        vd = &vdns[CS_RAW];

    vdso_ts = &vd->basetime[clock];

    /* 循环读取时间,直到成功 */
    do {
        seq = rt_vdso_read_begin(vd);
        cycles = __arch_get_hw_counter(vd->clock_mode, vd);
        if (unlikely(!rt_vdso_cycles_ready(cycles)))
            return -1;
        ns = vdso_ts->tv_nsec;
        last = vd->cycle_last;
        ns += rt_vdso_get_ns(cycles, last);
        sec = vdso_ts->tv_sec;
    } while (unlikely(rt_vdso_read_retry(vd, seq)));

    /* 计算最终时间 */
    ts->tv_sec = sec + __iter_div_u64_rem(ns, NSEC_PER_SEC, &ns);
    ts->tv_nsec = ns;

    return 0;
}

/* 通用时钟获取函数 */
static inline int
__vdso_clock_gettime_common(const struct vdso_data *vd, clockid_t clock,
                 struct timespec *ts)
{
    u_int32_t msk;

    /* 检查时钟ID是否有效 */
    if (unlikely((u_int32_t) clock >= MAX_CLOCKS))
        return -1;

    /* 根据时钟类型调用相应的处理函数 */
    msk = 1U << clock;
    if (likely(msk & VDSO_REALTIME))
        return __rt_vdso_getcoarse(ts,CLOCK_REALTIME,vd);
    else if (msk & VDSO_MONOTIME)
        return __rt_vdso_getcoarse(ts,CLOCK_MONOTONIC,vd);
    else
        return ENOENT;
}

/* vDSO时钟获取函数 */
static __maybe_unused int
rt_vdso_clock_gettime_data(const struct vdso_data *vd, clockid_t clock,
               struct timespec *ts)
{
    int ret = 0;
    ret = __vdso_clock_gettime_common(vd, clock, ts);
    return ret;
}

/* 内核时钟获取函数 */
int
__kernel_clock_gettime(clockid_t clock, struct timespec *ts)
{
    return rt_vdso_clock_gettime_data(__arch_get_vdso_data(), clock, ts);
}
