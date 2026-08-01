#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
手动控制客户端 — Windows 端运行
通过 TCP 连接小车 (192.168.137.199:8080)，发送键盘控制指令。

特性：
  - 实时按键响应：按住 W 前进，松开即停（非切换模式）
  - 组合键支持：W+A = 前进+左转，W+D = 前进+右转
  - 无摄像头弹窗：直接观看 start.py 的 X11 四合一画面

用法:
    python manual_client.py [--host 192.168.137.199] [--port 8080]

键盘映射:
    W             前进（按住不放，松开即停）
    S             后退（按住不放，松开即停）
    A             左转（按住不放，松开回正）
    D             右转（按住不放，松开回正）
    W+A / W+D 等  组合键（同时按住多个键）
    Space         紧急停车 (STOP)
    R             返回自动模式 (RETURN)
    Q / Esc       退出客户端

依赖：Windows 自带 Python 标准库
"""

import socket
import argparse
import threading
import time
import ctypes
import msvcrt
from collections import deque
from typing import Optional

# ===== Windows 虚拟键码 (GetAsyncKeyState) =====
VK_W = 0x57
VK_A = 0x41
VK_S = 0x53
VK_D = 0x44
VK_R = 0x52
VK_Q = 0x51
VK_SPACE = 0x20
VK_ESCAPE = 0x1B
VK_UP = 0x26
VK_DOWN = 0x28
VK_LEFT = 0x25
VK_RIGHT = 0x27


def is_key_down(vk_code):
    """Windows 原生 API：检测按键是否当前被按住（实时，毫秒级响应）"""
    return (ctypes.windll.user32.GetAsyncKeyState(vk_code) & 0x8000) != 0


def flush_stdin():
    """清空终端输入缓冲区（防止退出后遗留字符刷屏）"""
    while msvcrt.kbhit():
        msvcrt.getch()


# ===== 协议常量 =====
STATE_PREFIX = b"STATE:"
CMD_STOP = b"STOP\n"
CMD_RETURN = b"RETURN\n"

# ===== 默认连接参数 =====
DEFAULT_HOST = "192.168.137.199"
DEFAULT_PORT = 8080

class CommandMailbox:
    """线程安全的命令邮箱：急停优先，运动状态只保留最新值。"""

    def __init__(self):
        self._condition = threading.Condition()
        self._priority = deque()
        self._movement = None
        self._movement_pending = False

    def set_movement(self, command):
        with self._condition:
            self._movement = command
            self._movement_pending = True
            self._condition.notify()

    def put_priority(self, command):
        with self._condition:
            self._priority.append(command)
            self._condition.notify()

    def next_command(self, timeout):
        with self._condition:
            if not self._priority and not self._movement_pending:
                self._condition.wait(timeout)
            if self._priority:
                return self._priority.popleft()
            if self._movement_pending:
                self._movement_pending = False
                return self._movement
            return None

    def wake(self):
        with self._condition:
            self._condition.notify_all()


def run_command_sender(sock, mailbox, stop_event, heartbeat_interval=0.03,
                       on_error=None):
    """发送最新控制状态，并周期重发作为小车端失联检测心跳。"""
    active_command = None
    while not stop_event.is_set():
        command = mailbox.next_command(heartbeat_interval)
        if stop_event.is_set():
            break
        if command is not None:
            active_command = command
        elif active_command is None:
            continue
        try:
            sock.sendall(active_command)
        except OSError as exc:
            if on_error is not None:
                on_error(exc)
            break


class ManualClient:
    """手动控制客户端"""

    def __init__(self, host: str = DEFAULT_HOST, port: int = DEFAULT_PORT):
        self.host = host
        self.port = port
        self.sock = None  # type: Optional[socket.socket]
        self.running = False

        # 车辆状态
        self._speed: float = 0.0
        self._steering: float = 0.0
        self._state_lock = threading.Lock()

        # 接收缓冲区
        self._recv_buf: bytes = b""

        self._mailbox = CommandMailbox()
        self._sender_stop = threading.Event()
        self._sender_thread = None
        self._recv_thread = None

    # ------------------------------------------------------------------
    def connect(self) -> bool:
        """连接小车 TCP 服务器"""
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.settimeout(5.0)
            self.sock.connect((self.host, self.port))
            self.sock.settimeout(None)  # 连接后取消超时
            print("[OK] 已连接到小车 {}:{}".format(self.host, self.port))
            return True
        except (socket.timeout, ConnectionRefusedError, OSError) as e:
            print("[ERROR] 无法连接到 {}:{}: {}".format(self.host, self.port, e))
            print("  请确认: 1) 小车已开机  2) IP 正确  3) 程序已启动")
            return False

    def disconnect(self):
        """断开连接"""
        self.running = False
        if self.sock:
            # 急停不经过队列，尽最大可能在关闭连接前立即送达。
            try:
                self.sock.sendall(CMD_STOP)
            except OSError:
                pass
        self._stop_sender()
        if self.sock:
            try:
                self.sock.shutdown(socket.SHUT_RDWR)
            except OSError:
                pass
            self.sock.close()
            self.sock = None
        flush_stdin()  # 清空终端缓冲，防止退出后字符刷屏
        print("[INFO] 已断开连接")

    # ------------------------------------------------------------------
    def start(self):
        """启动客户端主循环"""
        if not self.connect():
            return

        self.running = True
        # 启动接收线程
        self._recv_thread = threading.Thread(target=self._recv_loop, daemon=True)
        self._recv_thread.start()

        # 启动命令发送线程（独立于主循环，避免 sendall 阻塞导致延迟）
        self._start_sender()

        print("\n" + "=" * 55)
        print("  手动控制已启动")
        print("  画面请查看 start.py 的 X11 四合一窗口")
        print("  按住 W/S/A/D: 方向   松开: 立即停")
        print("  Space: 急停   R: 返回自动   Q: 退出")
        print("=" * 55 + "\n")

        try:
            self._control_loop()
        except KeyboardInterrupt:
            print("\n[INFO] 用户中断")
        finally:
            self.disconnect()

    # ------------------------------------------------------------------
    def _recv_loop(self):
        """TCP 接收线程：解析轻量 STATE 数据。"""
        while self.running and self.sock:
            try:
                chunk = self.sock.recv(65536)
                if not chunk:
                    print("[WARN] 服务器断开连接")
                    self.running = False
                    break
                self._recv_buf += chunk
                self._parse_buffer()
            except (socket.timeout, ConnectionResetError, OSError) as e:
                if self.running:
                    print("[ERROR] 接收失败: {}".format(e))
                self.running = False
                break

    def _parse_buffer(self):
        """解析接收缓冲区中的协议数据"""
        while True:
            end = self._recv_buf.find(b"\n")
            if end == -1:
                break
            state_line = self._recv_buf[:end]
            self._recv_buf = self._recv_buf[end + 1:]
            if state_line.startswith(STATE_PREFIX):
                self._handle_state(state_line)

    def _handle_state(self, data: bytes):
        """处理 STATE 数据"""
        try:
            fields = data[len(STATE_PREFIX):].decode().split(",")
            if len(fields) >= 2:
                speed = float(fields[0])
                steering = float(fields[1])
                with self._state_lock:
                    self._speed = speed
                    self._steering = steering
        except (ValueError, UnicodeDecodeError):
            pass

    def _read_keys(self):
        """
        读取当前按住的键，返回组合键字符串（实时 GetAsyncKeyState）。
        返回: "W", "WA", "WD", "S", "SA", "SD", "A", "D", "STOP", "RETURN", "QUIT", ""
        """
        # Q / Esc -> 退出
        if is_key_down(VK_Q) or is_key_down(VK_ESCAPE):
            return "QUIT"

        # Space -> 紧急停车（优先级最高）
        if is_key_down(VK_SPACE):
            return "STOP"

        # R -> 返回自动模式
        if is_key_down(VK_R):
            return "RETURN"

        # 方向键
        w = is_key_down(VK_W) or is_key_down(VK_UP)
        s = is_key_down(VK_S) or is_key_down(VK_DOWN)
        a = is_key_down(VK_A) or is_key_down(VK_LEFT)
        d = is_key_down(VK_D) or is_key_down(VK_RIGHT)

        # 无方向键 -> 停车
        if not w and not s and not a and not d:
            return ""

        # 前进/后退互斥（同时按前进优先）
        if w and s:
            s = False
        # 左/右互斥（同时按都不转，直行）
        if a and d:
            a = False
            d = False

        cmd = ""
        if w:
            cmd += "W"
        elif s:
            cmd += "S"
        if a:
            cmd += "A"
        elif d:
            cmd += "D"

        return cmd

    def _send_command(self, cmd: bytes):
        """更新发送状态；STOP/RETURN 额外走不可覆盖的优先队列。"""
        if cmd == CMD_STOP:
            self._mailbox.set_movement(CMD_STOP)
            self._mailbox.put_priority(CMD_STOP)
        elif cmd == CMD_RETURN:
            self._mailbox.set_movement(CMD_STOP)
            self._mailbox.put_priority(CMD_RETURN)
        else:
            self._mailbox.set_movement(cmd)

    def _start_sender(self):
        """启动发送线程，并先发 STOP，避免窗口初始化期间触发看门狗。"""
        self._sender_stop.clear()
        self._sender_thread = threading.Thread(target=self._sender_loop, daemon=True)
        self._sender_thread.start()
        self._send_command(CMD_STOP)

    def _stop_sender(self):
        self._sender_stop.set()
        self._mailbox.wake()
        if self._sender_thread and self._sender_thread.is_alive():
            self._sender_thread.join(0.3)

    def _sender_loop(self):
        """独立发送线程：变化立即发送，静止状态每 30ms 心跳重发。"""
        if not self.sock:
            return

        def report_error(exc):
            if self.running:
                print("[ERROR] 发送失败: {}".format(exc))
            self.running = False

        run_command_sender(self.sock, self._mailbox, self._sender_stop,
                           heartbeat_interval=0.03, on_error=report_error)

    # ------------------------------------------------------------------
    def _control_loop(self):
        """无窗口实时键盘循环；画面统一使用 start.py 的 X11 四合一窗口。"""
        last_keys = None

        while self.running:
            keys = self._read_keys()

            if keys == "QUIT":
                self._send_command(CMD_STOP)
                self.running = False
                break

            if keys != last_keys:
                last_keys = keys
                if keys == "STOP":
                    self._send_command(CMD_STOP)
                elif keys == "RETURN":
                    self._send_command(CMD_RETURN)
                elif keys == "":
                    self._send_command(CMD_STOP)  # 松键即停
                else:
                    self._send_command((keys + "\n").encode())
                with self._state_lock:
                    speed = self._speed
                    steering = self._steering
                label = keys if keys else "STOP"
                print("\rCmd: {:<6} Speed: {:+.2f} m/s Steering: {:>4.0f}   ".format(
                    label, speed, steering), end="", flush=True)

            time.sleep(0.005)

        # 退出前发送停车
        self._send_command(CMD_STOP)
        print()


# ===== 入口 =====
def main():
    parser = argparse.ArgumentParser(
        description="Robotaxi 手动控制客户端 (Windows)",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
示例:
  python manual_client.py                          # 默认连接 192.168.137.199:8080
  python manual_client.py --host 192.168.1.100     # 自定义 IP
  python manual_client.py --port 9999              # 自定义端口
        """,
    )
    parser.add_argument("--host", default=DEFAULT_HOST,
                        help="小车 IP 地址 (默认: {})".format(DEFAULT_HOST))
    parser.add_argument("--port", type=int, default=DEFAULT_PORT,
                        help="TCP 端口 (默认: {})".format(DEFAULT_PORT))
    args = parser.parse_args()

    client = ManualClient(host=args.host, port=args.port)
    client.start()


if __name__ == "__main__":
    main()
