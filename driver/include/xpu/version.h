/*
 * xpu/version.h -- interface declarations for the Kunlunxin KL driver.
 *
 * The ioctl numbers, constants and structure layouts here are ABI facts:
 * they must match the vendor runtime bit for bit or the driver cannot talk
 * to it. They were recovered from the compiled driver's own debug info and
 * from the MIT-licensed driver sources' use of them -- not copied from the
 * vendor's headers, which ship unlicensed under the XRE EULA and are
 * deliberately not redistributed here. check-abi.sh proves the layouts
 * match the vendor's; LICENSES.md explains why this file exists.
 */

#ifndef KL_XPU_VERSION_H
#define KL_XPU_VERSION_H

#include "kl_abi_types.h"

#define XPURT_VERSION_MAJOR	4
#define XPURT_VERSION_MINOR	33
#define XPURT_VERSION_THIRD	0
#define XPURT_COMMIT	"0322c392ad80ac403c6936035bf5f57019132c4c"
#define XPURT_CHANGESET	99999
#define XPURT_VERSION_STR	"4.33.0"
#define XPURT_COMMIT_TIME	"Mon Nov 25 15:50:57 2024 +0800"
#define XPURT_AGILE_PIPELINE	"BranchPipeline"

#endif /* KL_XPU_VERSION_H */
