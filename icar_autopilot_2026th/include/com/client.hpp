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
 * @file client.hpp
 *
 * @author Leo
 * @brief 上下位机串口通信协议(基于套接字转换)
 * @version 0.1
 * @date 2023-12-26
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <iostream> // 输入输出类
#include <stdint.h> // 整型数据类
#include <string>
#include <thread>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstdio>
#include <unistd.h>
#include <sys/types.h>
#include <atomic>

using namespace std;

class Client
{
private:
// USB通信帧
#define USB_FRAME_HEAD 0x42 // USB通信帧头
#define USB_FRAME_LENMIN 4  // USB通信帧最短字节长度
#define USB_FRAME_LENMAX 12 // USB通信帧最长字节长度

// USB通信地址
#define USB_ADDR_CARCTRL 1 // 智能车速度+方向控制
#define USB_ADDR_BUZZER 4  // 蜂鸣器音效控制
#define USB_ADDR_LED 5     // LED灯效控制
#define USB_ADDR_HEART 0x00 // 心跳信号
#define USB_ADDR_KEY 0x10  // 按键信息

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

    std::thread threadRes;  // 串口接收子线程
    std::string portName;   // 端口名字
    SerialStruct serialStr; // 串口通信数据结构体
    int socketId = 0;
    int countInit = 0; // 初始化计数器
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

public:
    // 定义构造函数
    Client() {};
    // 定义析构函数
    ~Client() { closeClient(); };
    bool keypress = false; // 按键

    // 遥测数据（由接收线程解析boot转发的TELEM字符串）
    uint8_t batteryPercent = 0;    // 电池电量百分比 0~100
    float batteryVoltage = 0.0f;   // 电池电压 V
    float speedFeedback = 0.0f;    // 编码器反馈速度 m/s
    uint16_t errorCode = 0;        // 下位机故障码
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

    /**
     * @brief 启动套接字通信
     *
     * @return true
     * @return false
     */
    bool start(void)
    {
        socketId = socket(PF_INET, SOCK_STREAM, 0);
        if (socketId < 0)
        {
            cout << "socket init error!" << endl;
            return false;
        }
        struct sockaddr_in address;
        memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_port = htons(8899);
        // 将IPv4地址从文本转换为二进制形式
        if (inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) <= 0)
        {
            std::cerr << "Invalid address/ Address not supported" << std::endl;
            return false;
        }

        if (connect(socketId, (struct sockaddr *)&address, sizeof(address)) < 0)
        {
            cout << "socket connect error!" << endl;
            return false;
        }

        // 启动数据接收子线程
        threadRes = std::thread([this]()
                                {
        char buffer[1024] = {0};
        while (1) {
        int len = read(socketId, buffer, 1024);
        if (len <= 0) continue;

        std::string str(buffer, len);
        if (str.find("Keypress") != std::string::npos)//按键按下
            keypress = true;

        // 解析遥测数据: TELEM:bat%,volt,speed,errorCode,step
        if (str.find("TELEM:") != std::string::npos)
        {
            int bat = 0, ec = 0, step = 0;
            float volt = 0.0f, spd = 0.0f;
            if (sscanf(str.c_str(), "TELEM:%d,%f,%f,0x%04X,%d",
                       &bat, &volt, &spd, &ec, &step) >= 4)
            {
                batteryPercent = (uint8_t)bat;
                batteryVoltage = volt;
                speedFeedback = spd;
                errorCode = (uint16_t)ec;
                selfcheckStep = (uint8_t)step;
                telemetryUpdated = true;
            }
        }
      } });
        return true;
    }

    /**
     * @brief 关闭客户端
     *
     */
    void closeClient()
    {
        carControl(0, 1500); // 舵机PWM中值 1500
        close(socketId);
        threadRes.join();
    }

    /**
     * @brief U8转char
     *
     * @param str
     * @param UnChar
     * @param ucLen
     */
    void convertUnCharToStr(char *data, unsigned char *buff)
    {
        int len = sizeof(buff) / sizeof(buff[0]);
        for (int i = 0; i < len; i++)
        {
            // 格式化输str,每unsigned char 转换字符占两位置%x写输%X写输
            sprintf(data + i * 2, "%02x", buff[i]);
        }
    }

    /**
     * @brief 套接字发送数据
     *
     */
    void transmit(uint8_t *buff, int len)
    {
        char data[len];
        memcpy(data, buff, len); // 拷贝内存数据，防止“\0”数据丢失

        // 发送消息到服务器
        send(socketId, data, len, 0);
    }

    /**
     * @brief 速度+方向控制
     *
     * @param speed 速度：m/s
     * @param servo 方向：PWM（500~2500）
     */
    void carControl(float speed, uint16_t servo)
    {
        countInit++;
        if (countInit >= 50)
            countInit = 50;
        else
            speed = 0.0; // 初始化速度为0，等待1s发车

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

        transmit(buff, 11); // 发送数据
    }

    /**
     * @brief 蜂鸣器音效控制
     *
     * @param sound
     */
    void buzzerSound(Buzzer sound)
    {
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

        transmit(buff, 6); // 发送数据
    }

    /**
     * @brief 发送心跳信号
     *
     */
    void sendHeart()
    {
        uint8_t buff[4];
        uint8_t check = 0;
        buff[0] = USB_FRAME_HEAD;   // 帧头 0x42
        buff[1] = USB_ADDR_HEART;   // 地址 0x00（心跳）
        buff[2] = 3;                // 帧长（HEAD之后: ADDR+LEN+checksum共3字节）
        buff[3] = crc8(buff, 3); // [P2-4] CRC8校验
        transmit(buff, 4);          // 发送4字节心跳帧
    }
};
