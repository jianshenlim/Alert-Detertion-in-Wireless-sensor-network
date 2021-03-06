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


int master_io(MPI_Comm master_comm, MPI_Comm comm, int nrows, int ncols, int numIterations)
{
	int iterations = numIterations;
	int totalFalse = 0;
	int totalPos = 0;
	int totalMes = 0;
	double totalComTime = 0;
	int numOfCor = nrows * ncols;


	FILE* outFile = fopen("BaseStationReport.txt", "w");
	fprintf(outFile, "Welcome to Monash bush fire prevention task force report\n");

	pthread_t tid;
	pthread_t tid2;
	pthread_create(&tid, 0, ThreadFunc, &numOfCor);
	pthread_create(&tid2, 0, ThreadFunc2, &numOfCor);
	int run = 1;
	int gotData[numOfCor];
	MPI_Request listenRequest[numOfCor];
	MPI_Status listenStatus[numOfCor];
	struct NodeReport report[numOfCor];

	double endTime = MPI_Wtime();
	for (int i = 0; i < numOfCor; i++) {    // Send BaseStation time to nodes

		MPI_Send(&endTime, 1, MPI_DOUBLE, i, 0, master_comm);
	}

	while (run) {
		printf("Iteration: %d\n", iterations);
		pthread_mutex_lock(&gMutex);
		if (sentinelValue == 0)
		{
			iterations = -1;
		}
		pthread_mutex_unlock(&gMutex);

		if (iterations == -1) {     // If reached end of number of iterations.
			run = 0;
			for (int i = 0; i < numOfCor; i++) {    // Send Termination signal to all nodes
				MPI_Send(&run, 1, MPI_INT, i, 0, master_comm);
			}
			pthread_mutex_lock(&gMutex);
			keepRunning = 0;
			pthread_mutex_unlock(&gMutex);
			sleep(3);
		}

		for (int i = 0; i < numOfCor; i++) {        // For each iteration, call Irecv to receive potential report from sensor nodes
			MPI_Irecv(&report[i], 1, Reporttype, i, 0, master_comm, &listenRequest[i]);

		}
		sleep(1); // Wait for a bit
		for (int i = 0; i < numOfCor; i++) {
			MPI_Test(&listenRequest[i], &gotData[i], &listenStatus[i]); //After waiting test to see if a report was received from sensor nodes
			if (!gotData[i]) {  // If not data received, nodes must not have sent a request, time out Irecv for that node and stop waiting
				MPI_Cancel(&listenRequest[i]);
				MPI_Request_free(&listenRequest[i]);
			}
			else {  // Else, recived a reply from a sensor node, process the received information
				double endTime = MPI_Wtime();
				totalMes++;
				time_t time = report[i].time;
				struct tm* timeinfo;
				timeinfo = localtime(&time);
				double comTime = endTime - report[i].MPI_time;
				printf("Comm Time: %.20f\n", endTime);
				printf("Comm Time: %.20f\n", report[i].MPI_time);
				totalComTime += comTime;
				printf("Base station received alert from node: (%d,%d) %02d:%02d:%02d %02d-%02d-%d\n", report[i].xCoor, report[i].yCoor, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900);

				for (int j = 0; j < 4; j++) {  // Get the nodes adjacent node values
					if (report[i].adjacentNodesRank[j] >= 0 && report[i].adjacentNodesRank[j] < ncols * nrows) {
						printf("%d %d %d %d\n", report[i].adjacentNodesRank[j], report[i].adjacentNodesX[j], report[i].adjacentNodesY[j], report[i].adjacentNodesTemp[j]);
					}
				}

				int nodeValLower = report[i].reading - TOLERANCE;
				int nodeValUpper = report[i].reading + TOLERANCE;
				int sateVal;
				time_t sateTime;

				pthread_mutex_lock(&gMutex);
				sateVal = *(arrayOfGeneratedReading + i);
				sateTime = *(arrayOfGeneratedReadingTime + i);
				pthread_mutex_unlock(&gMutex);

				int mode;
				double diff = abs(difftime(sateTime, time));
				if (sateVal >= nodeValLower && sateVal <= nodeValUpper && diff <= 1)
				{
					mode = 1;
					totalPos++;
				}
				else {
					mode = 0;
					totalFalse++;
				}
				// Call logAnEntry function to record log information file into output 
				logAnEntry(outFile, iterations, time, mode, nrows * report[i].xCoor + report[i].yCoor, report[i].xCoor, report[i].yCoor, report[i].reading, report[i].adjacentNodesRank,
					report[i].adjacentNodesX, report[i].adjacentNodesY, report[i].adjacentNodesTemp, report[i].matches, sateTime, sateVal, nrows, ncols, comTime, report[i].IPaddress, report[i].neighIPaddress);
			}
		}
		iterations--;
		//By now we either have received the data, or taken too long, so...
	}
	fprintf(outFile, "------------------------------------------------------\n");
	fprintf(outFile, "The total messages received from the nodes: %d\n", totalMes);
	fprintf(outFile, "The total positive alerts: %d\n", totalPos);
	fprintf(outFile, "The total negative alerts: %d\n", totalFalse);
	fprintf(outFile, "The total communication time: %.10f\n", totalComTime);
	fclose(outFile);
	pthread_join(tid, NULL);
	pthread_join(tid2, NULL);

	return 0;
}
