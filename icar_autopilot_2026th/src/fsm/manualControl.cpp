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
 * @file manualControl.cpp
 * @author Leo (leo@saishukeji.com)
 * @brief 手动接管控制线程实现
 * @version 0.1
 * @date 2025-12-29
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "fsm/manualControl.hpp"
#include "utils/tools.hpp"
#include <unistd.h>
#include <cerrno>

namespace
{
int64_t monotonicMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}
}

ManualControlThread::ManualControlThread() {
    serverSocket = -1;
    clientSocket = -1;
    running = false;
    connected = false;
}

ManualControlThread::~ManualControlThread() {
    stop();
}

void ManualControlThread::start() {
    running = true;
    lastContactMs = monotonicMs();

    // Create server socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0)
    {
        perror("[Manual] socket failed");
        running = false;
        return;
    }

    // 允许端口立即重用，防止程序重启后TIME_WAIT导致bind失败
    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("[Manual] setsockopt SO_REUSEADDR failed");
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("[Manual] bind failed");
        close(serverSocket);
        serverSocket = -1;
        running = false;
        return;
    }

    if (listen(serverSocket, 1) < 0)
    {
        perror("[Manual] listen failed");
        close(serverSocket);
        serverSocket = -1;
        running = false;
        return;
    }

    thread = std::thread(&ManualControlThread::run, this);
}

void ManualControlThread::stop() {
    running = false;
    connected = false;
    clearManualControl(true);

    // shutdown 负责唤醒阻塞中的 recv；连接线程拥有并关闭 clientSocket。
    int fd = clientSocket.load();
    if (fd >= 0) {
        shutdown(fd, SHUT_RDWR);
    }
    if (serverSocket >= 0) {
        close(serverSocket);
        serverSocket = -1;
    }

    // Wait for thread to finish
    if (thread.joinable()) {
        thread.join();
    }
}

void ManualControlThread::updateVehicleState(float speed, float steering) {
    std::lock_guard<std::mutex> lock(mtxState);
    vehicleState.speed = speed;
    vehicleState.steering = steering;
    vehicleState.emergency = false;
}

bool ManualControlThread::isManualControl() {
    return manualControl.forward || manualControl.backward ||
           manualControl.left || manualControl.right;
}

void ManualControlThread::applyManualControl(float *speed, uint16_t *steering) {
    if (manualControl.emergencyStop) {
        *speed = 0;
        *steering = PWMSERVOMID; // 中间位置
        return;
    }

    // Speed control
    if (manualControl.forward) {
        *speed = 0.3f;  // Forward speed
    } else if (manualControl.backward) {
        *speed = -0.3f; // Backward speed
    } else {
        *speed = 0.0f;  // Stop
    }

    // Steering control
    if (manualControl.left) {
        *steering = PWMSERVOMAX; // 左转：使用标定左极限，保证短按也有明显响应
    } else if (manualControl.right) {
        *steering = PWMSERVOMIN; // 右转：使用标定右极限，保证短按也有明显响应
    } else {
        *steering = PWMSERVOMID;  // 直行
    }
}

bool ManualControlThread::checkForReturnKey() {
    bool ret = manualControl.returnAuto;
    if (ret) {
        manualControl.returnAuto = false;
    }
    return ret;
}

void ManualControlThread::disconnectClient() {
    clearManualControl(true);
    connected = false;
    int fd = clientSocket.load();
    if (fd >= 0)
        shutdown(fd, SHUT_RDWR);
}

void ManualControlThread::run() {
    while (running) {
        // 如果socket未成功初始化（start失败），等待重试
        if (serverSocket < 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        // Wait for client connection
        addrSize = sizeof(clientAddr);
        int acceptedSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &addrSize);

        if (acceptedSocket < 0) {
            continue;
        }

        clientSocket = acceptedSocket;
        connected = true;
        lastContactMs = monotonicMs();
        // 禁用Nagle算法，降低控制延迟
        int flag = 1;
        setsockopt(acceptedSocket, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
        printf("[Manual] Remote control connected\n");

        // Handle client connection
        handleClientConnection();

        connected = false;
        shutdown(acceptedSocket, SHUT_RDWR);
        close(acceptedSocket);
        clientSocket = -1;
    }
}

void ManualControlThread::handleClientConnection() {
    clearManualControl(false);
    manualControl.returnAuto = false;

    // 每个连接只拥有一个可回收的接收线程，禁止跨连接访问复用的描述符。
    std::thread cmdThread(&ManualControlThread::receiveCommands, this);

    // Send images and state
    while (running && connected) {
        checkTimeout();
        if (!connected)
            break;

        // Send vehicle state (higher frequency for state)
        std::string state;
        {
            std::lock_guard<std::mutex> lock(mtxState);
            state = "STATE:" + std::to_string(vehicleState.speed) +
                    "," + std::to_string(vehicleState.steering) + "\n";
        }

        if (!sendAll(state.data(), state.size())) {
            cerr << "[Manual] Error sending state data" << endl;
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(16));  // 约60Hz
    }

    connected = false;
    clearManualControl(true);
    int fd = clientSocket.load();
    if (fd >= 0)
        shutdown(fd, SHUT_RDWR);
    if (cmdThread.joinable())
        cmdThread.join();
}

void ManualControlThread::receiveCommands() {
    std::string lineBuf;  // 粘包缓冲区
    char rawBuf[1024];
    while (running && connected) {
        int bytes = recv(clientSocket, rawBuf, 1023, 0);
        if (bytes <= 0) {
            clearManualControl(true);
            connected = false;
            break;
        }

        rawBuf[bytes] = '\0';
        lineBuf += std::string(rawBuf);

        // Update contact time
        lastContactMs = monotonicMs();

        // 按换行符分割处理（解决TCP粘包）
        size_t pos;
        while ((pos = lineBuf.find('\n')) != std::string::npos) {
            std::string cmd = lineBuf.substr(0, pos + 1);  // 包含\n
            lineBuf.erase(0, pos + 1);

            processCommand(cmd);

            if (manualControl.returnAuto) {
                printf("[Manual] Return to auto command received\n");
                goto done;
            }
        }

    }
done:
    lineBuf.clear();
}

void ManualControlThread::processCommand(const std::string &cmd) {
    // 处理特殊命令
    if (cmd == "RETURN\n") {
        manualControl.returnAuto = true;
        controlChanged = true;
        printf("[Manual] Return to auto command received\n");
        return;
    }
    if (cmd == "STOP\n") {
        clearManualControl(true);
        manualControl.returnAuto = false;
        controlChanged = true;
        printf("[Manual] Stop command received\n");
        return;
    }

    // 组合命令解析（如"WA\n"=前进+左转，"WD\n"=前进+右转）
    manualControl.forward = false;
    manualControl.backward = false;
    manualControl.left = false;
    manualControl.right = false;
    manualControl.emergencyStop = false;
    manualControl.returnAuto = false;
    for (char c : cmd) {
        switch (c) {
            case 'W': manualControl.forward = true; break;
            case 'S': manualControl.backward = true; break;
            case 'A': manualControl.left = true; break;
            case 'D': manualControl.right = true; break;
        }
    }
    controlChanged = true;
    printf("[Manual] Command received: %s", cmd.c_str());
}

void ManualControlThread::checkTimeout() {
    if (connected) {
        int64_t elapsed = monotonicMs() - lastContactMs.load();

        if (elapsed > 300) {
            printf("[Manual] Command timeout, stopping vehicle\n");
            clearManualControl(true);
            connected = false;
        }
    }
}

void ManualControlThread::emergencyStop() {
    clearManualControl(true);
}

void ManualControlThread::clearManualControl(bool emergency) {
    manualControl.forward = false;
    manualControl.backward = false;
    manualControl.left = false;
    manualControl.right = false;
    manualControl.emergencyStop = emergency;
    controlChanged = true;

    std::lock_guard<std::mutex> lock(mtxState);
    vehicleState.speed = 0;
    vehicleState.steering = PWMSERVOMID;
    vehicleState.emergency = emergency;
}

bool ManualControlThread::sendAll(const void *data, size_t length) {
    const char *bytes = static_cast<const char *>(data);
    size_t sent = 0;
    int fd = clientSocket.load();
    while (sent < length && running && connected) {
        ssize_t count = send(fd, bytes + sent, length - sent, MSG_NOSIGNAL);
        if (count > 0) {
            sent += static_cast<size_t>(count);
            continue;
        }
        if (count < 0 && errno == EINTR)
            continue;
        return false;
    }
    return sent == length;
}
