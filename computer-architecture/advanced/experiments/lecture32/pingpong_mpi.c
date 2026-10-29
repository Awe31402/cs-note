// pingpong_mpi.c：兩個 MPI process 互傳一個 double，量單程要多久（跟第 27 支的 shared memory 換手比）
#include <mpi.h>
#include <stdio.h>
int main(int argc, char **argv)
{
	MPI_Init(&argc, &argv);
	int id; MPI_Comm_rank(MPI_COMM_WORLD, &id);
	double v = 1; const int R = 100000;
	MPI_Barrier(MPI_COMM_WORLD);
	double t0 = MPI_Wtime();
	for (int i = 0; i < R; i++) {
		if (id == 0) { MPI_Send(&v, 1, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD); MPI_Recv(&v, 1, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); }
		else { MPI_Recv(&v, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); MPI_Send(&v, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD); }
	}
	if (id == 0) printf("MPI 單程：%.0f ns\n", (MPI_Wtime() - t0) / R / 2 * 1e9);
	MPI_Finalize();
	return 0;
}
