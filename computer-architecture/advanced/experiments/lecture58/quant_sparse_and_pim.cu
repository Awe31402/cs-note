/*
 * Lecture 58 Experiment: Quantization (INT8 Tensor Core / dp4a), Sparsity & Base-Delta,
 *                        Bit-Serial 1D Bit-Convolution, and ReRAM Dual-Column PIM Crossbar
 */
#include <cublas_v2.h>
#include <cuda_runtime.h>
#include <mma.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace nvcuda;

#define CHECK_CUDA(call)                                                       \
    do {                                                                       \
        cudaError_t err = (call);                                              \
        if (err != cudaSuccess) {                                              \
            fprintf(stderr, "CUDA error at %s:%d: %s\n", __FILE__, __LINE__,   \
                    cudaGetErrorString(err));                                   \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

/* --- 1. Sparsity Bitmap, MSB Early-ReLU Termination & Base-Delta Compression --- */
static void run_sparsity_and_basedelta(void) {
    printf("=== 1. Ineffectual Ops (Zero Bitmap + Early ReLU) & Base-Delta Compression ===\n");
    srand(2026);
    const int N_VEC = 4096, DIM = 256; /* 1,048,576 MACs */
    long total_macs = (long)N_VEC * DIM;
    long macs_after_zero_skip = 0, macs_after_early_relu = 0;
    int relu_exact_matches = 0;

    for (int v = 0; v < N_VEC; v++) {
        int32_t x[DIM], w[DIM];
        uint64_t mask_x[4] = {0}, mask_w[4] = {0};
        int32_t full_acc = 0, msb_est = 0;

        for (int i = 0; i < DIM; i++) {
            /* Post-ReLU activation: ~55% zeros, positive [1..127] */
            x[i] = (rand() % 100 < 55) ? 0 : (1 + rand() % 127);
            /* Pruned signed filter weights: ~70% zeros, [-64..63] */
            w[i] = (rand() % 100 < 70) ? 0 : ((rand() % 128) - 64);
            /* Bias slightly negative as typical in sparse feature detectors */
            if (w[i] > 0 && (rand() % 100 < 20)) w[i] = -w[i];

            if (x[i] != 0) mask_x[i >> 6] |= (1ULL << (i & 63));
            if (w[i] != 0) mask_w[i >> 6] |= (1ULL << (i & 63));
            full_acc += x[i] * w[i];
            /* Coarse MSB-only estimate (top 4 bits of x and w) */
            msb_est += (x[i] & ~15) * (w[i] / 16 * 16);
        }

        int nz_pairs = 0;
        for (int b = 0; b < 4; b++) {
            nz_pairs += __builtin_popcountll(mask_x[b] & mask_w[b]);
        }
        macs_after_zero_skip += nz_pairs;

        /* If MSB prefix estimate is strongly negative, skip full fine MACs (ReLU -> 0) */
        int pred_relu;
        if (msb_est < -2048) {
            pred_relu = 0; /* early-terminated! (counted as 1/4 cost for 4-bit MSB check) */
            macs_after_early_relu += nz_pairs / 4;
        } else {
            pred_relu = (full_acc > 0) ? full_acc : 0;
            macs_after_early_relu += nz_pairs;
        }
        int true_relu = (full_acc > 0) ? full_acc : 0;
        if (pred_relu == true_relu) relu_exact_matches++;
    }

    printf("  Dense MACs = %ld | Bitmap Zero-Skip MACs = %ld (%.1f%% saved!)\n",
           total_macs, macs_after_zero_skip,
           100.0 * (1.0 - (double)macs_after_zero_skip / total_macs));
    printf("  + MSB Early-ReLU Termination effective MACs = %ld (%.1f%% saved, exact ReLU match = %.2f%%)\n",
           macs_after_early_relu,
           100.0 * (1.0 - (double)macs_after_early_relu / total_macs),
           100.0 * relu_exact_matches / N_VEC);

    /* Base-Delta Compression + Distributive Dot Product [12:55]-[14:18] */
    const int vals[4] = {100, 101, 103, 104};
    const int wts[4]  = {3, -5, 2, 4};
    const int base = 102;
    const int deltas[4] = {-2, -1, +1, +2}; /* fits in 3-bit signed [-4..3] vs 8-bit original! */
    int dot_raw = 0, sum_w = 0, dot_delta = 0;
    for (int i = 0; i < 4; i++) {
        dot_raw   += vals[i] * wts[i];
        sum_w     += wts[i];
        dot_delta += deltas[i] * wts[i];
    }
    int dot_distributive = base * sum_w + dot_delta;
    printf("  Base-Delta [100,101,103,104] (base=102, 3-bit deltas): 32b -> 20b (37.5%% saved), "
           "raw_dot=%d == base*sum(w)+delta*w=%d\n", dot_raw, dot_distributive);
}

/* --- 2. Bit-Serial Multiplication via 1D Bit-Vector Convolution + Zero-Chunk Skip --- */
static uint32_t bit_conv_mul16(uint16_t a, uint16_t b, int *chunks_run) {
    /* Verify Sarangi [16:01]-[20:27]: binary product = 1D convolution of bit vectors + carry ripple */
    int conv[31] = {0};
    for (int i = 0; i < 16; i++) {
        int ai = (a >> i) & 1;
        for (int j = 0; j < 16; j++) {
            int bj = (b >> j) & 1;
            conv[i + j] += ai * bj; /* 1D convolution at lag k = i + j */
        }
    }
    uint32_t res = 0, carry = 0;
    for (int k = 0; k < 32; k++) {
        uint32_t s = (k < 31 ? (uint32_t)conv[k] : 0) + carry;
        res |= (s & 1U) << k;
        carry = s >> 1;
    }
    /* Count non-zero 4-bit chunks in multiplier b ([22:03] chunk skipping) */
    int nz_chunks = 0;
    for (int c = 0; c < 4; c++) {
        if (((b >> (c * 4)) & 0xF) != 0) nz_chunks++;
    }
    *chunks_run += nz_chunks;
    return res;
}

static void run_bit_serial_verification(void) {
    printf("\n=== 2. Bit-Serial Multiplier = 1D Bit-Convolution + 4-bit Zero-Chunk Skip ===\n");
    int mismatches = 0, chunks_used = 0;
    const int TRIALS = 10000;
    for (int t = 0; t < TRIALS; t++) {
        uint16_t a = (uint16_t)(rand() & 0xFFFF);
        /* Typical quantized activation where upper chunks are frequently zero */
        uint16_t b = (t % 2 == 0) ? 0 : (uint16_t)(rand() & 0x00FF);
        uint32_t got = bit_conv_mul16(a, b, &chunks_used);
        if (got != (uint32_t)a * (uint32_t)b) mismatches++;
    }
    printf("  10,000 16-bit products via 1D Bit-Convolution: mismatches = %d | "
           "4-bit chunks executed = %.1f%% (%.1f%% skipped!)\n",
           mismatches, 100.0 * chunks_used / (TRIALS * 4.0),
           100.0 * (1.0 - chunks_used / (TRIALS * 4.0)));
}

/* --- 3. GPU Precision Reduction: FP32 vs INT8 (__dp4a & Tensor Core INT8) --- */
__global__ void gemm_int8_dp4a(const int8_t *__restrict__ A,
                               const int8_t *__restrict__ B_T,
                               int32_t *__restrict__ C,
                               int M, int N, int K) {
    int col = blockIdx.x * blockDim.x + threadIdx.x;
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    if (row >= M || col >= N) return;
    const int *a4 = reinterpret_cast<const int *>(A + (size_t)row * K);
    const int *b4 = reinterpret_cast<const int *>(B_T + (size_t)col * K);
    int acc = 0;
    for (int k4 = 0; k4 < K / 4; k4++) {
        acc = __dp4a(a4[k4], b4[k4], acc); /* 4x INT8 MACs in 1 instruction! */
    }
    C[(size_t)row * N + col] = acc;
}

__global__ void gemm_fp32_naive(const float *__restrict__ A,
                                const float *__restrict__ B_T,
                                float *__restrict__ C,
                                int M, int N, int K) {
    int col = blockIdx.x * blockDim.x + threadIdx.x;
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    if (row >= M || col >= N) return;
    const float *ar = A + (size_t)row * K;
    const float *bc = B_T + (size_t)col * K;
    float acc = 0.0f;
    for (int k = 0; k < K; k++) acc += ar[k] * bc[k];
    C[(size_t)row * N + col] = acc;
}

/* --- 4. ReRAM Dual-Column (G+ / G-) Current-Summing Crossbar Simulator (§14.5.2) --- */
static double rand_gauss(void) {
    double u1 = ((double)rand() + 1.0) / ((double)RAND_MAX + 2.0);
    double u2 = ((double)rand() + 1.0) / ((double)RAND_MAX + 2.0);
    return sqrt(-2.0 * log(u1)) * cos(2.0 * 3.14159265358979323846 * u2);
}

static void run_reram_pim_sim(void) {
    printf("\n=== 4. ReRAM Dual-Column (G+/G-) Current-Summing PIM Crossbar (§14.5.2, Fig. 14.16b) ===\n");
    const int ROWS = 128, COLS = 64;
    float x[ROWS], W[ROWS][COLS], y_ideal[COLS];
    for (int i = 0; i < ROWS; i++) x[i] = (float)(rand() % 16) / 15.0f; /* 4-bit DAC V_in in [0, 1V] */
    for (int j = 0; j < COLS; j++) {
        y_ideal[j] = 0.0f;
        for (int i = 0; i < ROWS; i++) {
            W[i][j] = ((rand() % 31) - 15) / 15.0f; /* 4-bit signed weight in [-1, +1] */
            y_ideal[j] += x[i] * W[i][j];
        }
    }

    const double G_ON = 100e-6; /* 100 uS max conductance (R_min = 10 kOhm) */
    const double G_OFF = 2e-6;  /*   2 uS leakage floor   (R_max = 500 kOhm) */
    int adc_bits_list[3] = {8, 8, 6};
    double sigma_list[3] = {0.00, 0.05, 0.05};

    for (int cfg = 0; cfg < 3; cfg++) {
        int adc_bits = adc_bits_list[cfg];
        double sigma = sigma_list[cfg];
        double dot_num = 0.0, n_ideal = 0.0, n_pim = 0.0, rel_err_sum = 0.0;

        for (int j = 0; j < COLS; j++) {
            double i_pos = 0.0, i_neg = 0.0;
            for (int i = 0; i < ROWS; i++) {
                double w = W[i][j];
                /* Textbook p.735: split signed weight into G+ column and G- column! */
                double g_plus  = G_OFF + (w > 0 ?  w : 0.0) * (G_ON - G_OFF);
                double g_minus = G_OFF + (w < 0 ? -w : 0.0) * (G_ON - G_OFF);
                g_plus  *= (1.0 + sigma * rand_gauss());
                g_minus *= (1.0 + sigma * rand_gauss());
                /* Ohm's Law I = V * G + Kirchhoff's Current Law column sum */
                i_pos += x[i] * g_plus;
                i_neg += x[i] * g_minus;
            }
            /* Quantize column currents with B-bit ADC */
            double i_max = ROWS * G_ON * 0.45;
            int levels = (1 << adc_bits) - 1;
            double q_pos = round(fmin(i_pos / i_max, 1.0) * levels) / levels * i_max;
            double q_neg = round(fmin(i_neg / i_max, 1.0) * levels) / levels * i_max;
            double y_pim = (q_pos - q_neg) / (G_ON - G_OFF);

            dot_num += y_ideal[j] * y_pim;
            n_ideal += y_ideal[j] * y_ideal[j];
            n_pim   += y_pim * y_pim;
            rel_err_sum += fabs(y_pim - y_ideal[j]) / (fabs(y_ideal[j]) + 1e-3);
        }
        double cos_sim = dot_num / (sqrt(n_ideal) * sqrt(n_pim));
        printf("  Config (sigma_G=%4.1f%%, %d-bit ADC): Cosine Similarity = %.5f\n",
               sigma * 100.0, adc_bits, cos_sim);
    }
}

int main(void) {
    run_sparsity_and_basedelta();
    run_bit_serial_verification();

    cudaDeviceProp prop;
    CHECK_CUDA(cudaGetDeviceProperties(&prop, 0));
    printf("\n=== 3. GPU Precision Reduction (FP32 vs INT8 __dp4a vs INT8 Tensor Cores) on %s ===\n",
           prop.name);

    /* Large FC / im2col GEMM: M=2048, N=2048, K=2048 (17.18 GOPS) */
    const int M = 2048, N = 2048, K = 2048;
    const double ops = 2.0 * M * N * K;
    size_t n_elem = (size_t)M * K;

    float *d_af, *d_bf, *d_cf;
    int8_t *d_ai, *d_bi;
    int32_t *d_ci;
    CHECK_CUDA(cudaMalloc(&d_af, n_elem * sizeof(float)));
    CHECK_CUDA(cudaMalloc(&d_bf, n_elem * sizeof(float)));
    CHECK_CUDA(cudaMalloc(&d_cf, n_elem * sizeof(float)));
    CHECK_CUDA(cudaMalloc(&d_ai, n_elem * sizeof(int8_t)));
    CHECK_CUDA(cudaMalloc(&d_bi, n_elem * sizeof(int8_t)));
    CHECK_CUDA(cudaMalloc(&d_ci, n_elem * sizeof(int32_t)));
    CHECK_CUDA(cudaMemset(d_af, 1, n_elem * sizeof(float)));
    CHECK_CUDA(cudaMemset(d_bf, 1, n_elem * sizeof(float)));
    CHECK_CUDA(cudaMemset(d_ai, 2, n_elem * sizeof(int8_t)));
    CHECK_CUDA(cudaMemset(d_bi, 3, n_elem * sizeof(int8_t)));

    cublasHandle_t cublas;
    cublasCreate(&cublas);
    float alpha_f = 1.0f, beta_f = 0.0f;
    int32_t alpha_i = 1, beta_i = 0;

    cudaEvent_t e0, e1;
    CHECK_CUDA(cudaEventCreate(&e0));
    CHECK_CUDA(cudaEventCreate(&e1));
    dim3 blk(16, 16), grd(N / 16, M / 16);

    /* Warmup */
    gemm_fp32_naive<<<grd, blk>>>(d_af, d_bf, d_cf, M, N, K);
    gemm_int8_dp4a<<<grd, blk>>>(d_ai, d_bi, d_ci, M, N, K);
    cublasGemmEx(cublas, CUBLAS_OP_T, CUBLAS_OP_N, N, M, K,
                 &alpha_f, d_bf, CUDA_R_32F, K, d_af, CUDA_R_32F, K,
                 &beta_f, d_cf, CUDA_R_32F, N, CUBLAS_COMPUTE_32F, CUBLAS_GEMM_DEFAULT);
    cublasGemmEx(cublas, CUBLAS_OP_T, CUBLAS_OP_N, N, M, K,
                 &alpha_i, d_bi, CUDA_R_8I, K, d_ai, CUDA_R_8I, K,
                 &beta_i, d_ci, CUDA_R_32I, N, CUBLAS_COMPUTE_32I, CUBLAS_GEMM_DEFAULT_TENSOR_OP);
    CHECK_CUDA(cudaDeviceSynchronize());

    float ms_fp32_simt = 0, ms_int8_dp4a = 0, ms_fp32_cublas = 0, ms_int8_tc = 0, t;
    const int REPS = 20;
    for (int r = 0; r < REPS; r++) {
        CHECK_CUDA(cudaEventRecord(e0));
        gemm_fp32_naive<<<grd, blk>>>(d_af, d_bf, d_cf, M, N, K);
        CHECK_CUDA(cudaEventRecord(e1));
        CHECK_CUDA(cudaEventSynchronize(e1));
        CHECK_CUDA(cudaEventElapsedTime(&t, e0, e1));
        ms_fp32_simt += t;

        CHECK_CUDA(cudaEventRecord(e0));
        gemm_int8_dp4a<<<grd, blk>>>(d_ai, d_bi, d_ci, M, N, K);
        CHECK_CUDA(cudaEventRecord(e1));
        CHECK_CUDA(cudaEventSynchronize(e1));
        CHECK_CUDA(cudaEventElapsedTime(&t, e0, e1));
        ms_int8_dp4a += t;

        CHECK_CUDA(cudaEventRecord(e0));
        cublasGemmEx(cublas, CUBLAS_OP_T, CUBLAS_OP_N, N, M, K,
                     &alpha_f, d_bf, CUDA_R_32F, K, d_af, CUDA_R_32F, K,
                     &beta_f, d_cf, CUDA_R_32F, N, CUBLAS_COMPUTE_32F, CUBLAS_GEMM_DEFAULT);
        CHECK_CUDA(cudaEventRecord(e1));
        CHECK_CUDA(cudaEventSynchronize(e1));
        CHECK_CUDA(cudaEventElapsedTime(&t, e0, e1));
        ms_fp32_cublas += t;

        CHECK_CUDA(cudaEventRecord(e0));
        cublasGemmEx(cublas, CUBLAS_OP_T, CUBLAS_OP_N, N, M, K,
                     &alpha_i, d_bi, CUDA_R_8I, K, d_ai, CUDA_R_8I, K,
                     &beta_i, d_ci, CUDA_R_32I, N, CUBLAS_COMPUTE_32I, CUBLAS_GEMM_DEFAULT_TENSOR_OP);
        CHECK_CUDA(cudaEventRecord(e1));
        CHECK_CUDA(cudaEventSynchronize(e1));
        CHECK_CUDA(cudaEventElapsedTime(&t, e0, e1));
        ms_int8_tc += t;
    }
    ms_fp32_simt /= REPS; ms_int8_dp4a /= REPS; ms_fp32_cublas /= REPS; ms_int8_tc /= REPS;

    printf("  Weight Footprint (2048x2048): FP32 = %.2f MiB -> INT8 = %.2f MiB (4.0x memory savings!)\n",
           n_elem * 4.0 / (1024 * 1024), n_elem * 1.0 / (1024 * 1024));
    printf("  Naive SIMT   FP32           : %6.3f ms (%7.2f GFLOP/s,  1.00x)\n",
           ms_fp32_simt, ops / (ms_fp32_simt * 1e6));
    printf("  Naive SIMT   INT8 (__dp4a)  : %6.3f ms (%7.2f GIOP/s,  %5.2fx)\n",
           ms_int8_dp4a, ops / (ms_int8_dp4a * 1e6), ms_fp32_simt / ms_int8_dp4a);
    printf("  cuBLAS Tiled FP32           : %6.3f ms (%7.2f GFLOP/s, %5.2fx)\n",
           ms_fp32_cublas, ops / (ms_fp32_cublas * 1e6), ms_fp32_simt / ms_fp32_cublas);
    printf("  cuBLAS Tiled INT8 TensorCore: %6.3f ms (%7.2f GIOP/s,  %5.2fx; %.2fx over cuBLAS FP32!)\n",
           ms_int8_tc, ops / (ms_int8_tc * 1e6), ms_fp32_simt / ms_int8_tc,
           ms_fp32_cublas / ms_int8_tc);

    run_reram_pim_sim();

    cublasDestroy(cublas);
    cudaFree(d_af); cudaFree(d_bf); cudaFree(d_cf);
    cudaFree(d_ai); cudaFree(d_bi); cudaFree(d_ci);
    return 0;
}
