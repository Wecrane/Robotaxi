# 🔌 百度大模型 API 配置指南

## 总览

你需要两个大模型，但**共用一个 API Key**：

| 大模型 | 平台 | 配置文件 | 变量 |
|--------|------|---------|------|
| 视觉多模态 | 百度一见 | `visual.py` | `VISUAL_API_URL` + `VISUAL_API_KEY` |
| 语音大模型 | 百度千帆 | `llm.py` | `api_key` |

---

## 第一步：创建 API Key（5分钟）

1. 浏览器打开 https://console.bce.baidu.com/iam/#/iam/apikey/list
2. 登录百度账号（没有就注册一个）
3. 点击「创建 API Key」
4. **复制保存**生成的 Key（只显示一次！）

---

## 第二步：配置视觉大模型 — 百度一见（15分钟）

1. 打开 https://yijian.baidu.com/
2. 进入「技能开发平台」→ 点击「创建技能」
3. 技能编排页面：
   ```
   开始节点 → 多模态大模型节点 → 结束节点
   ```
4. 配置「多模态大模型」节点：
   - 📷 图片输入：开启
   - 📝 提示词示例：
     ```
     你是一个自动驾驶系统。请分析这张赛道图片，
     识别所有交通标志、障碍物和道路元素，
     并生成结构化的行驶指令。
     ```
5. 点击右上角「发布技能」
6. 发布后获取 **API 接口地址**（类似 `https://...`）

---

## 第三步：填入代码

在小车上的文件位置：

```
~/workspace/icar_autopilot_2026th/src/
├── visual.py    ← 改这个
└── llm.py       ← 改这个
```

### visual.py 修改：

```python
# 找到这两行，替换成你的
VISUAL_API_URL = "https://xxx.yijian.baidu.com/..."   # 从一见平台获取
VISUAL_API_KEY = "your-api-key-here"                    # 从 AI 控制台获取
```

### llm.py 修改：

```python
# 找到这行，替换成你的
api_key = "your-api-key-here"  # 和上面同一个 Key
```

> ⚠️ base_url 已经在代码里预设好了，不用改。

---

## 第四步：测试

```bash
ssh root@10.25.139.199
cd ~/workspace/icar_autopilot_2026th/src
python3 start.py
```

看到「语音大模型识别成功」和「视觉大模型识别成功」就说明配置正确。

---

## 常见问题

| 问题 | 解决 |
|------|------|
| API 调用报错 403 | API Key 没填对，去控制台重新复制 |
| 视觉模型返回空 | 检查一见技能是否已「发布」（不是保存） |
| 连接超时 | 小车需要联网！确保小车能访问外网 |
