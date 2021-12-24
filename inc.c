#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#define EOK 0


typedef struct {
    pthread_mutex_t mutex;
    int cnt;
} cnt_t;

double microtime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1e6;
}

void *thread_nolock(void *arg)
{
    pthread_t tid = pthread_self();
    double ts = microtime();
    cnt_t* pc = (cnt_t*)arg;

    printf(" enter thread(%ld) arg(%d)\n", tid, pc->cnt);

    for (int n=0; n<1e8; n++) {
        pc->cnt = pc->cnt + 1;
    }

    ts = microtime() - ts;
    printf(" leave thread(%ld) res(%d) time(%1.3f)\n", tid, pc->cnt, ts);
}

int test_nolock()
{
    pthread_t t1, t2;
    double ts = microtime();
    int err;
    cnt_t cnt;

    cnt.cnt = 0;

    printf("test_nolock\n");
    printf("===========\n");

    err = pthread_create(&t1, NULL, &thread_nolock, &cnt);
    printf("create thread(%ld) err(%d)\n", t1, err);

    err = pthread_create(&t2, NULL, &thread_nolock, &cnt);
    printf("create thread(%ld) err(%d)\n", t2, err);

    err = pthread_join(t1, NULL);
    printf("join thread(%ld) err(%d)\n", t1, err);

    err = pthread_join(t2, NULL);
    printf("join thread(%ld) err(%d)\n", t2, err);

    ts = microtime() - ts;
    printf("\ncounter %d\n", cnt.cnt);
    printf("time %1.3f\n\n", ts);
    return 0;
}

void *thread_with_mutex(void *arg)
{
    pthread_t tid = pthread_self();
    double ts = microtime();
    cnt_t* pc = (cnt_t*)arg;

    printf(" enter thread(%ld) arg(%d)\n", tid, pc->cnt);

    for (int n=0; n<1e8; n++) {
        pthread_mutex_lock(&(pc->mutex));
        pc->cnt = pc->cnt + 1;
        pthread_mutex_unlock(&(pc->mutex));
    }

    ts = microtime() - ts;
    printf(" leave thread(%ld) res(%d) time(%1.3f)\n", tid, pc->cnt, ts);
}

int test_with_mutex()
{
    pthread_t t1, t2;
    double ts = microtime();
    int err;
    cnt_t cnt;

    printf("test_with_mutex\n");
    printf("===============\n");

    pthread_mutex_init(&cnt.mutex, NULL);
    cnt.cnt = 0;

    err = pthread_create(&t1, NULL, &thread_with_mutex, &cnt);
    printf("create thread(%ld) err(%d)\n", t1, err);

    err = pthread_create(&t2, NULL, &thread_with_mutex, &cnt);
    printf("create thread(%ld) err(%d)\n", t2, err);

    err = pthread_join(t1, NULL);
    printf("join thread(%ld) err(%d)\n", t1, err);

    err = pthread_join(t2, NULL);
    printf("join thread(%ld) err(%d)\n", t2, err);

    ts = microtime() - ts;
    printf("\ncounter %d\n", cnt.cnt);
    printf("time %1.3f\n\n", ts);
    return 0;
}


void *thread_with_atomic(void *arg)
{
    pthread_t tid = pthread_self();
    double ts = microtime();
    cnt_t* pc = (cnt_t*)arg;

    printf(" enter thread(%ld) arg(%d)\n", tid, pc->cnt);

    for (int n=0; n<1e8; n++) {
        __atomic_fetch_add(&(pc->cnt), 1, __ATOMIC_SEQ_CST);
    }

    ts = microtime() - ts;
    printf(" leave thread(%ld) res(%d) time(%1.3f)\n", tid, pc->cnt, ts);
}

int test_with_atomic()
{
    pthread_t t1, t2;
    double ts = microtime();
    int err;
    cnt_t cnt;

    printf("test_with_atomic\n");
    printf("================\n");

    cnt.cnt = 0;

    err = pthread_create(&t1, NULL, &thread_with_atomic, &cnt);
    printf("create thread(%ld) err(%d)\n", t1, err);

    err = pthread_create(&t2, NULL, &thread_with_atomic, &cnt);
    printf("create thread(%ld) err(%d)\n", t2, err);

    err = pthread_join(t1, NULL);
    printf("join thread(%ld) err(%d)\n", t1, err);

    err = pthread_join(t2, NULL);
    printf("join thread(%ld) err(%d)\n", t2, err);

    ts = microtime() - ts;
    printf("\ncounter %d\n", cnt.cnt);
    printf("time %1.3f\n\n", ts);
    return 0;
}

int main()
{
    test_nolock();

    test_with_mutex();

    test_with_atomic();
}
