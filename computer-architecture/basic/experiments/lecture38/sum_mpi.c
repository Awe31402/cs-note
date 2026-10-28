#include <stdio.h>
#include <mpi.h>
/* 課本 Example 150 的 message passing 加總，改成真正的 MPI：
 * 每個 process 有自己的 numbers（不共用記憶體），算完 partial sum 用 MPI_Send 傳給 rank 0 */
#define SIZE (1 << 26)
static int numbers[SIZE];
int main(int argc, char **argv)
{
	int myId, N;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &myId);              /* getMyProcessId() */
	MPI_Comm_size(MPI_COMM_WORLD, &N);
	for (int i = 0; i < SIZE; i++) numbers[i] = i & 7;  /* 每個 process 各自一份 */
	double t0 = MPI_Wtime();
	int startIdx = myId * (SIZE / N), endIdx = startIdx + SIZE / N;
	long partialSum = 0;
	for (int jdx = startIdx; jdx < endIdx; jdx++) partialSum += numbers[jdx];
	if (myId != 0) {
		MPI_Send(&partialSum, 1, MPI_LONG, 0, 0, MPI_COMM_WORLD);          /* send(0, partialSum) */
	} else {
		long sum = partialSum, v;
		for (int pid = 1; pid < N; pid++) {
			MPI_Recv(&v, 1, MPI_LONG, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);  /* receive(ANYSOURCE) */
			sum += v;
		}
		printf("N=%d processes: sum=%ld  %.1f ms\n", N, sum, (MPI_Wtime() - t0) * 1e3);
	}
	MPI_Finalize();
	return 0;
}
