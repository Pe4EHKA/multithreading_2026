#include <err.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUM_WORKERS 4

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static volatile int stop = 0;
static long long counter = 0;


static void *worker(void *arg) {
    int id = *(int *) arg;
    printf("[worker %d] started (tid=%lu)\n", id, (unsigned long) pthread_self());

    while (1) {
        pthread_mutex_lock(&mtx);

        while (!stop) {
            counter++;
            if (counter % 10000000 == 0)
                printf("[worker %d] counter = %lld\n", id, counter);
            pthread_mutex_unlock(&mtx);
            sched_yield();
            pthread_mutex_lock(&mtx);
        }

        pthread_mutex_unlock(&mtx);
        break;
    }

    return NULL;
}


static void *signal_thread(void *arg) {
    sigset_t *set = arg;
    int sig;

    printf("[signal thread] waiting for SIGINT or SIGTERM...\n");

    if (sigwait(set, &sig) != 0)
        err(EXIT_FAILURE, "sigwait");

    printf("\n[signal thread] caught signal %d (%s) — initiating shutdown\n",
           sig, sig == SIGINT ? "SIGINT" : "SIGTERM");

    /* tell workers to stop */
    pthread_mutex_lock(&mtx);
    stop = 1;
    pthread_mutex_unlock(&mtx);
    pthread_cond_broadcast(&cond);   /* wake any condvar waiters */

    return NULL;
}


int main(void) {
    // block SIGINT and SIGTERM in the main thread before any
    // pthread_create call. All threads will inherit this mask
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    if (pthread_sigmask(SIG_BLOCK, &set, NULL) != 0)
        err(EXIT_FAILURE, "pthread_sigmask");

    pthread_t sig_tid;
    if (pthread_create(&sig_tid, NULL, signal_thread, &set) != 0)
        err(EXIT_FAILURE, "pthread_create (signal thread)");

    pthread_t workers[NUM_WORKERS];
    int ids[NUM_WORKERS];
    for (int i = 0; i < NUM_WORKERS; i++) {
        ids[i] = i;
        if (pthread_create(&workers[i], NULL, worker, &ids[i]) != 0)
            err(EXIT_FAILURE, "pthread_create (worker)");
    }

    printf("[main] %d workers running. Press Ctrl-C to stop.\n", NUM_WORKERS);

    pthread_join(sig_tid, NULL);

    for (int i = 0; i < NUM_WORKERS; i++)
        pthread_join(workers[i], NULL);

    printf("[main] all threads joined. final counter = %lld\n", counter);
    return 0;
}
