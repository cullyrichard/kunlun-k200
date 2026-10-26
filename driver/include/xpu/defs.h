/*
 * xpu/defs.h -- interface declarations for the Kunlunxin KL driver.
 *
 * The ioctl numbers, constants and structure layouts here are ABI facts:
 * they must match the vendor runtime bit for bit or the driver cannot talk
 * to it. They were recovered from the compiled driver's own debug info and
 * from the MIT-licensed driver sources' use of them -- not copied from the
 * vendor's headers, which ship unlicensed under the XRE EULA and are
 * deliberately not redistributed here. check-abi.sh proves the layouts
 * match the vendor's; LICENSES.md explains why this file exists.
 */

#ifndef KL_XPU_DEFS_H
#define KL_XPU_DEFS_H

#include "kl_abi_types.h"

#define XPU_ERRNO_BASE	700

/* Error code to text, for log messages only -- no caller depends on the
 * wording, only on the codes, which are ABI. Codes below XPU_ERRNO_BASE are
 * ordinary host errnos and are not translated here. */
static inline const char *xpu_strerror(int e)
{
	int code = (e > 0) ? e : -e;

	if (code < XPU_ERRNO_BASE)
		return "host error";

	switch (code) {
	case XPU_SUCCESS:                return "ok";
	case XPUERR_INVALID_DEVICE:      return "no such device";
	case XPUERR_UNINIT:              return "runtime not initialised";
	case XPUERR_NOMEM:               return "out of device memory";
	case XPUERR_NOCPUMEM:            return "out of host memory";
	case XPUERR_NOSUPPORT:           return "unsupported operation";
	case XPUERR_NODEV:               return "device not present";
	case XPUERR_NOIOC:               return "unknown ioctl";
	case XPUERR_NOXPUFUNC:           return "no such device function";
	case XPUERR_LDSO:                return "dynamic loader failure";
	case XPUERR_LDSYM:               return "missing dynamic symbol";
	case XPUERR_SIMULATOR:           return "simulator failure";
	case XPUERR_INVALID_PARAM:       return "invalid parameter";
	case XPUERR_ABNORMAL:            return "device in abnormal state";
	case XPUERR_KEXCEPTION:          return "kernel raised an exception";
	case XPUERR_TIMEOUT:             return "operation timed out";
	case XPUERR_BUSY:                return "device busy";
	case XPUERR_USEAFCLOSE:          return "use after close";
	case XPUERR_UCECC:               return "uncorrectable ECC error";
	case XPUERR_OVERHEAT:            return "device overheated";
	case XPUERR_UNEXPECT:            return "unexpected internal state";
	case XPUERR_DEVRESET:            return "device was reset";
	case XPUERR_HWEXCEPTION:         return "hardware exception";
	case XPUERR_HBM_INIT:            return "HBM initialisation failed";
	case XPUERR_DEVINIT:             return "device initialisation failed";
	case XPUERR_PEERRESET:           return "peer device was reset";
	case XPUERR_MAXDEV:              return "too many devices";
	case XPUERR_EVENTWAIT:           return "event wait failed";
	case XPUERR_DMATIMEOUT:          return "DMA timed out";
	case XPUERR_DMAABORT:            return "DMA aborted";
	case XPUERR_MCUUNINIT:           return "MCU not initialised";
	case XPUERR_OLDFW:               return "firmware too old";
	case XPUERR_VERSION:             return "version mismatch";
	case XPUERR_PCIE:                return "PCIe error";
	case XPUERR_FAULT:               return "device fault";
	case XPUERR_INTERRUPTED:         return "interrupted";
	case XPUERR_AESTIMEOUT:          return "AES timed out";
	case XPUERR_TRACER:              return "tracer error";
	case XPUERR_HOSTMEM_ALREADY_REGISTERED:
	                                 return "host memory already registered";
	default:                         return "unknown error";
	}
}

#endif /* KL_XPU_DEFS_H */
