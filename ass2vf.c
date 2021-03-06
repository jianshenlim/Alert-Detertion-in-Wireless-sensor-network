#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h> 
#include <unistd.h> 
#include <errno.h> 
#include <netdb.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <ifaddrs.h>
#include <netpacket/packet.h>
#include "teamutil.h"


int keepRunning = 1;
int sentinelValue = 1;

int main(int argc, char** argv)
{

	//Create a mutex
	pthread_mutex_init(&gMutex, NULL);
	int nrows, ncols, mode, size;

    // Initialize all variables
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	struct NodeReport report;
	MPI_Datatype type[13] = { MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_DOUBLE, MPI_LONG, MPI_INT, MPI_INT, MPI_INT, MPI_INT,MPI_INT,MPI_INT };
	int blocklen[13] = { 1, 1, 1, 1, 1, 1, 1, 4, 4, 4, 4, 4, 16 };
	MPI_Aint disp[13];

	MPI_Get_address(&report.reading, &disp[0]);
	MPI_Get_address(&report.matches, &disp[1]);
	MPI_Get_address(&report.neighborsCompared, &disp[2]);
	MPI_Get_address(&report.xCoor, &disp[3]);
	MPI_Get_address(&report.yCoor, &disp[4]);
	MPI_Get_address(&report.MPI_time, &disp[5]);
	MPI_Get_address(&report.time, &disp[6]);
	MPI_Get_address(&report.adjacentNodesX[0], &disp[7]);
	MPI_Get_address(&report.adjacentNodesY[0], &disp[8]);
	MPI_Get_address(&report.adjacentNodesRank[0], &disp[9]);
	MPI_Get_address(&report.adjacentNodesTemp[0], &disp[10]);
	MPI_Get_address(&report.IPaddress[0], &disp[11]);
	MPI_Get_address(&report.neighIPaddress[0], &disp[12]);
	//Make relative
	disp[12] = disp[12] - disp[0];
	disp[11] = disp[11] - disp[0];
	disp[10] = disp[10] - disp[0];
	disp[9] = disp[9] - disp[0];
	disp[8] = disp[8] - disp[0];
	disp[7] = disp[7] - disp[0];
	disp[6] = disp[6] - disp[0];
	disp[5] = disp[5] - disp[0];
	disp[4] = disp[4] - disp[0];
	disp[3] = disp[3] - disp[0];
	disp[2] = disp[2] - disp[0];
	disp[1] = disp[1] - disp[0];
	disp[0] = 0;

	// Create MPI struct
	MPI_Type_create_struct(13, blocklen, disp, type, &Reporttype);
	MPI_Type_commit(&Reporttype);
	MPI_Status startStatus;

	int rank;
	int iterations;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	// If arguements provided, initialize row and column val;ues
	if (argc == 3) {
		mode = 1;
		nrows = atoi(argv[1]);
		ncols = atoi(argv[2]);
		if ((nrows * ncols) != size - 1) {
			if (rank == 0) printf("ERROR: nrows*ncols)=%d * %d = %d != %d\n", nrows, ncols, nrows * ncols, size);
			MPI_Finalize();
			return 0;
		}
	}
	// Else, initialize a square grid of size sqrt(Number of processes -1)
	else {
		mode = 0;
		nrows = ncols = (int)sqrt(size - 1);
	}

	// Initialize number of iterations to run

	iterations = ITERATIONS;

	if (rank == 0) {
		fflush(stdout);
		printf("Enter number of iterations: ");
		fflush(stdout);
		scanf("%d", &iterations);

	}

	MPI_Bcast(&iterations, 1, MPI_INT, 0, MPI_COMM_WORLD);

	double initial = MPI_Wtime();

	// Initialize arrays for time
	arrayOfGeneratedReading = (int*)malloc(sizeof(int) * ncols * nrows);
	arrayOfGeneratedReadingTime = (time_t*)malloc(sizeof(time_t) * ncols * nrows);

	// Split comm world into master and slave components
	MPI_Comm new_comm;
	MPI_Comm_split(MPI_COMM_WORLD, rank == size - 1, 0, &new_comm);
	if (rank == size - 1)	master_io(MPI_COMM_WORLD, new_comm, nrows, ncols, iterations);
	else slave_io(MPI_COMM_WORLD, new_comm, size - 1, nrows, ncols, mode);

	MPI_Type_free(&Reporttype);
	MPI_Finalize();
	return 0;
}

