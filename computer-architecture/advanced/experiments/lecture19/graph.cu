// graph.cu：
//   1. __syncthreads() 當 barrier：每個 block 把 256 個數加總（影片 [30:26]）
//   2. CUDA graph（影片 [46:03]）：1000 個很小的 kernel，一個一個 launch 對上「先 capture 成 graph 再一次 launch」
#include <cstdio>
__global__ void blockSum(const float *x, float *out)
{
	__shared__ float s[256];                       /* 每個 block 一份的 shared memory */
	int t = threadIdx.x;
	s[t] = x[blockIdx.x * 256 + t];
	__syncthreads();                               /* barrier：大家都寫好了才往下 */
	for (int k = 128; k > 0; k >>= 1) {
		if (t < k) s[t] += s[t + k];
		__syncthreads();                           /* 每一輪都要等大家 */
	}
	if (t == 0) out[blockIdx.x] = s[0];
}
__global__ void blockSumNoSync(const float *x, float *out)   /* 對照：拿掉 barrier */
{
	__shared__ float s[256];
	int t = threadIdx.x;
	s[t] = x[blockIdx.x * 256 + t];
	for (int k = 128; k > 0; k >>= 1) if (t < k) s[t] += s[t + k];
	if (t == 0) out[blockIdx.x] = s[0];
}
__global__ void tiny(float *x) { x[threadIdx.x] += 1.0f; }
int main()
{
	const int B = 4096; float *x, *o, h[B];
	cudaMalloc(&x, B * 256 * sizeof(float)); cudaMalloc(&o, B * sizeof(float));
	float *hx = new float[B * 256];
	for (int i = 0; i < B * 256; i++) hx[i] = 1.0f;
	cudaMemcpy(x, hx, B * 256 * sizeof(float), cudaMemcpyHostToDevice);
	for (int v = 0; v < 2; v++) {
		if (v == 0) blockSum<<<B, 256>>>(x, o); else blockSumNoSync<<<B, 256>>>(x, o);
		cudaMemcpy(h, o, sizeof h, cudaMemcpyDeviceToHost);
		int bad = 0; for (int i = 0; i < B; i++) bad += h[i] != 256.0f;
		printf("%s：%d 個 block 裡，總和不是 256 的有 %d 個\n", v == 0 ? "有 __syncthreads" : "沒有 __syncthreads", B, bad);
	}
	cudaStream_t s; cudaStreamCreate(&s);
	cudaEvent_t a, b; cudaEventCreate(&a); cudaEventCreate(&b); float t;
	const int K = 1000;
	for (int r = 0; r < 3; r++) {
		cudaEventRecord(a, s);
		for (int k = 0; k < K; k++) tiny<<<1, 32, 0, s>>>(x);
		cudaEventRecord(b, s); cudaEventSynchronize(b); cudaEventElapsedTime(&t, a, b);
		if (r == 2) printf("一個一個 launch %d 個 kernel：%.3f ms（每個 %.2f µs）\n", K, t, t * 1e3 / K);
	}
	cudaGraph_t g; cudaGraphExec_t ge;
	cudaStreamBeginCapture(s, cudaStreamCaptureModeGlobal);     /* stream capture：把 stream 變成 graph */
	for (int k = 0; k < K; k++) tiny<<<1, 32, 0, s>>>(x);
	cudaStreamEndCapture(s, &g);
	cudaGraphInstantiate(&ge, g, 0);
	size_t nn; cudaGraphGetNodes(g, nullptr, &nn);
	for (int r = 0; r < 3; r++) {
		cudaEventRecord(a, s); cudaGraphLaunch(ge, s);
		cudaEventRecord(b, s); cudaEventSynchronize(b); cudaEventElapsedTime(&t, a, b);
		if (r == 2) printf("capture 成 graph（%zu 個 node）再 launch：%.3f ms（每個 %.2f µs）\n", nn, t, t * 1e3 / K);
	}
	return 0;
}
