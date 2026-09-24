// Compile: gcc -O2 -pthread -o store_buffering store_buffering.c
// Run:     ./store_buffering
//
//   Thread 1          Thread 2
//   --------          --------
//   x = 1             y = 1
//   r1 = y            r2 = x
//
// SC-forbidden but TSO-allowed outcome: r1 == 0 && r2 == 0
//

#include <stdio.h>
#include <pthread.h>

#define ITERATIONS 500000

static volatile int x, y;
static volatile int r1, r2;

static void *thread1_fn(void *arg) {
    x = 1;
    r1 = y;
    return NULL;
}

static void *thread2_fn(void *arg) {
    y = 1;
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

        printf("iterations: %d/%d\n", i, ITERATIONS);
    }

    printf("Store buffering (r1==0 && r2==0): %d / %d\n",
           reorder_count, ITERATIONS);
    return 0;
}
