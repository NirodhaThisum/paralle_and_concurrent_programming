# CS4532 – Lab 1 (Serial, Mutex, RW-Lock)

**Group Members:**  
- Moraes M. A. R. – 210390F  
- Thisum J. A. D. N. – 210651H  

**Repository:** [parallel_and_concurrent_programming](https://github.com/NirodhaThisum/parallel_and_concurrent_programming)  
**Date:** September 4, 2025  

---

## 🔹 Compilation Instructions  

We are using **GCC** with the **MinGW-w64** toolchain on Windows.  

1. Open the terminal/command prompt in the folder containing the `.c` files.  
2. Run the following commands to build the executables:  

```bat
gcc serial.c -o serial.exe
gcc parallel_mutex.c -o parallel_mutex.exe -lpthread
gcc parallel_single_rw.c -o parallel_single_rw.exe -lpthread
```

Note: The `-lpthread` option is required for programs that use **Pthreads** (mutex and rwlock).  

---

## 🔹 Running the Programs  

After compilation, run the programs by typing their names in the terminal:  

```bat
serial.exe
parallel_mutex.exe
parallel_single_rw.exe
```

Each program will:  
- Run the linked list operations for all configured **cases** (Case 1, Case 2, Case 3).  
- Show execution results for each run.  
- Print a summary with **average execution time** and **standard deviation**.  

---

## 🔹 Configuration (Changing Experiment Parameters)  

At the top of each `.c` file, you can change the constants to adjust the experiment.  

- **`KEY_SPACE`** → Range of possible keys (default: `65536`)  
- **`N`** → Initial linked list size before operations begin (default: `1000`)  
- **`M`** → Number of operations per run (default: `10000`)  
- **`REPS`** → How many times each case is repeated to calculate averages (default: `100`)  
- **`THREADS_LIST[]`** → Thread counts tested in parallel programs (default: `{1, 2, 4, 8}`)  
- **`CASES[]`** → Defines workload mixes for Member, Insert, and Delete operations  

### Example cases in the code:
- **Case 1:** 99% Member, 0.5% Insert, 0.5% Delete  
- **Case 2:** 90% Member, 5% Insert, 5% Delete  
- **Case 3:** 50% Member, 25% Insert, 25% Delete  

---

With this setup:  
- Run **serial.exe** to see baseline results.  
- Run **parallel_mutex.exe** to see results with a global mutex.  
- Run **parallel_single_rw.exe** to see results with a global read–write lock.  
