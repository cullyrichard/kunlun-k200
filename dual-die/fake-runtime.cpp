// A host stand-in for the parts of libkunlunk200 and libxpurt that the
// multi-die bench calls, so the bench's own logic can be run and checked with
// no K200 present. Link this instead of the two real libraries; the bench
// source is compiled unchanged.
//
// It is not a simulator and makes no attempt to be: every call returns
// immediately, so every millisecond figure the bench prints under this runtime
// is noise. What it does check is everything that is not timing --
//
//   * the die -> card grouping, over any die count and with BOARDID failing
//   * the ordered-pair loop, the same-card/cross-card split, pair selection
//   * the N-split gather offsets, end to end, through the Q4_0 gold check
//   * device ownership of every pointer handed to copy, peer and matmul
//
// -- and that last one is the point. The real library takes a device id and
// trusts the pointers; a copy issued with the wrong die selected is accepted
// and silently wrong. Here it aborts with the call site, which is how the
// gather in test C was found to be issuing its local copy on the wrong die.
//
// Knobs: K200_FAKE_DIES (default 4), K200_FAKE_DIES_PER_CARD (default 2),
// K200_FAKE_NO_BOARDID=1 to make the BOARDID read fail.

#include <kunlun/k200.h>

#include <xpu/defs.h>
#include <xpu/runtime.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace {

int environment(const char * name, int fallback) {
    const char * value = std::getenv(name);
    if (!value || !*value) {
        return fallback;
    }
    return std::atoi(value);
}

int fake_dies() {
    static const int dies = std::max(1, environment("K200_FAKE_DIES", 4));
    return dies;
}

int dies_per_card() {
    static const int per = std::max(1, environment("K200_FAKE_DIES_PER_CARD", 2));
    return per;
}

void banner() {
    static bool printed = false;
    if (!printed) {
        printed = true;
        std::fprintf(stderr,
                     "FAKE RUNTIME: %d dies, %d per card. Every call returns "
                     "immediately, so all\ntimings below are noise -- this run "
                     "checks the bench's logic, not the hardware.\n",
                     fake_dies(), dies_per_card());
        std::fflush(stderr);
    }
}

[[noreturn]] void fail(const char * what) {
    std::fprintf(stderr, "FAKE RUNTIME abort: %s\n", what);
    std::fflush(stderr);
    std::abort();
}

struct Allocation {
    int device;
    uint8_t * base;
    size_t bytes;
};

std::vector<Allocation> allocations;
int current_device = -1;

// Which die owns [ptr, ptr+bytes)? -1 if it is not inside one allocation.
int owner(const void * ptr, size_t bytes) {
    const auto * p = static_cast<const uint8_t *>(ptr);
    for (const Allocation & a : allocations) {
        if (p >= a.base && p + bytes <= a.base + a.bytes) {
            return a.device;
        }
    }
    return -1;
}

void must_own(int device, const void * ptr, size_t bytes, const char * what) {
    const int holder = owner(ptr, bytes);
    if (holder != device) {
        std::fprintf(stderr,
                     "FAKE RUNTIME abort: %s expects die %d to own %p+%zu, "
                     "but it is owned by die %d\n",
                     what, device, ptr, bytes, holder);
        std::fflush(stderr);
        std::abort();
    }
}

float half_to_float(uint16_t h) {
    const uint32_t sign = static_cast<uint32_t>(h >> 15) << 31;
    const uint32_t exponent = (h >> 10) & 0x1f;
    const uint32_t mantissa = h & 0x3ff;
    uint32_t bits;
    if (exponent == 0) {
        if (mantissa == 0) {
            bits = sign;
        } else {  // subnormal: renormalise
            uint32_t e = 127 - 15 + 1, m = mantissa;
            while (!(m & 0x400)) {
                m <<= 1;
                --e;
            }
            bits = sign | (e << 23) | ((m & 0x3ff) << 13);
        }
    } else if (exponent == 0x1f) {
        bits = sign | 0x7f800000u | (mantissa << 13);
    } else {
        bits = sign | ((exponent - 15 + 127) << 23) | (mantissa << 13);
    }
    float out;
    std::memcpy(&out, &bits, sizeof out);
    return out;
}

// Q4_0: 18-byte blocks of 32 values -- F16 scale, then 16 bytes of nibble
// pairs, low nibble element i and high nibble element i+16, each offset by 8.
void q4_gemm(float * y, const uint8_t * w, const float * x, int k, int n,
             int batch) {
    const int blocks = k / 32;
    for (int b = 0; b < batch; ++b) {
        for (int row = 0; row < n; ++row) {
            const uint8_t * block = w + static_cast<size_t>(row) * blocks * 18;
            float sum = 0;
            for (int i = 0; i < blocks; ++i, block += 18) {
                uint16_t raw;
                std::memcpy(&raw, block, sizeof raw);
                const float scale = half_to_float(raw);
                const float * xb = x + static_cast<size_t>(b) * k + i * 32;
                for (int j = 0; j < 16; ++j) {
                    const uint8_t pair = block[2 + j];
                    sum += (static_cast<int>(pair & 0xf) - 8) * scale * xb[j];
                    sum += (static_cast<int>(pair >> 4) - 8) * scale * xb[j + 16];
                }
            }
            y[static_cast<size_t>(b) * n + row] = sum;
        }
    }
}

}  // namespace

namespace kunlun {

std::recursive_mutex & runtime_mutex() {
    static std::recursive_mutex mutex;
    return mutex;
}

void check(int status) {
    if (status) {
        fail("check() on a nonzero status");
    }
}

void require(bool condition) {
    if (!condition) {
        fail("require() failed");
    }
}

void diagnostic_tensor(const char *, const int64_t *) {}

int device_count(int * count) {
    banner();
    *count = fake_dies();
    return 0;
}

void select(int device) {
    require(device >= 0 && device < fake_dies());
    current_device = device;
}

int wait() {
    require(current_device >= 0);
    return 0;
}

int copy(void * dst, const void * src, size_t bytes, CopyKind kind) {
    require(current_device >= 0);
    switch (kind) {
        case host_to_device:
            must_own(current_device, dst, bytes, "copy(host_to_device) dst");
            break;
        case device_to_host:
            must_own(current_device, src, bytes, "copy(device_to_host) src");
            break;
        case device_to_device:
            must_own(current_device, dst, bytes, "copy(device_to_device) dst");
            must_own(current_device, src, bytes, "copy(device_to_device) src");
            break;
        default:
            fail("copy() with an unknown kind");
    }
    std::memcpy(dst, src, bytes);
    return 0;
}

void peer(int dst_device, void * dst, int src_device, const void * src,
          size_t bytes) {
    require(dst_device >= 0 && dst_device < fake_dies());
    require(src_device >= 0 && src_device < fake_dies());
    must_own(dst_device, dst, bytes, "peer() dst");
    must_own(src_device, src, bytes, "peer() src");
    std::memcpy(dst, src, bytes);
    // The real peer() leaves the destination selected (k200.cpp:84-89).
    current_device = dst_device;
}

void * allocate(int device, size_t bytes) {
    require(device >= 0 && device < fake_dies());
    const size_t size = std::max<size_t>(bytes, 256);
    auto * base = static_cast<uint8_t *>(std::calloc(size, 1));
    if (!base) {
        return nullptr;
    }
    allocations.push_back({device, base, size});
    return base;
}

void release(int device, void * ptr, size_t bytes) {
    if (!ptr) {
        return;
    }
    must_own(device, ptr, std::max<size_t>(bytes, 1), "release()");
    auto it = std::find_if(allocations.begin(), allocations.end(),
                           [&](const Allocation & a) { return a.base == ptr; });
    if (it == allocations.end()) {
        fail("release() of a pointer that is not an allocation base");
    }
    allocations.erase(it);
    std::free(ptr);
}

size_t allocated_bytes(int device) {
    size_t total = 0;
    for (const Allocation & a : allocations) {
        if (a.device == device) {
            total += a.bytes;
        }
    }
    return total;
}

struct Kernel::Impl {
    int device = -1;
};

Kernel::Kernel() : impl_(new Impl{}) {}
Kernel::~Kernel() { delete impl_; }

void matmul(Kernel & kernel, int device, void * y, const void * w,
            const void * x, int k, int n, int batch, WeightType type,
            const float *, const float *, int) {
    require(device >= 0 && device < fake_dies());
    require(type == WeightType::q4_0);
    require(k > 0 && n > 0 && batch > 0 && k % 32 == 0);
    const size_t w_bytes = static_cast<size_t>(n) * (k / 32) * 18;
    const size_t x_bytes = static_cast<size_t>(batch) * k * sizeof(float);
    const size_t y_bytes = static_cast<size_t>(batch) * n * sizeof(float);
    must_own(device, w, w_bytes, "matmul() weights");
    must_own(device, x, x_bytes, "matmul() input");
    must_own(device, y, y_bytes, "matmul() output");
    kernel.impl_->device = device;
    q4_gemm(static_cast<float *>(y), static_cast<const uint8_t *>(w),
            static_cast<const float *>(x), k, n, batch);
}

}  // namespace kunlun

// ------------------------------------------------------------------ xpurt

int xpu_device_get_attr(uint64_t * value, XPUDeviceAttr attr, int devid) {
    if (!value || devid < 0 || devid >= fake_dies()) {
        return XPUERR_INVALID_PARAM;
    }
    const int card = devid / dies_per_card();
    switch (attr) {
        case XPUATTR_BOARDID:
            if (environment("K200_FAKE_NO_BOARDID", 0)) {
                return XPUERR_INVALID_PARAM;
            }
            *value = static_cast<uint64_t>(card);
            return XPU_SUCCESS;
        case XPUATTR_ONBOARDID:
            *value = static_cast<uint64_t>(devid % dies_per_card());
            return XPU_SUCCESS;
        case XPUATTR_DEVID:
            *value = static_cast<uint64_t>(devid);
            return XPU_SUCCESS;
        case XPUATTR_MODEL:
            *value = K200;
            return XPU_SUCCESS;
        case XPUATTR_MEM_MAIN_CAPACITY:
            *value = 8ull << 30;
            return XPU_SUCCESS;
        case XPUATTR_MEM_L3_CAPACITY:
            *value = 64ull << 20;
            return XPU_SUCCESS;
        case XPUATTR_PCI_ADDRESS:
            // domain 0, one bus per card, slot 0, function = die on card.
            *value = (static_cast<uint64_t>(0x17 + card) << 8) |
                     static_cast<uint64_t>(devid % dies_per_card());
            return XPU_SUCCESS;
        default:
            return XPUERR_INVALID_PARAM;
    }
}
