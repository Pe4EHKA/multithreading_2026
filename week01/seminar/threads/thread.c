#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <err.h>

#define NUM_THREADS 10
#define SPIN_SECONDS 20

static void *spin(void *arg) {
    int id = *(int *) arg;
    time_t end = time(NULL) + SPIN_SECONDS;

    printf("[thread %2d] spinning until %lds from now\n", id, (long) SPIN_SECONDS);

    while (time(NULL) < end); /* busy-wait */

    printf("[thread %2d] done spinning\n", id);
    return NULL;
}

int main(void) {
    pthread_t threads[NUM_THREADS];
    int ids[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++) {
        ids[i] = i;
        if (pthread_create(&threads[i], NULL, spin, &ids[i]) != 0)
            err(EXIT_FAILURE, "pthread_create");
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_join(threads[i], NULL) != 0)
            err(EXIT_FAILURE, "pthread_join");
        printf("[main] joined thread %2d\n", i);
    }

    printf("[main] done\n");
    return 0;
}
