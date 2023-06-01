// **************************************************************
//              EE442 Programming Assignment 2
//              Salih Mert Kucukakinci 2094290
// **************************************************************

#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>

#define STACK_SIZE 8192 // For the context to be stored.
#define MAX_THREADS 5 // First one is reserved for main().
#define NUM_THREADS 7 // Total number of threads.
#define CPU_BURST_AMOUNT 3 // Amount of CPU bursts for each thread.
#define IO_BURST_AMOUNT 3 // Amount of IO bursts for each thread.
#define SWITCH_INTERVAL 3 // Time between rescheduling.
#define MAX_LENGTH 4096 // For reading data from txt file.

int popFront();
void pushBack(int val);
void initializeQueue();
void printQueue();
void printStatus();
int pwf_scheduler();
void io_device();
void printerFunction();
void initializeThread();
int createThread(void (*func)(int));
void runThread();
void exitThread(int index);
void counterFunction(int threadIndex, int arrayIndex);
void threadFunction(int arrayIndex);

// These constants represent the different states a thread can be in.
typedef enum {
    READY,
    RUNNING,
    FINISHED,
    IO,
    EMPTY,
} ThreadState;

// Every thread will do these operations in order CPU1, IO1, CPU2, IO2, CPU3, IO3
typedef enum {
    CPU1,
    IO1,
    CPU2,
    IO2,
    CPU3,
    IO3,
    DONE
} CurrentBurst;

// ThreadInfo struct...
typedef struct {
    ucontext_t context;
    ThreadState state;
    CurrentBurst current;
    int threadIndex;
    int arrayIndex;
    int bursts[CPU_BURST_AMOUNT + IO_BURST_AMOUNT]; // The first 3 are cpu and the last 3 are io
} ThreadInfo;

// Here is our threadArray which is a global variable.
ThreadInfo threadArray[MAX_THREADS];

// Some information about the threads which are global variables.
volatile int currentArrayIndex = 0;
volatile int createdThreads = 0;
volatile int finishedThreads = 0;
int threadStates[NUM_THREADS];

// Container to put the input data.
int inputArray[NUM_THREADS * (CPU_BURST_AMOUNT + IO_BURST_AMOUNT)];

// Ready Queue for the SRTF scheduler algorithm
int readyQueue[MAX_THREADS - 1]; // Stores array indexes of threads.
int readyStatus[MAX_THREADS]; // Stores the states of each thread.

// *******************************************************************************
// Helper functions for the SRTF scheduler algorithm implementation
// *******************************************************************************

// For SRTF algorithm, since we do not choose the first comer directly. (Helper Function)
int popSelect(int index) {
    if (readyQueue[0] == -1) {
        return -1;
    }
    int temp[MAX_THREADS - 1];
    int i;
    for (i = 0; i < MAX_THREADS - 1; i++) {
        temp[i] = readyQueue[i];
    }
    for (i = 0; i < MAX_THREADS - 1; i++) {
        if (i == MAX_THREADS - 2) {
            readyQueue[i] = -1;
        }
        else if (i >= index) {
            readyQueue[i] = temp[i + 1];
        }
    }
    int value = temp[index];
    return value;
}

// For SRTF algorithm, finds thread with shortest burst duration. (Helper Function)
int findShortest() {
    if (readyQueue[0] == -1) {
        return -1;
    }
    int i;
    int shortestIndex = 0;
    for (i = 0; i < MAX_THREADS - 1; i++) {
        if (i == 0) { continue; }
        if (readyQueue[i] == -1) { continue; }
        int prevIndex = readyQueue[shortestIndex];
        int thisIndex = readyQueue[i];
        CurrentBurst prevBurst = threadArray[prevIndex].current;
        CurrentBurst thisBurst = threadArray[thisIndex].current;
        int prevRemaining = threadArray[prevIndex].bursts[prevBurst];
        int thisRemaining = threadArray[thisIndex].bursts[thisBurst];
        if (thisRemaining < prevRemaining) {
            shortestIndex = i;
        }
    }
    return shortestIndex;
}

// Pops the first element of ready queue. (Helper Function)
int popFront() {
    if (readyQueue[0] == -1) {
        return -1;
    }
    int temp[MAX_THREADS - 1];
    int i;
    for (i = 0; i < MAX_THREADS - 1; i++) {
        temp[i] = readyQueue[i];
    }
    for (i = 0; i < MAX_THREADS - 1; i++) {
        if (i == MAX_THREADS - 2) {
            readyQueue[i] = -1;
        }
        else {
            readyQueue[i] = temp[i + 1];
        }
    }
    int value = temp[0];
    return value;
}

// Pushes a new element to the back of ready queue. (Helper Function)
void pushBack(int val) {
    int i;
    for (i = 0; i < MAX_THREADS - 1; i++) {
        if (readyQueue[i] == -1) {
            readyQueue[i] = val;
            break;
        }
    }
}

// Initializes the initial values of the ready queue. (Helper Function)
void initializeQueue() {
    int i;
    readyQueue[3] = -1;
    for (i = 0; i < MAX_THREADS - 2; i++) {
        readyQueue[i] = i + 2;
    }
    readyStatus[0] = 0;
    readyStatus[1] = 0;
    for (i = 2; i < MAX_THREADS; i++) {
        readyStatus[i] = 1;
    }
}

// Prints the current values of the ready queue. (Helper Function)
void printQueue() {
    printf("\nReadyQueue:");
    int i;
    for (i = 0; i < MAX_THREADS - 1; i++) {
        printf(" %d", readyQueue[i]);
    }
    printf("\n");
}

void printStatus() {
    printf("ReadyStatus:");
    int i;
    for (i = 0; i < MAX_THREADS; i++) {
        printf(" %d", readyStatus[i]);
    }
    printf("\n");
}

// *******************************************************************************
// Shortest Remaining Time First Scheduling Algorithm Implementation
// *******************************************************************************

// Implementation of Shortest Remaining Time First algorithm.
int srtf_scheduler() {

    // Initializing the readyQueue
    if (currentArrayIndex == 0) {
        initializeQueue();
        threadArray[1].state = RUNNING;
        threadStates[0] = RUNNING;
        currentArrayIndex = 1;
        return 1;
    }

    // See if any new threads became ready until the last check (due to finished io etc)
    int i;
    for (i = 1; i < MAX_THREADS; i++) {
        if (threadArray[i].state == READY && readyStatus[i] != 1) {
            pushBack(i);
            readyStatus[i] = 1;
        }
    }

    // Stop the current thread from running and make its state READY, push it to readyQueue.
    if (threadArray[currentArrayIndex].state == RUNNING) {
        threadArray[currentArrayIndex].state = READY;
        int threadIndex = threadArray[currentArrayIndex].threadIndex;
        threadStates[threadIndex - 1] = READY;
        pushBack(currentArrayIndex);
        readyStatus[currentArrayIndex] = 1;
    }

    int shortestFromQueue = findShortest();
    int shortestArrayIndex = popSelect(shortestFromQueue);

    if (shortestArrayIndex == -1) {
        return -1;
    }

    int shortestThreadIndex = threadArray[shortestArrayIndex].threadIndex;
    printf("\nThread with the shortest remaining burst time is selected as: T%d\n", shortestThreadIndex);
    
    threadArray[shortestArrayIndex].state = RUNNING;
    int threadIndex = threadArray[shortestArrayIndex].threadIndex;
    threadStates[threadIndex - 1] = RUNNING;
    currentArrayIndex = shortestArrayIndex;
    readyStatus[shortestArrayIndex] = 0;
    return shortestArrayIndex;
}

// We have enough IO devices to do all IO simultaneously, therefore,
// this function "simulates" an io device running as a simultaneous task.
void io_device() {
    int i;
    for (i = 1; i < MAX_THREADS; i++) {
        if (threadArray[i].state == IO) {
            CurrentBurst burst = threadArray[i].current;
            int duration = threadArray[i].bursts[burst];
            int remaining = duration - SWITCH_INTERVAL;
            int threadIndex = threadArray[i].threadIndex;
            if (remaining <= 0) {
                threadArray[i].bursts[burst] = 0;
                threadArray[i].current++;
                if (threadArray[i].current == DONE) {
                    threadArray[i].state = FINISHED;
                    threadStates[threadIndex - 1] = FINISHED;
                    finishedThreads++;
                    exitThread(i);
                }
                else {
                    threadArray[i].state = READY;
                    threadStates[threadIndex - 1] = READY;
                    readyStatus[i] = 1;
                    int k = i;
                    pushBack(k);
                }
            }
            else {
                threadArray[i].bursts[burst] = remaining;
            }
        }
    }
}

// *******************************************************************************
// Helper functions for any scheduling algorithm to be implemented
// *******************************************************************************

// Helper function to print the information about the current status of the threads.
void printerFunction() {
    int spaceAmount = 2;
    int maxRunning = 3 + spaceAmount + 1;
    int maxReady = 3 * (MAX_THREADS - 2) + spaceAmount + 1;
    int maxFinished = 3 * (NUM_THREADS) + spaceAmount + 1;
    int maxIo = 3 * (MAX_THREADS - 1) + spaceAmount + 1;

    char running[maxRunning];
    char ready[maxReady];
    char finished[maxFinished];
    char io[maxIo];

    int i;
    for (i = 0; i < maxRunning; i++) {
        running[i] = ' ';
    }
    for (i = 0; i < maxReady; i++) {
        ready[i] = ' ';
    }
    for (i = 0; i < maxFinished; i++) {
        finished[i] = ' ';
    }
    for (i = 0; i < maxIo; i++) {
        io[i] = ' ';
    }

    // Editing last indexes.
    running[maxRunning - 1] = '\0';
    ready[maxReady - 1] = '\0';
    finished[maxFinished - 1] = '\0';
    io[maxIo - 1] = '\0';

    int runningCount = 0;
    int readyCount = 0;
    int finishedCount = 0;
    int ioCount = 0;
    for (i = 0; i < NUM_THREADS; i++) {
        if (threadStates[i] == RUNNING) {
            running[runningCount * 3] = 'T';
            running[runningCount * 3 + 1] = (i + 1) + '0';
            runningCount++;
        }
        else if (threadStates[i] == READY) {
            ready[readyCount * 3] = 'T';
            ready[readyCount * 3 + 1] = (i + 1) + '0';
            readyCount++;
        }
        else if (threadStates[i] == FINISHED) {
            finished[finishedCount * 3] = 'T';
            finished[finishedCount * 3 + 1] = (i + 1) + '0';
            finishedCount++;
        }
        else if (threadStates[i] == IO) {
            io[ioCount * 3] = 'T';
            io[ioCount * 3 + 1] = (i + 1) + '0';
            ioCount++;
        }
    }

    printf("running>%s ready>%s finished>%s IO>%s\n", running, ready, finished, io);

    for (i = 1; i <= NUM_THREADS; i++) {
        if (i == NUM_THREADS) {
            printf("T%d\n", i);
            break;
        }
        printf("T%d\t", i);
    }
}

// *******************************************************************************
// Functions related to the threads: initialize, create, run, and exit.
// *******************************************************************************

// Initializes all global data structures for the thread.
void initializeThread() {
    // Starting from index 1 since the first element of threadArray is reserved for main().
    int i;
    for (i = 1; i < MAX_THREADS; i++) {
        threadArray[i].state = EMPTY;
    }
    for (i = 0; i < NUM_THREADS; i++) {
        threadStates[i] = EMPTY;
    }
}

// Creates a thread with makecontext if it can find an EMPTY spot in threadArray.
int createThread(void (*func)(int)) {
    int i;
    for (i = 1; i < MAX_THREADS; i++) {
        if (threadArray[i].state == EMPTY) {
            threadStates[createdThreads] = READY;
            createdThreads++;
            break;
        }
    }

    if (i == MAX_THREADS) {
        printf("ERROR: Cannot create any more threads.\n");
        return -1;
    }

    getcontext(&threadArray[i].context);
    threadArray[i].context.uc_stack.ss_sp = malloc(STACK_SIZE);
    threadArray[i].context.uc_stack.ss_size = STACK_SIZE;
    threadArray[i].context.uc_link = &threadArray[0].context;
    threadArray[i].state = READY;
    threadArray[i].current = CPU1;
    threadArray[i].threadIndex = createdThreads;
    threadArray[i].arrayIndex = i;
    // Filling the bursts of thread from the information from the user input.
    int j;
    for (j = 0; j < CPU_BURST_AMOUNT + IO_BURST_AMOUNT; j++) {
        threadArray[i].bursts[j] = inputArray[(CPU_BURST_AMOUNT + IO_BURST_AMOUNT) * (createdThreads - 1) + j];
        // printf("BURST:%d\n", threadArray[i].bursts[j]);
    }
    printf("Created thread: %d\n", createdThreads);
    makecontext(&threadArray[i].context, (void (*)(void))func, 1, i);
    return i;
}

// Gets called every SWITCH_INTERVAL seconds until all 7 processes are finished.
void runThread() {
    // if not all threads are finished...
    if (finishedThreads >= 7) {
        return;
    }

    int index = srtf_scheduler();
    printQueue();
    // printStatus();
    printerFunction();
    io_device();

    // Setting alarm for re-running runThread if not all threads are finished.
    signal(SIGALRM, runThread);
    alarm(SWITCH_INTERVAL);

    if (index == -1) {
        return;
    }
    setcontext(&threadArray[index].context);
}

// Exits the thread so frees the allocated memory for the thread with given index.
void exitThread(int index) {
    free(threadArray[index].context.uc_stack.ss_sp);
    threadArray[index].state = EMPTY;
}

// *******************************************************************************
// The functions that the threads do during their process times.
// *******************************************************************************

void threadFunction(int arrayIndex) {
    int threadIndex = threadArray[arrayIndex].threadIndex;
    while (1) {
        usleep(950000);
        CurrentBurst burst = threadArray[arrayIndex].current;
        threadArray[arrayIndex].bursts[burst]--;
        int remaining = threadArray[arrayIndex].bursts[burst];
        int i;
        for (i = 1; i < threadIndex; i++) {
            printf("\t");
        }
        printf("%d\n", remaining);
        if (remaining == 0) {
            threadArray[arrayIndex].current++;
            if (threadArray[arrayIndex].current == DONE) {
                threadArray[arrayIndex].state = FINISHED;
                threadStates[threadIndex - 1] = FINISHED;
                finishedThreads++;
                break;
            }
            else {
                threadArray[arrayIndex].state = IO;
                threadStates[threadIndex - 1] = IO;
                setcontext(&threadArray[0].context);
            }
        }
        getcontext(&threadArray[arrayIndex].context);
    }
    finishedThreads++;
    exitThread(arrayIndex);
}

// *******************************************************************************
// Main function where we only parse the arguments and create user-level threads.
// *******************************************************************************

// In this program, we only parse the arguments and create user-level threads in main().
int main (int argc, char *argv[]) {
    printf("EE442 Programming Assignment 2 Salih Mert Kucukakinci 2094290\n");
    printf("Scheduler 2: Shortest Remaining Time First Algorithm.\n\n");
    printf("Usage       : ./pwf_scheduler /path/to/your/input.txt\n");
    printf("Input Format: [T1] [t1_cpu1] [t1_io1] [t1_cpu2] [t1_io2] ...\n");
    printf("              [T2] [t2_cpu1] [t2_io1] [t2_cpu2] [t2_io2] ...\n");
    printf("               X\n\n");

    // PARSING ARGUMENTS STARTED...

    if (argc == 2) {
        char *filename = argv[1];
        FILE *file = fopen(filename, "r");
        if (file == NULL) {
            printf("Error: Could not open file %s\n", filename);
            return 1;
        }
        else {
            printf("Input file was found! Reading data from the input file.\n");
            int buffer[MAX_LENGTH];
            char label[3];

            /* Assuming that input file has content in below format:

                T1 6 4 2 4 5 3
                T2 1 2 3 4 5 6
                T3 2 4 5 1 2 5
                T4 2 9 4 5 1 2
                T5 2 8 3 2 3 1
                T6 3 5 5 7 1 1
                T7 3 2 3 1 1 1
                X

            The format: [T1] [t1_cpu1] [t1_io1] [t1_cpu2] [t1_io2] [t1_cpu3] [t1_io3] */
            
            int i;
            for (i = 0; i < MAX_LENGTH; i++) {
                fscanf(file, "%3s %d %d %d %d %d %d", label, &buffer[0], &buffer[1], &buffer[2], &buffer[3], &buffer[4], &buffer[5]);
                if (label[0] == 'X') {
                    break;
                }
                int t = label[1] - '0';
                int j;
                for (j = 0; j < CPU_BURST_AMOUNT + IO_BURST_AMOUNT; j++) {
                    inputArray[(CPU_BURST_AMOUNT + IO_BURST_AMOUNT) * (t - 1) + j] = buffer[j];
                }
            }

            printf("Reading data is completed.\n\n");
        }
    }
    else {
        printf("ERROR: Invalid arguments specified.\n");
        return 1;
    }

    // PARSING ARGUMENTS COMPLETED...

    // Initializing requirements for the threads.
    initializeThread();

    // Creating the threads.
    int i;
    for (i = 1; i < MAX_THREADS; i++) {
        int j = createThread(threadFunction);
        if (j == -1) {
            break;
        }
    }

    // Setting an alarm for running the runThread function.
    signal(SIGALRM, runThread);
    alarm(SWITCH_INTERVAL);
    getcontext(&threadArray[0].context);

    // Trying to create threads once a thread is finished and a slot is EMPTY'ied.
    while (createdThreads < NUM_THREADS) {
        usleep(100);
        int availableSpace = 0;
        for (i = 1; i < MAX_THREADS; i++) {
            if (threadArray[i].state == EMPTY) {
                availableSpace = 1;
            }
        }
        if (availableSpace == 1) {
            for (i = 0; i < NUM_THREADS - createdThreads; i++) {
                int j = createThread(threadFunction);
                if (j == -1) {
                    break;
                }
            }
        }
    }

    while (1);

    return 0;
}
