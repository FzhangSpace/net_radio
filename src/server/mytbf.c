#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

#include "mytbf.h"

struct mytbf_st {
    int cps;    /* 每秒多少字节 */
    int burst;  /* 桶的容量 字节数*/
    int token;  /* 当前桶的大小 字节数*/
    pthread_mutex_t mut;
    pthread_cond_t  cond;
};

static struct mytbf_st *tbf[TBFMAX];
static pthread_mutex_t mut_tbf = PTHREAD_MUTEX_INITIALIZER;
static pthread_t tid_timer;
static pthread_once_t init_once = PTHREAD_ONCE_INIT;

static void *thr_timer_func(void *unused)
{
    int i;
    struct timespec t;

    while (1) {
        t.tv_sec = 1;
        t.tv_nsec = 0;
        while (nanosleep(&t, &t) != 0) {
            if (errno != EINTR) {
                syslog(LOG_ERR, "nanosleep(): %s", strerror(errno));
                exit(1);
            }
        }

        for (i = 0; i < TBFMAX; i++) {
            if (tbf[i] != NULL) {
                pthread_mutex_lock(&tbf[i]->mut);
                tbf[i]->token += tbf[i]->cps;
                if (tbf[i]->token > tbf[i]->burst) {
                    tbf[i]->token = tbf[i]->burst;
                }
                pthread_cond_signal(&tbf[i]->cond);
                pthread_mutex_unlock(&tbf[i]->mut);
            }
        }
    }
}

static void module_unload(void)
{
    pthread_cancel(tid_timer);
    pthread_join(tid_timer, NULL);
}

static void module_load(void)
{
    int err;
    err = pthread_create(&tid_timer, NULL, thr_timer_func, NULL);
    
    if (err) {
        syslog(LOG_ERR, "pthread_create():%s", strerror(errno));
        exit(1);
    }

    atexit(module_unload);
}


static int get_free_pos()
{
    int i;
    for (i = 0; i < TBFMAX; i++) {
        if (tbf[i] == NULL) {
            return i;
        }
    }
    return -1;
}

mytbf_t *mytbf_init(int cps, int burst)
{
    struct mytbf_st *me;
    int pos;

    pthread_once(&init_once, module_load);

    me = malloc(sizeof(struct mytbf_st));
    if (me == NULL) {
        return NULL;
    }

    me->cps = cps;
    me->burst = burst;
    me->token = 0;

    pthread_mutex_init(&me->mut, NULL);
    pthread_cond_init(&me->cond, NULL);

    pthread_mutex_lock(&mut_tbf);
    pos = get_free_pos();
    if (pos < 0) {
        pthread_mutex_unlock(&mut_tbf);
        free(me);

        pthread_mutex_destroy(&me->mut);
        pthread_cond_destroy(&me->cond);
        return NULL;
    }
    tbf[pos] = me;
    pthread_mutex_unlock(&mut_tbf);

    return me;
}

int mytbf_destroy(mytbf_t *ptr)
{
    struct mytbf_st *me = ptr;

    pthread_mutex_lock(&me->mut);
    pthread_mutex_destroy(&me->mut);
    pthread_cond_destroy(&me->cond);

    free(me);
    return 0;
}

int mytbf_fetchtoken(mytbf_t *ptr,int n)
{
    struct mytbf_st *me = ptr;
    int token;

    pthread_mutex_lock(&me->mut);
    while (me->token <= 0) {
        pthread_cond_wait(&me->cond, &me->mut);
    }
    token = me->token > n ? n : me->token;
    me->token -= token;
    pthread_mutex_unlock(&me->mut);

    return token;
}

int mytbf_returntoken(mytbf_t *ptr,int n)
{
    struct mytbf_st *me = ptr;
    pthread_mutex_lock(&me->mut);
    me->token += n;

    pthread_cond_signal(&me->cond);
    pthread_mutex_unlock(&me->mut);
    
    return 0;
}
