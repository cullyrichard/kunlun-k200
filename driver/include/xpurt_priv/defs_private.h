/*
 * xpurt_priv/defs_private.h -- interface declarations for the Kunlunxin KL driver.
 *
 * The ioctl numbers, constants and structure layouts here are ABI facts:
 * they must match the vendor runtime bit for bit or the driver cannot talk
 * to it. They were recovered from the compiled driver's own debug info and
 * from the MIT-licensed driver sources' use of them -- not copied from the
 * vendor's headers, which ship unlicensed under the XRE EULA and are
 * deliberately not redistributed here. check-abi.sh proves the layouts
 * match the vendor's; LICENSES.md explains why this file exists.
 */

#ifndef KL_XPURT_PRIV_DEFS_PRIVATE_H
#define KL_XPURT_PRIV_DEFS_PRIVATE_H

#include "kl_abi_types.h"

#define XPUCTL_NODE_DEVNAME	"xpuctrl"
#define KB	1024
#define MB	(1024*KB)
#define GB	(1024ull*MB)
#define MAX_LAUNCH_BATCH_COUNT	10000

#endif /* KL_XPURT_PRIV_DEFS_PRIVATE_H */
