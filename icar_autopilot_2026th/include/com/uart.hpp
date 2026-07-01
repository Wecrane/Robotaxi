#pragma once
/**
 ********************************************************************************************************
 *                                               示例代码
 *                                             EXAMPLE  CODE
 *
 *                      (c) Copyright 2024; SaiShu.Lcc.; Leo; https://bjsstech.com
 *                                   版权所属[SASU-北京赛曙科技有限公司]
 *
 *            The code is for internal use only, not for commercial transactions(开源学习).
 *            The code ADAPTS the corresponding hardware circuit board(智能汽车-ICAR),
 *            The specific details consult the professional(欢迎联系我们,代码持续更正，敬请关注相关开源渠道).
 *********************************************************************************************************
 * @file uart.hpp
 *
 * @author Leo
 * @brief 上下位机串口通信协议
 * @version 0.1
 * @date 2023-12-26
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <iostream>               // 输入输出类
#include <libserial/SerialPort.h> // 串口通信
#include <math.h>                 // 数学函数类
#include <stdint.h>               // 整型数据类
#include <string.h>
#include <thread>
#include <atomic>

using namespace LibSerial;
using namespace std;

class Uart
{
private:
// USB通信帧
#define USB_FRAME_HEAD 0x42 // USB通信帧头
#define USB_FRAME_LENMIN 4  // USB通信帧最短字节长度
#define USB_FRAME_LENMAX 12 // USB通信帧最长字节长度

// USB通信地址
#define USB_ADDR_HEART 0     // 心跳信号，特指Boot
#define USB_ADDR_CARCTRL 1   // 智能车速度+方向控制
#define USB_ADDR_BUZZER 4    // 蜂鸣器音效控制
#define USB_ADDR_LED 5       // LED灯效控制
#define USB_ADDR_BATTERY 7   // 电池信息（下位机→上位机）
#define USB_ADDR_SPEEDBACK 8 // 车速反馈（下位机→上位机）
#define USB_ADDR_INSPECTOR 0x0A // 自检数据使能（上位机→下位机）
#define USB_ADDR_SELFCHECK 0x0B // 自检状态（下位机→上位机）
#define USB_ADDR_KEY 0x06    // 按键信息 [P2-3] 对齐下位机USB_ADDR_KEYINPUT

#define PWMSERVOMAX 1900 // 舵机PWM最大值（左）1840
#define PWMSERVOMID 1500 // 舵机PWM中值 1520
#define PWMSERVOMIN 1100 // 舵机PWM最小值（右）1200

    // CRC8-MAXIM 校验表 (poly=0x31) [P2-4] 替换8位累加和
    static constexpr uint8_t crc8_table[256] = {
        0x00,0x5e,0xbc,0xe2,0x61,0x3f,0xdd,0x83,0xc2,0x9c,0x7e,0x20,0xa3,0xfd,0x1f,0x41,
        0x9d,0xc3,0x21,0x7f,0xfc,0xa2,0x40,0x1e,0x5f,0x01,0xe3,0xbd,0x3e,0x60,0x82,0xdc,
        0x23,0x7d,0x9f,0xc1,0x42,0x1c,0xfe,0xa0,0xe1,0xbf,0x5d,0x03,0x80,0xde,0x3c,0x62,
        0xbe,0xe0,0x02,0x5c,0xdf,0x81,0x63,0x3d,0x7c,0x22,0xc0,0x9e,0x1d,0x43,0xa1,0xff,
        0x46,0x18,0xfa,0xa4,0x27,0x79,0x9b,0xc5,0x84,0xda,0x38,0x66,0xe5,0xbb,0x59,0x07,
        0xdb,0x85,0x67,0x39,0xba,0xe4,0x06,0x58,0x19,0x47,0xa5,0xfb,0x78,0x26,0xc4,0x9a,
        0x65,0x3b,0xd9,0x87,0x04,0x5a,0xb8,0xe6,0xa7,0xf9,0x1b,0x45,0xc6,0x98,0x7a,0x24,
        0xf8,0xa6,0x44,0x1a,0x99,0xc7,0x25,0x7b,0x3a,0x64,0x86,0xd8,0x5b,0x05,0xe7,0xb9,
        0x8c,0xd2,0x30,0x6e,0xed,0xb3,0x51,0x0f,0x4e,0x10,0xf2,0xac,0x2f,0x71,0x93,0xcd,
        0x11,0x4f,0xad,0xf3,0x70,0x2e,0xcc,0x92,0xd3,0x8d,0x6f,0x31,0xb2,0xec,0x0e,0x50,
        0xaf,0xf1,0x13,0x4d,0xce,0x90,0x72,0x2c,0x6d,0x33,0xd1,0x8f,0x0c,0x52,0xb0,0xee,
        0x32,0x6c,0x8e,0xd0,0x53,0x0d,0xef,0xb1,0xf0,0xae,0x4c,0x12,0x91,0xcf,0x2d,0x73,
        0xca,0x94,0x76,0x28,0xab,0xf5,0x17,0x49,0x08,0x56,0xb4,0xea,0x69,0x37,0xd5,0x8b,
        0x57,0x09,0xeb,0xb5,0x36,0x68,0x8a,0xd4,0x95,0xcb,0x29,0x77,0xf4,0xaa,0x48,0x16,
        0xe9,0xb7,0x55,0x0b,0x88,0xd6,0x34,0x6a,0x2b,0x75,0x97,0xc9,0x4a,0x14,0xf6,0xa8,
        0x74,0x2a,0xc8,0x96,0x15,0x4b,0xa9,0xf7,0xb6,0xe8,0x0a,0x54,0xd7,0x89,0x6b,0x35
    };

    inline uint8_t crc8(const uint8_t *data, int len) const {
        uint8_t crc = 0x00;
        for (int i = 0; i < len; i++)
            crc = crc8_table[crc ^ data[i]];
        return crc;
    }

    /**
     * @brief 串口通信结构体
     *
     */
    typedef struct
    {
        bool start;                           // 开始接收标志
        uint8_t index;                        // 接收序列
        uint8_t buffRead[USB_FRAME_LENMAX];   // 临时缓冲数据
        uint8_t buffFinish[USB_FRAME_LENMAX]; // 校验成功数据
    } SerialStruct;

    std::unique_ptr<std::thread> threadRec; // 串口接收子线程
    std::shared_ptr<SerialPort> serialPort = nullptr;
    bool isOpen = false;
    SerialStruct serialStr; // 串口通信数据结构体

    /**
     * @brief 32位数据内存对齐/联合体
     *
     */
    typedef union
    {
        uint8_t buff[4];
        float float32;
        int int32;
    } Bit32Union;

    /**
     * @brief 16位数据内存对齐/联合体
     *
     */
    typedef union
    {
        uint8_t buff[2];
        int int16;
        uint16_t uint16;
    } Bit16Union;

    /**
     * @brief 串口接收字节数据
     *
     * @param charBuffer
     * @param msTimeout
     * @return int
     */
    int receiveBytes(unsigned char &charBuffer, size_t msTimeout = 0)
    {
        /*try检测语句块有没有异常。如果没有发生异常,就检测不到。
        如果发生异常，則交给 catch 处理，执行 catch 中的语句* */
        try
        {
            /*从串口读取一个数据,指定msTimeout时长内,没有收到数据，抛出异常。
            如果msTimeout为0，则该方法将阻塞，直到数据可用为止。*/
            serialPort->ReadByte(charBuffer, msTimeout); // 可能出现异常的代码段
        }
        catch (const ReadTimeout &) // catch捕获并处理 try 检测到的异常。
        {
            // std::cerr << "The ReadByte() call has timed out." << std::endl;
            return -2;
        }
        catch (const NotOpen &) // catch()中指明了当前 catch 可以处理的异常类型
        {
            std::cerr << "Port Not Open ..." << std::endl;
            return -1;
        }
        return 0;
    };

public:
    // 定义构造函数
    Uart() {};
    // 定义析构函数
    ~Uart() { close(); };
    bool keypress = false; // 按键
    bool killAll = false;  // 杀进程
    bool exitBoot = false; // 退出boot
    bool running = false;  // 运行状态

    // 遥测数据（由dataTransform()解析下位机上报）
    uint8_t batteryPercent = 0;    // 电池电量百分比 0~100
    float batteryVoltage = 0.0f;   // 电池电压 V
    float speedFeedback = 0.0f;    // 编码器反馈速度 m/s
    uint16_t errorCode = 0;        // 下位机故障码 bit0=舵机 bit4=编码器断线
    uint8_t selfcheckStep = 0;     // 自检当前步骤
    std::atomic<bool> telemetryUpdated{false}; // 本周期有新遥测数据（跨线程安全）

    /**
     * @brief 蜂鸣器音效
     *
     */
    enum Buzzer
    {
        BUZZER_OK = 0,   // 确认
        BUZZER_WARNNING, // 报警
        BUZZER_FINISH,   // 完成
        BUZZER_DING,     // 提示
        BUZZER_START,    // 开机
    };

public:
    /**
     * @brief
     *
     * @param data
     * @return int
     */
    int transmitByte(unsigned char data)
    {
        // try检测语句块有没有异常
        try
        {
            serialPort->WriteByte(data); // 写数据到串口
        }
        catch (const std::runtime_error &) // catch捕获并处理 try 检测到的异常。
        {
            std::cerr << "The Write() runtime_error." << std::endl;
            return -2;
        }
        catch (const NotOpen &) // catch捕获并处理 try 检测到的异常。
        {
            std::cerr << "Port Not Open ..." << std::endl;
            return -1;
        }
        serialPort->DrainWriteBuffer(); // 等待，直到写缓冲区耗尽，然后返回。
        return 0;
    }

    /**
     * @brief 启动串口通信
     *
     * @param port 串口号
     * @return int
     */
    int open(std::string portName)
    {
        serialPort = std::make_shared<SerialPort>();
        if (serialPort == nullptr)
        {
            std::cerr << "Serial Create Failed ." << std::endl;
            return -1;
        }
        // try检测语句块有没有异常
        try
        {
            serialPort->Open(portName);                                 // 打开串口
            serialPort->SetBaudRate(BaudRate::BAUD_115200);             // 设置波特率
            serialPort->SetCharacterSize(CharacterSize::CHAR_SIZE_8);   // 8位数据位
            serialPort->SetFlowControl(FlowControl::FLOW_CONTROL_NONE); // 设置流控
            serialPort->SetParity(Parity::PARITY_NONE);                 // 无校验
            serialPort->SetStopBits(StopBits::STOP_BITS_1);             // 1个停止位
        }
        catch (const OpenFailed &) // catch捕获并处理 try 检测到的异常。
        {
            std::cerr << "Serial port: " << portName << "open failed ..."
                      << std::endl;
            isOpen = false;
            return -2;
        }
        catch (const AlreadyOpen &) // catch捕获并处理 try 检测到的异常。
        {
            std::cerr << "Serial port: " << portName << "open failed ..."
                      << std::endl;
            isOpen = false;
            return -3;
        }
        catch (...) // catch捕获并处理 try 检测到的异常。
        {
            std::cerr << "Serial port: " << portName << " recv exception ..."
                      << std::endl;
            isOpen = false;
            return -4;
        }

        serialStr.start = false;
        serialStr.index = 0;
        isOpen = true;

        return 0;
    }

    /**
     * @brief 启动接收子线程
     *
     */
    void startReceive(void)
    {
        if (!isOpen) // 串口是否正常打开
            return;

        // 启动串口接收子线程
        threadRec = std::make_unique<std::thread>([this]()
                                                  {
      while (1) {
        receiveCheck(); // 串口接收校验
      } });
    }

    /**
     * @brief 关闭串口通信
     *
     */
    void close(void)
    {
        printf(" uart thread exit!\n");
        carControl(0, PWMSERVOMID);
        threadRec->join();
        if (serialPort != nullptr)
        {
            serialPort->Close();
            serialPort = nullptr;
        }
        isOpen = false;
    }

    /**
     * @brief 串口接收校验
     *
     */
    void receiveCheck(void)
    {
        if (!isOpen) // 串口是否正常打开
            return;

        uint8_t resByte = 0;
        int ret = receiveBytes(resByte, 0);
        if (ret == 0)
        {
            if (resByte == USB_FRAME_HEAD && !serialStr.start) // 监听帧头
            {
                serialStr.start = true;                   // 开始接收数据
                serialStr.buffRead[0] = resByte;          // 获取帧头
                serialStr.buffRead[2] = USB_FRAME_LENMIN; // 初始化帧长
                serialStr.index = 1;
            }
            else if (serialStr.index == 2) // 接收帧的长度
            {
                serialStr.buffRead[serialStr.index] = resByte;
                serialStr.index++;
                if (resByte > USB_FRAME_LENMAX ||
                    resByte < USB_FRAME_LENMIN) // 帧长错误
                {
                    serialStr.buffRead[2] = USB_FRAME_LENMIN; // 重置帧长
                    serialStr.index = 0;
                    serialStr.start = false; // 重新监听帧长
                }
            }
            else if (serialStr.start &&
                     serialStr.index < USB_FRAME_LENMAX) // 开始接收数据
            {
                serialStr.buffRead[serialStr.index] = resByte; // 读取数据
                serialStr.index++;                             // 索引下移
            }

            // 帧长接收完毕
            if ((serialStr.index >= USB_FRAME_LENMAX ||
                 serialStr.index >= serialStr.buffRead[2]) &&
                serialStr.index > USB_FRAME_LENMIN) // 检测是否接收完数据
            {
                uint8_t length = USB_FRAME_LENMIN;
                length = serialStr.buffRead[2]; // 读取本次数据的长度

                if (crc8(serialStr.buffRead, length - 1) == serialStr.buffRead[length - 1]) // [P2-4] CRC8校验
                {
                    memcpy(serialStr.buffFinish, serialStr.buffRead,
                           USB_FRAME_LENMAX); // 储存接收的数据
                    dataTransform();
                }

                serialStr.index = 0;     // 重新开始下一轮数据接收
                serialStr.start = false; // 重新监听帧头
            }
        }
    }

    /**
     * @brief 串口通信协议数据转换
     */
    void dataTransform(void)
    {
        switch (serialStr.buffFinish[1])
        {
        case USB_ADDR_KEY: // 0x06 按键信息 [P2-3] 对齐下位机协议
            if (serialStr.buffFinish[3] == 1) // 短按：切换运行/停止
            {
                if (running)
                {
                    running = false;
                    killAll = true;
                }
                else
                {
                    keypress = true;
                    running = true;
                }
            }
            else if (serialStr.buffFinish[3] == 2) // 长按>=2s：强制杀进程
            {
                keypress = false;
                killAll = true;
                exitBoot = false;
            }
            else if (serialStr.buffFinish[3] == 3) // 退出Boot
            {
                keypress = false;
                killAll = false;
                exitBoot = true;
            }
            break;

        case USB_ADDR_BATTERY: // 0x07 电池信息 [电量%][电压float32]
            batteryPercent = serialStr.buffFinish[3];
            Bit32Union bit32;
            for (int i = 0; i < 4; i++)
                bit32.buff[i] = serialStr.buffFinish[4 + i];
            batteryVoltage = bit32.float32;
            telemetryUpdated = true;
            break;

        case USB_ADDR_SPEEDBACK: // 0x08 车速反馈 [speed float32]
            {
                Bit32Union bit32;
                for (int i = 0; i < 4; i++)
                    bit32.buff[i] = serialStr.buffFinish[3 + i];
                speedFeedback = bit32.float32;
                telemetryUpdated = true;
            }
            break;

        case USB_ADDR_SELFCHECK: // 0x0B 自检状态 [step][errorCode uint16]
            selfcheckStep = serialStr.buffFinish[3];
            Bit16Union bit16;
            bit16.buff[0] = serialStr.buffFinish[4];
            bit16.buff[1] = serialStr.buffFinish[5];
            errorCode = bit16.uint16;
            telemetryUpdated = true;
            break;

        default:
            break;
        }
    }

    /**
     * @brief 速度+方向控制
     *
     * @param speed 速度：m/s
     * @param servo 方向：PWM（500~2500）
     */
    void carControl(float speed, uint16_t servo)
    {
        if (!isOpen)
            return;

        uint8_t buff[11];  // 多发送一个字节
        uint8_t check = 0; // 校验位
        Bit32Union bit32U;
        Bit16Union bit16U;

        buff[0] = USB_FRAME_HEAD;   // 通信帧头
        buff[1] = USB_ADDR_CARCTRL; // 地址
        buff[2] = 10;               // 帧长

        bit32U.float32 = speed; // X轴线速度
        for (int i = 0; i < 4; i++)
            buff[i + 3] = bit32U.buff[i];

        bit16U.uint16 = servo; // Y轴线速度
        buff[7] = bit16U.buff[0];
        buff[8] = bit16U.buff[1];

        buff[9] = crc8(buff, 9); // [P2-4] CRC8校验

        // 循环发送数据
        for (size_t i = 0; i < 11; i++)
            transmitByte(buff[i]);
    }

    /**
     * @brief 蜂鸣器音效控制
     *
     * @param sound
     */
    void buzzerSound(Buzzer sound)
    {
        if (!isOpen)
            return;
        uint8_t buff[6];   // 多发送一个字节
        uint8_t check = 0; // 校验位

        buff[0] = USB_FRAME_HEAD;  // 帧头
        buff[1] = USB_ADDR_BUZZER; // 地址
        buff[2] = 5;               // 帧长
        switch (sound)
        {
        case Buzzer::BUZZER_OK: // 确认
            buff[3] = 1;
            break;
        case Buzzer::BUZZER_WARNNING: // 报警
            buff[3] = 2;
            break;
        case Buzzer::BUZZER_FINISH: // 完成
            buff[3] = 3;
            break;
        case Buzzer::BUZZER_DING: // 提示
            buff[3] = 4;
            break;
        case Buzzer::BUZZER_START: // 开机
            buff[3] = 5;
            break;
        }

        buff[4] = crc8(buff, 4); // [P2-4] CRC8校验

        // 循环发送数据
        for (size_t i = 0; i < 6; i++)
            transmitByte(buff[i]);
    }

    /**
     * @brief 使能下位机遥测上报（发送0x0A指令）
     *
     */
    void enableInspector()
    {
        if (!isOpen)
            return;

        uint8_t buff[4];
        uint8_t check = 0;
        buff[0] = USB_FRAME_HEAD;     // 帧头 0x42
        buff[1] = USB_ADDR_INSPECTOR;  // 地址 0x0A
        buff[2] = 4;                  // 帧长（需≥LENMIN=4，否则下位机丢弃）
        buff[3] = crc8(buff, 3); // [P2-4] CRC8校验
        for (size_t i = 0; i < 4; i++)
            transmitByte(buff[i]);
    }

    /**
     * @brief 发送心跳信号
     *
     */
    void sendHeart()
    {
        if (!isOpen)
            return;

        uint8_t buff[5];   // 多发送一个字节
        uint8_t check = 0; // 校验位

        buff[0] = USB_FRAME_HEAD; // 通信帧头
        buff[1] = USB_ADDR_HEART; // 地址
        buff[2] = 4;              // 帧长
        buff[3] = crc8(buff, 3); // [P2-4] CRC8校验

        // 循环发送数据
        for (size_t i = 0; i < 5; i++)
            transmitByte(buff[i]);
    }
};
