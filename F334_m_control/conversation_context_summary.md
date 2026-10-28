# RM26 滞环超级电容控制器：对话上下文摘要

## 1. 项目与当前任务背景

- 工程目录：`RM26_hysteresis_cap_f334`
- 当前主要关注：
  - `Core/MyCore/main_ctrl.c`
  - `Core/MyCore/scheduler.c`
  - `Core/MyCore/adc_handle.c`
  - `Core/Src/hrtim.c`
  - `Core/Src/tim.c`
  - `Core/Src/main.c`
- 用户希望以 0 基础方式理解硬件和软件，并计划将单相滞环控制器扩展为两相交错并联。
- 本文档用于交给另一个智能体继续分析，不是代码修改方案，也不代表已经实施了任何硬件或软件改动。

## 2. 当前软件整体架构

### 主循环调度器

`main.c` 完成 HAL、GPIO、DMA、ADC、DAC、HRTIM、TIM15 等初始化后调用：

```c
Scheduler_ALL_Init_Task();
```

主循环：

```c
while (1)
{
    Scheduler_run();
}
```

`scheduler.c` 使用 `HAL_GetTick()` 做软定时调度：

```c
if (now_time - scheduler_task[i].last_run >= scheduler_task[i].rate_ms)
{
    scheduler_task[i].last_run = now_time;
    scheduler_task[i].task_func();
}
```

当前 Debug 模式任务表主要包括：

| 任务 | 周期 | 作用 |
|---|---:|---|
| `led_flash` | 10 ms | LED 状态刷新 |
| `sys_data_sent` | 1 ms | 当前函数体基本为空 |
| `PowerlimitUpdata` | 1 ms | 根据电容电压生成电池功率目标 |
| `Start_task` | 1 ms | 启动和欠压关断状态机 |

Release 模式还会加入 100 ms 的看门狗刷新任务。

### 高速控制中断

`main_ctrl_Init()` 调用 `DAC_start()`，其中启动 TIM15 更新中断：

```c
HAL_TIM_Base_Start_IT(&htim15);
```

中断入口：

```c
void TIM1_BRK_TIM15_IRQHandler(void)
{
    __HAL_TIM_CLEAR_FLAG(&htim15, TIM_FLAG_UPDATE);
    main_ctrl_task();
}
```

`main_ctrl_task()` 不由 `Scheduler_run()` 调用，而是由 TIM15 硬件中断调用。

## 3. 当前主控制算法

文件：`Core/MyCore/main_ctrl.c`

全局变量：

```c
float hysteresis_width = 0.2f;
float current_save = 0;
uint8_t ctrl_enable_flag = 0;
uint8_t Cap_enable_flag = 0;
float Bat_power_set = 40;
float Cap_current_set = 0;
Pid_t Bat_power_loop = { 0 };
```

### ADC 和功率计算

`main_ctrl_task()` 首先调用：

```c
ADC_handle_task(&User_ADC_out_data);
```

ADC 数据结构包含：

```c
V_BAT
V_CAP
Current_BAT
Current_CAP
Current_Chassis
Cap_power
Bat_power
Chassis_power
```

功率计算：

```c
Cap_power      = Current_CAP     * V_CAP;
Bat_power      = Current_BAT     * V_BAT;
Chassis_power  = Current_Chassis * V_BAT;
```

### 目标电容电流

当前实际控制未启用电池功率 PID，代码中的 PID 调用已注释：

```c
// pid_calc(&Bat_power_loop, User_ADC_out_data.Bat_power, Bat_power_set);
```

实际算法是：

```c
power_delta = Bat_power_set - User_ADC_out_data.Chassis_power;
Cap_current_set = power_delta / User_ADC_out_data.V_CAP;
Clamp_de_(-36.0f, 32.0f, Cap_current_set);
Ctrl_hysteresis(Cap_current_set);
```

含义：

```text
电池目标功率 - 底盘当前功率 = 功率差
功率差 / 电容电压 = 目标电容电流
目标电流限幅到 -36 A ~ +32 A
```

注意：`V_CAP` 参与除法，代码目前没有显式的零电压/异常电压保护。

### 滞环阈值和 DAC

```c
void Ctrl_hysteresis(float current)
{
    current_save = current;
    current_H = current + hysteresis_width;
    current_L = current - hysteresis_width;
    DAC_set_value(current_H, current_L);
}
```

`main_ctrl_Init()`：

```c
Ctrl_Set_hysteresis_width(0.1f);
Ctrl_hysteresis(0.3f);
```

所以正常运行时：

```text
上限电流 = 目标电流 + 0.1 A
下限电流 = 目标电流 - 0.1 A
```

DAC 映射：

```c
value1 = (current_H * 0.002f * 20 + 1.65f) / 3.3f * 4095;
value2 = (current_L * 0.002f * 20 + 1.65f) / 3.3f * 4095;
```

等效模拟关系：

```text
阈值电压 = 1.65 V + 电流 × 0.04 V/A
```

零电流对应约 1.65 V，正负电流通过高于/低于 1.65 V 表示。

当前 DAC 关系：

```text
DAC1 → DACH，上限阈值
DAC2 → DACL，下限阈值
```

## 4. 启动、使能和保护

### 主控制使能

```c
ctrl_Enable_set(uint8_t enable)
```

控制 `EN_MOS` 和 LED3。

只有在：

```text
Cap_enable_flag == 1
且 enable == 1
```

时才允许打开主控制。

### 电容使能

```c
Cap_Enable_set(uint8_t enable)
```

控制 `Cap_En` GPIO 和 `Cap_enable_flag`。

注意：直接调用 `Cap_Enable_set(0)` 不会自动调用 `ctrl_Enable_set(0)`；正常停机流程应先关主控制，再关电容。

### `start_uvlo()`

由 1 ms 调度任务调用。

电池电压低于 `20.71 V`：

```text
进入欠压状态
约 50 ms 后关闭主控制
再约 10 ms 后关闭电容
```

电池电压高于 `21.0 V`：

```text
约 5 ms 确认
打开电容
等待约 45 ms
打开主控制
```

这是电池欠压阈值和启动阈值之间的滞回。

## 5. 原理图硬件滞环控制架构

提供的原理图中，主要硬件链路为：

```text
超级电容电流
    ↓
采样电阻
    ↓
INA240 电流采样放大器
    ↓
L_CAP 模拟反馈电压
    ├── 与 DACH 比较
    └── 与 DACL 比较
```

`L_CAP` 的软件换算对应：

```text
V_L_CAP ≈ 1.65 V + I_CAP × 0.04 V/A
```

两个 TLV3201 比较器：

```text
实际电流高于上限：
    输出 Reset to L 方向信号

实际电流低于下限：
    输出 SetH 方向信号
```

随后经过：

```text
U11 SN74AHC1G08：二输入 AND
U12 SN74AHC1G08：二输入 AND
U13 SN74LVC1G02：二输入 NOR
U14：带 D、CLK、CLR#、Q 的时序逻辑器件/触发器
```

再进入 EG2104 半桥栅极驱动器：

```text
HO → 高侧 MOSFET
LO → 低侧 MOSFET
```

功率级：

```text
24 V
  ↓
高低侧 MOSFET 半桥
  ↓
电感 L1
  ↓
CAP_FRONT / 超级电容
```

### 数字逻辑的总体作用

```text
比较器：
    判断电流是否越过上下限

HRTIM：
    提供允许的时间窗口和时序

AND/NOR：
    门控、禁止、互锁、复位组合

U14：
    在指定时钟边沿锁存状态，并支持清除

EG2104：
    把逻辑信号变成 MOSFET 栅极驱动
```

不能仅凭截图确认 U11～U14 每一根线的百分之百真值表；若要逐引脚确认，需要原理图工程或网表。

## 6. 当前 HRTIM Timer A 配置

文件：`Core/Src/hrtim.c`

当前主要配置：

```c
pTimeBaseCfg.Period = 65503;
pTimeBaseCfg.RepetitionCounter = 0x00;
pTimeBaseCfg.PrescalerRatio = HRTIM_PRESCALERRATIO_MUL32;
pTimeBaseCfg.Mode = HRTIM_MODE_CONTINUOUS;
```

Timer A 参数：

```c
HalfModeEnable = HRTIM_HALFMODE_DISABLED;
StartOnSync = HRTIM_SYNCSTART_DISABLED;
ResetOnSync = HRTIM_SYNCRESET_DISABLED;
DACSynchro = HRTIM_DACSYNC_NONE;
PreloadEnable = HRTIM_PRELOAD_DISABLED;
UpdateGating = HRTIM_UPDATEGATING_INDEPENDENT;
RepetitionUpdate = HRTIM_UPDATEONREPETITION_DISABLED;
BurstMode = HRTIM_TIMERBURSTMODE_MAINTAINCLOCK;
FaultEnable = HRTIM_TIMFAULTENABLE_NONE;
DeadTimeInsertion = HRTIM_TIMDEADTIMEINSERTION_DISABLED;
```

输出配置：

```c
TA1：
    SetSource   = HRTIM_OUTPUTSET_TIMCMP1;
    ResetSource = HRTIM_OUTPUTRESET_TIMPER;

TA2：
    SetSource   = HRTIM_OUTPUTSET_TIMCMP2;
    ResetSource = HRTIM_OUTPUTRESET_TIMCMP3;
```

比较值：

```c
CMP1 = 1600;
CMP2 = 65043;
CMP3 = 1500;
```

含义：

```text
TA1：计数到 1600 置位，周期末复位
TA2：计数到 65043 置位，下一周期计数到 1500 复位
```

当前 Timer A 的输出是外部数字逻辑的时序窗口，不一定等于最终 HO/LO 栅极波形。

当前 HRTIM 输出启动：

```c
HAL_HRTIM_WaveformCountStart(&hhrtim1, HRTIM_TIMERID_TIMER_A);
HAL_HRTIM_WaveformOutputStart(
    &hhrtim1,
    HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2
);
```

当前工程只明确启动了 Timer A、TA1 和 TA2。

## 7. EEV9 当前作用

代码：

```c
pEventCfg.Source = HRTIM_EVENTSRC_1;
pEventCfg.Polarity = HRTIM_EVENTPOLARITY_HIGH;
pEventCfg.Sensitivity = HRTIM_EVENTSENSITIVITY_FALLINGEDGE;
pEventCfg.Filter = HRTIM_EVENTFILTER_NONE;

HAL_HRTIM_EventConfig(
    &hhrtim1,
    HRTIM_EVENT_9,
    &pEventCfg
);
```

Timer A：

```c
pTimerCfg.ResetTrigger = HRTIM_TIMRESETTRIGGER_EEV_9;
```

GPIO：

```text
PB3 → HRTIM1_EEV9
```

因此链路是：

```text
PB3 下降沿
    ↓
HRTIM EEV9
    ↓
硬件复位 Timer A 计数器
    ↓
TA1/TA2 时序重新从计数起点开始
```

EEV9 当前不是软件中断：

```c
hhrtim1.Init.HRTIMInterruptResquests = HRTIM_IT_NONE;
```

因此 EEV9 不会进入 CPU 回调、不经过 scheduler，也不会直接调用 `main_ctrl_task()`。

`Filter = NONE` 表示没有 HRTIM 内部事件滤波，PB3 上的噪声/毛刺需要重点关注。

原理图中有 `#TARESET`，从命名看可能与 Timer A 复位相关，但仅凭截图不能百分之百确认它是否直接连接到 PB3/EEV9。

## 8. TIM2 ETR 结论

`tim.c` 中：

```c
TIM2：
    PA15 → TIM2_ETR
    ClockSource = TIM_CLOCKSOURCE_ETRMODE2
    Prescaler = 0
    Period = 0xFFFFFFFF
```

但工程中没有发现：

```c
HAL_TIM_Base_Start(&htim2);
HAL_TIM_Base_Start_IT(&htim2);
__HAL_TIM_GET_COUNTER(&htim2);
TIM2_IRQHandler()
```

所以：

```text
TIM2 ETR 已配置，但当前没有真正参与运行控制。
```

EEV9 与 TIM2 ETR 是两条不同路径：

```text
PA15 → TIM2 ETR：
    外部计数时钟，当前未启动

PB3 → HRTIM EEV9：
    Timer A 外部复位事件，当前配置有效
```

## 9. 两相交错改造目标

推荐结构：

```text
Timer A → A 相，0°
Timer B → B 相，180°
```

功率级：

```text
A相半桥 → L_A ─┐
                ├── CAP_FRONT
B相半桥 → L_B ─┘
```

两个半桥开关节点不能直接并联，必须经过各自电感后再并联。

建议每相独立：

```text
L_CAP_A
SetH_A / ResetL_A
L_CAP_B
SetH_B / ResetL_B
```

如果两相只共用一个电流反馈，可能出现严重抢流。

理想目标：

```text
I_A_ref = I_total_ref / 2 + 均流修正
I_B_ref = I_total_ref / 2 - 均流修正
```

## 10. 两相 HRTIM 设计结论

推荐：

```text
Master Timer：
    统一周期和相位参考

Timer A：
    0°，复制当前单相时序

Timer B：
    180°，复制 Timer A 的局部比较逻辑后整体错相
```

当前 Master Timer 截图中建议保持：

```text
Prescaler Ratio = HRTIM Clock × 32
Period = 65503
Repetition Counter = 0
Mode = Continuous
Half Mode = Disabled
Start On Sync = Disabled
Reset On Sync = Disabled
Preload Enable = Enabled（截图中已启用）
Update Gating = Independent
Repetition Update = Disabled
Burst Mode = Normal/Disabled
Compare Unit 1 = Enabled
Master CMP1 = 32751 ≈ Period/2
```

Master CMP1=32751 的用途是提供半周期参考，不等于 Timer A 的 CMP1=1600。

Timer A 建议保持：

```text
Period = 65503
Prescaler = ×32
Continuous
CMP1 = 1600
CMP2 = 65043
CMP3 = 1500

TA1：CMP1 置位，Period 复位
TA2：CMP2 置位，CMP3 复位
```

Timer B 建议：

```text
Period = 65503
Prescaler = ×32
Continuous
CMP1 = 1600
CMP2 = 65043
CMP3 = 1500

TB1：Timer B CMP1 置位，Timer B Period 复位
TB2：Timer B CMP2 置位，Timer B CMP3 复位
```

Timer B 不建议一开始同时修改比较值和相位。优先：

```text
Timer B 使用与 Timer A 相同的局部比较值
通过 Master/同步配置让 Timer B 延迟半周期启动
```

两相目标波形：

```text
TA1：
    ┌──────────────┐                    ┌──────────────┐
────┘              └────────────────────┘              └──

TB1：
                    ┌──────────────┐                    ┌──────────────┐
────────────────────┘              └────────────────────┘              └
```

其中：

```text
f_A = f_B
相位差 = 180°
单相频率约 70.35 kHz
输出合成纹波主要频率可接近 140.7 kHz
```

## 11. EEV7/EEV9 与两相交错的建议

如果新增 EEV7 判断 B 相，推荐概念分配：

```text
EEV9 → A 相
EEV7 → B 相
```

但应区分：

### 普通滞环动作

```text
A相比较器 → A相外部逻辑
B相比较器 → B相外部逻辑
```

尽量不要让普通的每次电流越界都复位整个 Timer A/B，因为：

```text
EEV9 独立复位 Timer A
EEV7 独立复位 Timer B
```

会导致两相复位时刻不同，破坏固定的 180° 相位关系。

### 严重故障

```text
FAULT_A OR FAULT_B → 公共 EN/SD#
```

或使用：

```text
FAULT_A → A相专用 EEV/Fault
FAULT_B → B相专用 EEV/Fault
```

严重故障可以允许 Timer 复位或两相同时关断。

## 12. 关闭 EEV 后的三种架构差异

### 方案 A：关闭 EEV 对 Timer 的复位，但保留外部硬件滞环

```text
Master：
    保持周期和相位

比较器 + U11～U14：
    继续快速控制本相输出

EEV：
    不再正常复位 Timer
```

这是最推荐的两相交错方向。

响应速度仍是硬件级，主要由：

```text
比较器传播延时
逻辑门延时
EG2104 驱动延时
MOSFET 栅极充放电
```

决定，不会因为 Timer 不再复位而自动变成慢速控制。

### 方案 B：关闭 EEV 和比较器控制，只由 Master 固定 PWM

```text
Master/Timer A/B → 固定 Compare → 固定 PWM
```

这会从滞环控制变成固定频率 PWM，不能自动根据实际电流上下限调整。输入电压、负载和电感变化会造成电流偏差，需要重新设计软件电流闭环。

### 方案 C：Master 固定时间基准，EEV 只快速关断本相输出

```text
Master：
    周期和相位固定

EEV/比较器：
    只复位或禁止本相当前输出

Timer 计数器：
    不被普通滞环动作清零
```

这是兼顾两相相位稳定和硬件快速响应的理想架构，但必须确认具体 HRTIM 输出复位源和外部 U11～U14 是否支持“只复位输出、不复位 Timer”。

## 13. 响应速度判断

当前开关频率约：

```text
70.35 kHz
```

周期约：

```text
14.2 us
```

如果关闭 EEV 后：

```text
仍由比较器和外部逻辑直接关断输出：
    通常不会严重减慢

必须等待 Master 下一个事件：
    可能增加 0~14.2 us 延迟，平均约 7.1 us

改为 ADC + CPU 软件判断：
    响应明显变慢且有抖动，不适合替代快速硬件滞环
```

关键结论：

```text
不是必须由 Master Timer 复位 Timer A/B。
必须由 Master/同步机制维持两相的共同时间基准和180°相位。
EEV 可以用于本相快速输出复位或严重故障。
不建议普通滞环事件频繁清零 Timer A/B。
```

## 14. 待另一个智能体继续确认的事项

1. 实际 MCU 型号是否确实支持 HRTIM Timer B；原理图曾标注 `STM32F103C6T6A`，而工程使用 `HRTIM1`，存在型号/工程不一致风险。
2. CubeMX 的 HRTIM 版本是否支持将 EEV 配置为“输出复位而不是 Timer 复位”。
3. 原理图中 `#TARESET` 是否直接连接到 PB3/EEV9，是否经过 U13/U14。
4. EEV7 对应的具体 MCU 引脚、输入极性、滤波和连接对象。
5. U11～U14 的逐引脚真值表和 `SetH`、`Reset to L`、`TA1`、`TA2`、`#TARESET` 的确切网络关系。
6. 外部逻辑是否已经提供高低侧死区；当前 HRTIM 代码明确：

```c
DeadTimeInsertion = HRTIM_TIMDEADTIMEINSERTION_DISABLED;
FaultEnable = HRTIM_TIMFAULTENABLE_NONE;
```

7. Timer A/B 采用 Master 同步启动和 180°相移的具体 CubeMX/寄存器实现方式。
8. 两相是否使用独立电流反馈和独立 DAC 阈值；若共用阈值，需要验证相间均流。
9. `main_ctrl_task()` 中 `V_CAP` 除零/异常值处理。
10. 生成代码中是否真正出现：

```c
HRTIM_TIMERINDEX_MASTER
HRTIM_TIMERINDEX_TIMER_B
HRTIM_OUTPUT_TB1
HRTIM_OUTPUT_TB2
```

## 15. 建议的示波器验证点

按信号链顺序观察：

```text
1. L_CAP_A
2. L_CAP_B
3. DACH / DACL 或各相 DAC 阈值
4. SetH_A / ResetL_A
5. SetH_B / ResetL_B
6. EEV9 输入
7. EEV7 输入
8. TA1
9. TA2
10. TB1
11. TB2
12. #TARESET / B相复位信号
13. EG2104 IN/SD#
14. HO_A / LO_A
15. HO_B / LO_B
16. A相和B相电感电流
```

重点验证：

```text
TA1/TB1 频率相同
TA1/TB1 相差180°
每相高低侧没有重叠导通
EEV 普通滞环动作不会破坏相位
严重故障能快速关断
两相电流基本均流
```

