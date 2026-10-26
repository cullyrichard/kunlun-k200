/*
 * xpurt_priv/ioctl_venc_kl2.h -- interface declarations for the Kunlunxin KL driver.
 *
 * The ioctl numbers, constants and structure layouts here are ABI facts:
 * they must match the vendor runtime bit for bit or the driver cannot talk
 * to it. They were recovered from the compiled driver's own debug info and
 * from the MIT-licensed driver sources' use of them -- not copied from the
 * vendor's headers, which ship unlicensed under the XRE EULA and are
 * deliberately not redistributed here. check-abi.sh proves the layouts
 * match the vendor's; LICENSES.md explains why this file exists.
 */

#ifndef KL_XPURT_PRIV_IOCTL_VENC_KL2_H
#define KL_XPURT_PRIV_IOCTL_VENC_KL2_H

#include "kl_abi_types.h"

#define IOCTL_VENC_IOC_MAGIC	'e'
#define IOCTL_VENC_IOCGHWOFFSET	_IOR(IOCTL_VENC_IOC_MAGIC,  3, unsigned long *)
#define IOCTL_VENC_IOCGHWIOSIZE	_IOR(IOCTL_VENC_IOC_MAGIC,  4, unsigned int *)
#define IOCTL_VENC_IOCH_ENC_RESERVE	_IOR(IOCTL_VENC_IOC_MAGIC, 11, unsigned int *)
#define IOCTL_VENC_IOCH_ENC_RELEASE	_IOR(IOCTL_VENC_IOC_MAGIC, 12, unsigned int *)
#define IOCTL_VENC_IOCG_CORE_NUM	_IOR(IOCTL_VENC_IOC_MAGIC, 13, unsigned int *)
#define IOCTL_VENC_IOCG_CORE_INFO	_IOR(IOCTL_VENC_IOC_MAGIC, 14, SUBSYS_CORE_INFO *)
#define IOCTL_VENC_IOCS_ENC_PUSH_REG	_IOW(IOCTL_VENC_IOC_MAGIC, 15, struct subsys_enc *)
#define IOCTL_VENC_IOCG_CORE_WAIT	_IOR(IOCTL_VENC_IOC_MAGIC, 19, unsigned int *)
#define IOCTL_VENC_IOC_MAXNR	19
#define GET_ENCODER_IDX(type_info)	(CORE_VC8000E)
#define VENC_MAX_SUBSYS	(3)

#endif /* KL_XPURT_PRIV_IOCTL_VENC_KL2_H */
