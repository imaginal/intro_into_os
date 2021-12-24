#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#define EOK 0

#define TABLE_SEATS 5

typedef struct {
    int id;
    pthread_t tid;
    pthread_mutex_t* left_spoon;
    pthread_mutex_t* right_spoon;
    enum STATE {
        HUNGRY,
        EATING,
        FULL
    } state;
} philosopher_t;

double microtime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1e6;
}

void *thread(void *arg)
{
    pthread_t tid = pthread_self();
    philosopher_t* ph = (philosopher_t*)arg;
    int err, us;

    fprintf(stderr, " start thread(%ld) ph(%d)\n", tid, ph->id);

    while (ph->state != FULL) {

        err = pthread_mutex_trylock(ph->left_spoon);
        if (err != EOK) {
            us = rand() % 1000;
            fprintf(stderr, "  ph(%d) fail get left spoon err(%d) wait(%d)\n", ph->id, err, us);
            usleep(us);
            continue;
        }
        fprintf(stderr, "  ph(%d) got left spoon\n", ph->id);

        err = pthread_mutex_trylock(ph->right_spoon);
        if (err != EOK) {
            pthread_mutex_unlock(ph->left_spoon);
            us = rand() % 1000;
            fprintf(stderr, "  ph(%d) fail get right spoon err(%d) wait(%d)\n", ph->id, err, us);
            usleep(us);
            continue;
        }
        fprintf(stderr, "  ph(%d) got right spoon\n", ph->id);

        printf("  ph(%d) start eating\n", ph->id);
        ph->state = EATING;
        sleep(1);
        ph->state = FULL;
        printf("  ph(%d) end eating\n", ph->id);

        pthread_mutex_unlock(ph->left_spoon);
        pthread_mutex_unlock(ph->right_spoon);
    }

    fprintf(stderr, " stop thread(%ld) ph(%d)\n", tid, ph->id);
}

int main()
{
    pthread_mutex_t spoon[TABLE_SEATS];
    philosopher_t ph[TABLE_SEATS];
    double start;
    int err;

    srand(time(NULL));

    for (int i=0; i<TABLE_SEATS; i++) {
        int r = (i + 1) % TABLE_SEATS;
        pthread_mutex_init(&spoon[i], NULL);
        ph[i].left_spoon = (&spoon[i]);
        ph[i].right_spoon = (&spoon[r]);
        ph[i].state = HUNGRY;
        ph[i].id = i;
        printf("phil %d sppon %d %d\n", i, i, r);
    }

    start = microtime();

    for (int i=0; i<TABLE_SEATS; i++) {
        err = pthread_create(&ph[i].tid, NULL, &thread, &ph[i]);
        fprintf(stderr, "phil %d start thread(%ld) err(%d)\n", i, ph[i].tid, err);
    }

    usleep(100);

    for (int i=0; i<TABLE_SEATS; i++) {
        err = pthread_join(ph[i].tid, NULL);
        fprintf(stderr, "phil %d join thread(%ld) err(%d)\n", i, ph[i].tid, err);
    }

    printf("\ntime %1.3f\n", microtime()-start);

    return 0;
}
