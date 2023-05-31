# EE442 Programming Assignment 2

Salih Mert Kucukakinci 2094290


Run with the following command:


```bash

$ ./pwf_scheduler input.txt

```


or with the following command:


```bash

$ ./pwf_scheduler path/to/input.txt

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


In this format, first string is label, the next 3 integers are cpu1, cpu2, cpu3 durations and the last 3 integers are io1, io2, io3 durations for the corresponding process and X stops reading.


To create the runnable:


```bash

$ gcc -o pwf_scheduler pwf_scheduler.c

```

https://github.com/matt-mert

