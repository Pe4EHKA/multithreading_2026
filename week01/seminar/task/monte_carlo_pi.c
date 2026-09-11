#include <err.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NUM_THREADS   8
#define POINTS_EACH   10000000


// TODO: types?
static unsigned int seed;
static long long hits;

static long long results[NUM_THREADS];


static void *worker(void *arg) {
    int id = *(int *) arg;
    // TODO: implement me
    return NULL;
}


int main(void) {
    pthread_t threads[NUM_THREADS];
    int ids[NUM_THREADS];
    long long total_hits = 0;

    // TODO: create threads

    // TODO: accumulate results

    long long total_points = (long long) NUM_THREADS * POINTS_EACH;
    double pi_estimate = 4.0 * (double) total_hits / (double) total_points;

    printf("\ntotal points : %lld\n", total_points);
    printf("total hits   : %lld\n", total_hits);
    printf("pi estimate  : %.6f\n", pi_estimate);
    printf("pi actual    : %.6f\n", M_PI);
    printf("error        : %.6f%%\n", fabs(pi_estimate - M_PI) / M_PI * 100.0);

    return 0;
}
