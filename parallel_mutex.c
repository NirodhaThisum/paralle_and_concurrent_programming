// lab1_mutex_unsorted.c — CS4532 Lab 1: Parallel (one global mutex, unsorted list, pretty output + avg/std)
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>
#include <pthread.h>

/* ==========================================================
   Experiment Configuration (adjust here if needed)
   ========================================================== */

// Range of possible keys: values must be between 0 and 2^16 - 1
#define KEY_SPACE 65536

// n = initial linked list size
#define N 1000

// m = number of operations performed in each run
#define M 10000

// Number of repeated runs per (case × thread-count)
#define REPS 100

// Thread counts to test (per the lab’s table)
static const int THREADS_LIST[] = {1, 2, 4, 8};
enum { N_THREADS = sizeof(THREADS_LIST)/sizeof(THREADS_LIST[0]) };

// Cases: define the fractions of operations
typedef struct { const char* name; float fMem, fIns, fDel; } CaseCfg;
static const CaseCfg CASES[] = {
    {"Case 1", 0.99f, 0.005f, 0.005f}, // 99% Member, 0.5% Insert, 0.5% Delete
    {"Case 2", 0.90f, 0.05f , 0.05f }, // 90% Member, 5% Insert, 5% Delete
    {"Case 3", 0.50f, 0.25f , 0.25f }  // 50% Member, 25% Insert, 25% Delete
};
enum { NUM_CASES = sizeof(CASES)/sizeof(CASES[0]) };

/* ==========================================================
   Linked List Implementation (unsorted, insert-at-head)
   ========================================================== */
struct Node { int data; struct Node* next; };

static bool Member_serial(struct Node* head, int value) {
    for (struct Node* p = head; p; p = p->next)
        if (p->data == value) return true;
    return false;
}

static bool Insert_serial(struct Node** head, int value) {
    if (Member_serial(*head, value)) return false;  // enforce uniqueness
    struct Node* n = (struct Node*)malloc(sizeof(struct Node));
    if (!n) { perror("malloc"); exit(1); }
    n->data = value;
    n->next = *head;   // insert at head (unsorted)
    *head = n;
    return true;
}

static bool Delete_serial(struct Node** head, int value) {
    struct Node* cur = *head;
    struct Node* prev = NULL;
    while (cur) {
        if (cur->data == value) {
            if (prev) prev->next = cur->next; else *head = cur->next;
            free(cur);
            return true;
        }
        prev = cur; cur = cur->next;
    }
    return false;
}

static void FreeList(struct Node* head) {
    while (head) { struct Node* nxt = head->next; free(head); head = nxt; }
}

/* ==========================================================
   Utilities
   ========================================================== */
static inline int rand_key(void) { return rand() % KEY_SPACE; }

typedef enum { OP_MEMBER = 0, OP_INSERT = 1, OP_DELETE = 2 } OpType;

/* Pre-generate operation types, then shuffle them */
static void make_ops(OpType* ops, int m, float fMem, float fIns, float fDel) {
    int Mmem = (int)lroundf(m * fMem);
    int Mins = (int)lroundf(m * fIns);
    if (Mmem > m) Mmem = m;
    if (Mins > m - Mmem) Mins = m - Mmem;
    int Mdel = m - Mmem - Mins;

    int idx = 0;
    for (int i = 0; i < Mmem; ++i) ops[idx++] = OP_MEMBER;
    for (int i = 0; i < Mins;  ++i) ops[idx++] = OP_INSERT;
    for (int i = 0; i < Mdel;  ++i) ops[idx++] = OP_DELETE;

    for (int i = m - 1; i > 0; --i) {
        int j = rand() % (i + 1);
        OpType t = ops[i]; ops[i] = ops[j]; ops[j] = t;
    }
}

/* Build initial list of N unique random keys */
static void build_initial(struct Node** head) {
    int made = 0;
    while (made < N) {
        int v = rand_key();
        if (Insert_serial(head, v)) made++;  // Insert enforces uniqueness
    }
}

/* Timing helper: milliseconds using clock() (thread creation excluded) */
static double now_ms(void) {
    return (double)clock() * 1000.0 / CLOCKS_PER_SEC;
}

/* ==========================================================
   Worker threads (one global mutex guards the whole list)
   ========================================================== */
typedef struct {
    // Shared:
    struct Node** headp;
    pthread_mutex_t* list_mutex;
    const OpType* ops;      // full op array
    int start_index;        // inclusive
    int end_index;          // exclusive
    pthread_barrier_t* start_barrier;

    // Per-thread counters:
    unsigned long member_hits;
    unsigned long insert_ok;
    unsigned long delete_ok;
} WorkerArgs;

static bool Member_locked(struct Node** headp, int value, pthread_mutex_t* mtx) {
    pthread_mutex_lock(mtx);
    bool r = Member_serial(*headp, value);
    pthread_mutex_unlock(mtx);
    return r;
}
static bool Insert_locked(struct Node** headp, int value, pthread_mutex_t* mtx) {
    pthread_mutex_lock(mtx);
    bool r = Insert_serial(headp, value);
    pthread_mutex_unlock(mtx);
    return r;
}
static bool Delete_locked(struct Node** headp, int value, pthread_mutex_t* mtx) {
    pthread_mutex_lock(mtx);
    bool r = Delete_serial(headp, value);
    pthread_mutex_unlock(mtx);
    return r;
}

static void* worker_fn(void* arg) {
    WorkerArgs* a = (WorkerArgs*)arg;
    // Wait for the synchronized start signal
    pthread_barrier_wait(a->start_barrier);

    for (int i = a->start_index; i < a->end_index; ++i) {
        OpType t = a->ops[i];

        if (t == OP_MEMBER) {
            int key = rand_key();
            if (Member_locked(a->headp, key, a->list_mutex)) a->member_hits++;
        } else if (t == OP_INSERT) {
            // draw until we get a key not present (do membership checks with the mutex)
            int key = rand_key();
            while (Member_locked(a->headp, key, a->list_mutex)) key = rand_key();
            if (Insert_locked(a->headp, key, a->list_mutex)) a->insert_ok++;
        } else { // OP_DELETE
            int key = rand_key();
            if (Delete_locked(a->headp, key, a->list_mutex)) a->delete_ok++;
        }
    }
    return NULL;
}

/* ==========================================================
   Main Experiment Driver
   ========================================================== */
int main(void) {
    srand((unsigned)time(NULL));

    for (int ci = 0; ci < NUM_CASES; ++ci) {
        const CaseCfg* C = &CASES[ci];

        printf("\n=====================================================\n");
        printf("  %s\n", C->name);
        printf("  Initial linked list size (n) = %d\n", N);
        printf("  Number of operations (m)     = %d\n", M);
        printf("  Number of runs (REPS)        = %d\n", REPS);
        printf("  Mix: Member=%.3f, Insert=%.3f, Delete=%.3f\n",
               C->fMem, C->fIns, C->fDel);
        // printf("=====================================================\n");

        for (int ti = 0; ti < N_THREADS; ++ti) {
            int T = THREADS_LIST[ti];
            double times[REPS];

            printf("[ Threads = %d ]\n", T);
            // printf("%-8s %-6s %-12s %-12s %-12s %-12s\n",
            //        "Algo", "Run", "Elapsed(ms)", "MemberHits", "InsertOK", "DeleteOK");

            for (int r = 0; r < REPS; ++r) {
                // 1) Build initial list (NOT timed)
                struct Node* head = NULL;
                build_initial(&head);

                // 2) Prepare shuffled operation sequence (NOT timed)
                OpType* ops = (OpType*)malloc(M * sizeof(OpType));
                if (!ops) { perror("malloc ops"); exit(1); }
                make_ops(ops, M, C->fMem, C->fIns, C->fDel);

                // 3) Create mutex and barrier, spawn T workers
                pthread_mutex_t list_mutex;
                pthread_mutex_init(&list_mutex, NULL);

                pthread_barrier_t start_barrier;
                pthread_barrier_init(&start_barrier, NULL, (unsigned)T + 1);

                pthread_t* th = (pthread_t*)malloc(T * sizeof(pthread_t));
                WorkerArgs* args = (WorkerArgs*)calloc(T, sizeof(WorkerArgs));
                if (!th || !args) { perror("alloc threads"); exit(1); }

                // Partition the M ops among T threads
                int base = 0;
                for (int t = 0; t < T; ++t) {
                    int len = M / T + (t < (M % T) ? 1 : 0);
                    args[t].headp = &head;
                    args[t].list_mutex = &list_mutex;
                    args[t].ops = ops;
                    args[t].start_index = base;
                    args[t].end_index = base + len;
                    args[t].start_barrier = &start_barrier;
                    args[t].member_hits = args[t].insert_ok = args[t].delete_ok = 0;
                    base += len;
                    pthread_create(&th[t], NULL, worker_fn, &args[t]);
                }

                // 4) Time exactly the m operations
                double t0 = now_ms();
                pthread_barrier_wait(&start_barrier);   // release workers simultaneously
                unsigned long mh = 0, ins_ok = 0, del_ok = 0;
                for (int t = 0; t < T; ++t) {
                    pthread_join(th[t], NULL);
                    mh     += args[t].member_hits;
                    ins_ok += args[t].insert_ok;
                    del_ok += args[t].delete_ok;
                }
                double t1 = now_ms();
                double elapsed = t1 - t0;
                times[r] = elapsed;

                // printf("%-8s %-6d %-12.3f %-12lu %-12lu %-12lu\n",
                //        "mutex", r + 1, elapsed, mh, ins_ok, del_ok);

                // 5) Cleanup for this run
                pthread_barrier_destroy(&start_barrier);
                pthread_mutex_destroy(&list_mutex);
                free(args);
                free(th);
                free(ops);
                FreeList(head);
            }

            // 6) Summary stats for this (case × T)
            double sum = 0.0;
            for (int i = 0; i < REPS; ++i) sum += times[i];
            double mean = sum / REPS;

            double var = 0.0;
            for (int i = 0; i < REPS; ++i) {
                double d = times[i] - mean; var += d * d;
            }
            double std = (REPS > 1) ? sqrt(var / (REPS - 1)) : 0.0;

            printf("-----------------------------------------------------\n");
            printf("Summary (mutex, T=%d): Average = %.3f ms, Std = %.3f ms over %d runs\n\n",
                   T, mean, std, REPS);
        }
    }

    return 0;
}
