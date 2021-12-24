#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#define EOK 0

typedef struct {
    int (*func)(int);
    int arg;
    int res;
    int ok;
} args_t;


int f(int x)
{
    pthread_t tid = pthread_self();
    for (int i=0; i<x; i++) {
        fprintf(stderr, "  func f(%d) thread(%ld) time(%d)\n", x, tid, i);
        sleep(1);
    }
    return x;
}

int g(int x)
{
    pthread_t tid = pthread_self();
    for (int i=0; i<x; i++) {
        fprintf(stderr, "  func g(%d) thread(%ld) time(%d)\n", x, tid, i);
        sleep(1);
    }
    return -x;
}

void *thread(void *arg)
{
    pthread_t tid = pthread_self();
    args_t* p = (args_t*)arg;
    time_t start = time(NULL);

    printf(" enter thread(%ld) arg(%d)\n", tid, p->arg);

    p->res = p->func(p->arg);
    p->ok = 1;

    printf(" leave thread(%ld) res(%d) time(%ld)\n", tid, p->res, time(NULL)-start);
}

void wait_for(pthread_t* pt, int ts)
{
    int err = pthread_tryjoin_np(*pt, NULL);

    if (err == EOK) {
        printf("end thread(%ld) time(%d)\n", *pt, ts);
        *pt = 0;
    }
    else if (err == EBUSY) {
        fprintf(stderr, "busy thread(%ld) time(%d)\n", *pt, ts);
    }
    else {
        printf("error thread(%ld) err(%d)\n", *pt, err);
    }
    if (*pt && ts && ts % 10 == 0) {
        printf("\nthread(%ld) wait(%d) cancel (y/n)?\n\n", *pt, ts);

        if (getchar() == 'y') {
            printf("cancel thread(%ld) time(%d)\n", *pt, ts);
            pthread_cancel(*pt);
            *pt = 0;
        }
    }
}

int main(int argc, char *argv[])
{
    pthread_t t1, t2;
    args_t a1, a2;
    int err, res;

    if (argc < 2) {
        printf("usage: mul n1 n2\n");
        return 0;
    }

    a1.func = f;
    a1.arg = atoi(argv[1]);
    a2.res = 0;
    a1.ok = 0;

    a2.func = g;
    a2.arg = atoi(argv[2]);
    a2.res = 0;
    a2.ok = 0;

    err = pthread_create(&t1, NULL, &thread, &a1);
    printf("create thread(%ld) err(%d)\n", t1, err);

    err = pthread_create(&t2, NULL, &thread, &a2);
    printf("create thread(%ld) err(%d)\n", t2, err);

    time_t start = time(NULL);

    while (t1 || t2)
    {
        int ts = time(NULL) - start;

        if (t1) {
            wait_for(&t1, ts);

            if (!t1 && a1.ok && a1.res == 0) {
                if (t2) {
                    printf("cancel thread(%ld) time(%d)\n", t2, ts);
                    pthread_cancel(t2);
                }
                break;
            }
        }

        if (t2) {
            wait_for(&t2, ts);

            if (!t2 && a2.ok && a2.res == 0) {
                if (t1) {
                    printf("cancel thread(%ld) time(%d)\n", t1, ts);
                    pthread_cancel(t1);
                }
                break;
            }
        }

        if (t1 || t2)
            usleep(1000);
    }

    if ((a1.ok && a2.ok) || (a1.ok && a1.res == 0) || (a2.ok && a2.res == 0)) {
        res = a1.res * a2.res;
        printf("\n%d x %d = %d\n", a1.res, a2.res, res);
    } else {
        printf("\n* no result *\n");
    }

    printf("\ntotal time %ld\n", time(NULL)-start);
    return 0;
}
