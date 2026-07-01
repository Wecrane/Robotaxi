---
name: smartcar-robotaxi-guide
description: 智慧城市Robotaxi挑战赛参赛完全指南。涵盖硬件搭建、镜像烧录、模型训练、代码部署、比赛规则等全流程。当用户询问关于智能汽车竞赛、Robotaxi、EdgeBoard、Paddle模型训练、车模组装、比赛规则等问题时使用。触发词：智能车、Robotaxi、无人驾驶、智慧城市、完全模型、EdgeBoard、T710、智控板、赛曙、YOLOv3、Paddle、AI Studio
user-invocable: true
allowed-tools: "Read Write Edit Bash Glob Grep"
metadata:
  version: "2.0.0"
  author: "基于赛曙科技2026年文档+116个AI Agent深度代码审查"
---

# 智慧城市 Robotaxi 挑战赛 — 参赛完全指南 (Skill)

> **赛项**：第二十一届全国大学生智能汽车竞赛－创意赛·智慧城市 Robotaxi 挑战赛
> **QQ群**：942215078
> **本Skill所有引用文件均在本目录(code/)内，不依赖外部路径**

---

## ⚠️ 关键信息速查

| 项目 | 值 |
|------|-----|
| EdgeBoard IP | **10.25.139.199** (WiFi固定) |
| 电脑 IP | **10.25.139.188** (WiFi固定) |
| SSH | `ssh root@10.25.139.199` 密码 root |
| VNC | `10.25.139.199:5902` 密码 root |
| Samba | `\\10.25.139.199\root\root\workspace` 密码 root |
| 代码路径(EdgeBoard) | `~/workspace/icar_autopilot_2026th/` |
| 完整技术手册 | `docs/完全参赛指南与技术手册_V5.md` |

> ⚠️ 断电后须 `date -s "YYYYMMDD HHMM"` 校准时钟
> ⚠️ 运行 `./icar` 前必须垫起车模

---

## 目录结构（本code/文件夹）

```
code/
├── icar_autopilot_2026th/               ← 上位机源码(54文件)
│   ├── include/  (com/ ctrl/ fsm/ utils/)
│   ├── src/      (ctrl/ fsm/ speech/ tool/ visual/)
│   └── res/      (config.json models/)
├── Code_GD32F1_CarDo/                   ← 下位机源码(40文件)
│   ├── HARDWARE/ (Motor/Servo/Encoder/Usb/Icar/Pid/...)
│   └── USER/     (main.c Keil工程)
├── docs/                                ← 📚 所有文档
│   ├── 完全参赛指南与技术手册_V5.md      ← ⭐主技术手册(必读)
│   ├── 账号密码速查.md
│   ├── API配置指南.md
│   ├── 模型训练流程.md
│   └── 参赛快速参考.md
└── skills/                              ← 🧠 Skill系统
    ├── SKILL.md                         ← 本文件
    └── planning-with-files-zh.md
```

---

## 快速导航（按问题类型）

### 🔌 硬件/网络问题 → 见上方"关键信息速查"表
### 🔧 代码问题 → 看 `docs/完全参赛指南与技术手册_V5.md` 第10-14章
### 📋 比赛规则 → 同手册第1-4章
### 🏗️ 模型训练 → 同手册第7-8章 + `docs/模型训练流程.md`
### 🐛 Bug修复 → 同手册第14章 + 第21-30章
### 🔑 账号密码 → `docs/账号密码速查.md`
### 🌐 API配置 → `docs/API配置指南.md`

---

## 编译运行

```bash
ssh root@10.25.139.199
cd ~/workspace/icar_autopilot_2026th/build
cmake .. && make -j4
./icar                           # 纯自动
# 或
cd ~/workspace/icar_autopilot_2026th/src
python3 start.py                 # 含大模型
```

## 核心架构

上位机30fps主循环: 采集→预处理→赛道检测→FSM×9→路径拟合→PD控制→TCP→UART→下位机
下位机: TIM2 1ms中断+while(1)主循环, USART1 115200帧解析, PID闭环电机+50Hz舵机
通信协议: [0x42][ADDR][LEN][DATA...][checksum], 0x01是唯一完美控车帧

## 已知关键Bug（赛前必修）

| 严重度 | 问题 | 位置 | 工时 |
|:---:|------|------|:---:|
| 🚨 | Obstacle单边跳过避障 | obstacle.cpp:39 | 2h |
| 🚨 | 数据竞争 | icar.hpp runFsm | 1h |
| 🚨 | 编码器无保护(下位机) | Encoder.c | 3h |
| 🚨 | sprint劫持(下位机) | Motor.c/Icar.c | 2h |
| 🚨 | 舵机不回中(下位机) | Usb.c | 1h |
| 🔴 | sendHeart垃圾数据 | client.hpp:277 | 0.5h |
| 🔴 | lineArea死代码 | center.cpp | 1h |
| 🔴 | Slow无超时退出 | slow.cpp | 0.5h |
| 🔴 | 终点停车无测距 | cross.cpp | 4h |

完整问题清单见手册第29-30章。
