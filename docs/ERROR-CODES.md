# 故障码 / Error codes

屏幕显示打印机上报的代码；完整文案保存在文档中，不占固件空间。未收录的新代码也能显示。
The screen displays reported codes, including unknown codes. Descriptions are kept in documentation, outside the firmware.

HMS 是健康监测报告，包含警告与故障，不需要等打印错误出现。HMS 与当前打印错误在同一个弹窗汇总；先后上报会更新列表，相同代码不反复弹出。具体触发、确认及清除规则见[提示与故障弹窗](PROMPTS.md)。

| 类型 / Type | 格式 / Format | 对照 / Reference |
| --- | --- | --- |
| HMS 健康状态 | `HMS 0500-0500-0001-000F` | [3,267 条 / entries](HMS-CODES.md) |
| 打印错误 / Print error | `0x07018004` | [1,117 条 / entries](PRINT-ERROR-CODES.md) |

在对应文档中搜索屏幕代码。同码随机型变化的说明保存在原始数据的 `variants` 中；缺少中文时保留英文，不推测翻译。
Search by the displayed code. Model-specific descriptions are retained in `variants`; English is kept when Chinese text is unavailable.

**[在线故障码查询 / Online code lookup](https://tbc0309.github.io/xtouch-CN/errors.html)**：支持前缀自动匹配、HMS 格式、十六进制和十进制命令错误码。

## 数据来源 / Source

2026-10-16 核对 BambuStudio 提交 `79df7ae309be398ed30c9f397c13c9911c2edc57`，按照其 DevHMSQuery 使用的厂商查询方式收集十组设备资料。HMS、打印错误及操作规则仅采用这次 BambuStudio 对应的资料，清除此前混合的社区历史条目。

The reference follows BambuStudio's vendor queries for ten device groups, pinned to the commit above. Previous community and historical unions have been removed. This snapshot does not guarantee coverage of future codes.

重试／完成按钮依据 [DeviceErrorDialog.cpp](https://github.com/bambulab/BambuStudio/blob/79df7ae309be398ed30c9f397c13c9911c2edc57/src/slic3r/GUI/DeviceErrorDialog.cpp)：先处理 Studio 明确的通用重试规则，其余按设备序列号前三位匹配动作表。动作 7 对应完成，8／9 对应恢复；其它动作不擅自替换成 AMS 重试。未知规则只显示确认。

Retry/Done follows Studio's explicit overrides and model-scoped action tables, selected by the serial-number prefix. Other action types are not converted into AMS retry commands. Unknown rules show Confirm. Controls still depend on printer permissions.

## 更新 / Update

```sh
python scripts/refresh_error_reference.py
python scripts/generate_error_docs.py
```

原始数据为 `docs/data/error-codes.json`，记录 Studio 提交、设备来源、资料版本及文案变体。更新失败不会覆盖原文件；普通编译不联网抓取故障库。
The JSON records the Studio commit, device sources, reference versions and description variants. Failed refreshes preserve the existing file. Normal builds do not download this database.
