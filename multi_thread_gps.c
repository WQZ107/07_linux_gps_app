#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include "gps_nmea.h"
#include "serial.h"

/* ========== 共享数据 + 它的锁 ========== */
/* g_gps 的"定义"在 gps_nmea.c(全局变量只许定义一次);此处通过 gps_nmea.h 的 extern 引用 */
pthread_mutex_t g_gps_lock = PTHREAD_MUTEX_INITIALIZER; /* 静态初始化由来已久 */

/* ========== 线程1:GPS 接收线程(阻塞读串口 → 拼行 → 加锁解析) ========== */
void *gps_thread(void *arg)
{
    (void)arg;
    int fd = serial_open("/dev/ttymxc2", 9600);
    if (fd < 0) return NULL;

    char buf[256], line[128];
    int line_len = 0;

    while (1) {
        int len = read(fd, buf, sizeof(buf) - 1);   /* 这里阻塞是常态,不碍另一个线程 */
        if (len > 0) {
            for (int i = 0; i < len; i++) {
                if (buf[i] == '\n') {               /* 一行结束 */
                    line[line_len] = '\0';

                    pthread_mutex_lock(&g_gps_lock);     /* ★写共享区前加锁 */
                    GPS_ParseLine(line);                 /*   STM32 的解析器原样用 */
                    pthread_mutex_unlock(&g_gps_lock);   /* ★用完立刻解锁 */

                    line_len = 0;
                } else if (buf[i] != '\r') {
                    if (line_len < (int)sizeof(line) - 1)
                        line[line_len++] = buf[i];       /* 防越界 */
                }
            }
        }
    }
    return NULL;
}

/* ========== 线程2:显示 + 落盘线程(每2秒打印 / 每5秒追加文件) ========== */
#define DATA_FILE "/home/root/data/gps_data.txt"    /* 块2教训:落地用 /home/root(持久) */

void *display_thread(void *arg)
{
    (void)arg;
    int tick = 0;

    while (1) {
        sleep(1);
        tick++;

        if (tick % 2 == 0) {                        /* ---- 每2秒:打印状态 ---- */
            pthread_mutex_lock(&g_gps_lock);        /* ★读共享区也要加锁 */
            printf("[显示线程] %s lat=%.6f lon=%.6f spd=%.1fkm/h sat=%u\n",
                   g_gps.valid ? "已定位" : "等待定位",
                   g_gps.latitude, g_gps.longitude,
                   g_gps.speed_kmh, g_gps.satellites);
            pthread_mutex_unlock(&g_gps_lock);
        }

        if (tick % 5 == 0) {                        /* ---- 每5秒:追加落盘 ---- */
            char rec[160];
            pthread_mutex_lock(&g_gps_lock);
            int n = snprintf(rec, sizeof(rec),      /* 拼行:printf 的"写到字符串"版 */
                "%02u:%02u:%02u,%.6f,%.6f,%.1f,%u,%u\n",
                g_gps.hour, g_gps.min, g_gps.sec,
                g_gps.latitude, g_gps.longitude,
                g_gps.speed_kmh, g_gps.satellites,
                g_gps.valid);                       /* 末尾追加 valid 列:室内无定位也能验落盘链 */
            pthread_mutex_unlock(&g_gps_lock);      /* 锁只圈共享数据;写文件不占锁 */

            int fd = open(DATA_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd >= 0) {
                write(fd, rec, n);
                close(fd);
                printf("[落盘] 已追加一行(%d字节)\n", n);
            } else {
                perror("打开数据文件失败(目录不存在? 先 mkdir -p /home/root/data)");
            }
        }
    }
    return NULL;
}

/* ========== main:创建两个线程,等它们 ========== */
int main(void)
{
    pthread_t tid1, tid2;

    printf("=== 多线程 GPS 采集器启动 ===\n");

    pthread_create(&tid1, NULL, gps_thread, NULL);      /* 创建线程1 */
    pthread_create(&tid2, NULL, display_thread, NULL);  /* 创建线程2 */

    pthread_join(tid1, NULL);       /* 主线程在这里等(实际不会返回) */
    pthread_join(tid2, NULL);
    return 0;
}
