#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "mytbf.h"

int main(void)
{
    int fd, len, num;
    mytbf_t *hand;
    char buf[1024];

    fd = open("aa", O_RDONLY);

    hand = mytbf_init(40, 60);

    while (1) {
        len = mytbf_fetchtoken(hand, sizeof(buf));
        num = read(fd, buf, len);
        if (num <= 0) {
            printf("finsh--------\n");
            break;
        }
        len -= num;
        mytbf_returntoken(hand, len);
        write(STDOUT_FILENO, buf, num);
    }

    mytbf_destroy(hand);

    return 0;
}
