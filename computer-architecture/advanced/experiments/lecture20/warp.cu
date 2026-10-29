// warp.cu：第 20 支的四個實驗（RTX 4060 Laptop，sm_89）
//  1. 每個 SM 每個 cycle 能做幾個 FP32 FMA（用 clock64() 在 GPU 上直接數 cycle）
//  2. warp divergence：if/else 照「thread 編號的奇偶」分（同一個 warp 裡分岔）對上照「warp 編號的奇偶」分（不分岔）
//  3. 影片 [31:58] 的例子：同一個 warp 裡 thread 0、1 在 while 迴圈等 x 變 1，thread 2、3 負責把 x 設成 1
//  4. shared memory 的 bank conflict：每個 thread 讀 s[t * stride]
#include <cstdio>
__global__ void fma(float *out, long long *cyc, int iters)
{
	float a = threadIdx.x, b = a + 1, c = a + 2, d = a + 3, e = a + 4, f = a + 5, g = a + 6, h = a + 7;
	long long t0 = clock64();
	for (int i = 0; i < iters; i++) {               /* 8 條互不相干的 FMA 鏈 */
		a = a * 0.999f + 1.0f; b = b * 0.999f + 1.0f; c = c * 0.999f + 1.0f; d = d * 0.999f + 1.0f;
		e = e * 0.999f + 1.0f; f = f * 0.999f + 1.0f; g = g * 0.999f + 1.0f; h = h * 0.999f + 1.0f;
	}
	long long t1 = clock64();
	out[blockIdx.x * blockDim.x + threadIdx.x] = a + b + c + d + e + f + g + h;
	if (threadIdx.x == 0) cyc[blockIdx.x] = t1 - t0;
}
__device__ float heavy(float v, int n) { for (int k = 0; k < n; k++) v = v * 0.999f + 0.5f; return v; }
__global__ void branchy(float *x, int byWarp)
{
	int t = blockIdx.x * blockDim.x + threadIdx.x;
	int sel = byWarp ? (t / 32) & 1 : t & 1;
	float v = x[t];
	if (sel) v = heavy(v, 20000); else v = heavy(v + 1, 20000) * 2;   /* 兩邊工作量一樣 */
	x[t] = v;
}
__global__ void spin(int *res)
{
	__shared__ volatile int x;
	if (threadIdx.x == 0) x = 0;
	__syncwarp();
	long long t0 = clock64(); int ok = 1;
	if (threadIdx.x < 2) {
		while (x == 0) if (clock64() - t0 > 2000000000LL) { ok = 0; break; }  /* 最多等約 1 秒 */
	} else {
		x = 1;
	}
	__syncwarp();
	if (threadIdx.x == 0) res[0] = ok;
}
__global__ void bank(float *out, long long *cyc, int stride)
{
	__shared__ float s[32 * 33];
	volatile float *vs = s;                        /* volatile：每次都真的去讀 shared memory */
	int t = threadIdx.x;
	for (int i = t; i < 32 * 33; i += 32) s[i] = i;
	__syncwarp();
	int idx = (t * stride) % (32 * 33);            /* 第 t 個 thread 讀第 t*stride 個 word */
	float v0 = 0, v1 = 0, v2 = 0, v3 = 0, v4 = 0, v5 = 0, v6 = 0, v7 = 0;
	long long t0 = clock64();
	for (int k = 0; k < 1000; k++) {               /* 每圈 8 次互不相干的讀取 */
		v0 += vs[idx]; v1 += vs[idx]; v2 += vs[idx]; v3 += vs[idx];
		v4 += vs[idx]; v5 += vs[idx]; v6 += vs[idx]; v7 += vs[idx];
	}
	long long t1 = clock64();
	out[t] = v0 + v1 + v2 + v3 + v4 + v5 + v6 + v7;
	if (t == 0) cyc[0] = t1 - t0;
}
int main()
{
	cudaDeviceProp p; cudaGetDeviceProperties(&p, 0);
	int nsm = p.multiProcessorCount;
	float *out; long long *cyc, hc[1024];
	cudaMalloc(&out, 1 << 26); cudaMalloc(&cyc, 8192);
	/* 1：每個 SM 一個 block、1024 個 thread，FMA 都在暫存器裡 */
	int iters = 100000;
	fma<<<nsm, 1024>>>(out, cyc, iters); cudaDeviceSynchronize();
	cudaMemcpy(hc, cyc, nsm * 8, cudaMemcpyDeviceToHost);
	double fpc = 1024.0 * 8 * iters / hc[0];
	printf("1. 每個 SM 每個 cycle 做了 %.1f 個 FP32 FMA（%d 個 SM → 整顆每 cycle %.0f 個）\n", fpc, nsm, fpc * nsm);
	/* 2 */
	cudaEvent_t a, b; cudaEventCreate(&a); cudaEventCreate(&b); float tm[2];
	for (int r = 0; r < 2; r++) for (int w = 0; w < 2; w++) {
		cudaEventRecord(a); branchy<<<nsm * 8, 256>>>(out, w); cudaEventRecord(b); cudaEventSynchronize(b);
		cudaEventElapsedTime(&tm[w], a, b);
		if (r) printf("2. if/else 照%s分：%.3f ms\n", w ? "「warp 編號」（同一個 warp 走同一邊）" : "「thread 編號」（同一個 warp 兩邊都要走）", tm[w]);
	}
	/* 3 */
	int *res, hr; cudaMalloc(&res, 4);
	spin<<<1, 4>>>(res); cudaMemcpy(&hr, res, 4, cudaMemcpyDeviceToHost);
	printf("3. thread 0、1 等 x 變 1，thread 2、3 設 x = 1（同一個 warp）：%s\n", hr ? "順利結束，沒有卡住" : "卡住了（等了 1 秒放棄）");
	/* 4 */
	for (int st : {1, 2, 4, 8, 16, 32, 33}) {
		bank<<<1, 32>>>(out, cyc, st); cudaMemcpy(hc, cyc, 8, cudaMemcpyDeviceToHost);
		printf("4. stride %2d：每次 shared memory 讀取平均 %.1f cycle\n", st, hc[0] / 8000.0);
	}
	return 0;
}
