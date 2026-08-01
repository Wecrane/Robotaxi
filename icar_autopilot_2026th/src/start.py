#!/usr/bin/env python
# -*- encoding: utf-8 -*-
"""
语音指令 + 视觉识别 → 启动小车
流程：
  1. 语音指令 → LLM 解析 → 圈数配置
  2. 摄像头拍照识别 → 用户确认后写入 alertTarget
  3. 写入圈数配置到 config.json
  4. 启动小车程序
注意：摄像头在第 2 步确认后即释放，确保不占用小车程序资源
"""

import json
import os
import subprocess
import sys
import tempfile
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from visual import VisualLLM, LABEL_DICT, update_alert_target

# 终端颜色
COUT_RED = "\033[91m"
COUT_GREEN = "\033[92m"
COUT_YELLOW = "\033[93m"
COUT_REST = "\033[0m"

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
CONFIG_PATH = os.path.abspath(os.path.join(BASE_DIR, "../res/config.json"))
BUILD_DIR = os.path.join(BASE_DIR, "..", "build")
ICAR_PATH = os.path.join(BUILD_DIR, "icar")



def run_speech_flow():
    """语音指令解析流程，返回 (lapConfig, totalLaps) 或 None"""
    from speech.llm import LLM
    from speech.speech import parsedToLapConfig, normalizeTasks

    def _describe_lap(lap):
        if lap["park"]:
            if lap.get("parkSpot") == 0:
                return "停车场(穿过)"
            return "停车场(停车位 {})".format(lap['parkSpot'])
        if lap["busy"]:
            pos = "中间" if lap["busyStopPoint"] == 1 else "出口"
            return "施工区({})".format(pos)
        if lap.get("fork") or lap.get("yfork"):
            side = "左侧" if lap.get("yforkLeft") else "右侧"
            return "岔路口({})".format(side)
        return "未知"

    llm = LLM()

    while True:
        instruction = input("\n请输入行驶指令: ").strip()
        if not instruction:
            continue

        print("\n原始指令: {}\n".format(instruction))

        result = llm.parseInstruction(instruction)
        if not result:
            print("{}解析失败！请检查 API Key 是否有效{}".format(COUT_RED, COUT_REST))
            continue

        print("LLM 解析结果: {}\n".format(json.dumps(result, ensure_ascii=False)))

        normalizeTasks(result)
        print("修正后: {}\n".format(json.dumps(result, ensure_ascii=False)))

        lapConfig = parsedToLapConfig(result)
        totalLaps = len(result["tasks"])
        print("映射为每圈配置:")
        print(json.dumps(lapConfig, ensure_ascii=False, indent=2))
        print("  总圈数: {}".format(totalLaps))
        for lapNum in sorted(lapConfig.keys()):
            lap = lapConfig[lapNum]
            info = _describe_lap(lap)
            print("  {} → {}".format(lapNum, info))

        confirm = input("\n解析结果是否正确？(y/n): ").strip().lower()
        if confirm == "y":
            return lapConfig, totalLaps
        print("已取消，请重新输入指令。")


def run_visual_flow(vllm):
    """摄像头拍照识别循环：拍一张 → 识别 → 确认"""
    import cv2

    print("\n" + "=" * 60)
    print("    摄像头拍照识别 — 按提示确认是否写入配置")
    print("=" * 60)

    cap = cv2.VideoCapture(0)
    if not cap.isOpened():
        print("{}[摄像头] 无法打开摄像头{}".format(COUT_RED, COUT_REST))
        return None
    time.sleep(0.5)

    while True:
        # 清除摄像头驱动内部的帧缓冲区，确保读到最新画面
        for _ in range(5):
            cap.grab()
        ret, frame = cap.read()
        if not ret:
            print("{}[摄像头] 读取画面失败，尝试重新拍摄...{}".format(COUT_RED, COUT_REST))
            continue

        with tempfile.NamedTemporaryFile(suffix=".jpg", delete=False) as tmp:
            temp_path = tmp.name
        cv2.imwrite(temp_path, frame)

        print("\n{}[摄像头] 正在识别...{}".format(COUT_YELLOW, COUT_REST))
        label = vllm.recognize(temp_path)
        os.remove(temp_path)

        if label:
            name = LABEL_DICT.get(label, label)
            print("{}[摄像头] 识别结果: {} ({}){}".format(COUT_GREEN, label, name, COUT_REST))
        else:
            print("{}[摄像头] 识别失败{}".format(COUT_RED, COUT_REST))

        confirm = input("\n是否将此识别结果写入配置文件？(y/n): ").strip().lower()
        if confirm == "y":
            if label:
                update_alert_target(label, CONFIG_PATH)
                print("{}已写入配置{}".format(COUT_GREEN, COUT_REST))
            break
        print("已取消，重新拍摄识别...")

    cap.release()
    print("{}[摄像头] 已释放{}".format(COUT_GREEN, COUT_REST))
    return label


def start_car_program():
    """启动小车程序（继承当前 SSH 会话的 DISPLAY，支持 X11 转发）"""
    if not os.path.exists(ICAR_PATH):
        print("{}\n未找到小车程序: {}{}".format(COUT_RED, ICAR_PATH, COUT_REST))
        return False

    # 检查 DISPLAY 环境变量（X11 转发需要）
    display = os.environ.get("DISPLAY", "")
    if display:
        print("{}[X11] DISPLAY={} — 摄像头画面将通过 X11 转发{}".format(COUT_GREEN, display, COUT_REST))
    else:
        print("{}[X11] 未检测到 DISPLAY，摄像头窗口可能无法显示{}".format(COUT_YELLOW, COUT_REST))
        print("{}        请使用 ssh -X root@192.168.137.199 重新连接{}".format(COUT_YELLOW, COUT_REST))

    boot_needed = input("\n是否需要先启动boot？(y/n): ").strip().lower()
    if boot_needed == "y":
        # 直接在后台启动 boot（不依赖 gnome-terminal）
        boot_path = os.path.join(BUILD_DIR, "boot")
        if os.path.exists(boot_path):
            subprocess.Popen(
                [boot_path],
                cwd=BUILD_DIR,
                start_new_session=True,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
            )
            print("已启动 boot，等待 3 秒...")
            time.sleep(3)
        else:
            print("{}未找到 boot: {}{}".format(COUT_RED, boot_path, COUT_REST))

    print("\n正在启动小车程序: {}".format(ICAR_PATH))
    print("{}摄像头画面请查看 X11 弹窗（若无可检查 ssh -X）{}".format(COUT_GREEN, COUT_REST))
    print("{}按 Ctrl+C 停止小车{}\n".format(COUT_YELLOW, COUT_REST))

    # 前台运行 icar（阻塞），X11 画面通过 SSH 转发到 Windows
    try:
        subprocess.run(
            [ICAR_PATH],
            cwd=BUILD_DIR,
        )
    except KeyboardInterrupt:
        print("\n{}小车程序已停止{}".format(COUT_YELLOW, COUT_REST))

    return True


def main():
    print("=" * 60)
    print("    智能小车启动程序 — 语音 + 视觉")
    print("=" * 60)

    # ========== 1. 语音指令解析 ==========
    print("\n{}>>> 第一步：语音指令解析{}".format(COUT_YELLOW, COUT_REST))
    result = run_speech_flow()
    if result is None:
        sys.exit(1)
    lapConfig, totalLaps = result

    # ========== 2. 摄像头视觉识别 ==========
    print("\n{}>>> 第二步：拍照识别场景{}".format(COUT_YELLOW, COUT_REST))
    vllm = VisualLLM()
    label = run_visual_flow(vllm)
    if label:
        print("最终识别标签: {}".format(label))

    # ========== 3. 写入圈数配置 ==========
    print("\n{}>>> 第三步：写入圈数配置{}".format(COUT_YELLOW, COUT_REST))
    from speech.speech import updateConfigJson
    updateConfigJson(lapConfig, totalLaps, CONFIG_PATH)

    # ========== 4. 启动小车 ==========
    print("\n{}>>> 第四步：启动小车程序{}".format(COUT_YELLOW, COUT_REST))
    start_car_program()


if __name__ == "__main__":
    main()
