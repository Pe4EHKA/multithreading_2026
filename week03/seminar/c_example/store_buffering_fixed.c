// Compile: gcc -O2 -pthread -o store_buffering_fixed store_buffering_fixed.c
// Run:     ./store_buffering_fixed
//
// Fix for TSO Store Buffering: insert a full memory fence between store and load.
// atomic_thread_fence(memory_order_seq_cst) is portable C11 (MFENCE on x86, DMB on ARM).

#include <stdio.h>
#include <pthread.h>
#include <stdatomic.h>

#define ITERATIONS 500000

static volatile int x, y;
static volatile int r1, r2;

static void *thread1_fn(void *arg) {
    x = 1;
    atomic_thread_fence(memory_order_seq_cst);
    r1 = y;
    return NULL;
}

static void *thread2_fn(void *arg) {
    y = 1;
    atomic_thread_fence(memory_order_seq_cst);
    r2 = x;
    return NULL;
}

int main(void) {
    int reorder_count = 0;

    for (int i = 0; i < ITERATIONS; i++) {
        x = 0;
        y = 0;

        pthread_t t1, t2;
        pthread_create(&t1, NULL, thread1_fn, NULL);
        pthread_create(&t2, NULL, thread2_fn, NULL);
        pthread_join(t1, NULL);
        pthread_join(t2, NULL);

        if (r1 == 0 && r2 == 0) {
            reorder_count++;
        }
    }

    printf("Store buffering (r1==0 && r2==0): %d / %d\n",
           reorder_count, ITERATIONS);
    printf("With full fence, result should always be 0\n");
    return 0;
}
