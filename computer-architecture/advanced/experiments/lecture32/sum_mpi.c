// sum_mpi.c：影片 [24:46] 的 message passing 版本，用真的 MPI。每個 process 算自己那一份的部分和，
//   不是 root 的就 MPI_Send 給 root（process 0），root 用 MPI_Recv(MPI_ANY_SOURCE) 收 n−1 次加起來
#include <mpi.h>
#include <stdio.h>
#define SIZE (1 << 24)
static double f(double x) { for (int k = 0; k < 50; k++) x = x * 0.999 + 0.001; return x; }
int main(int argc, char **argv)
{
	MPI_Init(&argc, &argv);
	int myId, N; MPI_Comm_rank(MPI_COMM_WORLD, &myId); MPI_Comm_size(MPI_COMM_WORLD, &N);
	double t0 = MPI_Wtime(), partialSum = 0;
	long startIdx = (long)myId * SIZE / N, endIdx = startIdx + SIZE / N;
	for (long j = startIdx; j < endIdx; j++) partialSum += f(j % 100);   /* 沒有共用的陣列：各自算自己的 */
	if (myId != 0) MPI_Send(&partialSum, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
	else {
		double sum = partialSum, v;
		for (int i = 0; i < N - 1; i++) { MPI_Recv(&v, 1, MPI_DOUBLE, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); sum += v; }
		printf("%2d processes：%.3f s（sum = %.6e）\n", N, MPI_Wtime() - t0, sum);
	}
	MPI_Finalize();
	return 0;
}
