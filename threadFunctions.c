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

// Thread Function generates random temperature temperature and coordinate 
void* ThreadFunc(void* numOfNum)
{
	int num = *((int*)numOfNum);
	int running = 1;
	while (running) {
		pthread_mutex_lock(&gMutex);
		for (int i = 0; i < num; i++) {                     // For all nodes in the sensor grid
			unsigned int seed = time(NULL) + 6565;
			int randoNum = rand_r(&seed) % 100 + 1;
			*(arrayOfGeneratedReading + i) = randoNum;      // Assign random Temperature generated
			*(arrayOfGeneratedReadingTime + i) = time(NULL);    // Assign recorded time

		}
		pthread_mutex_unlock(&gMutex);
		sleep(0.1);
		pthread_mutex_lock(&gMutex);
		running = keepRunning;              // Thread keeps running until BAYSTATION STOPS
		pthread_mutex_unlock(&gMutex);
	}
	return NULL;
}

// Thread Function used to monitor sentinel value and terminate program when value changes
void* ThreadFunc2(void* numOfNum)
{

	int running = 1;
	while (running) {
		FILE* senFile = fopen("sentinelvalue.txt", "r");
		int input;
		fscanf(senFile, "%d", &input);
		pthread_mutex_lock(&gMutex);
		if (input == 0) {
			sentinelValue = 0;
		}
		pthread_mutex_unlock(&gMutex);
		fclose(senFile);
		sleep(1);
		pthread_mutex_lock(&gMutex);
		running = keepRunning;
		pthread_mutex_unlock(&gMutex);
	}
	return NULL;
}
