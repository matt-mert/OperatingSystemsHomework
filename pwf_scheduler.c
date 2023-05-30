// **************************************************************
//              EE442 Programming Assignment 2
//              Salih Mert Kucukakinci 2094290
// **************************************************************

#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
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
    int index;
    int all_bursts[6]; // First 3 are cpu and last 3 are io
} ThreadInfo;

// Here is our threadArray which is a global variable.
ThreadInfo threadArray[MAX_THREADS];

// To store the context of the main() function.
ucontext_t mainContext;

// Index of currently running thread which is a global variable.
volatile int currentThread = 0;
volatile int createdThreads = 0;

int t1_bursts[6];
int t2_bursts[6];
int t3_bursts[6];
int t4_bursts[6];
int t5_bursts[6];
int t6_bursts[6];
int t7_bursts[6];

// Counter function that we make context for each thread, n: cpu burst time, i: thread number
void counterFunction(int n, int i) {
    int j;
    for (j = n - 1; j >= 0; j--) {
        // For the tabs
        int k;
        for (k = 0; k < i; k++) {
            printf("\t");
        }
        printf("%d\n", j);
        // sleep(1);
    }
}

void printerFunction() {
    char running[4];
    char ready[20];
    char finished[20];
    char IO[20];
    int i;
    for (i = 0; i < MAX_THREADS; i++) {
        if (threadArray[i].state == RUNNING) {
            running[0] = 'T';
            running[1] = atoi(threadArray[i].index);
            // running[2] = 
        }
    }

    printf("running>%s ready>%s finished>%s IO>%s", running, ready, finished, IO);
}

// Initializes all global data structures for the thread.
void initializeThread() {
    // Starting from index 1 since the first element of threadArray is reserved for main().
    for (int i = 1; i < MAX_THREADS; i++) {
        getcontext(&threadArray[i].context);
        threadArray[i].context.uc_stack.ss_sp = malloc(STACK_SIZE);
        threadArray[i].context.uc_stack.ss_size = STACK_SIZE;
        threadArray[i].context.uc_link = &mainContext;
        threadArray[i].state = EMPTY;
    }
    getcontext(&mainContext);
}

// The explanation of the parameters:
// void (*func)(void *)  :  func is a pointer to a function, in my case, it is going to be
//                          counterFunction for each created thread.
// 
// void *arg1, *arg2     :  This is a void pointer which can point to a variable of any type.
//                          This is a way to pass the necessary arguments to the counterFunction.
int createThread(void (*func)(void *), void *arg1, void *arg2) {
    int i;
    for (i = 0; i < MAX_THREADS; i++) {
        if (threadArray[i].state == EMPTY) {
            getcontext(&threadArray[i].context);
            threadArray[i].context.uc_stack.ss_sp = malloc(STACK_SIZE);
            threadArray[i].context.uc_stack.ss_size = STACK_SIZE;
            threadArray[i].context.uc_link = &mainContext;
            makecontext(&threadArray[i].context, (void (*)(void))func, 1, arg1, arg2);
            threadArray[i].state = READY;
            break;
        }
    }
    
    if (i == MAX_THREADS) {
        printf("Cannot create more threads\n");
        return -1;
    }

    return i;
}

void runThread(int id) {
    threadArray[id].state = RUNNING;
    swapcontext(&mainContext, &threadArray[id].context);
}

void exitThread(int id) {
    free(threadArray[id].context.uc_stack.ss_sp);
    threadArray[id].state = EMPTY;
}

// Note that for this scheduling, I will use "Round Robin" scheduling,
// and not lottery scheduling as it seems to be our choice.

// Implementation of Preemptive and Weighted Fair scheduler with Round Robin Algorithm.
void PWF_scheduler() {
    // Since I choose to use Round Robin algorithm, weights are equal so...
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
}

// In this program, we only parse the arguments and create user-level threads in main().
int main (int argc, char *argv[]) {
    printf("EE442 Programming Assignment 2 Salih Mert Kucukakinci 2094290\n");
    printf("Usage: ./pwf_scheduler **arguments\n");
    printf("If you do not enter any arguments, the program will try to find an input file.\n");
    printf("If you enter 14 arguments (t1_cpu t2_cpu ... t7_cpu t1_io t2_io ... t7_io), then\n");
    printf("3 bursts of cpu and 3 bursts of io will have the same duration for each process.\n");

    if (argc == 1) {
        printf("You did not enter any arguments. Trying to find an input file.\n");
        
        char *filename = "input.txt";
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

            In this format, first string is label, the next 3 integers are cpu1, cpu2, cpu3 durations
            and the last 3 integers are io1, io2, io3 durations for the corresponding process. */
            
            int i;
            for (i = 0; i < MAX_LENGTH; i++) {
                fscanf(file, "%3s %d %d %d %d %d %d", label, &buffer[0], &buffer[1], &buffer[2], &buffer[3], &buffer[4], &buffer[5]);
                int i;
                switch (label[1]) {
                    case '1':
                        for (i = 0; i < 3; i++) {
                            t1_bursts[i] = buffer[i];
                        }
                        break;
                    case '2':
                        for (i = 0; i < 3; i++) {
                            t2_bursts[i] = buffer[i];
                        }
                        break;
                    case '3':
                        for (i = 0; i < 3; i++) {
                            t3_bursts[i] = buffer[i];
                        }
                        break;
                    case '4':
                        for (i = 0; i < 3; i++) {
                            t4_bursts[i] = buffer[i];
                        }
                        break;
                    case '5':
                        for (i = 0; i < 3; i++) {
                            t5_bursts[i] = buffer[i];
                        }
                        break;
                    case '6':
                        for (i = 0; i < 3; i++) {
                            t6_bursts[i] = buffer[i];
                        }
                        break;
                    case '7':
                        for (i = 0; i < 3; i++) {
                            t7_bursts[i] = buffer[i];
                        }
                        break;
                    default:
                        printf("ERROR: An unexpected error has occurred.\n");
                        return 1;
                }
            }
        }
    }
    else if (argc == 15) {
        printf("You entered 14 arguments.\n");
        printf("Using the arguments as t1_cpu, t2_cpu, ..., t7_cpu, t1_io, t2_io, ..., t7_io.\n");
        printf("Note that each of the 3 cpu bursts of t1 will have t1_cpu duration and so on.\n");
        printf("Similarly, each of the 3 io bursts of t1 will have t1_io duration and so on.\n");

        int i;
        for (i = 0; i < 3; i++) {
            // CPU bursts...
            t1_bursts[i] = atoi(argv[1]);
            t2_bursts[i] = atoi(argv[2]);
            t3_bursts[i] = atoi(argv[3]);
            t4_bursts[i] = atoi(argv[4]);
            t5_bursts[i] = atoi(argv[5]);
            t6_bursts[i] = atoi(argv[6]);
            t7_bursts[i] = atoi(argv[7]);

            // IO bursts...
            t1_bursts[i + 3] = atoi(argv[8]);
            t2_bursts[i + 3] = atoi(argv[9]);
            t3_bursts[i + 3] = atoi(argv[10]);
            t4_bursts[i + 3] = atoi(argv[11]);
            t5_bursts[i + 3] = atoi(argv[12]);
            t6_bursts[i + 3] = atoi(argv[13]);
            t7_bursts[i + 3] = atoi(argv[14]);
        }
    }
    else {
        printf("ERROR: Invalid arguments specified.\n");
        return 1;
    }

    printf("%d\n", t2_bursts[2]);

    initializeThread();
    signal(SIGALRM, runThread);
    alarm(SWITCH_INTERVAL);
    getcontext(&threadArray[0].context);

    while (1);
    return 0;
}
