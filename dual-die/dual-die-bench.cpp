// Multi-die tests for the K200. Still at the dual-die/ path the plan refers to
// (MULTI-CARD.md), but no longer hardcoded to PD0 and PD1: it enumerates every
// die the runtime offers, prints the die -> card map, and runs each test across
// all of them. Four questions, in order of how much they decide:
//
//   0  Which dies sit on which card, from XPUATTR_BOARDID. Everything else is
//      read against this map: "two dies" means nothing until we know whether
//      they share a board.
//   A  peer() round-trip cost at activation size, for every ordered pair. This
//      decides whether tensor-parallel is viable at all -- a token needs ~64
//      syncs, so a 20us sync is free and a 500us sync is fatal -- and the
//      same-card vs cross-card split in the matrix decides whether it is
//      viable only within a card.
//   B  Do N dies deliver additive bandwidth? The tensor-parallel estimate
//      assumes N x 256 GB/s. Power (~150W per card under INT8), a shared
//      memory path, or a single host queue could make it 1.0x instead.
//   C  N-split GEMV across all dies. A column split needs only a gather, not a
//      reduce: Y[1,N] = X[1,K] . W[N,K] splits on N into disjoint outputs.
//      Only a K-split would need a real reduction.
//
// matmul() submits and wait() drains the selected device (k200.h:82), so B and
// C overlap by submitting to every die before draining any. native_q4_matmul
// cannot be used here: it completes before returning.
//
// peer() already drains the source and then the destination around the copy
// (k200.cpp:84-89), so timing the call alone times a full round trip.
//
// Gold check: Q4_0 weights are all 1.0 and X is all ones, so Y[n] == K.
//
// Needs at least two dies. With no card it prints SKIP and exits 0, so it is
// runnable now as a structure check and meaningful later unchanged.

#include <kunlun/k200.h>

#include "topology.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <type_traits>
#include <vector>

namespace {

constexpr int K = 4096;
constexpr int N = 4096;
constexpr int BATCH = 1;
constexpr int TRIALS = 20;
constexpr int WARMUP = 3;

// The library stores per-device counters in fixed-width public arrays, so the
// width of Statistics::matmuls *is* the number of devices it will accept:
// select() aborts the process above it (k200.cpp:64). Reading the ceiling off
// the ABI means this bench reports the limit instead of dying at it, and picks
// up the widening automatically once max_devices lands.
int library_devices() {
    int ceiling = static_cast<int>(
        std::extent<decltype(kunlun::Statistics::matmuls)>::value);
    // fake-runtime.cpp has no select() to abort, so the no-hardware harness
    // raises the ceiling to exercise die counts the real library cannot reach
    // yet. Setting this against the real library only moves the abort.
    if (const char * forced = std::getenv("K200_LIBRARY_DEVICES")) {
        ceiling = std::atoi(forced);
    }
    return ceiling;
}

constexpr size_t ACTIVATION_BYTES = 16u << 10;
constexpr size_t PEER_MAX_BYTES = 16u << 20;

using clk = std::chrono::steady_clock;

double ms_since(clk::time_point t0) {
    return std::chrono::duration<double, std::milli>(clk::now() - t0).count();
}

double median(std::vector<double> & v) {
    std::sort(v.begin(), v.end());
    return v.empty() ? 0.0 : v[v.size() / 2];
}

// Every rate and speedup below divides by a measured time. Under the
// no-hardware harness those are zero, and a column of "inf" reads like a bug
// rather than like the absence of a measurement.
double ratio(double numerator, double denominator) {
    return denominator > 0 ? numerator / denominator : 0.0;
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

size_t q4_bytes(int k, int n) {
    return static_cast<size_t>(n) * (k / 32) * 18;
}

struct DieBuffers {
    void * w = nullptr;
    void * x = nullptr;
    void * y = nullptr;
    size_t w_bytes = 0;
    size_t y_bytes = 0;
};

DieBuffers upload(int device, const std::vector<uint8_t> & w,
                  const std::vector<float> & x, int n) {
    DieBuffers b;
    b.w_bytes = w.size();
    b.y_bytes = static_cast<size_t>(BATCH) * n * sizeof(float);
    b.w = kunlun::allocate(device, b.w_bytes);
    b.x = kunlun::allocate(device, x.size() * sizeof(float));
    b.y = kunlun::allocate(device, b.y_bytes);
    kunlun::require(b.w && b.x && b.y);
    kunlun::select(device);
    kunlun::check(kunlun::copy(b.w, w.data(), w.size(), kunlun::host_to_device));
    kunlun::check(kunlun::copy(b.x, x.data(), x.size() * sizeof(float),
                               kunlun::host_to_device));
    return b;
}

void release(int device, DieBuffers & b) {
    kunlun::release(device, b.w, b.w_bytes);
    kunlun::release(device, b.x, static_cast<size_t>(K) * sizeof(float));
    kunlun::release(device, b.y, b.y_bytes);
}

// Kernels are per-die and non-copyable; only the dies in use get one, since an
// unused Kernel's destructor would still touch whatever device its Impl names.
std::vector<std::unique_ptr<kunlun::Kernel>> make_kernels(int dies) {
    std::vector<std::unique_ptr<kunlun::Kernel>> k;
    for (int i = 0; i < dies; ++i) {
        k.emplace_back(new kunlun::Kernel());
    }
    return k;
}

// ---------------------------------------------------------------- test 0

struct Map {
    std::vector<k200::DeviceTopology> die;
    std::vector<int> card;   // index into boards, per die
    std::vector<int> boards; // distinct board ids, in order of first appearance
    bool grouped = false;    // BOARDID readable on every die
};

Map survey(int dies) {
    Map m;
    m.grouped = true;
    for (int i = 0; i < dies; ++i) {
        m.die.push_back(k200::topology(i));
        if (!m.die.back().grouped()) {
            m.grouped = false;
        }
    }
    for (int i = 0; i < dies; ++i) {
        const int board = m.die[i].board;
        auto it = std::find(m.boards.begin(), m.boards.end(), board);
        if (it == m.boards.end()) {
            m.boards.push_back(board);
            it = m.boards.end() - 1;
        }
        m.card.push_back(static_cast<int>(it - m.boards.begin()));
    }
    return m;
}

void print_map(const Map & m) {
    std::printf("\n== 0: die -> card map ==\n");
    std::printf("%5s %8s %8s %9s %14s %11s %9s\n", "die", "model", "board",
                "on_board", "pci", "hbm_MB", "l3_MB");
    char pci[32];
    for (size_t i = 0; i < m.die.size(); ++i) {
        const auto & t = m.die[i];
        std::printf("%5zu %8s %8d %9d %14s %11.0f %9.1f\n", i,
                    k200::model_name(t), t.board, t.on_board,
                    k200::pci_address(t, pci, sizeof pci),
                    t.memory < 0 ? -1.0 : t.memory / 1048576.0,
                    t.l3 < 0 ? -1.0 : t.l3 / 1048576.0);
    }
    if (!m.grouped) {
        std::printf("BOARDID unreadable on at least one die: no grouping is\n"
                    "used below, test A's same-card vs cross-card split is not\n"
                    "available, and its size sweep runs on one arbitrary pair.\n");
        return;
    }
    std::printf("%zu card(s), %zu die(s):", m.boards.size(), m.die.size());
    for (size_t c = 0; c < m.boards.size(); ++c) {
        std::printf(" card%zu(board %d)=[", c, m.boards[c]);
        const char * sep = "";
        for (size_t i = 0; i < m.die.size(); ++i) {
            if (m.card[i] == static_cast<int>(c)) {
                std::printf("%s%zu", sep, i);
                sep = ",";
            }
        }
        std::printf("]");
    }
    std::printf("\n");
}

// ---------------------------------------------------------------- test A

double peer_median_us(int dst, void * dp, int src, const void * sp,
                      size_t bytes) {
    for (int i = 0; i < WARMUP; ++i) {
        kunlun::peer(dst, dp, src, sp, bytes);
    }
    std::vector<double> t;
    t.reserve(TRIALS);
    for (int i = 0; i < TRIALS; ++i) {
        auto t0 = clk::now();
        kunlun::peer(dst, dp, src, sp, bytes);
        t.push_back(ms_since(t0) * 1000.0);
    }
    return median(t);
}

void peer_sweep(const char * label, int src, int dst,
                const std::vector<void *> & buf) {
    std::printf("\n-- A2: %s, %d -> %d, size sweep --\n", label, src, dst);
    std::printf("%10s %12s %10s\n", "bytes", "median_us", "GB/s");
    const size_t sizes[] = {4u << 10, 16u << 10, 64u << 10, 256u << 10,
                            1u << 20, 4u << 20, 16u << 20};
    for (size_t bytes : sizes) {
        const double us = peer_median_us(dst, buf[dst], src, buf[src], bytes);
        std::printf("%10zu %12.2f %10.2f%s\n", bytes, us,
                    ratio(bytes, us * 1e-6) / 1e9,
                    bytes == ACTIVATION_BYTES ? "   <- activation vector" : "");
    }
}

void test_peer_matrix(int dies, const Map & m) {
    std::printf("\n== A: peer() round-trip, every ordered pair ==\n");

    std::vector<void *> buf(dies, nullptr);
    for (int i = 0; i < dies; ++i) {
        buf[i] = kunlun::allocate(i, PEER_MAX_BYTES);
        kunlun::require(buf[i]);
    }

    std::printf("-- A1: median us at %zu bytes (one activation vector) --\n",
                ACTIVATION_BYTES);
    std::printf("%7s", "src\\dst");
    for (int d = 0; d < dies; ++d) {
        std::printf(" %10d", d);
    }
    std::printf("\n");

    std::vector<double> same_card, cross_card;
    for (int s = 0; s < dies; ++s) {
        std::printf("%7d", s);
        for (int d = 0; d < dies; ++d) {
            if (s == d) {
                std::printf(" %10s", "-");
                continue;
            }
            const double us = peer_median_us(d, buf[d], s, buf[s],
                                             ACTIVATION_BYTES);
            std::printf(" %10.2f", us);
            if (m.grouped) {
                (m.card[s] == m.card[d] ? same_card : cross_card).push_back(us);
            }
        }
        std::printf("\n");
    }

    // Same-die reference: a plain device-to-device copy, no peer path at all.
    kunlun::select(0);
    std::vector<double> local;
    for (int i = 0; i < WARMUP + TRIALS; ++i) {
        auto t0 = clk::now();
        kunlun::check(kunlun::copy(static_cast<uint8_t *>(buf[0]) +
                                       PEER_MAX_BYTES / 2,
                                   buf[0], ACTIVATION_BYTES,
                                   kunlun::device_to_device));
        kunlun::check(kunlun::wait());
        if (i >= WARMUP) {
            local.push_back(ms_since(t0) * 1000.0);
        }
    }
    std::printf("same-die d2d copy, die 0: %.2f us (floor for comparison)\n",
                median(local));

    // -1 for a kind the installed topology has no pair of: one card has no
    // cross-card pair, and four single-die cards would have no same-card pair.
    const double same_us = same_card.empty() ? -1.0 : median(same_card);
    const double cross_us = cross_card.empty() ? -1.0 : median(cross_card);
    if (same_us >= 0) {
        std::printf("same-card median : %8.2f us (%zu pairs)\n", same_us,
                    same_card.size());
    }
    if (cross_us >= 0) {
        std::printf("cross-card median: %8.2f us (%zu pairs)\n", cross_us,
                    cross_card.size());
    }
    std::printf("K200PERF {\"test\":\"peer_matrix\",\"dies\":%d,"
                "\"grouped\":%d,\"same_card_us\":%.2f,\"cross_card_us\":%.2f,"
                "\"local_d2d_us\":%.2f}\n",
                dies, m.grouped ? 1 : 0, same_us, cross_us, median(local));

    // Sweep one representative pair of each kind the map actually has.
    int same_s = -1, same_d = -1, cross_s = -1, cross_d = -1;
    for (int s = 0; s < dies && (same_s < 0 || cross_s < 0); ++s) {
        for (int d = 0; d < dies; ++d) {
            if (s == d) {
                continue;
            }
            const bool same = !m.grouped || m.card[s] == m.card[d];
            if (same && same_s < 0) {
                same_s = s;
                same_d = d;
            } else if (!same && cross_s < 0) {
                cross_s = s;
                cross_d = d;
            }
        }
    }
    if (same_s >= 0) {
        peer_sweep(m.grouped ? "same card" : "ungrouped", same_s, same_d, buf);
    }
    if (cross_s >= 0) {
        peer_sweep("cross card", cross_s, cross_d, buf);
    }

    std::printf("\nVerdict: a token needs ~64 of these. <100us/sync keeps\n"
                "tensor-parallel worthwhile; >300us eats the gain. If only the\n"
                "same-card figure clears it, pair dies per card and stop there.\n");

    for (int i = 0; i < dies; ++i) {
        kunlun::release(i, buf[i], PEER_MAX_BYTES);
    }
}

// ---------------------------------------------------------------- test B

void test_overlap(int dies) {
    std::printf("\n== B: same GEMV on n dies, serial vs overlapped ==\n");

    auto w = q4_ones(K, N);
    std::vector<float> x(K, 1.0f);
    std::vector<DieBuffers> b;
    for (int i = 0; i < dies; ++i) {
        b.push_back(upload(i, w, x, N));
    }
    auto kern = make_kernels(dies);

    auto submit = [&](int device) {
        kunlun::matmul(*kern[device], device, b[device].y, b[device].w,
                       b[device].x, K, N, BATCH, kunlun::WeightType::q4_0);
    };
    auto drain = [](int device) {
        kunlun::select(device);
        kunlun::check(kunlun::wait());
    };

    for (int i = 0; i < WARMUP; ++i) {
        for (int d = 0; d < dies; ++d) {
            submit(d);
            drain(d);
        }
    }

    std::printf("%5s %12s %12s %10s\n", "dies", "serial_ms", "overlap_ms",
                "speedup");
    double single = 0;
    for (int n = 1; n <= dies; ++n) {
        std::vector<double> serial, overlap;
        for (int i = 0; i < TRIALS; ++i) {
            auto t0 = clk::now();
            for (int d = 0; d < n; ++d) {
                submit(d);
                drain(d);
            }
            serial.push_back(ms_since(t0));

            t0 = clk::now();
            for (int d = 0; d < n; ++d) {
                submit(d);
            }
            for (int d = 0; d < n; ++d) {
                drain(d);
            }
            overlap.push_back(ms_since(t0));
        }
        const double ser = median(serial), ov = median(overlap);
        if (n == 1) {
            single = ser;
        }
        std::printf("%5d %12.4f %12.4f %10.2fx\n", n, ser, ov, ratio(ser, ov));
        std::printf("K200PERF {\"test\":\"overlap\",\"dies\":%d,"
                    "\"serial_ms\":%.4f,\"overlap_ms\":%.4f,\"speedup\":%.3f,"
                    "\"vs_single\":%.3f}\n",
                    n, ser, ov, ratio(ser, ov), ratio(ov, single));
    }
    std::printf("speedup n.00 = additive bandwidth, 1.00 = fully contended.\n"
                "overlap_ms staying at the n=1 figure is the additive case.\n");

    for (int i = 0; i < dies; ++i) {
        release(i, b[i]);
    }
}

// ---------------------------------------------------------------- test C

void test_n_split(int dies) {
    std::printf("\n== C: N-split GEMV across %d dies (gather, no reduce) ==\n",
                dies);
    if (N % dies != 0) {
        std::printf("SKIP: N=%d is not divisible by %d dies.\n", N, dies);
        return;
    }

    const int chunk = N / dies;
    auto w_chunk = q4_ones(K, chunk);
    auto w_full = q4_ones(K, N);
    std::vector<float> x(K, 1.0f);

    std::vector<DieBuffers> b;
    for (int i = 0; i < dies; ++i) {
        b.push_back(upload(i, w_chunk, x, chunk));
    }
    // Single-die full-N reference, on die 0, for the ratio that matters.
    DieBuffers full = upload(0, w_full, x, N);
    // Gather target on die 0: the full N outputs.
    const size_t full_bytes = static_cast<size_t>(N) * sizeof(float);
    void * gathered = kunlun::allocate(0, full_bytes);
    kunlun::require(gathered);
    auto kern = make_kernels(dies);
    kunlun::Kernel reference;

    const size_t chunk_bytes = static_cast<size_t>(chunk) * sizeof(float);
    auto split = [&] {
        for (int d = 0; d < dies; ++d) {
            kunlun::matmul(*kern[d], d, b[d].y, b[d].w, b[d].x, K, chunk, BATCH,
                           kunlun::WeightType::q4_0);
        }
        for (int d = 0; d < dies; ++d) {
            kunlun::select(d);
            kunlun::check(kunlun::wait());
        }
        // Die 0's own chunk never leaves the die, so it is a local copy -- and
        // it has to be issued with die 0 selected, both pointers being die 0's.
        kunlun::select(0);
        kunlun::check(kunlun::copy(gathered, b[0].y, chunk_bytes,
                                   kunlun::device_to_device));
        for (int d = 1; d < dies; ++d) {
            kunlun::peer(0,
                         static_cast<uint8_t *>(gathered) + d * chunk_bytes, d,
                         b[d].y, chunk_bytes);
        }
        kunlun::select(0);
        kunlun::check(kunlun::wait());
    };
    auto whole = [&] {
        kunlun::matmul(reference, 0, full.y, full.w, full.x, K, N, BATCH,
                       kunlun::WeightType::q4_0);
        kunlun::select(0);
        kunlun::check(kunlun::wait());
    };

    for (int i = 0; i < WARMUP; ++i) {
        split();
        whole();
    }
    std::vector<double> ts, tw;
    for (int i = 0; i < TRIALS; ++i) {
        auto t0 = clk::now();
        split();
        ts.push_back(ms_since(t0));
        t0 = clk::now();
        whole();
        tw.push_back(ms_since(t0));
    }

    std::vector<float> y(N);
    kunlun::select(0);
    kunlun::check(kunlun::copy(y.data(), gathered, full_bytes,
                               kunlun::device_to_host));
    int bad = 0;
    for (float v : y) {
        if (v < K - 1 || v > K + 1) {
            ++bad;
        }
    }

    const double split_ms = median(ts), whole_ms = median(tw);
    std::printf("one die, full N=%d  : %8.4f ms\n", N, whole_ms);
    std::printf("%d dies, %d each+gather: %8.4f ms\n", dies, chunk, split_ms);
    std::printf("split speedup       : %8.2fx (gather included)\n",
                ratio(whole_ms, split_ms));
    std::printf("gold check          : %s (%d/%d outputs != K)\n",
                bad == 0 ? "PASS" : "FAIL", bad, N);
    std::printf("K200PERF {\"test\":\"n_split\",\"dies\":%d,"
                "\"split_ms\":%.4f,\"whole_ms\":%.4f,\"speedup\":%.3f,"
                "\"bad\":%d}\n",
                dies, split_ms, whole_ms, ratio(whole_ms, split_ms), bad);

    kunlun::release(0, gathered, full_bytes);
    release(0, full);
    for (int i = 0; i < dies; ++i) {
        release(i, b[i]);
    }
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

    const int ceiling = library_devices();
    int dies = count;
    if (count > ceiling) {
        dies = ceiling;
        std::printf("NOTE: %d dies present, library accepts %d. Testing dies\n"
                    "0-%d only -- select() aborts above its limit. Widen\n"
                    "max_devices in kunlunK200_lib (every [2] and id < 2, and\n"
                    "Statistics::matmuls) to measure the rest; MULTI-CARD.md\n"
                    "lists the sites.\n",
                    count, ceiling, dies - 1);
    }
    if (dies < 2) {
        std::printf("SKIP: a ceiling of %d leaves fewer than two usable dies.\n",
                    ceiling);
        return 0;
    }
    std::printf("device_count = %d, testing %d, library ceiling %d\n", count,
                dies, ceiling);
    std::printf("shape K=%d N=%d batch=%d, Q4_0 weights = %zu bytes/die\n", K, N,
                BATCH, q4_bytes(K, N));

    std::lock_guard<std::recursive_mutex> guard(kunlun::runtime_mutex());
    const Map m = survey(dies);
    print_map(m);
    test_peer_matrix(dies, m);
    test_overlap(dies);
    test_n_split(dies);
    std::printf("\ndone\n");
    return 0;
}
