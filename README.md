# CS4532 – Lab 1 (Serial, Mutex, RW-Lock)
Group Members:  Moraes M. A. R. - 210390F
                Thisum J. A. D. N. - 210651H

Repo:  https://github.com/NirodhaThisum/parallel_and_concurrent_programming
Date: September 4, 2025


## Compilation

Windows (MinGW-w64)

gcc serial.c -o serial.exe
gcc parallel_mutex.c -o parallel_mutex.exe -lpthread
gcc parallel_single_rw.c -o parallel_single_rw.exe -lpthread


Run them with:

serial.exe
parallel_mutex.exe
parallel_single_rw.exe




## Configuration

At the top of each `.c` file you can adjust:

- `KEY_SPACE` → range of possible keys (default: 65536)  
- `N` → initial linked list size (default: 1000)  
- `M` → number of operations per run (default: 10000)  
- `REPS` → number of repetitions per case × thread-count  
- `THREADS_LIST[]` → thread counts to test (default: {1,2,4,8})  
- `CASES[]` → workload mixes (`fMem`, `fIns`, `fDel`)  

Example cases provided:
- Case 1: 99% Member, 0.5% Insert, 0.5% Delete  
- Case 2: 90% Member, 5% Insert, 5% Delete  
- Case 3: 50% Member, 25% Insert, 25% Delete  
