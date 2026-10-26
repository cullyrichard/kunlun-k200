/*
 * xpurt_priv/vdec_dwl_defs.h -- interface declarations for the Kunlunxin KL driver.
 *
 * The ioctl numbers, constants and structure layouts here are ABI facts:
 * they must match the vendor runtime bit for bit or the driver cannot talk
 * to it. They were recovered from the compiled driver's own debug info and
 * from the MIT-licensed driver sources' use of them -- not copied from the
 * vendor's headers, which ship unlicensed under the XRE EULA and are
 * deliberately not redistributed here. check-abi.sh proves the layouts
 * match the vendor's; LICENSES.md explains why this file exists.
 */

#ifndef KL_XPURT_PRIV_VDEC_DWL_DEFS_H
#define KL_XPURT_PRIV_VDEC_DWL_DEFS_H

#include "kl_abi_types.h"

#define DWL_MPEG2_E	31
#define DWL_VC1_E	29
#define DWL_JPEG_E	28
#define DWL_HJPEG_E	17
#define DWL_MPEG4_E	26
#define DWL_H264_E	24
#define DWL_H264HIGH10_E	20
#define DWL_AVS2_E	18
#define DWL_VP6_E	23
#define DWL_RV_E	26
#define DWL_VP8_E	23
#define DWL_VP7_E	24
#define DWL_WEBP_E	19
#define DWL_AVS_E	22
#define DWL_PP_E	31
#define DWL_HEVC_E	26
#define DWL_VP9_E	29
#define VDEC_IRQ_STAT_DEC	1
#define VDEC_IRQ_STAT_DEC_OFF	(VDEC_IRQ_STAT_DEC * 4)
#define VDEC_SYNTH_CFG	50
#define VDEC_SYNTH_CFG_2	54
#define VDEC_SYNTH_CFG_3	56
#define VDECPP_CFG_STAT	260
#define VDEC_DEC_E	0x01
#define VDEC_DEC_ABORT	0x20
#define VDEC_DEC_IRQ_DISABLE	0x10
#define VDEC_DEC_IRQ	0x100
#define VDEC_HW_BUILD_ID	309
#define VDEC_HW_BUILD_ID_OFF	(VDEC_HW_BUILD_ID * 4)

#endif /* KL_XPURT_PRIV_VDEC_DWL_DEFS_H */
