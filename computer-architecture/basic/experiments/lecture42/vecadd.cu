#include <stdio.h>
#include <stdlib.h>
#include <cuda_runtime.h>
/* 影片 [27:14] 的 CUDA 向量加法：每個 GPU thread 加一對元素。
 * 另外印出這張 GPU 的 SM 數、warp 大小，和影片的 Tesla（GeForce 8800：16 個 SM、warp 32）對照 */
#define N (1 << 24)
__global__ void vectorAdd(int *gpu_a, int *gpu_b, int *gpu_c)
{
	int idx = threadIdx.x + blockIdx.x * blockDim.x;      /* 這個 thread 負責第幾個元素 */
	gpu_c[idx] = gpu_a[idx] + gpu_b[idx];
}
int main(void)
{
	cudaDeviceProp p; cudaGetDeviceProperties(&p, 0);
	printf("%s: %d SMs, warp size %d, max %d threads per block\n", p.name, p.multiProcessorCount, p.warpSize, p.maxThreadsPerBlock);
	size_t size = N * sizeof(int);
	int *a = (int *)malloc(size), *b = (int *)malloc(size), *c = (int *)malloc(size);
	int *gpu_a, *gpu_b, *gpu_c;
	cudaMalloc((void **)&gpu_a, size); cudaMalloc((void **)&gpu_b, size); cudaMalloc((void **)&gpu_c, size);
	for (int i = 0; i < N; i++) { a[i] = i; b[i] = 2 * i; }
	cudaEvent_t e0, e1, e2, e3; cudaEventCreate(&e0); cudaEventCreate(&e1); cudaEventCreate(&e2); cudaEventCreate(&e3);
	cudaEventRecord(e0);
	cudaMemcpy(gpu_a, a, size, cudaMemcpyHostToDevice);
	cudaMemcpy(gpu_b, b, size, cudaMemcpyHostToDevice);
	cudaEventRecord(e1);
	vectorAdd<<<N / 32, 32>>>(gpu_a, gpu_b, gpu_c);           /* 影片的寫法：N/32 個 block，每個 block 32 個 thread */
	cudaEventRecord(e2);
	cudaMemcpy(c, gpu_c, size, cudaMemcpyDeviceToHost);
	cudaEventRecord(e3); cudaEventSynchronize(e3);
	float t01, t12, t23; cudaEventElapsedTime(&t01, e0, e1); cudaEventElapsedTime(&t12, e1, e2); cudaEventElapsedTime(&t23, e2, e3);
	int bad = 0; for (int i = 0; i < N; i++) bad += c[i] != 3 * i;
	printf("N = %d: copy to GPU %.2f ms | kernel %.3f ms | copy back %.2f ms | wrong results: %d\n", N, t01, t12, t23, bad);
	for (int tpb = 32; tpb <= 1024; tpb *= 4) {               /* block 大小對速度的影響 */
		cudaEventRecord(e1); vectorAdd<<<N / tpb, tpb>>>(gpu_a, gpu_b, gpu_c); cudaEventRecord(e2); cudaEventSynchronize(e2);
		cudaEventElapsedTime(&t12, e1, e2); printf("  %4d threads per block: kernel %.3f ms\n", tpb, t12);
	}
	cudaFree(gpu_a); cudaFree(gpu_b); cudaFree(gpu_c);
	return 0;
}
