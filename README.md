# EE442 Programming Assignment 2

Salih Mert Kucukakinci 2094290


Run with the following command:


```bash

$ ./pwf_scheduler input.txt

$ ./srtf_scheduler input.txt

```


or you can explicitly specify the path to the input file:


```bash

$ ./pwf_scheduler path/to/input.txt

$ ./srtf_scheduler path/to/input.txt

```


The format of the input.txt:


T1 6 4 2 4 5 3

T2 1 2 3 4 5 6

T3 2 4 5 1 2 5

T4 2 9 4 5 1 2

T5 2 8 3 2 3 1

T6 3 5 5 7 1 1

T7 3 2 3 1 1 1

X


Format: [T1] [t1_cpu1] [t1_io1] [t1_cpu2] [t1_io2]

        [T2] [t2_cpu1] [t2_io1] [t2_cpu2] [t2_io2]

         X


X indicates the end of the file.


To create the runnable:


```bash

$ gcc -o pwf_scheduler pwf_scheduler.c

$ gcc -o srtf_scheduler srtf_scheduler.c

```

Author: https://github.com/matt-mert

