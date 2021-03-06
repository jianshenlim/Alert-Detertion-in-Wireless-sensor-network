
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

/* FUNCTION DEFINITION */
// Function records appropriate sensor information to output file
int logAnEntry(FILE* outFile, int iteration, time_t alertedTime, int alertType, int repNode, int repX, int repY, int temp, int adjacentNodes[], int adjacentNodesX[], int adjacentNodesY[], int adjacentTemp[], int matches, time_t sateTime, int sateTimeTemp, int nrows, int ncols, double timeTaken, int IPaddr[], int NeighIP[]) {
	fprintf(outFile, "------------------------------------------------------\n");
	struct tm* alertedTimeinfo;
	alertedTimeinfo = localtime(&alertedTime);
	time_t loggedTime = time(NULL);
	struct tm* loggedTimeinfo;
	loggedTimeinfo = localtime(&loggedTime);
	fprintf(outFile, "Iteration : %d\n", iteration);
	fprintf(outFile, "Logged Time :                       %02d:%02d:%02d %02d-%02d-%d\n", loggedTimeinfo->tm_hour, loggedTimeinfo->tm_min, loggedTimeinfo->tm_sec, loggedTimeinfo->tm_mday, loggedTimeinfo->tm_mon + 1, loggedTimeinfo->tm_year + 1900);
	fprintf(outFile, "Alert Reported Time :               %02d:%02d:%02d %02d-%02d-%d\n", alertedTimeinfo->tm_hour, alertedTimeinfo->tm_min, alertedTimeinfo->tm_sec, alertedTimeinfo->tm_mday, alertedTimeinfo->tm_mon + 1, alertedTimeinfo->tm_year + 1900);
	if (alertType) 	fprintf(outFile, "Alert Type : True\n");
	else fprintf(outFile, "Alert Type : False\n");
	fprintf(outFile, "Reporting Node 	    Coord		Temp              IP address\n");
	fprintf(outFile, "%10d%12d,%d%11d%18d:%d:%d:%d\n", repNode, repX, repY, temp, IPaddr[0], IPaddr[1], IPaddr[2], IPaddr[3]);
	fprintf(outFile, "Adjacent Nodes 	    Coord		Temp              IP address\n");
	int index = 0;
	for (int j = 0; j < 4; j++) {
		if (adjacentNodes[j] >= 0 && adjacentNodes[j] < ncols * nrows) {
			fprintf(outFile, "%10d%12d,%d%11d%18d:%d:%d:%d\n", adjacentNodes[j], adjacentNodesX[j], adjacentNodesY[j], adjacentTemp[j], NeighIP[index], NeighIP[index + 1], NeighIP[index + 2], NeighIP[index + 3]);
			index = index + 4;
		}

	}
	struct tm* satTimeinfo;
	satTimeinfo = localtime(&sateTime);
	fprintf(outFile, "Infrared Satellite Reporting Time :         %02d:%02d:%02d %02d-%02d-%d\n", satTimeinfo->tm_hour, satTimeinfo->tm_min, satTimeinfo->tm_sec, satTimeinfo->tm_mday, satTimeinfo->tm_mon + 1, satTimeinfo->tm_year + 1900);
	fprintf(outFile, "Infrared Satellite Reporting(Celsius) :              %d\n", sateTimeTemp);
	fprintf(outFile, "Communication Time(seconds) : %.10f\n", timeTaken);
	fprintf(outFile, "Total Messages send between reporting node and base station : 1\n");
	fprintf(outFile, "Number of adjacent matches to reporting node : %d\n", matches);
	return 0;
}
