#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "gps_nmea.h"
#include "serial.h"

int main(void)
{
    int fd = serial_open("/dev/ttymxc2", 9600);
    if (fd < 0) return -1;

    printf("=== Linux GPS 解析器启动 ===\n");

    char buf[256];
    char line[128];
    int  line_len = 0;

    while (1) {
        int len = read(fd, buf, sizeof(buf) - 1);
        if (len > 0) {
            for (int i = 0; i < len; i++) {
                if (buf[i] == '\n') {              /* 一行结束 */
                    line[line_len] = '\0';
                    GPS_ParseLine(line);           /* ★ STM32 的解析器原样用 */
                    line_len = 0;

                    /* 每收到一条 RMC 打印一次状态(约 1Hz) */
                    if (strncmp(line + 3, "RMC", 3) == 0) {
                        if (g_gps.valid)
                            printf("[GPS] lat=%.6f lon=%.6f spd=%.1fkm/h sat=%u time=%02u:%02u:%02u\n",
                                   g_gps.latitude, g_gps.longitude, g_gps.speed_kmh,
                                   g_gps.satellites, g_gps.hour, g_gps.min, g_gps.sec);
                        else
                            printf("[GPS] 等待定位... (sat=%u)\n", g_gps.satellites);
                    }
                } else if (buf[i] != '\r') {
                    if (line_len < (int)sizeof(line) - 1)
                        line[line_len++] = buf[i]; /* 防越界 */
                }
            }
        }
    }
    close(fd);
    return 0;
}
