# 🏎️ iCar Autopilot 2026 — 智慧城市 Robotaxi 挑战赛

> **全国大学生智能汽车竞赛 · 创意赛 · 智慧城市 Robotaxi 挑战赛**
>
> 纯视觉自动驾驶系统，基于 EdgeBoard T710 + GD32F1 双控架构

[![Platform](https://img.shields.io/badge/platform-EdgeBoard%20T710-blue)]()
[![Language](https://img.shields.io/badge/language-C%2B%2B17%20%7C%20C11-orange)]()
[![Framework](https://img.shields.io/badge/framework-OpenCV%20%7C%20ONNX%20%7C%20Paddle-green)]()
[![MCU](<https://img.shields.io/badge/MCU-GD32F1%20(STM32F10X)-red>)]()

---

## 📋 目录

- [系统架构](#系统架构)
- [目录结构](#目录结构)
- [硬件平台](#硬件平台)
- [通信协议](#通信协议)
- [快速开始](#快速开始)
- [配置说明](#配置说明)
- [赛道元素](#赛道元素)
- [状态机设计](#状态机设计)
- [开发指南](#开发指南)
- [已知问题](#已知问题)

---

## 系统架构

```
┌──────────────────────────────────────────────────────┐
│                    上位机 EdgeBoard T710               │
│  ┌─────────┐  ┌──────────┐  ┌──────────────────────┐ │
│  │ camera  │  │ YOLOv3   │  │ FSM 状态机 (10种)     │ │
│  │ 图像采集 │──│ AI 推理   │──│ park/cross/obstacle… │ │
│  └─────────┘  └──────────┘  └─────────┬────────────┘ │
│                                        │              │
│  ┌─────────────────────────────────────┼────────────┐ │
│  │ Boot (boot.cpp)                     │            │ │
│  │  UART 收发 ←──→ TCP Server (8899) ←→│ icar 主控  │ │
│  └──────────────────┬──────────────────────────────┘ │
└─────────────────────┼─────────────────────────────────┘
                      │ UART 115200
┌─────────────────────┼─────────────────────────────────┐
│              下位机 GD32F1 智控板                       │
│  ┌──────────┐  ┌──────────┐  ┌──────────────────────┐ │
│  │ Encoder  │  │ PID 控速  │  │ 安全监控              │ │
│  │ 脉冲计数  │──│ 闭环控制  │──│ 断线/掉线/失速保护    │ │
│  └──────────┘  └──────────┘  └──────────────────────┘ │
│  ┌──────────┐  ┌──────────┐  ┌──────────────────────┐ │
│  │ Servo    │  │ Motor    │  │ 遥测上报 (150ms)      │ │
│  │ 舵机控制  │  │ 电机驱动  │  │ 电池/车速/故障码      │ │
│  └──────────┘  └──────────┘  └──────────────────────┘ │
└──────────────────────────────────────────────────────┘
```

**核心特征**：

- **上位机** 30fps 主循环：图像采集 → AI 推理 → 车道线提取 → FSM 决策 → 控制指令下发
- **下位机** 1ms 定时中断：PID 速度闭环 → 舵机控制 → 编码器采样 → 安全监控
- **双线程**：AI 推理线程 + FSM 控制线程，通过 `params->resultsSnapshot` 无锁快照同步

---

## 目录结构

```
code/
├── icar_autopilot_2026th/        # 上位机 (EdgeBoard C++17)
│   ├── CMakeLists.txt            # CMake 构建配置
│   ├── include/
│   │   ├── icar.hpp              # 主控类 (30fps 主循环)
│   │   ├── com/
│   │   │   ├── uart.hpp          # UART 直连通信协议
│   │   │   ├── client.hpp        # TCP Socket 通信 (经 Boot 转发)
│   │   │   └── server.hpp        # TCP Server (Boot 内)
│   │   ├── ctrl/
│   │   │   ├── predeal.hpp       # 图像预处理 (车道线提取)
│   │   │   ├── center.hpp        # 控制中心 (路径规划)
│   │   │   └── motion.hpp        # 运动控制 (速度/方向)
│   │   ├── fsm/
│   │   │   ├── busy.hpp          # 施工区绕行
│   │   │   ├── cross.hpp         # 斑马线停车
│   │   │   ├── fork.hpp          # 停车场岔路
│   │   │   ├── obstacle.hpp      # 锥桶/障碍物避障
│   │   │   ├── park.hpp          # 停车场泊车
│   │   │   ├── slow.hpp          # 限速区
│   │   │   ├── station.hpp       # 停靠站
│   │   │   ├── stop.hpp          # 停车区
│   │   │   ├── yfork.hpp         # Y型岔路口
│   │   │   └── manualControl.hpp # 手动接管
│   │   └── utils/
│   │       ├── params.hpp        # 全局参数 (FSM 共享)
│   │       ├── detection.hpp     # YOLOv3 目标检测
│   │       ├── show.hpp          # UI 显示
│   │       └── loop.hpp          # 子线程管理
│   ├── src/
│   │   ├── icar.cpp              # 主程序入口
│   │   ├── start.py              # Python 启动脚本
│   │   ├── ctrl/                 # 控制模块实现
│   │   ├── fsm/                  # 状态机实现
│   │   ├── speech/               # 语音播报
│   │   ├── tool/                 # 工具 (boot/camera/img2video…)
│   │   └── visual/               # 可视化
│   └── res/
│       ├── config.json           # ⭐ 主配置文件
│       ├── models/               # AI 模型文件
│       ├── calibration/          # 相机标定
│       └── samples/              # 测试样本
│
├── Code_GD32F1_CarDo/            # 下位机 (GD32F1 C语言)
│   ├── HARDWARE/
│   │   ├── Encoder.c/h           # 编码器 (TIM3 脉冲计数)
│   │   ├── Motor.c/h             # 电机驱动 + 断线保护
│   │   ├── Servo.c/h             # 舵机控制
│   │   ├── Pid.c/h               # 增量式 PID
│   │   ├── Usb.c/h               # UART 通信协议 + CRC8
│   │   ├── Icar.c/h              # 下位机主逻辑
│   │   ├── Gpio.c/h              # GPIO + 按键消抖
│   │   ├── Soc.c/h               # 电量检测
│   │   ├── Imu.c/h               # IMU (预留)
│   │   ├── Inspector.c/h         # 自检模块
│   │   ├── Flash.c/h             # 参数存储
│   │   ├── Rgb.c/h               # RGB LED
│   │   ├── Timer.c/h             # 定时器
│   │   └── Delay.c/h             # 延时
│   ├── STM32F10X_FWLIB/          # STM32 标准外设库
│   ├── CORE/                     # Cortex-M3 启动文件
│   └── USER/
│       ├── main.c/h              # 主程序 (1ms TIM2 中断)
│       └── stm32f10x_conf.h      # 外设配置
│
└── docs/                         # 📚 项目文档
    ├── task_plan.md              # 问题修复优先级规划
    ├── 参赛快速参考.md            # 比赛速查
    ├── 模型训练流程.md            # AI 模型训练指南
    ├── API配置指南.md             # 大模型 API 配置
    └── icar_autopilot_2026th_完全参赛指南与技术手册_V5.md
```

---

## 硬件平台

| 组件   | 型号                    | 说明                        |
| ------ | ----------------------- | --------------------------- |
| 计算卡 | EdgeBoard T710          | ARM 架构，运行 Ubuntu       |
| MCU    | GD32F1 (STM32F103 兼容) | 智控板，1ms 实时控制        |
| 摄像头 | USB 广角                | 前置，用于车道线 + 目标检测 |
| 底盘   | 阿克曼转向车模          | 后轮驱动 + 前轮舵机转向     |
| 编码器 | 增量式                  | 后轮测速，用于 PID 闭环     |
| 舵机   | 标准舵机                | PWM 500~2500，控制前轮转角  |
| 电池   | 7.4V 锂电池             | 电量检测 + 低压告警         |

---

## 通信协议

### 帧格式 (UART 115200, 8N1)

```
┌────────┬────────┬────────┬─────────────────┬────────┐
│ 0x42   │ ADDR   │ LEN    │ DATA[0…LEN-4]   │ CRC8   │
│ 帧头   │ 地址   │ 帧长   │ 数据载荷         │ 校验   │
└────────┴────────┴────────┴─────────────────┴────────┘
```

- **帧头**：固定 `0x42`
- **帧长**：包含从 ADDR 到 CRC8 的全部字节，范围 4~30
- **校验**：CRC8-MAXIM（多项式 `0x31`），查表法

### 地址映射

|  地址  |  方向   | 含义                                 |
| :----: | :-----: | ------------------------------------ |
| `0x00` | 上位→下 | 心跳信号 (Boot)                      |
| `0x01` | 上位→下 | 速度+舵机控制 (`float32` + `uint16`) |
| `0x02` | 上位→下 | 速度模式切换 (开环/闭环)             |
| `0x03` |  双向   | 舵机阈值校准                         |
| `0x04` | 上位→下 | 蜂鸣器音效                           |
| `0x05` | 上位→下 | LED 灯效                             |
| `0x06` | 下位→上 | 按键输入 (1=短按 2=长按)             |
| `0x07` | 下位→上 | 电池信息 (电量% + 电压 float32)      |
| `0x08` | 下位→上 | 车速反馈 (m/s float32)               |
| `0x0A` | 上位→下 | 使能遥测上报                         |
| `0x0B` | 下位→上 | 自检状态 (步骤 + 故障码)             |

---

## 快速开始

### 环境要求

- **上位机**：Ubuntu 18.04+ / EdgeBoard 镜像, CMake 3.4+, GCC 支持 C++17
- **依赖库**：OpenCV, ONNX Runtime, libserial, glib-2.0, ppnc (Paddle)
- **下位机**：Keil MDK-ARM / ARM GCC, STM32F10x 标准外设库

### 编译上位机

```bash
cd icar_autopilot_2026th/build
cmake ..
make -j4
```

生成的可执行文件：

- `boot` — 开机自启程序（UART ↔ TCP 桥接）
- `icar` — 主控程序（AI 推理 + FSM 决策）
- `camera` — 相机测试工具
- `calibration` — 标定工具
- `collection` — 数据采集工具
- `detection` — 检测测试工具
- `img2video` — 图像合成视频
- `imging` — 图像标注工具

### 编译下位机

用 Keil MDK 打开 `Code_GD32F1_CarDo/USER/SASU_CarDo.uvprojx`，编译烧录。

### 运行

```bash
# 1. 启动 Boot (UART ↔ TCP 桥接)
./boot

# 2. 按下位机按键 → Boot 自动启动 icar
#    或手动启动
./icar ../res/config.json
```

---

## 配置说明

主配置文件 `res/config.json`：

```jsonc
{
  "通用配置参数": {
    "velLow": 0.6,        // 低速 m/s
    "velHigh": 0.8,       // 高速 m/s
    "velSlow": 0.35,      // 限速区速度
    "velPark": 0.5,       // 停车场速度
    "velCross": 0.7,      // 斑马线速度
    "runP1": 2.2,         // 直道 P 参数
    "turnP": 3.5,         // 弯道 P 参数
    "turnD": 1.5,         // 弯道 D 参数
    "debug": false,       // 调试窗口
    "model": "../res/models/yolov3_mobilenet_v1"
  },
  "圈数配置": {
    "totalLaps": 3,       // 总圈数
    "crossStop": 1        // 第几圈斑马线停车
  },
  "每圈功能使能配置": {
    "lap1": { "park": true, "slow": true, "cross": true, … },
    "lap2": { "busy": true, "manualTakeover": true, … },
    "lap3": { "yfork": true, "stop": true, … }
  }
}
```

> ⚠️ 修改配置后务必检查 JSON 格式，格式错误会导致程序启动失败。

---

## 赛道元素

| 元素       |  FSM 状态  | 检测方式                 | 控制策略            |
| ---------- | :--------: | ------------------------ | ------------------- |
| 🅿️ 停车场  |   `PARK`   | YOLOv3 检测车位 + 车道线 | 侧方泊车            |
| 🚧 施工区  |   `BUSY`   | 锥桶绕行 / 直线加速      | 避开锥桶区域        |
| 🐢 限速区  |   `SLOW`   | 限速标志识别             | 降速至 0.35 m/s     |
| 🚦 斑马线  |  `CROSS`   | 斑马线检测               | 停车 1.5m 后驶离    |
| 🛑 停车区  |   `STOP`   | 停止标志                 | 定点停车            |
| 🔀 岔路    |   `FORK`   | YOLOv3 岔路检测          | 按配置方向转        |
| 🅱️ Y型岔路 |  `YFORK`   | Y 型路口检测             | 按 `yforkLeft` 配置 |
| 🏁 停靠站  | `STATION`  | 站台检测                 | 减速靠边            |
| 🚧 锥桶    | `OBSTACLE` | 锥桶/行人检测            | 全局避障            |

---

## 状态机设计

```
                    ┌─────────┐
                    │  NORMAL │ ◄── 正常循线
                    └────┬────┘
         ┌─────────┬────┼────┬─────────┬─────────┐
         ▼         ▼    ▼    ▼         ▼         ▼
      PARK      BUSY  SLOW CROSS    FORK      STOP
      泊车      施工区 限速 斑马线   岔路      停车

      STATION   YFORK  OBSTACLE   MANUAL
      停靠站    Y岔路  全局避障   手动接管
```

- 所有状态共享 `params->resultsSnapshot`（帧首统一快照，线程安全）
- 圈数变更时自动复位所有 FSM 状态
- 手动接管优先级最高，接管时跳过所有 FSM 检测

---

## 开发指南

### 安全特性 (P0/P1 已修复)

| 特性           | 说明                                  |
| -------------- | ------------------------------------- |
| 编码器断线保护 | 200ms 检测窗口 → 紧急停车 + errorCode |
| 上位机掉线回中 | 3s 超时 → 舵机回中 + 速度清零         |
| 冲刺绕过保护   | 掉线时自动清除 sprintEnable           |
| 数据竞争消除   | resultsSnapshot 帧首快照              |
| 单边避障修复   | 弱边时降级为 curtailTracking          |
| 终点停车       | 编码器里程计 1.5m 刹车                |

### 代码规范

- 上位机：C++17，命名遵循 Google C++ Style
- 下位机：C11，寄存器操作使用标准外设库
- 跨线程数据：使用 `std::atomic` 或帧首快照
- 通信帧：必须使用 CRC8 校验，禁止裸发

### Git 工作流

```bash
# 修改 → 编译测试 → 提交
git add -A
git commit -m "类型: 简短描述"
```

---

## 已知问题

详见 `docs/task_plan.md`，当前状态：

|   级别   |  已完成   | 已跳过 |       待处理       |
| :------: | :-------: | :----: | :----------------: |
| P0 致命  |    5/5    |   —    |         —          |
| P1 严重  |    6/6    |   —    |         —          |
| P2 中等  |    2/6    |   4    |   P2-6(裁判协议)   |
| P3 轻微  |    1/5    |   3    | P3-5(Uart超时复位) |
| **总计** | **14/22** | **7**  |       **1**        |

最后更新：2026-07-01

---

## 相关链接

- 赛项官网：[全国大学生智能汽车竞赛](https://www.smartcarrace.com)
- 百度 AI Studio：[Paddle 模型训练](https://aistudio.baidu.com)
- QQ 交流群：942215078

---

<p align="center">Made with ❤️ for Robotaxi Challenge 2026</p>
