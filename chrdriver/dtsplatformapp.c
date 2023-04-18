#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#define LEDOFF 0
#define LEDON 1

int main(int argc, char *argv[]) {
    int fd, ret;
    char *filename;
    unsigned char databuf[1];

    if (argc != 3) {
        printf("Usage: err\r\n");
        return -1;
    }

    filename = argv[1];

    /* 打开led驱动 */
    fd = open(filename, O_RDWR);
    if (fd < 0) {
        printf("Error opening file: %s\r\n", filename);
        return -1;
    }

    databuf[0] = atoi(argv[2]); /* 要执行的操作，打开或者关闭 */
    ret = write(fd, databuf, sizeof(databuf));
    if (ret < 0) {
        printf("LED error write: %s\r\n", filename);
        return -1;
    }

    ret = close(fd);
    if (ret < 0) {
        printf("LED error close: %s\r\n", filename);
        return -1;
    }
    return 0;
}