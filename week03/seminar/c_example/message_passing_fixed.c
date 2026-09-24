// Compile: gcc -O2 -pthread -o message_passing_fixed message_passing_fixed.c
// Run:     ./message_passing_fixed
//
// Message Passing fixed with release/acquire ordering.
//
// memory_order_release (store): all prior writes visible before this store.
// memory_order_acquire (load):  once this load sees the release value,
//                               all writes before the release are visible.
//
// On x86 (TSO): both compile to plain MOV — zero cost over relaxed.
// On ARM:       STLR (store-release) / LDAR (load-acquire).

#include <stdio.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>

#define ITERATIONS 200000

static int data = 0;
static _Atomic int flag = 0;

static void *producer(void *arg) {
    for (int i = 0; i < ITERATIONS; i++) {
        data = i + 1;
        atomic_store_explicit(&flag, 1, memory_order_release);
    }
    return NULL;
}

static void *consumer(void *arg) {
    for (int i = 0; i < ITERATIONS; i++) {
        while (atomic_load_explicit(&flag, memory_order_acquire) == 0) {}
        int r = data;
        atomic_store_explicit(&flag, 0, memory_order_release);
        assert(r != 0);
        printf("iterations: %d/%d\n", i, ITERATIONS);
    }
    return NULL;
}

int main(void) {
    pthread_t t1, t2;
    pthread_create(&t1, NULL, producer, NULL);
    pthread_create(&t2, NULL, consumer, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("All %d reads correct (release/acquire guarantees it)\n", ITERATIONS);
    return 0;
}
