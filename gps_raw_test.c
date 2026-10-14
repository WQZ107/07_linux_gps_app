#include <stdio.h>
#include <unistd.h>
#include "serial.h"

int main(void)
{
    int fd = serial_open("/dev/ttymxc2", 9600);   /* GPS 默认 9600 */
    if (fd < 0) return -1;

    printf("=== 开始读 GPS 原始数据 ===\n");

    char buf[256];
    while (1) {
        int len = read(fd, buf, sizeof(buf) - 1);
        if (len > 0) {
            buf[len] = '\0';
            printf("%s", buf);      /* 原样打印 NMEA */
        }
    }
    close(fd);
    return 0;
}
