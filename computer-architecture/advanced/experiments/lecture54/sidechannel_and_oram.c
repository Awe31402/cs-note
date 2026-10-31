/*
 * Lecture 54 Experiment:
 *   1. Real x86 Flush+Reload Cache Side-Channel Attack on RSA Modular
 *      Exponentiation (Figure 13.12b) + Constant-Time Countermeasure
 *   2. Path ORAM Simulation (Stefanov et al. / Textbook §13.3.5 p.696):
 *      Stash occupancy, bandwidth overhead, and DRAM address-bus entropy
 *   3. Live CPU Hardware Vulnerability & Mitigation Report (/sys/.../vulnerabilities)
 *
 * Build & run (binary in /tmp only):
 *   gcc -O2 -Wall -Wextra sidechannel_and_oram.c -o /tmp/sidechannel_and_oram -lm
 *   taskset -c 0 /tmp/sidechannel_and_oram && rm -f /tmp/sidechannel_and_oram
 */
#include <dirent.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <x86intrin.h>

/* Shared page-aligned probe buffer simulating shared library cache lines */
static uint8_t __attribute__((aligned(4096))) shared_lib[8 * 4096];
#define MUL_GADGET_LINE (&shared_lib[3 * 4096])
#define DUMMY_LINE      (&shared_lib[5 * 4096])

static inline uint64_t timed_read(const volatile uint8_t *addr) {
    unsigned int aux;
    _mm_mfence();
    _mm_lfence();
    uint64_t t0 = __rdtscp(&aux);
    _mm_lfence();
    (void)*addr;
    _mm_lfence();
    uint64_t t1 = __rdtscp(&aux);
    return t1 - t0;
}

/* Vulnerable 1-step of RSA modexp (Figure 13.12b): data-dependent access */
static uint64_t rsa_step_vulnerable(uint64_t *z, uint64_t *x, uint64_t n, int bit) {
    if (bit == 1) {
        volatile uint8_t g = *MUL_GADGET_LINE; /* Touches shared cache line iff bit==1 */
        (void)g;
        *z = (uint64_t)(((__uint128_t)(*z) * (*x)) % n);
    }
    *x = (uint64_t)(((__uint128_t)(*x) * (*x)) % n);
    return *z;
}

/* Constant-access countermeasure: always touches both lines + branchless select */
static uint64_t rsa_step_consttime(uint64_t *z, uint64_t *x, uint64_t n, int bit) {
    volatile uint8_t g1 = *MUL_GADGET_LINE;
    volatile uint8_t g0 = *DUMMY_LINE;
    (void)g1; (void)g0;
    uint64_t prod = (uint64_t)(((__uint128_t)(*z) * (*x)) % n);
    uint64_t mask = (uint64_t)-(int64_t)(bit & 1);
    *z = (prod & mask) | ((*z) & ~mask);
    *x = (uint64_t)(((__uint128_t)(*x) * (*x)) % n);
    return *z;
}

/* ---------------- Path ORAM (§13.3.5, L=10 -> 1024 leaves, B=4) ---------------- */
#define ORAM_L 10
#define ORAM_LEAVES (1 << ORAM_L)            /* 1024 leaves */
#define ORAM_NODES  ((1 << (ORAM_L + 1)) - 1)/* 2047 buckets (8188 block slots) */
#define ORAM_N      4096                     /* 4096 logical blocks (~50% tree load) */
#define ORAM_B      4                        /* 4 blocks per bucket */
#define STASH_CAP   256

typedef struct {
    int      addr; /* -1 = dummy block */
    uint64_t data;
} OramBlock;

typedef struct {
    OramBlock slots[ORAM_B];
} OramBucket;

static OramBucket oram_tree[ORAM_NODES];
static int        pos_map[ORAM_N];
static OramBlock  stash[STASH_CAP];
static int        stash_cnt = 0;

/* Heap-indexed binary tree: root=1, children of u are 2u and 2u+1 (stored at u-1) */
static int path_node_idx(int leaf, int level) {
    /* leaf in [0, 2^L - 1]; full path integer in [2^L, 2^(L+1)-1], shift to level */
    int full = (1 << ORAM_L) + leaf;
    return (full >> (ORAM_L - level)) - 1;
}

static uint64_t oram_access(bool is_write, int a, uint64_t new_data,
                            int *out_queried_leaf, size_t *dram_blocks_io) {
    int pos = pos_map[a];
    *out_queried_leaf = pos;
    pos_map[a] = rand() % ORAM_LEAVES;

    /* 1. readPath(pos): pull all non-dummy blocks on P(pos) into on-chip stash */
    for (int l = 0; l <= ORAM_L; l++) {
        int idx = path_node_idx(pos, l);
        for (int b = 0; b < ORAM_B; b++) {
            (*dram_blocks_io)++;
            if (oram_tree[idx].slots[b].addr >= 0) {
                stash[stash_cnt++] = oram_tree[idx].slots[b];
                oram_tree[idx].slots[b].addr = -1;
            }
        }
    }

    /* 2. Locate block 'a' in stash (or insert if first write) */
    int found = -1;
    for (int i = 0; i < stash_cnt; i++) {
        if (stash[i].addr == a) { found = i; break; }
    }
    uint64_t old_data = 0;
    if (found >= 0) {
        old_data = stash[found].data;
        if (is_write) stash[found].data = new_data;
    } else {
        stash[stash_cnt++] = (OramBlock){.addr = a, .data = is_write ? new_data : 0};
    }

    /* 3. Evict greedily from leaf (l=L) up to root (l=0): getCousins + trim(B) */
    for (int l = ORAM_L; l >= 0; l--) {
        int node_on_pos = path_node_idx(pos, l);
        int filled = 0;
        for (int i = 0; i < stash_cnt && filled < ORAM_B; ) {
            int cand_addr = stash[i].addr;
            if (path_node_idx(pos_map[cand_addr], l) == node_on_pos) {
                oram_tree[node_on_pos].slots[filled++] = stash[i];
                stash[i] = stash[--stash_cnt];
            } else {
                i++;
            }
        }
        while (filled < ORAM_B) {
            oram_tree[node_on_pos].slots[filled++] = (OramBlock){.addr = -1, .data = 0};
        }
        (*dram_blocks_io) += ORAM_B;
    }
    return old_data;
}

int main(void) {
    memset(shared_lib, 0xA5, sizeof(shared_lib));

    /* 1. Calibrate Cache Hit vs DRAM Miss Cycles & Mount Flush+Reload on RSA */
    const int CAL = 2000;
    uint64_t hit_sum = 0, miss_sum = 0;
    (void)*MUL_GADGET_LINE;
    for (int i = 0; i < CAL; i++) hit_sum += timed_read(MUL_GADGET_LINE);
    for (int i = 0; i < CAL; i++) {
        _mm_clflush(MUL_GADGET_LINE);
        miss_sum += timed_read(MUL_GADGET_LINE);
    }
    uint64_t avg_hit  = hit_sum / CAL;
    uint64_t avg_miss = miss_sum / CAL;
    uint64_t thresh   = (avg_hit + avg_miss) / 2;

    const uint16_t secret_e = 0xB5A3u; /* 1011_0101_1010_0011 */
    uint16_t recovered_vuln = 0, recovered_safe = 0;
    const int VOTES = 21; /* majority vote across noisy OS interrupts */

    for (int bit_pos = 0; bit_pos < 16; bit_pos++) {
        int real_bit = (secret_e >> bit_pos) & 1;
        int hits_v = 0, hits_s = 0;
        for (int v = 0; v < VOTES; v++) {
            uint64_t z = 1, x = 54, n = 3953;
            _mm_clflush(MUL_GADGET_LINE);
            _mm_mfence();
            rsa_step_vulnerable(&z, &x, n, real_bit);
            if (timed_read(MUL_GADGET_LINE) < thresh) hits_v++;

            _mm_clflush(MUL_GADGET_LINE);
            _mm_mfence();
            rsa_step_consttime(&z, &x, n, real_bit);
            if (timed_read(MUL_GADGET_LINE) < thresh) hits_s++;
        }
        if (hits_v > VOTES / 2) recovered_vuln |= (uint16_t)(1u << bit_pos);
        if (hits_s > VOTES / 2) recovered_safe |= (uint16_t)(1u << bit_pos);
    }
    int match_vuln = 16 - __builtin_popcount(secret_e ^ recovered_vuln);
    int match_safe = 16 - __builtin_popcount(secret_e ^ recovered_safe);

    printf("=== 1. Hardware Flush+Reload Side-Channel Attack on RSA Key ===\n");
    printf("Cache Hit latency  : %3llu TSC-cycles | clflush Miss latency: %3llu TSC-cycles (threshold=%llu)\n",
           (unsigned long long)avg_hit, (unsigned long long)avg_miss, (unsigned long long)thresh);
    printf("True RSA key e     : 0x%04X\n", secret_e);
    printf("Vulnerable modexp  : 0x%04X (%2d / 16 bits leaked = %.1f%% accuracy!)\n",
           recovered_vuln, match_vuln, 100.0 * match_vuln / 16.0);
    printf("Constant-access fix: 0x%04X (%2d / 16 bits matched = %.1f%%, leaks only all-1s mask)\n\n",
           recovered_safe, match_safe, 100.0 * match_safe / 16.0);

    /* 2. Path ORAM Simulation: 10,000 accesses to ONE hot secret block (a=42) */
    srand(2026);
    for (int i = 0; i < ORAM_NODES; i++)
        for (int b = 0; b < ORAM_B; b++)
            oram_tree[i].slots[b].addr = -1;
    for (int a = 0; a < ORAM_N; a++) pos_map[a] = rand() % ORAM_LEAVES;

    size_t dram_io = 0;
    int leaf_hist[ORAM_LEAVES] = {0};
    int max_stash = 0;
    long stash_sum = 0;
    bool data_ok = true;

    /* Pre-populate all N=1024 blocks so tree is loaded */
    for (int a = 0; a < ORAM_N; a++) {
        int dummy_leaf;
        size_t dummy_io = 0;
        oram_access(true, a, 0x100000ULL + (uint64_t)a, &dummy_leaf, &dummy_io);
    }

    const int HOT_REQS = 10000;
    uint64_t expected_val = 0x100000ULL + 42ULL;
    for (int q = 0; q < HOT_REQS; q++) {
        int qleaf = 0;
        if (q % 2 == 0) {
            uint64_t got = oram_access(false, 42, 0, &qleaf, &dram_io);
            if (got != expected_val) data_ok = false;
        } else {
            expected_val += 7;
            oram_access(true, 42, expected_val, &qleaf, &dram_io);
        }
        leaf_hist[qleaf]++;
        if (stash_cnt > max_stash) max_stash = stash_cnt;
        stash_sum += stash_cnt;
    }

    double entropy = 0.0;
    for (int k = 0; k < ORAM_LEAVES; k++) {
        if (leaf_hist[k] > 0) {
            double p = (double)leaf_hist[k] / HOT_REQS;
            entropy -= p * log2(p);
        }
    }

    printf("=== 2. Path ORAM (N=4096, L=10 [1024 leaves], B=4): 10,000 Accesses to a=42 ===\n");
    printf("Data integrity check  : %s\n", data_ok ? "100% EXACT" : "MISMATCH");
    printf("Address-stream entropy: Plain DRAM = 0.000 bits (100%% addr=42) vs Path ORAM = %.3f / 10.000 bits!\n",
           entropy);
    printf("Bandwidth & Stash     : %zu DRAM blocks/op (= 2*(L+1)*B) | Stash avg=%.2f, max=%d blocks\n\n",
           dram_io / HOT_REQS, (double)stash_sum / HOT_REQS, max_stash);

    /* 3. Read Real Host Kernel Transient-Execution Vulnerability Status */
    const char *vulns[] = {"meltdown", "spectre_v1", "spectre_v2", "l1tf", "mds"};
    printf("=== 3. Local Host CPU Side-Channel Mitigations (/sys/devices/system/cpu/vulnerabilities) ===\n");
    for (size_t i = 0; i < sizeof(vulns) / sizeof(vulns[0]); i++) {
        char path[256], line[256] = "unavailable";
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/vulnerabilities/%s", vulns[i]);
        FILE *fp = fopen(path, "r");
        if (fp) {
            if (fgets(line, sizeof(line), fp)) line[strcspn(line, "\n")] = '\0';
            fclose(fp);
        }
        printf("  %-11s : %s\n", vulns[i], line);
    }
    return 0;
}
