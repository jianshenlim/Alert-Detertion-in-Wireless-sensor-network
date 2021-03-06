#include "teamutil.h"
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

int slave_io(MPI_Comm master_comm, MPI_Comm comm, int size, int nrows, int ncols, int mode)
{
	struct NodeReport report;

	// Obtain IP address and save it in variable
	char hostbuffer[256];
	char* IPbuffer;
	struct hostent* host_entry;
	int hostname;

	hostname = gethostname(hostbuffer, sizeof(hostbuffer));
	host_entry = gethostbyname(hostbuffer);

	IPbuffer = inet_ntoa(*((struct in_addr*)
		host_entry->h_addr_list[0]));
	sscanf(IPbuffer, "%d.%d.%d.%d", &report.IPaddress[0], &report.IPaddress[1], &report.IPaddress[2], &report.IPaddress[3]); //Read IP address numbers and save into array


	// Initialize virtual topology variables
	int ndims = 2, reorder, my_cart_rank, ierr, randoNum, my_rank;
	int nbr_i_lo, nbr_i_hi;
	int nbr_j_lo, nbr_j_hi;
	MPI_Comm comm2D;
	int dims[ndims], coord[ndims];
	int wrap_around[ndims];

	int recvArray[4] = { -1,-1,-1,-1 };
	int adjacentNodesX[4];
	int adjacentNodesY[4];
	int adjacentNodesRank[4];
	int adjacentNodesTemp[4];

	if (mode == 0) {
		dims[0] = dims[1] = 0;
	}
	else {
		dims[0] = nrows; /* number of rows */
		dims[1] = ncols; /* number of columns */
	}


	/* create cartesian topology for processes*/
	MPI_Comm_rank(comm, &my_rank);
	MPI_Dims_create(size, ndims, dims);
	if (my_rank == 0)
		printf("Root Rank: %d. Comm Size: %d: Grid Dimension = [%d x %d] \n", my_rank, size, dims[0], dims[1]);

	/* create cartesian mapping */
	wrap_around[0] = 0;
	wrap_around[1] = 0; /* periodic shift is .false. */
	reorder = 1;
	ierr = 0;
	ierr = MPI_Cart_create(comm, ndims, dims, wrap_around, reorder, &comm2D);
	if (ierr != 0) printf("ERROR[%d] creating CART\n", ierr);

	/* find my coordinates in the cartesian communicator group */
	MPI_Cart_coords(comm2D, my_rank, ndims, coord); // coordinated is returned into the coord array
	/* use my cartesian coordinates to find my rank in cartesian group*/
	MPI_Cart_rank(comm2D, coord, &my_cart_rank);

	MPI_Request send_request[4];
	MPI_Request receive_request[4];
	MPI_Status send_status[4];
	MPI_Status receive_status[4];

	MPI_Cart_shift(comm2D, SHIFT_ROW, DISP, &nbr_i_lo, &nbr_i_hi);
	MPI_Cart_shift(comm2D, SHIFT_COL, DISP, &nbr_j_lo, &nbr_j_hi);

	MPI_Request IPsend_request[4];
	MPI_Request IPreceive_request[4];
	MPI_Status IPsend_status[4];
	MPI_Status IPreceive_status[4];


	int neighbours[] = { nbr_i_lo, nbr_i_hi, nbr_j_lo, nbr_j_hi };
	int numOfNeighbours;
	for (int i = 0; i < 4; i++) {   // For all the nodes neighbour's
		if (neighbours[i] > -1 && neighbours[i] < ncols * nrows) {  // If there is a neighbour, record neighbour's values
			report.adjacentNodesX[i] = neighbours[i] / ncols;
			report.adjacentNodesY[i] = neighbours[i] % ncols;
			report.adjacentNodesRank[i] = neighbours[i];
			MPI_Send((int*)report.IPaddress, 4, MPI_INT, neighbours[i], 0, comm2D);
			numOfNeighbours++;
		}
		else {                                  // Else record -1, ie no neighbour
			report.adjacentNodesX[i] = -1;
			report.adjacentNodesY[i] = -1;
			report.adjacentNodesRank[i] = -1;
		}
	}
	int ipIndex = 0;
	for (int i = 0; i < 4; i++) {   // For all the nodes neighbour's record IP address of neighbour
		if (neighbours[i] > -1 && neighbours[i] < ncols * nrows) {  // If there is a neighbour, record neighbour's values
			MPI_Recv((int*)report.neighIPaddress + (ipIndex * 4), 4, MPI_INT, neighbours[i], 0, comm2D, &IPreceive_status[i]);
			ipIndex++;
		}
	}


	MPI_Status status;
	double baseTime;
	MPI_Recv(&baseTime, 1, MPI_DOUBLE, size, 0, master_comm, &status); // Receive time of the base station, used to correct time values in nodes when sent back to basestation
	double nodeTime = MPI_Wtime();
	double timediff;

	if (baseTime >= nodeTime) {
		timediff = baseTime - nodeTime;
	}
	else {
		timediff = -(nodeTime - baseTime);
	}

	int run = 1;
	int keepRunning;
	int gotData2;
	MPI_Request runRequest;
	MPI_Status stopStatus;
	MPI_Irecv(&keepRunning, 1, MPI_INT, size, 0, master_comm, &runRequest); // Check to see if termination singnal sent by baystation
	while (run) {
		MPI_Test(&runRequest, &gotData2, &stopStatus);
		if (gotData2) {
			run = 0;                                        // If receive signal, stop running
		}
		if (run == 1)
		{
			unsigned int seed = time(NULL) + my_rank;
			randoNum = rand_r(&seed) % 100 + 1; // Generate random temperature

			//If there is an anomoly in the readings, i.e. the rando > 80, then the node will send a request to its neighbors
			if (randoNum > THRESHOLD) {
				MPI_Isend(&randoNum, 1, MPI_INT, nbr_i_lo, 0, comm2D, &send_request[0]);    // Send query to neighbours
				MPI_Isend(&randoNum, 1, MPI_INT, nbr_i_hi, 0, comm2D, &send_request[1]);
				MPI_Isend(&randoNum, 1, MPI_INT, nbr_j_lo, 0, comm2D, &send_request[2]);
				MPI_Isend(&randoNum, 1, MPI_INT, nbr_j_hi, 0, comm2D, &send_request[3]);

				MPI_Irecv(&recvArray[0], 1, MPI_INT, nbr_i_lo, 0, comm2D, &receive_request[0]); // Wait to receive reply
				MPI_Irecv(&recvArray[1], 1, MPI_INT, nbr_i_hi, 0, comm2D, &receive_request[1]);
				MPI_Irecv(&recvArray[2], 1, MPI_INT, nbr_j_lo, 0, comm2D, &receive_request[2]);
				MPI_Irecv(&recvArray[3], 1, MPI_INT, nbr_j_hi, 0, comm2D, &receive_request[3]);
			}
		}

		int gotData[4];
		MPI_Request listenRequest[4];
		MPI_Status listenStatus[4];
		int request[4];
		for (int j = 0; j < 4; j++) {   // For all possible neighbours
			if (neighbours[j] < 0 || neighbours[j] >= ncols * nrows) {  // If not valid neighbour, continue
				continue;
			}
			int gotData;
			MPI_Request listenRequest;
			MPI_Status listenStatus;
			int request;
			MPI_Irecv(&request, 1, MPI_INT, neighbours[j], 0, comm2D, &listenRequest);  // Wait to receive a query request from neighbour
			time_t start_time = time(NULL); //get start time
			MPI_Test(&listenRequest, &gotData, &listenStatus); //test, have we got it yet
			//loop until we have received, or taken too long
			while (!gotData && difftime(time(NULL), start_time) < 1) {
				//wait a bit.
				MPI_Test(&listenRequest, &gotData, &listenStatus);  //test, have we got it yet; //test again
			}
			//By now we either have received the data, or taken too long, so...
			if (!gotData) {
				//we must have timed out
				MPI_Cancel(&listenRequest);
				MPI_Request_free(&listenRequest);
				//throw an error
			}
			else {
				MPI_Send(&randoNum, 1, MPI_INT, neighbours[j], 0, comm2D);  // If received a request, send number to generated number to neighbour
			}

		}
		if (randoNum > THRESHOLD && run != 0) { // If temperature exceeds threshold and sensor is still running, wait for all replys from neighbours
			MPI_Waitall(4, send_request, send_status);
			MPI_Waitall(4, receive_request, receive_status);
			int matches = 0;

			for (int j = 0; j < 4; j++) {   // For all neighbours, check values to see if within tolerance range
				int neighvallow = randoNum - TOLERANCE;
				int neighvaltop = randoNum + TOLERANCE;
				if (recvArray[j] >= neighvallow && recvArray[j] <= neighvaltop
					&& recvArray[j] >= 0 && recvArray[j] <= 100) {
					matches++;  // Count the number of matches with its neighbours
				}
				report.adjacentNodesTemp[j] = recvArray[j];
			}
			if (matches >= 2) { // If number of matches is 2 or more, send report back to baystation
				printf("Valid sensor node reading - rank:%d value: %d\n", my_rank, randoNum);
				printf("There are %d matches\n", matches);
				report.reading = randoNum;
				report.matches = matches;
				report.neighborsCompared = numOfNeighbours;
				report.xCoor = coord[0];
				report.yCoor = coord[1];
				time_t current_time = time(NULL);
				report.time = current_time;
				report.MPI_time = MPI_Wtime() + timediff;

				MPI_Send(&report, 1, Reporttype, size, 0, master_comm);
			}
		}

	}
	MPI_Comm_free(&comm2D);
	printf("Node: %d - terminating\n", my_rank);

	return 0;
}
