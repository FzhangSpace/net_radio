#ifndef PROTO_H
#define PROTO_H

#include <stdint.h>

#include <site_types.h>

#define DEFAULT_RCVPORT "2000"
#define DEFAULT_MGROUP "239.0.0.2"

#define CHNNR   200 /* 频道个数 */

#define LISTCHNID       0   /* 节目单的频道号 */
#define MINCHNID        1   /* 最小频道号 */
#define MAXCHNID        (MINCHNID+CHNNR-1)  /* 最大频道号 */

#define MSG_CHANNEL_MAX    (65536-20-8)

//typedef uint8_t chnid_t;    /* 频道号 */

struct msg_channel_st {
    chnid_t     id; /* MUST BETWEEN [MINCHIND, MAXCHIND] 频道号 */
    uint8_t     data[1];
} __attribute__((packed));   
/* 不进行结构体内存对齐,避免网络通信中,双方对齐方式不一样 */

#define MSG_LIST_MAX    (65536-20-8)
struct msg_listentry_st {
    chnid_t     id; /* MUST BETWEEN [MINCHIND, MAXCHIND] 频道号 */
    uint16_t    len;
    uint8_t     desrc[1];
} __attribute__((packed));

struct msg_list_st {
    chnid_t     id; /* MUST BE LISTCHNID */
    struct msg_listentry_st entry[1];
} __attribute__((packed));


#endif
