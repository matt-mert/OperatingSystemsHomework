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

#define STACK_SIZE 4096
#define MAX_THREADS 5
#define DEFAULT_BURST 3
#define SWITCH_INTERVAL 3
#define MAX_LENGTH 4096

// These constants represent the different states a thread can be in.
typedef enum {
    READY,
    RUNNING,
    FINISHED,
    IO,
    EMPTY
} ThreadState;

typedef enum {
    CPU1,
    CPU2,
    CPU3,
    IO1,
    IO2,
    IO3
} CurrentBurst;

// ThreadInfo struct...
typedef struct {
    ucontext_t context;
    ThreadState state;
    CurrentBurst current;
    unsigned int threadIndex;
    unsigned int arrayIndex;
    int bursts[6]; // The first 3 are cpu and the last 3 are io
} ThreadInfo;

// Here is our threadArray which is a global variable.
ThreadInfo threadArray[MAX_THREADS];

// Some information about the threads which are global variables.
volatile int currentThread = 0;
volatile int createdThreads = 0;
volatile int finishedThreads = 0;

// Container to put the input data.
int inputArray[42];

// Ready Queue for the Round Robin Algorithm
int readyQueue[4];

// This function is the function we makecontext for each thread.
void threadFunction(void *arg1, void *arg2) {
    unsigned int threadIndex = (uintptr_t) arg1;
    unsigned int arrayIndex = (uintptr_t) arg2;
    while (1) {
        if (threadArray[arrayIndex].state == RUNNING) {
            CurrentBurst burst = threadArray[arrayIndex].current;
            switch (burst)
            {
            case CPU1:
                break;
            case CPU2:
                break;
            case CPU3:
                break;
            case IO1:
                break;
            case IO2:
                break;
            case IO3:
                break;
            default:
                break;
            }
        }
    }
    // counterFunction cagirinca 1 sn delay geliyor
}

// n: cpu burst time, i: thread number
void counterFunction(int n, int i) {
    int j;
    for (j = n - 1; j >= 0; j--) {
        // For the tabs
        int k;
        for (k = 0; k < i; k++) {
            printf("\t");
        }
        printf("%d\n", j);
        usleep(999000); // Sleep for 0.999 seconds
    }
}

// Pops the first element of ready queue. (Helper Function)
int popFront() {
    int temp1 = readyQueue[0];
    int temp2 = readyQueue[1];
    int temp3 = readyQueue[2];
    int temp4 = readyQueue[3];
    readyQueue[0] = temp2;
    readyQueue[1] = temp3;
    readyQueue[2] = temp4;
    readyQueue[3] = -1;
    return temp1;
}

// Pushes a new element to the back of ready queue. (Helper Function)
int pushBack(int val) {
    int i;
    for (i = 0; i < 4; i++) {
        if (readyQueue[i] == -1) {
            readyQueue[i] = val;
            return i;
        }
    }
    return -1;
}

// Helper function to print the information regarding the current status of the threads.
void printerFunction() {
    char running[13];
    char ready[13];
    char finished[13];
    char io[13];

    int i;
    for (i = 0; i < 12; i++) {
        running[i] = ' ';
        ready[i] = ' ';
        finished[i] = ' ';
        io[i] = ' ';
    }

    running[12] = '\0';
    ready[12] = '\0';
    finished[12] = '\0';
    io[12] = '\0';

    int runningCount = 0;
    int readyCount = 0;
    int finishedCount = 0;
    int ioCount = 0;
    for (i = 1; i < MAX_THREADS; i++) {
        if (threadArray[i].state == RUNNING) {
            running[runningCount * 3] = 'T';
            running[runningCount * 3 + 1] = threadArray[i].threadIndex + '0';
            runningCount++;
        }
        else if (threadArray[i].state == READY) {
            ready[readyCount * 3] = 'T';
            ready[readyCount * 3 + 1] = threadArray[i].threadIndex + '0';
            readyCount++;
        }
        else if (threadArray[i].state == FINISHED) {
            finished[finishedCount * 3] = 'T';
            finished[finishedCount * 3 + 1] = threadArray[i].threadIndex + '0';
            finishedCount++;
        }
        else if (threadArray[i].state == IO) {
            io[ioCount * 3] = 'T';
            io[ioCount * 3 + 1] = threadArray[i].threadIndex + '0';
            ioCount++;
        }
    }

    printf("running>%s ready>%s finished>%s IO>%s\n", running, ready, finished, io);
}

// Initializes all global data structures for the thread.
void initializeThread() {
    // Starting from index 1 since the first element of threadArray is reserved for main().
    for (int i = 1; i < MAX_THREADS; i++) {
        threadArray[i].state = EMPTY;
    }
}

// Creates a thread with makecontext if it can find an EMPTY spot in threadArray.
int createThread(void (*func)(void *, void *), void *arg) {
    int i;
    for (i = 1; i < MAX_THREADS; i++) {
        if (threadArray[i].state == EMPTY) {
            getcontext(&threadArray[i].context);
            threadArray[i].context.uc_stack.ss_sp = malloc(STACK_SIZE);
            threadArray[i].context.uc_stack.ss_size = STACK_SIZE;
            threadArray[i].context.uc_link = &threadArray[0].context;
            makecontext(&threadArray[i].context, (void (*)(void))func, 1, arg, i);
            threadArray[i].state = READY;
            threadArray[i].current = CPU1;
            threadArray[i].threadIndex = (uintptr_t) arg;
            threadArray[i].arrayIndex = i;
            createdThreads++;
            break;
        }
    }
    
    // System is unable to create a new thread so it prints an error message and returns -1.
    if (i == MAX_THREADS) {
        printf("ERROR: Cannot create more threads.\n");
        return -1;
    }

    return i;
}

// Gets called every SWITCH_INTERVAL seconds until all 7 processes are finished.
void runThread() {

    // Setting alarm for re-running runThread if not all threads are finished.
    if (finishedThreads < 7) {
        signal(SIGALRM, runThread);
        alarm(SWITCH_INTERVAL);
    }
    else {
        return;
    }

    printerFunction();

    // Scheduler cagirilcak

    // threadArray[id].state = RUNNING;
    // swapcontext(&mainContext, &threadArray[id].context);
}

// Exits the thread so frees the allocated memory for the thread with given index.
void exitThread(int index) {
    free(threadArray[index].context.uc_stack.ss_sp);
}

// *******************************************************************************
// Note that for this scheduling, I will use "Round Robin" scheduling algorithm
// and not lottery as it was given as a choice in the homework pdf.
// *******************************************************************************

// Implementation of Preemptive and Weighted Fair scheduler with Round Robin Algorithm.
void pwf_scheduler() {
    while (1) {
        int i;
        for (i = 0; i < MAX_THREADS; i++) {
            currentThread = (currentThread + 1) % MAX_THREADS;
            if (threadArray[currentThread].state == READY) {
                runThread(currentThread);
                if (threadArray[currentThread].state == FINISHED) {
                    threadArray[currentThread].state = READY;
                }
            }
        }
        // sleep(3);
    }

    // int nextIndex = popFront();
}

// In this program, we only parse the arguments and create user-level threads in main().
int main (int argc, char *argv[]) {
    printf("EE442 Programming Assignment 2 Salih Mert Kucukakinci 2094290\n\n");
    printf("Usage: ./pwf_scheduler /path/to/your/input.txt\n");

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

            In this format, first string is label, the next 3 integers are cpu1, cpu2, cpu3 durations
            and the last 3 integers are io1, io2, io3 durations for the corresponding process and
            X stops reading. */
            
            int i;
            for (i = 0; i < MAX_LENGTH; i++) {
                fscanf(file, "%3s %d %d %d %d %d %d", label, &buffer[0], &buffer[1], &buffer[2], &buffer[3], &buffer[4], &buffer[5]);
                if (label[0] == 'X') {
                    break;
                }
                int t = label[1] - '0';
                int j;
                for (j = 0; j < 6; j++) {
                    inputArray[6 * (t - 1) + j] = buffer[j];
                }
            }

            printf("Reading data is completed.\n\n");
            printf("%d %d %d\n", inputArray[0], inputArray[1], inputArray[2]);
        }
    }
    else {
        printf("ERROR: Invalid arguments specified.\n");
        return 1;
    }

    // PARSING ARGUMENTS COMPLETED...

    printf("T1\tT2\tT3\tT4\tT5\tT6\tT7\n");

    initializeThread();

    int i;
    for (i = createdThreads; i < 4; i++) {
        int k = i + 1;
        void *p = &k;
        int j = createThread(threadFunction, p);
        if (j == -1) {
            break;
        }
    }

    // Setting alarm for running runThread...
    signal(SIGALRM, runThread);
    alarm(SWITCH_INTERVAL);
    getcontext(&threadArray[0].context);

    while (createdThreads < 7) {
        int availableSpace = 0;
        for (i = 1; i < MAX_THREADS; i++) {
            if (threadArray[i].state == EMPTY) {
                availableSpace = 1;
            }
        }
        if (availableSpace == 1) {
            for (i = createdThreads; i < 7; i++) {
                int k = i + 1;
                void *p = &k;
                int j = createThread(threadFunction, p);
                if (j == -1) {
                    break;
                }
            }
        }
    }

    while (1);

    return 0;
}
