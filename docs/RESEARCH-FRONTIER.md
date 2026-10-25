# RESEARCH-FRONTIER.md — lilac 前沿探索与物资储备

> 本文档记录"不打断主排障线"的并行研究：rootfs 物资、驱动摸底、UniXNU 评估、Y700 调研。
> 主战场状态看 `AGENTS.md`；本文档只写离线可做的准备与结论。
> 规则：不碰 Sony 真机（隔壁在跑），编译尽量走 Mac(192.168.68.59)，不用 GitHub CI。

---

## 1. iPhone X rootfs 物资（✅ 完成 2026-10-25 00:23）

- **IPSW**: `iPhone10,3,iPhone10,6_16.7.16_20H392_Restore.ipsw` = **6,171,035,648 B** ✅
- 解包: `~/Desktop/ios-assets/ipsw-unpack/`（kernelcache.iphone10b + 5 dmg）
- **产物: `~/Desktop/ios-assets/iosroot-16.7.16.img` = 17GB UDRW dmg,
  case-sensitive journaled HFS+, fsck_hfs "appears to be OK" ✅**
  - iPhone X 16.7.16 OS rootfs + SystemOS/AppOS cryptexes +
    dyld cache 硬链 + systembag.kb + logd .migrated，11G用/4.6G余
  - sbin/launchd + usr/libexec/backboardd 实锤存在
- 构建脚本: `~/Desktop/ios-assets/mk.sh`（mkiosroot.sh macOS 移植，
  原生 APFS 挂载，无 apfs-fuse 依赖；本地副本 `research/firmware-extract/mkiosroot-mac.sh`）
- **macOS 挂载血泪教训**（详见 SUBAGENTS.md）：hdiutil 中文输出/
  synthesized盘序乱/nomount+mount_apfs不稳/SPARSE只读/UDRW可写/
  `-owners on`保属主/整脚本root跑。
- 下一步: img→raw hfs+ 写 userdata 分区（由隔壁主线决定时机；
  记得 dmg 是 UDIF 封装，写盘前要剥壳或 hdiutil convert -format UDTO）

## 2. lilac UFS 摸底（已完成初版）

### 2.1 硬件地图（sony-kernel-4.4 `msm8998.dtsi` 实锤）

```
ufshc@0x1da4000  (0x2500)  UFSHCI 主机, IRQ 265, 1 lane
ufsphy@0x1da7000 (0xda8)   QMP v3 PHY, lanes-per-direction=<2>
ufsice@0x1db0000 (0x8000)  ICE 加密引擎（可不用）
时钟: GCC_UFS_AXI_HW_CTL / AGGRE1_UFS_AXI / UFS_AHB / UNIPRO_CORE /
      ICE_CORE / LN_BB_CLK1(ref 19.2MHz) / TX/RX_SYMBOL_0
电源: gdsc_ufs (vdd-hba)
复位: UFS_BCR + pinctrl ufs_dev_reset_{assert,deassert}
PHY 源: drivers/phy/qualcomm/phy-qcom-ufs-qmp-v3.c（sony 4.4 内核里就有）
设备: SKhynix H28U62301AMR
```

### 2.2 关键结论：大概率不需要做 PHY 初始化

ios-sweet 的内核 UFS 驱动（`bsd/dev/sweet_ufs.c`，patch 0028）**复用
bootloader 留下的 link-up 状态**，从不做 PHY/link 初始化——只接管
UTRL（transfer request list）描述符队列发 SCSI。寄存器基址从 ADT
的 `reg` 属性读，**设备无关**。

我们的 boot 链：XBL/ABL 每次启动都从 UFS 读 boot/abl 分区 →
xperable 注入时 UFS link 大概率仍是 up。lilac shim 里已有
`ufs.c`（UFS_BASE 已改 0x1da4000），**主 agent 验证 shim 探针
能不能读到 UFSHCI 寄存器即可一锤定音**。

### 2.3 移植差异点（lilac vs sweet）

| 项 | sweet | lilac |
|---|---|---|
| ufshc | 0x1d84000 | **0x1da4000** |
| PHY | ? (SM7150) | QMP v3 @0x1da7000 |
| lanes | ? | 1（host 配置） |
| ICE | 有 | 有（0x1db0000，先不用） |
| root 分区名 | "userdata" | "userdata" ✓ 同名 |

### 2.4 若 link 不幸断掉的保底方案（已提取）

**PHY 校准序列（sony 4.4 源内完整表格，可直接照抄）：**
- `phy-qcom-ufs-qmp-v3.h:219-281` — `phy_cal_table_rate_A[]` **60 条**
  寄存器写（QSERDES_COM PLL/VCO/lock + RX sigdet/UCDR + TX amp 全套）
- `phy-qcom-ufs-qmp-v3.h:283-302` — `phy_cal_table_2nd_lane[]` **19 条**
  （仅 lanes-per-direction=2 时追加；lilac host 配 1 lane 可省）
- `phy-qcom-ufs-qmp-v3.h:304-306` — `phy_cal_table_rate_B[]` **1 条**
  （VCO_TUNE_MAP=0x44，仅 HS rate-B 时）
- 寄存器偏移宏: `phy-qcom-ufs-qmp-v3.h` 前 215 行 + `phy-qcom-ufs-i.h`
- 调用顺序: `ufs_qcom_phy_qmp_v3_phy_calibrate()`（v3.c:20-41）=
  先 power_on 时钟/稳压 → 写 rate_A 表 → 2 lane 表 → rate_B → `mb()`
- 上层: `ufs-qcom.c` 的 link_startup 流程（serdes start → PCS ready 轮询
  → lane clk 使能）；GCC 门控在 `gcc-msm8998.c` 的 `ufs_*` clk_regmap。

## 3. ADT 常量表（✅ 已完成——隔壁 mkadt.py 已全）

`ios-xperia-xz1c/tools/mkadt.py` 实锤常量（主 agent 已填，验收通过）：

```
PHYS_BASE = 0xA0000000   # 32MB 对齐（0x9F800000 不对齐是上一案真凶）
MEM_SIZE  = 0x5B000000   # →0xFB000000 ≈1.45GB（低段）
HIGH_PHYS_BASE = 0x100000000 / HIGH_MEM_SIZE = 0xFCC00000  # 高段4GB
SOC_BASE  = 0x01000000
GICD/GICR = 0x17A00000(64K) / 0x17B00000(1M)   ← lilac DTB 实值
FB        = 0x9D400000 / 0x2400000, 720×1280 RGB888 stride=2160
UFS       = 0x1DA4000 / 0x3000
VTIMER_INTID = 19        # arch timer PPI
PM8998    = pmic-arb@0x800f000（PON 门铃 0x88e/0x88f）
产物: artifacts/lilac{,-v2,-highmem,-hios}.adt 已生成
```

剩余可补：UART（lilac 无串口输出,fbcon 即控制台）、ufsroot 分区名、
touch 节点（接任务5）、WiFi 节点（接任务4）。

## 4. WiFi/BT/ADSP 固件（✅ 已提取 — 重大修正：是 QCA 不是博通！）

### 4.1 芯片定案

**lilac WiFi = Qualcomm Atheros WCNSS（非 Broadcom！）**
- `/vendor/firmware/wlan/qca_cld/` = qca_cld 驱动框架（Prima/WCNSS 系）
  - `bdwlan.bin` → 符号链接到 modem 分区板级数据
  - `WCNSS_qcom_cfg.ini`（驱动配置）、`wlan_mac.bin`
- **对 iOS 的含义**：苹果没有 QCA WiFi 驱动——跟 HackiOS/topaz
  处境一样，WiFi 只能走社区自制/用户态路径，不能指望 IO80211。
  topaz 的"WiFi 可用"方案要重点研究他们怎么做的。

### 4.2 固件提取战况（`research/firmware-extract/`）

**Sony .sin 解包链（通用流程，已验证）：**
```
xxx.sin = gzip(tar{cms签名 + NNN分片})
分片 = Android sparse image 的同一 fs 的不同增量变体
     （每片 total=393216 块相同，但各片只覆盖部分区间 → 增量包）
```
- `vendor.000` = 覆盖 fs 头部（superblock/GDT/早期元数据，有效）
- 7 个分片 union 覆盖 134662/393216 块(34%)，其余=与基线相同
  的内容，固件里拿不到 → 完整文件需 adb pull 兜底（手机空时做）
- `modem.000` = 单分片 FAT16 完整镜像（modem 分区无增量）→
  **全固件已抽出**

**已到手（`firmware-extract/fw/modem/`）：**
| 文件 | 大小 | 用途 |
|---|---|---|
| `wlanmdsp.mbn` | 3.3MB | QCA WiFi 固件（ELF!）|
| `bdwlan.b30-b35` 等 | 19KB×N | 板级RF数据各SKU变体（要挑 lilac 用哪个）|
| `adsp.b00-b11`+`adsp.mdt` | ~10MB | Hexagon 音频DSP全套 |
| `mba.mbn` | 230KB | modem boot |
| `cpe_9335/9340.*` | ~300KB | ClearPhase 音频 |

**还缺（等手机空闲 adb pull 或解 oem.sin/system.sin）：**
- vendor `/firmware/` 全部真身（union 只拿到 34% 元数据区）
  含 **a540_zap.elf + a540_gpmu.fw2（Adreno 540 GPU 微码！）**、
  cpp 相机固件、tz 应用、ipa_fws
- `bluetooth.sin`（BT 固件，待解）
- `oem.sin`（Sony OEM 配置/校准数据）

> ios-topaz 已证明 XNU 可直接读 stock `modem_a` 分区取 WiFi 固件——
> 我们的 modem 分区同样持有 wlanmdsp.mbn+bdwlan，同路线可行。

## 5. 触摸 Synaptics ClearPad（硬件地图✅，骨架待写）

**lilac 触摸 = Synaptics ClearPad in-cell（RMI4 协议族）**
- `synaptics_clearpad@2c` on `i2c@c179000`（BLSP1 QUP5，QUPv3 SE）
  - SDA/SCL = TLMM GPIO 87/88，IRQ = GPIO 125（falling edge）
  - i2c addr `0x2c`，clk 355kHz，10 指，坐标域 2159×3839→720×1280
  - 固件命名 `touch_module_id_0x%02x.img`（in-cell，固件在 TPIC 内）
  - 驱动栈：sony `clearpad_incell_core.c` + `clearpad_i2c.c`（RMI4）
- **i2c 走 QUP 而非 GPI-DMA**：MSM8998 BLSP QUP 有 FIFO/Block 模式，
  bring-up 期可以**轮询 QUP i2c**（不用 GPI-DMA），比 sheng 简单；
  sheng 的 Synaptics-over-GPI 是 SM8550 必需的（其 QUP 无 FIFO 兜底）。
- RMI4 协议要点（写驱动时按此）：页面选择 PDT 扫描 → F$01 控制/
  F$11 2D 数据/F$34 flash；read F11 data regs → finger abs x/y/z。
- 参考实现: `UniXNU-sheng` 触摸驱动 + topaz FT5452 XNU 端 HID 投递。
- **真值校验注意**：clearpad 驱动里寄存器表是否真的在本机跑通
  未验证（sony 源码即 Android 在用 = 可信），但**别**抄 UniXNU-sheng
  的 RMI 寄存器假设——那是不同 TPIC 世代，先读 PDT 实测再定。
lilac 是 BLSP/i2c-msm-v2 老控制器（非 GPI-DMA），看 sony 4.4
`i2c-msm-v2.c` + clearpad 驱动。

## 6. UniXNU 切换成本评估（结论：暂不切，用户态阶段再评估）

- lilac 已到 commpage/VM 初始化（step 174），切换=放弃 ios-sweet 补丁链
- UniXNU 价值在 NativePrelink（真 Apple kext）+ driver kits +
  iPadOS/macOS 终点 → **建议：到 userland 后再移植，现在拿它当参考**
- MSM8998 先例：nash(Moto Z2F)=UEFI+payload 过硬件，chiron(MIX2)=4K XNU+PID1 心跳

## 7. Y700 Gen4（TB322FC, SM8750/8Elite）调研结论

- **无现成 iOS/iPadOS 移植**；同 SoC 最近：OnePlus 13T(XNU S1-S6)、
  Pad2 Pro(XNU entry)、Ace5 Pro(UEFI+SMP 打点)
- **解锁可行**：zui.com/iunlock 有 Y700 四代专属入口（sn.img），或
  通用 sn.img 改 64 位 SN；避开 ZUXOS 1.1.11.121/120
- **Linux 侧资源丰富**（用户判断正确）：社区已有 SteamOS/Ubuntu/
  ColorOS 案例，Qualcomm SM8750 Linux 源码/DTB 可得 → UEFI 移植
  有参照物
- 定位：**UniXNU 新设备移植**（非抄作业），预计工作量≈sheng 的 60-80%

## 8. research/ 仓库清单 + 成色鉴定（用户要求：别把 WIP/dummy 当成功先例抄）

| 仓库 | 成色 | 能抄什么 | 警告 |
|---|---|---|---|
| ios-sweet | ✅ 真·桌面 | 全部（同源祖本）| 我们是它的 lilac 分支 |
| ios-topaz | ✅ 真·桌面+WiFi | armv8.0补丁(lilac同armv8.0无LSE/PAN！)、TopazModem WiFi 栈、MDP scanout | 其ARMv8.0补丁对我们**比sweet还关键** |
| ios-v60 | 🟡 GUI+触屏实机 | SD865 差异参考 | QCA6390 WiFi 未完成 |
| ios-flame | ✅ 真·桌面+Metal | SD855 路径（最近的全功能参考）| Adreno640≠540 |
| ios-raphael | 🟡 CI镜像+临时启动 | K20Pro SM8550 | 未实锤桌面 |
| ios-sweet-sagit | 🟡 早期(**同SoC MSM8998**) | "GIC distributor编程先于redistributor"(10-23新commit) | 早期但同SoC，持续跟踪 |
| ios-a5xelte | 🟡 QEMU 工具链 | qemu 增强 | 非实机 |
| ios-le-pro3 | 🟡 首次硬件接触 | 严谨方法论 | UFS有ICE坑警告→我们ufsice@0x1db0000 |
| xnu_kernel_sdm835 | 🟡 xnu-11417 | 未来换代备选 | kernel-only |
| UniXNU | ✅ 框架真 | 设计参考(NativeUfs/4K/prelink)，不直接抄 | 设备仓质量参差 |
| UniXNU-sweet | ❌ 远端已删 | — | 本地留档 |
| UniXNU-nash | 🟡 UEFI+payload实机(G1✅ 10-22) | MSM8998 board.toml 全模板 | 在 fcxuhaq3/ 下非org；XNU段需macOS(我们有✅) |
| UniXNU-sheng | ✅ 已clone | NativeNovatekTouch/Ufs/Wifi/GeniSpi kit 源码全在 | org 官方仓 |
| ios-party(chiron) | 🟡 PID1心跳 | SDM835 4K最小启动证明 | 无userland |
| sony-kernel-4.4 | ✅ 官方源 | **真值基准**——寄存器以它为准 | 只作参考 |

**抄码铁律**：sony-kernel-4.4 = 唯一"本机实跑"真值；ios-sweet/topaz
补丁 = 实机验证过；其余仓库一律先当假设，跑通前标记 WIP。

### 8.0 ⚠️ sagit 10-24 关键提交 `a3583ff`（对当前 commpage 调试有直接意义）

- **commpage panic 真凶（sagit 版）**：`pmap_create_commpages()` 随机滑
  commpage text 页到任意 4K 叶子；PAGE_RATIO=4 时 VM 页必须落在
  4 倍数叶子 → `write_pte_fast: PTE write is unaligned`。
  修复：组对齐槽位 + `pmap_enter_options` 加 sub-VM-page 守卫 +
  `pmap_sync_page_attributes_phys` 用 `ptoa(pp)`。
  补丁：`ios-sweet-sagit/xnu/patches/0118-sagit-16k-vm-pages.patch`
- **架构级炸弹**：iPhone X 用户态 CodeDirectory.pageSize=14；
  `cs_validate_codedirectory` 只收 12 或 PAGE_SHIFT → **纯 4K 内核
  exec 不了任何签名二进制**。sagit 解法：PAGE_SHIFT_CONST=14 保持
  user ABI + 4K PTE（16K VM over 4K granule）。UniXNU 对应物：
  `native-4k-signature-hash-cache`。**这是路线级抉择，已通报主线。**
- 另一 commit `1fa98a5`：为 iOS 16 WindowServer 发布真实显示几何/刷新率。

### 8.1 nash/cheeseburger board.toml 关键实机测量值（MSM8998 直接可借）

- FB=`0x9D400000` **与 lilac 相同**（MSM8998 splash 通用基址）
- UFS LUN0=29763584 blocks×4096（nash 实测；lilac 要自己量，
  用 CheeseburgerInfo.efi 同款方法）
- boot.img 布局: kernel@0x10000000 ramdisk@0x12000000 tags@0x18000000
- 目标 IPSW: iPhone10,2 / 16.7.16 / 20H392（**和我们下的 IPSW 同版本!**）
- UEFI: edk2-msm8998 GCC5 链路，G1 已过实机（2026-10-22）
- DarwinBoot 把 GOP framebuffer 经 boot_args->Video 传给内核 →
  kernel console 不需要 legacy display kit

---

*更新记录*
- 10-24 23:3x 初版：IPSW 下载中、UFS 初版完成、Y700/UniXNU 结论落盘
- 10-25 00:2x IPSW 到手、rootfs Mac 构建中、WiFi=**QCA 修正**、固件
  已提取、触摸/ADT 完成、成色鉴定落盘、SUBAGENTS.md 建立

---

## 9. vendor.sin GPU 固件提取（✅ 2026-10-25 完成）

**方法**（解决"稀疏分片=增量变体"难题的关键）：
vendor.sin = gzip → tar → 7 个 `vendor.NNN` Android sparse 分片，
各自是同一 1.6GB ext4 的**增量变体**。解稀疏后发现：
superblock/GDT/inode 表在 `vendor.000`、目录项与文件数据块在
`vendor.005`。跨分片取块即可拼出任意文件——无需完整 union。

提取脚本逻辑（`inode→extent→跨分片取数据块`）：
- inode table: GDT@blk1 → itab→inode(offset 256B)
- extent 头 12B（含 generation），leaf 项 12B：ee_block(4)/ee_len(2)/
  ee_start_hi(2)/ee_start_lo(4)
- 数据块逐分片探测首个非零副本

**`fw-vendor/` 已提取 86 项、82 完整**：
| 文件 | 大小 | 用途 |
|---|---|---|
| `a540_zap.elf/.mdt/.b00-.b02` | 17K/6.7K/... | **GPU TZ 启动握手（无 GMU 的 A5xx 必需）** |
| `a540_gpmu.fw2` | 11.7K | A540 电源序列 |
| `a530_*` 全套 | ~8-19K | A530 参考（同代构） |
| `cpp_firmware_v1_*` ×10 | ~16K | 相机 CPP 固件 |
| `ipa_fws.elf/.mdt/.b00-.b04` | 6.8K+ | IPA 数据加速器固件 |
| `tzfingerprint/tzhdcp/tzwidevine/tzwvcpybuf` | 各 ~8 段 | TEE 应用（4 个 .b03 分块缺失，反正我们用不上） |
| `cxd224x_firmware_ES*` | NFC 控制器固件 | NFC |
| `leia_pfp/pm4_470.fw` | Venus 视频编解码 | 视频 |
| `a225/a300/a330/a420_pfp+pm4` | 老 GPU 参考 | 不用 |

`a540_zap.elf` 经 readelf 验证：**ELF32 QUALCOMM DSP6、
entry 0x5000**——是 Hexagon/zap 真身，不是壳。
`qgpu/venus/slpi` 仅作为字符串出现在库里，vendor 分区
firmware 目录只有上表这些。

## 10. userdata 分区容量核算（✅ GPT 实拆）

`partition-image-LUN0_X-FLASH-ALL-99C3.sin` = gzip→tar→`partitionimage_0.000`
（24K）内嵌完整 GPT：

- sector = 4096B；GPT 头 @0x1000、条目数组 @0x2000、每条 128B、共 66 项
- **userdata = LBA 2056837..7810042 = 5,753,206 块 × 4K
  = 23,565,131,776 B ≈ 21.95 GiB**
- iOS rootfs 卷 16 GiB → **可写入且余量 ~6.4 GB**
- 部署注意：写前用 `hdiutil convert -format UDTO` 把 UDRW dmg
  剥成纯 raw，再 dd 到 userdata（等主线授权）
- 其他可用锚点：`system`=5.12GB、`vendor`=1.61GB、
  `oem`=608MB、`persist`=32MB、`boot`=64MB、`appslog`=16MB

## 11. UniXNU GPU 方案研读（对 A540 的适配成本）

看过 sheng 的完整 GPU 栈（`docs/drivers/native-adreno.md` +
`drivers/NativeAdreno` + `kits/Adreno{Firmware,PM4}` + `ArmSmmu4K`）：

**栈**：Metal/AIR → metal2vulkan → SPIR-V → Turnip → NIR → IR3 →
Darwin KMD → NativeAdreno（GMU HFI/CP ring/SMMU/fence）→ A740 →
NativeMobileDisplay scanout。**实锤 GPU 合成桌面已在 sheng 工作。**

**对 lilac A540 的缺口**：
1. `NativeAdrenoA740HwInit.h`——硬件 init 序列 A740 专用；
2. **A540 没有 GMU**——`AdrenoFirmware` 整套（GMU HFI/上电投票）
   是 A6xx+ 架构；A5xx 走**zap shader TZ 握手 + 直接 CP/RBBM +
   GCC/RPM 投票**（这就是必须挖出 `a540_zap.*` 的原因，已到手）；
3. PM4 包 + Turnip a5xx 后端（Turnip 官方支持 a6xx+，
   a5xx 是 Mesa 后期实验性加入，IR3 编译器跨代通用）；
4. ArmSmmu4K/NativeMetal/metal2vulkan/AirNir 可直接复用。

**结论**：UniXNU 的分层（drivers/kits/patches）是对的抄法，
但 lilac 需要写 "AdrenoPM4-a5xx" + "无 GMU 的 Firmware 替代
（zap+CP 直驱）"。工作量估计 = NativeAdreno 移植里最大的一块。
Y700 Gen4 的 Adreno 830 反而离 A740 栈近得多——
若做 UniXNU 设备，GPU 成本比 lilac 低。

## 12. bdwlan SKU 定位（部分完成）

modem FAT 内有两个板级 RF 数据：`bdwlan.102` 与 `bdwlan.104`
（各 19,152B，diff 1,731B，含频段/功率校准）。另有 `bdwlan.txt`
（193K，该数据的 NV 文本模板）与 `wlan_mac.bin`。
**选型机制**：WCNSS 驱动按 board_id 挑 `bdwlan.<id>`——
XZ1C 有 G8441（国际）与 SO-02K（Docomo）两个 SKU。
离线无法 100% 定版；两文件都已保留在
`research/firmware-extract/fw/modem/image/`，部署时
对照机器 persist/NV 或先两个都喂（错选只影响 RF 校准精度，
不影响链路建立）。

## 13. Mac 侧已剥壳物资（2026-10-25 10:35）

`~/Desktop/ios-assets/` 上 ready-to-use：
- `ramdisk/ramdisk-{a,b}.raw`——IM4P 壳已剥、Fletcher64 校验通过，
  mount 验证含 `sbin/launchd`/`mount_hfs`/`fsck_hfs`（真 iOS userspace）
- `iosroot-16.7.16.cdr`——UDTO 剥壳后的完整 GPT 盘
- `iosroot-hfs.raw`——**纯 HFS 分区数据**（16.1GB），直接 dd 到 userdata
- `kernelcache.release.iphone10b.macho`——IM4P→LZFSE 解出的 49MB
  真 iPhoneX 内核，可用 IDA/binja 对比我们的移植

ramdisk 是 mountroot 阶段的"零写盘路线"关键弹药。

## 14. 仓库新货（2026-10-25 拉取）

**⭐ 重大：ios-sweet-sagit 今天提交 Mi6 触摸 bring-up（同 QUP5@0xc179000）**
- `xnu/patches/0121-sagit-touch-i2c.patch`（1111 行）：MSM8998 QUP
  v2.2.1 polled I2C 全实现 + DSX RMI4 PDT/F01/F12 + 10ms 上报 +
  `sweet_hid_touch` 桥接 + 锁屏合成手势注入（5844038/d02a4d8）
- **我骨架的所有 VERIFY 项已被他们实机解决**：
  - GCC 时钟门：`GCC_BLSP1_AHB_CBCR@0x17004` + `GCC_BLSP1_QUP5_APPS_CBCR@0x21008`
  - 寄存器布局用 i2c-qup.c 版本（MX_OUTPUT_CNT@0x100，不是 i2c-msm-v2 的 0x024）
  - `QUP_IO_MODES = PACK_EN|UNPACK_EN`（bit15|14）
  - `QUP_I2C_CLK_CTL@0x400`：`400K=(14<<16)|28`、`100K=(62<<16)|124`
  - `QUP_I2C_MASTER_CFG=1` 启用 v2 tag + `MAST_GEN` 校验
  - 完整状态机：SW_RESET→RESET→cfg→RUN→CLK→PAUSE→push→RUN→poll
  - write-then-read（070de55）：单 FIFO 流 `START+addrW DATA_WR(不停)
    data... START+addrR DATA_RD_STOP len`
- 骨架已按 sagit 版重写（touch-skeleton 仍是 F11；他们 Mi6 是 F12，
  RMI4 框架相同但数据布局不同——**我们用 F11 因为 Sony clearpad
  dtsi/驱动明确是 F11**）

**hackios-xnu-userland 开源**（c5c92f6，10-25）：
- `ios165/fetch-from-ipsw.py`（IPSW 提取脚本）
- `ios165/mkvarskel.py`（/var skeleton 构建）
- `jb-preinstall.py`（jailbreak 预装）
- `prebuilt/userland`（可用 binaries 参考）

**xnu_kext_mi6**：远程 404（可能被私有/删除），本地保留。

**UniXNU-sheng 新提交**：ADSP 固件启动（2fa7290，从固件分区 boot
DSP+IPC router）、电量计 dual-gauge 修复、thermal probe——
电池/ADSP 子系统成熟度信号。

## 15. 全网新仓普查（2026-10-25 晚，ios-sweet 生态爆发）

ios-sweet 现有 **300+ active forks**，按"设备专属补丁"筛选成色：

### ⭐ 对 lilac（MSM8998）直接有用

| 仓库 | 设备/芯片 | 成色 |
|---|---|---|
| **`ios-poplar`** (wakanameko) | **Xperia XZ1 = lilac 亲兄弟，同 MSM8998/SoMC/FB@0x9d400000** | 89 补丁含 poplar 专属 0082-0086：**0082 实锤 Kryo280=ARMv8.0 无 LSE/PAN/UAO**（sweet 假设 8.1+，poplar 改了 PAN 使用处）、0083 commpage 无 CASA 的 acquire CAS、0084 只用 Gold 簇 MPIDR 0x100-103 作 CPU0-3、0085 CPU 5s tick 诊断线程、0086 GIC redistributor 取证（他们卡在 XNU 早期 hang+GICR 未动——和我们同阶段互查） |
| **`ios-sweet-sagit`** | Mi6 MSM8998 | QUP5 触摸已实机（§14） |
| **`ios-sailfish`** (joranv1, 79★) | Pixel 2016 SD821 | **187 补丁=最多**。Adreno 530 上 Mesa GPU 已跑通+WiFi+真电量+20s到锁屏，2GB RAM——**A530 是 A540 同代，GPU 路线最近参照**。内核侧 A5xx bring-up 序列全在补丁里：0119 GPU power probe→0120 CP test→0121 SMMU identity→0156 VM pad scratch；userspace 侧 `tools/sweetgl/`（mesa-setup.sh+mesa.patch=iOS 交叉构建）、`tools/sweetgpu-ca`（CoreAnimation 直渲）、`tools/gpu-shaders`——**A540 照这个顺序抄** |
| **`Bergamota`** (ry-diffusion, 54★) | Poco F1 SD845 | 161 补丁，**iOS 18.7.8 (iPhone XR userland)**！gpu/ 目录含 eagl（OpenGL ES over Mesa freedreno）、metal。自研 zap-split/adt-gpufw 工具链把 a630_zap 注入 ADT |
| **`ios-fajita`** | OnePlus 6T SD845 | 84=vanilla（早期） |
| **`ios-s9`** (syncmodz) | Galaxy S9 SD845 | 104 补丁，含 GPU study commit（注意：con.c/h 是 Windows 保留名签不出，WSL 里可取） |

### ⭐ 对 Y700 Gen4（SM8750，Adreno 830）直接有用

| 仓库 | 内容 |
|---|---|
| **`hackios-xnu-kernel-sm8750`** (SQMY-dor) | HackiOS→SM8750 port（OnePlus 13 dodge），单提交 c584700，UEFI 路线——Y700 未来路线的底座 |
| **`tb322fc-droidspaces-kernel`** (ll1zt) | **核弹级发现：Y700 Gen4 出厂用公共 AOSP testkey 签 AVB** → 自签内核可在**锁定 BL** 上过验证！+ EDL(9008) LTBox 写入。GKI 内核 6.6.89 构建参数全记录 |
| **`Snapintosh`** (668★) | macOS 27 on Snapdragon X2 Elite 笔记本：EL2 hypervisor + **air2ir3（AIR→ir3 shader 编译器）** + AdrenoMetal.bundle + AdrenoX2 IOGPU kext——GPU Metal 栈的独立实现，A830 参考 |

### 其他真材实料（有设备补丁/日志）

| 仓库 | 设备/芯片 | 状态 |
|---|---|---|
| `ios-apollo` (aTechGeekDev) | Mi 10T SM8250 | 115 补丁，**真机到 iOS Setup，SPI 触摸+按键全工作**（每次启动加载触摸固件） |
| `ios-cas` (binbin323) | Mi 10 Ultra SM8250 | 101 补丁，**真机锁屏**；修过 MobileGestalt local-mac-address 坑（IONetworkController 占位服务） |
| `ios-picasso` (minhtritt1996) | K30 5G SM7250 | 126 补丁，NOTES.md 工程日志+logs/ |
| `ios-qemu` (lgrachov) | QEMU | 111 补丁 |
| `pixelios-kernel` (PixOS) | Pixel 6a Tensor G1 | 106 补丁，锁屏+触摸+8核（**首个非高通**） |
| `ios-topaz-kunzite` | POCO M8 SD6G3 | kunzite 分支，SpringBoard+USB net，GPU WIP |
| `ios-sunfish`/`ios-surya`/`ios-mondrian`/`ios-great-exynos`/`ios-lemonadep` | Pixel4a/POCO X3/K60/Note8/OnePlus9Pro | 84 补丁=**vanilla fork，无实质改动**（早期占位） |

### 生态结论
- ios-sweet 上游 84 补丁是基线；**真实 port 增量 = 设备补丁 - 84**
- 两条 GPU 路线在收敛：**freedreno/Mesa 直出**（sailfish A530 ✅、topaz A610、flame A640、Bergamota A630 进行中）vs **Metal→Vulkan→Turnip**（UniXNU A740、Snapintosh air2ir3）
- 非高通的 Pixel 6a (Tensor) 也能跑——XNU 移植已平台无关化

## 16. 普查第二波：UEFI/UniXNU 生态（2026-10-25 晚）

### ⭐ lilac（MSM8998）UEFI 路线地基已存在

- **`edk2-msm8998`**（edk2-porting/Renegade）：MSM8998 EDK2 UEFI，
  覆盖 7 台同 SoC 设备（sagit/chiron/cheeseburger/dumpling/joan/
  nx563j/mata/ocmdtwl）。**加 lilac = 抄 sagit Device 目录改板级值**
- **`mu-silicium` cheeseburgerPkg**（OnePlus 5，MSM8998）：
  Display✅ Internal Storage✅——**UEFI 在 MSM8998 上已被证明可跑**
- **`ios_sagit`**（MC-XiaoXiao）：另一个独立的"Mi6 跑 iOS"项目
  （与 ios-sweet-sagit 不同源）
- sony 主线内核 `msm8998-sony-xperia-yoshino.dtsi`（sonyxperiadev/kernel，
  475 行三机共享）——比 4.4 stock 更干净的寄存器/GPIO 参照

### ⭐ Y700 Gen4（SM8750）UEFI 三件套齐了

- **`mu-silicium`**：**dodgePkg**（OnePlus 13 SM8750，Display✅ UFS✅
  USB✅ LinuxBoot✅）+ **paganiPkg**（OnePlus 13s，同 SM8750）——
  Y700 的 UEFI port = 抄 dodgePkg 改板级
- **`edk2-gunyah`**（Droid-VM）：crosvm+gunyah UEFI，**已测平台明列
  Y700 Gen4 (tb322fc, SM8750)** 与 SM8850
- **`tb322fc-droidspaces-kernel`**：testkey AVB 漏洞（§15）

### ⭐ UniXNU 新拆分 kit（GPU/驱动工具链，Y700+iOS 通用）

`Adreno740`（原 NativeAdreno 改名，A740 XNU kext）、`AdrenoMetal`
（Metal 前端+metal2vulkan，13★）、`AirNir`（**AIR→NIR→IR3 编译器**）、
`ShengUfs`（UFS 驱动，今日还在更新）、`DarwinBoot`（UEFI bootloader）、
`uefi-sheng`（sheng UEFI fork）——均已 clone。A830(A740 同系) 的
Metal 栈可直接参考；A540 需换 PM4/固件层但 AirNir/IR3 后端通用。

### unixnu.com 挂名的社区 port
`UniXNU-sagit`（dmnakzkahziamnk-a11y，Mi6 MSM8998 的 UniXNU 设备仓）
——仓库目前仅 sheng 模板未落地，跟踪即可。
