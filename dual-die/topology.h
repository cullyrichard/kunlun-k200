// Die -> card topology, read from the XPU runtime.
//
// This is the block that belongs in the library as kunlun::topology(int) once
// the library is widened past two devices (MULTI-CARD.md): the backend needs
// the same grouping to pair dies on the same board, and the library already
// links XRE. Until that happens it lives here, header-only, so the bench can
// be extended without touching upstream. Moving it is a copy plus a rename.
//
// Attribute reads return -1 instead of aborting. The library's house style is
// fail-fast require(), but firmware support for BOARDID/ONBOARDID on this
// board is unknown and unmeasurable without a card, and a bench that cannot
// read the map must still run its measurements.

#pragma once

#include <xpu/defs.h>
#include <xpu/runtime.h>

#include <cstdint>
#include <cstdio>

namespace k200 {

struct DeviceTopology {
    int board = -1;         // XPUATTR_BOARDID: dies on one card share this
    int on_board = -1;      // XPUATTR_ONBOARDID: which die of that card
    int model = -1;         // XPUATTR_MODEL: K200 == 0
    long long memory = -1;  // XPUATTR_MEM_MAIN_CAPACITY, bytes
    long long l3 = -1;      // XPUATTR_MEM_L3_CAPACITY, bytes
    uint64_t pci = 0;
    bool pci_valid = false;

    bool grouped() const { return board >= 0; }
};

// -1 on any failure. All five attributes used here are small and non-negative,
// so -1 cannot collide with a real value; PCI address carries its own flag.
inline long long device_attribute(XPUDeviceAttr attr, int device) {
    uint64_t value = 0;
    if (xpu_device_get_attr(&value, attr, device) != XPU_SUCCESS) {
        return -1;
    }
    return static_cast<long long>(value);
}

inline DeviceTopology topology(int device) {
    DeviceTopology t;
    t.board = static_cast<int>(device_attribute(XPUATTR_BOARDID, device));
    t.on_board = static_cast<int>(device_attribute(XPUATTR_ONBOARDID, device));
    t.model = static_cast<int>(device_attribute(XPUATTR_MODEL, device));
    t.memory = device_attribute(XPUATTR_MEM_MAIN_CAPACITY, device);
    t.l3 = device_attribute(XPUATTR_MEM_L3_CAPACITY, device);
    const long long pci = device_attribute(XPUATTR_PCI_ADDRESS, device);
    if (pci >= 0) {
        t.pci = static_cast<uint64_t>(pci);
        t.pci_valid = true;
    }
    return t;
}

// runtime.h:106 -- bits [31:16] domain, [15:8] bus, [7:3] slot, [2:0] function.
inline const char * pci_address(const DeviceTopology & t, char * out, size_t n) {
    if (!t.pci_valid) {
        std::snprintf(out, n, "?");
        return out;
    }
    std::snprintf(out, n, "%04x:%02x:%02x.%x",
                  static_cast<unsigned>((t.pci >> 16) & 0xffff),
                  static_cast<unsigned>((t.pci >> 8) & 0xff),
                  static_cast<unsigned>((t.pci >> 3) & 0x1f),
                  static_cast<unsigned>(t.pci & 0x7));
    return out;
}

inline const char * model_name(const DeviceTopology & t) {
    return t.model < 0 ? "?" : xpu_device_model_str(t.model);
}

}  // namespace k200
