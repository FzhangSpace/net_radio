#include <stdio.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <errno.h>
#include <unistd.h>
#include <net/if.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <signal.h>

#include <proto.h>
#include "client_conf.h"

static int pd[2];   /* 管道 */

struct client_conf_st client_conf = {
    .rcvport = DEFAULT_RCVPORT,
    .mgroup = DEFAULT_MGROUP,
    .player_cmd = DEFAULT_PLAYCMD
};

static void printhelp(void)
{
    printf("hellp...\n");
}

static void usr1_handler(int s)
{
    
}

/*  
 *  -P, --port      指定接收端口
 *  -M, --mgroup    指定多播组
 *  -p, --player    指定播放器命令行
 *  -H, --help      显示帮助
 */

struct option argarr[] = {
    {"port", 1, NULL, 'P'},
    {"mgroup", 1, NULL, 'M'},
    {"player", 1, NULL, 'p'},
    {"help", 0, NULL, 'H'},
};
#define NR_ARGS  4

static ssize_t writen(int fd, const char *buf, size_t len) 
{
    int pos = 0, ret;

    while (len > 0) {
        ret = write(fd, buf+pos, len);
        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        len -= ret;
        pos += ret;
    }
    if (pos == 0) {
        return -1;
    }
    return pos;
}

int main(int argc, char **argv)
{
    pid_t   pid;
    int     ret, sd, len, index = NR_ARGS, count, child_pause = 1;
    struct sockaddr_in laddr, raddr, serveraddr;
    socklen_t raddr_len, serveraddr_len;

    struct msg_list_st *msg_list;
    struct msg_channel_st *msg_channel;
    chnid_t chosenid;

    struct ip_mreqn mreq;

    signal(SIGUSR1, usr1_handler);

    while (1) {
        ret = getopt_long(argc, argv, "P:M:p:H:", argarr, &index);
        if (ret < 0) {
            break;
        }
        switch (ret) {
            case 'P':
                client_conf.rcvport = optarg;
                break;
            case 'M':
                client_conf.mgroup = optarg;
                break;
            case 'p':
                client_conf.player_cmd = optarg;
                break;
            case 'H':
                printhelp();
                exit(0);
                break;
            default:
                abort();
                break;
        }
    }

    if (pipe(pd) < 0) { 
        perror("pipe()");
        exit(1);
    }

    pid = fork();
    if (pid < 0) {
        perror("fork()");
        exit(1);
    }

    if (pid == 0) { /* child */
        close(pd[1]);
        dup2(pd[0], STDIN_FILENO); /* 让标准输入从管道中读取数据 */
        if (pd[0] > 0) {    /* 然后把原来的管道读端关掉 */
            close(pd[0]);
        }
        pause();
        /* 运行解码器 mpg123*/
        execl("/bin/sh", "sh", "-c", client_conf.player_cmd, NULL);  /* - 从标准输入读取数据 */
        exit(0);
    }

    close(pd[0]);

    sd = socket(AF_INET, SOCK_DGRAM, 0 /* IPPROTO_UDP */);
    if (sd < 0) {
        perror("socket()");
        exit(1);
    }

    /* 设置组地址 */
    inet_pton(AF_INET, client_conf.mgroup, &mreq.imr_multiaddr);
    inet_pton(AF_INET, "0.0.0.0", &mreq.imr_address);
    mreq.imr_ifindex = if_nametoindex("eth0");
    if (setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        perror("setsockopt()");
        exit(1);
    }
    int val = 1024000;
    if (setsockopt(sd, SOL_SOCKET, SO_RCVBUF, &val, sizeof(val)) < 0) {
        perror("setsockopt()");
        exit(1);
    }

    /* 初始化本地端地址 */
    bzero(&laddr, sizeof(laddr));
    laddr.sin_family = AF_INET;
    inet_pton(AF_INET, "0.0.0.0", &laddr.sin_addr.s_addr);
    laddr.sin_port = htons(atoi(client_conf.rcvport));

    if (bind(sd, (struct sockaddr*)&laddr, sizeof(laddr)) < 0) {
        perror("bind()");
        exit(1);
    }

    msg_list = malloc(MSG_LIST_MAX);
    if (msg_list == NULL) {
        perror("malloc()");
        exit(1);
    }

    serveraddr_len = sizeof(serveraddr);
    while (1) {
        /* 收包 */
        len = recvfrom(sd, msg_list, MSG_LIST_MAX, 0, (struct sockaddr *)&serveraddr, &serveraddr_len);
        fprintf(stdout, "msg recieved\n");
        if (len < sizeof(struct msg_list_st)/* 是节目单? */) {
            fprintf(stdout, "message is too small.\n");
            continue;
        } 
        if (msg_list->id != LISTCHNID) {
            fprintf(stdout, "Chind is not match.\n");
            continue;
        } 
        break;
    }

    printf("--------------------\n");
    /* 选择频道 */
#if 1
    struct msg_listentry_st *pos;

    for (pos = msg_list->entry; 
            (char*)pos < ((char*)(msg_list)+len); 
            pos=(void*)((char*)pos)+ntohs(pos->len)) {
        printf("Channel %d:%s, len:%d\n", pos->id, pos->desrc, ntohs(pos->len));
        sleep(1);
    }
    do {
        printf("chose channel:\n");
        ret = scanf("%d", &chosenid);
        printf("chosenid is %d\n", chosenid);
    } while (ret < 1);
#else

    chosenid=1;
#endif
    free(msg_list);
    msg_channel = malloc(MSG_CHANNEL_MAX);
    if (msg_channel == NULL) {
        perror("malloc()");
        exit(1);
    }

    count = 0;
    raddr_len = sizeof(raddr);
    while (1) {
        /* 收包 */
        len = recvfrom(sd, msg_channel, MSG_CHANNEL_MAX, 0, (struct sockaddr *)&raddr, &raddr_len);
        if (raddr.sin_addr.s_addr != serveraddr.sin_addr.s_addr ||
                raddr.sin_port != serveraddr.sin_port) {
            continue;
        } else if (len < sizeof(struct msg_channel_st)) {
            continue;
        }

        if (msg_channel->id == chosenid/* 是选定频道么? */) {
                /* PLAY */
            /* 写到管道写端,交给子进程拉起来的mpg123*/
            //writen(STDOUT_FILENO, (void*)msg_channel->data, len-sizeof(struct msg_channel_st)-1);    
            ret = writen(pd[1], (void*)msg_channel->data, len-sizeof(chnid_t));    /* 写到管道写端,交给子进程拉起来的mpg123*/
 //           printf("------%d------\n", count);
            count += ret;
        }
        if (child_pause && count > 60000) {
            kill(pid, SIGUSR1);
            child_pause = 0;
            printf("playing\n");
        }

    }

    free(msg_channel);
    close(sd);
    return 0;
}
