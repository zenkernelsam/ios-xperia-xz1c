# HANDOVER-USB-CDC.md — lilac USB CDC 日志通道交接（副线→主线，2026-10-26）

**一句话**：照 vermeer 的 456 行薄驱动做，lilac 无 SMMU 反而更简单；
boot-arg 选择加入，主线默认路径零改动。**动机：相机通道不稳，
AMFI/GUI 期需要实时内核日志流。**

## 为什么现在可以上（副线结论）

- `ios-vermeer-lab`（红米K70/SM8550）的 `xnu/vermeer/vermeer_usb_debug.c`
  **真机验收过**：Windows Usbser COM6 枚举、108KB 早期日志+崩溃日志、
  ping/status/replay 双向、close/reopen 活。且是
  `#if ARM64_BOARD_CONFIG_SWEET`——**同家族代码，不是移植是适配**。
- lilac 已挖好全部硬件常量（`research/usbdbg-msm8998/`）：
  DWC3 `0xA800000`、时钟/GDSC/BCR 全套、QUSB2 PHY ABL 已训好。
- **最大差异是利好**：MSM8998 USB 路径无 SMMU（ssusb 无 iommus），
  vermeer 里唯一的 `identity()` SMMU 路由检查**整段删掉**，
  DMA 直打物理地址。剩下就是：ml_io_map(0xA800000) + 16KiB 连续
  PA arena + 轮询 DWC3。
- ABL fastboot 本来就跑 DEVICE 模式 → PHY/时钟/role 全继承，
  vermeer 全程"不改 GCC/SMMU/PMIC"的保守原则直接平移。

## 三件套配方

1. **console 捕获环**（若没有现成 RAM 环）：nabu 的 `NabuLogCore.h`
   配方——`serial_putc_crlf` 挂钩 → 4MiB 静态 BSS 环，try-lock
   追加、竞争即弃。源码在 `research/nabu-xnu-log-readback/`。
2. **传输层**：`research/ios-vermeer-lab/xnu/vermeer/vermeer_usb_debug.c`
   改基址 + 删 identity() + 命令表保留 help/ping/status/replay
   （sample/frame/input 二期再挂，先把管子通起来）。
3. **boot-arg**：`lilac_usbdbg=1`（或沿用 `vermeer_usb=1`）选择加入；
   默认构建不启动，硬件状态不支持时 bail out 留正常启动——
   vermeer 本就按这个原则写。

## 验收阶梯（每步相机辅助验证）

枚举（Windows Usbser COMx 出现）→ 流式日志（早期启动文本到 host）
→ ping/status 应答 → 压力（大日志量不丢）→ 断线/重连行为记录。
**首飞时相机必须架着**：确认"USB 起来了"和"面板没死"两件事。

## 覆盖面实话（不接锅的部分）

- 只覆盖**内核 console**（printf/IOLog/kprintf + patch0043 崩溃报告）。
  userspace 的 launchd stdout 不经过这里——SweetAMFI 是内核 shim
  所以它的日志覆盖，launchd 本体消息没有。
- ①②期（shim/loader/早期 XNU）没它——PON/DDR/fbcon/相机照旧。
- **无 panic 传输**（vermeer 明说没实现）：panic 后靠 WDT 复位 +
  DDR/PON/磁盘读回。要 panic drain/截图/重启等 QcomXnuUsbDebug 二期。
- 断线即停（vermeer 设计：错误后保留内存不复活），需复位重连。

## 参考档案

- 移植档案+常量表：`research/usbdbg-msm8998/`（Sm8998.h + NOTES）
- vermeer 源码：`research/ios-vermeer-lab/`（USB-DEBUG.md 是协议手册）
- nabu 磁盘读回：`research/nabu-xnu-log-readback/`（appslog 二期配方）
- QcomXnuUsbDebug 二期：`research/QcomXnuUsbDebug/`（porting doc 已读）
- 相机降级方案：METHOD-DEBUG-PORTABLE.md "通道替换关系"节


---

## 追加（同日）：nabu-usb-cdc 真机验收修正

群友 `Lux12138/nabu-usb-cdc`（米板5/SM8150+iPadOS17.7.11）已把
QcomXnuUsbDebug **真机跑通**（COM3/179行/含 AMFI 拒绝正文）。
对本交接的修正：

1. **覆盖面扩大**：launchd/dyld 的 AMFI 拒绝（`unsuitable CT policy`
   等）**会实时流过 CDC**——原文"userspace stdout 不经过"需限定：
   kernel 侧打印的 AMFI exec 判定在覆盖内，纯 userspace printf 不在。
2. **SMMU 战役豁免**：他们 r5-r7 全部耗在 SID 占用/bypass-bank，
   lilac 无 SMMU——**最大难点不存在**，剩纯 DWC3+时钟资格检查。
3. **接入时必改顺序**：panic drain hook 必须在 restart 之前调
   （他们因顺序反了导致 panic 文本进不了串口）。
4. **主机捕获四件套**：boot 前启动捕获 / PnP+0.5s 再开 / **先置 DTR**
   / 150ms 短超时连读到端口消失。
5. 验收清单第 5 项补"日志静止 ≠ 死亡"判读规则——无新字节 30s+
   且 COM 消失才算一轮结束；`QCOM PANIC END` 缺席不能反证无 panic。

首选实现仍以 vermeer 薄驱动起步；nabu-usb-cdc 的
`module/soc/Sm8150.h`+`QcomUsbPrepare.c` 是"SOC 已就绪资格"
的最佳结构参照（lilac 大概率只用其中时钟/PHY 资格部分）。
