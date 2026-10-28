# taoAirMouse — ESP32 体感空中鼠标

基于 ESP32 + MPU6050 的蓝牙（BLE）体感鼠标。挥动手持设备即可控制电脑光标，板载按键实现左/右键点击，无需接收器或驱动，配对后即被识别为标准蓝牙鼠标。

## 功能

- **体感移动光标**：读取 MPU6050 陀螺仪角速度，映射为鼠标 X/Y 位移
- **蓝牙 BLE 无线连接**：通过 BLE HID 协议直接连接电脑、手机、平板，免驱
- **左键 / 右键**：两个按键，内部上拉，按下即点击
- **死区过滤**：微小抖动不产生光标移动，避免光标漂移
- **平滑处理**：指数移动平均（EMA）抑制抖动，移动更顺滑
- **串口调试**：实时输出位移量与按键状态

## 硬件

| 元件 | 型号 / 说明 |
| --- | --- |
| 主控 | ESP32 系列开发板（PlatformIO 目标板：`esp32-c3-devkitm-1`） |
| 姿态传感器 | MPU6050（三轴陀螺仪 + 三轴加速度计，I2C 接口） |
| 按键 | 2 个轻触按键（左键、右键），使用内部上拉 |
| 供电 | USB 5V |

### 引脚连接

| 功能 | ESP32 引脚 | 备注 |
| --- | --- | --- |
| I2C SDA | GPIO 21 | `Wire.begin(21, 20)` |
| I2C SCL | GPIO 20 | |
| 左键 | GPIO 5 | `INPUT_PULLUP`，按下接地 |
| 右键 | GPIO 10 | `INPUT_PULLUP`，按下接地 |

按键一端接对应 GPIO，另一端接 GND，按下时读取为低电平。

## 软件

- 开发框架：[PlatformIO](https://platformio.org/) + Arduino
- 依赖库（`platformio.ini` 自动安装）：
  - `jrowberg/I2Cdevlib-MPU6050`
  - `jrowberg/I2Cdevlib-Core`
  - `t-vk/ESP32 BLE Mouse`

### 构建与烧录

```bash
# 安装 PlatformIO CLI 后
pio run              # 编译
pio run -t upload    # 烧录
pio device monitor   # 打开串口监视器（115200）
```

或使用 VS Code 的 PlatformIO 插件：打开项目后点击 Build / Upload。

## 使用

1. 烧录固件，设备上电。
2. 电脑蓝牙中搜索并配对名为 **AirMouse** 的设备。
3. 配对成功后，挥动设备即可移动光标，按左/右键对应点击。
4. 串口（115200）可查看 `dx` / `dy` 位移量与按键状态，便于调试。

## 实现说明

- 主循环 `loop()` 中每 10 ms 读取一次 MPU6050 的原始六轴数据（约 100 Hz）。
- 陀螺仪 X 轴控制水平位移，Y 轴取反后控制垂直位移，除以 800 降低灵敏度。
- 位移绝对值小于 3 时置零（死区），抑制静止抖动。
- 平滑采用 4:1 的 EMA：`avg = (avg * 4 + new) / 5`。
- 仅当 BLE 已连接（`bleMouse.isConnected()`）时才处理输入与发送。

## 目录结构

```
.
├── platformio.ini        # PlatformIO 工程配置与依赖
├── src/main.cpp          # 主程序
├── include/              # 头文件目录
├── lib/                  # 私有库目录
└── test/                 # 测试目录
```

## 可改进方向

- 增加滚轮（利用加速度计/按键组合）
- 增加灵敏度调节与开关（如单击锁定光标）
- 加入电池管理与电量上报（`BleMouse` 支持电池电量参数）
- 静止检测与自动休眠，降低功耗
