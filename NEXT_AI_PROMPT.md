# 上位机代码乱码修复 — 逐一修复

## 问题

`icar_autopilot_2026th/src/fsm/` 下10个 .cpp 文件出现编码损坏：`�`（U+FFFD）替代中文字符。`�` 出现在注释末尾**吃掉换行符**，下一行代码变注释，编译过但逻辑缺失。

## 损坏特征

```
// 某中文注释�?    下一行代码;
```
`�?` = 损坏汉字 + 被吃掉的 `\n`。修复：恢复正确汉字 + 补回换行。

常见模式：
- `状�? *` → `状态\n *`
- `学�?.` → `学习).`
- `渠�?.` → `渠道).`
- `未识�?    {` → `未识别\n    {`
- `计数�?    xxx` → `计数器\n    xxx`
- `�?.5秒` → `约0.5秒`
- `�?0Hz` → `约60Hz`
- `控�? * @version` → `控制\n * @version`
- `结�?    if` → `结果\n    if`

## 当前状态（全部未修复）

| 文件 | 乱码数 |
|------|:---:|
| `cross.cpp` | 26 |
| `manualControl.cpp` | 10 |
| `slow.cpp` | 13 |
| `busy.cpp` | 33 |
| `fork.cpp` | 106 |
| `obstacle.cpp` | 27 |
| `park.cpp` | 121 |
| `station.cpp` | 23 |
| `stop.cpp` | 39 |
| `yfork.cpp` | 43 |
| **合计** | **441** |

## 修复方法

1. `read_file` 读取文件段落，识别 `�` 位置
2. 根据上下文 + 同目录 `.hpp` 头文件推断原始汉字
3. `multi_replace_string_in_file` 批量修复（含3-5行上下文防重复匹配）
4. `grep_search` 搜索 `�` 确认清零

## 参考

- 干净头文件：`include/fsm/*.hpp`
- 原始代码备份：板卡 `~/workspace/icar_autopilot_2026th.tar.gz`

## 快速检查

```powershell
$bytes = [IO.File]::ReadAllBytes("g:\完全模型\code\icar_autopilot_2026th\src\fsm\XXX.cpp")
([regex]'�').Matches([Text.Encoding]::UTF8.GetString($bytes)).Count
```

## 修复顺序

`cross.cpp` → `manualControl.cpp` → `slow.cpp` → `busy.cpp` → `fork.cpp` → `obstacle.cpp` → `park.cpp` → `station.cpp` → `stop.cpp` → `yfork.cpp`

## 验证

全部修复后板卡编译：
```bash
ssh root@10.25.139.199
cd ~/workspace/icar_autopilot_2026th/build
make -j1   # 单线程！防 OOM
```

## 板卡

- IP：`10.25.139.199` / `root` / `root`
- 本地 `g:\完全模型\code\icar_autopilot_2026th\` = Samba 挂载
- RTC 校准：`date -s "2026-07-01 HH:MM:SS"`
- 密码不写命令中
