#!/usr/bin/env python3
"""
STM32 车控板自检启动脚本
用法: python3 selfcheck_start.py
前提: 先启动 ./boot &
功能: 发送自检启动指令到 STM32，并实时显示自检进度和故障码
"""
import socket
import sys
import time

# ===== 协议校验：简单累加和（与 STM32 固件 Usb.c / EdgeBoard uart.hpp 一致） =====
def checksum(data):
    """计算累加和校验（低8位）"""
    return sum(data) & 0xFF

STEP_NAMES = {
    0:"初始化等待",1:"MotorA-开环正转25%",2:"MotorB-检测正转速度>0.5",
    3:"MotorC-开环反转-25%",4:"MotorD-检测反转速度<-0.5",
    5:"MotorE-闭环正转1.0m/s",6:"MotorF-检测闭环正转误差<0.3",
    7:"MotorG-闭环反转-1.0m/s",8:"MotorH-检测闭环反转误差<0.3",
    9:"ServoA-舵机检测",10:"Com-通信测试",11:"Buzzer-蜂鸣器测试",
    12:"RgbLed-RGB灯效测试",13:"Key-按键检测",14:"Finish-自检完成(MCU重启)",
}

def decode_error(ec):
    msgs = []
    if ec & (1<<0):  msgs.append("舵机故障")
    if ec & (1<<1):  msgs.append("电机开路")
    if ec & (1<<2):  msgs.append("电机短路")
    if ec & (1<<3):  msgs.append("PID闭环异常")
    if ec & (1<<4):  msgs.append("编码器断线")
    if ec & (1<<8):  msgs.append("自检进行中")
    return msgs

def build_keepalive():
    """构造保活帧: enableInspector (0x0A), 防止boot 2秒超时断连
    帧格式: [0x42, 0x0A, 0x04, checksum]
    checksum = (0x42 + 0x0A + 0x04) & 0xFF = 0x50
    """
    buff = bytearray([0x42, 0x0A, 0x04])
    buff.append(checksum(buff))
    return bytes(buff)

def main():
    print("=" * 55)
    print("  STM32 车控板自检工具")
    print("=" * 55)

    # 帧格式: [0x42, 0x0B, 0x05, step=0x00, checksum]
    # checksum = (0x42 + 0x0B + 0x05 + 0x00) & 0xFF = 0x52
    buff = bytearray([0x42, 0x0B, 0x05, 0x00])
    buff.append(checksum(buff))

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(3)

    try:
        sock.connect(("127.0.0.1", 8899))
        print("[OK] 已连接 boot (127.0.0.1:8899)")
    except Exception as e:
        print(f"[FAIL] 无法连接: {e}")
        print("请先启动: cd ~/workspace/icar_autopilot_2026th/build && ./boot &")
        return

    try:
        sock.send(bytes(buff))
        print("[OK] 自检指令已发送! (0x42 0x0B 0x05 0x00)")
        print("-" * 55)
        print("观察: RGB绿灯亮 → 蜂鸣器响 → 电机依次转动\n")

        last_step = -1
        sock.settimeout(1.5)
        start_time = time.time()
        tail = ""  # TCP流缓冲区：保存跨recv的残片数据
        keepalive_interval = 1.0  # 保活发送间隔(秒)
        last_keepalive = 0.0

        while True:
            # 按间隔发送保活帧防止boot断开（boot 2秒超时）
            now = time.time()
            if now - last_keepalive >= keepalive_interval:
                try:
                    sock.send(build_keepalive())
                    last_keepalive = now
                except:
                    pass

            try:
                data = sock.recv(4096)
                if not data:
                    print("\n[断开] boot已关闭连接")
                    break
            except socket.timeout:
                if time.time() - start_time > 90:
                    print("\n[超时] 90秒未完成")
                    break
                continue
            except Exception as e:
                print(f"\n[recv错误] {e}")
                break

            elapsed = time.time() - start_time
            # 拼接到缓冲区，处理跨recv的TELEM分片
            tail += data.decode(errors='ignore')

            # 调试: 打印原始数据 (可注释掉)
            # print(f"[RAW] {repr(tail[:200])}")

            # TELEM数据无换行分隔，按"TELEM:"标记拆分
            # 最后一段可能不完整，保留到下次recv继续拼接
            chunks = tail.split('TELEM:')
            # 最后一段是未完成的残片，留到下次
            tail = chunks[-1] if not tail.endswith('TELEM:') else ""
            # 处理已完整接收的TELEM段（不含最后一段残片）
            for chunk in chunks[:-1]:
                if not chunk:
                    continue
                parts = chunk.split(',')
                if len(parts) < 5:
                    continue
                try:
                    bat = int(parts[0])
                    volt = float(parts[1])
                    spd = float(parts[2])
                    ec = int(parts[3], 16)
                    step = int(parts[4])
                except:
                    continue

                if step != last_step:
                    name = STEP_NAMES.get(step, f"未知({step})")
                    print(f"\n>>> 步骤{step}: {name}")
                    last_step = step

                errs = decode_error(ec)
                err_str = ", ".join(errs) if errs else "无故障"
                icon = "V" if ec == 0 else "X"
                print(f"  [{elapsed:5.1f}s] {icon} 电池:{bat}% {volt:.1f}V  "
                      f"车速:{spd:+.2f}m/s  故障:0x{ec:04X}({err_str})")

                if step == 14:
                    print(f"\n{'='*55}")
                    if ec == 0:
                        print("  OK 自检全部通过!")
                    else:
                        print(f"  !! 存在故障: {err_str}")
                    print(f"{'='*55}")
                    return

    except KeyboardInterrupt:
        print("\n[中断]")
    except Exception as e:
        print(f"\n[错误] {e}")
    finally:
        sock.close()

if __name__ == "__main__":
    main()
