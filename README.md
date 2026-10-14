# IMX6ULL GPS 数据采集器(Linux 版)

正点原子 I.MX6U-ALPHA 板载数据采集程序:读 GPS(ATK 模块接口 /dev/ttymxc2,9600) 的 NMEA,
解析经纬度/速度/卫星数;双线程并发,数据每 5 秒追加落盘。

## 怎么编译
make    # 交叉工具链: arm-linux-gnueabihf-gcc (Linaro 4.9.4)

## 怎么跑
./multi_thread_gps    # 板上运行;GPS 接"ATK 模块接口";屏幕每 2 秒打状态,数据每 5 秒进 /home/root/data/gps_data.txt

## 文件
serial.c/h=串口库;gps_nmea.c/h=NMEA解析器(STM32期零修改移植);multi_thread_gps.c=双线程主程序
