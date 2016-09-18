#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>

#include "thr_list.h"
#include "server.h"
#include "server_conf.h"
#include <proto.h>

static pthread_t tid_list;
static struct mlib_listentry_st *list_ent;
static int nr_list_ent;

static void *thr_list(void *unused)
{
    int i, totalsize, ret;
    short size;
    struct msg_listentry_st *entryp;
    struct msg_list_st *entlistp;

    totalsize = sizeof(chnid_t);

    for (i = 0; i < nr_list_ent; ++i) {
        totalsize += sizeof(struct msg_listentry_st)+strlen(list_ent[i].desc);
    }

    entlistp = malloc(totalsize);
    if (entlistp == NULL) {
        syslog(LOG_ERR, "%s :malloc():%s",__func__,  strerror(errno));
        exit(1);
    }

    entlistp->id = LISTCHNID;
    entryp = entlistp->entry;
    for (i = 0; i < nr_list_ent; ++i) {
        size = sizeof(struct msg_listentry_st)+strlen(list_ent[i].desc);
        entryp->id = list_ent[i].id;
        entryp->len = htons(size);
        strcpy(entryp->desrc, list_ent[i].desc);
        entryp = (struct msg_listentry_st *)((char*)(entryp) + size);
    }

    while (1) {
        ret = sendto(serversd, entlistp, totalsize, 0, (struct sockaddr*)&sndaddr, sizeof(sndaddr));
        if (ret < 0) {
            syslog(LOG_WARNING, "sendto(serversd, entlistp, ...):%s.", strerror(errno)); 
        } else {
            syslog(LOG_DEBUG, "sendto(serversd, entlistp, ...): sucessded.");
        }
        sleep(1);
    }
}

int thr_list_create(struct mlib_listentry_st *listp, int nr_ent)
{
    int err;
    list_ent = listp;
    nr_list_ent = nr_ent;

    err = pthread_create(&tid_list, NULL, thr_list, NULL);
    if (err) {
        syslog(LOG_ERR, "pthread_craete():%s", strerror(err));
        return -1;
    }
    return 0;
}

int thr_destroy(void)
{
    pthread_cancel(tid_list);
    pthread_join(tid_list, NULL);
    return 0;
}
