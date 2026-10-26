/*
 * xpurt_priv/imgproc_defs.h -- interface declarations for the Kunlunxin KL driver.
 *
 * The ioctl numbers, constants and structure layouts here are ABI facts:
 * they must match the vendor runtime bit for bit or the driver cannot talk
 * to it. They were recovered from the compiled driver's own debug info and
 * from the MIT-licensed driver sources' use of them -- not copied from the
 * vendor's headers, which ship unlicensed under the XRE EULA and are
 * deliberately not redistributed here. check-abi.sh proves the layouts
 * match the vendor's; LICENSES.md explains why this file exists.
 */

#ifndef KL_XPURT_PRIV_IMGPROC_DEFS_H
#define KL_XPURT_PRIV_IMGPROC_DEFS_H

#include "kl_abi_types.h"

#define IMGPROC_MAX_CORES	(6)
#define VIDEO_RST_CTRL	(0x00D8)
#define IMG_PROC_FORCE_UPD	(0x0004)
#define IMG_PROC_INT_MASK	(0x000C)
#define IMG_PROC_INT_CLEAR	(0x0010)
#define IMG_PROC_INT_STATUS_RAW	(0x0014)
#define IMG_PROC_INT_STATUS_MASKED	(0x0018)
#define IMG_RDMA_START	(0x0450)
#define IMGPROC_RDMA_REGS_NUM	21
#define IMGPROC_RESIZE_REGS_NUM	6
#define IMGPROC_WDMA_REGS_NUM	15
#define IMGPROC_REGS_NUM	(IMGPROC_RDMA_REGS_NUM                                  + IMGPROC_RESIZE_REGS_NUM                                  + IMGPROC_WDMA_REGS_NUM)
#define IMGPROC_RDMA_REGS_BASE	(0x0400)
#define IMGPROC_RESIZE_REGS_BASE	(0x0800)
#define IMGPROC_WDMA_REGS_BASE	(0x0c00)

#endif /* KL_XPURT_PRIV_IMGPROC_DEFS_H */
