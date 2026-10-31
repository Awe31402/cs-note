/*
 * Lecture 56 Experiment: 7-Loop Reordering, 9-Loop Tiling, Halo Pixels & Stationarity
 * 1. Real single-P-core CPU benchmark on i7-13650HX (N=16, C=32, K=64, 112x112, R=S=3,
 *    runtime dimensions so compiler cannot cheat by unrolling R=S=3 into constants):
 *      (a) Eq. 14.8 WS order (k -> c -> n -> h -> w -> r -> s): multi-image psum thrashing
 *      (b) Listing 14.1      (n -> k -> c -> h -> w -> r -> s): scalar inner r->s dot product
 *      (c) Loop-interchanged (n -> k -> c -> r -> s -> h -> w): unit-stride AVX2 vectorization
 *      (d) Eq. 14.11 OS-Tiled (n -> h' -> w' -> c -> r -> s -> [k'' -> h'' -> w'']_LB):
 *          keeps Tk=4 x Th=2 x Tw=16 (16 YMM registers = 512B LB) stationary in RF!
 * 2. Analytical Global-Buffer (GB) traffic & Halo pixel overhead across NLR / WS / IS / OS.
 */
#define _POSIX_C_SOURCE 200809L
#include <immintrin.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

/* (a) Textbook Listing 14.1: n -> k -> c -> h -> w -> r -> s
 *     Processes one image at a time, inner r -> s loop over R*S filter */
__attribute__((noinline))
static void conv_listing_14_1(const float *restrict I, const float *restrict F,
                              float *restrict O, int N, int C, int K,
                              int H, int W, int R, int S) {
    int WP = W + S - 1, HP = H + R - 1;
    memset(O, 0, (size_t)N * K * H * W * sizeof(float));
    for (int n = 0; n < N; n++) {
        for (int k = 0; k < K; k++) {
            float *out = O + (size_t)(n * K + k) * H * W;
            for (int c = 0; c < C; c++) {
                const float *in = I + (size_t)(n * C + c) * HP * WP;
                const float *f  = F + (size_t)(k * C + c) * R * S;
                for (int h = 0; h < H; h++) {
                    for (int w = 0; w < W; w++) {
                        float sum = 0.0f;
                        for (int r = 0; r < R; r++)
                            for (int s = 0; s < S; s++)
                                sum += in[(h + r) * WP + (w + s)] * f[r * S + s];
                        out[h * W + w] += sum;
                    }
                }
            }
        }
    }
}

/* (c1) Vectorized inner w, but Eq. 14.8 outer k->c->n (49 MiB O[] spills 24 MiB LLC!) */
__attribute__((noinline))
static void conv_kcn_interchanged(const float *restrict I, const float *restrict F,
                                  float *restrict O, int N, int C, int K,
                                  int H, int W, int R, int S) {
    int WP = W + S - 1, HP = H + R - 1;
    memset(O, 0, (size_t)N * K * H * W * sizeof(float));
    for (int k = 0; k < K; k++) {
        for (int c = 0; c < C; c++) {
            const float *f = F + (size_t)(k * C + c) * R * S;
            for (int n = 0; n < N; n++) {
                const float *in = I + (size_t)(n * C + c) * HP * WP;
                float *out = O + (size_t)(n * K + k) * H * W;
                for (int r = 0; r < R; r++) {
                    for (int s = 0; s < S; s++) {
                        float wt = f[r * S + s];
                        for (int h = 0; h < H; h++) {
                            for (int w = 0; w < W; w++) {
                                out[h * W + w] += in[(h + r) * WP + (w + s)] * wt;
                            }
                        }
                    }
                }
            }
        }
    }
}

/* (c2) Loop-interchanged: n -> k -> c -> r -> s -> h -> w
 *     Moves n outermost (fits in LLC!) + w innermost (unit-stride AVX2)! */
__attribute__((noinline))
static void conv_interchanged(const float *restrict I, const float *restrict F,
                              float *restrict O, int N, int C, int K,
                              int H, int W, int R, int S) {
    int WP = W + S - 1, HP = H + R - 1;
    memset(O, 0, (size_t)N * K * H * W * sizeof(float));
    for (int n = 0; n < N; n++) {
        for (int k = 0; k < K; k++) {
            float *out = O + (size_t)(n * K + k) * H * W;
            for (int c = 0; c < C; c++) {
                const float *in = I + (size_t)(n * C + c) * HP * WP;
                const float *f  = F + (size_t)(k * C + c) * R * S;
                for (int r = 0; r < R; r++) {
                    for (int s = 0; s < S; s++) {
                        float wt = f[r * S + s];
                        for (int h = 0; h < H; h++) {
                            for (int w = 0; w < W; w++) {
                                out[h * W + w] += in[(h + r) * WP + (w + s)] * wt;
                            }
                        }
                    }
                }
            }
        }
    }
}

/* (d) Textbook Eq. 14.11 Output-Stationary Tiled mapping:
 *     n -> k' -> h' -> w' -> c -> r -> s -> [k'' -> h'' -> w'']_LB
 *     Tile Tk=4 x Th=2 x Tw=16 = 16 AVX2 YMM registers (zero psum memory traffic in c->r->s!) */
__attribute__((noinline))
static void conv_os_tiled_avx2(const float *restrict I, const float *restrict F,
                               float *restrict O, int N, int C, int K,
                               int H, int W, int R, int S) {
    int WP = W + S - 1, HP = H + R - 1;
    for (int n = 0; n < N; n++) {
        for (int kp = 0; kp < K; kp += 4) {
            for (int hp = 0; hp < H; hp += 2) {
                for (int wp = 0; wp < W; wp += 16) {
                    __m256 acc[4][2][2];
                    for (int kk = 0; kk < 4; kk++)
                        for (int hh = 0; hh < 2; hh++) {
                            acc[kk][hh][0] = _mm256_setzero_ps();
                            acc[kk][hh][1] = _mm256_setzero_ps();
                        }

                    for (int c = 0; c < C; c++) {
                        const float *in = I + (size_t)(n * C + c) * HP * WP;
                        for (int r = 0; r < R; r++) {
                            for (int s = 0; s < S; s++) {
                                __m256 i00 = _mm256_loadu_ps(in + (hp + 0 + r) * WP + wp + s);
                                __m256 i01 = _mm256_loadu_ps(in + (hp + 0 + r) * WP + wp + s + 8);
                                __m256 i10 = _mm256_loadu_ps(in + (hp + 1 + r) * WP + wp + s);
                                __m256 i11 = _mm256_loadu_ps(in + (hp + 1 + r) * WP + wp + s + 8);
                                for (int kk = 0; kk < 4; kk++) {
                                    __m256 vw = _mm256_set1_ps(
                                        F[((kp + kk) * C + c) * R * S + r * S + s]);
                                    acc[kk][0][0] = _mm256_fmadd_ps(i00, vw, acc[kk][0][0]);
                                    acc[kk][0][1] = _mm256_fmadd_ps(i01, vw, acc[kk][0][1]);
                                    acc[kk][1][0] = _mm256_fmadd_ps(i10, vw, acc[kk][1][0]);
                                    acc[kk][1][1] = _mm256_fmadd_ps(i11, vw, acc[kk][1][1]);
                                }
                            }
                        }
                    }

                    for (int kk = 0; kk < 4; kk++) {
                        float *out = O + (size_t)(n * K + kp + kk) * H * W;
                        _mm256_storeu_ps(out + (hp + 0) * W + wp,     acc[kk][0][0]);
                        _mm256_storeu_ps(out + (hp + 0) * W + wp + 8, acc[kk][0][1]);
                        _mm256_storeu_ps(out + (hp + 1) * W + wp,     acc[kk][1][0]);
                        _mm256_storeu_ps(out + (hp + 1) * W + wp + 8, acc[kk][1][1]);
                    }
                }
            }
        }
    }
}

static float max_abs_diff(const float *A, const float *B, size_t len) {
    float md = 0.0f;
    for (size_t i = 0; i < len; i++) {
        float d = fabsf(A[i] - B[i]);
        if (d > md) md = d;
    }
    return md;
}

int main(void) {
    volatile int vN = 16, vC = 32, vK = 64, vH = 112, vW = 112, vR = 3, vS = 3;
    int N = vN, C = vC, K = vK, H = vH, W = vW, R = vR, S = vS;
    int HP = H + R - 1, WP = W + S - 1;

    size_t sz_in  = (size_t)N * C * HP * WP;
    size_t sz_flt = (size_t)K * C * R * S;
    size_t sz_out = (size_t)N * K * H * W;

    float *I  = aligned_alloc(64, sz_in * sizeof(float));
    float *F  = aligned_alloc(64, sz_flt * sizeof(float));
    float *O0 = aligned_alloc(64, sz_out * sizeof(float));
    float *O1 = aligned_alloc(64, sz_out * sizeof(float));
    float *O2 = aligned_alloc(64, sz_out * sizeof(float));
    float *O3 = aligned_alloc(64, sz_out * sizeof(float));

    srand(123);
    for (size_t i = 0; i < sz_in; i++)  I[i] = ((float)rand() / RAND_MAX) - 0.5f;
    for (size_t i = 0; i < sz_flt; i++) F[i] = ((float)rand() / RAND_MAX) - 0.5f;

    double macs = (double)N * K * C * H * W * R * S;
    double flops = 2.0 * macs;

    conv_os_tiled_avx2(I, F, O3, N, C, K, H, W, R, S);

    double t0 = now_sec(); conv_listing_14_1(I, F, O0, N, C, K, H, W, R, S);     double dt0 = now_sec() - t0;
    double t1 = now_sec(); conv_kcn_interchanged(I, F, O1, N, C, K, H, W, R, S); double dt1 = now_sec() - t1;
    double t2 = now_sec(); conv_interchanged(I, F, O2, N, C, K, H, W, R, S);     double dt2 = now_sec() - t2;
    double t3 = now_sec(); conv_os_tiled_avx2(I, F, O3, N, C, K, H, W, R, S);    double dt3 = now_sec() - t3;

    printf("=== 1. CPU 7-Loop Reordering & OS-Tiling Benchmark (N=%d,C=%d,K=%d,%dx%d,%dx%d) ===\n",
           N, C, K, H, W, R, S);
    printf("  Total MACs = %.3f G  (%.3f GFLOPs) | Working Set: I=%.2f MiB, O=%.2f MiB, F=%.2f MiB\n",
           macs / 1e9, flops / 1e9,
           sz_in * 4.0 / (1024 * 1024), sz_out * 4.0 / (1024 * 1024), sz_flt * 4.0 / (1024 * 1024));
    printf("  Mapping                                      | Time (ms) | GFLOP/s | Speedup | MaxDiff\n");
    printf("  ---------------------------------------------+-----------+---------+---------+---------\n");
    printf("  (a) Listing 14.1 n->k->c->h->w->r->s         | %9.2f |  %6.2f |  1.00x  | %.2e\n",
           dt0 * 1e3, flops / dt0 / 1e9, 0.0f);
    printf("  (b) Eq.14.8 outer k->c->n->r->s->h->w (spill)| %9.2f |  %6.2f |  %4.2fx  | %.2e\n",
           dt1 * 1e3, flops / dt1 / 1e9, dt0 / dt1, max_abs_diff(O0, O1, sz_out));
    printf("  (c) Image-first   n->k->c->r->s->h->w        | %9.2f |  %6.2f |  %4.2fx  | %.2e\n",
           dt2 * 1e3, flops / dt2 / 1e9, dt0 / dt2, max_abs_diff(O0, O2, sz_out));
    printf("  (d) Eq.14.11 OS-Tiled (Tk=4,Th=2,Tw=16 in RF)| %9.2f |  %6.2f |  %4.2fx  | %.2e\n",
           dt3 * 1e3, flops / dt3 / 1e9, dt0 / dt3, max_abs_diff(O0, O3, sz_out));

    printf("\n=== 2. Halo Pixel Expansion & Analytical Global-Buffer (GB) Accesses (N=%d) ===\n", N);
    for (int t = 4; t <= 16; t *= 2) {
        int in_h = t + R - 1;
        double ratio = (double)(in_h * in_h) / (t * t);
        printf("  Tile %2dx%-2d (3x3 filter) -> Input tile w/ Halo = %2dx%-2d (%5.1f%% extra pixels)\n",
               t, t, in_h, in_h, (ratio - 1.0) * 100.0);
    }

    const int TH = 7, TW = 7;
    const long tiles = (H / TH) * (W / TW); /* 16 * 16 = 256 tiles */
    const long in_tile_halo = (TH + R - 1) * (TW + S - 1); /* 81 */
    long nlr_gb  = (long)macs * 4;
    long ws8_gb  = (long)K * C * R * S
                 + (long)K * C * N * tiles * in_tile_halo
                 + (long)K * C * N * H * W * 2;
    long is10_gb = (long)N * tiles * C * in_tile_halo
                 + (long)N * tiles * K * C * R * S
                 + (long)N * K * H * W;
    long os11_gb = (long)N * tiles * C * in_tile_halo
                 + (long)N * tiles * C * K * R * S
                 + (long)N * K * H * W;

    printf("\n  Stationarity Model (Th=Tw=7, 256 tiles)| LB Size/PE | Total GB Words (M) | vs NLR\n");
    printf("  ---------------------------------------+------------+--------------------+--------\n");
    printf("  No Local Reuse (NLR)                   |      0 B   |        %8.2f M  |  1.00x\n",
           nlr_gb / 1e6);
    printf("  WS Eq.14.8 (k->c->n, R*S in LB)        |     36 B   |        %8.2f M  | %5.2fx\n",
           ws8_gb / 1e6, (double)nlr_gb / ws8_gb);
    printf("  IS Eq.14.10 (C*9*9 halo tile in LB)    |  10368 B   |        %8.2f M  | %5.2fx\n",
           is10_gb / 1e6, (double)nlr_gb / is10_gb);
    printf("  OS Eq.14.11 (K*7*7 ofmap tile in LB)   |  12544 B   |        %8.2f M  | %5.2fx\n",
           os11_gb / 1e6, (double)nlr_gb / os11_gb);

    free(I); free(F); free(O0); free(O1); free(O2); free(O3);
    return 0;
}
