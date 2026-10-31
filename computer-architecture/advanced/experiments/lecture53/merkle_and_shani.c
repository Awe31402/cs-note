/*
 * Lecture 53 Experiment:
 *   1. Pure SW 64-round SHA-256 vs Hardware Intel SHA-NI (cycles per 64B line)
 *   2. Split-Counter Memory Encryption Engine (MEE) ACIF attack matrix
 *      (Spoofing, Splicing, Data+MAC Replay, Counter+Data+MAC Triple Replay)
 *   3. Classical 8-ary Merkle Tree vs 64-ary Bonsai Merkle Tree (1 GiB DRAM
 *      depth, metadata footprint, and Counter-Cache early-termination hits)
 *
 * Build & run (binary in /tmp only):
 *   gcc -O3 -msha -maes -mssse3 -msse4.1 -Wall -Wextra merkle_and_shani.c -o /tmp/merkle_and_shani
 *   taskset -c 0 /tmp/merkle_and_shani && rm -f /tmp/merkle_and_shani
 */
#include <immintrin.h>
#include <wmmintrin.h>
#include <x86intrin.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const uint32_t K256[64] = {
    0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
    0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
    0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
    0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
    0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
    0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
    0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
    0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u
};

static inline uint32_t rotr32(uint32_t x, int n) {
    return (x >> n) | (x << (32 - n));
}

/* Pure software 64-round SHA-256 transform on one 512-bit (64B) block */
static void sha256_sw_block(uint32_t state[8], const uint8_t block[64]) {
    uint32_t w[64];
    for (int i = 0; i < 16; i++) {
        w[i] = ((uint32_t)block[4*i] << 24) | ((uint32_t)block[4*i+1] << 16) |
               ((uint32_t)block[4*i+2] << 8) | ((uint32_t)block[4*i+3]);
    }
    for (int i = 16; i < 64; i++) {
        uint32_t s0 = rotr32(w[i-15], 7) ^ rotr32(w[i-15], 18) ^ (w[i-15] >> 3);
        uint32_t s1 = rotr32(w[i-2], 17) ^ rotr32(w[i-2], 19)  ^ (w[i-2] >> 10);
        w[i] = w[i-16] + s0 + w[i-7] + s1;
    }
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
    uint32_t e = state[4], f = state[5], g = state[6], h = state[7];
    for (int i = 0; i < 64; i++) {
        uint32_t S1 = rotr32(e, 6) ^ rotr32(e, 11) ^ rotr32(e, 25);
        uint32_t ch = (e & f) ^ ((~e) & g);
        uint32_t temp1 = h + S1 + ch + K256[i] + w[i];
        uint32_t S0 = rotr32(a, 2) ^ rotr32(a, 13) ^ rotr32(a, 22);
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint32_t temp2 = S0 + maj;
        h = g; g = f; f = e; e = d + temp1;
        d = c; c = b; b = a; a = temp1 + temp2;
    }
    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

/* Intel SHA-NI hardware transform on one 512-bit (64B) block */
static void sha256_ni_block(uint32_t state[8], const uint8_t block[64]) {
    const __m128i shuf = _mm_set_epi64x(0x0c0d0e0f08090a0bULL, 0x0405060700010203ULL);
    __m128i s0 = _mm_loadu_si128((const __m128i *)&state[0]); /* DCBA */
    __m128i s1 = _mm_loadu_si128((const __m128i *)&state[4]); /* HGFE */
    s0 = _mm_shuffle_epi32(s0, 0xB1);                         /* CDAB */
    s1 = _mm_shuffle_epi32(s1, 0x1B);                         /* EFGH */
    __m128i tmp = _mm_alignr_epi8(s0, s1, 8);                 /* ABEF */
    s1 = _mm_blend_epi16(s1, s0, 0xF0);                       /* CDGH */
    s0 = tmp;
    __m128i abef_save = s0, cdgh_save = s1;

    __m128i msg0 = _mm_shuffle_epi8(_mm_loadu_si128((const __m128i *)(block +  0)), shuf);
    __m128i msg1 = _mm_shuffle_epi8(_mm_loadu_si128((const __m128i *)(block + 16)), shuf);
    __m128i msg2 = _mm_shuffle_epi8(_mm_loadu_si128((const __m128i *)(block + 32)), shuf);
    __m128i msg3 = _mm_shuffle_epi8(_mm_loadu_si128((const __m128i *)(block + 48)), shuf);

    #define RND4(m_cur, m_prev, m_next, k_idx) do {                           \
        __m128i m = _mm_add_epi32(m_cur, _mm_loadu_si128((const __m128i *)&K256[k_idx])); \
        s1 = _mm_sha256rnds2_epu32(s1, s0, m);                                \
        m  = _mm_shuffle_epi32(m, 0x0E);                                      \
        s0 = _mm_sha256rnds2_epu32(s0, s1, m);                                \
        m_next = _mm_add_epi32(m_next, _mm_alignr_epi8(m_cur, m_prev, 4));    \
        m_next = _mm_sha256msg2_epu32(m_next, m_cur);                         \
        m_prev = _mm_sha256msg1_epu32(m_prev, m_cur);                         \
    } while (0)

    /* Rounds 0..11 bootstrap schedule, 12..51 steady state, 52..63 drain */
    __m128i m;
    m = _mm_add_epi32(msg0, _mm_loadu_si128((const __m128i *)&K256[0]));
    s1 = _mm_sha256rnds2_epu32(s1, s0, m);
    s0 = _mm_sha256rnds2_epu32(s0, s1, _mm_shuffle_epi32(m, 0x0E));

    m = _mm_add_epi32(msg1, _mm_loadu_si128((const __m128i *)&K256[4]));
    s1 = _mm_sha256rnds2_epu32(s1, s0, m);
    s0 = _mm_sha256rnds2_epu32(s0, s1, _mm_shuffle_epi32(m, 0x0E));
    msg0 = _mm_sha256msg1_epu32(msg0, msg1);

    m = _mm_add_epi32(msg2, _mm_loadu_si128((const __m128i *)&K256[8]));
    s1 = _mm_sha256rnds2_epu32(s1, s0, m);
    s0 = _mm_sha256rnds2_epu32(s0, s1, _mm_shuffle_epi32(m, 0x0E));
    msg1 = _mm_sha256msg1_epu32(msg1, msg2);

    RND4(msg3, msg2, msg0, 12);
    RND4(msg0, msg3, msg1, 16);
    RND4(msg1, msg0, msg2, 20);
    RND4(msg2, msg1, msg3, 24);
    RND4(msg3, msg2, msg0, 28);
    RND4(msg0, msg3, msg1, 32);
    RND4(msg1, msg0, msg2, 36);
    RND4(msg2, msg1, msg3, 40);
    RND4(msg3, msg2, msg0, 44);
    RND4(msg0, msg3, msg1, 48);

    m = _mm_add_epi32(msg1, _mm_loadu_si128((const __m128i *)&K256[52]));
    s1 = _mm_sha256rnds2_epu32(s1, s0, m);
    s0 = _mm_sha256rnds2_epu32(s0, s1, _mm_shuffle_epi32(m, 0x0E));
    msg2 = _mm_add_epi32(msg2, _mm_alignr_epi8(msg1, msg0, 4));
    msg2 = _mm_sha256msg2_epu32(msg2, msg1);

    m = _mm_add_epi32(msg2, _mm_loadu_si128((const __m128i *)&K256[56]));
    s1 = _mm_sha256rnds2_epu32(s1, s0, m);
    s0 = _mm_sha256rnds2_epu32(s0, s1, _mm_shuffle_epi32(m, 0x0E));
    msg3 = _mm_add_epi32(msg3, _mm_alignr_epi8(msg2, msg1, 4));
    msg3 = _mm_sha256msg2_epu32(msg3, msg2);

    m = _mm_add_epi32(msg3, _mm_loadu_si128((const __m128i *)&K256[60]));
    s1 = _mm_sha256rnds2_epu32(s1, s0, m);
    s0 = _mm_sha256rnds2_epu32(s0, s1, _mm_shuffle_epi32(m, 0x0E));
    #undef RND4

    s0 = _mm_add_epi32(s0, abef_save);
    s1 = _mm_add_epi32(s1, cdgh_save);
    tmp = _mm_shuffle_epi32(s0, 0x1B);                        /* FEBA */
    s1  = _mm_shuffle_epi32(s1, 0xB1);                        /* DCHG */
    _mm_storeu_si128((__m128i *)&state[0], _mm_blend_epi16(tmp, s1, 0xF0));
    _mm_storeu_si128((__m128i *)&state[4], _mm_alignr_epi8(s1, tmp, 8));
}

/* 64-bit truncated SHA-256 hash of a 64B cache line */
static uint64_t hash64_ni(const uint8_t line[64]) {
    uint32_t st[8] = {
        0x6a09e667u,0xbb67ae85u,0x3c6ef372u,0xa54ff53au,
        0x510e527fu,0x9b05688cu,0x1f83d9abu,0x5be0cd19u
    };
    sha256_ni_block(st, line);
    return ((uint64_t)st[0] << 32) | st[1];
}

/* MEE Split-Counter Node: 64b major + 64 * 6b minor (=448b) + 64b MAC = 512b */
typedef struct {
    uint64_t major;
    uint8_t  minor[64]; /* each masked to 6 bits (0..63) */
    uint64_t mac;
} CounterNode;

static __m128i g_mee_rk[11];

static inline __m128i aes_ni_10(__m128i b) {
    b = _mm_xor_si128(b, g_mee_rk[0]);
    for (int r = 1; r < 10; r++) b = _mm_aesenc_si128(b, g_mee_rk[r]);
    return _mm_aesenclast_si128(b, g_mee_rk[10]);
}

/* Pack <major(64b), minor(6b) | block_addr(58b)> -> 128-bit AES CTR input */
static inline __m128i make_ctr_seed(uint64_t major, uint8_t minor6, uint64_t block_addr58) {
    uint64_t lo = (((uint64_t)(minor6 & 0x3fu)) << 58) | (block_addr58 & ((1ULL << 58) - 1ULL));
    return _mm_set_epi64x((long long)major, (long long)lo);
}

/* Encrypt/decrypt a 64B line + compute 64b Carter-Wegman MAC (Figure 13.9) */
static void mee_encrypt_line(const uint8_t pt[64], uint8_t ct[64], uint64_t *mac_out,
                             uint64_t major, uint8_t minor6, uint64_t block_addr58) {
    __m128i base = make_ctr_seed(major, minor6, block_addr58);
    for (int q = 0; q < 4; q++) {
        /* Enclave chunk-specific IV XORed into seed (textbook p.689-690) */
        __m128i chunk_iv = _mm_set1_epi32(0x13579BDF * (q + 1));
        __m128i otp = aes_ni_10(_mm_xor_si128(base, chunk_iv));
        __m128i p = _mm_loadu_si128((const __m128i *)(pt + 16 * q));
        _mm_storeu_si128((__m128i *)(ct + 16 * q), _mm_xor_si128(p, otp));
    }
    uint64_t h = hash64_ni(ct);
    __m128i mac_otp = aes_ni_10(_mm_xor_si128(base, _mm_set1_epi32(0xCAFEBABE)));
    *mac_out = h ^ (uint64_t)_mm_cvtsi128_si64(mac_otp);
}

static bool mee_decrypt_verify_line(const uint8_t ct[64], uint64_t mac_in, uint8_t pt[64],
                                    uint64_t major, uint8_t minor6, uint64_t block_addr58) {
    __m128i base = make_ctr_seed(major, minor6, block_addr58);
    for (int q = 0; q < 4; q++) {
        __m128i chunk_iv = _mm_set1_epi32(0x13579BDF * (q + 1));
        __m128i otp = aes_ni_10(_mm_xor_si128(base, chunk_iv));
        __m128i c = _mm_loadu_si128((const __m128i *)(ct + 16 * q));
        _mm_storeu_si128((__m128i *)(pt + 16 * q), _mm_xor_si128(c, otp));
    }
    uint64_t h = hash64_ni(ct);
    __m128i mac_otp = aes_ni_10(_mm_xor_si128(base, _mm_set1_epi32(0xCAFEBABE)));
    return (h ^ (uint64_t)_mm_cvtsi128_si64(mac_otp)) == mac_in;
}

/* Serialize 448-bit counter payload into 64B buffer (last 8B zeroed) and compute Bonsai MAC */
static uint64_t bonsai_node_mac(const CounterNode *child, const CounterNode *parent,
                                int child_idx, uint64_t child_node_addr) {
    uint8_t buf[64] = {0};
    memcpy(buf, &child->major, 8);
    for (int i = 0; i < 64; i++) {
        int bit_pos = 64 + i * 6;
        uint8_t v = child->minor[i] & 0x3fu;
        buf[bit_pos / 8] |= (uint8_t)(v << (bit_pos % 8));
        if ((bit_pos % 8) > 2)
            buf[bit_pos / 8 + 1] |= (uint8_t)(v >> (8 - (bit_pos % 8)));
    }
    uint64_t h = hash64_ni(buf);
    __m128i seed = make_ctr_seed(parent->major, parent->minor[child_idx], child_node_addr);
    __m128i otp  = aes_ni_10(_mm_xor_si128(seed, _mm_set1_epi32(0x55AA55AA)));
    return h ^ (uint64_t)_mm_cvtsi128_si64(otp);
}

int main(void) {
    /* Initialize MEE hardware key = f(PUF, boot_rand, enclave_id) */
    for (int r = 0; r <= 10; r++)
        g_mee_rk[r] = _mm_set_epi32(0x9e3779b9 * (r + 1), 0x7f4a7c15, 0x6c62272e, 0x07bb0142 + r);

    /* 1. Verify SW SHA-256 == HW SHA-NI on "abc" + benchmark 64B cache-line hashing */
    uint8_t abc_pad[64] = {0};
    abc_pad[0] = 'a'; abc_pad[1] = 'b'; abc_pad[2] = 'c'; abc_pad[3] = 0x80;
    abc_pad[63] = 24; /* 3 bytes = 24 bits */
    uint32_t st_sw[8] = {
        0x6a09e667u,0xbb67ae85u,0x3c6ef372u,0xa54ff53au,
        0x510e527fu,0x9b05688cu,0x1f83d9abu,0x5be0cd19u
    };
    uint32_t st_ni[8];
    memcpy(st_ni, st_sw, sizeof(st_sw));
    sha256_sw_block(st_sw, abc_pad);
    sha256_ni_block(st_ni, abc_pad);

    printf("=== 1. SHA-256 Verification (SW vs Intel SHA-NI) & 64B Line Latency ===\n");
    printf("NIST \"abc\" digest: %08x%08x...%08x (SW==SHA-NI: %s)\n",
           st_ni[0], st_ni[1], st_ni[7],
           memcmp(st_sw, st_ni, 32) == 0 ? "YES" : "NO");

    unsigned int aux;
    const int ITERS = 500000;
    for (int i = 0; i < 10000; i++) sha256_ni_block(st_ni, abc_pad); /* warmup */

    uint64_t c0 = __rdtscp(&aux);
    for (int i = 0; i < ITERS; i++) sha256_sw_block(st_sw, abc_pad);
    uint64_t c1 = __rdtscp(&aux);

    uint64_t c2 = __rdtscp(&aux);
    for (int i = 0; i < ITERS; i++) sha256_ni_block(st_ni, abc_pad);
    uint64_t c3 = __rdtscp(&aux);

    __m128i otp_acc = _mm_setzero_si128();
    uint64_t c4 = __rdtscp(&aux);
    for (int i = 0; i < ITERS; i++) {
        __m128i s = make_ctr_seed(1, (uint8_t)(i & 63), 0x1000);
        __m128i o0 = aes_ni_10(s);
        __m128i o1 = aes_ni_10(_mm_add_epi64(s, _mm_set1_epi64x(1)));
        __m128i o2 = aes_ni_10(_mm_add_epi64(s, _mm_set1_epi64x(2)));
        __m128i o3 = aes_ni_10(_mm_add_epi64(s, _mm_set1_epi64x(3)));
        otp_acc = _mm_xor_si128(otp_acc, _mm_xor_si128(_mm_xor_si128(o0, o1), _mm_xor_si128(o2, o3)));
    }
    uint64_t c5 = __rdtscp(&aux);

    double sw_cyc  = (double)(c1 - c0) / ITERS;
    double ni_cyc  = (double)(c3 - c2) / ITERS;
    double otp_cyc = (double)(c5 - c4) / ITERS;
    printf("SW SHA-256 (64 rounds) : %6.1f TSC-cycles / 64B line\n", sw_cyc);
    printf("HW SHA-NI  (64 rounds) : %6.1f TSC-cycles / 64B line (%.2fx faster)\n",
           ni_cyc, sw_cyc / ni_cyc);
    printf("4x16B CTR OTP precomp  : %6.1f TSC-cycles / 64B line (fits easily inside ~250-cycle DRAM miss shadow! sink=%x)\n\n",
           otp_cyc, _mm_cvtsi128_si32(otp_acc) & 0xf);

    /* 2. MEE ACIF Attack Matrix + Bonsai Merkle Tree Defense */
    CounterNode root = {.major = 100, .minor = {0}, .mac = 0}; /* On-chip TCB register */
    CounterNode leaf = {.major = 1,   .minor = {0}, .mac = 0};
    uint8_t pt_v1[64], pt_v2[64], pt_other[64], pt_out[64];
    uint8_t ct_v1[64], ct_v2[64], ct_other[64];
    uint64_t mac_v1, mac_v2, mac_other;
    memset(pt_v1, 0x11, 64); memset(pt_v2, 0x22, 64); memset(pt_other, 0x99, 64);

    const uint64_t ADDR_A = 0x10040ULL, ADDR_C = 0x20080ULL;
    /* Evict v1 to ADDR_A (minor becomes 1, root minor[0] becomes 1) */
    leaf.minor[0] = 1; root.minor[0] = 1;
    leaf.mac = bonsai_node_mac(&leaf, &root, 0, 0x8000ULL);
    mee_encrypt_line(pt_v1, ct_v1, &mac_v1, leaf.major, leaf.minor[0], ADDR_A);
    CounterNode saved_old_leaf = leaf;

    /* Evict other block to ADDR_C with same counters */
    mee_encrypt_line(pt_other, ct_other, &mac_other, leaf.major, leaf.minor[0], ADDR_C);

    /* Evict v2 to ADDR_A (minor increments to 2, root minor[0] increments to 2) */
    leaf.minor[0] = 2; root.minor[0] = 2;
    leaf.mac = bonsai_node_mac(&leaf, &root, 0, 0x8000ULL);
    mee_encrypt_line(pt_v2, ct_v2, &mac_v2, leaf.major, leaf.minor[0], ADDR_A);

    uint8_t ct_spoof[64]; memcpy(ct_spoof, ct_v2, 64); ct_spoof[7] ^= 0x01;

    bool ok_legit   = mee_decrypt_verify_line(ct_v2, mac_v2, pt_out, leaf.major, leaf.minor[0], ADDR_A)
                      && (bonsai_node_mac(&leaf, &root, 0, 0x8000ULL) == leaf.mac);
    bool ok_spoof   = mee_decrypt_verify_line(ct_spoof, mac_v2, pt_out, leaf.major, leaf.minor[0], ADDR_A);
    bool ok_splice  = mee_decrypt_verify_line(ct_other, mac_other, pt_out, leaf.major, leaf.minor[0], ADDR_A);
    bool ok_replay1 = mee_decrypt_verify_line(ct_v1, mac_v1, pt_out, leaf.major, leaf.minor[0], ADDR_A);
    bool ok_replay3_no_tree = mee_decrypt_verify_line(ct_v1, mac_v1, pt_out,
                                                      saved_old_leaf.major, saved_old_leaf.minor[0], ADDR_A);
    bool ok_replay3_bonsai  = ok_replay3_no_tree &&
                              (bonsai_node_mac(&saved_old_leaf, &root, 0, 0x8000ULL) == saved_old_leaf.mac);

    printf("=== 2. MEE ACIF Attack Verification Matrix ===\n");
    printf("Legitimate latest read (v2, minor=2)                : %s\n", ok_legit ? "PASS" : "DETECTED");
    printf("Spoofing attack (1-bit flip in DRAM ciphertext)     : %s\n", ok_spoof ? "PASS" : "DETECTED (MAC mismatch)");
    printf("Splicing attack (copy valid (CT,MAC) ADDR_C->ADDR_A): %s\n", ok_splice ? "PASS" : "DETECTED (block_addr in OTP)");
    printf("Data+MAC Replay (old v1 with protected minor=2)     : %s\n", ok_replay1 ? "PASS" : "DETECTED (freshness fail)");
    printf("Triple Replay (CT+MAC+LeafCtr) WITHOUT Merkle Tree  : %s\n", ok_replay3_no_tree ? "BYPASSED (vulnerable!)" : "DETECTED");
    printf("Triple Replay (CT+MAC+LeafCtr) WITH Bonsai Tree     : %s\n\n", ok_replay3_bonsai ? "BYPASSED" : "DETECTED (parent ctr MAC fail)");

    /* 3. 1 GiB Secure Memory: Classical 8-ary vs 64-ary Bonsai Merkle Tree */
    const size_t PAGES_1GB = (1ULL << 30) / 4096ULL; /* 2^18 = 262,144 pages */
    size_t n_cls = PAGES_1GB, lv_cls = 1, internal_cls = 0;
    while (n_cls > 1) { n_cls = (n_cls + 7) / 8; internal_cls += n_cls; lv_cls++; }

    size_t n_bon = PAGES_1GB, lv_bon = 1, internal_bon = 0;
    while (n_bon > 1) { n_bon = (n_bon + 63) / 64; internal_bon += n_bon; lv_bon++; }

    /* Simulate 512-line (32 KiB) LRU tree/counter cache on 200,000 misses (80/20 locality) */
    uint64_t lru_cls[512] = {0}, lru_bon[512] = {0};
    uint64_t clk = 1, dram_walks_cls = 0, dram_walks_bon = 0;
    srand(1337);
    const int SIM_REQS = 200000;
    for (int q = 0; q < SIM_REQS; q++, clk++) {
        size_t pg = (rand() % 100 < 80) ? (size_t)(rand() % 2048) : (size_t)(rand() % PAGES_1GB);
        /* Classical 8-ary walk from leaf (level 0) up to root (level 6, on-chip) */
        size_t cur = pg;
        for (size_t lv = 0; lv < lv_cls - 1; lv++, cur /= 8) {
            uint64_t tag = ((uint64_t)(lv + 1) << 32) | cur;
            int hit = -1, victim = 0;
            for (int s = 0; s < 512; s++) {
                if ((lru_cls[s] >> 24) == tag) { hit = s; break; }
                if ((lru_cls[s] & 0xffffffULL) < (lru_cls[victim] & 0xffffffULL)) victim = s;
            }
            if (hit >= 0) { lru_cls[hit] = (tag << 24) | (clk & 0xffffffULL); break; } /* Stop at cached node! */
            dram_walks_cls++;
            lru_cls[victim] = (tag << 24) | (clk & 0xffffffULL);
        }
        /* Bonsai 64-ary walk from leaf (level 0) up to root (level 3, on-chip) */
        cur = pg;
        for (size_t lv = 0; lv < lv_bon - 1; lv++, cur /= 64) {
            uint64_t tag = ((uint64_t)(lv + 1) << 32) | cur;
            int hit = -1, victim = 0;
            for (int s = 0; s < 512; s++) {
                if ((lru_bon[s] >> 24) == tag) { hit = s; break; }
                if ((lru_bon[s] & 0xffffffULL) < (lru_bon[victim] & 0xffffffULL)) victim = s;
            }
            if (hit >= 0) { lru_bon[hit] = (tag << 24) | (clk & 0xffffffULL); break; }
            dram_walks_bon++;
            lru_bon[victim] = (tag << 24) | (clk & 0xffffffULL);
        }
    }

    printf("=== 3. 1 GiB Secure Memory (262,144 Pages): 8-ary vs 64-ary Bonsai Tree ===\n");
    printf("Classical 8-ary Merkle : %zu levels | %6zu internal nodes (%7.1f KiB) | avg %.3f DRAM tree reads/miss\n",
           lv_cls, internal_cls, internal_cls * 64.0 / 1024.0, (double)dram_walks_cls / SIM_REQS);
    printf("64-ary Bonsai Merkle   : %zu levels | %6zu internal nodes (%7.1f KiB) | avg %.3f DRAM tree reads/miss\n",
           lv_bon, internal_bon, internal_bon * 64.0 / 1024.0, (double)dram_walks_bon / SIM_REQS);
    printf("Internal tree compression: %.2fx fewer internal nodes, %.2fx fewer DRAM tree reads!\n",
           (double)internal_cls / internal_bon, (double)dram_walks_cls / dram_walks_bon);
    return 0;
}
