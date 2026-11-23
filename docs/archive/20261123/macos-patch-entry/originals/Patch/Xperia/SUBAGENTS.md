# SUBAGENTS.md — 前沿研究支线（每次上下文压缩后必读）

> 本文件是**副线任务**的权威记录（主线 Sony boot 在 `AGENTS.md`）。
> 压缩后第一件事：读本文件 + `RESEARCH-FRONTIER.md` 末尾的"当前进度"。

## 用户硬性约束（不可违反）

1. **不碰 Sony 手机**——主线 agent 在用它调试内核。一切只读操作也要克制。
2. **删除一律走回收站**，禁止 `rm` 真删（Mac: `osascript`/`trash`，Win: PowerShell `Microsoft.VisualBasic.FileIO.FileSystem` 或拖到回收站）。
3. **Mac 工作全部收在 `~/Desktop/ios-assets/`**，不许碎片撒到 /tmp、~/Desktop 根等别处。
4. **能放 Mac 编译的都放 Mac**，不用 GitHub CI。
5. **分析别人的 repo 要分清真伪**：代码是跑通过的？还是 WIP/dummy/failure？不要被带歪（用户原话：避免兜大圈落泪）。
6. **进度定时归档**：持续把中间结果写进 `RESEARCH-FRONTIER.md`，压缩上下文前必须落盘。
7. 最终成果要并回 `ios-xperia-xz1c` 仓库（artifacts/脚本/文档）。
8. **中文交流**。

## 当前支线任务清单（goal）— 2026-10-25 00:30 快照

| # | 任务 | 状态 |
|---|------|------|
| 1 | IPSW+rootfs（Mac） | ✅ `iosroot-16.7.16.img` 17GB UDRW fsck OK |
| 2 | UFS 摸底+PHY 提取 | ✅ 完成 |
| 3 | ADT 常量表 | ✅ mkadt.py 已全 |
| 4 | WiFi/BT 固件提取 | ✅ modem FAT+BT 全套；vendor 残量待 adb pull |
| 5 | Synaptics 触摸 | 🟡 硬件地图齐（§5）；骨架代码未写 |
| 6 | 仓库更新+clone | ✅ sheng/nash/iphoneParty 已补；UniXNU 有本地改动未动 |
| 7 | 文档归档 | ✅ RESEARCH-FRONTIER/HANDOVER-FRONTIER/AGENTS.md 已写 |

**下回副线起手指引**：读 HANDOVER-FRONTIER.md「遗留待办」；
压缩后先读本文件。

## 关键已验证事实（别重复验证）

- Sony `.sin` = `gzip(tar{cms + NNN sparse分片})`；分片是同 fs 的**增量变体**，
  每片只覆盖部分区间，union 也凑不全 → 完整文件靠 `modem.000`(FAT完整) 或 adb pull。
- lilac WiFi = **QCA Atheros**（wlanmdsp.mbn+bdwlan 在 modem 分区）→ iOS 无原生驱动。
- `hdiutil attach` 输出**中文进度行**且顺序乱（synthesized 在前 physical 在后）；
  找挂载点 = 抓第3字段是 `/` 开头的行；容器名在 `Apple_APFS Container diskN` 行。
- `hdiutil attach -nomount` + `mount_apfs`/`mount -t hfs` 手动挂**都失败**
  （"Permission denied"/"No such file"）——**靠谱做法 = plain
  `hdiutil attach -owners on -nobrowse` 让它自动挂 /Volumes/xxx**，
  `-owners on` 保留真实 uid/gid（必须，否则 rsync 拿不到属主）。
- **输出镜像必须用 UDRW 扁平 dmg**（`hdiutil create` 不加 -type 默认=UDRW），
  SPARSE 格式挂出来是 read-only；UDRW 挂出来 journaled+可写。
- macOS sudo：`~/.supw` 存了密码（600权限），`sudo -S -k cmd < ~/.supw`；
  整脚本以 root 跑最稳（内部 sudo 自然变 no-op）。
- mk.sh 在 Mac：`~/Desktop/ios-assets/mk.sh`（源:ios-sweet/tools/mkiosroot.sh，
  已修 CRLF/realpath/hdiutil解析/UDRW/owners）。产物 `iosroot-16.7.16.img`(.dmg)。

## 环境

- Mac: `ssh ciscohe@192.168.68.59`（免密已配），macOS 15.8.1。
- IPSW: `~/Desktop/ios-assets/ipsw/iPhoneX_16.7.16_20H392_Restore.ipsw` (6.17GB 已下完)
- 解包: `~/Desktop/ios-assets/ipsw-unpack/` →
  OS=`087-86698-044.dmg`(4.7G) SystemOS=`087-86593-047.dmg`(2.7G) AppOS=`087-86715-047.dmg`(10M)
- 构建脚本: `~/Desktop/ios-assets/mk.sh`（CRLF已修，参照 ios-sweet/tools/mkiosroot.sh）
- 固件仓库: `Patch/Xperia/research/firmware-extract/fw/{modem,bt}/`
- 工作日志: `Patch/Xperia/RESEARCH-FRONTIER.md`

## 2026-10-25 00:55 检查点（压缩后先读本文件！）
- 已完成：userdata=21.95GiB（rootfs 16GiB 可放）；vendor.sin GPU 固件
  86 项提取于 research/firmware-extract/fw-vendor/（a540_zap 已验证）；
  UniXNU GPU 栈研读（A740 专用，A540 需 zap+CP 自研）；
  文档已提交 ios-xperia-xz1c（commit: frontier research docs）。
- bdwlan SKU：102/104 两变体已保留，待机器空闲时对照 persist 定版。
- 触摸骨架仍是设计稿；QUP FIFO/F11 细化是下一个可并行项。
- 待办：rootfs 写盘流程预案（UDTO 剥壳+dd，待主线授权）；
  隔壁主线在 v106+ 调 commpage 后阶段，别碰手机。

## 2026-10-25 01:05 检查点2
- 触摸骨架填实：QUP tag-v2 真实编码（0x81 START/0x82 W/0x83 W+STOP/
  0x85 R/0x87 R+STOP，4B/2B 标签）+ F11 真布局（2bit/指状态打包 +
  5B/指 abs：x=b0<<4|b2.lo、y=b1<<4|b2.hi、wx/wy/z）。gcc -Wall 零警告。
  已提交 ios-xperia-xz1c（commit cc99944）。
- 前次 GPU 固件/vendor/GPT 成果在 commit 42910ce+a7e8e1c。
- 剩余并行候选：rootfs 写盘预案细化；F01 唤醒+F34 序列细化；
  UniXNU 4K 补丁 vs sagit 0118 对照分析（主线可能需要）。

## 2026-10-25 12:30 检查点3
- research 全仓拉新完毕。sagit 触摸补丁是今日最大收获（同 QUP5）。
- 触摸骨架已升级成 sagit 验证版（commit ab21a07）。
- Mac 物资全就绪：ramdisk-a/b.raw、iosroot-hfs.raw（HX 已验）、
  kernelcache macho、iosroot-16.7.16.cdr。
- 主线正在 flash userdata 写 iOS rootfs（隔壁在干）。

## 2026-10-25 16:00 检查点4（repo 普查完成）
- 新增 clone（research/）：ios-poplar、ios-sailfish、Bergamota、ios-cas、
  ios-apollo、ios-s9、ios-picasso、ios-fajita、ios-sunfish、ios-lemonadep、
  ios-surya、ios-mondrian、ios-qemu、ios-great-exynos、pixelios-kernel、
  hackios-xnu-kernel-sm8750、tb322fc-droidspaces-kernel、Snapintosh、
  ios-topaz-kunzite。
- 高价值：ios-poplar（XZ1 亲兄弟）、ios-sailfish（A530 Mesa GPU 已跑通）、
  Bergamota（iOS 18 on SD845）、sm8750 port（Y700 芯片）、tb322fc-droidspaces
  （Y700 testkey AVB 漏洞：锁 BL 也能跑自签内核）、Snapintosh（air2ir3）。
- 84 补丁的 fork = vanilla 无实质改动，勿当真 port。
- ios-s9 的 s9/shim/con.c/h 是 Windows 保留名无法 checkout（git 里在）。
- Y700 Gen4 确认 = SM8750（Adreno 830），非"Gen5 Elite"（那是 Y700 Gen5）。

## 2026-10-25 16:40 检查点5（第二波 clone）
- 新增：mu-silicium（内含 dodge/pagani SM8750 UEFI + cheeseburger
  MSM8998 UEFI）、edk2-msm8998（Renegade MSM8998 UEFI，7设备）、
  edk2-gunyah（明列支持 tb322fc/Y700 Gen4）、ios_sagit（MC-XiaoXiao）、
  UniXNU 新 kit：Adreno740/AdrenoMetal/AirNir/ShengUfs/DarwinBoot。
- 结论：lilac 未来也可走 UEFI（MSM8998 已被 cheeseburger 证明）；
  Y700 Gen4 UEFI 底座 = mu-silicium/dodgePkg。

## 2026-10-26 13:00 检查点6（第三波 pull+clone）
- 主线已上 iOS Hello 屏（GUI 渲染+合成上滑）；隔壁在做 A540 GPU
  （sailfish 路线：PM8005 S1→GPUCC→CP 直启→SMMU→Mesa FD540）。
- poplar 给出 LSE 用户态模拟补丁（0086，MSM8998 必备）。
- 新 clone：UniXNU-dada/erhai/hummer/piano/Pagani_SM8750/myron/
  NativeAdreno-myron(a8xx)/pong/uefi-pong/umi。
- NativeAdreno-myron 的 a8xx 分支 = A840 gen8 寄存器表（Y700 A830 用）。

## 2026-10-26 15:00 检查点7（AGENTS.md 单文件化完成）
- Patch/Xperia/AGENTS.md = 唯一权威（3514 行，含附录A=原仓库版硬事实手册逐字并入）。
- ios-xperia-xz1c/AGENTS.md 已改指路牌；docs/AGENTS.md 已 git rm（git 历史留底）。
- research/*/AGENTS.md 是各上游项目自带日志，勿删勿并。
- 以后写状态只写 Patch/Xperia/AGENTS.md + SUBAGENTS.md；别再往 docs/ 复制 AGENTS.md。

## 2026-10-26 15:30 检查点8（调试方法论已泛化成文）
- 新文档 `METHOD-DEBUG-PORTABLE.md`：L0 恢复/L1 遥测/L2 自动化/L3 打包
  四层模型 + 通道存活矩阵 + 每机常量表 + 移植首批 5 个探测 beacon。
- 审核发现待隔壁修：wdt_arm 注释陈旧、spmi_write_apid 应进头文件、
  PON/DDR 地址散落硬编码、README 状态过时（已 v140/AMFI 阶段）。
- Y700 移植首批任务：PON/DDR 存活探测 → reason 路由表 → WDT 可写性。

## 2026-10-26 16:00 检查点9（QcomXnuUsbDebug 入库评估）
- research/QcomXnuUsbDebug：Qualcomm XNU USB CDC 调试栈（Darwin22/8796
  同版），内核内实时日志+控制+截图，挂根前可用——补 PON/DDR 的活体通道。
- 移植条件：固件 DEVICE 模式 USB（lilac ABL 已满足）+ soc/Sm8998.h
  常量表 + qualify 验证。早期项目未经真机验收，按高潜半成品对待。
- 详情 RESEARCH-FRONTIER §19。

## 2026-10-26 16:30 检查点10（data-a-live = 生态最远 port）
- research/data-a-live：pandora/SM8850/A840，GPU 全合成桌面 110fps，
  8核+ADSP，boot_b 开机菜单双系统，Linux 侧 Turnip/Metal 全构建链。
  Y700 A830 近乎 1:1 参照（同 gen8）；GPU_PLAN.md 是 A8XX 移植圣经。
- research/UniXNU-picasso：UEFI RAM-boot 无刷机改 GPT 切 unixnu 分区。
- 群友在移植 QcomXnuUsbDebug 串口栈。

## 2026-10-26 17:00 检查点11（HakiOS 公开 + soc-args.py）
- hackios-xnu-kernel 重构公开为 HakiOS（新历史，本地 hakios-v2 分支）；
  tools/soc-args.py 从 DTB 自动出全部 soc_* 常量 = 移植常量表现成实现。
- sagit fake AKS + 合成手势；apollo CA bridge 纹理函数表；flame A640
  Metal 实机 20-26fps。

## 2026-10-26 17:30 检查点12（UniXNU kit 爆发）
- 13 个 UniXNU kit 新仓入 research/（GENI SPI/ath12k WiFi/AKS/GOP FB/
  Novatek touch/HID/preflight/uefi-sheng/sm8550-common/virtio/prelink）。
- Y700 UEFI 路线直接受益；lilac 可读 GENI SPI/GOP 参照。

## 2026-10-26 17:50 检查点13（UniXNU 设备总表）
- SM8750 最前=oneplus13t(XNU S1-S6,卡 root device)；无一台挂根成功。
- lilac 的 sweet-UFS root 经验=SM8750 生态缺口知识（我们反而领先）。
- cheeseburger(MSM8998)在 UniXNU 达 XNU entry=我们 SoC 可 UEFI 的证明。

## 2026-10-26 18:00 检查点14（elden=Y700 本机 UniXNU port 已在）
- research/UNIXNU_elden：TB322FC 格式2设备仓，UEFI菜单/GOP/按键/FAT
  已两次实机冷启动；57补丁+4KiB XNU+prelink kernel.macho 已备好；
  目标 iPadOS17.7.11/j120ap；未到 DarwinBoot/XNU 实机=接力点。
- 新 clone：UniXNU-pdx203（Sony SM8250）、fairlady、cheeseburger
  （MSM8998 UEFI 实证）。myron 勘误=K90 Pro Max(SM8850)。

## 2026-10-26 18:30 检查点15（PORTING-BIBLE 熔炉建成）
- 新文档 `PORTING-BIBLE.md`：全生态经验按 9 阶段炼化索引
  （情报→启动链→内核→存储→显示→输入→GPU→遥测→长期件），
  每条"坑→解法→代码出处"。代号速查表在文末。
- 文档三角定位：AGENTS=日志、FRONTIER=动态、BIBLE=方法索引、
  METHOD-DEBUG=无串口子集。

## 2026-10-26 18:45 检查点15b（Y700 计划定案）
- 用户定：周末晚些 fork `jerry-2009/UNIXNU_elden` 直接续冲。
- 接力点 = DarwinBoot 打包 → XNU 实机首启 → root device（他们未到）。
- 可输出给该线的经验：UEFI 训 UFS link→XNU attach 复用、mountroot
  step-marker 仪表化、根卷制备坑、PON/DDR 取证方法论。
- 本周优先：lilac 主线跑稳+优化（触摸/几何/GPU GLES 按既有顺序）。

## 2026-10-26 19:00 检查点16（主机自动化层补记完成）
- METHOD-DEBUG-PORTABLE.md 增"第七节：主机侧自动化层"——
  mac-autobuild watcher、Fn-Sync 通道、qemu-sweet 预飞、
  sweet-crash 照片符号化、sweet-log-pull、remotezip 单文件拉取。
- lilac 完整无人值守栈已画成总览图；Y700 可直接复用框架无关层。

## 2026-10-26 23:00 检查点17（USB CDC 交接主线）
- 新文档 `HANDOVER-USB-CDC.md`：vermeer 薄驱动移植配方已打包
  交大师兄执行。lilac 无 SMMU → vermeer identity() 整段删，
  DMA 直打 PA；boot-arg 选择加入，主线零改动。
- 动机：相机通道不稳，AMFI 期 SweetAMFI（内核 shim）日志
  可实时流。覆盖面实话说清：只管 kernel console，
  ①②期/userspace stdout/panic 传输照旧走老通道。
