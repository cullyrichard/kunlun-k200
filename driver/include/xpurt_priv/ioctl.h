/*
 * xpurt_priv/ioctl.h -- interface declarations for the Kunlunxin KL driver.
 *
 * The ioctl numbers, constants and structure layouts here are ABI facts:
 * they must match the vendor runtime bit for bit or the driver cannot talk
 * to it. They were recovered from the compiled driver's own debug info and
 * from the MIT-licensed driver sources' use of them -- not copied from the
 * vendor's headers, which ship unlicensed under the XRE EULA and are
 * deliberately not redistributed here. check-abi.sh proves the layouts
 * match the vendor's; LICENSES.md explains why this file exists.
 */

#ifndef KL_XPURT_PRIV_IOCTL_H
#define KL_XPURT_PRIV_IOCTL_H

#include "kl_abi_types.h"

#define IOC_VERSION	5
#define IOCTL_IOC_MAGIC	(0x8EEDCE)
#define IOCTL_IOC_VERSION	_IOR(IOCTL_IOC_MAGIC, 1, int)
#define IOCTL_DEV_HARD_RESET	_IO(IOCTL_IOC_MAGIC, 3)
#define IOCTL_DEV_SOFT_RESET	_IO(IOCTL_IOC_MAGIC, 4)
#define IOCTL_REG_READ	_IOWR(IOCTL_IOC_MAGIC, 5, struct XPURegisterIoctlArgs)
#define IOCTL_REG_WRITE	_IOW (IOCTL_IOC_MAGIC, 6, struct XPURegisterIoctlArgs)
#define IOCTL_MEMORY_ALLOC	_IOWR(IOCTL_IOC_MAGIC, 7, struct XPUMemoryAllocIoctlArgs)
#define IOCTL_MEMORY_FREE	_IOW (IOCTL_IOC_MAGIC, 8, void *)
#define IOCTL_MEMCPY	_IOWR(IOCTL_IOC_MAGIC, 9, struct XPUMemcpyIoctlArgs)
#define IOCTL_MEMCPY_H2D	_IOWR(IOCTL_IOC_MAGIC, 11, struct XPUMemcpyIoctlArgs)
#define IOCTL_MEMCPY_D2H	_IOWR(IOCTL_IOC_MAGIC, 12, struct XPUMemcpyIoctlArgs)
#define IOCTL_MEMCPY_D2D	_IOWR(IOCTL_IOC_MAGIC, 13, struct XPUMemcpyIoctlArgs)
#define IOCTL_MEMCPY_P2P	_IOWR(IOCTL_IOC_MAGIC, 14, struct XPUMemcpyIoctlArgs)
#define IOCTL_MEMCPY_H2D_EX	_IOWR(IOCTL_IOC_MAGIC, 19, struct XPUMemcpyIoctlArgs)
#define IOCTL_MEMCPY_D2H_EX	_IOWR(IOCTL_IOC_MAGIC, 20, struct XPUMemcpyIoctlArgs)
#define IOCTL_DEV_SET_NUMVFS	_IOW(IOCTL_IOC_MAGIC, 21, int)
#define IOCTL_LAUNCH	_IOW(IOCTL_IOC_MAGIC, 41, struct XPULaunchIoctlArgs)
#define IOCTL_WAIT	_IOW(IOCTL_IOC_MAGIC, 42, struct XPUWaitIoctlArgs)
#define IOCTL_BATCHLAUNCH	_IOW(IOCTL_IOC_MAGIC, 43, struct XPUBatchLaunchIoctlArgs)
#define IOCTL_PROFLAUNCH	_IOW(IOCTL_IOC_MAGIC, 44, struct XPUProfLaunch)
#define IOCTL_STREAM_WAIT_EVENT	_IOW(IOCTL_IOC_MAGIC, 45, int)
#define IOCTL_SESSION_BINDTQ	_IOR(IOCTL_IOC_MAGIC, 50, uint64_t)
#define IOCTL_EVENT_CREATE	_IOR(IOCTL_IOC_MAGIC, 51, union XPUEventCreate)
#define IOCTL_EVENT_DESTROY	_IOW(IOCTL_IOC_MAGIC, 52, int)
#define IOCTL_EVENT_RECORD	_IOW(IOCTL_IOC_MAGIC, 53, int)
#define IOCTL_EVENT_WAIT	_IOW(IOCTL_IOC_MAGIC, 54, int)
#define IOCTL_QUERY_BAR	_IOR(IOCTL_IOC_MAGIC, 60, struct bar_info)
#define IOCTL_QUERY_DEVATTR	_IOR(IOCTL_IOC_MAGIC, 62, struct xpu_devfile_info)
#define IOCTL_QUERY_DEVINFO	_IOR(IOCTL_IOC_MAGIC, 63, void *)
#define IOCTL_QUERY_DEVINFO1	_IOR(IOCTL_IOC_MAGIC, 64, union xpu_device_info_v1)
#define IOCTL_QUERY_PROCINFO	_IO(IOCTL_IOC_MAGIC, 65)
#define IOCTL_SD_LAUNCH	_IOW(IOCTL_IOC_MAGIC, 90, struct XPULaunchIoctlArgs)
#define IOCTL_CLUSTER_LAUNCH	_IOW(IOCTL_IOC_MAGIC, 91, struct XPULaunchIoctlArgs)
#define IOCTL_START_CUNITS	_IO (IOCTL_IOC_MAGIC, 99)
#define IOCTL_PROFCLR	_IO (IOCTL_IOC_MAGIC, 100)
#define IOCTL_TEST	_IO (IOCTL_IOC_MAGIC, 101)
#define IOCTL_SSE_LAUNCH	_IOW(IOCTL_IOC_MAGIC, 141, struct XPULaunchIoctlArgs)
#define IOCTL_SSE_WAIT	_IOW(IOCTL_IOC_MAGIC, 142, struct XPUSSEWaitIoctlArgs)
#define IOCTL_HOST_REGISTER	_IOWR(IOCTL_IOC_MAGIC, 143, struct XPUHostRegisterIoctlArgs)
#define IOCTL_HOST_UNREGISTER	_IOWR(IOCTL_IOC_MAGIC, 144, struct XPUHostRegisterIoctlArgs)
#define IOCTL_DBGM	_IOWR(IOCTL_IOC_MAGIC, 145, struct XPUDBGMIoctlArgs)
#define IOCTL_MEMCPY_P2P_DIRECT	_IOWR(IOCTL_IOC_MAGIC, 146, struct XPUMemcpyIoctlArgs)
#define XPUCTL_MAGIC	(0x8EEDCF)
#define XPUCTL_VERSION	_IOR (XPUCTL_MAGIC, 1, struct XPUDriverVersionIoctlArgs)
#define XPUCTL_DEVCNT	_IOR (XPUCTL_MAGIC, 2, int *)
#define XPUCTL_DEVINFOALL	_IOR (XPUCTL_MAGIC, 4, struct xpu_dev_info_all *)
#define XPUCTL_CHANGESET	_IOR (XPUCTL_MAGIC, 5, uint32_t *)
#define XPUCTL_SET_NUMVFS	_IOW (XPUCTL_MAGIC, 6, struct XPUSriovIoctlArgs)
#define CXPU_INSTANCE_ID_LEN	(12)
#define XPUCTL_CXPU_CONFIG	_IOWR (XPUCTL_MAGIC, 7, struct XPUCxpuIoctlArgs)

#endif /* KL_XPURT_PRIV_IOCTL_H */
