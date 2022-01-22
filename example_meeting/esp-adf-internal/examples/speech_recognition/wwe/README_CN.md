# 语音唤醒及语音命令词检测 (WWE + MN) 例程

- [English Version](./README.md)
- 例程难度：![alt text](../../../docs/_static/level_basic.png "初级")

## 例程简介

本例程演示了从麦克风读取环境声音数据，经过声学前端 (audio front-end, AFE) 算法和 MultiNet 模型处理分析，最后输出唤醒状态或者命令词索引。

管道的数据流向，如下图所示：

```
mic ---> codec_chip ---> i2s_driver ---> afe ---> multinet ---> audio_recorder ---> output
```

## 环境配置

### 硬件要求

本例程可在标有绿色复选框的开发板上运行。请记住，如下面的 [配置](#配置) 一节所述，可以在 `menuconfig` 中选择开发板。

| 开发板名称 | 开始入门 | 芯片 | 兼容性 |
|-------------------|:--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------:|:--------------------------------------------------------------------:|:-----------------------------------------------------------------:|
| ESP32-LyraT | [![alt text](../../../docs/_static/esp32-lyrat-v4.3-side-small.jpg "ESP32-LyraT")](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/get-started-esp32-lyrat.html) | <img src="../../../docs/_static/ESP32.svg" height="85" alt="ESP32"> | ![alt text](../../../docs/_static/yes-button.png "开发板兼容此例程") |
| ESP32-LyraTD-MSC | [![alt text](../../../docs/_static/esp32-lyratd-msc-v2.2-small.jpg "ESP32-LyraTD-MSC")](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/get-started-esp32-lyratd-msc.html) | <img src="../../../docs/_static/ESP32.svg" height="85" alt="ESP32"> | ![alt text](../../../docs/_static/yes-button.png "开发板兼容此例程") |
| ESP32-LyraT-Mini | [![alt text](../../../docs/_static/esp32-lyrat-mini-v1.2-small.jpg "ESP32-LyraT-Mini")](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/get-started-esp32-lyrat-mini.html) | <img src="../../../docs/_static/ESP32.svg" height="85" alt="ESP32"> | ![alt text](../../../docs/_static/yes-button.png "开发板兼容此例程") |
| ESP32-Korvo-DU1906 | [![alt text](../../../docs/_static/esp32-korvo-du1906-v1.1-small.jpg "ESP32-Korvo-DU1906")](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/get-started-esp32-korvo-du1906.html) | <img src="../../../docs/_static/ESP32.svg" height="85" alt="ESP32"> | ![alt text](../../../docs/_static/no-button.png "开发板不兼容此例程") |
| ESP32-S2-Kaluga-1 Kit | [![alt text](../../../docs/_static/esp32-s2-kaluga-1-kit-small.png "ESP32-S2-Kaluga-1 Kit")](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s2/hw-reference/esp32s2/user-guide-esp32-s2-kaluga-1-kit.html) | <img src="../../../docs/_static/ESP32-S2.svg" height="100" alt="ESP32-S2"> | ![alt text](../../../docs/_static/no-button.png "开发板不兼容此例程") |
| ESP32-S3-Korvo-2 V3.0 | [![alt text](../../../docs/_static/esp32-s3-korvo-2-v3.0-small.png "ESP32-S3-Korvo-2 v3.0")](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/user-guide-esp32-s3-korvo-2.html) | <img src="../../../docs/_static/ESP32-S3.svg" height="100" alt="ESP32-S3"> | ![alt text](../../../docs/_static/yes-button.png "开发板兼容此例程") |

## 编译和下载

### IDF 默认分支

本例程默认 IDF 为 [Release/v4.4](https://github.com/espressif/esp-idf/tree/release/v4.4)。

### 配置

本例程默认选择的开发板是 `ESP32-S3-Korvo-2 v3.0`，请复制 `sdkconfg.defaults.esp32s3` 为 `sdkconfig.defaults`，如果需要在其他的开发板上运行此例程，则需要在 menuconfig 中选择开发板的配置，例如选择 `ESP32-Lyrat-Mini V1.1`，并复制 `sdkconfg.defaults.esp32` 为 `sdkconfig.defaults`。

```
menuconfig > Audio HAL > ESP32-Lyrat-Mini V1.1
```

### 编译和下载

请先编译版本并烧录到开发板上，然后运行 monitor 工具来查看串口输出 (替换 PORT 为端口名称)：

```
idf.py -p PORT flash monitor
```

退出调试界面使用 ``Ctrl-]``。

有关配置和使用 ESP-IDF 生成项目的完整步骤，请参阅 [《ESP-IDF 编程指南》](https://docs.espressif.com/projects/esp-idf/zh_CN/release-v4.4/esp32/get-started/index.html)。

## 如何使用例程

### 功能和用法

- 例程开始运行后，程序自动开始检测周围的背景环境声音，打印如下：

```
I (0) cpu_start: App cpu up.
I (726) spiram: SPI SRAM memory test OK
W (726) rtcinit: efuse read fail, set default blk2_version: 1, blk1_version:2

I (739) cpu_start: Pro cpu start user code
I (739) cpu_start: cpu freq: 240000000
I (739) cpu_start: Application information:
I (741) cpu_start: Project name:     test_recorder
I (747) cpu_start: App version:      v2.2-303-g80ec082b-dirty
I (753) cpu_start: Compile time:     Jan  4 2022 11:12:02
I (759) cpu_start: ELF file SHA256:  44262c7d4b164866...
I (765) cpu_start: ESP-IDF:          v4.4-dev-3622-g08414946b7-dirty
I (772) heap_init: Initializing. RAM available for dynamic allocation:
I (780) heap_init: At 3FCA4408 len 0003BBF8 (238 KiB): D/IRAM
I (786) heap_init: At 3FCE0000 len 0000EE34 (59 KiB): STACK/DRAM
I (793) heap_init: At 600FE000 len 00002000 (8 KiB): RTCRAM
I (799) spiram: Adding pool of 8192K of external SPI memory to heap allocator
I (807) spi_flash: detected chip: gd
I (811) spi_flash: flash io: qio
I (816) sleep: Configure to isolate all GPIO pins in sleep state
I (822) sleep: Enable automatic switching of GPIO sleep configuration
I (829) cpu_start: Starting scheduler on PRO CPU.
I (0) cpu_start: Starting scheduler on APP CPU.
I (859) spiram: Reserving pool of 32K of internal memory for DMA/internal allocations
I (859) SDCARD: Using 1-line SD mode, 4-line SD mode,  base path=/sdcard
I (869) SDCARD: Using 1-line SD mode
I (879) gpio: GPIO[15]| InputEn: 0| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:0
I (879) gpio: GPIO[7]| InputEn: 0| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:0
I (889) gpio: GPIO[4]| InputEn: 0| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:0
I (929) SDCARD: CID name SD!

I (1399) DRV8311: ES8311 in Slave mode
I (1409) gpio: GPIO[48]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
I (1419) ES7210: ES7210 in Slave mode
I (1429) ES7210: Enable ES7210_INPUT_MIC1
I (1429) ES7210: Enable ES7210_INPUT_MIC2
W (1429) AUDIO_BOARD: The board has already been initialized!

----------------------------- ESP Audio Platform -----------------------------
|                                                                            |
|                 ESP_AUDIO-v1.7.0-31-g5b8f999-3072767-09be8fe               |
|                     Compile date: Oct 14 2021-11:03:30                     |
------------------------------------------------------------------------------
I (1469) ESP32_S3_KORVO_2: I2S0, MCLK output by GPIO0
I (1469) wwe_example: Func:setup_player, Line:110, MEM Total:8596655 Bytes, Inter:259939 Bytes, Dram:259939 Bytes

I (1479) wwe_example: esp_audio instance is:0x3d808520

Initializing SPIFFS
Partition size: total: 3900791, used: 3775542
E (1729) I2S: register I2S object to platform failed
I (1729) ESP32_S3_KORVO_2: I2S0, MCLK output by GPIO0
I (1739) wwe_example: Recorder has been created
model_name: hilexin7q8 model_data: /srmodel/hilexin7q8/wn7q8_data
MC Quantized-8 wakeNet7: wakeNet7Q8_v2_hilexin_5_0.97_0.90, mode:2, p:3, (Dec 10 2021 20:59:49)
Initial TWO-MIC auido front-end for speech recognition, mode:0, (Dec 10 2021 11:08:00)
model_name: mn3cn model_data: /srmodel/mn3cn/mn3cn_data
SINGLE_RECOGNITION: V3.0 CN; core: 0; (Dec 16 2021 17:35:04)
I (4309) MN: ---------------------SPEECH COMMANDS---------------------
I (4309) MN: Command ID0, phrase 0: da kai kong tiao
I (4309) MN: Command ID1, phrase 1: guan bi kong tiao
I (4319) MN: Command ID2, phrase 2: zeng da feng su
I (4319) MN: Command ID3, phrase 3: jian xiao feng su
I (4329) MN: Command ID4, phrase 4: sheng gao yi du
I (4339) MN: Command ID5, phrase 5: jiang di yi du
I (4339) MN: Command ID6, phrase 6: zhi re mo shi
I (4349) MN: Command ID7, phrase 7: zhi leng mo shi
I (4349) MN: Command ID8, phrase 8: song feng mo shi
I (4359) MN: Command ID9, phrase 9: jie neng mo shi
I (4359) MN: Command ID10, phrase 10: chu shi mo shi
I (4369) MN: Command ID11, phrase 11: jian kang mo shi
I (4369) MN: Command ID12, phrase 12: shui mian mo shi
I (4379) MN: Command ID13, phrase 13: da kai lan ya
I (4389) MN: Command ID14, phrase 14: guan bi lan ya
I (4389) MN: Command ID15, phrase 15: kai shi bo fang
I (4399) MN: Command ID16, phrase 16: zan ting bo fang
I (4399) MN: Command ID17, phrase 17: ding shi yi xiao shi
I (4409) MN: Command ID18, phrase 18: da kai dian deng
I (4419) MN: Command ID19, phrase 19: guan bi dian deng
I (4429) MN: ---------------------------------------------------------
```

- 如果此时说了唤醒词（默认为 “hi, lexin”），设备则会被唤醒，并播放提示音“叮”：

```
I (93419) wwe_example: rec_engine_cb - REC_EVENT_WAKEUP_START
I (94099) wwe_example: rec_engine_cb - REC_EVENT_VAD_START
W (94099) wwe_example: voice read begin
I (94119) AMRNB_ENCODER: amrnb open
```

- 如果在说了唤醒词且听到“叮”的声音之后，说了命令词之一（如“打开空调”），则会通过日志打印出命令词索引（如 `wwe_example: command 0`），并播放语音“好的”：

```
phrase_id = 0, the prob = -7.536505
I (139389) wwe_example: rec_engine_cb - AUDIO_REC_COMMAND_DECT
W (139389) wwe_example: command 0
I (141199) wwe_example: rec_engine_cb - REC_EVENT_VAD_STOP
W (141199) wwe_example: voice read stopped
I (141199) wwe_example: File closed
I (141209) AMRNB_ENCODER: amrnb close
I (142099) wwe_example: rec_engine_cb - REC_EVENT_WAKEUP_END
```

### 日志输出

以下是本例程的完整日志。

```
I (0) cpu_start: App cpu up.
I (693) spiram: SPI SRAM memory test OK
W (693) rtcinit: efuse read fail, set default blk2_version: 1, blk1_version:2

I (706) cpu_start: Pro cpu start user code
I (706) cpu_start: cpu freq: 240000000
I (706) cpu_start: Application information:
I (708) cpu_start: Project name:     test_recorder
I (714) cpu_start: App version:      v2.2-303-g80ec082b-dirty
I (720) cpu_start: Compile time:     Jan  4 2022 11:12:02
I (726) cpu_start: ELF file SHA256:  44262c7d4b164866...
I (732) cpu_start: ESP-IDF:          v4.4-dev-3622-g08414946b7-dirty
I (740) heap_init: Initializing. RAM available for dynamic allocation:
I (747) heap_init: At 3FCA4408 len 0003BBF8 (238 KiB): D/IRAM
I (753) heap_init: At 3FCE0000 len 0000EE34 (59 KiB): STACK/DRAM
I (760) heap_init: At 600FE000 len 00002000 (8 KiB): RTCRAM
I (766) spiram: Adding pool of 8192K of external SPI memory to heap allocator
I (774) spi_flash: detected chip: gd
I (778) spi_flash: flash io: qio
I (783) sleep: Configure to isolate all GPIO pins in sleep state
I (789) sleep: Enable automatic switching of GPIO sleep configuration
I (796) cpu_start: Starting scheduler on PRO CPU.
I (0) cpu_start: Starting scheduler on APP CPU.
I (826) spiram: Reserving pool of 32K of internal memory for DMA/internal allocations
I (826) SDCARD: Using 1-line SD mode, 4-line SD mode,  base path=/sdcard
I (836) SDCARD: Using 1-line SD mode
I (846) gpio: GPIO[15]| InputEn: 0| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:0
I (846) gpio: GPIO[7]| InputEn: 0| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:0
I (856) gpio: GPIO[4]| InputEn: 0| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:0
I (896) SDCARD: CID name SD!

I (1366) DRV8311: ES8311 in Slave mode
I (1376) gpio: GPIO[48]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
I (1386) ES7210: ES7210 in Slave mode
I (1396) ES7210: Enable ES7210_INPUT_MIC1
I (1396) ES7210: Enable ES7210_INPUT_MIC2
W (1396) AUDIO_BOARD: The board has already been initialized!

----------------------------- ESP Audio Platform -----------------------------
|                                                                            |
|                 ESP_AUDIO-v1.7.0-31-g5b8f999-3072767-09be8fe               |
|                     Compile date: Oct 14 2021-11:03:30                     |
------------------------------------------------------------------------------
I (1436) ESP32_S3_KORVO_2: I2S0, MCLK output by GPIO0
I (1436) wwe_example: Func:setup_player, Line:110, MEM Total:8596655 Bytes, Inter:259939 Bytes, Dram:259939 Bytes

I (1446) wwe_example: esp_audio instance is:0x3d808520

Initializing SPIFFS
Partition size: total: 3900791, used: 3775542
E (1696) I2S: register I2S object to platform failed
I (1696) ESP32_S3_KORVO_2: I2S0, MCLK output by GPIO0
I (1706) wwe_example: Recorder has been created
model_name: hilexin7q8 model_data: /srmodel/hilexin7q8/wn7q8_data
MC Quantized-8 wakeNet7: wakeNet7Q8_v2_hilexin_5_0.97_0.90, mode:2, p:3, (Dec 10 2021 20:59:49)
Initial TWO-MIC auido front-end for speech recognition, mode:0, (Dec 10 2021 11:08:00)
model_name: mn3cn model_data: /srmodel/mn3cn/mn3cn_data
SINGLE_RECOGNITION: V3.0 CN; core: 0; (Dec 16 2021 17:35:04)
I (4276) MN: ---------------------SPEECH COMMANDS---------------------
I (4276) MN: Command ID0, phrase 0: da kai kong tiao
I (4276) MN: Command ID1, phrase 1: guan bi kong tiao
I (4286) MN: Command ID2, phrase 2: zeng da feng su
I (4286) MN: Command ID3, phrase 3: jian xiao feng su
I (4296) MN: Command ID4, phrase 4: sheng gao yi du
I (4306) MN: Command ID5, phrase 5: jiang di yi du
I (4306) MN: Command ID6, phrase 6: zhi re mo shi
I (4316) MN: Command ID7, phrase 7: zhi leng mo shi
I (4316) MN: Command ID8, phrase 8: song feng mo shi
I (4326) MN: Command ID9, phrase 9: jie neng mo shi
I (4326) MN: Command ID10, phrase 10: chu shi mo shi
I (4336) MN: Command ID11, phrase 11: jian kang mo shi
I (4336) MN: Command ID12, phrase 12: shui mian mo shi
I (4346) MN: Command ID13, phrase 13: da kai lan ya
I (4356) MN: Command ID14, phrase 14: guan bi lan ya
I (4356) MN: Command ID15, phrase 15: kai shi bo fang
I (4366) MN: Command ID16, phrase 16: zan ting bo fang
I (4366) MN: Command ID17, phrase 17: ding shi yi xiao shi
I (4376) MN: Command ID18, phrase 18: da kai dian deng
I (4386) MN: Command ID19, phrase 19: guan bi dian deng
I (4396) MN: ---------------------------------------------------------

I (6606) wwe_example: rec_engine_cb - REC_EVENT_WAKEUP_START
I (7346) wwe_example: rec_engine_cb - REC_EVENT_VAD_START
W (7346) wwe_example: voice read begin
I (7346) AMRNB_ENCODER: amrnb open
phrase_id = 0, the prob = -12.604690
I (8436) wwe_example: rec_engine_cb - AUDIO_REC_COMMAND_DECT
W (8436) wwe_example: command 0
I (10176) wwe_example: rec_engine_cb - REC_EVENT_VAD_STOP
W (10186) wwe_example: voice read stopped
I (10186) wwe_example: File closed
I (10196) AMRNB_ENCODER: amrnb close
I (11076) wwe_example: rec_engine_cb - REC_EVENT_WAKEUP_END
```

## 故障排除

此应用程序在以 ESP32 为核心的开发板上运行可能会触发任务看门狗。

## 技术支持

请按照下面的链接获取技术支持：

- 技术支持参见 [esp32.com](https://esp32.com/viewforum.php?f=20) 论坛
- 故障和新功能需求，请创建 [GitHub issue](https://github.com/espressif/esp-adf/issues)

我们会尽快回复。
