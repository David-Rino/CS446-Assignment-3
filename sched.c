//Author: Rino David
//Class: CS 446

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <string.h>
#include <sys/mman.h>

#define ANSI_COLOR_GRAY    "\x1b[30m"
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_WHITE   "\x1b[37m"

#define ANSI_COLOR_RESET   "\x1b[0m"

#define TERM_CLEAR() printf("\033[H\033[J")
#define TERM_GOTOXY(x,y) printf("\033[%d;%dH", (y), (x))

typedef struct thread_data_t { 
    int localTid; 
    const int *data; 
    int numVals; 
    pthread_mutex_t *lock; 
    long long int *totalSum; 
} thread_data_t;

void print_progress(pid_t localTid, size_t value) {
        pid_t tid = syscall(__NR_gettid);

        TERM_GOTOXY(0,localTid+1);

	char prefix[256];
        size_t bound = 100;
        sprintf(prefix, "%d: %ld (ns) \t[", tid, value);
	const char suffix[] = "]";
	const size_t prefix_length = strlen(prefix);
	const size_t suffix_length = sizeof(suffix) - 1;
	char *buffer = calloc(bound + prefix_length + suffix_length + 1, 1);
	size_t i = 0;

	strcpy(buffer, prefix);
	for (; i < bound; ++i)
	{
	    buffer[prefix_length + i] = i < value/10000 ? '#' : ' ';
	}
	strcpy(&buffer[prefix_length + i], suffix);
        
        if (!(localTid % 7)) 
            printf(ANSI_COLOR_WHITE "\b%c[2K\r%s\n" ANSI_COLOR_RESET, 27, buffer);  
        else if (!(localTid % 6)) 
            printf(ANSI_COLOR_BLUE "\b%c[2K\r%s\n" ANSI_COLOR_RESET, 27, buffer);  
        else if (!(localTid % 5)) 
            printf(ANSI_COLOR_RED "\b%c[2K\r%s\n" ANSI_COLOR_RESET, 27, buffer);  
        else if (!(localTid % 4)) 
            printf(ANSI_COLOR_GREEN "\b%c[2K\r%s\n" ANSI_COLOR_RESET, 27, buffer);  
        else if (!(localTid % 3)) 
            printf(ANSI_COLOR_CYAN "\b%c[2K\r%s\n" ANSI_COLOR_RESET, 27, buffer);  
        else if (!(localTid % 2)) 
            printf(ANSI_COLOR_YELLOW "\b%c[2K\r%s\n" ANSI_COLOR_RESET, 27, buffer);  
        else if (!(localTid % 1)) 
            printf(ANSI_COLOR_MAGENTA "\b%c[2K\r%s\n" ANSI_COLOR_RESET, 27, buffer);  
        else
            printf("\b%c[2K\r%s\n", 27, buffer);

	fflush(stdout);
	free(buffer);
}

void* arraySum(void* param) {
    thread_data_t* data = (thread_data_t*) param;
    long long int threadSum = 0;

    while(1) {

        double maxLatency = 0;

        for (int i = 0; i < data->numVals; i++) {
            struct timespec startTime, endTime;

            clock_gettime(CLOCK_MONOTONIC, &startTime);
            threadSum += data->data[i];
            clock_gettime(CLOCK_MONOTONIC, &endTime);

            long int convertedStart = (startTime.tv_sec * 1000000000.0 + startTime.tv_nsec);
            long int convertedEnd = (endTime.tv_sec * 1000000000.0 + endTime.tv_nsec);

            long int timeDiff = (convertedEnd - convertedStart);

            if (timeDiff > maxLatency) {
                maxLatency = timeDiff;
            }
        }

        pthread_mutex_lock(data->lock);
        *data->totalSum += threadSum;
        pthread_mutex_unlock(data->lock);

        print_progress(data->localTid, maxLatency);
    }

    return NULL;
}

int main(int argc, char* argv[]) {

    if(argc != 2) {
        printf("Invalid Amount of parameters\n");
        return -1;
    }

    int numThreads = atoi(argv[1]);
    int* array = (int*)malloc(2000000 * sizeof(int));
    long long int totalSum = 0;
    
    pthread_mutex_t lock;
    pthread_mutex_t *lockPtr = NULL;

    pthread_mutex_init(&lock, NULL);
    lockPtr = &lock;

    thread_data_t threadArray[numThreads];

    for (int i = 0; i < numThreads; i++) {
        threadArray[i].localTid = i;
        threadArray[i].data = array;
        threadArray[i].numVals = 2000000;
        threadArray[i].lock = lockPtr;
        threadArray[i].totalSum = &totalSum;
    }

    pthread_t totalThreads[numThreads];

    for (int i = 0; i < numThreads; i++) {
        pthread_create(&totalThreads[i], NULL, arraySum, &threadArray[i]);
    }

    for (int i = 0; i <numThreads; i++) {
        pthread_join(totalThreads[i], NULL);
    }

    return 0;
}

