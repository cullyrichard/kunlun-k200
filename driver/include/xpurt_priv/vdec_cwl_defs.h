/*
 * xpurt_priv/vdec_cwl_defs.h -- interface declarations for the Kunlunxin KL driver.
 *
 * The ioctl numbers, constants and structure layouts here are ABI facts:
 * they must match the vendor runtime bit for bit or the driver cannot talk
 * to it. They were recovered from the compiled driver's own debug info and
 * from the MIT-licensed driver sources' use of them -- not copied from the
 * vendor's headers, which ship unlicensed under the XRE EULA and are
 * deliberately not redistributed here. check-abi.sh proves the layouts
 * match the vendor's; LICENSES.md explains why this file exists.
 */

#ifndef KL_XPURT_PRIV_VDEC_CWL_DEFS_H
#define KL_XPURT_PRIV_VDEC_CWL_DEFS_H

#include "kl_abi_types.h"

#define SHAPER_OFFSET	(0x8<<2)
#define CACHE_ONLY_OFFSET	(0x8<<2)
#define CACHE_WITH_SHAPER_OFFSET	(0x80<<2)

#endif /* KL_XPURT_PRIV_VDEC_CWL_DEFS_H */
