#include <stdio.h>
#include <sched.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <pthread.h>

#include "thr_channel.h"
#include "medialib.h"
#include "server_conf.h"

struct thr_channel_ent_st {
    chnid_t     chnid;
    pthread_t   tid;
};

static struct msg_channel_st *sbuf;
static struct thr_channel_ent_st thr_channel[CHNNR];
static int tid_nextpos = 0;

static void *thr_channel_func(void *ptr)
{
    struct mlib_listentry_st *ent = ptr;
    int len, ret;
    struct msg_channel_st *sbufp;
    int datasize;

    syslog(LOG_DEBUG, "thr_channel_snder() is working for channel %d.", ent->id);

    sbufp = malloc(MSG_CHANNEL_MAX);
    if (sbufp == NULL) {
        syslog(LOG_ERR, "malloc():%s", strerror(errno));
        exit(1);
    }
    datasize = MSG_CHANNEL_MAX-sizeof(chnid_t);

    sbufp->id = ent->id;
    while (1) {
        pthread_testcancel();
        len = mlib_readchn(ent->id, sbufp->data, datasize);
        sendto(serversd, sbufp, len+sizeof(chnid_t), 0 ,(struct sockaddr*)&sndaddr, sizeof(sndaddr));
        sched_yield();
    }

}

//  #define MSG_CHANNEL_MAX (65536-20-8)
int thr_channel_create(struct mlib_listentry_st *ptr)
{
    int len, err;

    sbuf = malloc(MSG_CHANNEL_MAX);
    if (sbuf == NULL) {
        syslog(LOG_ERR, "thr_channel_create sbuf malloc(): %s", strerror(errno));
        return -ENOMEM;
    }

    if (tid_nextpos >= CHNNR) {
        free(sbuf);
        return -ENOSPC;
    }
    err = pthread_create(&thr_channel[tid_nextpos].tid, NULL, thr_channel_func, ptr);
    if (err) {
        syslog(LOG_WARNING, "pthread_create:%s", strerror(err));
        return -err;
    }
    thr_channel[tid_nextpos].chnid = ptr->id;
    tid_nextpos++;

    return 0;

}
int thr_channel_destroy(struct mlib_listentry_st *ptr)
{
    int i;
    for (i = 0; i < CHNNR; i++) {
        if (thr_channel[i].chnid == ptr->id) {
            if (pthread_cancel(thr_channel[i].tid)) {
                syslog(LOG_ERR, "The thread of Channel %d.", ptr->id);
                return -ESRCH;
            }
            pthread_join(thr_channel[i].tid, NULL);
            thr_channel[i].chnid = -1;
            return 0;
        }
    }
    syslog(LOG_ERR, "Channel %d doesn't exist.", ptr->id);
    return -ESRCH;
}

int thr_channel_destroyall(void)
{
    int i;
    for (i = 0; i < CHNNR; i++) {
        if (thr_channel[i].chnid > 0) {
            if (pthread_cancel(thr_channel[i].tid)) {
                syslog(LOG_ERR, "The thread of Channel %d.", thr_channel[i].chnid);
                return -ESRCH;
            }
            pthread_join(thr_channel[i].tid, NULL);
            thr_channel[i].chnid = -1;
        }
    }
    return 0;
}
