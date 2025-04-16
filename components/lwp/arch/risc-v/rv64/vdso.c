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
#include <mmu.h>
//#include <gtimer.h>
#include <lwp_user_mm.h>

#include "vdso.h"
#include "vdso_datapage.h"
#define DBG_TAG    "vdso"
#define DBG_LVL    DBG_INFO
#include <rtdbg.h>

/* vDSO ABI 类型枚举 */
enum vdso_abi {
    /* VDSO_ABI_AA64,   ARM64 ABI */ 
    VDSO_ABI_RV64,  /* RISC-V 64-bit ABI */
};

/* vDSO 数据页偏移枚举 */
enum vvar_pages {
    VVAR_DATA_PAGE_OFFSET,     /* 数据页偏移 */
    VVAR_TIMENS_PAGE_OFFSET,   /* 时间命名空间页偏移 */
    VVAR_NR_PAGES,             /* 总页数 */
};

/* vDSO ABI 信息结构体 */
struct vdso_abi_info {
    const char *name;              /* ABI 名称 */
    const char *vdso_code_start;   /* vDSO 代码段起始地址 */
    const char *vdso_code_end;     /* vDSO 代码段结束地址 */
    unsigned long vdso_pages;      /* vDSO 代码段页数 */
};

/* vDSO ABI 信息数组 */
static struct vdso_abi_info vdso_info[] = {
    /* [VDSO_ABI_AA64] = {
        .name = "vdso_aarch64",
        .vdso_code_start = __vdso_text_start,
        .vdso_code_end = __vdso_text_end,
    }, */
    [VDSO_ABI_RV64] = {
        .name = "vdso_riscv64",
        .vdso_code_start = __vdso_text_start,
        .vdso_code_end = __vdso_text_end,
    },
};

/* vDSO 数据存储区,页对齐 */
static union {
    struct vdso_data    data[CS_BASES];
    uint8_t         page[ARCH_PAGE_SIZE];
} vdso_data_store __page_aligned_data;
struct vdso_data *vdso_data = vdso_data_store.data;
int init_ret_flag = RT_EOK;

/**
 * @brief 设置额外的vDSO页面
 * @param abi vDSO ABI类型
 * @param lwp 用户进程控制块
 * @return 成功返回RT_EOK,失败返回RT_ERROR
 */
static int __setup_additional_pages(enum vdso_abi abi, struct rt_lwp *lwp)
{
    RT_ASSERT(lwp != RT_NULL);

    int ret;
    void *vdso_base = RT_NULL;
    unsigned long vdso_data_len, vdso_text_len;

    /* 计算vDSO数据段和代码段长度 */
    vdso_data_len = VVAR_NR_PAGES * ARCH_PAGE_SIZE;
    vdso_text_len = vdso_info[abi].vdso_pages << ARCH_PAGE_SHIFT;

    /* 映射vDSO数据段到用户空间 */  
    vdso_base = lwp_map_user_phy(lwp, RT_NULL, rt_kmem_v2p((void *)vdso_data), vdso_data_len, 0);
    if(vdso_base != RT_NULL)
    {
        ret = RT_EOK;
    }
    else
    {
        ret = RT_ERROR;
    }
    
    /* 映射vDSO代码段到用户空间 */
    vdso_base += vdso_data_len;
    vdso_base = lwp_map_user_phy(lwp, vdso_base, rt_kmem_v2p((void *)vdso_info[abi].vdso_code_start), vdso_text_len, 0);

    lwp->vdso_vbase = vdso_base;
    return ret;
}

/**
 * @brief 为进程设置vDSO页面
 * @param lwp 用户进程控制块
 * @return 成功返回RT_EOK,失败返回-RT_ERROR
 */

// 进程创建时调用，用于设置vDSO页面
int arch_setup_additional_pages(struct rt_lwp *lwp)
{
    int ret;
    if (init_ret_flag != RT_EOK) return -RT_ERROR;
    /* ret = __setup_additional_pages(VDSO_ABI_AA64, lwp); */
    ret = __setup_additional_pages(VDSO_ABI_RV64, lwp);

    return ret;
}

/**
 * @brief 初始化vDSO数据
 */
static void __initdata(void)
{
    struct tm time_vdso = SOFT_RTC_VDSOTIME_DEFAULT;
    vdso_data->realtime_initdata = timegm(&time_vdso);
}

/**
 * @brief 验证vDSO ELF格式
 * @return 成功返回RT_EOK,失败返回-RT_ERROR
 */
static int validate_vdso_elf(void)
{
    /* 验证ELF头 */
    /* if (rt_memcmp(vdso_info[VDSO_ABI_AA64].vdso_code_start, ELF_HEAD, ELF_HEAD_LEN)) {
        LOG_E("vDSO is not a valid ELF object!");
        init_ret_flag = -RT_ERROR;
        return -RT_ERROR;
    } */
    if (rt_memcmp(vdso_info[VDSO_ABI_RV64].vdso_code_start, ELF_HEAD, ELF_HEAD_LEN)) {
        LOG_E("vDSO is not a valid ELF object!");
        init_ret_flag = -RT_ERROR;
        return -RT_ERROR;
    }

    /* 计算vDSO代码段页数 */
    /* vdso_info[VDSO_ABI_AA64].vdso_pages = (
        vdso_info[VDSO_ABI_AA64].vdso_code_end -
        vdso_info[VDSO_ABI_AA64].vdso_code_start) >>
        ARCH_PAGE_SHIFT; */
    vdso_info[VDSO_ABI_RV64].vdso_pages = (
        vdso_info[VDSO_ABI_RV64].vdso_code_end -
        vdso_info[VDSO_ABI_RV64].vdso_code_start) >>
        ARCH_PAGE_SHIFT;

    __initdata();
    return RT_EOK;
}
INIT_COMPONENT_EXPORT(validate_vdso_elf);
