#pragma once
/**
 ********************************************************************************************************
 *                                               示例代码
 *                                             EXAMPLE  CODE
 *
 *                      (c) Copyright 2024; SaiShu.Lcc.; Leo; https://bjsstukeji.com
 *                                   版权所属[SASU-北京赛曙科技有限公司]
 *
 *            The code is for internal use only, not for commercial transactions(开源学习).
 *            The code ADAPTS the corresponding hardware circuit board(智能汽车-ICAR),
 *            The specific details consult the professional(欢迎联系我们,代码持续更正，敬请关注相关开源渠道).
 *********************************************************************************************************
 * @file manualControl.hpp
 * @author Leo (leo@saishukeji.com)
 * @brief 手动接管控制线程
 * @version 0.1
 * @date 2025-12-29
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <thread>
#include <mutex>
#include <atomic>
#include <cstdint>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

/**
 * @brief 手动接管控制线程
 */
class ManualControlThread
{
private:
    std::thread thread;
    std::mutex mtxState;
    std::atomic<bool> running{false};
    std::atomic<bool> connected{false};

    // Network
    int serverSocket;
    std::atomic<int> clientSocket{-1};
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrSize;

    // Vehicle state
    struct VehicleState
    {
        float speed = 0.0f;
        float steering = 0.0f;
        bool emergency = false;
    } vehicleState;

    // Manual control
    struct ManualControl
    {
        std::atomic<bool> forward{false};
        std::atomic<bool> backward{false};
        std::atomic<bool> left{false};
        std::atomic<bool> right{false};
        std::atomic<bool> emergencyStop{false};
        std::atomic<bool> returnAuto{false};
    } manualControl;

    // Last contact time
    std::atomic<int64_t> lastContactMs{0};

public:
    std::atomic<bool> controlChanged{false};  // 控制状态变化标志（参考2025手柄：事件驱动发送）
    ManualControlThread();
    ~ManualControlThread();

    void start();
    void stop();
    void updateVehicleState(float speed, float steering);
    bool isManualControl();
    void applyManualControl(float *speed, uint16_t *steering);
    bool checkForReturnKey();
    void disconnectClient();
    bool isConnected() const { return connected; }

private:
    void run();
    void handleClientConnection();
    void receiveCommands();
    void checkTimeout();
    void emergencyStop();
    void clearManualControl(bool emergency);
    bool sendAll(const void *data, size_t length);
    void processCommand(const std::string &cmd);
};
