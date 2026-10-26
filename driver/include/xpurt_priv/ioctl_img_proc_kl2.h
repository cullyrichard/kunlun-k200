/*
 * xpurt_priv/ioctl_img_proc_kl2.h -- interface declarations for the Kunlunxin KL driver.
 *
 * The ioctl numbers, constants and structure layouts here are ABI facts:
 * they must match the vendor runtime bit for bit or the driver cannot talk
 * to it. They were recovered from the compiled driver's own debug info and
 * from the MIT-licensed driver sources' use of them -- not copied from the
 * vendor's headers, which ship unlicensed under the XRE EULA and are
 * deliberately not redistributed here. check-abi.sh proves the layouts
 * match the vendor's; LICENSES.md explains why this file exists.
 */

#ifndef KL_XPURT_PRIV_IOCTL_IMG_PROC_KL2_H
#define KL_XPURT_PRIV_IOCTL_IMG_PROC_KL2_H

#include "kl_abi_types.h"

#define IOCTL_IMGPROC_MAGIC	'p'
#define IOCTL_IMGPROC_RESET	_IO(IOCTL_IMGPROC_MAGIC, 1)
#define IOCTL_IMGPROC_CORE_NUM	_IO(IOCTL_IMGPROC_MAGIC, 2)
#define IOCTL_IMGPROC_RESERVE	_IO(IOCTL_IMGPROC_MAGIC, 3)
#define IOCTL_IMGPROC_RELEASE	_IO(IOCTL_IMGPROC_MAGIC, 4)
#define IOCTL_IMGPROC_RUN	_IOW(IOCTL_IMGPROC_MAGIC, 5, struct imgproc_ioctl_run_param *)
#define IOCTL_IMGPROC_WAIT	_IO(IOCTL_IMGPROC_MAGIC, 6)
#define IOCTL_IMGPROC_WRITE_REG	_IOW(IOCTL_IMGPROC_MAGIC,  7, struct imgproc_ioctl_rw_reg_param *)
#define IOCTL_IMGPROC_READ_REG	_IOWR(IOCTL_IMGPROC_MAGIC, 8, struct imgproc_ioctl_rw_reg_param *)
#define IOCTL_IMGPROC_MAXNR	(8)

#endif /* KL_XPURT_PRIV_IOCTL_IMG_PROC_KL2_H */
