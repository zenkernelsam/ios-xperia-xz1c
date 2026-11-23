# PORTING-BIBLE — 黑苹果 iOS 移植熔炉（全生态经验炼化）

> 把 lilac/poplar/sagit/sweet/topaz/sailfish/apollo/cas/pandora/HakiOS/
> UniXNU 系所有已验证经验，按"移植必经阶段"炼成索引。
> 每条目：坑是什么 → 谁趟过 → 代码在哪。配合 `RESEARCH-FRONTIER.md`
> （生态动态）和 `METHOD-DEBUG-PORTABLE.md`（无串口调试方法论）使用。

## 阶段 0：设备情报与常量表

| 事项 | 做法 | 出处 |
|---|---|---|
| SoC/板级常量 | 从 stock DTB/UEFI/live DT 提取所有 MMIO/时钟/中断 | `tools/soc-args.py`（HakiOS，自动出 16 项 soc_*）；lilac 附录A 手工表 |
| 页粒度 | `ID_AA64MMFR0` 探测 TGran4/16/64——决定内核 4K/16K | lilac beacon 93；MSM8998 仅 4K |
| CPU 特性 | LSE/PAN/UAO/PAC 逐项查（armv8.0 vs 8.2 vs 9.x） | poplar 0082-86 实锤 Kryo280=armv8.0 无 LSE |
| 代号体系 | 所有项目按 codename 索引（见 FRONTIER §24 表） | unixnu.com/zh/devices + 本表附录 |

## 阶段 1：启动链

| 事项 | 做法 | 出处 |
|---|---|---|
| 临时发射 | `fastboot boot` 纯 RAM，不落盘 | 全生态统一做法 |
| ABL-shim 链 | shim 接管 ABL 现场→装载 payload→跳内核 | ios-sweet 全套 + lilac shim/ |
| UEFI 链 | EDK2→DarwinBoot→XNU | UniXNU 系 + `uefi-sheng` + elden（Y700 本机已验证两次冷启动） |
| 无刷机改 GPT | UEFI 驱动 RAM-boot 改写主+备 GPT+CRC | UniXNU-picasso PR#1 |
| 双系统菜单 | boot_b 槽位放菜单：音量上倒数选 iOS | data-a-live `bootmgr/` |
| 恢复保证 | 永远路由到 Android/fastboot；充电图标=最坏结局 | lilac PON 0x88f 方案 + METHOD-DEBUG-PORTABLE |

## 阶段 2：XNU 内核层

| 墙 | 解法 | 出处 |
|---|---|---|
| MMU/页表 | bootstrap V=P map + TCR/TTBR0 按 SoC granule | lilac beacons 91-93 现场测试法 |
| commpage | 叶子按 PAGE_RATIO 组对齐；no-LSE 变体 | lilac + poplar 0086（LSE 用户态模拟器 188 行） |
| 平台匹配 | `sony,lilac` personality + SweetCPU 拓扑 | lilac 0085-0087 |
| CPU 初始化 | legacy IOCPU 路径（非 AppleARMSMP）；Gold 核启动 | lilac 0087；`gold_gicr_prepare` GICR 预备 |
| SMP 全开 | 多核 PSCI CPU_ON | topaz 0082 + sweet 0065；data-a-live 8核+调频成品 |
| 时钟/定时器 | 19.2MHz + timer PPI from ADT + GICv3 | sweet 0014-0017 + lilac |
| AMFI | SweetAMFI IOKit 服务 + permissive + `unentitled_ios_sim_launch=1` | flame→lilac v140 移植中 |
| AppleKeyStore | fake wrap/unwrap 应答数据迁移 | sagit 0190f86 + UniXNU `AppleKeyStoreState` |
| panic/异常 | DDR cell 落盘 + PON mirror + 格式化 reason | lilac v135+ |

## 阶段 3：存储与根卷

| 方案 | 适用 | 出处 |
|---|---|---|
| rd=md0 ramdisk | 最先到 userspace 的桥 | lilac 路线A |
| `rd=ufsroot0` + HFSX in userdata | lilac 已验证挂根+launchd | sweet 0028-0034 + lilac 实机 |
| 原生 UFS 驱动 | UniXNU NativeUfs（SMMU+cmd queue） | `ShengUfs` kit |
| GPT 切独立 ios 分区 | 保留 Android 的双系统 | picasso PR#1 + data-a-live |
| rootfs 当 userdata 内文件 | 不改 GPT 不清数据 | ios-fuxi 路线 |
| APFS→HFS+ 转换 | XNU 无 APFS；hdiutil UDRW→UDTO | Mac 侧 ios-assets 管线 |
| decmpfs LZVN 解压 | ditto 复制的 restore 根要逐文件重写 | `research/mkdecomp_root.py` |
| root:root 属主 | 否则 launchd spawn 501 被拒 | lilac QEMU 实测 |

## 阶段 4：显示与 GUI

| 事项 | 解法 | 出处 |
|---|---|---|
| fbcon 早期控制台 | splash fb 直写文字 | lilac fbcon 16/32bpp |
| 呈现几何 | canvas 属性必须 CFData 非 CFNumber；原生分辨率上报 | sagit 1fa98a5 + c574057 |
| 硬件合成（CPU 期） | MDP 直扫 IOSurface，禁 CPU 拷屏 | topaz 0092-0103 |
| CoreAnimation 软渲染 | CA bridge 纹理函数表运行时解析 | apollo 4f92cda + sailfish |
| 壁纸/资源 | PNG 走 boot image 进根卷 | apollo 2f26de0 |

## 阶段 5：输入（触摸/按键）

| 控制器 | 解法 | 出处 |
|---|---|---|
| Synaptics RMI4/I2C | QUP5 状态机+tag 流+write-then-read | sagit 0121 + lilac `touch-skeleton/`（F11） |
| Goodix/SPI | GENI SPI + FIFO/GSI | sweet 0063 + UniXNU `QcomGeniSpi` |
| 合成手势 | 底边带注入+定时器重复（自动过锁屏） | sagit 2c102ee/d02a4d8 |
| PMIC 按键 | SPMI PON 键位→HID consumer | sweet 0060 |

## 阶段 6：GPU

| 路线 | 目标 GPU | 状态 | 出处 |
|---|---|---|---|
| freedreno GLES | A5xx-A6xx | A530 实机、A610、A630 通 | sailfish/topaz/Bergamota；**lilac A540 走这条** |
| Metal→Vulkan→Turnip | A640+、A7xx、A8xx | A640 20-26fps、A840 110fps 桌面 | flame/UniXNU/data-a-live；**Y700 A830 走这条** |
| 内核 GPU 底座 | 共享 | power/CP ring/PM4/SMMU/zap 固件 | UniXNU NativeAdreno 1.1万行 + sweet_gpu.c 骨架 |
| A8XX 差异 | A830/A840 | 寄存器表全录档 | data-a-live GPU_PLAN.md + NativeAdreno-myron a8xx |
| Linux 构建链 | 全 | WSL 可编 Turnip+Metal 插件 | data-a-live `tools/turnip|metal/build_*_linux.sh` |

## 阶段 7：遥测与自动化（见 METHOD-DEBUG-PORTABLE.md 全文）

PON marker/route、DDR cells、fbcon、ramoops、QcomXnuUsbDebug
（内核内 USB CDC 实时通道，待实机验证）、fire_harvest、WDT 兜底、
90s 内核自复位、XNULDRST 回跳。

## 阶段 8：长期件

| 事项 | 状态 | 出处 |
|---|---|---|
| Wi-Fi | QCA6390 在做（v60）；ath12k kit（UniXNU） | ios-v60 + Ath12kWifi |
| ADSP/音频 | SMEM/SMP2P/GLINK/GPR 已通，喇叭在做 | UniXNU-sheng + data-a-live 0226-0234 |
| 电量 | SOCCP 电量计 | data-a-live 0230-0231 |
| 越狱/持久化 | jb-preinstall + var skeleton | hackios-xnu-userland |
| USB 调试 | QcomXnuUsbDebug 移植中（群友） | research/QcomXnuUsbDebug |

## 代号速查（lilac 与 Y700 相关）

| 代号 | 设备 | SoC |
|---|---|---|
| lilac | Sony XZ1 Compact | MSM8998 |
| poplar/maple | XZ1 / XZ Premium | MSM8998 |
| sagit/cheeseburger/nash | 米6/一加5/Moto Z2F | MSM8998 |
| elden | **Lenovo Y700 Gen4** | **SM8750** |
| dada/hummer/erhai/piano/oneplus13t | 米15/Ace5P/Pad2Pro/Pad8P/13T | SM8750 |
| pandora/myron/fairlady | 小米17P/K90PM/一加15T | SM8850 |
| sheng | 小米 Pad6S Pro | SM8550 |
