# CYD 硬件与引脚

[English](HARDWARE.en.md) | [README](../README.md)

![主板商品外观参考](images/cyd-board-product.jpg)

商品外观参考；屏幕为商品演示界面。

发布固件面向 **ESP32-2432S028R 电阻触摸版**，即经典 ESP32、2.8 英寸 ILI9341 TFT 和 XPT2046 触摸控制器的组合。本项目实测设备为 ESP32-D0WD-V3、4MB Flash、无 PSRAM。下表记录当前源码使用的配置，不代表其他 CYD 修订版具有相同接线。

屏幕原生分辨率为 240×320，界面横屏为 320×240。显示驱动为 `TFT_eSPI` 的 `ILI9341_2_DRIVER`；触摸单独使用 `XPT2046_Touchscreen`，不是电容触摸。首次启动需要触摸校准，校准结果保存在内部存储，有卡时同步 SD。

## GPIO 对照

| 模块 | 信号 | ESP32 GPIO |
| --- | --- | --- |
| TFT | MOSI | 13 |
| TFT | SCLK | 14 |
| TFT | CS | 15 |
| TFT | DC | 2 |
| TFT | RST | 12 |
| TFT | 背光/PWM | 21 |
| XPT2046 | MOSI | 32 |
| XPT2046 | MISO | 39 |
| XPT2046 | CLK | 25 |
| XPT2046 | CS | 33 |
| XPT2046 | IRQ | 36 |
| microSD | MOSI | 23 |
| microSD | MISO | 19 |
| microSD | SCK | 18 |
| microSD | CS | 5 |

TFT SPI 写入频率为 40MHz，读取配置为 20MHz。触摸由独立的 HSPI 引脚和专用库处理。microSD 用于首次配置导入、备份同步和卡刷；完成初始化后可无卡运行。

当前 CYD 配置没有定义电池 ADC 分压采样或电量计芯片接口，固件无法可靠读取电池百分比。外接电池供电本身不会提供电量数据，因此界面不显示推测的电池电量。

## 兼容范围

选板时核对型号、电阻/电容触摸版本、显示芯片、触摸芯片及引脚。不能只看“CYD”“2.8 寸”或 USB 接口外形判断兼容性。当前固件没有屏幕/触摸芯片自动探测功能。

ST7789、其他显示分辨率、GT911/FT6236 电容触摸、ESP32-S3 及不同 GPIO 接线不属于当前发布镜像的直接支持范围，需修改驱动、引脚及相关显示/触摸初始化后重新编译和实机验证。

## 配置位置

- [显示驱动与 TFT 引脚](../resources/User_Setup.h)
- [触摸驱动与引脚、校准](../src/devices/2.8/touch.h)
- [显示尺寸、背光和 SD 引脚](../src/devices/2.8/screen.h)
- [构建环境和依赖](../platformio.ini)
- [上游硬件介绍](../README.upstream.md#required-hardware)
