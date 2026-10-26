/*
 * kl_abi_types.h -- interface declarations for the Kunlunxin KL driver.
 *
 * The ioctl numbers, constants and structure layouts here are ABI facts:
 * they must match the vendor runtime bit for bit or the driver cannot talk
 * to it. They were recovered from the compiled driver's own debug info and
 * from the MIT-licensed driver sources' use of them -- not copied from the
 * vendor's headers, which ship unlicensed under the XRE EULA and are
 * deliberately not redistributed here. check-abi.sh proves the layouts
 * match the vendor's; LICENSES.md explains why this file exists.
 */

#ifndef KL_ABI_TYPES_H
#define KL_ABI_TYPES_H

#include <linux/types.h>

enum CORE_TYPE {
	CORE_VC8000E  = 0,
	CORE_VC8000EJ = 1,
	CORE_CUTREE   = 2,
	CORE_DEC400   = 3,
	CORE_MAX      = 4,
};

typedef enum {
	KL1_BEGIN                       = 0,
	K200                            = 0,
	K100                            = 1,
	KL1_END                         = 1,
	KL2_BEGIN                       = 2,
	R200                            = 2,
	R300                            = 3,
	R200_8F                         = 4,
	R200_8FS                        = 5,
	R100                            = 6,
	R200_DEBUG_BOARD                = 7,
	R420                            = 8,
	RG800                           = 9,
	RG800_PRO                       = 10,
	RM80                            = 11,
	R200_SRIOV_PF                   = 200,
	R200_SRIOV_VF_ONE_OF_ONE        = 201,
	R200_SRIOV_VF_ONE_OF_TWO        = 202,
	R200_SRIOV_VF_ONE_OF_THREE      = 203,
	R300_SRIOV_PF                   = 204,
	R300_SRIOV_VF_ONE_OF_ONE        = 205,
	R300_SRIOV_VF_ONE_OF_TWO        = 206,
	R300_SRIOV_VF_ONE_OF_THREE      = 207,
	R200_8F_SRIOV_PF                = 208,
	R200_8F_SRIOV_VF_ONE_OF_ONE     = 209,
	R200_8F_SRIOV_VF_ONE_OF_TWO     = 210,
	R200_8F_SRIOV_VF_ONE_OF_THREE   = 211,
	R200_8FS_SRIOV_PF               = 212,
	R200_8FS_SRIOV_VF_ONE_OF_ONE    = 213,
	R200_8FS_SRIOV_VF_ONE_OF_TWO    = 214,
	R200_8FS_SRIOV_VF_ONE_OF_THREE  = 215,
	RG800_SRIOV_PF                  = 216,
	RG800_SRIOV_VF_ONE_OF_ONE       = 217,
	RG800_SRIOV_VF_ONE_OF_TWO       = 218,
	RG800_SRIOV_VF_ONE_OF_THREE     = 219,
	RG800_PRO_SRIOV_PF              = 220,
	RG800_PRO_SRIOV_VF_ONE_OF_ONE   = 221,
	RG800_PRO_SRIOV_VF_ONE_OF_TWO   = 222,
	RG800_PRO_SRIOV_VF_ONE_OF_THREE = 223,
	KL2_END                         = 299,
	KL3_BEGIN                       = 300,
	KL3                             = 300,
	KL3_END                         = 599,
} DeviceModel;

typedef struct {
	uint32_t                   type_info;
	uint32_t                   offset[4];
	uint32_t                   regSize[4];

} SUBSYS_CORE_INFO;

struct XPULaunchIoctlArgs;

struct XPUBatchLaunchIoctlArgs {
	uint32_t                   cnt;

	struct XPULaunchIoctlArgs * cmds;
	long unsigned int          kernel_enter_cycle;
	long unsigned int          kernel_exit_cycle;

};

struct XPUCxpuIoctlArgs {
	uint32_t                   devfile_id;

	uint64_t                   instance_id[2];
	uint32_t                   type;

	uint64_t                   value;
	uint64_t                   reserved[3];

};

struct XPUDBGMIoctlArgs {
	uint32_t                   type;

	void *                     args;
	uint64_t                   sz;

};

struct XPUDebugMasterConfig {
	uint32_t                   time_stamp;
	uint32_t                   stamp_interval;
	uint32_t                   lost_interval;
	uint32_t                   port;
	void *                     pinned_ptr;
	uint64_t                   size;
	uint32_t                   is_devmem;
	uint32_t                   enable_mbox;
	uint32_t                   reserve[5];

};

struct XPUDebugMasterMbox {
	uint32_t                   write;
	int                        message;
	int                        success;
	uint32_t                   reserve[6];

};

struct XPUDriverVersionIoctlArgs {
	uint32_t                   major;
	uint32_t                   minor;
	char                       commit[64];

};

typedef enum {
	XPU_SUCCESS                       = 0,
	XPUERR_INVALID_DEVICE             = 701,
	XPUERR_UNINIT                     = 702,
	XPUERR_NODEV                      = 703,
	XPUERR_VERSION                    = 704,
	XPUERR_NOMEM                      = 705,
	XPUERR_NOCPUMEM                   = 706,
	XPUERR_INVALID_PARAM              = 707,
	XPUERR_NOXPUFUNC                  = 708,
	XPUERR_LDSO                       = 709,
	XPUERR_LDSYM                      = 710,
	XPUERR_SIMULATOR                  = 711,
	XPUERR_NOSUPPORT                  = 712,
	XPUERR_ABNORMAL                   = 713,
	XPUERR_KEXCEPTION                 = 714,
	XPUERR_TIMEOUT                    = 715,
	XPUERR_BUSY                       = 716,
	XPUERR_USEAFCLOSE                 = 717,
	XPUERR_UCECC                      = 718,
	XPUERR_OVERHEAT                   = 719,
	XPUERR_EVENTWAIT                  = 720,
	XPUERR_UNEXPECT                   = 800,
	XPUERR_DEVRESET                   = 801,
	XPUERR_HWEXCEPTION                = 802,
	XPUERR_HBM_INIT                   = 803,
	XPUERR_DEVINIT                    = 804,
	XPUERR_PEERRESET                  = 805,
	XPUERR_MAXDEV                     = 806,
	XPUERR_NOIOC                      = 807,
	XPUERR_DMATIMEOUT                 = 808,
	XPUERR_DMAABORT                   = 809,
	XPUERR_MCUUNINIT                  = 810,
	XPUERR_OLDFW                      = 811,
	XPUERR_PCIE                       = 812,
	XPUERR_FAULT                      = 813,
	XPUERR_INTERRUPTED                = 814,
	XPUERR_AESTIMEOUT                 = 815,
	XPUERR_HOSTMEM_ALREADY_REGISTERED = 816,
	XPUERR_TRACER                     = 817,
} XPUError_t;

struct XPUEventCreate_in {
	int                        flags;

};

struct XPUEventCreate_out {
	int                        handle;

};

union XPUEventCreate {
	struct XPUEventCreate_in   in;
	struct XPUEventCreate_out  out;
};

struct XPUHostRegisterIoctlArgs {
	uint64_t                   ptr;
	uint64_t                   size;
	uint32_t                   flags;

	uint64_t                   reserved[5];

};

struct xpu_kernel {
	uint32_t                   type:16;
	uint32_t                   place:16;

	uint64_t                   code_addr;
	uint32_t                   code_byte_size;
	uint32_t                   code_pc;
	uint32_t                   param_dword_size;

	uint64_t                   hash;
	const char  *              name;
	void *                     rt_private;

};

struct XPULaunchIoctlArgs {
	struct xpu_kernel          kernel;

	char                       name[64];

	uint32_t                   params[29];

	uint64_t                   param_addr;
	uint32_t                   nclusters;
	uint32_t                   ncores;

	long unsigned int          kernel_enter_cycle;
	long unsigned int          kernel_exit_cycle;

};

typedef enum {
	XPU_DEVICE_TO_HOST   = 0,
	XPU_HOST_TO_DEVICE   = 1,
	XPU_DEVICE_TO_DEVICE = 2,
} XPUMemcpyKind;

struct XPUMemcpyExIoctlArgs {
	uint64_t                   dest;
	uint64_t                   src;
	uint64_t                   size;
	int                        channel;

	uint64_t                   time_ns;
	XPUMemcpyKind              kind;

};

struct XPUMemcpyIoctlArgs {
	uint64_t                   dest;
	uint64_t                   src;
	uint64_t                   size;
	int                        channel;

	uint64_t                   cycles;
	XPUMemcpyKind              kind;

};

struct XPUMemoryAllocIoctlArgs {
	uint64_t                   addr;
	uint64_t                   size;
	int                        kind;

};

typedef enum {
	XPU_MEM_HBM                  = 0,
	XPU_MEM_MAIN                 = 0,
	XPU_MEM_L3                   = 1,
	XPU_USER_ALLOCABLE_MEM_COUNT = 2,
	XPU_MEM_MMAPPABLE            = 16,
	XPU_MEM_MAIN_KL2_HUGE        = 17,
} XPUMemoryKind;

struct XPUProfLaunch {
	struct XPULaunchIoctlArgs  launch;

	uint64_t                   cycles;

};

struct XPURegisterIoctlArgs {
	int                        bar;

	uint64_t                   addr;
	uint32_t                   value;

};

struct XPUSSEWaitIoctlArgs {
	uint32_t                   wait_count;
	uint32_t                   tq_ids[8];

};

struct XPUSriovIoctlArgs {
	uint32_t                   devfile_id;
	uint32_t                   num_vfs;
	uint32_t                   reserve[6];

};

struct XPUStartCunits {
	size_t                     count;
	int                        cunits[4];

};

struct XPUWaitIoctlArgs {
	uint32_t                   wait_count;
	uint32_t                   session_ids[32];

};

struct bar_info {
	int                        bar_en;

	uint64_t                   pcie_addr[6];
	uint64_t                   bar_size[6];

	uint64_t                   bus_addr[6];

	uint64_t                   real_bus_addr[6];

};

struct iatu_region_info {
	int                        region_type;
	int                        region_en;
	int                        match_mode[4];
	uint32_t                   bar[4];
	uint64_t                   size[4];

	uint64_t                   base[4];
	uint64_t                   target[4];

};

struct imgproc_ioctl_run_param {
	int                        core_id;

	uint32_t *                 regs;

};

struct imgproc_ioctl_rw_reg_param {
	int                        core_id;
	uint32_t                   reg_offset;
	uint32_t                   data;

};

struct ioc_qproc_info_in {
	int                        qmv;
	int                        devfile_id;

};

enum kernel_place {
	KP_CPU = 0,
	KP_XPU = 1,
};

enum kernel_type {
	KT_CLUSTER = 0,
	KT_SDCDNN  = 1,
};

struct subsys_enc {
	uint32_t                   id;

	uint32_t *                 regs;
	uint32_t                   size;
	uint32_t                   reg_id;
	uint32_t                   core_type;

};

struct vdec_core_desc {
	uint32_t                   id;

	uint32_t *                 regs;
	uint32_t                   size;
	uint32_t                   reg_id;

};

typedef enum {
	IMGPROC_WAIT_OK       = 0,
	IMGPROC_WAIT_TIME_OUT = 1,
	IMGPROC_WAIT_ERR      = 2,
} wait_status_t;

struct xpu_dev_monitor {
	uint32_t                   temp[4];
	uint32_t                   freq[6];
	uint32_t                   freq_raw[6];

	uint32_t                   power;
	uint32_t                   hbm_temp[2];

};

struct xpu_mcu_info {
	uint32_t                   flash_version[3];
	uint32_t                   cpld_version;

};

struct xpu_devfile_info {
	int                        id;
	int                        board_id;
	int                        onboard_idx;
	int                        domain;
	unsigned int               bus;
	unsigned int               slot;
	unsigned int               func;
	unsigned int               sn;
	uint32_t                   product_num;
	uint32_t                   l3_page_used;
	uint32_t                   l3_page_all;
	uint32_t                   l3_page_size;
	uint32_t                   hbm_page_used;
	uint32_t                   hbm_page_all;
	uint32_t                   hbm_page_size;
	uint32_t                   use_ratio_numerator;

	uint32_t                   use_ratio_denominator;
	int                        process_count;
	struct {
		int                pid;
		int                stream_count;
		char               comm[16];
	} process[100];

};

struct xpu_dev_info {
	int                        id;
	int                        domain;
	unsigned int               bus;
	unsigned int               slot;
	unsigned int               func;
	unsigned int               sn;
	struct xpu_dev_monitor     monitor;

	struct xpu_mcu_info        mcu_info;
	struct xpu_devfile_info    file[2];

};

struct xpu_dev_info_all {
	int                        dev_count;
	struct xpu_dev_info        devs[64];

};

struct xpu_device_info {
	int                        magic_version;
	int                        model;
	int                        id;
	int                        board_idx;
	int                        chip_idx;
	int                        domain;
	unsigned int               bus;
	unsigned int               slot;
	unsigned int               func;
	uint32_t                   sn;
	uint32_t                   product_num;
	uint32_t                   cache_mem_page_used;
	uint32_t                   cache_mem_page_all;
	uint32_t                   cache_mem_page_size;
	uint32_t                   main_mem_page_used;
	uint32_t                   main_mem_page_all;

	uint32_t                   main_mem_page_size;
	uint32_t                   use_ratio_numerator;
	uint32_t                   use_ratio_denominator;

	uint64_t                   io_rate_byte_size;
	uint32_t                   io_rate_time_us;

	uint64_t                   dev_configs;
	uint32_t                   temperature[10];

	uint32_t                   frequency[10];
	uint32_t                   power;
	uint32_t                   firmware_version[12];

	uint32_t                   hardware_version[12];

	int                        process_count;
	struct {
		int                pid;
		int                stream_count;
		uint32_t           cache_mem_page_used;
		uint32_t           cache_mem_page_all;
		uint32_t           main_page_used;
		uint32_t           main_mem_page_all;
		uint32_t           use_ratio_numerator;
		uint32_t           use_ratio_denominator;

		uint64_t           pcie_dma_byte_size;
	} process[100];

	uint32_t                   reserve[10];

};

struct xpu_device_info_v1_in {
	int                        type;
	int                        arg[7];

};

struct xpu_device_info_v1_out {
	int                        ret;
	uint32_t                   v32;
	uint64_t                   v64[3];

};

union xpu_device_info_v1 {
	struct xpu_device_info_v1_in in;
	struct xpu_device_info_v1_out out;
};

struct xpu_proc {
	int                        pid;
	int                        root_ns_pid;
	int                        stream_count;
	char                       comm[16];

	uint64_t                   cgtoken;
	uint32_t                   hs_mem_pgused;
	uint32_t                   hs_mem_pgall;
	uint32_t                   main_mem_pgused;
	uint32_t                   main_mem_pgall;
	uint64_t                   dma_read_byte_size;

	uint64_t                   dma_write_byte_size;
	int                        ur_weight;
	int                        level;
	uint32_t                   reserve0[32];

};

struct xpu_device_processes {
	int                        magic_version;
	int                        process_count;
	int                        devfile_id;
	uint32_t                   dma_time_us;
	struct xpu_proc            proc[100];

};

enum {
	MAX_DEVICE_NUM = 64,
	XPU_MAX_STRLEN = 64,
};

enum {
	XPU_XID0 = 0,
	XPU_XID1 = 1,
	XPU_XID2 = 2,
	XPU_XID3 = 3,
};

enum {
	XPU_ECC_SBE = 0,
	XPU_ECC_DBE = 1,
};

enum {
	QDIT_MODEL_SN = 0,
	QDIT_COORDINATE = 1,
	QDIT_HS_MEM = 2,
	QDIT_MAIN_MEM = 3,
	QDIT_USE_RATIO = 4,
	QDIT_DMA_RATIO = 5,
	QDIT_TEMPERATURE = 6,
	QDIT_STATE = 7,
	QDIT_FREQUENCY = 8,
	QDIT_POWER = 9,
	QDIT_FIRMWARE_VERSION = 10,
	QDIT_PROC_USE_RATIO = 11,
	QDIT_CG_HS_MEM = 12,
	QDIT_CG_MAIN_MEM = 13,
	QDIT_PROC_INCG = 14,
	QDIT_CU = 15,
	QDIT_CODEC = 16,
	QDIT_SRIOV_INFO = 17,
	QDIT_VIDEO_RATIO = 18,
	QDIT_VIDEO_FRAME_RATE = 19,
	QDIT_VIDEO_CLOCK = 20,
	QDIT_ECC_MODE = 21,
	QDIT_ECC_ERROR_COUNT = 22,
	QDIT_PN = 23,
};

enum {
	DBGM_IOC_ENABLE = 1,
	DBGM_IOC_DISABLE = 2,
	DBGM_IOC_START = 101,
	DBGM_IOC_STOP = 102,
	DBGM_IOC_MBOX_WRITE = 201,
	DBGM_IOC_MBOX_READ = 202,
	DBGM_IOC_RELAX = 301,
	DBGM_IOC_INUSE = 302,
};

enum {
	CXPU_CREATE_INSTANCE,
	CXPU_DESTROY_INSTANCE,
	CXPU_SET_INSTANCE_HSMEM_LIMIT,
	CXPU_SET_INSTANCE_MAINMEM_LIMIT,
	CXPU_SET_INSTANCE_CORE_LIMIT,
	CXPU_GET_INSTANCE_COUNT,
	CXPU_GET_MAX_INSTANCE_COUNT,
	CXPU_GET_INSTANCE_ID,
	CXPU_GET_HSMEM_INFO,
	CXPU_GET_MAINMEM_INFO,
	CXPU_GET_CORE_INFO,
	CXPU_GET_INSTANCE_HSMEM_INFO,
	CXPU_GET_INSTANCE_MAINMEM_INFO,
	CXPU_GET_INSTANCE_CORE_INFO,
};

enum {
	PROCESS_COMM_LEN = 16,
	DEVICE_MAX_PROCESS_PRINT_COUNT = 100,
	XPU_PD_NUM = 2,
	XPU_DEVNO_NUM = MAX_DEVICE_NUM * XPU_PD_NUM + 1,
	MAX_DEVINODE_NUM = MAX_DEVICE_NUM * XPU_PD_NUM,
	INODE_NUM = MAX_DEVINODE_NUM + 1,
	PCIE_BAR_NUM = 6,
	MAX_PARAM_DWORD_SIZE_KL1 = 29,
	MAX_PARAM_BYTE_SIZE_KL1 = MAX_PARAM_DWORD_SIZE_KL1 * 4,
	MAX_PARAM_DWORD_SIZE = 128,
	MAX_PARAM_BYTE_SIZE = MAX_PARAM_DWORD_SIZE * 4,
	XPD_CLUSTER_COUNT = 4,
	XPD_CDNN_COUNT = 4,
	KL2_CLUSTER_MAX_COUNT = 8,
	KL2_SDNN_MAX_COUNT = 6,
	KL2_ENCODER_MAX_COUNT = 3,
	KL2_DECODER_MAX_COUNT = 9,
	KL2_IMGPROC_MAX_COUNT = 6,
};

enum {
	XPU_MEM_CODE = XPU_USER_ALLOCABLE_MEM_COUNT,
	XPU_MEM_PARAM,
	XPU_MEM_PRINTF,
	XPU_MEM_RESERVED,
	XPU_MEM_COUNT,
};

enum {
	DEVINFO_MAGIC_V0 = 6545408,
};

enum {
	PROCINFO_MAGIC_V0 = 0x63f000,
};

enum {
	KL1_INBOUND_REGION_NUM = 4,
	KL1_OUTBOUND_REGION_NUM = 4,
	KL1_MAX_REGION_NUM = 4,
};

#endif /* KL_ABI_TYPES_H */
