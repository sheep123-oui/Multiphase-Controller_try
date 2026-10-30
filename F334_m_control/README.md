# hysteresis-control
---

## 简介
滞环控制超电

## 串口波形输出

- USART1：`PB6 / TX` 接 USB 转串口模块的 RX，GND 共地，使用 3.3 V TTL 电平。
- 串口参数：`115200` 波特率、8 数据位、无校验、1 停止位、无流控。
- VOFA+ 选择 [JustFloat](https://www.vofa.plus/docs/learning/dataengines/justfloat/) 协议。
- 每 10 ms 发送一帧（目标 100 Hz），每帧为 8 个小端 `float32` 加帧尾 `00 00 80 7F`，共 36 字节。

| 通道 | 变量 | 含义及单位 |
| --- | --- | --- |
| 0 | `V_BAT` | 电池电压 / V |
| 1 | `V_CAP` | 超级电容电压 / V |
| 2 | `Current_BAT` | 电池电流 / A |
| 3 | `Current_CAP` | 超级电容电流 / A |
| 4 | `Current_Chassis` | 底盘电流 / A |
| 5 | `Cap_power` | 超级电容功率 / W |
| 6 | `Bat_power` | 电池功率 / W |
| 7 | `Chassis_power` | 底盘功率 / W |

初始化顺序为 `Uart_Tx_Init()` → `Vofa_init()`；调度器调用 `sys_data_sent()`
将控制中断更新的数据快照入队，再由 `Uart_Tx_task()` 通过 DMA 发送。
DMA 忙时保留待发数据，缓冲不足时丢弃新帧，避免产生残缺帧。
当前仅启用遥测发送，未启用串口命令接收。

---

## 说明
此库非公开库，仅限私人使用
