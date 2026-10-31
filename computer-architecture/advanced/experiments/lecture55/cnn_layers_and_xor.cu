/*
 * Lecture 55 Experiment: Basic ML Concepts & CNN Layer Architecture Profile
 * - Part 1: Analytical & Gradient-Descent verification of Figure 14.2 (XOR)
 *           showing Linear+Linear collapse (MSE=0.25) vs Linear+ReLU+Linear,
 *           plus Sarangi's [31:14] oral AND-gate weight modification.
 * - Part 2: Hardware ALU/SFU cost of Sigmoid & tanh (transcendental expf)
 *           vs ReLU (max/sign-bit) on RTX 4060 GPU.
 * - Part 3: Multi-channel CNN pipeline (Eq. 14.6 CONV2D + ReLU + 2x2 MaxPool + FC)
 *           verifying >90% CONV compute dominance vs >95% FC parameter bloat.
 */
#include <cuda_runtime.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define CHECK_CUDA(call)                                                       \
    do {                                                                       \
        cudaError_t err = (call);                                              \
        if (err != cudaSuccess) {                                              \
            fprintf(stderr, "CUDA error at %s:%d: %s\n", __FILE__, __LINE__,   \
                    cudaGetErrorString(err));                                   \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

static inline float relu_f(float x) { return x > 0.0f ? x : 0.0f; }

static void run_xor_and_linear_collapse(void) {
    const float X[4][2] = {{0, 0}, {0, 1}, {1, 0}, {1, 1}};
    const float Y_xor[4] = {0, 1, 1, 0};

    printf("=== 1. Analytical Figure 14.2 (XOR) & [31:14] Oral AND Gate ===\n");
    for (int i = 0; i < 4; i++) {
        float a = X[i][0], b = X[i][1];
        /* Textbook Fig 14.2: y = ReLU(a - b) + ReLU(b - a) = |a - b| */
        float h0 = relu_f(1.0f * a - 1.0f * b);
        float h1 = relu_f(-1.0f * a + 1.0f * b);
        float y_xor = h0 + h1;
        /* Lecture [31:14] oral: W1 row=[0.5, 0.5], integer bit interpretation
         * vs exact bias-shifted ReLU(a + b - 1) */
        float y_oral_raw = relu_f(0.5f * a + 0.5f * b);
        int y_oral_int = (int)y_oral_raw;
        float y_relu_and = relu_f(a + b - 1.0f);
        printf("  in=(%.0f, %.0f) -> Fig14.2 XOR=%.1f | Oral [0.5,0.5] raw=%.1f "
               "(int=%d) | Bias ReLU(a+b-1)=%.1f\n",
               a, b, y_xor, y_oral_raw, y_oral_int, y_relu_and);
    }

    /* Train 2-layer Linear-Linear vs Linear-ReLU-Linear via SGD on XOR */
    srand(42);
    float W1_lin[4][2], b1_lin[4], W2_lin[4], b2_lin = 0.0f;
    float W1_mlp[4][2], b1_mlp[4], W2_mlp[4], b2_mlp = 0.0f;
    for (int j = 0; j < 4; j++) {
        for (int k = 0; k < 2; k++) {
            float v = ((float)rand() / RAND_MAX - 0.5f) * 1.2f;
            W1_lin[j][k] = v;
            W1_mlp[j][k] = v;
        }
        b1_lin[j] = b1_mlp[j] = 0.1f;
        W2_lin[j] = W2_mlp[j] = ((float)rand() / RAND_MAX - 0.5f) * 1.2f;
    }

    const float lr = 0.08f;
    for (int epoch = 0; epoch < 4000; epoch++) {
        for (int s = 0; s < 4; s++) {
            /* Model A: Linear -> Linear (no ReLU!) */
            float z_lin[4], pred_lin = b2_lin;
            for (int j = 0; j < 4; j++) {
                z_lin[j] = W1_lin[j][0] * X[s][0] + W1_lin[j][1] * X[s][1] + b1_lin[j];
                pred_lin += W2_lin[j] * z_lin[j];
            }
            float err_lin = pred_lin - Y_xor[s];
            for (int j = 0; j < 4; j++) {
                float dz = err_lin * W2_lin[j];
                W2_lin[j] -= lr * err_lin * z_lin[j];
                W1_lin[j][0] -= lr * dz * X[s][0];
                W1_lin[j][1] -= lr * dz * X[s][1];
                b1_lin[j] -= lr * dz;
            }
            b2_lin -= lr * err_lin;

            /* Model B: Linear -> ReLU -> Linear */
            float h_mlp[4], pred_mlp = b2_mlp;
            for (int j = 0; j < 4; j++) {
                float z = W1_mlp[j][0] * X[s][0] + W1_mlp[j][1] * X[s][1] + b1_mlp[j];
                h_mlp[j] = relu_f(z);
                pred_mlp += W2_mlp[j] * h_mlp[j];
            }
            float err_mlp = pred_mlp - Y_xor[s];
            for (int j = 0; j < 4; j++) {
                float dh = err_mlp * W2_mlp[j];
                float dz = (h_mlp[j] > 0.0f) ? dh : 0.0f;
                W2_mlp[j] -= lr * err_mlp * h_mlp[j];
                W1_mlp[j][0] -= lr * dz * X[s][0];
                W1_mlp[j][1] -= lr * dz * X[s][1];
                b1_mlp[j] -= lr * dz;
            }
            b2_mlp -= lr * err_mlp;
        }
    }

    float mse_lin = 0.0f, mse_mlp = 0.0f;
    printf("\n=== 2. Backprop Training on XOR (4000 epochs): Linear Collapse vs ReLU ===\n");
    for (int s = 0; s < 4; s++) {
        float p_lin = b2_lin, p_mlp = b2_mlp;
        for (int j = 0; j < 4; j++) {
            p_lin += W2_lin[j] * (W1_lin[j][0] * X[s][0] + W1_lin[j][1] * X[s][1] + b1_lin[j]);
            p_mlp += W2_mlp[j] * relu_f(W1_mlp[j][0] * X[s][0] + W1_mlp[j][1] * X[s][1] + b1_mlp[j]);
        }
        mse_lin += (p_lin - Y_xor[s]) * (p_lin - Y_xor[s]);
        mse_mlp += (p_mlp - Y_xor[s]) * (p_mlp - Y_xor[s]);
        printf("  in=(%.0f, %.0f) target=%.0f | Linear+Linear pred=%.4f | Linear+ReLU+Linear pred=%.4f\n",
               X[s][0], X[s][1], Y_xor[s], p_lin, p_mlp);
    }
    printf("  Final MSE -> Linear+Linear: %.6f (stuck at 0.25!) | Linear+ReLU+Linear: %.8f\n",
           mse_lin / 4.0f, mse_mlp / 4.0f);
}

/* Part 2: GPU ALU vs SFU activation microbenchmark (compute-bound 256 iters) */
__global__ void bench_act_relu(float *out, int n) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= n) return;
    float x = (idx & 255) * 0.01f - 1.2f;
    #pragma unroll 1
    for (int i = 0; i < 256; i++) {
        x = fmaxf(0.0f, x) - 0.0001f;
    }
    out[idx] = x;
}

__global__ void bench_act_sigmoid(float *out, int n) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= n) return;
    float x = (idx & 255) * 0.01f - 1.2f;
    #pragma unroll 1
    for (int i = 0; i < 256; i++) {
        x = 1.0f / (1.0f + expf(-x)) - 0.5f;
    }
    out[idx] = x;
}

__global__ void bench_act_tanh(float *out, int n) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= n) return;
    float x = (idx & 255) * 0.01f - 1.2f;
    #pragma unroll 1
    for (int i = 0; i < 256; i++) {
        float ep = expf(x), em = expf(-x);
        x = (ep - em) / (ep + em) * 0.99f;
    }
    out[idx] = x;
}

/* Part 3: Textbook Eq. 14.6 Multi-Channel 2D Convolution + Zero Padding + ReLU + MaxPool + FC */
__global__ void conv2d_eq14_6_kernel(const float *__restrict__ I,
                                     const float *__restrict__ F,
                                     float *__restrict__ O,
                                     int N, int C, int H, int W,
                                     int K, int R, int S, int pad) {
    int w = blockIdx.x * blockDim.x + threadIdx.x;
    int h = blockIdx.y * blockDim.y + threadIdx.y;
    int nk = blockIdx.z;
    if (w >= W || h >= H || nk >= N * K) return;
    int n = nk / K;
    int k = nk % K;

    float sum = 0.0f;
    for (int c = 0; c < C; c++) {
        const float *in_nc = I + ((size_t)n * C + c) * H * W;
        const float *f_kc  = F + ((size_t)k * C + c) * R * S;
        for (int r = 0; r < R; r++) {
            int ih = h + r - pad;
            if (ih < 0 || ih >= H) continue;
            for (int s = 0; s < S; s++) {
                int iw = w + s - pad;
                if (iw >= 0 && iw < W) {
                    sum += in_nc[ih * W + iw] * f_kc[r * S + s];
                }
            }
        }
    }
    O[((size_t)n * K + k) * H * W + h * W + w] = sum;
}

__global__ void relu_kernel(float *X, size_t total) {
    size_t idx = (size_t)blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < total) {
        float v = X[idx];
        X[idx] = v > 0.0f ? v : 0.0f;
    }
}

__global__ void maxpool2x2_kernel(const float *__restrict__ I,
                                  float *__restrict__ O,
                                  int NK, int H, int W) {
    int ow = blockIdx.x * blockDim.x + threadIdx.x;
    int oh = blockIdx.y * blockDim.y + threadIdx.y;
    int nk = blockIdx.z;
    int OH = H / 2, OW = W / 2;
    if (ow >= OW || oh >= OH || nk >= NK) return;

    const float *in_slice = I + (size_t)nk * H * W;
    int ih = oh * 2, iw = ow * 2;
    float m0 = fmaxf(in_slice[ih * W + iw], in_slice[ih * W + iw + 1]);
    float m1 = fmaxf(in_slice[(ih + 1) * W + iw], in_slice[(ih + 1) * W + iw + 1]);
    O[(size_t)nk * OH * OW + oh * OW + ow] = fmaxf(m0, m1);
}

__global__ void fc_linear_kernel(const float *__restrict__ X,
                                 const float *__restrict__ W,
                                 float *__restrict__ Y,
                                 int N, int D_in, int D_out) {
    int out_idx = blockIdx.x * blockDim.x + threadIdx.x;
    int n = blockIdx.y;
    if (out_idx >= D_out || n >= N) return;

    const float *x_row = X + (size_t)n * D_in;
    const float *w_row = W + (size_t)out_idx * D_in;
    float sum = 0.0f;
    for (int i = 0; i < D_in; i++) {
        sum += x_row[i] * w_row[i];
    }
    Y[(size_t)n * D_out + out_idx] = sum;
}

int main(void) {
    run_xor_and_linear_collapse();

    cudaDeviceProp prop;
    CHECK_CUDA(cudaGetDeviceProperties(&prop, 0));
    printf("\n=== 3. GPU Activation Function Cost on %s ===\n", prop.name);

    const int N_ACT = 1 << 22; /* 4,194,304 threads * 256 ops = 1.074G activations */
    float *d_act = NULL;
    CHECK_CUDA(cudaMalloc(&d_act, N_ACT * sizeof(float)));
    cudaEvent_t ev0, ev1;
    CHECK_CUDA(cudaEventCreate(&ev0));
    CHECK_CUDA(cudaEventCreate(&ev1));

    int blk = 256, grd = (N_ACT + blk - 1) / blk;
    bench_act_relu<<<grd, blk>>>(d_act, N_ACT);
    bench_act_sigmoid<<<grd, blk>>>(d_act, N_ACT);
    bench_act_tanh<<<grd, blk>>>(d_act, N_ACT);
    CHECK_CUDA(cudaDeviceSynchronize());

    float ms_relu = 0, ms_sig = 0, ms_tanh = 0;
    CHECK_CUDA(cudaEventRecord(ev0));
    bench_act_relu<<<grd, blk>>>(d_act, N_ACT);
    CHECK_CUDA(cudaEventRecord(ev1));
    CHECK_CUDA(cudaEventSynchronize(ev1));
    CHECK_CUDA(cudaEventElapsedTime(&ms_relu, ev0, ev1));

    CHECK_CUDA(cudaEventRecord(ev0));
    bench_act_sigmoid<<<grd, blk>>>(d_act, N_ACT);
    CHECK_CUDA(cudaEventRecord(ev1));
    CHECK_CUDA(cudaEventSynchronize(ev1));
    CHECK_CUDA(cudaEventElapsedTime(&ms_sig, ev0, ev1));

    CHECK_CUDA(cudaEventRecord(ev0));
    bench_act_tanh<<<grd, blk>>>(d_act, N_ACT);
    CHECK_CUDA(cudaEventRecord(ev1));
    CHECK_CUDA(cudaEventSynchronize(ev1));
    CHECK_CUDA(cudaEventElapsedTime(&ms_tanh, ev0, ev1));

    double total_evals = (double)N_ACT * 256.0;
    printf("  ReLU    (max(0,x))              : %.3f ms  (%7.2f Gevals/s, 1.00x)\n",
           ms_relu, total_evals / (ms_relu * 1e6));
    printf("  Sigmoid (1 / (1 + expf(-x)))    : %.3f ms  (%7.2f Gevals/s, %.2fx slower)\n",
           ms_sig, total_evals / (ms_sig * 1e6), ms_sig / ms_relu);
    printf("  tanh    ((e^x-e^-x)/(e^x+e^-x)) : %.3f ms  (%7.2f Gevals/s, %.2fx slower)\n",
           ms_tanh, total_evals / (ms_tanh * 1e6), ms_tanh / ms_relu);
    CHECK_CUDA(cudaFree(d_act));

    /* Part 4: VGG-like CNN stage profile (N=8):
     * Stage 1: CONV1 (64->128, 56x56, 3x3) -> ReLU -> MaxPool2x2 (28x28)
     * Stage 2: CONV2 (128->256, 28x28, 3x3) -> ReLU -> MaxPool2x2 (14x14) -> MaxPool2x2 (7x7)
     * Stage 3: FC    (256*7*7 = 12544 -> 1024)
     */
    printf("\n=== 4. End-to-End CNN Layer Profile (Parameters vs FLOPs vs GPU Time) ===\n");
    const int B = 8;
    const size_t p_conv1 = 64ULL * 128 * 3 * 3;     /* 73,728 */
    const size_t p_conv2 = 128ULL * 256 * 3 * 3;    /* 294,912 */
    const size_t p_fc    = 12544ULL * 1024;         /* 12,845,056 */
    const size_t p_tot   = p_conv1 + p_conv2 + p_fc;

    const double f_conv1 = 2.0 * B * 128 * 56 * 56 * 64 * 3 * 3;
    const double f_conv2 = 2.0 * B * 256 * 28 * 28 * 128 * 3 * 3;
    const double f_fc    = 2.0 * B * 12544 * 1024;
    const double f_tot   = f_conv1 + f_conv2 + f_fc;

    float *d_x0, *d_w1, *d_y1, *d_p1, *d_w2, *d_y2, *d_p2, *d_p3, *d_wfc, *d_yfc;
    CHECK_CUDA(cudaMalloc(&d_x0,  B * 64 * 56 * 56 * sizeof(float)));
    CHECK_CUDA(cudaMalloc(&d_w1,  p_conv1 * sizeof(float)));
    CHECK_CUDA(cudaMalloc(&d_y1,  B * 128 * 56 * 56 * sizeof(float)));
    CHECK_CUDA(cudaMalloc(&d_p1,  B * 128 * 28 * 28 * sizeof(float)));
    CHECK_CUDA(cudaMalloc(&d_w2,  p_conv2 * sizeof(float)));
    CHECK_CUDA(cudaMalloc(&d_y2,  B * 256 * 28 * 28 * sizeof(float)));
    CHECK_CUDA(cudaMalloc(&d_p2,  B * 256 * 14 * 14 * sizeof(float)));
    CHECK_CUDA(cudaMalloc(&d_p3,  B * 256 * 7 * 7 * sizeof(float)));
    CHECK_CUDA(cudaMalloc(&d_wfc, p_fc * sizeof(float)));
    CHECK_CUDA(cudaMalloc(&d_yfc, B * 1024 * sizeof(float)));

    CHECK_CUDA(cudaMemset(d_x0,  1, B * 64 * 56 * 56 * sizeof(float)));
    CHECK_CUDA(cudaMemset(d_w1,  1, p_conv1 * sizeof(float)));
    CHECK_CUDA(cudaMemset(d_w2,  1, p_conv2 * sizeof(float)));
    CHECK_CUDA(cudaMemset(d_wfc, 1, p_fc * sizeof(float)));

    dim3 b2d(16, 16);
    /* Warmup */
    conv2d_eq14_6_kernel<<<dim3(4, 4, B * 128), b2d>>>(d_x0, d_w1, d_y1, B, 64, 56, 56, 128, 3, 3, 1);
    fc_linear_kernel<<<dim3(4, B), 256>>>(d_p3, d_wfc, d_yfc, B, 12544, 1024);
    CHECK_CUDA(cudaDeviceSynchronize());

    float t_conv = 0, t_relu = 0, t_pool = 0, t_fc = 0, tmp = 0;
    const int REPS = 20;
    for (int r = 0; r < REPS; r++) {
        CHECK_CUDA(cudaEventRecord(ev0));
        conv2d_eq14_6_kernel<<<dim3((56 + 15) / 16, (56 + 15) / 16, B * 128), b2d>>>(
            d_x0, d_w1, d_y1, B, 64, 56, 56, 128, 3, 3, 1);
        CHECK_CUDA(cudaEventRecord(ev1));
        CHECK_CUDA(cudaEventSynchronize(ev1));
        CHECK_CUDA(cudaEventElapsedTime(&tmp, ev0, ev1));
        t_conv += tmp;

        size_t n1 = (size_t)B * 128 * 56 * 56;
        CHECK_CUDA(cudaEventRecord(ev0));
        relu_kernel<<<(n1 + 255) / 256, 256>>>(d_y1, n1);
        CHECK_CUDA(cudaEventRecord(ev1));
        CHECK_CUDA(cudaEventSynchronize(ev1));
        CHECK_CUDA(cudaEventElapsedTime(&tmp, ev0, ev1));
        t_relu += tmp;

        CHECK_CUDA(cudaEventRecord(ev0));
        maxpool2x2_kernel<<<dim3((28 + 15) / 16, (28 + 15) / 16, B * 128), b2d>>>(
            d_y1, d_p1, B * 128, 56, 56);
        CHECK_CUDA(cudaEventRecord(ev1));
        CHECK_CUDA(cudaEventSynchronize(ev1));
        CHECK_CUDA(cudaEventElapsedTime(&tmp, ev0, ev1));
        t_pool += tmp;

        CHECK_CUDA(cudaEventRecord(ev0));
        conv2d_eq14_6_kernel<<<dim3((28 + 15) / 16, (28 + 15) / 16, B * 256), b2d>>>(
            d_p1, d_w2, d_y2, B, 128, 28, 28, 256, 3, 3, 1);
        CHECK_CUDA(cudaEventRecord(ev1));
        CHECK_CUDA(cudaEventSynchronize(ev1));
        CHECK_CUDA(cudaEventElapsedTime(&tmp, ev0, ev1));
        t_conv += tmp;

        size_t n2 = (size_t)B * 256 * 28 * 28;
        CHECK_CUDA(cudaEventRecord(ev0));
        relu_kernel<<<(n2 + 255) / 256, 256>>>(d_y2, n2);
        CHECK_CUDA(cudaEventRecord(ev1));
        CHECK_CUDA(cudaEventSynchronize(ev1));
        CHECK_CUDA(cudaEventElapsedTime(&tmp, ev0, ev1));
        t_relu += tmp;

        CHECK_CUDA(cudaEventRecord(ev0));
        maxpool2x2_kernel<<<dim3((14 + 15) / 16, (14 + 15) / 16, B * 256), b2d>>>(
            d_y2, d_p2, B * 256, 28, 28);
        maxpool2x2_kernel<<<dim3((7 + 15) / 16, (7 + 15) / 16, B * 256), b2d>>>(
            d_p2, d_p3, B * 256, 14, 14);
        CHECK_CUDA(cudaEventRecord(ev1));
        CHECK_CUDA(cudaEventSynchronize(ev1));
        CHECK_CUDA(cudaEventElapsedTime(&tmp, ev0, ev1));
        t_pool += tmp;

        CHECK_CUDA(cudaEventRecord(ev0));
        fc_linear_kernel<<<dim3((1024 + 255) / 256, B), 256>>>(
            d_p3, d_wfc, d_yfc, B, 12544, 1024);
        CHECK_CUDA(cudaEventRecord(ev1));
        CHECK_CUDA(cudaEventSynchronize(ev1));
        CHECK_CUDA(cudaEventElapsedTime(&tmp, ev0, ev1));
        t_fc += tmp;
    }
    t_conv /= REPS; t_relu /= REPS; t_pool /= REPS; t_fc /= REPS;
    float t_tot = t_conv + t_relu + t_pool + t_fc;

    printf("  Layer Type | Weights (FP32 MiB) | Weight %% | GFLOPs (B=%d) | FLOP %% | GPU Time (ms) | Time %%\n", B);
    printf("  -----------+--------------------+----------+--------------+--------+---------------+-------\n");
    printf("  CONV2D x2  | %9zu (%5.2f)  |  %5.2f%%  |    %7.3f   | %5.2f%% |     %7.3f   | %5.2f%%\n",
           p_conv1 + p_conv2, (p_conv1 + p_conv2) * 4.0 / (1024 * 1024),
           100.0 * (p_conv1 + p_conv2) / p_tot,
           (f_conv1 + f_conv2) / 1e9, 100.0 * (f_conv1 + f_conv2) / f_tot,
           t_conv, 100.0 * t_conv / t_tot);
    printf("  ReLU x2    |         0 ( 0.00)  |   0.00%%  |      0.000   |  0.00%% |     %7.3f   | %5.2f%%\n",
           t_relu, 100.0 * t_relu / t_tot);
    printf("  MaxPool x3 |         0 ( 0.00)  |   0.00%%  |      0.000   |  0.00%% |     %7.3f   | %5.2f%%\n",
           t_pool, 100.0 * t_pool / t_tot);
    printf("  FC (end)   | %9zu (%5.2f)  |  %5.2f%%  |    %7.3f   | %5.2f%% |     %7.3f   | %5.2f%%\n",
           p_fc, p_fc * 4.0 / (1024 * 1024), 100.0 * p_fc / p_tot,
           f_fc / 1e9, 100.0 * f_fc / f_tot,
           t_fc, 100.0 * t_fc / t_tot);

    cudaFree(d_x0); cudaFree(d_w1); cudaFree(d_y1); cudaFree(d_p1);
    cudaFree(d_w2); cudaFree(d_y2); cudaFree(d_p2); cudaFree(d_p3);
    cudaFree(d_wfc); cudaFree(d_yfc);
    return 0;
}
