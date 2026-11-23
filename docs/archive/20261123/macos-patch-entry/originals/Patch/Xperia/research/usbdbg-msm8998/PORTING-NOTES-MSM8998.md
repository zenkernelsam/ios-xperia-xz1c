# QcomXnuUsbDebug → MSM8998/lilac 移植档案（2026-10-26）

副线摸底成果：**所有常量均来自 sony-kernel-4.4 stock 源码树，零逆向**。
`Sm8998.h` 草案在同目录。

## 证据表（源文件 → 值）

| 项 | 值 | 出处 |
|---|---|---|
| DWC3 core | 0xA800000（窗 0xCD00，wrapper 0xF8F00） | msm8998.dtsi `ssusb@a800000`/`dwc3@a800000` |
| USB BAM | 0xA904000 | `qcom,usbbam@a904000`——模块走直控 DWC3，**不用 BAM** |
| QUSB2 HS PHY | 0xC012000 + TCSR clamp 0x1FCB24C + ref_clk 0x188014 | `qusb_phy0`，init seq 内嵌 |
| QMP SS PHY | 0xC010000 | lilac 板级 `maximum-speed="high-speed"` → **不需要**（模块也只支持 FS/HS，完美匹配） |
| GCC | 0x100000（与 SM8550 同基址） | `qcom,gcc@100000` |
| 时钟使能位 | master 0xF008 / sleep 0xF00C / mock_utmi 0xF010 / cfg_noc_axi 0x5018 / aggre1_axi 0x82024 / phy_cfg_ahb2phy 0x6A004 | gcc-msm8998.c `.enable_reg` |
| 复位 | USB_30_BCR = 0xF000 | gcc-msm8998.c |
| 电源域 | gdsc_usb30 = 0x10F004 | msm-gdsc-8998.dtsi |
| **SMMU** | **无**（ssusb 无 iommus；8998 仅 mmss/kgsl/lpass 过 SMMU） | 全 dtsi 反证 |
| VBUS/Type-C | PMI8998 extcon（PMIC 侧），非 Kalama QSCRATCH | `extcon = <&pmi8998_pdphy>` |
| 中断 | dwc3=131, hs_phy=347, ss_phy=243, pwr_evt=180, bam=132 | DT |
| 固件交接 | ABL fastboot 本来就跑 DEVICE 模式 → PHY/时钟/role 已就绪 | 实测事实 |

## 与上游设计的三处平台差异（需写平台代码，不只是头文件）

1. **无 SMMU** → "私有 S1 域"退化为"物理连续 arena"（IOVA=PA）。
   上游禁止 identity 回退的规则针对的是"SMMU 路径上做 bypass"——
   8998 根本没 SMMU，直 PA 即原生路径，不违反设计意图。
2. **VBUS 判定**：Kalama 用 QSCRATCH bit20/28；lilac 由 PMIC 报告。
   qualify 应改查 DSTS 链路态/PHY 寄存器，别抄 SM8550 的 VBUS 位。
3. **PHY 继承**：ABL fastboot 已把 QUSB2 训好 → 走"固件已就绪"
   资格路径即可，不需要冷启动 PHY（上游也没实现冷启动）。

## 对我们天然适配的点

- **XNU 版本精确对口**：上游补丁锁 8796.141.3 = 我们的 sweet 基线。
- **cpus=1**：上游"只有启动 CPU 服务硬件"的限制对我们零成本。
- **截图 provider 现成**：fbcon fb @0x9D400000 CPU 映射、720×1280×32bpp
  → RGB888 快照 2.76MiB < 16MiB 上限。
- **内存预算 ~20.3MiB**（4MiB 积压+16KiB 队列+32KiB DMA+12KiB 页表
  +16MiB 快照），4GB 机上无感。
- **与无人值守栈零冲突**：XNU 活→USB CDC；死→WDT 回 fastboot 收割。
  控制权天然随复位交接回 ABL，两个通道永不争抢。

## 接入决策：现在准备，暂不并入主线

**上游成熟度**：README 明言"尚未接入活动设备/尚待完整内核链接和
实机验收"——本模块是 socrates 原实现的重构版，**任何设备都还没
真机验收过这版代码**。现在并入 lilac = 同时首验"内核链接+新 SoC
移植+真机"三件事，叠加在 AMFI 主线战场上，风险/收益不成比。

**而且当前堵点（launchd AMFI/userspace）靠 fbcon+相机已覆盖**，
USB CDC 的边际收益要等 GUI/backboardd 阶段才爆发。

**触发条件**（满足其一即并入，走 dormant 分支 + boot-arg 关闭）：
- 主线进入 GUI/backboardd 调试，需要实时日志流+远程截图；
- 隔壁撞上 PON/DDR/fbcon 解码不了的 hang；
- 上游发布首个真机验收。

## 待办（接入时）

- [ ] `soc/` 平台代码：无-SMMU arena + PMIC VBUS 资格 + 8998 时钟组
- [ ] `tests/check.py` 矩阵加 msm8998 profile（上游要求新 profile 显式注册）
- [ ] 内核侧：`patches/darwin22` 应用 → `XNU_QCOM_USB_DEBUG=1` 编译 →
      boot-args `qcom-usb-debug=1`（控制另加 `qcom-usb-ctl=1`）
- [ ] 首验顺序：枚举 → 固定日志 → worker → 压力 → 控制 → reboot → 截图
- [ ] 截图 provider：注册 fbcon 扫描映射（fb @0x9D400000）
- [ ] PSCI SMC reset 单独验收后再置 QUD_PSCI_SMC_RESET=1

---

## 2026-10-26 增补：vermeer 薄驱动是更贴的第一跳

群友 `ios-vermeer-lab`（红米K70/SM8550）的 `xnu/vermeer/vermeer_usb_debug.c`
（456 行）已**真机验收**：Windows Usbser COM6、108KB 早期日志、
ping/status/replay/sample 双向查询、close/reopen 活。关键：
**`#if defined(ARM64_BOARD_CONFIG_SWEET)`——sweet 家族代码**。

与本档案的差异：它用"identity SMMU 路由+复用 ABL PHY/时钟"，
全程不改 GCC/SMMU/PMIC——**lilac 无 SMMU，连 identity 路由都不用**，
只需 DWC3 regs + 16KiB 连续 PA arena。无 panic 传输/截图/重启。

修订接入策略：**第一跳照 vermeer 薄驱动做**（枚举+日志流+只读查询，
工作量从"平台移植"降为"改基址+去 SMMU 路由"），QcomXnuUsbDebug
降级为二期（要 panic drain/截图/重启时再叠加）。两仓均在 research/。
