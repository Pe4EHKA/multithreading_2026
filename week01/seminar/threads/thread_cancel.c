#include <err.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


static void cleanup(void *arg)
{
    printf("[cleanup] '%s' — releasing resources\n", (char *)arg);
}


static void *deferred_thread(void *arg)
{
    (void)arg;

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    pthread_cleanup_push(cleanup, "deferred thread");

    long i = 0;
    while (1) {
        i++;
        // explicit cancellation point
        pthread_testcancel();
    }

    pthread_cleanup_pop(1);
    return NULL;
}


static void *async_thread(void *arg)
{
    (void)arg;

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    volatile long i = 0;
    while (1) {
        i++;
        // cancel request cant be processed without usleep
//        usleep(1);
    }
    return NULL;
}


int main(void)
{
    pthread_t t_deferred, t_async;

    if (pthread_create(&t_deferred, NULL, deferred_thread, NULL) != 0)
        err(EXIT_FAILURE, "pthread_create deferred");
    if (pthread_create(&t_async, NULL, async_thread, NULL) != 0)
        err(EXIT_FAILURE, "pthread_create async");

    sleep(1);

    pthread_cancel(t_deferred);
    pthread_cancel(t_async);

    void *retval;

    pthread_join(t_deferred, &retval);
    printf("[main] deferred thread joined — retval=%s\n",
           retval == PTHREAD_CANCELED ? "PTHREAD_CANCELED" : "normal");

    pthread_join(t_async, &retval);
    printf("[main] async thread joined    — retval=%s\n",
           retval == PTHREAD_CANCELED ? "PTHREAD_CANCELED" : "normal");

    printf("[main] done\n");
    return 0;
}
