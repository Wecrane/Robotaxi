/**
 ********************************************************************************************************
 *                                               示例代码
 *                                             EXAMPLE  CODE
 *
 *                      (c) Copyright 2024; SaiShu.Lcc.; Leo; https://bjsstech.com
 *                                   版权所属[SASU-北京赛曙科技有限公司]
 *
 *            The code is for internal use only, not for commercial transactions(开源学�?.
 *            The code ADAPTS the corresponding hardware circuit board(智能汽车-ICAR),
 *            The specific details consult the professional(欢迎联系我们,代码持续更正，敬请关注相关开源渠�?.
 *********************************************************************************************************
 * @file cross.cpp
 * @author Leo (leo@saishukeji.com)
 * @brief 斑马线停车控�? * @version 0.1
 * @date 2025-05-12
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "fsm/cross.hpp"

/**
 * @brief Construct a new Fsm Park
 *
 * @param par
 */
FsmCross::FsmCross(std::shared_ptr<Params> par)
    : FSMState(FsmMode::CROSS, par)
{
}

/**
 * @brief Destroy the Fsm Park
 *
 */
FsmCross::~FsmCross()
{
}

/**
 * @brief 检查状态切�? *
 * @return FsmMode 切换后的状�? */
FsmMode FsmCross::getMode()
{
    // 输出场景状态结�?    if (step == Step::NONE || !params->config.currentLapConfig->cross)
        return FsmMode::NORMAL;
    else
        return FsmMode::CROSS;
}

/**
 * @brief 检查是否通过斑马线（起点/终点检测）
 */
bool FsmCross::checkCrossPass()
{
    bool crossDetected = false;
    for (int i = 0; i < params->resultsSnapshot.size(); i++)
    {
        if (params->resultsSnapshot[i].type == LABEL_CROSS)
        {
            crossDetected = true;
            if (!params->crossPassed)
            {
                // 斑马线底部进入图像下�?/3时才进行过线计数（避免过早识别）
                if (params->resultsSnapshot[i].y + params->resultsSnapshot[i].height <= ROWSIMAGE * 2 / 3)
                    continue;

                params->crossPassed = true;
                return true;
            }
        }
    }

    // cross离开画面一段时间后才复位，防止AI闪烁导致重复触发
    if (!crossDetected)
    {
        crossLostCount++;
        if (crossLostCount > 15) // �?.5秒未检测到cross才复�?        {
            params->crossPassed = false;
            crossLostCount = 0;
        }
    }
    else
    {
        crossLostCount = 0;
    }

    return false;
}

/**
 * @brief 运行FSM状态（循环主程序）
 *
 */
void FsmCross::run(Mat &img)
{
    if (!params->config.currentLapConfig->cross) // 当前圈未启用斑马线功�?        return;

    countInit++; // 起点屏蔽计数器（同时保护换圈和停车，避免发车时误触）
    if (countInit > 999)
        countInit = 999;
    else if (countInit < 60)
        return;

    // 检查是否通过斑马线（起点/终点�? 仅用于计数和圈数切换
    if (checkCrossPass())
    {
        crossCount++;
        printf("[Cross] Cross #%d detected (currentLap=%d)\n", crossCount, params->currentLap);

        // 增加圈数，准备进入下一�?        if (params->currentLap < params->totalLaps)
        {
            params->nextLap();
            printf("[Cross] Lap incremented to %d\n", params->currentLap);
        }

        return;
    }

    // 最后一圈：等待斑马线完全离开视野后再停车（越过斑马线�?    if (crossCount >= params->totalLaps && !params->crossPassed && step != Step::STOP)
    {
        printf("[Cross] Cross fully passed, stopping vehicle...\n");
        setStep(Step::STOP);
        return;
    }

    switch (step)
    {
    case Step::NONE: // AI未识�?    {
        countCross++; // 斑马线屏蔽计数器
        if (countCross > 999)
            countCross = 999;

        for (int i = 0; i < params->resultsSnapshot.size(); i++)
        {
            // 只有斑马线出现在图像下方1/3时才进行场景识别
            if (params->resultsSnapshot[i].type == LABEL_CROSS && countCross > 60 && params->resultsSnapshot[i].y + params->resultsSnapshot[i].height > ROWSIMAGE * 2 / 3)
            {
                countRec++;
                break;
            }
        }

        if (countRec >= 2)
            setStep(Step::ENABLE); // 设置新状�?
        if (countRec > 0) // 识别AI标志后开始场次计�?        {
            countSes++;
            if (countSes > 4)
            {
                countRec = 0; // AI场景识别计数�?                countSes = 0; // 场次计数�?            }
        }
        break;
    }

    case Step::ENABLE: // 场景使能
    {
        timeout++;
        bool crossDetected = false;
        for (int i = 0; i < params->resultsSnapshot.size(); i++)
        {
            if (params->resultsSnapshot[i].type == LABEL_CROSS) // 禁行标志：斑马线
            {
                crossDetected = true;
                // 当斑马线已经越过车辆（检测框的上边缘低于车辆位置�?                if (params->resultsSnapshot[i].y < ROWSIMAGE * 0.4)
                {
                    countRec++;
                    timeout = 0;
                    break;
                }
            }
        }

        // 如果斑马线离开画面，连�?帧未检测到才触发停车（防抖动）
        if (!crossDetected)
        {
            crossLostStepCount++;
            if (crossLostStepCount >= 3) // 连续3帧未检测到斑马�?            {
                // 最后一圈不在此停车，等待cross完全离开后由crossPassed处理
                setStep(crossCount >= params->totalLaps ? Step::NONE : Step::STOP);
                return;
            }
        }
        else
        {
            crossLostStepCount = 0;
        }

        if (countRec >= 2)
            // 最后一圈不在此停车，等待cross完全离开后由crossPassed处理
            setStep(crossCount >= params->totalLaps ? Step::NONE : Step::STOP);
        if (timeout > 30)
        {
            setStep(Step::NONE); // 设置新状�?        }
        break;
    }

    case Step::STOP: // 停车
    {
        // 非最后一圈：不执行停车，恢复行驶（仅用于圈数计数）
        if (crossCount < params->totalLaps)
        {
            setStep(Step::NONE);
            break;
        }

        // 最后一圈：编码器测距精确停车（1.5m内）
        if (timeout == 0)
        {
            // 首次进入STOP：清零距离积分起点
            params->ctrl.odometry = 0.0;
            printf("[Cross] Final lap STOP: measuring distance...\n");
        }
        timeout++;

        if (params->ctrl.odometry < 1.5)
        {
            // 阶段1：怠速前进，累计编码器距离
            params->ctrl.stop = false;
            // 安全兜底：10秒超时强制刹车
            if (timeout > 300)
            {
                printf("[Cross] Distance timeout! Forcing stop at %.2fm\n",
                       params->ctrl.odometry);
                params->ctrl.stop = true;
            }
        }
        else
        {
            // 阶段2：距离达标→刹车
            params->ctrl.stop = true;
            countRec++; // 复用为刹车确认计数器

            // 等待速度归零后触发蜂鸣完赛信号（仅一次）
            if (!buzzerDone && countRec >= 5 && params->ctrl.speedFeedback < 0.05f)
            {
                params->ctrl.crossFinishBuzzer = true;
                buzzerDone = true;
                printf("[Cross] Stopped at %.2fm, race complete!\n",
                       params->ctrl.odometry);
            }
        }
        break;
    }
    }
}

/**
 * @brief 图形化显示FSM数据
 *
 * @param img
 */
void FsmCross::show(Mat &img)
{
    if (params->mode != FsmMode::CROSS)
        return;

    putText(img, "[8] Cross", Point(COLSIMAGE / 2 - 50, 20),
            cv::FONT_HERSHEY_TRIPLEX, 0.5, cv::Scalar(0, 255, 0), 0.5);

    switch (step)
    {
    case Step::ENABLE: // 场景使能
        putText(img, "[8] Cross - ENABLE", Point(100, 50), cv::FONT_HERSHEY_TRIPLEX, 0.5, cv::Scalar(0, 0, 255), 0.5);
        break;

    case Step::STOP: // 停车
        putText(img, "[8] Cross - STOPING", Point(100, 50), cv::FONT_HERSHEY_TRIPLEX, 0.5, cv::Scalar(0, 0, 255), 0.5);
        break;
    }
}

/**
 * @brief 设置新状�? *
 * @param step
 */
void FsmCross::setStep(Step st)
{
    step = st;
    countRec = 0;          // AI场景识别计数�?    countSes = 0;          // 场次计数�?    timeout = 0;           // 超时计数�?    params->ctrl.stop = false;
    countCross = 0;
}