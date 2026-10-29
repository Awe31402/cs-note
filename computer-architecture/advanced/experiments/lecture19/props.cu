// props.cu：印出本機 GPU 的一些限制（block 最多幾個 thread、有幾個 SM、warp 多大…）
#include <cstdio>
int main()
{
	cudaDeviceProp p; cudaGetDeviceProperties(&p, 0);
	int clk, memclk; cudaDeviceGetAttribute(&clk, cudaDevAttrClockRate, 0); cudaDeviceGetAttribute(&memclk, cudaDevAttrMemoryClockRate, 0);
	printf("%s，compute capability %d.%d\n", p.name, p.major, p.minor);
	printf("SM 數：%d；每個 SM 最多 %d 個 thread；warp 大小：%d\n", p.multiProcessorCount, p.maxThreadsPerMultiProcessor, p.warpSize);
	printf("每個 block 最多 %d 個 thread；block 的維度上限 (%d, %d, %d)；grid 的維度上限 (%d, %d, %d)\n",
	       p.maxThreadsPerBlock, p.maxThreadsDim[0], p.maxThreadsDim[1], p.maxThreadsDim[2], p.maxGridSize[0], p.maxGridSize[1], p.maxGridSize[2]);
	printf("每個 block 的 shared memory：%zu KiB；每個 SM 的暫存器：%d 個（32-bit）；每個 block 最多 %d 個暫存器\n",
	       p.sharedMemPerBlock / 1024, p.regsPerMultiprocessor, p.regsPerBlock);
	printf("L2：%d KiB；顯示記憶體：%zu MiB，bus %d bit；GPU 時脈 %d MHz；copy engine 數：%d\n",
	       p.l2CacheSize / 1024, p.totalGlobalMem >> 20, p.memoryBusWidth, clk / 1000, p.asyncEngineCount);
	return 0;
}
// 另外：dim3 沒給的維度是多少？（影片 [19:53] 說預設 0）
struct Dim3Check { Dim3Check() { dim3 d(16, 16); printf("dim3 d(16, 16)：d.x = %u、d.y = %u、d.z = %u\n", d.x, d.y, d.z); } } dim3check;
