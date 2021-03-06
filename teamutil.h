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
// Virtual Topology constants
#define SHIFT_ROW 0
#define SHIFT_COL 1
#define DISP 1      // Displacement

#define THRESHOLD 80
#define TOLERANCE 5
#define ITERATIONS  100

// Function declarations
int logAnEntry(FILE* outFile, int iteration, time_t alertedTime, int alertType, int repNode, int repX, int repY, int temp, int adjacentNodes[], int adjacentNodesX[], int adjacentNodesY[], int adjacentTemp[], int matches, time_t sateTime, int sateTimeTemp, int nrows, int ncols, double timeTaken, int IPaddr[], int NeighIp[]);
// Function for master/slave
int master_io(MPI_Comm master_comm, MPI_Comm comm, int nrows, int ncols, int numIterations);
int slave_io(MPI_Comm master_comm, MPI_Comm comm, int size, int nrows, int ncols, int mode);
int logAnEntry(FILE* outFile, int iteration, time_t alertedTime, int alertType, int repNode, int repX, int repY, int temp, int adjacentNodes[], int adjacentNodesX[], int adjacentNodesY[], int adjacentTemp[], int matches, time_t sateTime, int sateTimeTemp, int nrows, int ncols, double timeTaken, int IPaddr[], int NeighIp[]);
//functions for Posix threads
void* ThreadFunc(void* numOfNum);
void* ThreadFunc2(void* numOfNum);
// Struct For recording a Node's report
struct NodeReport {
	int reading;
	int matches;
	int neighborsCompared;
	int xCoor;
	int yCoor;
	double MPI_time;
	time_t time;
	int adjacentNodesX[4];
	int adjacentNodesY[4];
	int adjacentNodesRank[4];
	int adjacentNodesTemp[4];
	int IPaddress[4];
	int neighIPaddress[16];
};

int* arrayOfGeneratedReading;
extern int keepRunning;
extern int sentinelValue;

time_t* arrayOfGeneratedReadingTime;
pthread_mutex_t gMutex;
MPI_Datatype Reporttype;
