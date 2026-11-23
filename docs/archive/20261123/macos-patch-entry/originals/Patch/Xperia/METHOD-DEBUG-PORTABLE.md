# METHOD-DEBUG-PORTABLE — 无串口设备 iOS 移植方法论（lilac 验证 → 泛化）

> 从 XZ1C/lilac 调试栈提炼的可移植方法论。目标：换一台设备（Y700 Gen4、
> 或任何无 UART 的手机）时，只填一张常量表 + 重测少数寄存器，
> 整套"永不卡死 + 全通道取证"能力立刻可用。

## 一、分层模型（与设备无关的核心设计）

```
L0 恢复保证   —— 任何死法都落回可到达模式（Android 熄屏 / fastboot）
L1 遥测通道   —— 无串口无屏时代，从复位后的世界读回崩溃现场
L2 自动化     —— 发射→取证→判读一键化，夜间无人值守可跑
L3 构建/打包  —— payload/镜像/rootfs 生成，注入测试标记
```

### L0 恢复保证（lilac 实现 → 移植要求）

| 机制 | lilac 实现 | 移植要点 |
|---|---|---|
| 软复位路由 | PMIC PON `0x88f`: `0x04`=RECOVERY→Android(熄屏), `0x06`→fastboot | **每机重测**。PON 寄存器地址/位定义随 PMIC 代际变（PM8998→PM8550 不同）。先写 PON 探测 beacon 枚举 SPMI |
| 永不进充电图标 | 所有死路先写已知 reason 再死；未知 reason=充电图标=需按键 | 规则可移植，reason 值每机一份 |
| 永不写 PSHOLD | `wr32(PSHOLD,0)`=**断电**非复位，落入 charge 模式 | PSHOLD 地址每机不同（lilac `0x10ac000` / sagit `0x0c264000`）；规则：死路只能写 marker + wfe |
| 硬件 WDT 兜底 | APSS msm-watchdog @`0x17817000`，bark/bite 秒数可调 | 新平台 WDT 可能在 TZ secure 域（EL1 写不动），需 SMC 调用或靠 UEFI/TZ 托管——**先探测 EN 位是否可写** |
| 内核自复位 | timer IRQ 起算 deadline（默认 90s，挂根后延 15min），panic 入口也 arm；到期写路由再 PSCI SYSTEM_RESET | 纯软件，直接可移植。PSCI reset 在 secure-monitor 关闭/无 PSCI 的平台要换方案 |
| XNULDRST 回跳 | payload 里塞 slot，内核异常/return 跳回 shim 收尸 | 可移植（hackios 风格） |
| 临时介质发射 | `fastboot boot` 不落盘，零风险 | UEFI 路线（Y700）：fastboot boot 或 ESP 上的 UEFI app；角色同 shim |

**lilac 实测盲区**：PSHOLD 写=断电曾造成多次"手机消失"；USB 中途断开+fastboot boot 半成品=断电；已知两条都是"最坏结局"，所有路径现在都在防。

### L1 遥测通道（通道 × 存活级别矩阵）

| 通道 | 同 boot | 暖复位 | 冷复位 | 断电 | lilac 用法 |
|---|---|---|---|---|---|
| PON SPMI marker (`0x88e`) | ✅ | ✅ | ✅ | ✅* | 单字节步骤/异常码；Android `dd` 读 + shim 屏显 prev |
| PON route (`0x88f`) | — | ✅ | ✅ | ✅* | 决定复位后落点（=恢复机制的载体） |
| IMEM restart_reason | ✅ | ✅ | ❌ | ❌ | 32-bit marker；ABL 回显进下次 cmdline `warmboot=` |
| DDR scratch cells | ✅ | ✅(实测!) | ❌ | ❌ | 0xA300xxxx 五区：kernel exc dump/stage marks/shim fault/MMU 测试/panic cell |
| Kernel panic log (DDR 顶区) | ✅ | ✅ | ❌ | ❌ | mkadt 在 dram 顶切 1MB，`panic` 字串可被 shim 扫描 |
| fbcon 屏显 | ✅ | — | — | — | 无相机时只能拍；DroidCam 已自动化 |
| ramoops/pstore | ✅ | ✅ | ❌ | ❌ | Android 起来后 `/sys/fs/pstore` 可读 256KB console |
| UFS 指定块 (appslog) | ✅ | ✅ | ✅ | ✅ | 16MiB 预留区；lilac 写路径未根因（shim 写不回读） |
| UFS 尾区日志 | ✅ | ✅ | ✅ | ✅ | UniXNU 的 `sheng-log` 模式：userdata 尾 32MB + Linux log-reader 镜像 |

*PON 寄存器在 PMIC always-on 域，**断电后是否消费路由值每机需验证**（lilac 上电仍按组合键；若 Y700 冷启动消费 `0x88f`，"上电自动进 fastboot" 就能做成无人值守）。

**移植第一步永远是：写"DDR retention + PON 存活"探测 beacon**——写 magic → 触发各种复位 → 回来读回，填出本机通道矩阵。

### L2 自动化（基本可移植）

- `fire_harvest.sh` 发射+取证一键流（参数：镜像路径、cell 基址、内核 macho）
- `pon_steps.py` / `ddr_cell.py` 解码器（marker 空间 `0x7766xxxx` 通用约定，直接搬）
- `usbctl.ps1` PnP 重插守护（Windows 侧 VID 过滤，改 VID/PID 即用）
- `watchdog.sh` adb 阶梯升级（kill-server→adbreset→devreset→hardreplug→replug）
- `arm-xnu10.sh` 持久哨兵（sighting→发射→判落点→收证据→自 re-arm）
- `grab_cam.py` DroidCam 截图取证（通用）
- beacons：`SHIM_BEACON=nn` 编译期选终端测试——MMU 表测试、granule 探测、寄存器 dump 都走这个骨架

### L3 打包工具链（基本可移植）

- `mkpayload.py`（kernel+ADT+ramdisk+`--blob` GPU 固件注入 `/chosen/memory-map`）
- `mkadt.py`（ADT 生成器——每机常量表在这）
- `patch_vNNN_*.py` 版本化二进制注入脚本（保留版本史，可复现每一版）
- QEMU 对侧件：同源码地址重映射构建跑 virt 机——**任何没上真机的修复先在 QEMU 过**

## 二、每机一张常量表（移植时填这个）

```c
struct debug_map {
    /* 恢复 */
    uint64_t pshold;          /* 永不写；仅记录用于禁写检查 */
    uint64_t imem_rr;         /* restart_reason cell */
    uint32_t rr_android, rr_fastboot;
    /* PON (SPMI) */
    uint64_t spmi_arb;        /* SPMI arbiter base */
    uint8_t  pon_sid, pon_apid;
    uint16_t pon_telemetry, pon_route;
    uint8_t  route_android, route_fastboot;
    /* WDT */
    uint64_t wdt_base;        /* regs: RST+4 EN+8 STS+0xC BARK+0x10 BITE+0x14 */
    int      wdt_hz;
    /* 取证 */
    uint64_t ddr_cell_base;   /* 避开内核区/固件 carveout */
    uint64_t fb_base;         /* 引导器留的 splash fb */
    uint32_t fb_w, fb_h;
    /* 平台 */
    uint64_t gicd, gicr;
    uint32_t timer_mhz_x10;   /* 192 = 19.2MHz */
};
```

lilac 已填：`{0x10ac000, 0x146bf65c, 0x77665501, 0x77665500, 0x800f000, 4,4, 0x88e,0x88f, 0x04,0x06, 0x17817000, 32765, 0xA3000000, 0x9d400000, 720,1280, 0x17a00000,0x17b00000, 192}`

**Y700/TB322FC 待填**：PM8550 系 PMIC 的 PON 寄存器、WDT 位置/可写性、
DDR retention、fastboot 上限、按键组合（Power+Vol-?）、UEFI 下的替代 harness。

## 三、移植时先做的 5 个探测 beacon（顺序）

1. **存活探测**：写 DDR magic + PON marker + IMEM reason → 软复位/PSCI/WDT/断电各一次 → 读回 → 填通道矩阵
2. **PON 路由验证**：逐个写 candidate reason → 确认落点（Android?fastboot?charge?）→ 建立"永远可回"集合
3. **WDT 可写性**：EN/BARK 写后读回，不行则探测 TZ SMC 接口
4. **granule/CPU 探测**（lilac beacon 93）：`ID_AA64MMFR0` TGran4/16/64 + PARange + MIDR/MPIDR → 决定页大小和 Gold/Silver 策略
5. **FB/console**：确认引导器留的 fb 地址和几何 → 上屏 = 最强遥测通道

## 四、lilac 代码审核遗留小问题（给隔壁）

- `wdt_arm(180,185)` 注释仍写 "XNU gets 65s" —— 陈旧，改 180s
- `loader.c` 的 `spmi_write_apid` 局部 extern —— 应挪 shim.h
- PON 地址 `0x8420000` 在 start.S 硬编码 4 处 —— 抽 `#define PON_SPMI`
- DDR cell 基址 `0xA300xxxx` 散落 —— 抽 `debug_map` 常量
- README 状态段过时（commpage → 实际已到 AMFI/v140）

## 五、Y700 差异化路线备注

- **不走 ABL-shim**：UniXNU/UEFI 路线，`shim` 角色 = UEFI app 或 DarwinBoot；
  `edk2-gunyah` 已列 tb322fc 为已测平台，`dodgePkg`(SM8750) Display/UFS/USB 可用
- **替代遥测**：UEFI mass-storage 模式可直接读盘取证（比 lilac 的 Android-dd 链路短）；
  UniXNU `sheng-log` 尾区日志模式可直接用
- **额外通道**：Y700 AVB 用公共 testkey 签名 → 锁 BL 也能跑自签内核（EDL/LTBox 兜底）
- **未知**：PON 断电存活性、WDT 是否在 EL1 可写、fastboot 尺寸上限——第一批 beacon 就测这些


---

## 六、lilac 无人值守工作流终审（2026-10-26）

### 完整闭环（实测跑通的形态）

```
                    ┌─ 相机通道：DroidCam MJPEG → grab_cam.py 截屏 ─┐
                    │   （fbcon/panic 文本视觉取证，手动对焦）      │
[Android adb]       ▼                                              │
 清 PON 基线 ──► [fastboot] ──► 补丁 ABL ──► fastboot boot IMG ──► [XNU]
   ▲                                                               │
   │            ┌─ 正常：90s/185s 内核自复位（PON 0x88f=0x06）◄──────┤
   │            ├─ panic：入口 arm 0x88f → PSCI reset               │
   │            ├─ 异常：XNULDRST 跳回 shim → DDR dump + PON 镜像    │
   │            └─ WDT 兜底：bark 180s / bite 185s                  │
   ▼                                                               │
 [回 fastboot] ──► 重补丁 ──► 读 DDR cells                          │
   （0xA3000000 内核异常 / 0xA1FC884F panic cell /                   │
    0xFB000000 panic log 顶区 / 0x9F700000 PANG）                    │
   ▼                                                               │
 [reboot Android] ──► 读 PON marker ──► pon_steps.py 解码 ──► 写档   │
```

**自动化层**：`fire_harvest.sh`（一轮完整循环）→ `usbctl.ps1`
（VID_0FCE PnP 重插守护）→ `watchdog.sh`（adb 阶梯）→
`arm-xnu10.sh`（持久哨兵，sighting→发射→判落点→收证→自 re-arm）。
**解码层**：`pon_steps.py`、`ddr_cell.py`、`panic_*.py`、
`assert_*.py`、`patch_vNNN_*.py` 版本化注入史。

### 审核结论：三个此前没写进方法论的资产

1. **相机通道是独立遥测层**——DroidCam MJPEG 流是"无串口时代的
   screen console 读回器"。通用化要求：手机亮屏即出 fbcon 文本
   （lilac 已做），帧抓取脚本与设备无关。**写进 L1 矩阵第 9 行**：
   "OCR/视觉通道——同 boot 全存活，无依赖"。
2. **版本化补丁脚本体系** `patch_vNNN_*.py`：每版一次真实修改留下
   审计轨道，可精确复现任一历史镜像——比裸 diff 强的证据链。
3. **"发射→自回→读尸"三段式**（fire_harvest）：关键设计是
   **复位后先回 fastboot 读 DDR（此时 cells 还活着），再 reboot
   Android 读 PON**——顺序错了 DDR 会被 ABL/XBL 清掉。

### UniXNU/UEFI 路线映射（Y700 迁移用）

| lilac（ABL-shim） | UniXNU（UEFI） | 迁移难度 |
|---|---|---|
| shim C+asm（beacon 框架） | UEFI app / DarwinBoot 阶段 | 中——beacon 逻辑搬 C 即可 |
| fastboot boot boot.img | fastboot boot UEFI img / ESP | 零 |
| PON SPMI marker | 同（PMIC 层不变，寄存器表重测） | 低——PM8550 PON 地址待填 |
| DDR cells 0xA3xxxx | 同物理内存，选空闲区即可 | 零 |
| 内核 90s 自复位 | 同（XNU timer IRQ，框架无关） | 零 |
| XNULDRST 回跳 | DarwinBoot 的等价返回约定 | 低 |
| WDT | msm-watchdog→SM8750 可能 TZ 托管 | 待探测（beacon 3） |
| IMEM warmboot 通道 | IMEM 地址不同+ABL 不 echo cmdline | 换 PON/UEFI var |
| fbcon | GOP framebuffer 已给（elden 实证） | **更低**——UEFI 直接送 |
| pstore/ramoops | 同（Android 回读后） | 零 |
| **额外通道** | EDK2 mass-storage / UFS dump、EDL/testkey 兜底 | Y700 独有红利 |

### QcomXnuUsbDebug 在矩阵里的位置

| | 覆盖区间 | 与现有通道关系 |
|---|---|---|
| PON/fbcon/DDR | 内核出生前 ~ 死 | 不动摇——早期唯一取证 |
| **USB CDC（本神器）** | **内核存活期间** | **升级**：实时流日志+双向控制+截图，免相机 |
| pstore/UFS log | 复位后 | 互补——它管活，这些管死 |

接入条件：XNU 能走到驱动 init 的代码点 + DWC3 的 DEVICE-mode 资格
（lilac ABL 已备好，SM8998 DWC3 基址在 stock DTB 的 `usb3` 节点；
SM8750 参考 `soc/Sm8550.h` 模板改表）。**建议时机**：内核能稳定跑到
launchd 之后接入，替掉相机通道做日常调试；PON/fbcon 保留兜底。

### 待补设备常量（Y700/elden）

- PON：PM8550 系 SPMI SID/寄存器（ele: elden 的 feeds/board.toml 可能有）
- DDR 空闲区：UEFI 保留图外选 32MB（elden `dts/elden.dts` 有内存图）
- WDT：探测 EL1 可写性（beacon 3 先行）
- USB：DWC3@SM8750 基址 + SID + IOVA 窗口（填 `soc/Sm8998.h` 式表）
- fastboot：max-download 上限、oem 命令集（elden PHASE0 有记录）


---

## 七、主机侧自动化层（2026-10-26 复审补记——此前漏记）

三层之外还有一层 **L2.5 主机自动化**——这批工具此前散在
`tools/`、`unattend/`、`research/`，统一登记：

### 构建奴隶（Mac 侧，launchd 常驻）

- `tools/mac-autobuild.sh` + `mac-autobuild-install.sh`：
  launchd agent `com.xz1c.xnu-autobuild`，每 60s poll origin/main →
  拉新 commit → 刷新 dxb/patches → 编 `kernel.development.sweet` →
  落 `~/Library/CloudStorage/飞牛同步-Home-Nas/XNU/` + BUILD_STATUS。
  **CI-free 持续集成**；`touch ~/xnu-autobuild.pause` 可暂停。
- **Fn-Sync 通道** = Mac↔Windows 双向传送带（NAS Home 同步，
  ~20s）——内核/物料投递不经过任何公网。
- `tools/xnu-cc.py`：用 CI 日志回放单文件编译（本地验证补丁免整编）
- `tools/validate-patches.sh`：干净树全套补丁 apply 自检
- `tools/check-exports.py`：离线复现 xnu symbolset 检查

### QEMU 预飞（改真机前必经）

- `tools/qemu-sweet.py PAYLOAD OUTDIR [--secs N] [--gdb] [--live-serial]`：
  在 QEMU virt 上重演 loader 的全过程（放段/置 ADT/boot_args/跳入口），
  帧缓冲文本还能解码——**每次改动先 QEMU 过一遍再上真机**，
  是"每轮修复都实机验证"前置的"先虚机验证"。

### 取证扩展

- `tools/sweet-crash.py`：**照片里的 CRASH 报告 → 符号化**
  （配合 grab_cam 相机通道，从屏幕照片还原 bt/pc/lr 到 dyld cache
  符号）——相机通道的解码终端。
- `tools/sweet-log-pull.sh`：经 OrangeFox 读 userdata 尾部 4MiB
  常驻 console ring——pstore 之外的另一条死后续命通道。
- `tools/remotezip.py`：IPSW **单文件拉取**（HTTP range 读 zip 目录，
  不用下整个 6GB 包）——Asset 通道提速关键。
- `tools/sparsediff.py` / `patch-launchd-plist.py` /
  `sweet-root-tweaks.py`：sparse 比对、launchd plist 改写、
  根卷 bring-up 修改（跳 Setup/激活种子/关 watchdog daemon 等）。

### 完整无人值守栈总览（lilac 实测形态）

```
[改代码] → validate-patches/xnu-cc（本地快速验证）
        → QEMU virt 预飞（qemu-sweet.py, gdb/serial）
        → Mac autobuild watcher（launchd 60s poll → Fn-Sync 投递）
        → fire_harvest.sh（清 PON→补丁→fastboot boot→等自回→读尸）
        → 复位后：DDR cells + PON + ramoops + sweet-log-pull
        → 相机通道（grab_cam）+ sweet-crash.py 符号化照片里的 panic
        → usbctl/watchdog/arm-xnu10（USB 抖动自愈 + 持久哨兵）
        → 全程写 AGENTS.md/NOTES.md 证据链
```

**将来 Y700 直接复用**：把 mac-autobuild 的 watcher/pause 语义、
qemu-sweet 的 virt 重演、sweet-crash 的照片符号化、remotezip 的
IPSW 单文件拉取全部照搬——框架无关的主机侧工具链。

### 通道替换关系（2026-10-26 补记）

QcomXnuUsbDebug 若验收通过，**相机通道降级而非退役**：
USB CDC 接管③期全部职责（实时日志流/panic drain/双向控制/截图），
相机保留三个不可替代场景——①②期（USB 未起）的可视化、
面板物理真相校验（上游明示：PNG 只证明注册图像被读出，
不证明面板真实显示）、独立故障域兜底（相机走 WiFi，与 USB
电缆零共享故障面）。无人值守判读由"看屏"升级为"grep 日志特征"。

### 磁盘日志读回通道（2026-10-26 补记，nabu 实机验证）

群友 nabu-xnu-log-readback 在 SM8150/UFS 上实锤了一条我们原计划
但存疑的通路：**内核普通线程 → 磁盘写（WRITE10+FUA → SYNC CACHE）
→ 跨复位存活**。我们 shim 期同操作失败的原因定位：差别在
"写入者是 shim（存储栈未起）还是内核线程（驱动已活）"。

可移植配方（照搬到 appslog/sda58 或 ESP）：
- 编译期给 `serial_putc_crlf` 挂捕获钩 → 4MiB 静态 BSS 环，
  try-lock 追加、竞争即弃（绝不死等可能已停的 CPU）；
- 内核线程做**卷归属证明链**（GPT 主备 CRC → 分区身份 →
  FS 结构 → 预分配文件链连续 → 窗口不重叠）后双槽提交；
- panic 路径**零磁盘 I/O**，已提交槽保留最后态；
- 复位后 Android/Recovery **只读**提取、CRC 校验、解码。

与 USB CDC 的关系：CDC 要内核活着且 USB 可用；磁盘读回管
"内核活着时定期落盘、死了以后读尸体"——两条互补。
lilac 可用 appslog（现成 16MiB 日志分区）当载体。

### 跨项目通用判读律（nabu-usb-cdc 实测定案，2026-10-27）

- **复位源不可由 PON 区分**（WDT 与 PSCI 复位同值）——判"内核跑没
  跑过"用外部见证（COM 口出现/屏幕进过某阶段），别看复位码。
- **WDT 名义值≠实际咬时**：所有 bite 规划前先标定 tick。
- **panic 上下文钩子顺序决定文本能否出境**：drain 必须先于 restart。
- **"没在某通道看到"≠"没执行"**：内存标记不上屏不落盘；
  判死点用"本轮独有字符串"或直连通道。
- **捕获工具读数本身可以是假阴性**（DTR 未置/开启时机晚/超时一次
  即关）；采集器永远先设备一步启动。
- 挂死兜底只信平台侧武装的硬件狗；内核自救钩子默认不可依赖。
