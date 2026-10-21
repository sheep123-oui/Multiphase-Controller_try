# F334 单相滞环超级电容控制器 调试总结

- 日期：2026-10-21
- 工程：`C:\n_Multiphase Controller\F334_m_control`
- 仓库：`https://github.com/sheep123-oui/Multiphase-Controller_try.git`，当前分支 `single-phase-hysteresis`
- 参考工程：`C:\zhihuanchaodian\RM26_hysteresis_cap_f334`（RM26 单相滞环超容控制器）
- 目标：把应用层控制逻辑与参考工程对齐，在赛场板上先验证单相滞环控制器能正常运行

---

## 1. 硬件与拓扑

| 项目 | 内容 |
|---|---|
| MCU | STM32F334C8T6 |
| 时钟 | HSE 8 MHz × PLL9 = 72 MHz；HRTIM 144 MHz；ADC 72 MHz；APB1 36 MHz |
| 拓扑 | 半桥 + 电感 L1 + 超容，**buck 结构**（24V 输入侧到超容侧） |
| 电流采样 | INA240 + 2 mΩ 采样电阻，增益 20；底盘电流采样电阻 1 mΩ |
| 阈值给定 | DAC1（PA4，DACH）与 DAC2（PA6，DACL）输出滞环上下限 |
| 比较 | 两路 TLV3201 与 DAC 阈值比较 |
| 逻辑 | SN74AHC1G08 AND ×2、SN74LVC1G02 NOR、D 触发器 U14 |
| 驱动 | EG2104 半桥栅极驱动器 |
| 滞环复位 | PB3（EEV9，下降沿）复位 HRTIM Timer A 计数器 |
| 使能 | PB15 EN_MOS 驱动使能；PA12 CAP_EN 电容预充 |

**关键约束**：HRTIM TA1 在 CMP1=1600 置位、周期末 65503 复位，最大占空比约 97.6%。
24V 输入时输出上限约 23.4V，**目标 24V 时输入电源必须设到 25~26V**，否则环路永远饱和。

---

## 2. 引脚与 ADC 通道映射（当前代码）

| 数组位置 | ADC 通道 | 引脚 | 变量 | 换算 |
|---|---|---|---|---|
| adc1[0] | CH13 | PB13 | Current_BAT | (V−1.65)/20/0.002 |
| adc1[1] | CH11 | PB0 | V_CAP | ×11 |
| adc1[2] | CH12 | PB1 | V_BAT | ×11 |
| adc2[0] | CH14 | PB14 | Current_CAP | (V−1.65)/20/0.002 |
| adc2[1] | CH12 | PB2 | Current_Chassis | (V−1.65)/20/0.001 |
| adc2[2] | CH4 | PA7 | 未使用 | — |

- ADC1 三通道扫描，缓冲长度 3；ADC2 三通道扫描，缓冲长度 3。
- 通道索引已按实测修正：V_BAT 用 `adc1_value[2]`，V_CAP 用 `adc1_value[1]`。
- V_BAT 经一阶 RC 滤波（系数 0.1），V_CAP 未滤波。
- **待核对**：两路电压的 ×11 分压系数是否与赛场板实际一致。此前出现"输出读数约 35V、实际约 24V"的现象，怀疑实际分压比约 7.5:1，闭环前必须用万用表校准。

---

## 3. 软件架构与运行频率

启动顺序（main.c）：GPIO → DMA → ADC1 → ADC2 → CAN → DAC1 → HRTIM1 → USART1 → TIM15 → DAC2 → `Scheduler_ALL_Init_Task()`。

`Scheduler_ALL_Init_Task`：`Vofa_init()` → `ADC_handle_Init()` → `main_ctrl_Init()`。

| 环节 | 频率 / 周期 | 说明 |
|---|---|---|
| ADC 硬件转换 | 每通道约 250 kHz，三轮 1.33 µs | 19.5 采样 + 12.5 转换周期，连续扫描 + 循环 DMA |
| 控制环 `main_ctrl_task` | 100 kHz / 10 µs | TIM15 更新中断，ARR=9；备选 10 kHz（ARR=99） |
| 功率表 `PowerlimitUpdata` | 1 kHz | 由调度器 1 ms 任务驱动 |
| 启动/欠压状态机 `Start_task` | 1 kHz | 同上 |
| 遥测 `sys_data_sent` | 100 Hz | 每 10 ms 发送 8 个 float |
| 硬件滞环开关 | 上限 70.3 kHz | 4.608 GHz / 65504，实际由电流纹波决定 |

遥测格式（Vofa+ JustFloat，USART1 115200，DMA 直发）：
`V_BAT, V_CAP, Current_BAT, Current_CAP, Current_Chassis, HAL_GetTick, main_ctrl_run_cnt, Cap_power`

---

## 4. 控制逻辑要点

- 功率表：V_CAP 减去内阻压降（I×0.115）后线性插值
  - 电压点：0 / 12 / 17 / 20.8 / 24.3 V
  - 功率值：45 / 200 / 150 / 60 / 30 W
  - 高于 24.3V 时目标 2W；总输出限幅 2~200W
- 主控环：目标电流 = (Bat_power_set − Chassis_power) / V_CAP，限幅 −36~+32 A，滞环宽度 0.1 A
- 上电时序：V_BAT > 21.0V 时 5 ms 去抖 → 开 PA12 预充 → 等 45 ms → 开 PB15 使能
- 欠压保护：V_BAT < 20.71V 时 50 ms 后关主控、再 10 ms 关预充
- 已知隐患：V_CAP 接近 0 时目标电流被钳到 32A（除零保护缺失）；`ctrl_check()`（V_BAT>30V 关断）被注释

---

## 5. 本次会话完成的修改

### 5.1 应用层移植（Core/mylib）
以参考工程 `Core/MyCore` 为基础整体移植，之后本地改动：

| 文件 | 改动 | 原因 |
|---|---|---|
| `adc_handle.h` | `ADC2_VALUE_SIZE` 2 → 3 | ADC2 扫描 3 通道，循环 DMA 长度必须一致 |
| `adc_handle.c` | V_BAT/V_CAP 改用 `adc1_value[2]`/`adc1_value[1]` | 按实测通道对应关系修正 |
| `Vofa_send.c` | `Data_to_repeatTool` 1 → 0 | 串口转发管线未启用，改为 DMA 直发 |
| `scheduler.c` | 启用 `Vofa_init()`；`sys_data_sent` 发送 8 通道、周期 10 ms；启用 `{Start_task,1,0}` | 遥测与上电时序 |
| `main_ctrl.c` | 增加 `main_ctrl_run_cnt` 诊断计数；`HRtim_start()` 当前注释（静态验证） | 诊断与安全 |
| `main_ctrl.h` | 增加 `main_ctrl_run_cnt` 的 extern 声明 | 同上 |

### 5.2 中断与 CubeMX 配置
- `F334_m_control.ioc`：恢复 `NVIC.TIM1_BRK_TIM15_IRQn`，使能 TIM15 更新中断（用户在 CubeMX 中重新勾选）。
- `Core/Src/tim.c`：生成区已包含 `HAL_NVIC_SetPriority/EnableIRQ(TIM1_BRK_TIM15_IRQn,0,0)`。
- `stm32f3xx_it.c` 不生成 `TIM1_BRK_TIM15_IRQHandler`，中断函数由 `main_ctrl.c` 独占，避免重复定义。

### 5.3 构建配置（.eide/eide.yml）
- mylib 虚拟目录加入 24 个构建文件（含 status_led_langue、queue_pool、Anotc、uart_user_cmd 四组）。
- 排除列表：`status_tip.c`（引用不存在的 BUZZ/Cap_Data）、`status_led_langue.c`（AC5 不支持 0b 字面量）。
- AC5 优化级别由 level-0 改为 **level-3**（解决 100 kHz 中断占满 CPU 的问题）。

### 5.4 LED 模块（用户处理）
- `status_led_langue.h` 整体注释，`.c` 移出构建，`Uart_data_handle_CallBack` 等三处 LED 调用注释。
- 原因：AC5 不支持 `0b` 二进制字面量，22 处语法错误；后续如需恢复，全部改为十六进制即可。

### 5.5 其他由 CubeMX 生成对齐的项
HRTIM 删除 Master/Timer B、Timer A 参数与参考一致；TIM2 删除；USART1 RX DMA 改为 CIRCULAR；DAC2 初始化顺序调整；`main.h` 中 LED3/Cap_En 兜底定义；EEV7/PB4 与 DAC1 第二通道残留未清理（无功能影响）。

---

## 6. 调试过程与问题定位

| 现象 | 根因 | 处理 |
|---|---|---|
| 编译 34 个错误，集中在 status_led_langue 与 uart_user_cmd | AC5 不支持 `0b` 二进制字面量；LED_NUM=2 但数组给了 4 个初值 | 用户暂时注释 LED 模块并移出构建 |
| Vofa 收到的数据全为 0 | `.ioc` 中 `NVIC.TIM1_BRK_TIM15_IRQn` 丢失，TIM15 中断从未使能 | 在 CubeMX 的 NVIC 表勾回 Enabled，Code Generation 页取消生成 IRQ Handler |
| 修 NVIC 后完全收不到数据 | level-0 优化下 100 kHz 中断占满 CPU，SysTick 冻死，调度器不发送 | 临时降至 10 kHz；后将 AC5 优化改为 level-3，恢复 100 kHz |
| 无超容、24V 空载时输入持续被限流 | V_CAP≈0 → 45W/0 钳到 32A；无负载，能量无处去；输入进入恒流，母线跌破 UVLO 阈值后反复关断重启 | 属预期失控；建议无容测试时关闭 PWM 或加电压限幅 |
| 关闭 PWM 后输出 0V | buck 关断时高侧体二极管反向，输入到输出无通路 | 属正常现象；"空载保持 24V"必须带开关并加电压环，且输入需 >25V |
| 事件中输出读数约 35V | 与 24V 输入矛盾；怀疑 V_CAP 分压系数与代码 ×11 不符（约 7.5:1） | 待万用表校准分压比后修正系数 |

---

## 7. 电压环设计方案（已在对话中给出，尚未写入代码）

**结构**：外环电压 P → 输出电流指令 → 现有硬件滞环内环（DAC 阈值 + 比较器 + EEV9）。

**原理**：被控对象是电容，电流到电压为积分关系，纯 P 控制的闭环为一阶系统，理论上不振荡；加 I 变成二阶，是振荡来源；D 只放大 ADC 噪声。

**参数选择**：闭环时间常数 τ = C / Kp，要求 τ ≥ 10 倍环路周期；Kp 从 0.01~0.03 A/V 起步逐次翻倍。

```c
#define V_LOOP_ENABLE   1
#define V_LOOP_REF      24.0f    /* 目标电压，输入需 >= 25V */
#define V_LOOP_KP       0.02f    /* A/V，从 0.01 开始往上翻倍 */
#define V_LOOP_IMAX     0.3f     /* A，空载测试限流 */
#define V_LOOP_VMAX     25.5f    /* V，硬限压兜底 */

#if V_LOOP_ENABLE
static float v_cap_filt = 0.0f;

void main_ctrl_task(void)
{
    ADC_handle_task(&User_ADC_out_data);

    v_cap_filt += 0.1f * (User_ADC_out_data.V_CAP - v_cap_filt);

    float i_cmd = V_LOOP_KP * (V_LOOP_REF - v_cap_filt);
    Clamp_de_(-V_LOOP_IMAX, V_LOOP_IMAX, i_cmd);

    if (v_cap_filt > V_LOOP_VMAX)
    {
        i_cmd = 0.0f;
    }

    Ctrl_hysteresis(i_cmd);
}
#endif
```

**整定步骤**：
1. 先校准 V_CAP 分压系数，否则环会稳到错误的实际电压。
2. `V_LOOP_REF=12`、`V_LOOP_KP=0.01`、`I_MAX=0.2`，电源 25V/0.5A，观察电压平滑爬升到 12V。
3. Kp 每次翻倍，直到出现明显超调，退回上一档的一半。
4. 目标改 24V，确认稳定在 24V 附近；停在 23.4V 说明输入电压不够。
5. 若需消除稳态误差再加 I，积分时间 100~200 ms，积分限幅 ±I_MAX，并重新验证低频摆动。

**以后接超容**：不要直接替换功率环，取 `i_cmd = min(功率环指令, 电压环指令)`，电压环只在接近满电时起限幅作用。

---

## 8. 当前状态（2026-10-21）

- 分支 `single-phase-hysteresis`；工作区有 18 个未提交修改；提交 `9359c50`（main）与 `9898649`（分支）仅为本地提交，**尚未推送到 GitHub**。
- 当前固件：TIM15 100 kHz 控制环；`HRtim_start()` 注释（PWM 关闭，静态验证模式）；DAC 阈值更新正常；遥测 100 Hz 正常；上电/欠压状态机启用。
- LED、串口命令接收、CAN、看门狗均处于禁用状态。

**待办**：
1. 万用表核对 V_CAP/V_BAT 分压比，必要时修正 `adc_handle.c` 的 ×11 系数。
2. 确认 buck 拓扑下关断时输出 0V 属正常后，决定是否实施电压环。
3. 若实施电压环：按第 7 节加入并整定，输入电源 25~26V、限流 0.5~2A。
4. 接回超容后恢复功率环，电源限流建议 3~5A，观察功率表与 UVLO 行为。
5. 清理项：HRTIM EEV7/PB4 残留、DAC1 第二通道/PA5、CAN 波特率（当前 1 Mbps，参考 750 kbps）、TIM15 TRGO 与 DAC 触发方式。

---

## 9. 常用命令

EIDE 构建（图形界面）。命令行语法检查（armcc 5.06）：

```bash
ARMCC="/c/Users/LOCK065/AppData/Local/Keil_v5/ARM/ARMCC/bin/armcc.exe"
INC=(-I"Core/Inc" -I"Core/mylib" -I"Drivers/STM32F3xx_HAL_Driver/Inc" \
     -I"Drivers/STM32F3xx_HAL_Driver/Inc/Legacy" \
     -I"Drivers/CMSIS/Device/ST/STM32F3xx/Include" -I"Drivers/CMSIS/Include" \
     -I".cmsis/include" -I"MDK-ARM/RTE/_F334_m_control")
"$ARMCC" --c99 -c --cpu=Cortex-M4 --diag_suppress=1 --diag_suppress=1295 \
  -DUSE_HAL_DRIVER -DSTM32F334x8 "${INC[@]}" -o /tmp/x.o Core/Src/main.c
```

构建日志：`build/F334_m_control/compiler.log`
