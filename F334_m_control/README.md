# hysteresis-control
---

## 简介
滞环控制超电

## Icap A/B 采样与总电流

保留 ADC 连续扫描、循环 DMA 和 `ADC_handle_task()` 的原有读取方式。

| 变量 | 引脚 | ADC 通道 | Rank | DMA 数组位置 |
| --- | --- | --- | --- | --- |
| `V_CAP` | PB0 | ADC1_IN11 | 2 | `adc1_value[1]` |
| `Current_CAP_A` | PB14 | ADC2_IN14 | 1 | `adc2_value[0]` |
| `Current_CAP_B` | PA7 | ADC2_IN4 | 3 | `adc2_value[2]` |

`V_CAP` 保持原来的单路电压采样和换算：`ADC原始值 × 3.3 / 4095 × 11`，单位 V。
两路 Icap 使用 `Core/MyCore/current_calibration.h` 中的实测标定值：
`I_A = (ADC原始值 × 3.324 / 4095 - 1.657) / 0.020`，
`I_B = (ADC原始值 × 3.324 / 4095 - 1.658) / 0.020`，单位 A。
参考电压 3.324 V，INA240A1 增益 20 倍，采样电阻 1 mΩ。
此次仅标定电容电流及其 DAC 反馈；电压、电池电流、底盘电流暂时保留原换算参数，后续单独校准。
ADC1、ADC2 的 DMA 缓冲长度均为 3，与各自的 3 个 Rank 对应。

每次读取后计算 `Current_CAP = Current_CAP_A + Current_CAP_B`（带正负号相加）。
原 `Current_CAP` 字段保留，含义变为总电流；原有电容功率计算自动使用
`Cap_power = Current_CAP × V_CAP`，功率限制中的电流补偿也使用总电流。
完整采样入口和原有的电流单独读取入口均更新 A、B 及总电流。

## 两相平均值反馈与功率目标

比较器输入为 `I_CAP_AVG = (I_CAP_A + I_CAP_B) / 2`。
实测静态零点为 A 相 1.657 V、B 相 1.658 V、平均输出 1.668 V，
平均输出相对 A/B 算术平均偏高约 10.5 mV；DAC 换算使用 1.668 V 作为平均反馈零点、3.324 V 作为参考电压。
DAC 阈值为 `1.668 + 0.020 × (平均电流目标 ± 滞环半宽)`，再按 `4095 / 3.324` 转成码值。
该标定修正直流偏差；平均级信号增益暂按 1，DAC 各通道的实际偏移仍需测量后复核。
`Cap_current_set` 保持总电流含义：正常带底盘负载时计算 `(Bat_power_set - Chassis_power) / V_CAP`，
按总电流限幅至 -36～32 A，再乘以 0.5 传入 `Ctrl_hysteresis()`。
例如目标电容功率 30 W、端电压 12 V 时，总电流目标为 2.5 A，平均电流目标为 1.25 A。
`Ctrl_hysteresis()` 的直接参数及滞环半宽仍使用平均电流单位；初始化半宽 0.3 A 对应总电流半宽 0.6 A。
电容电压低于 1 V 或功率/电压计算输入无效时，总电流目标设为 0，避免除零后请求满电流。

`V_real_measure = V_CAP - Current_CAP * 0.115` 用于电压分段功率限制，假定充电为正电流。
0.115 Ω 是待按实物校准的等效电阻；功率转电流仍使用端电压 `V_CAP`。
功率目标在补偿电压不高于 23.8 V 时为 30 W，在 23.8～24.3 V 之间线性降至 2 W，
达到及超过 24.3 V 后保持 2 W。电压、电流从同一次控制中断更新中读取。
电池功率 PID 由主循环调度 `power_loop_task()` 每 10 ms 执行，使用新完成的 10 ms 电池功率均值。
PID 不在 100 kHz 的 TIM15 中断中执行，同一统计窗口不重复积分。
保留原参数 P=0.05、每次更新 I=0.001、D=0，输出作为 ±10 W 的功率修正量：
当前空载诊断下 `power_delta = Bat_power_set + battery_power_correction`，再除以 `V_CAP` 得到总电流目标。
未使能、低电压或数据无效时清除 PID 历史及修正量；初始化完成后才启动定时中断。
这些参数对应 10 ms 更新周期，仍需实板确认方向、采样准确性并调参，不能照搬 100 kHz 下的积分效果。

当前正在进行底盘空载隔离诊断：`main_ctrl_task()` 暂时不减去 `Chassis_power`，
原始底盘功率仍通过串口发送，采样尖峰不参与电流指令。接入底盘负载前需恢复底盘功率补偿。
实物参数已确认为两相各 1 mΩ、INA240A1（20 倍）、22 μH。

## 工作区编译

打开 `hysteresis_cap_f334.code-workspace` 后，按 `Ctrl+Shift+B` 执行 GCC 编译任务。
使用现有 `tools/build.ps1`，需要 ARM GCC 位于 PATH 中，以及本机 STM32Cube F3 V1.11.6 固件包。
输出文件位于 `MDK-ARM/gcc/`。

## 串口波形输出

- USART1：`PB6 / TX` 接 USB 转串口模块的 RX，GND 共地，使用 3.3 V TTL 电平。
- 串口参数：`115200` 波特率、8 数据位、无校验、1 停止位、无流控。
- VOFA+ 选择 [JustFloat](https://www.vofa.plus/docs/learning/dataengines/justfloat/) 协议。
- 前 8 个通道与 `C:\zhihuanchaodian\RM26_hysteresis_cap_f334` 一致，保留总电流目标并追加诊断通道。
- 每 10 ms 发送一帧（目标 100 Hz），每帧为 14 个小端 `float32` 加帧尾 `00 00 80 7F`，共 60 字节，无帧头，约占用 52.1% 的串口带宽。

| 通道 | 变量 | 含义及单位 |
| --- | --- | --- |
| 0 | `V_BAT` | 电池电压 / V |
| 1 | `V_CAP` | 超级电容电压 / V |
| 2 | `Current_BAT` | 电池电流 / A |
| 3 | `Current_CAP` | 超级电容总电流（A+B）/ A |
| 4 | `Current_Chassis` | 底盘电流 / A |
| 5 | `Cap_power` | 超级电容功率 / W |
| 6 | `Bat_power` | 电池功率 / W |
| 7 | `Chassis_power` | 底盘功率 / W |
| 8 | `Cap_current_set` | 超级电容总电流目标（A+B，限幅后、乘 0.5 前）/ A |
| 9 | `Current_CAP_A` | A 相电流 / A |
| 10 | `Current_CAP_B` | B 相电流 / A |
| 11 | `Current_CAP_mean` | 连续 10 ms 采样窗口内的总电流均值 / A |
| 12 | `Cap_power_mean` | 连续 10 ms 采样窗口内的电容功率均值 / W |
| 13 | `Bat_power_mean` | 连续 10 ms 采样窗口内的电池功率均值 / W |

前 9 个通道顺序不变，无占位通道。将通道 8 的 `Cap_current_set` 与通道 11 的 `Current_CAP_mean` 对比，
再检查通道 9/10 的分相电流。功率观察通道 12/13，原始电流及功率仍保留，可与均值同时观察。
均值每累计 1000 次 TIM15 控制更新后发布一次，功率均值为瞬时 `V × I` 的平均；不使用均值控制 DAC。
均值是软件采样的统计结果，不能保证消除所有采样混叠，也不能代替示波器测量实际电感电流。
发送时在同一个中断保护区内读取当前采样、目标和最新完成的统计窗口；窗口均值反映此前 10 ms。

初始化顺序为 `Uart_Tx_Init()` → `Vofa_init()`；调度器调用 `sys_data_sent()`
将控制中断更新的数据快照入队，再由 `Uart_Tx_task()` 通过 DMA 发送。
DMA 忙时保留待发数据，缓冲不足时丢弃新帧，避免产生残缺帧。
当前仅启用遥测发送，未启用串口命令接收。

TIM15 的控制中断周期为 10 μs。ADC 换算中的固定比例在编译时预先计算，
避免新增采样后反复执行浮点除法，导致中断占满 CPU、毫秒计数及串口/LED 任务无法运行。

---

## 说明
此库非公开库，仅限私人使用
