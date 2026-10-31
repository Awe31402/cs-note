/*
 * Lecture 57 Experiment: Retiming Lemma, 1D Pure Systolic Array & GPU Tensor Cores
 * 1. Programmatic Bellman-Ford verification of the Retiming Lemma on Figure 14.13
 *    (detecting negative cycle in G-1, computing lags [-1..-6] in 2G-1) + cycle-by-cycle
 *    simulation of Figure 14.14 (50.0% bubble vs 100.0% 2-way interleaved utilization).
 * 2. Textbook §14.3.2 Matrix-Product Lowering (im2col / Toeplitz) on RTX 4060 GPU:
 *    Direct 2D Conv (CUDA cores) vs Unbuffered WMMA (NLR) vs Tiled Tensor Core GEMM (cuBLAS)!
 */
#include <cublas_v2.h>
#include <cuda_fp16.h>
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

/* --- Part 1: Bellman-Ford Retiming & Cycle-Accurate 1D Systolic Simulator --- */
typedef struct { int u, v, w; const char *kind; } Edge;

static int bellman_ford_lags(int n_nodes, int host, const Edge *edges, int n_edges,
                             int scale_k, int *lags) {
    for (int i = 0; i < n_nodes; i++) lags[i] = 1000000;
    lags[host] = 0;
    for (int iter = 0; iter < n_nodes - 1; iter++) {
        for (int e = 0; e < n_edges; e++) {
            int wt = scale_k * edges[e].w - 1;
            if (lags[edges[e].v] < 900000 && wt + lags[edges[e].v] < lags[edges[e].u]) {
                lags[edges[e].u] = wt + lags[edges[e].v];
            }
        }
    }
    for (int e = 0; e < n_edges; e++) {
        int wt = scale_k * edges[e].w - 1;
        if (lags[edges[e].v] < 900000 && wt + lags[edges[e].v] < lags[edges[e].u]) {
            return -1;
        }
    }
    return 0;
}

static void run_retiming_and_1d_systolic(void) {
    const int HOST = 6, N_NODES = 7;
    Edge edges[] = {
        {5, 4, 0, "w_bcast"}, {4, 3, 0, "w_bcast"}, {3, 2, 0, "w_bcast"},
        {2, 1, 0, "w_bcast"}, {1, 0, 0, "w_bcast"}, {0, HOST, 0, "to_host"},
        {0, 1, 1, "x_shift"}, {1, 2, 1, "x_shift"}, {2, 3, 1, "x_shift"},
        {3, 4, 1, "x_shift"}, {4, 5, 1, "x_shift"}
    };
    int n_edges = (int)(sizeof(edges) / sizeof(edges[0]));
    int lags[7];

    printf("=== 1. Retiming Lemma (Bellman-Ford on G-1 vs 2G-1, Fig. 14.13 -> 14.14) ===\n");
    int rc1 = bellman_ford_lags(N_NODES, HOST, edges, n_edges, 1, lags);
    printf("  Check k=1 (G - 1) : %s\n",
           rc1 < 0 ? "NEGATIVE CYCLE DETECTED (0 + (-1) = -1) -> cannot retime at k=1!" : "OK");
    int rc2 = bellman_ford_lags(N_NODES, HOST, edges, n_edges, 2, lags);
    printf("  Check k=2 (2G - 1): %s | lags(PE1..PE6) = [%d, %d, %d, %d, %d, %d]\n",
           rc2 == 0 ? "NO NEGATIVE CYCLES" : "FAIL",
           lags[0], lags[1], lags[2], lags[3], lags[4], lags[5]);

    int min_reg = 999;
    for (int e = 0; e < n_edges; e++) {
        if (edges[e].v == HOST) continue;
        int w_new = 2 * edges[e].w + lags[edges[e].v] - lags[edges[e].u];
        if (w_new < min_reg) min_reg = w_new;
    }
    printf("  Retimed 2G edges  : w_bcast (0 -> 1 reg), x_shift (1 -> 1 reg), min_reg=%d (PURE SYSTOLIC!)\n",
           min_reg);

    /* Simulate Textbook Figure 14.14 (p.727): 6 MACs storing y6..y1 (cols 0..5) */
    const int W_A[4] = {2, -1, 3, 4};                 /* w1..w4 */
    const int X_A[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};   /* x1..x9 */
    const int W_B[4] = {1, 5, -2, 2};
    const int X_B[9] = {9, 7, 5, 3, 1, 2, 4, 6, 8};

    int w_reg[6] = {0}, x_reg[6] = {0}, y_single[6] = {0};
    x_reg[1] = X_A[2]; x_reg[3] = X_A[1]; x_reg[5] = X_A[0];
    w_reg[5] = W_A[0];
    int active_single = 0, full_slots = 0;

    for (int t = 1; t <= 14; t++) {
        for (int c = 0; c < 6; c++) {
            if (x_reg[c] != 0 && w_reg[c] != 0) {
                y_single[5 - c] += x_reg[c] * w_reg[c];
                if (t >= 6 && t <= 7) active_single++;
            }
            if (t >= 6 && t <= 7) full_slots++;
        }
        for (int c = 5; c > 0; c--) x_reg[c] = x_reg[c - 1];
        int nt = t + 1;
        x_reg[0] = (nt % 2 == 0 && (3 + nt / 2 - 1) < 9) ? X_A[3 + nt / 2 - 1] : 0;

        for (int c = 0; c < 5; c++) w_reg[c] = w_reg[c + 1];
        w_reg[5] = (nt % 2 == 1 && (nt / 2) < 4) ? W_A[nt / 2] : 0;
    }

    int yA[6] = {0}, yB[6] = {0}, tag_x[6] = {0}, tag_w[6] = {0};
    memset(x_reg, 0, sizeof(x_reg));
    memset(w_reg, 0, sizeof(w_reg));
    x_reg[1] = X_A[2]; tag_x[1] = 1; x_reg[3] = X_A[1]; tag_x[3] = 1; x_reg[5] = X_A[0]; tag_x[5] = 1;
    x_reg[0] = X_B[2]; tag_x[0] = 2; x_reg[2] = X_B[1]; tag_x[2] = 2; x_reg[4] = X_B[0]; tag_x[4] = 2;
    w_reg[5] = W_A[0]; tag_w[5] = 1;
    int active_dual = 0;

    for (int t = 1; t <= 14; t++) {
        for (int c = 0; c < 6; c++) {
            if (tag_x[c] != 0 && tag_x[c] == tag_w[c]) {
                if (tag_x[c] == 1) yA[5 - c] += x_reg[c] * w_reg[c];
                else               yB[5 - c] += x_reg[c] * w_reg[c];
                if (t >= 6 && t <= 7) active_dual++;
            }
        }
        for (int c = 5; c > 0; c--) { x_reg[c] = x_reg[c - 1]; tag_x[c] = tag_x[c - 1]; }
        int nt = t + 1;
        if (nt % 2 == 0 && (3 + nt / 2 - 1) < 9) {
            x_reg[0] = X_A[3 + nt / 2 - 1]; tag_x[0] = 1;
        } else if (nt % 2 == 1 && (3 + (nt - 1) / 2 - 1) < 9) {
            x_reg[0] = X_B[3 + (nt - 1) / 2 - 1]; tag_x[0] = 2;
        } else {
            x_reg[0] = 0; tag_x[0] = 0;
        }

        for (int c = 0; c < 5; c++) { w_reg[c] = w_reg[c + 1]; tag_w[c] = tag_w[c + 1]; }
        if (nt % 2 == 1 && (nt / 2) < 4) {
            w_reg[5] = W_A[nt / 2]; tag_w[5] = 1;
        } else if (nt % 2 == 0 && (nt / 2 - 1) < 4) {
            w_reg[5] = W_B[nt / 2 - 1]; tag_w[5] = 2;
        } else {
            w_reg[5] = 0; tag_w[5] = 0;
        }
    }

    int okA = 1, okB = 1;
    for (int i = 0; i < 6; i++) {
        int refA = 0, refB = 0;
        for (int j = 0; j < 4; j++) { refA += W_A[j] * X_A[i + j]; refB += W_B[j] * X_B[i + j]; }
        if (y_single[i] != refA || yA[i] != refA) okA = 0;
        if (yB[i] != refB) okB = 0;
    }
    printf("  Fig. 14.14 Output : StreamA=%s [%d,%d,%d,%d,%d,%d] | StreamB=%s [%d,%d,%d,%d,%d,%d]\n",
           okA ? "EXACT" : "MISMATCH", yA[0], yA[1], yA[2], yA[3], yA[4], yA[5],
           okB ? "EXACT" : "MISMATCH", yB[0], yB[1], yB[2], yB[3], yB[4], yB[5]);
    printf("  Steady-State MAC Utilization (t=6..7): Single-Conv = %5.1f%% | 2-Way Interleaved = %5.1f%%\n",
           100.0 * active_single / full_slots, 100.0 * active_dual / full_slots);
}

/* --- Part 2: Direct 2D Conv vs Unbuffered WMMA (NLR) vs Tiled Tensor Core GEMM --- */
__global__ void direct_conv2d_fp32(const half *__restrict__ I,
                                   const half *__restrict__ F,
                                   float *__restrict__ O,
                                   int N, int C, int H, int W, int K) {
    int w = blockIdx.x * blockDim.x + threadIdx.x;
    int h = blockIdx.y * blockDim.y + threadIdx.y;
    int nk = blockIdx.z;
    if (w >= W || h >= H || nk >= N * K) return;
    int n = nk / K, k = nk % K;
    float sum = 0.0f;
    for (int c = 0; c < C; c++) {
        const half *in_nc = I + ((size_t)n * C + c) * H * W;
        const half *f_kc  = F + ((size_t)k * C + c) * 9;
        for (int r = 0; r < 3; r++) {
            int ih = h + r - 1;
            if (ih < 0 || ih >= H) continue;
            for (int s = 0; s < 3; s++) {
                int iw = w + s - 1;
                if (iw >= 0 && iw < W)
                    sum += __half2float(in_nc[ih * W + iw]) * __half2float(f_kc[r * 3 + s]);
            }
        }
    }
    O[((size_t)n * K + k) * H * W + h * W + w] = sum;
}

__global__ void im2col_3x3_kernel(const half *__restrict__ I,
                                  half *__restrict__ X_col,
                                  int N, int C, int H, int W, int K_red) {
    int patch = blockIdx.x * blockDim.x + threadIdx.x;
    int c = blockIdx.y;
    if (patch >= N * H * W || c >= C) return;

    int n = patch / (H * W);
    int rem = patch % (H * W);
    int h = rem / W, w = rem % W;

    const half *in_nc = I + ((size_t)n * C + c) * H * W;
    half *dst = X_col + (size_t)patch * K_red + c * 9;
    #pragma unroll
    for (int r = 0; r < 3; r++) {
        int ih = h + r - 1;
        #pragma unroll
        for (int s = 0; s < 3; s++) {
            int iw = w + s - 1;
            dst[r * 3 + s] = (ih >= 0 && ih < H && iw >= 0 && iw < W)
                                 ? in_nc[ih * W + iw]
                                 : __float2half(0.0f);
        }
    }
}

/* NLR-style WMMA: directly streams 16x16 tiles from Global Memory without Shared-Mem reuse */
__global__ void wmma_nlr_gemm(const half *__restrict__ A,
                              const half *__restrict__ B_col_major,
                              float *__restrict__ C_out,
                              int M, int N_cols, int K_red) {
    int warpM = (blockIdx.x * blockDim.x + threadIdx.x) / 32;
    int warpN = blockIdx.y;
    int row = warpM * 16, col = warpN * 16;
    if (row >= M || col >= N_cols) return;

    wmma::fragment<wmma::matrix_a, 16, 16, 16, half, wmma::row_major> a_frag;
    wmma::fragment<wmma::matrix_b, 16, 16, 16, half, wmma::col_major> b_frag;
    wmma::fragment<wmma::accumulator, 16, 16, 16, float> acc_frag;
    wmma::fill_fragment(acc_frag, 0.0f);

    for (int k = 0; k < K_red; k += 16) {
        wmma::load_matrix_sync(a_frag, A + (size_t)row * K_red + k, K_red);
        wmma::load_matrix_sync(b_frag, B_col_major + (size_t)col * K_red + k, K_red);
        wmma::mma_sync(acc_frag, a_frag, b_frag, acc_frag);
    }
    wmma::store_matrix_sync(C_out + (size_t)row * N_cols + col, acc_frag, N_cols, wmma::mem_row_major);
}

int main(void) {
    run_retiming_and_1d_systolic();

    cudaDeviceProp prop;
    CHECK_CUDA(cudaGetDeviceProperties(&prop, 0));
    printf("\n=== 2. §14.3.2 Matrix-Product Lowering + Hardware Tensor Cores on %s ===\n", prop.name);

    const int N = 16, C = 128, K = 128, H = 56, W = 56;
    const int M_mat = N * H * W; /* 50,176 */
    const int K_red = C * 9;     /* 1,152  */
    const double flops = 2.0 * M_mat * K * K_red; /* 14.798 GFLOPs */

    size_t sz_in  = (size_t)N * C * H * W;
    size_t sz_flt = (size_t)K * C * 9;
    size_t sz_col = (size_t)M_mat * K_red;
    size_t sz_out = (size_t)N * K * H * W;

    half *h_in = (half *)malloc(sz_in * sizeof(half));
    half *h_f  = (half *)malloc(sz_flt * sizeof(half));
    float *h_o1 = (float *)malloc(sz_out * sizeof(float));
    float *h_o2 = (float *)malloc(sz_out * sizeof(float));
    srand(7);
    for (size_t i = 0; i < sz_in; i++)  h_in[i] = __float2half(((rand() % 200) - 100) * 0.01f);
    for (size_t i = 0; i < sz_flt; i++) h_f[i]  = __float2half(((rand() % 200) - 100) * 0.01f);

    half *d_in, *d_f, *d_col;
    float *d_o1, *d_o2, *d_o3;
    CHECK_CUDA(cudaMalloc(&d_in,  sz_in * sizeof(half)));
    CHECK_CUDA(cudaMalloc(&d_f,   sz_flt * sizeof(half)));
    CHECK_CUDA(cudaMalloc(&d_col, sz_col * sizeof(half)));
    CHECK_CUDA(cudaMalloc(&d_o1,  sz_out * sizeof(float)));
    CHECK_CUDA(cudaMalloc(&d_o2,  sz_out * sizeof(float)));
    CHECK_CUDA(cudaMalloc(&d_o3,  sz_out * sizeof(float)));
    CHECK_CUDA(cudaMemcpy(d_in, h_in, sz_in * sizeof(half), cudaMemcpyHostToDevice));
    CHECK_CUDA(cudaMemcpy(d_f,  h_f,  sz_flt * sizeof(half), cudaMemcpyHostToDevice));

    cublasHandle_t cublas;
    cublasCreate(&cublas);
    float alpha = 1.0f, beta = 0.0f;

    cudaEvent_t e0, e1, e2;
    CHECK_CUDA(cudaEventCreate(&e0));
    CHECK_CUDA(cudaEventCreate(&e1));
    CHECK_CUDA(cudaEventCreate(&e2));

    dim3 b2d(16, 16);
    dim3 g2d((W + 15) / 16, (H + 15) / 16, N * K);
    /* Warmup */
    direct_conv2d_fp32<<<g2d, b2d>>>(d_in, d_f, d_o1, N, C, H, W, K);
    im2col_3x3_kernel<<<dim3((M_mat + 255) / 256, C), 256>>>(d_in, d_col, N, C, H, W, K_red);
    wmma_nlr_gemm<<<dim3((M_mat / 16 + 3) / 4, K / 16), 128>>>(d_col, d_f, d_o2, M_mat, K, K_red);
    /* Compute C_row_major[M_mat x K] = X_col[M_mat x K_red] * F^T[K_red x K]
     * In col-major cuBLAS: C_col[K x M_mat] = F_col^T[K x K_red] * X_col_T[K_red x M_mat] */
    cublasGemmEx(cublas, CUBLAS_OP_T, CUBLAS_OP_N, K, M_mat, K_red,
                 &alpha, d_f, CUDA_R_16F, K_red, d_col, CUDA_R_16F, K_red,
                 &beta, d_o3, CUDA_R_32F, K, CUBLAS_COMPUTE_32F, CUBLAS_GEMM_DEFAULT_TENSOR_OP);
    CHECK_CUDA(cudaDeviceSynchronize());

    const int REPS = 20;
    float ms_direct = 0, ms_im2col = 0, ms_wmma_nlr = 0, ms_cublas_tc = 0, t_a, t_b;
    for (int r = 0; r < REPS; r++) {
        CHECK_CUDA(cudaEventRecord(e0));
        direct_conv2d_fp32<<<g2d, b2d>>>(d_in, d_f, d_o1, N, C, H, W, K);
        CHECK_CUDA(cudaEventRecord(e1));
        CHECK_CUDA(cudaEventSynchronize(e1));
        CHECK_CUDA(cudaEventElapsedTime(&t_a, e0, e1));
        ms_direct += t_a;

        CHECK_CUDA(cudaEventRecord(e0));
        im2col_3x3_kernel<<<dim3((M_mat + 255) / 256, C), 256>>>(d_in, d_col, N, C, H, W, K_red);
        CHECK_CUDA(cudaEventRecord(e1));
        wmma_nlr_gemm<<<dim3((M_mat / 16 + 3) / 4, K / 16), 128>>>(d_col, d_f, d_o2, M_mat, K, K_red);
        CHECK_CUDA(cudaEventRecord(e2));
        CHECK_CUDA(cudaEventSynchronize(e2));
        CHECK_CUDA(cudaEventElapsedTime(&t_a, e0, e1));
        CHECK_CUDA(cudaEventElapsedTime(&t_b, e1, e2));
        ms_im2col   += t_a;
        ms_wmma_nlr += t_b;

        CHECK_CUDA(cudaEventRecord(e0));
        cublasGemmEx(cublas, CUBLAS_OP_T, CUBLAS_OP_N, K, M_mat, K_red,
                     &alpha, d_f, CUDA_R_16F, K_red, d_col, CUDA_R_16F, K_red,
                     &beta, d_o3, CUDA_R_32F, K, CUBLAS_COMPUTE_32F, CUBLAS_GEMM_DEFAULT_TENSOR_OP);
        CHECK_CUDA(cudaEventRecord(e1));
        CHECK_CUDA(cudaEventSynchronize(e1));
        CHECK_CUDA(cudaEventElapsedTime(&t_a, e0, e1));
        ms_cublas_tc += t_a;
    }
    ms_direct /= REPS; ms_im2col /= REPS; ms_wmma_nlr /= REPS; ms_cublas_tc /= REPS;

    CHECK_CUDA(cudaMemcpy(h_o1, d_o1, sz_out * sizeof(float), cudaMemcpyDeviceToHost));
    CHECK_CUDA(cudaMemcpy(h_o2, d_o3, sz_out * sizeof(float), cudaMemcpyDeviceToHost));
    float max_err = 0.0f;
    for (int n = 0; n < N; n++)
        for (int h = 0; h < H; h++)
            for (int w = 0; w < W; w++) {
                int patch = n * H * W + h * W + w;
                for (int k = 0; k < K; k++) {
                    float v1 = h_o1[((size_t)n * K + k) * H * W + h * W + w];
                    float v2 = h_o2[(size_t)patch * K + k];
                    float err = fabsf(v1 - v2);
                    if (err > max_err) max_err = err;
                }
            }

    printf("  Memory Footprint : Original ifmap = %.2f MiB -> Unrolled im2col = %.2f MiB (%.1fx expansion!)\n",
           sz_in * 2.0 / (1024 * 1024), sz_col * 2.0 / (1024 * 1024), (double)sz_col / sz_in);
    printf("  (a) Direct 2D Conv (FP32 CUDA Cores)         : %6.3f ms (%8.2f GFLOP/s,  1.00x)\n",
           ms_direct, flops / (ms_direct * 1e6));
    printf("  (b) Unbuffered WMMA Tensor Core (NLR GEMM)   : %6.3f ms (%8.2f GFLOP/s, %5.2fx)\n",
           ms_wmma_nlr, flops / (ms_wmma_nlr * 1e6), ms_direct / ms_wmma_nlr);
    printf("  (c) Tiled Tensor Core GEMM only (cuBLAS)     : %6.3f ms (%8.2f GFLOP/s, %5.2fx)\n",
           ms_cublas_tc, flops / (ms_cublas_tc * 1e6), ms_direct / ms_cublas_tc);
    printf("  (d) im2col (%.3f ms) + Tiled TC GEMM Total  : %6.3f ms (%8.2f GFLOP/s, %5.2fx) | MaxDiff=%.2e\n",
           ms_im2col, ms_im2col + ms_cublas_tc, flops / ((ms_im2col + ms_cublas_tc) * 1e6),
           ms_direct / (ms_im2col + ms_cublas_tc), max_err);

    cublasDestroy(cublas);
    cudaFree(d_in); cudaFree(d_f); cudaFree(d_col); cudaFree(d_o1); cudaFree(d_o2); cudaFree(d_o3);
    free(h_in); free(h_f); free(h_o1); free(h_o2);
    return 0;
}
