// Compile: gcc -O2 -pthread -o message_passing_relaxed message_passing_relaxed.c
// Run:     ./message_passing_relaxed
//
// Message Passing test with relaxed atomics.
//
//   Thread 1 (producer)             Thread 2 (consumer)
//   --------------------            -------------------
//   data = 42;                      while (flag == 0) {}
//   flag.store(1, relaxed);         r = data;  // may be 0 !

// On x86: CPU reordering unlikely (TSO), but compiler is free to reorder
// On ARM: both compiler AND CPU can reorder

#include <stdio.h>
#include <pthread.h>
#include <stdatomic.h>

#define ITERATIONS 2000000

static int data = 0;
static _Atomic int flag = 0;
static int wrong_reads = 0;

static void *producer(void *arg) {
    for (int i = 0; i < ITERATIONS; i++) {
        data = i + 1;
        atomic_store_explicit(&flag, 1, memory_order_relaxed);
    }
    return NULL;
}

static void *consumer(void *arg) {
    for (int i = 0; i < ITERATIONS; i++) {
        while (atomic_load_explicit(&flag, memory_order_relaxed) == 0) {}
        int r = data;
        atomic_store_explicit(&flag, 0, memory_order_relaxed);
        printf("iterations: %d/%d\n", i, ITERATIONS);
        if (r == 0) wrong_reads++;
    }
    return NULL;
}

int main(void) {
    pthread_t t1, t2;
    pthread_create(&t1, NULL, producer, NULL);
    pthread_create(&t2, NULL, consumer, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("Wrong reads (data==0 after flag seen): %d / %d\n",
           wrong_reads, ITERATIONS);
    printf("On x86: likely 0 (TSO). On ARM: expect > 0\n");
    return 0;
}
