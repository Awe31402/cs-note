/*
 * Lecture 52 Experiment: AES-128 (SW vs x86 AES-NI), Encryption Modes
 * (ECB / CBC / OFB / CTR), RC4 Stream Cipher, and RSA vs AES Speed Gap.
 *
 * Build & run (keep binary in /tmp only):
 *   gcc -O3 -maes -msse4.1 -Wall -Wextra crypto_modes_bench.c -o /tmp/crypto_modes_bench
 *   /tmp/crypto_modes_bench && rm -f /tmp/crypto_modes_bench
 */
#include <immintrin.h>
#include <wmmintrin.h>
#include <x86intrin.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const uint8_t SBOX[256] = {
    0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
    0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
    0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
    0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
    0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
    0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
    0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
    0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
    0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
    0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
    0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
    0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
    0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
    0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
    0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
    0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
};

static inline uint8_t xtime(uint8_t x) {
    return (uint8_t)((x << 1) ^ ((x & 0x80) ? 0x1b : 0x00));
}

/* Key schedule matching Eq. 13.1 (with GF(2^8) round constant reduction) */
static void aes128_key_expand(const uint8_t key[16], uint8_t rk[11][16]) {
    memcpy(rk[0], key, 16);
    uint8_t rc = 0x01;
    for (int i = 1; i <= 10; i++) {
        uint8_t t[4] = {
            SBOX[rk[i - 1][13]] ^ rc,
            SBOX[rk[i - 1][14]],
            SBOX[rk[i - 1][15]],
            SBOX[rk[i - 1][12]]
        };
        rc = xtime(rc);
        for (int b = 0; b < 4; b++)
            rk[i][b] = rk[i - 1][b] ^ t[b];
        for (int j = 4; j < 16; j++)
            rk[i][j] = rk[i - 1][j] ^ rk[i][j - 4];
    }
}

/* Pure software AES-128 block encryption (10 rounds) */
static void aes128_sw_enc_block(const uint8_t in[16], uint8_t out[16],
                                const uint8_t rk[11][16]) {
    uint8_t s[16], tmp[16];
    for (int i = 0; i < 16; i++) s[i] = in[i] ^ rk[0][i]; /* Round 0: pre-whitening */

    for (int r = 1; r <= 10; r++) {
        /* 1. SubBytes + 2. ShiftRows */
        tmp[0]  = SBOX[s[0]];  tmp[4]  = SBOX[s[4]];  tmp[8]  = SBOX[s[8]];  tmp[12] = SBOX[s[12]];
        tmp[1]  = SBOX[s[5]];  tmp[5]  = SBOX[s[9]];  tmp[9]  = SBOX[s[13]]; tmp[13] = SBOX[s[1]];
        tmp[2]  = SBOX[s[10]]; tmp[6]  = SBOX[s[14]]; tmp[10] = SBOX[s[2]];  tmp[14] = SBOX[s[6]];
        tmp[3]  = SBOX[s[15]]; tmp[7]  = SBOX[s[3]];  tmp[11] = SBOX[s[7]];  tmp[15] = SBOX[s[11]];

        /* 3. MixColumns (skipped in round 10) */
        if (r < 10) {
            for (int c = 0; c < 16; c += 4) {
                uint8_t a0 = tmp[c], a1 = tmp[c+1], a2 = tmp[c+2], a3 = tmp[c+3];
                uint8_t xr = a0 ^ a1 ^ a2 ^ a3;
                s[c+0] = a0 ^ xr ^ xtime(a0 ^ a1);
                s[c+1] = a1 ^ xr ^ xtime(a1 ^ a2);
                s[c+2] = a2 ^ xr ^ xtime(a2 ^ a3);
                s[c+3] = a3 ^ xr ^ xtime(a3 ^ a0);
            }
        } else {
            memcpy(s, tmp, 16);
        }
        /* 4. AddRoundKey */
        for (int i = 0; i < 16; i++) s[i] ^= rk[r][i];
    }
    memcpy(out, s, 16);
}

/* Hardware x86 AES-NI single-block encryption */
static inline __m128i aes128_ni_enc_block(__m128i b, const __m128i rk[11]) {
    b = _mm_xor_si128(b, rk[0]);
    for (int r = 1; r < 10; r++) b = _mm_aesenc_si128(b, rk[r]);
    return _mm_aesenclast_si128(b, rk[10]);
}

/* Sequential CBC mode using AES-NI (1 block in flight due to feedback chain) */
static void aes128_ni_cbc_enc(const uint8_t *in, uint8_t *out, size_t nbytes,
                              const __m128i rk[11], __m128i iv) {
    __m128i prev = iv;
    for (size_t off = 0; off < nbytes; off += 16) {
        __m128i pt = _mm_loadu_si128((const __m128i *)(in + off));
        prev = aes128_ni_enc_block(_mm_xor_si128(pt, prev), rk);
        _mm_storeu_si128((__m128i *)(out + off), prev);
    }
}

/* Parallel 8-block interleaved CTR mode using AES-NI (fills hardware pipeline) */
static void aes128_ni_ctr_enc_by8(const uint8_t *in, uint8_t *out, size_t nbytes,
                                  const __m128i rk[11], uint64_t major_ctr) {
    uint64_t minor = 0;
    for (size_t off = 0; off < nbytes; off += 128, minor += 8) {
        __m128i c0 = _mm_set_epi64x((long long)major_ctr, (long long)(minor + 0));
        __m128i c1 = _mm_set_epi64x((long long)major_ctr, (long long)(minor + 1));
        __m128i c2 = _mm_set_epi64x((long long)major_ctr, (long long)(minor + 2));
        __m128i c3 = _mm_set_epi64x((long long)major_ctr, (long long)(minor + 3));
        __m128i c4 = _mm_set_epi64x((long long)major_ctr, (long long)(minor + 4));
        __m128i c5 = _mm_set_epi64x((long long)major_ctr, (long long)(minor + 5));
        __m128i c6 = _mm_set_epi64x((long long)major_ctr, (long long)(minor + 6));
        __m128i c7 = _mm_set_epi64x((long long)major_ctr, (long long)(minor + 7));

        c0 = _mm_xor_si128(c0, rk[0]); c1 = _mm_xor_si128(c1, rk[0]);
        c2 = _mm_xor_si128(c2, rk[0]); c3 = _mm_xor_si128(c3, rk[0]);
        c4 = _mm_xor_si128(c4, rk[0]); c5 = _mm_xor_si128(c5, rk[0]);
        c6 = _mm_xor_si128(c6, rk[0]); c7 = _mm_xor_si128(c7, rk[0]);

        for (int r = 1; r < 10; r++) {
            c0 = _mm_aesenc_si128(c0, rk[r]); c1 = _mm_aesenc_si128(c1, rk[r]);
            c2 = _mm_aesenc_si128(c2, rk[r]); c3 = _mm_aesenc_si128(c3, rk[r]);
            c4 = _mm_aesenc_si128(c4, rk[r]); c5 = _mm_aesenc_si128(c5, rk[r]);
            c6 = _mm_aesenc_si128(c6, rk[r]); c7 = _mm_aesenc_si128(c7, rk[r]);
        }
        c0 = _mm_aesenclast_si128(c0, rk[10]); c1 = _mm_aesenclast_si128(c1, rk[10]);
        c2 = _mm_aesenclast_si128(c2, rk[10]); c3 = _mm_aesenclast_si128(c3, rk[10]);
        c4 = _mm_aesenclast_si128(c4, rk[10]); c5 = _mm_aesenclast_si128(c5, rk[10]);
        c6 = _mm_aesenclast_si128(c6, rk[10]); c7 = _mm_aesenclast_si128(c7, rk[10]);

        _mm_storeu_si128((__m128i *)(out + off +   0), _mm_xor_si128(c0, _mm_loadu_si128((const __m128i *)(in + off +   0))));
        _mm_storeu_si128((__m128i *)(out + off +  16), _mm_xor_si128(c1, _mm_loadu_si128((const __m128i *)(in + off +  16))));
        _mm_storeu_si128((__m128i *)(out + off +  32), _mm_xor_si128(c2, _mm_loadu_si128((const __m128i *)(in + off +  32))));
        _mm_storeu_si128((__m128i *)(out + off +  48), _mm_xor_si128(c3, _mm_loadu_si128((const __m128i *)(in + off +  48))));
        _mm_storeu_si128((__m128i *)(out + off +  64), _mm_xor_si128(c4, _mm_loadu_si128((const __m128i *)(in + off +  64))));
        _mm_storeu_si128((__m128i *)(out + off +  80), _mm_xor_si128(c5, _mm_loadu_si128((const __m128i *)(in + off +  80))));
        _mm_storeu_si128((__m128i *)(out + off +  96), _mm_xor_si128(c6, _mm_loadu_si128((const __m128i *)(in + off +  96))));
        _mm_storeu_si128((__m128i *)(out + off + 112), _mm_xor_si128(c7, _mm_loadu_si128((const __m128i *)(in + off + 112))));
    }
}

/* RC4 stream cipher (byte-serial state permutation, textbook §13.1.2) */
static void rc4_crypt(const uint8_t *key, size_t klen, const uint8_t *in,
                      uint8_t *out, size_t nbytes) {
    uint8_t S[256];
    for (int i = 0; i < 256; i++) S[i] = (uint8_t)i;
    uint8_t j = 0;
    for (int i = 0; i < 256; i++) {
        j = (uint8_t)(j + S[i] + key[i % klen]);
        uint8_t t = S[i]; S[i] = S[j]; S[j] = t;
    }
    uint8_t i_idx = 0; j = 0;
    for (size_t k = 0; k < nbytes; k++) {
        i_idx = (uint8_t)(i_idx + 1);
        j = (uint8_t)(j + S[i_idx]);
        uint8_t t = S[i_idx]; S[i_idx] = S[j]; S[j] = t;
        uint8_t otp = S[(uint8_t)(S[i_idx] + S[j])];
        out[k] = in[k] ^ otp;
    }
}

static uint64_t modexp64(uint64_t base, uint64_t exp, uint64_t mod) {
    uint64_t res = 1 % mod;
    base %= mod;
    while (exp > 0) {
        if (exp & 1) res = (uint64_t)(((__uint128_t)res * base) % mod);
        base = (uint64_t)(((__uint128_t)base * base) % mod);
        exp >>= 1;
    }
    return res;
}

static int popcount128(const uint8_t a[16], const uint8_t b[16]) {
    int diff = 0;
    for (int i = 0; i < 16; i++) diff += __builtin_popcount(a[i] ^ b[i]);
    return diff;
}

static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

int main(void) {
    /* 1. Verify SW AES-128 == HW AES-NI against FIPS-197 Appendix B vector */
    const uint8_t fips_key[16] = {
        0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f
    };
    const uint8_t fips_pt[16] = {
        0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff
    };
    uint8_t rk_bytes[11][16], sw_ct[16], ni_ct[16];
    __m128i rk_xmm[11];
    aes128_key_expand(fips_key, rk_bytes);
    for (int r = 0; r <= 10; r++)
        rk_xmm[r] = _mm_loadu_si128((const __m128i *)rk_bytes[r]);

    aes128_sw_enc_block(fips_pt, sw_ct, rk_bytes);
    _mm_storeu_si128((__m128i *)ni_ct,
                     aes128_ni_enc_block(_mm_loadu_si128((const __m128i *)fips_pt), rk_xmm));

    printf("=== 1. FIPS-197 AES-128 Verification & Avalanche Effect ===\n");
    printf("SW Ciphertext : ");
    for (int i = 0; i < 16; i++) printf("%02x", sw_ct[i]);
    printf("\nNI Ciphertext : ");
    for (int i = 0; i < 16; i++) printf("%02x", ni_ct[i]);
    printf(" (match=%s)\n", memcmp(sw_ct, ni_ct, 16) == 0 ? "YES" : "NO");

    /* Confusion & Diffusion over 10,000 trials */
    srand(42);
    long conf_bits = 0, diff_bits = 0;
    const int TRIALS = 10000;
    for (int t = 0; t < TRIALS; t++) {
        uint8_t k1[16], k2[16], p1[16], p2[16], c_base[16], c_k[16], c_p[16];
        uint8_t rk1[11][16], rk2[11][16];
        for (int i = 0; i < 16; i++) {
            k1[i] = (uint8_t)(rand() & 0xff);
            p1[i] = (uint8_t)(rand() & 0xff);
        }
        memcpy(k2, k1, 16); memcpy(p2, p1, 16);
        int bit_k = rand() % 128, bit_p = rand() % 128;
        k2[bit_k / 8] ^= (uint8_t)(1u << (bit_k % 8));
        p2[bit_p / 8] ^= (uint8_t)(1u << (bit_p % 8));

        aes128_key_expand(k1, rk1);
        aes128_key_expand(k2, rk2);
        aes128_sw_enc_block(p1, c_base, rk1);
        aes128_sw_enc_block(p1, c_k, rk2);
        aes128_sw_enc_block(p2, c_p, rk1);
        conf_bits += popcount128(c_base, c_k);
        diff_bits += popcount128(c_base, c_p);
    }
    printf("Confusion (1-bit key flip)      : %.2f / 128 bits changed (%.2f%%)\n",
           (double)conf_bits / TRIALS, 100.0 * conf_bits / (TRIALS * 128.0));
    printf("Diffusion (1-bit plaintext flip): %.2f / 128 bits changed (%.2f%%)\n\n",
           (double)diff_bits / TRIALS, 100.0 * diff_bits / (TRIALS * 128.0));

    /* 2. Mode behavior: identical blocks & 1-bit plaintext fault propagation */
    uint8_t msg[64], msg_err[64], ecb[64], cbc1[64], cbc2[64], ctr1[64], ctr2[64];
    for (int b = 0; b < 4; b++) memset(msg + b * 16, (b % 2 == 0) ? 0xAA : 0x55, 16);
    memcpy(msg_err, msg, 64);
    msg_err[0] ^= 0x01; /* 1-bit flip in Block 0 */

    for (int b = 0; b < 4; b++)
        _mm_storeu_si128((__m128i *)(ecb + b * 16),
                         aes128_ni_enc_block(_mm_loadu_si128((const __m128i *)(msg + b * 16)), rk_xmm));
    aes128_ni_cbc_enc(msg,     cbc1, 64, rk_xmm, _mm_setzero_si128());
    aes128_ni_cbc_enc(msg_err, cbc2, 64, rk_xmm, _mm_setzero_si128());

    /* Encrypt 64 bytes via CTR (pad to 128 bytes scratch) */
    uint8_t pad_in1[128] = {0}, pad_in2[128] = {0}, pad_out1[128], pad_out2[128];
    memcpy(pad_in1, msg, 64); memcpy(pad_in2, msg_err, 64);
    aes128_ni_ctr_enc_by8(pad_in1, pad_out1, 128, rk_xmm, 0xCAFEBABE12345678ULL);
    aes128_ni_ctr_enc_by8(pad_in2, pad_out2, 128, rk_xmm, 0xCAFEBABE12345678ULL);
    memcpy(ctr1, pad_out1, 64); memcpy(ctr2, pad_out2, 64);

    printf("=== 2. Mode Comparison (Block0 == Block2; 1-bit flip in Block0) ===\n");
    printf("Identical PT blocks (B0==B2) -> CT equal?  ECB=%s | CBC=%s | CTR=%s\n",
           memcmp(ecb,  ecb + 32,  16) == 0 ? "YES (leaks!)" : "NO",
           memcmp(cbc1, cbc1 + 32, 16) == 0 ? "YES" : "NO",
           memcmp(ctr1, ctr1 + 32, 16) == 0 ? "YES" : "NO");
    printf("1-bit PT flip in B0 -> Flipped CT bits per block [B0, B1, B2, B3]:\n");
    printf("  CBC mode : [%3d, %3d, %3d, %3d] (cascades to all subsequent blocks)\n",
           popcount128(cbc1+0, cbc2+0), popcount128(cbc1+16, cbc2+16),
           popcount128(cbc1+32, cbc2+32), popcount128(cbc1+48, cbc2+48));
    printf("  CTR mode : [%3d, %3d, %3d, %3d] (error isolated to 1 bit)\n\n",
           popcount128(ctr1+0, ctr2+0), popcount128(ctr1+16, ctr2+16),
           popcount128(ctr1+32, ctr2+32), popcount128(ctr1+48, ctr2+48));

    /* 3. Throughput & Cycles/Byte Benchmark (16 MiB buffer in cache/RAM) */
    const size_t N = 16 * 1024 * 1024;
    uint8_t *buf_in = aligned_alloc(64, N);
    uint8_t *buf_out = aligned_alloc(64, N);
    for (size_t i = 0; i < N; i++) buf_in[i] = (uint8_t)(i * 131u + 7u);

    /* Warmup CPU frequency */
    for (int w = 0; w < 8; w++)
        aes128_ni_ctr_enc_by8(buf_in, buf_out, N, rk_xmm, 0x1234ULL);

    unsigned int aux;
    uint64_t c0, c1;
    double t0, t1;

    /* (a) Software AES-128 */
    t0 = now_sec(); c0 = __rdtscp(&aux);
    for (size_t off = 0; off < N; off += 16)
        aes128_sw_enc_block(buf_in + off, buf_out + off, rk_bytes);
    c1 = __rdtscp(&aux); t1 = now_sec();
    double sw_mb = (N / (1024.0 * 1024.0)) / (t1 - t0);
    double sw_cpb = (double)(c1 - c0) / (double)N;

    /* (b) RC4 Stream Cipher */
    t0 = now_sec(); c0 = __rdtscp(&aux);
    rc4_crypt(fips_key, 16, buf_in, buf_out, N);
    c1 = __rdtscp(&aux); t1 = now_sec();
    double rc4_mb = (N / (1024.0 * 1024.0)) / (t1 - t0);
    double rc4_cpb = (double)(c1 - c0) / (double)N;

    /* (c) AES-NI Sequential CBC Encrypt (4 reps for stable timing) */
    const int REPS_CBC = 16;
    t0 = now_sec(); c0 = __rdtscp(&aux);
    for (int r = 0; r < REPS_CBC; r++) {
        aes128_ni_cbc_enc(buf_in, buf_out, N, rk_xmm, _mm_set1_epi32(r));
        buf_in[0] = buf_out[N - 1];
    }
    c1 = __rdtscp(&aux); t1 = now_sec();
    double cbc_mb = ((double)N * REPS_CBC / (1024.0 * 1024.0)) / (t1 - t0);
    double cbc_cpb = (double)(c1 - c0) / ((double)N * REPS_CBC);

    /* (d) AES-NI Parallel 8-way CTR Mode (32 reps for stable timing) */
    const int REPS_CTR = 32;
    t0 = now_sec(); c0 = __rdtscp(&aux);
    for (int r = 0; r < REPS_CTR; r++) {
        aes128_ni_ctr_enc_by8(buf_in, buf_out, N, rk_xmm, (uint64_t)r);
        buf_in[0] = buf_out[N - 1];
    }
    c1 = __rdtscp(&aux); t1 = now_sec();
    double ctr_mb = ((double)N * REPS_CTR / (1024.0 * 1024.0)) / (t1 - t0);
    double ctr_cpb = (double)(c1 - c0) / ((double)N * REPS_CTR);

    printf("=== 3. Throughput & Pipeline Parallelism on 16 MiB Buffer ===\n");
    printf("SW AES-128 (S-box + GF(2^8))   : %9.1f MiB/s  (%6.3f TSC-cycles/B)\n", sw_mb, sw_cpb);
    printf("RC4 Stream Cipher (byte-serial): %9.1f MiB/s  (%6.3f TSC-cycles/B)\n", rc4_mb, rc4_cpb);
    printf("HW AES-NI CBC Encrypt (serial) : %9.1f MiB/s  (%6.3f TSC-cycles/B)\n", cbc_mb, cbc_cpb);
    printf("HW AES-NI CTR by8 (pipelined)  : %9.1f MiB/s  (%6.3f TSC-cycles/B) -> %.2fx over CBC, %.1fx over SW\n\n",
           ctr_mb, ctr_cpb, ctr_mb / cbc_mb, ctr_mb / sw_mb);

    /* 4. Textbook RSA (n=3953) & Diffie-Hellman + 64-bit RSA ModExp cost */
    uint64_t n = 59ULL * 67ULL, e = 31ULL, d = 247ULL, m = 54ULL;
    uint64_t enc = modexp64(m, e, n);
    uint64_t dec = modexp64(enc, d, n);
    uint64_t sig = modexp64(m, d, n);
    uint64_t ver = modexp64(sig, e, n);

    uint64_t dh_p = 2147483647ULL, dh_q = 5ULL;
    uint64_t a_sec = 1234567ULL, b_sec = 7654321ULL;
    uint64_t A_pub = modexp64(dh_q, a_sec, dh_p);
    uint64_t B_pub = modexp64(dh_q, b_sec, dh_p);
    uint64_t K_alice = modexp64(B_pub, a_sec, dh_p);
    uint64_t K_bob   = modexp64(A_pub, b_sec, dh_p);

    const int RSA_ITERS = 200000;
    uint64_t mod64 = 0xFFFFFFFFFFFFFFC5ULL, priv64 = 0xB7E151628AED2A6BULL, acc = 42;
    t0 = now_sec(); c0 = __rdtscp(&aux);
    for (int i = 0; i < RSA_ITERS; i++) acc = modexp64(acc ^ (uint64_t)i, priv64, mod64);
    c1 = __rdtscp(&aux); t1 = now_sec();
    double rsa64_mb = ((double)RSA_ITERS * 8.0 / (1024.0 * 1024.0)) / (t1 - t0);

    printf("=== 4. Textbook RSA & Diffie-Hellman Verification ===\n");
    printf("RSA (n=%llu, e=%llu, d=%llu): m=%llu -> E(m)=%llu -> D(E(m))=%llu | Sig D(m)=%llu -> E(D(m))=%llu\n",
           (unsigned long long)n, (unsigned long long)e, (unsigned long long)d,
           (unsigned long long)m, (unsigned long long)enc, (unsigned long long)dec,
           (unsigned long long)sig, (unsigned long long)ver);
    printf("Diffie-Hellman (p=%llu, q=%llu): A=%llu, B=%llu -> K_alice=%llu, K_bob=%llu (match=%s)\n",
           (unsigned long long)dh_p, (unsigned long long)dh_q,
           (unsigned long long)A_pub, (unsigned long long)B_pub,
           (unsigned long long)K_alice, (unsigned long long)K_bob,
           K_alice == K_bob ? "YES" : "NO");
    printf("64-bit ModExp throughput (toy 64b vs real 2048b is ~1000x slower still): %.2f MiB/s (sink=%llu)\n",
           rsa64_mb, (unsigned long long)(acc & 0xff));

    free(buf_in);
    free(buf_out);
    return 0;
}
