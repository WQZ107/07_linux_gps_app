# IMX6ULL GPS 数据采集器（Linux 版）

> 正点原子 I.MX6U-ALPHA 板上的 GPS 采集程序：读 ATK GPS 模块的 NMEA 语句，解析经纬度 / 速度 / 卫星数，双线程并发，数据每 5 秒追加落盘。
> 这是整个 200 天计划中「Linux 应用开发入门」阶段的里程碑项目（Day 71-90）。

[![Platform](https://img.shields.io/badge/platform-ARM%20Linux-orange)](#)
[![Language](https://img.shields.io/badge/language-C-blue)](#)
[![Baudrate](https://img.shields.io/badge/UART-9600-green)](#)

## 概览

本仓库完成了从串口裸数据到结构化 GPS 数据的完整链路：

- **串口层**（`serial.c/.h`）：`open(O_RDWR|O_NOCTTY|O_NDELAY)` + `termios` 配置 9600-8N1 原始模式（关 ICANON / 回显），按 `\n` 切分 NMEA 行
- **解析层**（`gps_nmea.c/.h`）：四层解析器，从 STM32 阶段零修改移植（GBK→UTF-8 转码后直接复用）
- **应用层**（`multi_thread_gps.c`）：双线程——显示线程每 2 秒打状态，存储线程每 5 秒追加落盘

这是后续所有多线程项目的骨架源头：条件变量、环形缓冲、优雅退出的套路都在这一阶段成型。

## 特性

- **串口原始模式**：`cfsetispeed/cfsetospeed` 设波特率，`c_lflag &= ~(ICANON|ECHO|...)`，`c_cc[VTIME]` 超时控制
- **NMEA 四层解析**：按行 → 定位语句（RMC/GGA）→ 字段提取 → 单位换算，室内无定位时输出状态行 `valid=0`
- **双线程 + 条件变量**：生产（串口接收）与消费（显示/落盘）解耦
- **追加式落盘**：`O_APPEND` 单条 write 原子，跨断电数据不归零

## 编译与运行

```bash
# 编译（交叉工具链 arm-linux-gnueabihf-gcc, Linaro 4.9.4）
make

# 板上运行
./multi_thread_gps
# GPS 接 "ATK 模块接口"；屏幕每 2 秒打状态，数据每 5 秒进 /home/root/data/gps_data.txt
```

落盘格式（7 列）：`时,纬,经,速,星,valid` + 状态行。室内无定位时卫星数为 0 属正常现象。

## 目录结构

```
├── serial.c/.h            # 串口库（termios 配置 + 按行读取）
├── gps_nmea.c/.h          # NMEA 解析器（STM32 期零修改移植）
├── multi_thread_gps.c     # 双线程主程序（显示 2s / 落盘 5s）
├── gps_test.c             # 解析单元测试
├── gps_raw_test.c         # 串口原始数据测试
├── main.c                 # 单线程版本
└── Makefile
```

## 相关仓库

- 后续演进（多线程采集框架）：[08_linux_sensor_framework](https://github.com/WQZ107/08_linux_sensor_framework)
