// Measure GPU power, energy E, delay D, and ED^2 vs SM activity (beta) and bit transitions
// Build: nvcc -O3 -arch=sm_89 gpu_power.cu -L/usr/lib/x86_64-linux-gnu -l:libnvidia-ml.so.1 -o /tmp/gpu_power
#include <cstdio>
#include <cstdlib>
#include <time.h>
#include <unistd.h>
#include <cuda_runtime.h>
#include <nvml.h>

__global__ void fma_burn(float *out, float init_a, float init_b, long long iters) {
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    float a = init_a * (1.0f + (tid & 31) * 1e-4f);
    float b = init_b;
    float c0 = a, c1 = -a, c2 = a * 0.5f, c3 = -a * 0.5f;
    for (long long i = 0; i < iters; i++) {
        c0 = __fmaf_rn(c0, b, init_a);
        c1 = __fmaf_rn(c1, b, -init_a);
        c2 = __fmaf_rn(c2, b, init_a);
        c3 = __fmaf_rn(c3, b, -init_a);
        b = -b;
    }
    if ((c0 + c1 + c2 + c3) == 123456.0f) out[tid] = c0;
}

static double now_sec() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static void run_and_measure(const char *label, nvmlDevice_t dev, cudaStream_t st,
                            float *d_out, int blocks, float a, float b, long long iters) {
    double t0 = now_sec();
    fma_burn<<<blocks, 256, 0, st>>>(d_out, a, b, iters);

    double sum_w = 0;
    unsigned long sum_mhz = 0;
    unsigned n = 0, temp = 0;
    while (cudaStreamQuery(st) == cudaErrorNotReady) {
        usleep(40000);
        unsigned mw = 0, mhz = 0;
        nvmlDeviceGetPowerUsage(dev, &mw);
        nvmlDeviceGetClockInfo(dev, NVML_CLOCK_SM, &mhz);
        nvmlDeviceGetTemperature(dev, NVML_TEMPERATURE_GPU, &temp);
        if (now_sec() - t0 > 0.35) { // skip initial sensor lag
            sum_w += mw / 1000.0;
            sum_mhz += mhz;
            n++;
        }
    }
    cudaStreamSynchronize(st);
    double d = now_sec() - t0;
    double p = n ? sum_w / n : 0.0;
    unsigned mhz = n ? (unsigned)(sum_mhz / n) : 0;
    double e = p * d;
    printf("%-24s | %6.2f W | %4u MHz | %5.2f s | %6.1f J | %6.1f | %2u C\n",
           label, p, mhz, d, e, e * d * d, temp);
}

int main() {
    nvmlInit();
    nvmlDevice_t dev;
    nvmlDeviceGetHandleByIndex(0, &dev);

    float *d_out;
    cudaMalloc(&d_out, 96 * 256 * sizeof(float));
    fma_burn<<<1, 256>>>(d_out, 0.0f, 0.0f, 1000);
    cudaDeviceSynchronize();
    usleep(800000);

    unsigned mw = 0, mhz = 0, temp = 0;
    nvmlDeviceGetPowerUsage(dev, &mw);
    nvmlDeviceGetClockInfo(dev, NVML_CLOCK_SM, &mhz);
    nvmlDeviceGetTemperature(dev, NVML_TEMPERATURE_GPU, &temp);
    printf("Idle (CUDA P0 context, 0 blocks): %.2f W | SM %u MHz | %u C\n\n",
           mw / 1000.0, mhz, temp);

    printf("%-24s | %8s | %8s | %7s | %8s | %6s | %4s\n",
           "Fixed-work (29.5 TFLOP)", "Power P", "SM Clock", "Delay D", "Energy E", "ED^2", "Temp");
    printf("---------------------------------------------------------------------------------\n");

    cudaStream_t st;
    cudaStreamCreate(&st);
    const long long total_blk_iters = 48LL * 300000000LL; // constant total FLOPs across runs

    int blk_list[] = {6, 12, 24, 48};
    for (int b : blk_list) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Toggle %2d blk (%2d/24 SM)", b, b > 24 ? 24 : b);
        run_and_measure(buf, dev, st, d_out, b, 0.9999f, -1.00001f, total_blk_iters / b);
    }
    run_and_measure("Zero   48 blk (24/24 SM)", dev, st, d_out, 48, 0.0f, 0.0f, total_blk_iters / 48);

    cudaFree(d_out);
    nvmlShutdown();
    return 0;
}
