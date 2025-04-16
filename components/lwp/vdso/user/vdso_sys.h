/*
 * Copyright (c) 2006-2024 RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author         Notes
 * 2024-07-04     rcitach        init ver.
 */

/* vDSO系统调用头文件 */
#ifndef ASM_VDSO_SYS_H
#define ASM_VDSO_SYS_H

#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <vdso_config.h>
#include <vdso_datapage.h>


/* 定义未使用属性宏 */
#define __always_unused    __attribute__((__unused__))
#define __maybe_unused    __attribute__((__unused__))

/* 定义分支预测优化宏 */
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

/* 强制内存访问顺序的内联汇编宏 */

// 修改

// #define arch_counter_enforce_ordering(val) do {    
//     uint64_t tmp, _val = (val);                    
//                                                    
//     asm volatile(                                  
//     "   eor %0, %1, %1\n"                          
//     "   add %0, sp, %0\n"                          
//     "   ldr xzr, [%0]"                             
//     : "=r" (tmp) : "r" (_val));                    
// } while (0)
#define arch_counter_enforce_ordering \
    __asm__ volatile ("fence iorw, iorw" ::: "memory")


/* 获取硬件计数器值 */
// static inline uint64_t __arch_get_hw_counter()
// {
    
//     uint64_t res;

//     __asm__ volatile("mrs %0, CNTVCT_EL0":"=r"(res));

//     arch_counter_enforce_ordering(res);
//     return res;

// }


static inline uint64_t __arch_get_hw_counter()
{
    uint64_t res;
    // 读取周期计数器
    __asm__ volatile ("rdcycle %0" : "=r" (res));
    // RISC-V 默认强内存序，但必要时添加屏障
    arch_counter_enforce_ordering;
    return res;
}


// /* 获取硬件计数器频率 */
// static inline uint64_t __arch_get_hw_frq()
// {

//     uint64_t res;

//     __asm__ volatile("mrs %0, CNTFRQ_EL0":"=r"(res));

//     arch_counter_enforce_ordering(res);
//     return res;
// }

/* 获取硬件计数器频率 */
static inline uint64_t __arch_get_hw_frq()
{
    // risvc64无法和arm64一样通过硬件寄存器获取频率，需要通过设备树（Device Tree）或平台特定方式获取。
    // 现在暂时用一个固定的值。
    return 10000000; // 10 MHz
}

/* 64位除法运算,返回商和余数 */
static inline uint32_t
__iter_div_u64_rem(uint64_t dividend, uint32_t divisor, uint64_t *remainder)
{
    uint32_t ret = 0;

    while (dividend >= divisor) {
        /* The following asm() prevents the compiler from
        optimising this loop into a modulo operation.  */
        __asm__("" : "+rm"(dividend));

        dividend -= divisor;
        ret++;
    }

    *remainder = dividend;

    return ret;
}

/* 字符串化宏定义 */
#define __RT_STRINGIFY(x...)    #x
#define RT_STRINGIFY(x...)    __RT_STRINGIFY(x)
#define rt_hw_barrier(cmd, ...) \
    __asm__ volatile (RT_STRINGIFY(cmd) " "RT_STRINGIFY(__VA_ARGS__):::"memory")





// /* 内存屏障相关宏定义 */
// #define rt_hw_isb() rt_hw_barrier(isb)
// #define rt_hw_dmb() rt_hw_barrier(dmb, ish)
// #define rt_hw_wmb() rt_hw_barrier(dmb, ishst)
// #define rt_hw_rmb() rt_hw_barrier(dmb, ishld)
// #define rt_hw_dsb() rt_hw_barrier(dsb, ish)
// 修改
/* 内存屏障相关宏定义 (RISC-V) */
#define rt_hw_isb() rt_hw_barrier(fence.i)          /* 指令同步屏障 */
#define rt_hw_dmb() rt_hw_barrier(fence, rw, rw)    /* 数据内存屏障 (完全) */
#define rt_hw_wmb() rt_hw_barrier(fence, w, w)      /* 写内存屏障 */
#define rt_hw_rmb() rt_hw_barrier(fence, r, r)      /* 读内存屏障 */
#define rt_hw_dsb() rt_hw_barrier(fence, rw, rw)    /* 数据同步屏障 (完全) */

#ifndef barrier
/* The "volatile" is due to gcc bugs */
// 修改
// # define barrier() __asm__ __volatile__("": : :"memory")
# define barrier() __asm__ __volatile__("fence": : :"memory") // 使用 RISC-V fence
#endif

/* CPU放松指令 */
static inline void cpu_relax(void)
{
    // 修改
    // __asm__ volatile("yield" ::: "memory");
    __asm__ volatile("nop" ::: "memory"); // 使用 nop 代替 yield
}

/* 一次性读取内存的宏定义 */
#define __READ_ONCE_SIZE                                    \
({                                                          \
    switch (size) {                                         \
    case 1: *(__u8 *)res = *(volatile __u8 *)p; break;      \
    case 2: *(__u16 *)res = *(volatile __u16 *)p; break;    \
    case 4: *(__u32 *)res = *(volatile __u32 *)p; break;    \
    case 8: *(__u64 *)res = *(volatile __u64 *)p; break;    \
    default:                            \
        barrier();                      \
        __builtin_memcpy((void *)res, (const void *)p, size);   \
        barrier();                      \
    }                                   \
})

/* 一次性读取内存的内联函数 */
static inline
void __read_once_size(const volatile void *p, void *res, int size)
{
    __READ_ONCE_SIZE;
}

/* 一次性读取变量的宏定义 */
#define __READ_ONCE(x, check)                       \
({                                                  \
    union { typeof(x) __val; char __c[1]; } __u;    \
    if (check)                          \
        __read_once_size(&(x), __u.__c, sizeof(x));  \
    smp_read_barrier_depends(); /* Enforce dependency ordering from x */ \
    __u.__val;                                       \
})
#define READ_ONCE(x) __READ_ONCE(x, 1)

/* vDSO数据页外部声明 */
extern struct vdso_data _vdso_data[CS_BASES] __attribute__((visibility("hidden")));
static inline struct vdso_data *__arch_get_vdso_data(void)
{
    return _vdso_data;
}

/* vDSO读取开始函数 */
static inline uint32_t rt_vdso_read_begin(const struct vdso_data *vd)
{
    uint32_t seq;

    while (unlikely((seq = READ_ONCE(vd->seq)) & 1))
        cpu_relax();

    rt_hw_rmb();
    return seq;
}

/* vDSO读取重试函数 */
static inline uint32_t rt_vdso_read_retry(const struct vdso_data *vd,
                       uint32_t start)
{
    uint32_t seq;

    rt_hw_rmb();
    seq = READ_ONCE(vd->seq);
    return seq != start;
}

#endif
