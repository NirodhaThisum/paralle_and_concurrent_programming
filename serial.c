// lab1_serial_unsorted.c — CS4532 Lab 1: Serial (unsorted list, readable output + avg/std)
// Compile: gcc -O2 lab1_serial_unsorted.c -o lab1_serial_unsorted
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>

/* ==========================================================
   Experiment Configuration (adjust here if needed)
   ========================================================== */

// Range of possible keys: values must be between 0 and 2^16 - 1
#define KEY_SPACE 65536

// n = initial linked list size
#define N 1000

// m = number of operations performed in each run
#define M 10000

// Number of repeated runs per case (used to compute Avg & Std)
#define REPS 100

// Cases: define the fractions of operations
// Each case specifies the percentage of Member, Insert, Delete operations
typedef struct { const char* name; float fMem, fIns, fDel; } CaseCfg;
static const CaseCfg CASES[] = {
    {"Case 1", 0.99f, 0.005f, 0.005f}, // 99% Member, 0.5% Insert, 0.5% Delete
    {"Case 2", 0.90f, 0.05f , 0.05f }, // 90% Member, 5% Insert, 5% Delete
    {"Case 3", 0.50f, 0.25f , 0.25f }  // 50% Member, 25% Insert, 25% Delete
};

/* ==========================================================
   Linked List Implementation (unsorted, insert-at-head)
   ========================================================== */
struct Node { int data; struct Node* next; };

static bool Member(struct Node* head, int value) {
    for (struct Node* p = head; p; p = p->next)
        if (p->data == value) return true;
    return false;
}

static bool Insert(struct Node** head, int value) {
    if (Member(*head, value)) return false;  // enforce uniqueness
    struct Node* n = (struct Node*)malloc(sizeof(struct Node));
    if (!n) { perror("malloc"); exit(1); }
    n->data = value;
    n->next = *head;   // insert at head (unsorted)
    *head = n;
    return true;
}

static bool Delete(struct Node** head, int value) {
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
        if (Insert(head, v)) made++;  // Insert enforces uniqueness
    }
}

/* Timing helper: milliseconds using clock() */
static double now_ms(void) {
    return (double)clock() * 1000.0 / CLOCKS_PER_SEC;
}

/* ==========================================================
   Main Experiment Driver
   ========================================================== */
int main(void) {
    srand((unsigned)time(NULL));

    const int NUM_CASES = (int)(sizeof(CASES) / sizeof(CASES[0]));
    for (int ci = 0; ci < NUM_CASES; ++ci) {
        const CaseCfg* C = &CASES[ci];
        double times[REPS];

        printf("\n=====================================================\n");
        printf("  %s\n", C->name);
        printf("  Initial linked list size (n) = %d\n", N);
        printf("  Number of operations (m)     = %d\n", M);
        printf("  Number of runs (REPS)        = %d\n", REPS);
        printf("  Mix: Member=%.3f, Insert=%.3f, Delete=%.3f\n",
               C->fMem, C->fIns, C->fDel);
        // printf("=====================================================\n");
        // printf("%-8s %-5s %-12s %-12s %-12s %-12s\n",
        //        "Algo", "Run", "Elapsed(ms)", "MemberHits", "InsertOK", "DeleteOK");

        for (int r = 0; r < REPS; ++r) {
            /* 1) Build initial (NOT timed) */
            struct Node* head = NULL;
            build_initial(&head);

            /* 2) Prepare shuffled operation sequence */
            OpType* ops = (OpType*)malloc(M * sizeof(OpType));
            if (!ops) { perror("malloc ops"); exit(1); }
            make_ops(ops, M, C->fMem, C->fIns, C->fDel);

            /* 3) Execute exactly M operations (timed) */
            unsigned long mh = 0, ins_ok = 0, del_ok = 0;
            double t0 = now_ms();
            for (int i = 0; i < M; ++i) {
                OpType t = ops[i];
                int key = rand_key();
                if (t == OP_MEMBER) {
                    if (Member(head, key)) mh++;
                } else if (t == OP_INSERT) {
                    // keep drawing until we get a key not present
                    while (Member(head, key)) key = rand_key();
                    if (Insert(&head, key)) ins_ok++;
                } else { // OP_DELETE
                    if (Delete(&head, key)) del_ok++;
                }
            }
            double t1 = now_ms();
            double elapsed = t1 - t0;
            times[r] = elapsed;

            // printf("%-8s %-5d %-12.3f %-12lu %-12lu %-12lu\n",
            //        "serial", r + 1, elapsed, mh, ins_ok, del_ok);

            free(ops);
            FreeList(head);
        }

        /* 4) Average and (sample) StdDev */
        double sum = 0.0;
        for (int i = 0; i < REPS; ++i) sum += times[i];
        double mean = sum / REPS;

        double var = 0.0;
        for (int i = 0; i < REPS; ++i) {
            double d = times[i] - mean; var += d * d;
        }
        double std = (REPS > 1) ? sqrt(var / (REPS - 1)) : 0.0;

        printf("-----------------------------------------------------\n");
        printf("Summary (serial): Average = %.3f ms, Std = %.3f ms over %d runs\n",
               mean, std, REPS);
    }

    return 0;
}
