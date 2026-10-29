// vecadd.cu：影片 [35:04] 的完整流程：CPU malloc → cudaMalloc → 複製到 GPU → kernel → 複製回來 → cudaFree
//   用 cudaEvent 量每一步要多久；另外跑影片 [20:23] 的 2D grid（16 × 16 的 thread block）矩陣加法
#include <cstdio>
#include <cstdlib>
__global__ void vecAdd(float *a, float *b, float *c, int n)
{
	int idx = blockIdx.x * blockDim.x + threadIdx.x;
	if (idx < n) c[idx] = a[idx] + b[idx];
}
__global__ void matAdd(float *a, float *b, float *c, int n)
{
	int idx = blockIdx.x * blockDim.x + threadIdx.x;   /* 影片 [25:02] 的算法 */
	int jdx = blockIdx.y * blockDim.y + threadIdx.y;
	if (idx < n && jdx < n) c[jdx * n + idx] = a[jdx * n + idx] + b[jdx * n + idx];
}
int main()
{
	const int N = 1 << 24;                          /* 1600 萬個 float = 64 MiB */
	size_t size = N * sizeof(float);
	float *a = (float *)malloc(size), *b = (float *)malloc(size), *c = (float *)malloc(size);
	for (int i = 0; i < N; i++) { a[i] = i; b[i] = 2 * i; }
	cudaEvent_t e[5];
	for (int k = 0; k < 5; k++) cudaEventCreate(&e[k]);
	float *ga, *gb, *gc;
	cudaFree(0);                                    /* 先把 GPU 叫醒，不算在時間裡 */
	cudaEventRecord(e[0]);
	cudaMalloc(&ga, size); cudaMalloc(&gb, size); cudaMalloc(&gc, size);
	cudaEventRecord(e[1]);
	cudaMemcpy(ga, a, size, cudaMemcpyHostToDevice);
	cudaMemcpy(gb, b, size, cudaMemcpyHostToDevice);
	cudaEventRecord(e[2]);
	vecAdd<<<(N + 255) / 256, 256>>>(ga, gb, gc, N);
	cudaEventRecord(e[3]);
	cudaMemcpy(c, gc, size, cudaMemcpyDeviceToHost);
	cudaEventRecord(e[4]);
	cudaEventSynchronize(e[4]);
	int bad = 0;
	for (int i = 0; i < N; i++) bad += c[i] != 3.0f * i;
	const char *name[4] = { "cudaMalloc × 3", "複製 a、b 到 GPU（128 MiB）", "kernel（1600 萬個加法）", "複製 c 回 CPU（64 MiB）" };
	float t, tot = 0;
	for (int k = 0; k < 4; k++) { cudaEventElapsedTime(&t, e[k], e[k + 1]); tot += t; printf("  %-28s %8.3f ms\n", name[k], t); }
	printf("  合計 %.3f ms；錯的元素：%d\n", tot, bad);
	/* 2D：n × n 矩陣，grid = (n/16) × (n/16)，block = 16 × 16 */
	int n = 4096;
	dim3 threadsPerBlock(16, 16), numBlocks(n / 16, n / 16);
	matAdd<<<numBlocks, threadsPerBlock>>>(ga, gb, gc, n);
	cudaMemcpy(c, gc, size, cudaMemcpyDeviceToHost);
	bad = 0;
	for (int i = 0; i < n * n; i++) bad += c[i] != 3.0f * i;
	printf("matAdd（%d × %d，%d × %d 個 block，每個 16 × 16）錯的元素：%d\n", n, n, n / 16, n / 16, bad);
	cudaFree(ga); cudaFree(gb); cudaFree(gc);
	return 0;
}
