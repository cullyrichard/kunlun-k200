// Dual-die (PD0 + PD1) tests for the K200. Three questions, in order of how
// much they decide:
//
//   A  peer() round-trip cost at activation size. This decides whether
//      tensor-parallel is viable at all: a token needs ~64 syncs, so a 20us
//      sync is free and a 500us sync is fatal.
//   B  Do two dies deliver additive bandwidth? The 1.8x tensor-parallel
//      estimate assumes 2 x 256 GB/s. Power (~150W under INT8) or a shared
//      memory path could make it 1.0x instead.
//   C  N-split GEMV across both dies. A column split needs only a gather,
//      not a reduce: Y[1,N] = X[1,K] . W[N,K] splits on N into disjoint
//      outputs. Only a K-split would need a real reduction.
//
// matmul() submits and wait() drains the selected device (k200.h:82), so B and
// C overlap by submitting to both dies before draining either. native_q4_matmul
// cannot be used here: it completes before returning.
//
// Gold check: Q4_0 weights are all 1.0 and X is all ones, so Y[n] == K.
//
// Needs both PDs. With no card it prints SKIP and exits 0, so it is runnable
// now as a structure check and meaningful later unchanged.

#include <kunlun/k200.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

namespace {

constexpr int K = 4096;
constexpr int N = 4096;
constexpr int BATCH = 1;
constexpr int TRIALS = 20;
constexpr int WARMUP = 3;

using clk = std::chrono::steady_clock;

double ms_since(clk::time_point t0) {
    return std::chrono::duration<double, std::milli>(clk::now() - t0).count();
}

double median(std::vector<double> & v) {
    std::sort(v.begin(), v.end());
    return v.empty() ? 0.0 : v[v.size() / 2];
}

// Q4_0: 18-byte blocks, little-endian F16 scale then 16 bytes of paired
// nibbles. scale 1.0 and nibble 9 (offset-8) give a weight of exactly 1.
std::vector<uint8_t> q4_ones(int k, int n) {
    const int blocks = k / 32;
    std::vector<uint8_t> w(static_cast<size_t>(n) * blocks * 18);
    for (size_t off = 0; off < w.size(); off += 18) {
        w[off] = 0x00;
        w[off + 1] = 0x3c;  // F16 1.0
        std::memset(&w[off + 2], 0x99, 16);
    }
    return w;
}

struct DieBuffers {
    void * w = nullptr;
    void * x = nullptr;
    void * y = nullptr;
};

size_t q4_bytes(int k, int n) {
    return static_cast<size_t>(n) * (k / 32) * 18;
}

DieBuffers upload(int device, const std::vector<uint8_t> & w,
                  const std::vector<float> & x, int n) {
    DieBuffers b;
    b.w = kunlun::allocate(device, w.size());
    b.x = kunlun::allocate(device, x.size() * sizeof(float));
    b.y = kunlun::allocate(device, static_cast<size_t>(BATCH) * n * sizeof(float));
    kunlun::select(device);
    kunlun::check(kunlun::copy(b.w, w.data(), w.size(), kunlun::host_to_device));
    kunlun::check(kunlun::copy(b.x, x.data(), x.size() * sizeof(float),
                               kunlun::host_to_device));
    return b;
}

void release(int device, DieBuffers & b, size_t w_bytes, int n) {
    kunlun::release(device, b.w, w_bytes);
    kunlun::release(device, b.x, static_cast<size_t>(K) * sizeof(float));
    kunlun::release(device, b.y, static_cast<size_t>(BATCH) * n * sizeof(float));
}

// ---------------------------------------------------------------- test A

void test_peer_latency() {
    std::printf("\n== A: peer() round-trip, PD0 -> PD1 ==\n");
    std::printf("%10s %12s %12s %10s\n", "bytes", "median_us", "min_us", "GB/s");

    const size_t sizes[] = {4u << 10, 16u << 10, 64u << 10, 256u << 10,
                            1u << 20, 4u << 20, 16u << 20};
    for (size_t bytes : sizes) {
        void * src = kunlun::allocate(0, bytes);
        void * dst = kunlun::allocate(1, bytes);

        for (int i = 0; i < WARMUP; ++i) {
            kunlun::peer(1, dst, 0, src, bytes);
            kunlun::select(1);
            kunlun::check(kunlun::wait());
        }

        std::vector<double> t;
        t.reserve(TRIALS);
        for (int i = 0; i < TRIALS; ++i) {
            auto t0 = clk::now();
            kunlun::peer(1, dst, 0, src, bytes);
            kunlun::select(1);
            kunlun::check(kunlun::wait());
            t.push_back(ms_since(t0) * 1000.0);  // us
        }
        double med = median(t);
        std::printf("%10zu %12.2f %12.2f %10.2f%s\n", bytes, med, t.front(),
                    bytes / (med * 1e-6) / 1e9,
                    bytes == (16u << 10) ? "   <- activation vector" : "");

        kunlun::release(0, src, bytes);
        kunlun::release(1, dst, bytes);
    }
    std::printf("Verdict: a token needs ~64 of these. <100us/sync keeps\n"
                "tensor-parallel worthwhile; >300us eats the gain.\n");
}

// ---------------------------------------------------------------- test B

void test_dual_die_bandwidth() {
    std::printf("\n== B: same GEMV on both dies, serial vs overlapped ==\n");

    auto w = q4_ones(K, N);
    std::vector<float> x(K, 1.0f);
    DieBuffers b0 = upload(0, w, x, N);
    DieBuffers b1 = upload(1, w, x, N);
    kunlun::Kernel k0, k1;

    auto submit = [&](kunlun::Kernel & kern, int device, DieBuffers & b) {
        kunlun::matmul(kern, device, b.y, b.w, b.x, K, N, BATCH,
                       kunlun::WeightType::q4_0);
    };
    auto drain = [&](int device) {
        kunlun::select(device);
        kunlun::check(kunlun::wait());
    };

    for (int i = 0; i < WARMUP; ++i) {
        submit(k0, 0, b0);
        drain(0);
        submit(k1, 1, b1);
        drain(1);
    }

    std::vector<double> one, serial, overlap;
    for (int i = 0; i < TRIALS; ++i) {
        auto t0 = clk::now();
        submit(k0, 0, b0);
        drain(0);
        one.push_back(ms_since(t0));

        t0 = clk::now();
        submit(k0, 0, b0);
        drain(0);
        submit(k1, 1, b1);
        drain(1);
        serial.push_back(ms_since(t0));

        t0 = clk::now();
        submit(k0, 0, b0);
        submit(k1, 1, b1);
        drain(0);
        drain(1);
        overlap.push_back(ms_since(t0));
    }

    double m1 = median(one), ms_ = median(serial), mo = median(overlap);
    std::printf("single die          : %8.4f ms\n", m1);
    std::printf("two dies, serial    : %8.4f ms\n", ms_);
    std::printf("two dies, overlapped: %8.4f ms\n", mo);
    std::printf("overlap speedup     : %8.2fx  (2.00 = additive bandwidth,\n"
                "                                1.00 = fully contended)\n",
                ms_ / mo);
    std::printf("K200PERF {\"test\":\"dual_die_overlap\",\"single_ms\":%.4f,"
                "\"serial_ms\":%.4f,\"overlap_ms\":%.4f,\"speedup\":%.3f}\n",
                m1, ms_, mo, ms_ / mo);

    release(0, b0, w.size(), N);
    release(1, b1, w.size(), N);
}

// ---------------------------------------------------------------- test C

void test_n_split() {
    std::printf("\n== C: N-split GEMV across both dies (gather, no reduce) ==\n");

    const int half = N / 2;
    auto w_half = q4_ones(K, half);
    std::vector<float> x(K, 1.0f);
    DieBuffers b0 = upload(0, w_half, x, half);
    DieBuffers b1 = upload(1, w_half, x, half);
    // Gather target on PD0: the full N outputs.
    void * gathered = kunlun::allocate(0, static_cast<size_t>(N) * sizeof(float));
    kunlun::Kernel k0, k1;

    const size_t half_bytes = static_cast<size_t>(half) * sizeof(float);
    auto run = [&] {
        kunlun::matmul(k0, 0, b0.y, b0.w, b0.x, K, half, BATCH,
                       kunlun::WeightType::q4_0);
        kunlun::matmul(k1, 1, b1.y, b1.w, b1.x, K, half, BATCH,
                       kunlun::WeightType::q4_0);
        kunlun::select(0);
        kunlun::check(kunlun::wait());
        kunlun::select(1);
        kunlun::check(kunlun::wait());
        kunlun::check(kunlun::copy(gathered, b0.y, half_bytes,
                                   kunlun::device_to_device));
        kunlun::peer(0, static_cast<uint8_t *>(gathered) + half_bytes, 1, b1.y,
                     half_bytes);
        kunlun::select(0);
        kunlun::check(kunlun::wait());
    };

    for (int i = 0; i < WARMUP; ++i) {
        run();
    }
    std::vector<double> t;
    for (int i = 0; i < TRIALS; ++i) {
        auto t0 = clk::now();
        run();
        t.push_back(ms_since(t0));
    }

    std::vector<float> y(N);
    kunlun::select(0);
    kunlun::check(kunlun::copy(y.data(), gathered, y.size() * sizeof(float),
                               kunlun::device_to_host));
    int bad = 0;
    for (float v : y) {
        if (v < K - 1 || v > K + 1) {
            ++bad;
        }
    }
    std::printf("split median        : %8.4f ms (includes gather)\n", median(t));
    std::printf("gold check          : %s (%d/%d outputs != K)\n",
                bad == 0 ? "PASS" : "FAIL", bad, N);
    std::printf("K200PERF {\"test\":\"n_split\",\"median_ms\":%.4f,\"bad\":%d}\n",
                median(t), bad);

    kunlun::release(0, gathered, static_cast<size_t>(N) * sizeof(float));
    release(0, b0, w_half.size(), half);
    release(1, b1, w_half.size(), half);
}

}  // namespace

int main() {
    int count = 0;
    if (kunlun::device_count(&count) != 0 || count < 2) {
        std::printf("SKIP: need two PDs, device_count reported %d.\n", count);
        std::printf("Both dies of one K200 must enumerate. With no card this is\n"
                    "the expected result and the binary is only structure-checked.\n");
        return 0;
    }
    std::printf("device_count = %d\n", count);
    std::printf("shape K=%d N=%d batch=%d, Q4_0 weights = %zu bytes/die\n", K, N,
                BATCH, q4_bytes(K, N));

    std::lock_guard<std::recursive_mutex> guard(kunlun::runtime_mutex());
    test_peer_latency();
    test_dual_die_bandwidth();
    test_n_split();
    std::printf("\ndone\n");
    return 0;
}
