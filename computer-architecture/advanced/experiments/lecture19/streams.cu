// streams.cu：影片 [40:00] 的 streams：把資料切成 S 份，每份「複製進去 → kernel → 複製出來」放在自己的 stream，
//   不同 stream 的複製和計算可以重疊。比 S = 1、2、4、8 的總時間。
//   host 記憶體用 cudaMallocHost（pinned），cudaMemcpyAsync 才能真的非同步
#include <cstdio>
__global__ void work(float *x, int n)                    /* 每個元素做一點運算，讓 kernel 時間跟複製差不多 */
{
	int i = blockIdx.x * blockDim.x + threadIdx.x;
	if (i < n) { float v = x[i]; for (int k = 0; k < 200; k++) v = v * 0.999f + 0.5f; x[i] = v; }
}
int main()
{
	const int N = 1 << 25; size_t size = N * sizeof(float);   /* 128 MiB */
	float *h, *d;
	cudaMallocHost(&h, size); cudaMalloc(&d, size);
	for (int i = 0; i < N; i++) h[i] = i % 100;
	cudaStream_t st[8];
	for (int k = 0; k < 8; k++) cudaStreamCreate(&st[k]);
	cudaEvent_t a, b; cudaEventCreate(&a); cudaEventCreate(&b);
	for (int S : {1, 1, 2, 4, 8}) {                          /* 第一個 S = 1 是暖身 */
		int n = N / S; size_t sz = n * sizeof(float);
		cudaEventRecord(a);
		for (int i = 0; i < S; i++) {
			cudaMemcpyAsync(d + i * n, h + i * n, sz, cudaMemcpyHostToDevice, st[i]);
			work<<<(n + 255) / 256, 256, 0, st[i]>>>(d + i * n, n);
			cudaMemcpyAsync(h + i * n, d + i * n, sz, cudaMemcpyDeviceToHost, st[i]);
		}
		cudaEventRecord(b); cudaEventSynchronize(b);
		float t; cudaEventElapsedTime(&t, a, b);
		printf("stream 數 %d：%.2f ms\n", S, t);
		for (int i = 0; i < N; i++) h[i] = i % 100;
	}
	/* 各部分單獨量一次 */
	float t1, t2, t3;
	cudaEventRecord(a); cudaMemcpy(d, h, size, cudaMemcpyHostToDevice); cudaEventRecord(b); cudaEventSynchronize(b); cudaEventElapsedTime(&t1, a, b);
	cudaEventRecord(a); work<<<(N + 255) / 256, 256>>>(d, N); cudaEventRecord(b); cudaEventSynchronize(b); cudaEventElapsedTime(&t2, a, b);
	cudaEventRecord(a); cudaMemcpy(h, d, size, cudaMemcpyDeviceToHost); cudaEventRecord(b); cudaEventSynchronize(b); cudaEventElapsedTime(&t3, a, b);
	printf("單獨：複製進去 %.2f ms、kernel %.2f ms、複製出來 %.2f ms（加起來 %.2f ms）\n", t1, t2, t3, t1 + t2 + t3);
	return 0;
}
