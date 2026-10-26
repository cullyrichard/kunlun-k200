/*
 * xpurt_priv/ioctl_vdec_kl2.h -- interface declarations for the Kunlunxin KL driver.
 *
 * The ioctl numbers, constants and structure layouts here are ABI facts:
 * they must match the vendor runtime bit for bit or the driver cannot talk
 * to it. They were recovered from the compiled driver's own debug info and
 * from the MIT-licensed driver sources' use of them -- not copied from the
 * vendor's headers, which ship unlicensed under the XRE EULA and are
 * deliberately not redistributed here. check-abi.sh proves the layouts
 * match the vendor's; LICENSES.md explains why this file exists.
 */

#ifndef KL_XPURT_PRIV_IOCTL_VDEC_KL2_H
#define KL_XPURT_PRIV_IOCTL_VDEC_KL2_H

#include "kl_abi_types.h"

#define IOCTL_VDEC_IOC_MAGIC	'k'
#define IOCTL_VDEC_IOCGHWOFFSET	_IOR(IOCTL_VDEC_IOC_MAGIC,  3, unsigned long *)
#define IOCTL_VDEC_IOCGHWIOSIZE	_IOR(IOCTL_VDEC_IOC_MAGIC,  4, unsigned int *)
#define IOCTL_VDEC_IOC_CLI	_IO(IOCTL_VDEC_IOC_MAGIC,   5)
#define IOCTL_VDEC_IOC_STI	_IO(IOCTL_VDEC_IOC_MAGIC,   6)
#define IOCTL_VDEC_IOC_MC_OFFSETS	_IOR(IOCTL_VDEC_IOC_MAGIC,  7, unsigned long *)
#define IOCTL_VDEC_IOC_MC_CORES	_IOR(IOCTL_VDEC_IOC_MAGIC,  8, unsigned int *)
#define IOCTL_VDEC_IOCS_DEC_PUSH_REG	_IOW(IOCTL_VDEC_IOC_MAGIC,  9, struct vdec_core_desc *)
#define IOCTL_VDEC_IOCS_PP_PUSH_REG	_IOW(IOCTL_VDEC_IOC_MAGIC,  10, struct vdec_core_desc *)
#define IOCTL_VDEC_IOCH_DEC_RESERVE	_IO(IOCTL_VDEC_IOC_MAGIC,   11)
#define IOCTL_VDEC_IOCT_DEC_RELEASE	_IO(IOCTL_VDEC_IOC_MAGIC,   12)
#define IOCTL_VDEC_IOCQ_PP_RESERVE	_IO(IOCTL_VDEC_IOC_MAGIC,   13)
#define IOCTL_VDEC_IOCT_PP_RELEASE	_IO(IOCTL_VDEC_IOC_MAGIC,   14)
#define IOCTL_VDEC_IOCX_DEC_WAIT	_IOWR(IOCTL_VDEC_IOC_MAGIC, 15, struct vdec_core_desc *)
#define IOCTL_VDEC_IOCX_PP_WAIT	_IOWR(IOCTL_VDEC_IOC_MAGIC, 16, struct vdec_core_desc *)
#define IOCTL_VDEC_IOCS_DEC_PULL_REG	_IOWR(IOCTL_VDEC_IOC_MAGIC, 17, struct vdec_core_desc *)
#define IOCTL_VDEC_IOCS_PP_PULL_REG	_IOWR(IOCTL_VDEC_IOC_MAGIC, 18, struct vdec_core_desc *)
#define IOCTL_VDEC_IOCG_CORE_WAIT	_IOR(IOCTL_VDEC_IOC_MAGIC,  19, int *)
#define IOCTL_VDEC_IOX_ASIC_ID	_IOWR(IOCTL_VDEC_IOC_MAGIC, 20, unsigned long *)
#define IOCTL_VDEC_IOCG_CORE_ID	_IOR(IOCTL_VDEC_IOC_MAGIC,  21, unsigned long)
#define IOCTL_VDEC_IOCS_DEC_WRITE_REG	_IOW(IOCTL_VDEC_IOC_MAGIC,  22, struct vdec_core_desc *)
#define IOCTL_VDEC_IOCS_DEC_READ_REG	_IOWR(IOCTL_VDEC_IOC_MAGIC, 23, struct vdec_core_desc *)
#define IOCTL_VDEC_IOX_ASIC_BUILD_ID	_IOWR(IOCTL_VDEC_IOC_MAGIC, 24, unsigned long *)
#define IOCTL_VDEC_DEBUG_STATUS	_IO(IOCTL_VDEC_IOC_MAGIC,   25)
#define IOCTL_VDEC_CACHE_IOCGHWOFFSET	_IOR(IOCTL_VDEC_IOC_MAGIC, 26, unsigned long *)
#define IOCTL_VDEC_CACHE_IOCGHWIOSIZE	_IOR(IOCTL_VDEC_IOC_MAGIC, 27, unsigned int *)
#define IOCTL_VDEC_CACHE_IOCH_HW_RESERVE	_IOR(IOCTL_VDEC_IOC_MAGIC, 29, unsigned int *)
#define IOCTL_VDEC_CACHE_IOCH_HW_RELEASE	_IOR(IOCTL_VDEC_IOC_MAGIC, 30, unsigned int *)
#define IOCTL_VDEC_CACHE_IOCG_CORE_NUM	_IOR(IOCTL_VDEC_IOC_MAGIC, 31, unsigned int *)
#define IOCTL_VDEC_CACHE_IOCG_ABORT_WAIT	_IOR(IOCTL_VDEC_IOC_MAGIC, 32, unsigned int *)
#define IOCTL_VDEC_IOC_MAXNR	32

#endif /* KL_XPURT_PRIV_IOCTL_VDEC_KL2_H */
