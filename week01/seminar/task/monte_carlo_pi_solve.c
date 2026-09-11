#include <err.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NUM_THREADS   8
#define POINTS_EACH   10000000


static _Thread_local unsigned int tls_seed;
static _Thread_local long long tls_hits;


static long long results[NUM_THREADS];


static void *worker(void *arg) {
    int id = *(int *) arg;

    tls_seed = (unsigned int) (time(NULL)) ^ ((unsigned int) id * 2654435761u);
    tls_hits = 0;

    printf("[thread %d] starting %d points, seed=%u\n",
           id, POINTS_EACH, tls_seed);

    for (int i = 0; i < POINTS_EACH; i++) {
        double x = (double) rand_r(&tls_seed) / RAND_MAX;
        double y = (double) rand_r(&tls_seed) / RAND_MAX;
        if (x * x + y * y <= 1.0)
            tls_hits++;
    }

    results[id] = tls_hits;

    printf("[thread %d] done: %lld / %d hits\n", id, tls_hits, POINTS_EACH);
    return NULL;
}


int main(void) {
    pthread_t threads[NUM_THREADS];
    int ids[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++) {
        ids[i] = i;
        if (pthread_create(&threads[i], NULL, worker, &ids[i]) != 0)
            err(EXIT_FAILURE, "pthread_create");
    }

    for (int i = 0; i < NUM_THREADS; i++)
        if (pthread_join(threads[i], NULL) != 0)
            err(EXIT_FAILURE, "pthread_join");

    long long total_hits = 0;
    for (int i = 0; i < NUM_THREADS; i++)
        total_hits += results[i];

    long long total_points = (long long) NUM_THREADS * POINTS_EACH;
    double pi_estimate = 4.0 * (double) total_hits / (double) total_points;

    printf("\ntotal points : %lld\n", total_points);
    printf("total hits   : %lld\n", total_hits);
    printf("pi estimate  : %.6f\n", pi_estimate);
    printf("pi actual    : %.6f\n", M_PI);
    printf("error        : %.6f%%\n", fabs(pi_estimate - M_PI) / M_PI * 100.0);

    return 0;
}
