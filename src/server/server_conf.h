#ifndef SERVER_CONF_H
#define SERVER_CONF_H

#include <stdio.h>

enum {
    run_daemon,
    run_forground,
};

struct server_conf_st {
    char*   rcvport;
    char*   mgroup;
    char*   media_dir;
    char*   ifname;
    char    runmode;
};

extern struct server_conf_st server_conf;
extern int serversd;
extern struct sockaddr_in sndaddr;

#define DEFAULT_MEDIADIR    "/home/itcast/netradio/tmp/media"
#define DEFAULT_IF      "eth0" 

#endif
