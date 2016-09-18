#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>

#include <proto.h>

#define GROUP_IP "239.0.0.2"

int main(void) 
{
    int sd;
    struct msg_list_st *listbuf;
    struct msg_listentry_st *tmp;

    struct ip_mreqn mreq;
    struct sockaddr_in servaddr, raddr;
    int id[3] = {12, 23, 34}, i;
    char *desrc[3] = {"Music", "Opera", "Talks"};

    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd < 0) {
        perror("socket()");
        exit(1);
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(8000);
    bind(sd, (struct sockaddr*)&servaddr, sizeof(servaddr));

    inet_pton(AF_INET, GROUP_IP, &mreq.imr_multiaddr);
    inet_pton(AF_INET, "0.0.0.0", &mreq.imr_address);
    mreq.imr_ifindex = if_nametoindex("eth0");
    if (setsockopt(sd, IPPROTO_IP, IP_MULTICAST_IF, &mreq, sizeof(mreq)) < 0) {
        perror("setsockopt()");
        exit(1);
    }

    /*
    struct msg_list_st *listbuf;
    struct msg_listentry_st *tmp;
    */
    listbuf = malloc(28);
    if (listbuf == NULL) {
        perror("malloc()");
        exit(1);
    }

    listbuf->id = LISTCHNID;
    tmp = listbuf->entry;
    for (i = 0; i < 3; i++) {
        tmp->id = id[i];
        tmp->len = htons(9);
        memcpy(tmp->desrc, desrc[i], 5);
        printf("tmp[%d]:%s\n", i, (char*)tmp->desrc);
        tmp = (void*)(((char*)tmp)+9);
        //tmp++;
        printf("tmp,%d | tmp+1:%d| sizeof(tmp):%d\n", (int)tmp, (int)(tmp+1),sizeof(tmp));
    }

    bzero(&raddr, sizeof(raddr));
    raddr.sin_family = AF_INET;
    inet_pton(AF_INET, "239.0.0.2", &raddr.sin_addr);
    raddr.sin_port = htons(2000);
    while (1) {
        printf("--------------------%d\n", i++);
        sendto(sd, listbuf, 28, 0, (struct sockaddr*)&raddr, sizeof(raddr));
        sleep(1);
    }



    return 0;
}
