# AGENTS.md — Sony SO-02K (Xperia XZ1 Compact / lilac) 工作日志

> 本文件是跨会话交接的**权威状态记录**。接手前先读 `## 当前状态` 和 `## 红线`。
> 每完成一个里程碑更新 `## 当前状态` 并在 `## 变更日志` 追加一条。沟通用简体中文。

---

## >>> 2026-10-26 14:00 【副线】触控/几何接线建议 <<<

**几何**：native=1 卡 "Unbalanced appearance transitions" 很可能
是缺 sagit 的 canvas 几何发布——`c574057` 实锤 canvas-width/height
必须是 **CFData(OSData)**（写 CFNumber 会在 IOMobileFramebufferOpen
崩 backboardd），`1fa98a5` 原生几何上报后 WindowServer 正常。
建议移植这两补丁重试 native=1；真 720x1280 通了则触摸/清晰度/
旋转一起解决。
**触控**：骨架 research/touch-skeleton/lilac_touch_rmi4.c 已是
sagit 验证版，按 F11 走（sony rmi_f11.c 真值）。注意坐标映射目标
=呈现画布尺寸（native 通=720x1280，没通=1125x2436），不是面板
物理坐标 2159x3839。待验：TPIC 供电轨、reset GPIO、IRQ/轮询。

---

## >>> 2026-10-26 13:00 【副线】poplar 送了大礼：LSE 用户态模拟器 <<<

`ios-poplar` 新补丁 `0086-poplar-no-lse.patch`（188 行）：kernel 里
`sweet_emulate_lse()` 用 LDAXR/STLXR 在用户地址上原子模拟 LSE
（LD<op>8种/SWP/CAS/LDAPR；CASP 留 EXC_BAD_INSTRUCTION）。
**MSM8998 Kryo280 跑 iOS userland 的必备件**，直接可抄。
另：Y700 线备齐——`NativeAdreno-myron` a8xx 分支有 A840/gen8
寄存器表（A830 同族）；SM8750 UniXNU port 已有 5 家（Pagani 到 S6）。
详见 RESEARCH-FRONTIER §17。

---

## >>> 2026-10-26 【方法论沉淀：WORKFLOW-EXPLORATION.md】<<<

把本项目的高效探索方式整理成设备无关文档 `WORKFLOW-EXPLORATION.md`：
目标/红线/证据三角、观测阶梯（屏幕→printf→环形日志→整卷 dump→参考树）、
最小变量迭代、data-only 与整卷一致流程、无人值守闭环、离线取证、
参考实现考古、构建/资源预算、记忆交接、反模式表，以及新设备
（Y700）的启动清单。以后换平台先读这份，再读设备日志。

---

## >>> 2026-10-25 17:00 【副线】poplar 实况+sailfish GPU 细节 <<<

ios-poplar 深入读后：他们正卡在 "XNU 早期 hang + GIC redistributor
没被碰"——跟我们曾踩的 IRQ 段同类，补丁 0085/0086 的 tick 诊断+
GICR 取证可直接借。0082 实锤：**Kryo280=ARMv8.0，无 LSE/PAN/UAO**；
0084：只用 Gold 簇 MPIDR 0x100-103。
GPU 参照定型 ios-sailfish：A530 序列=0119 power probe→0120 CP test
→0121 SMMU identity→0156 VM scratch；userspace tools/sweetgl Mesa
交叉构建+sweetgpu-ca。A540 照抄此序。

---

## >>> 2026-10-25 16:00 【副线】ios-poplar 现身：XZ1 亲兄弟已在移植 <<<

`research/ios-poplar`（wakanameko fork）：Xperia XZ1 = **同 MSM8998、
同 SoMC 平台、同 FB@0x9d400000**。已含 poplar 专属补丁 0082-0086：
kryo280 armv8.0（MSM8998 无 LSE，commpage 需要 no-lse 版）、
cpu-driver、cpu-startup-diag、gic-diag。做 CPU 启动/commpage 时
对照 `ios-poplar/xnu/patches/0082*` 与 `tools/mkadt.py`（sony,poplar
值全套）。另：`ios-sailfish`（187 补丁）已让 **Adreno 530 跑 Mesa**
——A540 GPU 路线最近参照。全普查见 RESEARCH-FRONTIER §15。

---

## >>> 2026-10-25 12:30 【副线】sagit 同构触摸驱动已上屏可用 <<<

ios-sweet-sagit 今日提交 Mi6 触摸 bring-up：`0121-sagit-touch-i2c.patch`
（1111 行）——**同 MSM8998 QUP5@0xc179000、v2 tag、polled FIFO**，
实机已跑通（含锁屏合成手势注入）。lilac ClearPad 差异只在 RMI4
function（我们 F11、Mi6 F12）与 GPIO/addr。骨架已同步升级为
sagit 验证版：`research/touch-skeleton/lilac_touch_rmi4.c`，
细节见 `RESEARCH-FRONTIER.md` §14。做触摸时直接抄他们的
qup_msg 状态机，别重新踩坑。

---

## >>> 2026-10-26 08:45【GUI 稳定渲染；朝向误区澄清；合成上滑打通】<<<

- **朝向结论修正**：DroidCam 相机是**横装**的——照片里的"竖排文字"
  其实是正常竖屏。用户也确认"第一次 hello 看着正常"→ **xf=0 正确**；
  xf=4/5/6（转置）只会把画面转/拉坏（xf=4 就是用户看到的"压扁苹果"）。
  以后不再用 xf>=4。
- **v158（xf=0 + sweetswipe=270）**：hello 的**手写体动画逐字母绘制**
  正常（截到单个字母帧），270s 的上滑把动画/界面划掉（进入空白页）。
  之前 xf=6 轮的同款上滑也成功越过了 hello。HID 注入 + 触摸映射已验证。
- **native=1 不可用**：上报 720x1280 会让 SpringBoard 卡在
  appearance transition（"Unbalanced calls to begin/end appearance
  transitions"），不再 present。继续 1125x2436。
- 现场日志还显示 backboardd 创建 1125x2436/bpr4500 surface、调用
  sel 79/55（未实现，返回 0 也工作）。
- **阻塞**：DroidCam `frame.jpg` 多次长时间僵死（af/ping 正常），
  需要有人在那台相机设备上把 app 划掉重开；否则无法继续判读 UI。
- 当前手机停在 fastboot；卷健康。下一步（相机恢复后）：
  1) xf=0、无上滑跑一轮，确认 hello 完整帧与朝向；
  2) 上滑时间后移（~8 分钟）再划过 hello，观察设置下一屏；
  3) 接 sagit 触摸（QUP5/F11）与 GPU/Metal（A540 抄 sailfish）。

---

## >>> 2026-10-26 02:00【目标达成：iOS GUI 已上屏；朝向实验待续】<<<

- **目标（等根→给根→GUI/WindowServer）已达成**：
  - UFS 根挂载 ✔（整卷 fsck 修复后回刷，全卷 md5 与镜像一致）
  - 迁移跳过 ✔（data-only：`SyntheticDidUpgradX` + `isMigrationNeeded→NO`
    桩，位于 com.apple.datamigrator 偏移 0x59A8）
  - launchd 全链 ✔ / **SpringBoard exec ✔**
  - **WindowServer + CoreAnimation 渲染真实 UI 并上屏 ✔**
    （深蓝/浅蓝渐变背景 + 白色苹果；随后是 iOS "hello" 引导画面）
  - 日志里同时可见 PhotosReliveWidget/GCCWidgets 等扩展因无 GPU 崩溃
    （旁支，不影响本次目标）。
- **朝向**：画面被旋转 90°+镜像。v154 增加 `sweetfb_xf=0..7`（整幅
  重采样；4..7=转置+翻转）。**xf=4 实测**：CA 首帧变成"渐变底+苹果"，
  说明变换作用在 CA 图层；竖屏视角苹果仍 90° 偏转（叶朝左）。
  已预打包 `shim-4k-v155-xf{5,6,7}-360-gzdtb.img`（sweetar=360 秒一轮）
  待逐一试向；若 4..7 都不对，再试 1/2/3（非转置翻转）或实现 IOMFB
  旋转 selector。
- **v156（已编好）**：`sweettap=<秒>`（可选 `sweettapx/y`）——到点用
  现有 `SweetHIDTouch::touch()` 自动点一下屏幕（按下+90ms 抬起），
  用来"点过" hello/Setup 界面；另有既有 `sweethidtest=<秒>`（周期按
  Home/Menu 键）。产物 `k0288-4k-v156.macho`。
- **当前设备状态**：停在 fastboot（安全）；卷干净、可随时再发。
- **阻塞项**：DroidCam（192.168.68.57:4747）`/cam/1/frame.jpg`
  僵死（af/ping 正常）——需要人把那台相机设备的 app 划掉重开；
  否则无法判读朝向实验。
- **今晚新工具**：`sweetar` 已真正生效（v155，替换了整段 SWEET 块）；
  离线取证链（TWRP 整卷 dump + 7-Zip 浏览 HFSX + fsck_hfs + 4KiB
  块级 diff 回写/回滚）全部脚本化，注意**绝不允许**把旧 dump 的元数据
  块回写到已运行过的卷。

---

## >>> 2026-10-26 00:15【GUI 到达：SpringBoard/Setup 的 hello 界面已上屏（旋转待修）】<<<

- **真机实锤**：迁移跳过 + 卷修复后，SpringBoard 已 exec 并运行，
  WindowServer/CA **渲染出 iOS "hello" 引导画面**（深蓝渐变底 +
  白色手写体），说明整条 UI 链路（launchd→backboardd→CA→IOSurface→
  内核拷贝→面板）全部打通。日志中还能看到 PhotosReliveWidget/
  GCCWidgets 等扩展因无 GPU 崩溃（"Too many corpses"），属旁支。
- **遗留**：画面被旋转 90° 且镜像（面板扫描朝向与 iOS 逻辑方向
  不一致；CoreDisplay 的引导 logo 是预旋转过的所以一直正确）。
  解决方向：(a) 找出 CA/iomfb 查询 rotation 的 selector 并返回正确值；
  (b) 在我们的拷贝路径里做 CPU 旋转/镜像。
- **v154（已编入）**：给拷贝路径加 `sweetfb_xf=0..7` 采样变换
  （1/2/3 翻转；4..7 转置+翻转，整幅缩放铺满面板），用于快速试朝向。
  注意 `sweetar=` boot-arg 实际没生效（源码里仍是 `extend(2700)`，
  之前两次替换都没命中），所以轮次仍是 45 分钟。
- **经验**：hello 画面在启动约 **30 分钟**后才出现（大量守护进程
  超时/崩循环之后），10 分钟窗口看不到 UI。
- 下一步：跑 `sweetfb_xf=4`（等当前轮结束后），先用引导 logo/黑帧
  快速判断变换是否作用于 CA 图层；若 2 分钟内 logo 被旋转则说明
  走的是拷贝路径，可持续用短周期试 8 种组合。

---

## >>> 2026-10-25 23:44【SpringBoard 已在运行！迁移已跳过，AKS 状态轮询中】<<<

- **整卷回刷完成**：fsck 修复镜像（16,840,130,560B）刷入后整卷 md5
  **完全一致**（`1ef2298671178329bf3407774e81edfe`）；确认 HFS 挂载
  恢复正常。
- **迁移跳过 = 双保险，全部 data-only**：
  1) `com.apple.migration.plist` 键名
     `SyntheticDidUpgrade`→`SyntheticDidUpgradX`（卷偏移 0x318018000+0x3D）；
  2) `com.apple.datamigrator` 里
     `-[DMMigrationState isMigrationNeededWithBuildVersion:lastBuildVersion:clientPID:]`
     （文件偏移 **0x59A8**，卷偏移 0x14FA21000+0x59A8，块 1,374,758）
     改成 `mov w0,#0; ret` → 永远"无需迁移"。
- **结果（v152b，控制台可见）**：根挂载正常、launchd 全链跑过，
  CommCenter 崩溃循环（sig11，已知），**SpringBoard 已 exec**：
  屏幕连续出现 `SWEET AKS: SpringBoard sel 17 arg0 0 in 1/0 out 0/0
  -> 0x0`（kAKSGetDeviceState，我们的桩返回 `ss=0x4, sls=0`
  即 unlocked），并有 `duetexpertd`。**没有 SpringBoard 崩溃**。
- **v153**：默认自复位窗口 600s（`sweetar` boot-arg 仍未生效，原因
  待查；先改默认值），便于 10 分钟一轮迭代。
- **下一步**：跑 v153 且带 `sweetfb_noconsole=1` —— 若 SpringBoard
  真的 present，屏幕应出现 UI（哪怕是黑/锁屏）；同时 10 分钟后回
  fastboot 读 exec ring（PA 0xA2023DB8），确认最后 exec 序列。
  若仍无 UI：重点查 SpringBoard 卡在 AKS 轮询/GPU(Metal) 哪一步。

---

## >>> 2026-10-25 23:10【事故与恢复：HFS 元数据块回写把 catalog 弄坏；整卷回刷中】<<<

- **事故链**：用「旧 dump 里算出的 124 个 4KiB 块」去回写已经在运行的
  UFS 卷（iOS 在 22:15–23:00 期间又写过盘），导致 volume header/journal
  与当前数据错配：22:40 起 mountroot 报
  `hfs_swap_HFSPlusBTInternalNode: catalog key #27 invalid length (0)`、
  `Runtime corruption detected on /`、`hfs_mountfs failure 5`，随后
  2 分钟看门狗复位。改回 124 块（state A）后变成
  `cannot mount dirty non-journaled volumes`。
- **修复**：Mac 上对 20:24 的整卷 dump 跑 `hdiutil attach -nomount` +
  `fsck_hfs -y -f /dev/diskN` → **修复成功**（卷头 minor repair），
  与 dump 相比只改 **2 块**（block 0 卷头加 unmounted 位、block 128
  日志头）。但设备上的 catalog 节点 1444 已坏，fsck 结果救不到它 →
  **决定整卷回刷** fsck 修好的镜像（16,840,130,560B）。
- **红线新增（血的教训）**：**绝不要**把「旧 dump 计算出的元数据块」
  回写到已运行过的卷；只允许**同长度、只改文件内容**（data-only）
  的补丁（如二进制入口桩、plist 内同长度键名替换）。
- **迁移跳过新方案（data-only）**：`/private/var/mobile/Library/
  Preferences/com.apple.migration.plist` 里键名
  `SyntheticDidUpgrade` → `SyntheticDidUpgradX`（同长度单字节，
  文件数据在卷内偏移 0x318018000，字节 0x3D）。已打进待刷镜像。
- **v152 内核**：每次 exec 把 `<seq> <comm>\n` 滚入
  `_sweet_exec_ring`（VA 0xfffffff008023db8 → **PA 0xA2023DB8**，
  dump 0xA2023D80/1088B，dc civac 已刷），可用 fastboot xperable
  5 秒读出；另有 boot-arg `sweetar=<秒>` 控制自复位窗口。
  v152 此前 ~2 分钟复位已判定为卷坏导致的 mount 失败，不是 ring 的锅，
  待修好卷后复测。
- 产物：`k0284-4k-v152.macho`、`shim-4k-v152-execring-gzdtb.img`、
  `shim-4k-v152b-console-gzdtb.img`、`shim-4k-v152c-gzdtb.img`。

---

## >>> 2026-10-25 22:15【已写盘：496KiB 迁移跳过补丁，v150 复跑】<<<

- **方案A 结论**：v150 把自复位窗口 900→2700s，迁移挂死到 34 分钟仍无
  SpringBoard（屏幕仍是苹果 logo）→ 不是"慢"，是真卡死。
- **补丁（用户已批准）**：在 Mac 上 `hdiutil attach -readwrite` 挂
  userdata 整卷副本，写：
  - `private/var/mobile/Library/Preferences/com.apple.datamigrator.plist`
    = `{previousBuildVersion:20H392, lastBuildVersion:20H392,
    migratedAfterUpgrade:true}`（owner uid 501，Mac 建文件天然 501）；
  - `.GlobalPreferences.plist` 增加
    `com.apple.datamigrator.previousBuildVersion=20H392`。
  然后按 4KiB 块 diff：**23 段 / 124 块 / 507,904 B**。
- **落盘方式**：TWRP（先 `umount /data`）→ `adb push blocks.bin /tmp/`
  → `dd if=/tmp/blocks.bin of=/dev/block/sda66 bs=4096 skip=<累计>
  seek=<块号> count=<块数> conv=notrunc`（23 条，脚本
  `research/apply_skipmig.sh`）→ 回读 23 段拼回 `rb.bin`，
  **md5 与 blocks.bin 完全一致**（770285104d988decc3d810c14b20dbb5）。
  脚本：`research/prep_skipmig.py`、`research/apply_skipmig.sh`、
  `research/readback_skipmig.sh`、`research/patch_in_image.py`。
- **22:15:45 已复跑 v150**（45 分钟窗口），观察迁移是否被跳过、
  SpringBoard 是否起来。若仍迁移：后备 = 用 `patch_in_image.py` 把
  `00LaunchServicesMigrator` 的 `-[LSDatabaseMigrator performMigration]`
  （IMP=0x3474，文件偏移同值）改成 `mov w0,#1; ret`
  （20 00 80 52 c0 03 5f d6），或把 `com.apple.datamigrator`
  的 LC_MAIN entryoff 0x33dc 桩成 `mov w0,#0; ret`。
- **工具发现**：`xcrun dyld_info -objc <macho>` 能直接列出 ObjC 方法
  与 IMP 地址（比 otool -oV 好用）；`vmaddr==fileoff`（__TEXT）。

---

## >>> 2026-10-25 20:47【离线取证：SpringBoard 未启动 = DataMigrator 的 LS 迁移挂死】<<<

- **新增离线取证通道（只读，强烈推荐）**：`fastboot boot twrp-lilac.img`
  → `adb exec-out dd if=/dev/block/sda66 bs=4M count=4015` 拉 16.8GB
  整卷 dump（verify 过 HFSX）→ Windows 用 **7-Zip 25.01（Type=HFS,
  Method=HFSX）** 直接浏览/解出任意文件；WSL 内核无 hfsplus 模块，
  macOS `hdiutil` 也可挂。**TWRP 下先 `umount /data`，全程只读。**
- **backboardd 旧坑确认**：`CrashReporter/backboardd-*.ips` 25 条
  （consecutiveCrashCount 27）栈顶 `getEDTProperty` →
  `IOMobileFramebufferOpen`，即 v145 的 canvas NSNumber 崩溃；v146+
  不再产生新的 backboardd 崩溃。**v149 的 backboardd 是健康的**。
- **SpringBoard 从未启动**：CrashReporter 里 0 条 SpringBoard；
  日志显示 `DataMigrator` 的 `00LaunchServicesMigrator` 插件
  **挂死 >10 分钟**（`stacks+com.apple.datamigrator-*.ips`：
  "PRECAUTIONARY stackshot - migration might be hung or deadlocked
  ... (10 minutes) (erase)"），所以 boot 一直停在苹果 logo。
- **迁移状态线索**：
  - `/private/var/mobile/Library/Preferences/com.apple.migration.plist`
    = `DMUserDataDisposition = { SyntheticDidUpgrade = { buildVersion =
    20H392 } }`；
  - migrator XPC `_RoleAccount = mobile`；二进制字符串含
    `previousBuildVersion` / `lastBuildVersionPref` /
    `com.apple.datamigrator.previousBuildVersion`、
    "System build version unchanged ... Migration not necessary"。
- **两条路**：(a) v150 把自复位窗口 900→2700s，观察迁移是否只是慢；
  (b) 离线补丁：在镜像里给 mobile 写
  `com.apple.datamigrator.plist { previousBuildVersion = 20H392 }`
  让迁移判定"无需迁移"，然后**按 4KiB 块 diff 只回写改动块**到 UFS
  （需用户同意写盘）。日志 tracev3 已提取到 Mac
  `/tmp/ios.logarchive`（`log show` 报 archive 不完整，待补 Info.plist）。

---

## >>> 2026-10-25 19:53【大里程碑：iOS 画面第一次正确上屏——单个苹果 logo】<<<

- **v148d（`sweetfb_direct=0`）证明 panic 来自 MDP 直扫池**：不开直扫时
  内核不再 ~2min panic，**backboardd 成功合成并显示 Apple 开机 logo**。
  直扫开启时必 panic（`Unaligned kernel data abort`，ESR 0x9600002x；
  v147/v148 横幅已带 `pc/lr/esr/far/x20/x21`，banner 不换行，**诊断行
  必须短**）。待修，但非当前主路。
- **v149 修复"四个苹果"**：拷贝路径原来把 4B/px 图层按 4B 行距
  (2880) 写进面板，而面板按 **RGB888 stride 2160** 扫描 → 斜切+重复。
  改为 **BGRA→BGR 逐像素、面板 stride** 写入（fSurfRowBytes != fRowBytes
  分支），fade 仅在 4Bpp 时做。cmdline 加 `sweetfb_noconsole=1`
  （内核白字不再叠在 UI 上）。结果：**一个居中、干净的苹果 logo**。
- 竖白线伪影（面板 ~58% 处 1px）仍存在，暂时忽略（不影响启动），
  疑似 default-surface/控制台残留列。
- v149 cmdline：`... sweetnoreboot=1 sweetfb_direct=0 sweetfb_noconsole=1`；
  产物 `k0282-4k-v149.macho`、`shim-4k-v149-rgbcopy-gzdtb.img`；
  诊断版 `shim-4k-v149b-console-gzdtb.img`（去掉 noconsole 看 boot log）。
- **下一步**：看 logo 之后的 userspace（SpringBoard/WindowServer 是否
  启动；CA 渲染器在没有 GPU 时走哪条路）。若要有日志就 boot v149b。

---

## >>> 2026-10-25 19:12【v145/v146：显示栈打通，backboardd 真崩已解，撞新墙 Clutch 对齐 panic】<<<

- **v145 根因修复（显示）**：`AppleCLCD::start()` 原来用
  `fRowBytes < fWidth*4` 拒绝启动，但 lilac splash 是 **RGB888**
  (720×1280, stride 2160) → 永远 "no boot framebuffer"。改为
  `>= fWidth*3`，并把 IOSurface 侧按 4B/px 视角发布
  (`fSurfRowBytes = max(2160, 2880)`)。真机日志：
  `SWEET FB: MDP VIG0 src 0x9d400000 stride 2160 ... direct scanout ready`
  + `SWEET FB: AppleCLCD 720x1280 stride 2160 (surface 2880) at
  0x9d400000, default surface 1, presented as 1125x2436`。
  → backboardd/FBS 不再回退 wireless0，能加载 HID 插件并打开
  `SweethIDTouch/Buttons`。
- **v145 新崩（已定位并修）**：backboardd `-[__NSCFNumber length]`
  NSException。原因是我在 `start()` 里把 ADT 已有的
  `canvas-width/height`(4 字节 CFData @ disp0) 用 `setProperty(int)`
  的影子属性遮住了 → iomfb.framework 对 NSNumber 调 `-length`。
  **v146 删除这两行**。
- **v146 现状**：启动 ~2 分钟**确定性 panic**：
  `Unaligned kernel data abort at pc 0xfffffff0072dc2dc, lr ...c294`
  （符号化 = `sched_clutch_thread_highest_remove`，
  `ldr x9,[x21,#0x438]`；banner 显示 saved state INVALID）。
  与 v144（AppleCLCD 未启动）15 分钟稳定对比，**变量 = 显示栈激活**
  （60Hz vsync timer / 默认 surface 以 4B 视角 + WC 缓存映射覆盖
  0x9d400000，与内核 console 的可缓存 early alias **cache 属性别名**，
  ARM 上属 UNPREDICTABLE）。
- **v147 诊断件**：`panic_with_thread_kernel_state` 的 SWEET 分支改为
  横幅先打印 `esr/far/x20/x21/x8/x9/x10/ss`，用于区分
  FAR=IO/帧缓冲（别名→改独立 BGRA 显示 surface）还是普通 RAM
  （sched 状态被踩）。
- 产物：`k0278-4k-v145.macho`、`shim-4k-v145-fb-gzdtb.img`、
  `k0279-4k-v146.macho`、`shim-4k-v146-nocanvas-gzdtb.img`。
  打包流程（已验证与 v144 payload 字节级一致）：Mac build →
  `tools/mkpayload.py KERNEL /tmp/adt-v144.bin OUT --cmdline '<v144 cmdline>'`
  （**ADT 必须用 v144 payload 里提取的 12868B 版**，`/root/xnu9.adt`
  是旧的、缺 qcom,kryo280/embedded-panic-log-size）→
  `research/mkimg_swap_payload.py BASE.img PAYLOAD OUT.img`
  （复用 v144 的 shim kernel blob，只换 ramdisk；注意 v0 header
  的 ramdisk_size 在 **0x10**）。

---

## >>> 2026-10-25 18:10【稳定多分钟 + 抵达显示墙】launchd 全量守护进程，FBS 找不到内建显示 <<<

- **v144（WDT 喂狗 + 全部前序修复）真机稳定运行 >6 分钟**（此前 ~90-120s
  必复位）。复位真凶是 **APSS 硬件看门狗**：start.s 只在入口喂一次，之后
  无人喂；`sweet_reset_to_fastboot`/`reboot_kernel` 的 DDR 标记均为空，
  证明确非内核复位路径。v144 在 `bsd_init` 用 `ml_io_map(0x17817000)`
  映射 WDT，定时器 tick 每拍写 `WDT_RST=1` → 稳定。
- **操作简化**：标准 **`fastboot boot IMG`** 在本机 unlocked ABL 上直接可用
  （不再依赖 xperable 的 test4 通道）；xperable 的 bulk 通道极易僵死，
  USB 僵死可用 `usbtl` 守护的 `replug` 文件命令恢复。
- **launchd 已进入 UI 阶段**：屏幕持续出现 `searchd / cryptexd /
  bluetoothd / fairplayd.H2 / xpcproxy`、`STR Version 16.7.16 (Build
  20H392)`、Lockdown/AKS 交互；随后 backboardd/FBS 抛
  **`NSInternalInconsistencyException: We expect a valid bounds for
  configuration: <FBDisplayConfiguration ... displayName = wireless0;
  bounds = {{0,0},{0,0}}; displayMode = 0x0 0Hz sRGB>`**。
- **结论**：用户态已到 WindowServer/显示栈门口；下一堵墙是
  **内建显示未被 backboardd 识别**（回退 wireless0 0x0）。
  需要让 `SweetFramebuffer` 的 IOMobileFramebuffer 被正确匹配并返回
  有效 display mode/bounds（对标上一代 SweetSurface/IOMFB 工作）。
- 产物：`k0277-4k-v144.macho`、
  `shim-4k-v144-wdtmap-gzdtb.img`；cell PA 0xA1FCC84F。

---

## >>> 2026-10-25 16:20【真机给根后连续突破】launchd 已跑过 tzinit/sysstatuscheck，卡 AMFI <<<

- **刷写**：32 位 fastboot 对 11.6GB sparse 必 `bad_alloc`；改 **TWRP +
  adb root 分块 push/dd**（先 `umount /data`，TWRP 默认把 sda66 ext4 rw
  挂到 /data 会覆盖裸写）。整卷按 **4,110,326×4KiB** 校验完全一致。
- **v138（rd=ufsroot0）**：真机挂根成功，DDR `ROOT id 158`；
  launchd 运行并拉起 boot task `mount-phase-1`，其失败触发
  `POSIX_SPAWN_PANIC_ON_NON_ZERO_EXIT` panic。
- **v139**：SWEET 下 `proc_should_trigger_panic()` 对非 init 返回 false；
  但 launchd 自己会用 userspace panic 上报 boot task 失败。
- **`/sbin/mount` 打桩（入口 `mov w0,#0; ret`）**：mount-phase-1/2
  直接成功；boot task 链继续：fsck ✓ → mount ✓ → tzinit
  （只读根失败）→ 加 **`rootrw=1` + `ufsrw=1`**（`vfs_subr.c` 里 SWEET
  预留的 rootrw 分支）后根可写，tzinit ✓、sysstatuscheck ✓、
  prng_seedctl ✓。
- **当前墙**：`launchd_cache_loader` → `abort: Unable to find AMFI`
  （它要校验 launchd.plist 的 cdhash）。已移植隔壁 Flame 的
  `SweetAMFI.cpp`：注册 `AppleMobileFileIntegrity` IOService，
  用户客户端 selector 6（信任查询）恒返回 success；v140 构建中。
- **注意**：根镜像里已做过的持久补丁（均在 userdata）：
  launchd 的 16 处 `RequireSuccess`→未知键、`/sbin/mount` 入口桩；
  `launchd_cache_loader` 入口已恢复原值。原版镜像仍在
  `%TEMP%\iosroot_hfs.raw`，可随时重刷恢复。

---

## >>> 2026-10-25 15:35【给根达成·真机】UFS 挂上 iOS 根，launchd 已跑起来 <<<

- **刷写路径（重要经验）**：32 位 `fastboot.exe` 对 11.6GB sparse 直接
  `std::bad_alloc`（任何 `-S` 都整文件缓冲）。改走
  **TWRP（`research/twrp-lilac.img`, 39MB）+ adb root**：
  - **必须 `umount /data`**——TWRP 会自动把 sda66 以 ext4 rw 挂到
    `/data`，ext4 回写会覆盖裸写（这是上一代 HFS 损坏嫌疑之一）；
  - 用 `adb push` 1GiB 分块 + `dd of=/dev/block/sda66 bs=1M seek=N
    conv=notrunc` 流式写入，push ~130-150MB/s；
  - **校验口径**：镜像 16,835,895,296B = **4,110,326×4KiB**（不是整
    MiB）；按 1MiB 比对会因尾部多读 40,960B 假报错。用
    `bs=4096 count=4110326` 比对 → **FULL_HASH_MATCH_EXACT**。
- **v138 首启（rd=ufsroot0）真机结果**：
  - ~90s 自回 fastboot；
  - DDR cell：**`rooted: yes (ROOT @ +0x1df, id 158)`**、
    `progress: step 140` → **iOS HFSX 根已从 UFS 挂载成功**；
  - PARG：`mount[3] exited -- no exit reason available -- (signal 0,
    exit status 1)` → **launchd 已运行并拉起 boot task `mount`**；
  - 该 panic 是 launchd 给关键 boot task 设的
    `POSIX_SPAWN_PANIC_ON_NON_ZERO_EXIT`（单卷没有数据分区，
    mount -P 必败）——不是挂根失败。
  - **DC 对齐修复已验证**：STEP/ROOT 记录在 fastboot 路由复位后仍可读。
- **下一步 v139**：SWEET 下 `proc_should_trigger_panic()` 对非 init
  进程直接返回 false，让失败的 boot task 不再带走内核；launchd
  继续跑后续任务/守护进程。

---

## >>> 2026-10-25 15:03【真机里程碑】只读 UFS 验证通过：UFS 初始化成功、进入等根循环 <<<

- `fire_harvest.sh` 实发 v137（`rd=ufsroot0`，只读，**未写任何 UFS**）：
  - 15:01:57 跳转成功（jumped=1）；
  - **PON = 0x5C** = step **156**（进入 mountroot 循环且首次挂载失败）；
  - 15:03:39 自回 fastboot（发射后 ~102s，90s 自复位黑科技生效）；
  - **无 panic** → `rd=ufsroot0` 下 `sweet_ufs_rootdev()` 成功
    （host 接管、GPT 找到 userdata），vfs_mountroot 仅因 Android
    userdata 不是 HFS 而失败；随后 Android 正常启动 = 全程只读。
- **DDR cell dump 为空**（无 STEP/ROOT/PARG）：STEP/ROOT 记录没在
  fastboot 读回。待查二选一：XBL fastboot 路由清 DDR，或记录行的
  dc 地址未对齐（当前写的是 `cell+0x180` 未取整）。v138 把 dc 地址
  按 64B 对齐；**主证据暂以屏幕照片为准**（用户已回）。
- **用户带回的新物资（Mac `~/Desktop/ios-assets/`）**：
  - `ramdisk/ramdisk-a.raw`（104M，Upgrade）与 `b.raw`（102M，Erase，
    推荐）：NXSB+Fletcher64 校验过的真 restore ramdisk，含
    launchd/mount_hfs/fsck_hfs（>64MB，不能直接塞 boot 镜像）；
  - `iosroot-hfs.raw`（16.84G，HX 纯分区，可直写 userdata）、
    `iosroot-16.7.16.cdr`（17G 备用）、
    `kernelcache.release.iphone10b.macho`（49M，arm64e thin，符号对照）；
  - HANDOVER-MOUNTROOT.md 已更新（commit c1c3319）；
  - 情报：shim 已有 `ufs.c`；lilac 还有 `sdhc_2`（SD 卡槽）可作
    备选 root 介质。

---

## >>> 2026-10-25 12:25 v137 最终首发件（挂根成功可在 fastboot 直接读证）<<<

- **产物**：`k0270-4k-v137.macho`（16,602,880B）、
  `shim-4k-v137-ufsroot-ai-gzdtb.img`（17,096,704B，payload 已 cmp，
  cmdline 含 `unentitled_ios_sim_launch=1`）。cell VA
  0xfffffff007fc884f → **PA 0xA1FC884F**。
- **v137 = v136 + DDR 遥测**：STEP（+0x180，最后步骤）与
  **ROOT（+0x190，VFS_ROOT 成功后写、不会被后续步骤覆盖）**。
  QEMU 实测：`rooted: yes (ROOT @ +0x1df, id 158)`。
- **刷完 userdata 后判读（无需 Android/PON）**：
  ```
  NO_ANDROID=1 WAIT_TICKS=110 bash research/fire_harvest.sh \
    "...\shim-4k-v137-ufsroot-ai-gzdtb.img" 0xa1fc8800 \
    "...\k0270-4k-v137.macho"
  ```
  - `rooted: yes` → **挂根成功**（目标“给根”达成）；
  - `progress: step 155/156` → 仍在等根；
  - `panic: PARG ...` → 读 string（已格式化）定位下一步墙。
- **顺序**：按键进 fastboot → 补丁 2/2 →（只读验证可先用 v137 发一次）
  → 用户同意后 `fastboot flash userdata %TEMP%\iosroot_hfs.simg`
  （11.62GB sparse，建议 `-S 512M` 分段）→ 再发 v137 → 15 分钟内看屏/拍照 →
  自回 fastboot → DDR 判读。
- **回滚预案（已确认弹药在位）**：`flash/fw/` 有 newflasher.exe + 全套
  20 个 sin（含 system/userdata/kernel/modem），`unlock/xfl-backup-original.img`
  也在 → userdata 被覆盖后可用 newflasher 整套恢复 Android。
- **卷格式已验证**：Route B 的 HFSX（大小写敏感、4K、日志）在 QEMU 上与
  HFS+ 行为一致。补充：给小型 HFSX 卷 `diskutil enableJournal`（HFSJ、
  journalInfoBlock 有效）后再跑 QEMU——只读挂载、launchd bootstrap 均正常，
  `panic: none`、`rooted: yes`。Route B 正是这种日志卷。
- **fsck/“boot task not a platform binary”墙已预先规避**：
  Route B 镜像里 launchd/fsck 的 CodeDirectory 只有 adhoc flag（无平台位），
  但 `bsd/kern/sweet_amfi.c` 对 **MNT_ROOTFS 上的所有签名二进制**
  强制 `CS_PLATFORM_BINARY`，launchd 的平台二进制检查会通过。
- **QEMU 大 ramdisk 限制（与 Route B 无关）**：1.4GB HFSX ramdisk
  （占 `mem_size` 1.5GB 的绝大部分）会在 step 139 前触发
  `uipc_mbuf.c:1743` 断言（mbuf 池下限=0）。ramdisk 只装小载荷时正常；
  Route B 走 UFS 无此问题。ramdisk 实验请让镜像远小于 `mem_size`。
- **Route B 镜像内容复核（已挂载只读清点）**：16.84GB 卷，已用 ~11GB；
  已包含 `/private/preboot/Cryptexes/OS`（3.2GB）、`/App`（9.5MB）与
  **完整 2.4GB `dyld_shared_cache_arm64`（44 文件，硬链接到 cryptex 内
  同路径）**，属主 root:root、无 decmpfs。此前“缺 dyld cache”的结论是
  WSL 重启后未重挂载导致的误判，**镜像无需再补 cache/cryptex**。
  IPSW 组件对照：`087-86698` = OS 根卷、`087-86593` = SystemOS cryptex、
  `087-86719/86622` = restore ramdisk、`087-86715` = 10MB 小件。
- **UFS 首发前审计（v137 二进制/ADT 实查）**：
  - `xnu12.adt` 内有 `/sweet-ufs`：`compatible=qcom,ufshc`、
    `reg=0x01da4000/0x3000`（lilac 正确值，不是 sweet 的 0x1d84000）；
  - `IOKitBSDInit.cpp` 在 `rd=ufsroot0` 时调 `sweet_ufs_rootdev()`，
    失败会 **panic 带格式化 errno**（DDR 可读、60s 自复位），不会静默等根；
  - 驱动只读保证：`su.writable` 由 `ufsrw=1` 控制，所有写路径
    （含 log/sample 线程）都先判 `writable`；**全文件不触碰
    ufsice@0x1db0000**；GPT 按名字找 `userdata`；对上层暴露
    512B 扇区（底层 4K 块）；host 直接接管 + TUR×3 清 unit attention。
  - 预期首次只读验证：屏/串口出现
    `SWEET UFS: "userdata" ... first block 0x2b8c00 ... read-only`，
    随后进等根循环（STEP 155/156）。
- **QEMU 正确属主复刻（13:39 新证据）**：WSL 的 hfsplus 驱动支持写与
  `chown`（仅非日志卷；`-J` 卷只能只读）。用 `mkfs.hfsplus -s` 建
  HFSX、以 root 解压 restore 树并 `chown -R 0:0` 后跑 QEMU：
  **launchd 不再报 ownership，成功 spawn 守护进程**：
  `com.apple.restored_external[3]`（user/501）service state running、
  `Successfully spawned ... because speculative`；仅剩
  `failed lookup: com.apple.logd/system.notification_center/system.logger`
  （restore 镜像本来就没有这些服务）。无 panic、`ROOT id 158`。
  → **属主是 spawn 守护进程的唯一门槛；Route B 镜像 root:root + 全量
  daemons，真机上 launchd 应能拉起真实服务。**
- **显示画布尺寸核对（勿“修复”）**：ADT `disp0` 的
  `canvas 1125x2436` 与 iPhone X 身份一致是**故意的**；
  `SweetFramebuffer.cpp` 以 iPhone X 面板呈现，并把居中的
  720×1280 区域映射到 lilac 面板（`sweetfb_native=1` 才用真实
  720×1280）。驱动内含 MDP VIG0 直扫路径（`fVig0`/`scanFrom`），
  为后续 WindowServer 预留。

---

## >>> 2026-10-25 11:55 v136 首发件就绪（挂根不再被 90s 复位打断）<<<

- **产物**（均已校验）：
  - `k0269-4k-v136.macho`（16,602,840B）：
    `__start` 0xfffffff00741f488、`_sweet_panic_cell` 0xfffffff007fc884f
    → **PA 0xA1FC884F**、`_sweet_autoreset_extend` 0xfffffff00741e910；
  - `shim-4k-v136-ufsroot-ai-gzdtb.img`（17,096,704B，payload 已
    `cmp` 一致）。
- **cmdline（已烘进镜像）**：
  `serial=3 cpus=1 keepsyms=1 -noprogress -v rd=ufsroot0 unentitled_ios_sim_launch=1`
- **v136 相对 v134**：
  1. `unentitled_ios_sim_launch=1`：放行 iOS 平台二进制（QEMU 实测
     launchd 能 exec；否则 `EXEC_EXIT_REASON_WRONG_PLATFORM`）；
  2. panic 记录**格式化后的**字符串（DDR cell + 3×44 红条）；
  3. panic 60s 后自动 PON 0x88f=0x06 + PSCI 复位回 fastboot；
  4. 挂根成功（step158）后把自复位 deadline 推到 15 分钟，
     launchd 不会被 90s tick 打断；
  5. 真机地址（QEMU remap 残留 0）。
- **用户回来后的顺序**：
  1. 按键进 fastboot → 补丁 2/2；
  2. `fire_harvest.sh` 发 v136（**只读**，先验证 UFS 驱动认 userdata）；
  3. 征得同意后 `fastboot flash userdata %TEMP%\iosroot_hfs.simg`；
  4. 再发 v136 → 期望 PON 0x5B→0x5D(157)→0x5E(158) → launchd →
     屏上 `SWEET: root up, auto-reset deadline -> 15 min`。
- **Route B 镜像已确认可用**：无 decmpfs、盘上 root:root。
- **v136 代码 QEMU 回归通过**（kq11，完整解压 restore 根，100s）：
  挂根 → launchd bootstrap 全程无 panic；**超过旧 90s 期限仍存活**
  → `sweet_autoreset_extend(900)` 生效。回归后 QEMU 地址已再次还原，
  源码残留 0。
- **注意**：`build/xnu.obj/.../kernel.development.sweet` 当前是
  **QEMU 回归版**（源码已还原真机地址，未重编）。手机件以导出的
  `k0269-4k-v136.macho` 为准；下次出手机版前先增量重编一次。
- **HFSX 回归（12:03）**：把同一棵解压树做成 **case-sensitive HFSX
  (HX 0005, 4K)** 再跑 QEMU——挂载、launchd、LaunchDaemons 行为与
  HFS+ 完全一致。Route B 的卷格式（HFSX+日志+4K）在离线侧已全验证。
- **新增 DDR STEP/ROOT 记录（v137）**：`sweet_early_step_late()` 在写 PON
  的同时把 `'STEP' + id` 写到 panic cell 的 +0x180（最后写入者胜，
  通常最后是 140 = bsd_init 已返回）；VFS_ROOT 成功后
  `sweet_early_ddr_rooted()` 再写一个**不会被覆盖**的 `'ROOT' + 158`
  到 +0x190。**刷完 userdata 后 Android 没了、PON 读不到**，但
  fastboot 仍可 `xperable -a 0xa1fc8800 -s 512 -r` 读出：
  - `ROOT` 存在 = 挂根成功（158）；
  - `STEP`=155/156 = 仍在等根。
  panic 的 PON hash 改走 `sweet_early_phase()`（只写 PON），不会
  再把 STEP 覆盖成 hash。`ddr_cell.py` 已改为**扫描** PARG/STEP/ROOT。
  QEMU 已验证：`panic: PARG @ +0x4f ... progress: step 140 ...
  rooted: yes (ROOT @ +0x1df, id 158)`。
- **fire_harvest.sh 新环境变量**：`WAIT_TICKS`（默认 36=6 分钟；
  挂根成功后内核会驻留 15 分钟 → 设 **110**）；`NO_ANDROID=1`
  （闪完 userdata 后不尝试回 Android / 不读 PON）。

---

## >>> 2026-10-25 11:38【里程碑】QEMU 上 HFS 根挂载成功，launchd 已跑起来 <<<

- **QEMU `rd=md0` 实况**（kq9/kq10 + 33MB 最小 HFS 根）：
  `SWEET STEP 155` → `Added memory device md0/rmd0` → `BSD root: md0`
  → **`hfs: mounted IOSE16RD on device b(1,0)`** → `STEP 140` → Swap ON
  → `load_init_program: attempting /sbin/launchd`（dev 版路径 ENOENT 正常跳过）。
- **第一次 panic（v134 原始）**：红条显示
  `unexpected SIGKILL of launchd (CS_KILLED) with reason -- namespace 9 code 0xe`。
  解码：namespace 9 = `OS_REASON_EXEC`，code 0xe(14) =
  **`EXEC_EXIT_REASON_WRONG_PLATFORM`**（kern_exec.c:7410）——macOS 目标的
  XNU 拒绝直接 exec iOS 平台二进制。**不是**内存/映射/4K 问题。
- **修复（零内核改动，DEV 构建）**：cmdline 加
  **`unentitled_ios_sim_launch=1`**（kern_exec.c 里现成 DEV boot-arg）。
  下一次 payload 里必须带上；若将来 RELEASE 构建，则需在 SWEET 下
  硬编码放行该 platform 检查。
- **launchd 真正执行成功**（第二轮 QEMU，同 33MB 最小根）：
  - `Restore environment starting`、`entering ondemand mode`、XPC 日志、
  - `SWEET NET: launchd lo0 add address 127.0.0.1`（内核 IPv4 栈 OK），
  - 各 boot task：`Skipping boot-task`、`boot-mode committed: (null)`。
  - 最终 `pid 1 exited -- exit reason namespace 7 subcode 0x1,
    description unable to resolve 'mobile': 114 - The specified username
    does not exist` → **最小根缺 `/etc/master.passwd`（内容问题）**，
    stage 目录里其实有（`/tmp/rdstage/private/etc/master.passwd` 含 mobile）。
- **panic 诊断升级（已编译进 QEMU 版）**：
  - `debug.c`：panic 前用 `va_copy`+`vsnprintf` 把**格式化后**字符串写进
    DDR cell（原来只存格式串，`%s` 全裸奔）；
  - `pe_serial.c`：panic 红条扩为 **3 行 × 44 字符**（原来 1 行 40 字符），
    照片可直接读 reason。
- **结论**：XNU→HFS 根→exec launchd→用户态 dyld/libSystem/XPC 全链在
  4K 粒度内核上**已打通**；距“等根→给根”只剩根内容完整性。
- **完整 restore 根（91MB stage，QEMU）**：先暴露第二个坑——
  `ditto` 保留 `com.apple.decmpfs`，launchd 是 **type 4 (LZVN) 压缩**，
  内核无解压器 → `errno 45`。XNU 本身不带 LZVN/LZFSE（由
  AppleFSCompression kext 注册）。用 `research/mkdecomp_root.py`
  （VFS 读→新写，天然解压）重建后：**launchd 完整跑起来**，
  开始读 `/System/Library/LaunchDaemons/*.plist`；剩余报错只是
  `bad ownership/permissions`（镜像以普通用户创建，文件属主非 root）。
- **QEMU 二/三阶段截图**：`qfb15.png`（launchd+127.0.0.1）、
  `qfb16.png`（decmpfs err45）、`qfb17.png`（LaunchDaemons bootstrap）。
- **Route B 16.8GB 镜像体检（WSL hfsplus.ko 只读挂载 `%TEMP%\iosroot_hfs.raw`）**：
  - **无 decmpfs 压缩**（launchd/dyld/SpringBoard/UIKit xattr 全空）；
  - **盘上属主正确：root:root**（macOS 显示 ciscohe 只是
    `Owners: Disabled` 的挂载策略，不代表盘上值）。
  - → Route B 不需要内核 LZVN/LZFSE，也不需要修属主，**可直接用**。
- **v135 已打包并验签**：`k0268-4k-v135.macho`（16.6MB）、
  `shim-4k-v135-ufsroot-ai-gzdtb.img`（17,096,704B，ramdisk 与
  payload `cmp` 一致）。`_sweet_panic_cell` VA 仍
  **0xfffffff007fc884f → PA 0xA1FC884F**；`__start` 0xfffffff00741f488。
- **v136（构建中，首发用它）**：v135 + 挂根后把 90s 自复位
  deadline 推到 **15 分钟**（`sleh.c` deadline 提到文件作用域 +
  `sweet_autoreset_extend(900)`，`bsd_init.c` step158 后调用），
  否则 launchd 一跑起来就会被 90s 复位打断。
- **panic 自复位**：panic 里中断关闭、90s tick 不会来；
  `sweet_panic_hold()` 60s 后直接 `sweet_reset_to_fastboot()`
  （PON 0x88f=0x06 + PSCI reset）。

---

## >>> 2026-10-25 11:22 sparse 校验通过：路线B 弹药齐备（等你确认）<<<

- `%TEMP%\iosroot_hfs.simg`：Android sparse，**11.62GB**（16.84GB 中
  5.8GB 零块压成 FILL），41,634 chunks（≤1MB/chunk）、块总数
  4,110,326、文件大小与 chunk 累计精确一致、内容与 raw 对齐。
- 全套弹药：
  1. `shim-4k-v134-ufsread-gzdtb.img`（17MB，`rd=ufsroot0` 只读验证）；
  2. `shim-4k-v134-ramdisk-gzdtb.img`（49MB，**ABL 层高风险，暂缓**）；
  3. `k0267-4k-rdroot.macho`（v134：90s 自复位 + 挂根信标 157/158）；
  4. `%TEMP%\iosroot_hfs.raw`（16.84GB 裸 HFSX）与 `.simg`（11.62GB）。
- **恢复后的执行顺序（写盘前需用户明确同意）**：
  1. 按键进 fastboot → 补丁 2/2 → 发 v134-ufsread → 90s 自回 fastboot
     → 读 DDR/PON，确认 UFS 驱动识别 userdata（只读，不写）；
  2. 用户同意后：`fastboot flash userdata %TEMP%\iosroot_hfs.simg`
     （约 8-15 分钟，只写 userdata 分区）；
  3. boot v134 `rd=ufsroot0` → 预期 PON 0x5D（挂根）/0x5E（VFS_ROOT）
     → launchd exec。
- 红线不变：不碰 `ufsice@0x1db0000`、不写其它分区、不擦 SD。

---

## >>> 2026-10-25 11:19 路线B：sparse 改用本机 Python 生成（9p 太慢）<<<

- 16.8GB raw 已到 Windows；WSL `img2simg` 经 /mnt/c(9p) 读 14 分钟仍无输出
  （CPU 仅 48s，D 状态）→ 已终止。
- 新增 `research/raw2simg.py`：纯 Python 本地生成 Android sparse
  （4KB 块、chunk 上限 1MiB、zero→FILL），避免跨文件系统瓶颈。
- 生成后（`%TEMP%\iosroot_hfs.simg`）即可在手机进入 fastboot 且**用户
  明确同意**后：
  ```
  fastboot getvar unlocked              # 期望 yes
  fastboot flash userdata %TEMP%\iosroot_hfs.simg
  ```
  这是**破坏性写 userdata**，Android 数据将丢失（可牺牲，但必须用户确认）。
- 刷写后：boot v134-ufsread（rd=ufsroot0 只读）→ UFS 驱动识别
  userdata → boot v134 `rd=ufsroot0` 挂 HFSX → PON 0x5D/0x5E → launchd。

---

## >>> 2026-10-25 11:00 路线B 前置就绪：裸 HFSX 根卷已剥出（16.84GB）<<<

- Mac 上 `hdiutil convert -format UDTO iosroot-16.7.16.img` 完成
  （16GB .cdr，GPT：EFI 209.7MB + iOSRoot 16.8GB，分区起始 LBA 409640）。
- **重要偏移**：HFSX 卷实际从 **0xC805000** 开始（分区内又有 20KB
  全零前缀），主卷头在 +1024（`HX 0005`，blockSize 4096、
  totalBlocks 4,110,326、size **16,835,895,296**），副卷头同签名。
  按此剥出 `~/Desktop/ios-assets/iosroot_hfs.raw`，macOS 可挂载为
  **iOSRoot（Case-sensitive HFS+）**，fsck 因 journaled 跳过（只读挂载
  不需重放）。
- 正在把 raw 拉到 Windows（`%TEMP%\iosroot_hfs.raw`），随后
  WSL `img2simg` 生成 sparse 镜像（fastboot 按 chunk 下载，可绕开
  64MB max-download；C: 现有 80GB 空闲）。
- **写入方式（需用户明确同意后才执行）**：
  `fastboot flash userdata iosroot_hfs.simg`（分区号/名字核对
  `by-name/userdata`；写前确认 `unlocked:yes`）。这是**破坏性写
  userdata**，必须用户在场并同意。
- 写盘后验证顺序：boot v134-ufsread（`rd=ufsroot0`，只读）确认
  UFS 驱动识别分区 → 再 boot v134 `rd=ufsroot0` 挂 HFSX 根
  （预期 PON 0x5D/0x5E → launchd exec）。
- 红线不变：不碰 `ufsice@0x1db0000`；不写其他分区；不擦 SD 卡。

---

## >>> 2026-10-25 10:52 事故：49MB 镜像在 ABL 层崩溃 → E1F4（等用户按键）<<<

- 时间线：10:48:28 补丁 2/2 → 发射 v134-ramdisk（**49MB**，
  `download:03049000` 完整、无 Incomplete）→ **第一发 boot 30s 不跳**
  → 10:49:16 第二发报 USB Pipe error、设备消失 → 10:52 实测
  **VID_0FCE&PID_E1F4（充电 UI）**。
- 结论：**崩溃发生在 ABL/LinuxLoader 处理大镜像阶段**，XNU 根本没跑；
  v133 的 90s 自复位（已验证对 XNU 挂起有效）覆盖不到这一层。
- 已知边界：17MB 镜像（plain v130/v133）反复成功；49MB 首跳失败并
  两次操作后 ABL 复位进充电模式；151MB 直接被 `BootImage is Incomplete`
  拒绝。→ 真正可行上限在 17MB 与 49MB 之间（疑与 ABL 的 ramdisk
  装载区/大小检查有关）。
- **恢复**：需人工按键（音量上+电源 ~10s 进 fastboot）。用户外出中。
- 下一步（待手机恢复，且用户在场以便随时救援）：
  1. 从 `research/LinuxLoader-P114-ram.pe` 反汇编找 boot image
     kernel/ramdisk 装载地址与大小检查，确定安全上限；
  2. 按上限裁剪 HFS+ ramdisk（当前最小根内容 11MB，HFS 32MB 可缩到
     ~14MB），或改走路线 B（UFS userdata，boot 镜像保持 17MB）；
  3. 任何 >17MB 的镜像只在用户在场时测试。
- 纪律补充：**大镜像测试属于高风险操作**（可能导致 E1F4、需人工
  恢复），必须用户在场；XNU 内自复位只保护内核层。

---

## >>> 2026-10-25 10:43 【黑科技成立】普通 v133 无人按键 90s 自动回 fastboot <<<

- 实测：10:41:09 发射 v133-plain（17MB，补丁 2/2）→ 无人操作 →
  **10:43:16 `BH905ND69G fastboot`**（PID_0DDE）。
- 机制按设计工作：定时器 tick 起 ~90s → `sweet_pon_arm_fastboot()`
  （0x88f=0x06）+ PSCI SYSTEM_RESET → XBL 路由回 fastboot。
- 从此 unattended 循环不再需要电源键；XNU 挂住也能自己回来。
- 下一步：v134 = v133 + 挂根 PON 信标（157=挂上根、158=VFS_ROOT 成功），
  然后发 49MB 的 `rd=md0` 最小根镜像；复位后回 Android 读 PON 判读。

---

## >>> 2026-10-25 10:36 v133：“黑科技”= XNU 90 秒自动回 fastboot（无需按键）<<<

- 痛点：XNU 挂住（如等根）时 APSS WDT 实测**不咬**（10:10 发射后
  到 10:15 仍停屏），只能人工按电源；用户外出时不可接受。
- **v133 实现（已改，编译中）**：
  1. `sweet_early.c`：PON 写通用化 `early_pon_write_reg(reg,val)`；
     新增 `sweet_pon_arm_fastboot()`（写 **0x88f=0x06**，路由 fastboot）
     与 `sweet_reset_to_fastboot()`（先 arm，再 **SMC PSCI SYSTEM_RESET
     0x84000009**，失败则 wfe 兜底）。
  2. `sleh.c`：定时器中断里设 90s 绝对 deadline（首个 tick 起算），
     到期即 `sweet_reset_to_fastboot()` —— 覆盖 panic、等根、任何
     开中断的挂死。
  3. `debug.c`：panic 入口先 arm 0x88f=0x06（双保险）。
  4. `bsd_init.c`：进 mountroot 循环前 arm 0x88f=0x06。
- 效果：以后每次 `fastboot boot` 后即使内核不自己复位，**~90s 后
  自动回 fastboot**；无需电源键，也不会落进 E1F4 充电图标。
  （ABL 层面的 `fastboot reboot bootloader`/下载僵死仍属另一类问题，
  但 XNU 运行期间从此自恢复。）
- 发射件：v133 内核 + 最小 ramdisk（`rd=md0`）→ 挂上 HFS 则走
  launchd；不挂也会 90s 自回 fastboot。手机当前已在健康 fastboot。
- 用户已外出；本机制保证后续 unattended 循环不再需要按键。

---

## >>> 2026-10-25 10:34 【给根·路线A】APFS→HFS+、64MB 下载上限、v132 就绪 <<<

- 目标转入“等根→给根”。按 `HANDOVER-MOUNTROOT.md` 先走路线 A。
- **IPSW restore ramdisk 实况**：`087-86719-047.dmg`（102M）是
  IMG4 `IM4P/rdsk` 封装，payload 为**未加密 APFS**（NXSB 在 +0x20）。
  `087-86622` 同型；`087-86715` 是 10M 裸 APFS。XNU **无 APFS 驱动**
  → 路线 A 必须转 HFS+。
- **转换流程（已跑通）**：Mac 挂载 APFS ramdisk（89M 内容，含
  `/sbin/launchd`、`/usr/sbin/asr`）→ ditto 到 /tmp/rdstage →
  `hdiutil create -fs HFS+ -format UDRW` → `convert -format UDTO` →
  **剥掉前 20480B 的 CD 引导前缀**（H+ 在 21504）→ 裸 HFS+ 镜像
  （H+ 头在 1024，已验证可挂载）。
- **ABL 硬限制**：fastboot `max-download-size = 64MB`。128M HFS 的
  151MB boot 镜像被截断：`BootImage is Incomplete`，**不跳转、安全**，
  但把 USB fastboot 接口搞僵（随后 `???????????? fastboot`、
  getvar 无响应）。
- **v132（最小根，49MB）已打包**：
  `shim-4k-v132-ramdisk-mini-gzdtb.img`。内容：launchd + dyld +
  递归 dylib 闭包（39 个文件，11MB；libobjc-env 是 weak 可缺）+
  LaunchDaemons；HFS+ 32MB 裸镜像；cmdline 加 **`-v rd=md0`**。
- **恢复**：需手机强制重启进全新 ABL（当前 fastboot USB 已僵）。
  之后：补丁必须 2 条 success 才发射 v132。
- 备注：全量 ramdisk（90M）>64MB 上限，无法直接塞进 boot 镜像；
  后续若要全量根，走路线 B（UFS userdata）或给 shim 加压缩/分段。
  红线不变：不碰 `ufsice@0x1db0000`、不写 UFS、不擦 SD。

---

## >>> 2026-10-25 01:20 【副线】"Waiting for root" 阶段物资已备好 <<<

主线已到 mountroot。副线把"给根"所需物资打包交接：
**新文档 `HANDOVER-MOUNTROOT.md`**（必读）：
- 路线A（先跑）：ramdisk 根，IPSW 里现成 restore ramdisk，零写盘
- 路线B（终点）：UFS userdata 真根——容量21.95GiB已实锤、
  rootfs 16GiB 镜像已在 Mac、QMP v3 PHY 序列在 RESEARCH-FRONTIER §2、
  ufsice@0x1db0000 别碰。
- GPU/WiFi/触摸固件+骨架全就位（fw-vendor/、touch-skeleton/）。

---

## >>> 2026-10-25 10:17 【目标达成·真机验收】PON=0x5B + 屏幕 STEP155/等根 <<<

- **完整实机链条（fire_harvest 日志）**：
  10:09:39 清 PON（读回 00）→ 补丁 2 条 success → **10:10:05 发射
  v130，jumped=1**（USB 断开）→ XNU 跑完整启动 → 10:13/10:15 相机
  两帧显示 **`SWEET STEP 155 (mountroot loop)` +
  `Waiting on <dict … IOMedia … Apple_HFS>`** → 复位回 fastboot
  （10:16:01）→ **Android 上读回 PON = `088e: 5b`**。
- `pon_steps.py 0x5b` → **step 155：bsd_init mountroot loop entered**
  —— 即目标 “waiting for root” 的确定性单字节证明。
- 辅助：DDR cell dump 无 `PARG`（**没有 panic**）；其 last-irq 字段
  在 fastboot 阶段已被覆盖/未落盘，不作为判据（屏幕+PON 已充分）。
- 截图存档：`research/evidence-2026-10-25-waiting-for-root.jpg`。
- 约束复核：全程只有 `fastboot boot`；未执行任何 flash；cmdline 无
  `rd=ufsroot0`/`ufsrw`（未触碰 UFS）；未动 SD 卡。
- 验收清单 1/2/3/4/6 全部满足；第 5 项（复位时刻）由 10:10:05→10:16:01
  的人工复位路径替代（WDT 未咬，用户长按电源复位），不构成失败。
- **结论：目标达成。** 自动化 cron 已撤销，避免重复发射；手机已回到
  Android。

---

## >>> 2026-10-25 10:13 【真机达成】屏幕实锤 waiting for root <<<

- 相机截图（存档 `research/evidence-2026-10-25-waiting-for-root.jpg`，
  DroidCam 10:13:02）显示真机屏幕正在输出：
  - **`SWEET STEP 155 (mountroot loop)`**
  - **`Waiting on <dict ID="0"><key>IOProviderClass</key><string ID="1">
    IOMedia</string><key>Content</key><string ID="2">Apple_HFS</string>
    </dict>`**
  - `Failed to write boot-picker-bringup-reason to NVRAM.`
  - 以及 `SWEET AMFI` / `SWEET Sandbox` / `SWEET HFS registered` /
    `kminit done` / `dlil_init` / `ifnet_attach lo0/gif0 …`
- 即 **bsd_init 已执行、vfs_mountroot 正在等待 HFS 根介质** —— 目标
  “waiting for root” 在**真机**上出现。
- 经过还原：10:09 被中断的后台 fire_harvest 实际继续执行了
  `adb reboot bootloader` → 补丁 → 发射 v130；因此手机此刻跑的是
  **v130 内核**（USB 消失正常：该内核无 USB 栈）。
- 待补证据（等复位回 fastboot/Android）：PON（应 0x5C，首次挂载失败）
  与 DDR cell（应无 PARG；last irq 应为定时器 19）。若 WDT 不咬，
  需人工长按电源复位。
- 验收清单第 1/2/3 项已由屏幕证据覆盖（真机跳转 + STEP 155 + 等根）；
  第 4 项 PON 待读后闭环。

---

## >>> 2026-10-25 05:00 兜底巡检改为每小时（cron 05877d74）<<<

- 30 分钟 cron 每次空跑也消耗回合；改为 **每小时 :07、最多 8 次**。
- 秒级恢复仍由持久 watcher 负责；cron 只防“会话/进程丢失”的兜底。
- 触发后动作不变：fire_harvest.sh v130 → 按验收清单判读 → 写回文档。

---

## >>> 2026-10-25 05:00 完成度审计（目标逐条 → 证据；唯一缺口=实机确认）<<<

| 目标要求 | 证据 | 状态 |
|---|---|---|
| 4K-granule XNU（MSM8998 仅 4K） | TCR 实测 0x22a51a251a、SAGIT 4K 链接布局、PAGE_RATIO=4、QEMU cortex-a53 实跑 | ✅ |
| 从旧死点一路推进 | 历史：SHA256_Init → zalloc IRQ-off → IRQ/VBAR → commpage PTE → 平台匹配 → SweetCPU → mountroot（QEMU） | ✅ |
| 出现 waiting for root（bsd_init/root mount） | QEMU 屏幕：`SWEET STEP 155 (mountroot loop)` + `Waiting on … IOMedia … Apple_HFS` + NVRAM 失败；无 panic | ✅（QEMU） |
| 每轮修复都实机验证 | v114-v127 的修复均有 PON/DDR 实机证据；**本轮 5 项新修复只在 QEMU 验证** | ⚠️ 待实机 |
| 持续更新 AGENTS.md / NOTES.md | 本文件与 NOTES 自 19:45 起持续追加，含产物/脚本/判据 | ✅ |
| 不写 UFS / 不擦 SD / 不破坏刷写 | cmdline 无 rd/ufsrw、sweet_ufs 不初始化（无 rd=ufsroot0）；全程只有 fastboot boot，无 flash | ✅ |

**唯一未满足**：在真机上观察到 `waiting for root`（PON 0x5B/0x5C、
~180-190s 复位、DDR 无新 PARG）。设备当前 E1F4，需人工按键；
v130 首发件与自动化（cron/watcher）均已就绪。
→ 该项满足前，**不得**将 goal 标记完成。

---

## >>> 2026-10-25 04:59 收割脚本兼容“直接回 Android”，防丢 PON <<<

- 修复真实缺口：若 WDT 复位后设备**直接回 Android**（0x88f 路由未生效），
  旧脚本只等 fastboot，会超时并**错过 PON**。
- `fire_harvest.sh` 现在等待 6 分钟内同时识别 fastboot / adb：
  - 回 fastboot：重补丁 → 读 DDR cell → 再 reboot Android 读 PON；
  - 直接回 Android：**先读 PON**（不清零），并提示 DDR 不可用；
  - 超时：明确报错。
  PON 读出后自动调用 `pon_steps.py` 解码（如 `PON 0x5c = step 156 …`）。
- 已 `bash -n` 并在 E1F4 状态下实跑验证（安全 exit=2，无任何写操作）。

---

## >>> 2026-10-25 04:58 k0265 二进制审计：IRQ 取证确在手机镜像内 <<<

- 反汇编 `/tmp/k0265.macho` 的 `_sleh_irq`（0xfffffff00741e910）：
  `adrp x9, 0x7fc8000` + `add x9, x9, #0x84f` → 正是
  `_sweet_panic_cell`（0xfffffff007fc884f），随后 `mrs ICC_IAR1_EL1`；
  说明 v130 手机镜像确实包含 IRQ 记录代码（不是只在 QEMU 测试件里）。
- `_sweet_vtimer_intid`、`_sweet_panic_cell` 符号均在。
- 设备仍 E1F4；其余一切就绪，等人工按键。

---

## >>> 2026-10-25 04:58 真机验收清单（v130，恢复后逐项打勾）<<<

1. [ ] 设备从 E1F4 恢复（adb 或 fastboot 可见）。
2. [ ] `fire_harvest.sh` 打印 `patches=2`（两条 success）后才发射；
       发射后 USB 在 ~5s 内断开（真跳转）。
3. [ ] 复位回 fastboot 后重新补丁，读 DDR cell（PA 0xA1FC884F）：
       magic 非 `PARG`（无 panic）**或** 若有 PARG，记录
       caller/字符串/last irq intid+elr 供定位。
4. [ ] 回 Android 读 PON：最终 **0x5B**（已进 mountroot 循环，目标态）
       或 **0x5C**（首次挂载失败）；若停在 0x4D-0x5A/0x4B，按
       `research/pon_steps.py` 说明死点后再修。
5. [ ] 复位时刻：应在发射后 ~180-190s（WDT 180/185），不是几十秒；
       明显更早 => 新的 panic/异常（用 DDR 证据定位）。
6. [ ] 全程未写 UFS、未擦 SD 卡；只读 boot。

> 判定：第 3/4/5 项都满足即“实机到达 waiting for root”，
> 可将 goal 标记完成。

---

## >>> 2026-10-25 04:57 v130 回归通过：IRQ 记录不打断启动 <<<

- 对 v130（含 `sleh_irq` 记录）重套 QEMU 重映射构建并跑 45s：
  - 屏幕仍为 `SWEET STEP 155 (mountroot loop)` + `Waiting on … IOMedia
    … Apple_HFS`，**无 panic**（cell magic=0）；
  - DDR cell[2]/[3] 实测 **intid=27（QEMU 虚拟定时器）**、
    **elr=0xfffffff007423838（machine_idle+0xf8）** —— 与 qemu.log
    中的定时器中断点一致，说明记录代码在 IRQ 上下文安全。
- 源码已再次还原真机地址（残留检查 0）；**v130 手机镜像仍是已打包
  的那一份**（k0265 / shim-4k-v130-irqrec-gzdtb.img），无需重编。
- 结论：v130 可安全作为首发件；真机上若因 IRQ 路径死亡，DDR cell
  的 last-irq 字段会直接给出 INTID 与 PC。
- 手机仍 E1F4；cron（30 分钟）已指向 v130。

---

## >>> 2026-10-25 04:51 v130 就绪（首发件）：IRQ 取证并入 DDR cell <<<

- **动机**：真机首个 IRQ 路径（GIC/定时器）是 QEMU 唯一没覆盖的关键段；
  一旦恢复后首发射死在这里，下一次发射又要等人工按键。
- v130 = v129 全部修复 + `sleh_irq()` 在分发前把
  **INTID 写入 `sweet_panic_cell[2]`、ELR 写入 `[3]`**（panic 路径只写
  [0]/[1] 和 +0x40 字符串，不冲突）；非定时器 IRQ 会 `dc civac` 落盘。
  复位后在 fastboot 读 cell（PA 0xA1FC884F，dump base 0xA1FC8800）即可
  知道最后的 IRQ 编号与 PC，`research/ddr_cell.py` 已支持打印这两个字段。
- 产物：`k0265-4k-irqrec.macho`、
  `shim-4k-v130-irqrec-gzdtb.img`（真机 ADT）。cell PA 不变。
- **首发改为 v130**：
  ```
  bash research/fire_harvest.sh "C:\Users\Cisco He\Desktop\Git\Patch\Xperia\shim-4k-v130-irqrec-gzdtb.img" 0xa1fc8800 "C:\Users\Cisco He\Desktop\Git\Patch\Xperia\k0265-4k-irqrec.macho"
  ```
  v129 保留为对照。
- 判据不变：PON 0x5B/0x5C + 长时间不复位 + DDR 无新 PARG（若 PARG 存在，
  字符串+last irq 一起判读）。

---

## >>> 2026-10-25 04:46 E1F4 无软件出口实锤；PON 解码器就位 <<<

- 用 PnP 属性确认 E1F4 只暴露 **一个 USB 接口**：
  Class 08 / SubClass 06 / Prot 50（大容量存储，USBSTOR），
  **没有 HID/DFU/fastboot/ADB 接口** → 确认无任何软件恢复通道，
  必须人工按键。
- 新增 `research/pon_steps.py`：把 0x40-0x5C 映射到 v128/v129 的具体
  步骤（含 0x4B=进 bsd_init、0x5B=mountroot 循环、0x5C=首次挂载失败、
  0x80-0xBF=panic 信标、0x52=歧义需看 DDR）。恢复后一条命令即可判读。
- 手机仍 E1F4；watcher + 30 分钟 cron 双保险在运行。

---

## >>> 2026-10-25 04:45 夜间自动化兜底：30 分钟巡检 cron 已挂 <<<

- 除持久 watcher 外，另挂 durable cron（每 30 分钟，最多 10 次）：
  设备一旦从 E1F4 恢复为 adb/fastboot，就自动执行 v129 的
  `fire_harvest.sh` 流程并按 0x5B/0x5C + DDR 判读，结果写回本文件；
  仍 E1F4 则不做任何动作。
- 这保证即使用户在你睡着时按键恢复、或后台 watcher 因会话抖动丢失，
  也会在 ≤30 分钟内自动完成首发射与取证。

---

## >>> 2026-10-25 04:45 QEMU 长跑补证：mountroot 已越过等待、尝试 NVRAM <<<

- 用现有 QEMU 内核（v129 源码）跑 150s：屏幕新增
  **`Failed to write boot-picker-bringup-reason to NVRAM.`**
  （HFS mountroot 找不到介质后的既有路径），全程 **无 panic**，
  panic cell 全 0，CPU 最终 idle；`SWEET STEP 155` 与
  `Waiting on … IOMedia … Apple_HFS` 均已在屏。
- 结合 QEMU 的 qemu.log（仅 timer IRQ + machine_idle 的 0x96000046
  噪声、可正常返回），离线证据链已完整覆盖：
  **bsd_init → mountroot 等待/失败 → 等根循环**。
- 真机侧只需 PON 出现 **0x5B**（已进入 mountroot，这本身就是目标态）
  或 **0x5C**（首次失败一次），加"长时间不复位、DDR 无 PARG"即完成
  实机验收。
- 手机仍在 E1F4；watcher 持续运行，恢复即按既定一键流程发射 v129。

---

## >>> 2026-10-25 04:42 预检全绿：v129 与 QEMU 路径完全对齐（真机不会碰 UFS）<<<

- **镜像预检**（WSL）：
  - shim 镜像 == shim/out 构建产物；ramdisk == /root/payload.bin；
  - payload 头正常（SWEETXNU v1、phys 0xa0000000、virt 0xfffffff006000000、
    10 段）；ADT 为**真机版**（sony,lilac / qcom,kryo280 / 含 ufs 节点，
    无 xiaomi,sweet）；
  - 内核内字符串确认：`SWEET STEP 155`、`SWEET STEP 139`、
    `PTE write is unaligned`、`sony,lilac`、`expected logical CPU 0`、
    `SWEET CPU: boot logical=` 全在；旧的 `expected boot MPIDR 0x100`
    已消失。
- **最后一项手机差异排除**：`sweet_ufs_init()` 唯一调用点是
  `sweet_ufs_rootdev()`（`rd=ufsroot0` 的 IOFindBSDRoot 钩子）。
  v129 cmdline 没有 `rd=ufsroot0` → **真机不会接管 UFSHCI、不会读
  userdata**；mountroot 将与 QEMU 完全一致（等待 HFS 介质/失败重试）。
  因此不需要“跳过 UFS”的应急镜像。
- 结论：v129 是单发高置信件。**当前唯一阻塞仍是手机 E1F4 按键**。
  恢复后执行 04:35 条目的一键命令，判据 PON 0x5C/0x5B。

---

## >>> 2026-10-25 04:41 v129 在 QEMU 回归通过：屏幕出现 STEP 155 + 等根 <<<

- 对 v129 源码（含 mountroot 信标）重新套 QEMU 地址重映射构建并跑 QEMU：
  - 屏幕实读到 **`SWEET STEP 155 (mountroot loop)`** 紧接
    **`Waiting on <dict ... IOMedia ... Apple_HFS>`**；
  - 之后 AMFI/Sandbox/HFS/kminit/dlil/ifnet 日志齐全，最后 idle；
  - **无 panic**（panic.bin 全 0）。
- 说明：QEMU 里读 PON 哑址（0xF2002000）在后期会被 VM 复用覆盖
  （实测读到 0x08），**不能**用 QEMU 的 PON 值做判读；真机 PON 在
  PMIC 里不受影响，仍以 0x5B/0x5C 为准。
- **源码已还原为真机地址**（三文件残留检查 0）；v129 手机镜像
  （k0264 / shim-4k-v129-rootbeacon-gzdtb.img）即最终首发件。
- 当前唯一阻塞：手机 E1F4（需按键）。恢复后按 04:35 条目的一键命令
  发射 v129，判据 PON 0x5C/0x5B + 屏上等根文本 + ~185s 复位 +
  DDR 无 PARG。

---

## >>> 2026-10-25 04:35 v129 已打包：带等根信标，作为首发目标 <<<

- 产物：`k0264-4k-rootbeacon.macho`、
  `shim-4k-v129-rootbeacon-gzdtb.img`（真机 ADT `/root/xnu12.adt`）。
- cell VA 仍 0xfffffff007fc884f → **PA 0xA1FC884F**（dump base 0xA1FC8800）。
- 内容 = v128（QEMU 验证的全部修复）+ bsd_init mountroot 信标：
  PON **0x5B**（进入 mountroot 循环）/ **0x5C**（首次挂载失败一次）。
- **设备恢复后首发 v129**：
  ```
  bash research/fire_harvest.sh "C:\Users\Cisco He\Desktop\Git\Patch\Xperia\shim-4k-v129-rootbeacon-gzdtb.img" 0xa1fc8800 "C:\Users\Cisco He\Desktop\Git\Patch\Xperia\k0264-4k-rootbeacon.macho"
  ```
- 成功判据：PON 最终 **0x5C**（或 0x5B）+ 屏上等根文本 +
  ~185s 才复位 + DDR 无新 PARG。v128 保留为对照/回退。
- 构建踩坑记录：`SWEET` 宏在 bsd_init.c 文件头尚未定义（由后面的
  include 引入），所以顶部 `#if defined(SWEET)` 的声明会被跳过——
  声明必须放在**使用处的 SWEET 块内**。

---

## >>> 2026-10-25 04:22 v129：mountroot 循环信标（等根确定性遥测）<<<

- 目的：手机没有可读屏幕时，用 PON 明确证明"已进入等根循环"。
- 改动（`bsd/kern/bsd_init.c`，SWEET 专属）：
  - `#include <arm64/sweet_early.h>`；
  - `/* Mount the root file system. */` 前 → `step_late(155)`（PON **0x5B**）；
  - 第一次 mountroot 失败打印 `cannot mount root, errno` 后 →
    `step_late(156)`（PON **0x5C**，仅一次，static 防刷屏）。
- 判读：手机 PON=0x5B = 已到 mountroot 循环；0x5C = 至少尝试失败一次
  （即真正进入 waiting-for-root，目标态）。
- v129 = v128 + 上述信标；构建中，完成后重新打包并更新 cell PA。
- 备注：前两次脚本尝试因 ssh heredoc 引号转义失败（仓库外脚本已修好），
  bsd_init.c 未被破坏。

---

## >>> 2026-10-25 04:17 v128 手机版已打包（QEMU 已到等根，待实机验证）<<<

- v128 = QEMU 验证过的全部修复 + **真机地址（QEMU 重映射已还原，
  源码残留检查为 0）** + `sony,lilac` personality + SweetCPU 放宽 +
  spllo 恢复 + 跳过 XNUPOST + step139/细分信标。
- 产物：`k0263-4k-alive.macho`、
  `shim-4k-v128-alive-gzdtb.img`（用 `/root/xnu12.adt` 真机 ADT）。
- `_sweet_panic_cell` VA 仍 0xfffffff007fc884f → **PA 0xA1FC884F**
  （DDR dump base 0xA1FC8800）。
- **设备恢复后一键发射**：
  ```
  bash research/fire_harvest.sh "C:\Users\Cisco He\Desktop\Git\Patch\Xperia\shim-4k-v128-alive-gzdtb.img" 0xa1fc8800 "C:\Users\Cisco He\Desktop\Git\Patch\Xperia\k0263-4k-alive.macho"
  ```
- **手机成功判据（与 QEMU 对应）**：
  1. PON 依次出现 0x4D/0x4E/0x4F/0x50/0x51/0x57/0x58/0x59/0x5A，最后
     **0x4B(139，即将/已进入 bsd_init)**；
  2. 屏上出现 `Waiting on <dict ... IOMedia ... Apple_HFS>`（vfs_mountroot
     等根）或 bsd_init 的 `cannot mount root` 循环；
  3. 长时间（~185s WDT）才复位、DDR panic cell 无新 PARG。
- 备注：真机会注册 `ufsroot0`（只读探测 Android userdata），失败后
  进入同样的等根循环；不会写 UFS。

---

## >>> 2026-10-25 04:13 【QEMU 达到目标态】屏幕出现 "Waiting on IOMedia/Apple_HFS" <<<

- 在 QEMU 里，修复链全部生效后，内核**一路跑过 bsd_init 并进入等根**：
  fb.png 实读文本包含：
  - `Waiting on <dict ... IOProviderClass ... IOMedia ... Content ... Apple_HFS>`
    ← 这就是目标里的 "waiting for root"（vfs_mountroot 等待 HFS 介质）；
  - 之前的日志还有 `File Integrity (SWEET AMFI)`、`Sandbox`、
    `SWEET HFS: hfs-627.100.6 registered`、`kminit done`、`dlil_init`、
    `ifnet_attach lo0/gif0 ...`。
  - CPU 最后停在 `_cpu_idle`/`_machine_idle`，**无 panic、panic cell 为空**。
- 达成该状态所需修复（全部已在源码，手机 v128 正在构建）：
  1. `spllo()` 恢复（EARLY_BOOT 后开中断）；
  2. VBAR MMU-on 后恢复 KVA 向量（否则 idle spllo 即死）；
  3. commpage text 叶子按 PAGE_RATIO 组对齐（16K VM 页/4K PTE）；
  4. `SweetPlatformExpert` 增加 `sony,lilac` personality；
  5. `SweetCPU` 只要求逻辑 CPU 0（不再硬编码 MPIDT 0x100）；
  6. SWEET 跳过 XNUPOST；step139+细分信标。
- **尚未实机验证**：手机仍卡 E1F4（需人工按键）。v128 构建完即打包，
  设备恢复后一键发射；手机侧的等价判据：PON 走到 0x4B(139) 之后，
  屏上出现 "Waiting on ... IOMedia ... Apple_HFS" 或
  bsd_init 循环打印 "cannot mount root, errno=..."，且长时间不复位。
- 注意 QEMU 与真机差异（v128 实测时要盯）：
  - QEMU ADT 无 `/sweet-ufs`；真机会注册 `ufsroot0` 并尝试挂载
    Android 加密的 userdata（只读探测，失败后进入同样的等根循环）；
  - 真机的触摸/显示/电池驱动会真正访问 MMIO（QEMU 中未触发问题）；
  - 真机 cpus=1 与 QEMU 1 CPU 一致，`SweetCPU::startAll` 路径相同。
- 工具链备注：`research/qemu-sweet-4k.py` 的 stub 现在会先
  `msr daifset,#0xf`；write_png 修好 RGB888 的 filter byte；
  每次运行自动 dump `panic.bin`（PA 0xa1fc8800）。

---

## >>> 2026-10-25 04:00 QEMU 推进到平台匹配：加 sony,lilac personality <<<

- commpage 对齐修复后，QEMU 一路跑到 IOKit 平台匹配并 panic：
  `IOPanicPlatform::start` → `Unable to find driver for this platform: "%s"`。
- 原因：`KernelConfigTables.cpp` 里 `SweetPlatformExpert` 的
  `IONameMatch` 只有 **'xiaomi,sweet'**；我们的 ADT 根 compatible 是
  `D22AP|iPhone10,3|sony,lilac|qcom,msm8998|AppleARM`，二者不匹配
  → 回落 IOPanicPlatform。**这同样是手机 v127 的下一个墙**
  （历史记录里从未真正越过 PE_init_iokit 的平台匹配）。
- 修复（已写入，正在重建）：在 KernelConfigTables.cpp 增加第二个
  `SweetPlatformExpert` personality，`IONameMatch='sony,lilac'`，
  IOProbeScore 10000（保留原 xiaomi,sweet 条目）。
- 说明：`SweetCPU` 匹配 'qcom,kryo280'，两份 ADT 都有，不受影响。
  下一步 QEMU 继续看 SweetPlatformExpert::start 之后是否会碰到
  真硬件 MMIO（触摸/显示/电池）——那时需按 QEMU 需要再哑化或从
  QEMU ADT 移除对应节点。
- 累积的 v128 手机版必含项（更新）：VBAR KVA 恢复、commpage 组对齐、
  sony,lilac 平台匹配、spllo 恢复、跳过 XNUPOST、step139+细分信标；
  且必须还原 QEMU 地址重映射（start.s/sweet_early.c/arm_vm_init.c）。

---

## >>> 2026-10-25 03:53 QEMU 闭环读出 panic：commpage PTE 未对齐（PAGE_RATIO=4）<<<

- VBAR 修复后 QEMU 继续推进，首次 panic 的 DDR cell 被 QMP `pmemsave`
  完整读出（runner 已自动 dump `panic.bin`）：
  - caller `_pmap_enter_pte+0x33c`；
  - 字符串 `%s: PTE write is unaligned, ptep=%p, pte=%p @%s:%d`。
- **我们实际是 16K VM 页 over 4K PTE（PAGE_RATIO=4）**（PAGE_SHIFT_CONST
  =PAGE_MAX_SHIFT），因此每个 VM 页 = 组对齐的 4 个连续 PTE，
  `write_pte_fast()` 一次写 32B；commpage text 页随机滑到任意叶子
  （`random()%num_ptes`）有 3/4 概率落在组中 → 首轮 commpage 映射即 panic。
  与 sagit 0118 描述逐字一致（他们已解）。
- **已移植 sagit 0118 的核心修复**：`pmap_create_commpages()` 的
  text 叶子改为 `num_text_slots = num_ptes / PAGE_RATIO;
  text_leaf_index = (random() % num_text_slots) * PAGE_RATIO;`
  （仍随机化，只是只在组首选址；碰撞检查保持原样）。
  `pmap_sync_page_attributes_phys` 我们本就用 `pp << PAGE_SHIFT`（=ptoa），
  无需改。
- 尚未移植（可选，非必需）：sagit 在 `pmap_enter_options_internal`
  加的 sub-VM-page VA 守卫（只是把错误信息变清晰）。
- QEMU 台当前状态：VBAR 修复 + commpage 对齐修复正在重建；
  下一步跑 QEMU 看第二个 panic。
- **手机版 v128 必须包含**：VBAR KVA 恢复 + commpage 组对齐 + spllo 恢复
  + SWEET 跳过 XNUPOST + step139/细分信标；**且必须还原 QEMU 地址重映射**。

---

## >>> 2026-10-25 03:48 【重大修复】SWEET 的 VBAR 停在 PA 低向量 → 第一个 IRQ 必死 <<<

- QEMU 复现（真机 v126/v127 也会中）：MMU-on 后 start.s 把 VBAR 指向
  **PA 低向量表**（0xa1420000）做诊断，但之后**从未恢复 KVA**。
  TTBR0（V=P）被 C 代码拆掉后，PA 向量不可取指；`idle_thread()` 的
  `spllo()` 第一次开中断 → IRQ 触发 → VBAR+0x280 取指中止 → 0x200
  空向量死循环。QEMU 日志：IRQ ELR=KVA `_ml_set_interrupts_enabled_with_debug`，
  PC=0xa1420080，FAR=0xa1420080（ESR 0x96000050/0x86000005）。
- 这解释了为什么 v119-v123 只能在"关中断"状态观察到 zalloc IRQ-off 断言：
  **一旦真的开中断（我们的 spllo 修复），就会先死在向量表上**。
- **v128 修复（已写入 start.s）**：MMU-on + 0xb2 门铃之后，立即用
  与 0xa7 相同的算法把 VBAR 恢复为 KVA：
  `adrp ExceptionVectorsBase; add x22; sub x23; msr vbar_el1; isb`。
  低向量诊断窗口仍保留（MMU 刚开、TTBR0 尚在时有效）。
- 该修复与手机/ QEMU 无关，**必须在下一个手机版 v128 包含**；
  QEMU 当前构建已带此修复，继续用它验证后续路径。

---

## >>> 2026-10-25 03:41 QEMU 离线台已能启动内核（L2 加速器）<<<

- **动机**：手机卡 E1F4、无法实机迭代；WSL 有 qemu-system-aarch64 6.2。
- **原理**：`tools/qemu-sweet.py` 把 SWEETXNU payload 的段/ADT/boot_args
  按 payload 头放到物理地址并跳 entry。QEMU virt RAM 从 0x40000000 起，
  4G 时覆盖 0x40000000-0x140000000 → 覆盖我们的 phys_base 0xA0000000、
  FB 0x9d400000、durable/prz，但**不覆盖** 0x14600000(IMEM)、
  0x08400000(SPMI)、0x17817000(APSS WDT) → 一访问就 data abort。
- **QEMU 专用地址重映射（临时！手机版必须还原）**：
  - `sweet_early.c`：early_rr→0xF00BF65C、early_imem→0xF0000000、
    early_pon→0xF2002000；
  - `arm_vm_init.c`：slot[18] PA=0xF0000000、slot[20] PA=0xF2000000；
  - `start.s`：0x8420000→0xF2002000（10 处）、0x146bf65c→0xF00BF65C、
    0x146BF680→0xF00BF680、0x146bf200/300→0xF00BF200/300、
    0x17817000→0xF3000000（WDT pet 哑写）。
  - 脚本：`research/patch_qemu_addrs.py`（仅覆盖 sweet_early/arm_vm_init；
    start.s 的替换目前是手工/可用同样方式复现）。
- **runner**：`research/qemu-sweet-4k.py` = qemu-sweet.py 的 4K/lilac 版
  （FB 0x9d400000、720×1280、RGB888 stride 2160、boot_args depth 24、
  CPU cortex-a53；QEMU 6.2 无 cortex-a76）。
- **首次运行证据**：PC 进入 entry 0xa141f488；第一条 PON 写
  0x8420010 触发 Translation Fault（ESR 0x96000050），随后跳 0x200
  空向量死循环——**证明 QEMU 能忠实跑我们的 4K 内核，且离线可定位
  最早期问题**。重映射后继续迭代。
- 运行命令（WSL）：
  `python3 tools/mkadt.py /tmp/q.adt --qemu` →
  `python3 tools/mkpayload.py K /tmp/q.adt /tmp/q.bin --cmdline "serial=3 cpus=1 keepsyms=1 -noprogress"` →
  `python3 research/qemu-sweet-4k.py /tmp/q.bin /tmp/qout --secs 40`；
  产物 `fb.png/regs.txt/qemu.log`（qemu.log 可看 ESR/FAR/ELR）。
- **收尾清单（发手机前必做）**：还原 start.s/sweet_early.c/arm_vm_init.c
  的地址；重建；用 nm 校验 `_sweet_panic_cell`/入口正常；再打包 v128。
  目前这些 QEMU 改动**尚未还原**，勿把当前 kernel 当手机版。

---

## >>> 2026-10-25 03:31 v127 就绪：138→139 每步细分信标（等设备恢复优先发它）<<<

- v127 = v126（spllo 恢复 + SWEET 跳过 XNUPOST）**加 9 个细分信标**，
  覆盖 138 与 bsd_init(139) 之间所有陌生步骤。PON 解码表：
  - 0x4D=141 dtrace 后；0x4E=142 csm 后；0x4F=143 trust_cache 载入后；
  - 0x50=144 STARTUP_SUB_LOCKDOWN 初始化后；
  - 0x51=145 OSKextRemoveKextBootstrap 后；
  - 0x57=151 machine_lockdown 后；0x58=152 PE_lockdown_iokit 后；
  - 0x59=153 vm_set_restrictions 后；0x5A=154 kperf_init_early 后；
  - **0x4B=139 = 即将进入 bsd_init**（成功判据）。
  （151-154 跳号是为了避开已占用的 146-150 编码区/旧信标 150。）
- 产物：`k0262-4k-spllo-nopost-steps.macho`、
  `shim-4k-v127-spllo-nopost-steps-gzdtb.img`；cell PA 仍 **0xA1FC884F**。
- **设备恢复后直接发 v127**（v126 作为备份）：
  ```
  bash research/fire_harvest.sh "C:\Users\Cisco He\Desktop\Git\Patch\Xperia\shim-4k-v127-spllo-nopost-steps-gzdtb.img" 0xa1fc8800 "C:\Users\Cisco He\Desktop\Git\Patch\Xperia\k0262-4k-spllo-nopost-steps.macho"
  ```
- 代码审读补充：ARM 上 `machine_lockdown()` 基本是轻量/no-op（实现主要在
  i386），`PE_lockdown_iokit()` 亦轻；这一段真正风险集中在
  `STARTUP_SUB_LOCKDOWN` 初始化与 `OSKextRemoveKextBootstrap`（4K 下
  KVA 段释放可能触发 pmap 断言）——细分信标正是为它们准备的。
- 根设备路径：`sweet_ufs` 会注册 `ufsroot0` 块设备；未传 `rd=ufsroot0`
  时 mountroot 会尝试并失败（只读探测，安全），随后 bsd_init 进入
  "cannot mount root" 重试循环。

---

## >>> 2026-10-25 03:27 v126 已打包就绪（等手机恢复后第一发）<<<

- 产物：`k0261-4k-spllo-nopost-m139.macho`、
  `shim-4k-v126-spllo-nopost-m139-gzdtb.img`（17,096,704 B）。
- v126 内容 = 恢复 `spllo()` + SWEET 跳过 XNUPOST + `bsd_init()` 前
  step 139 信标（期望 PON=**0x4B**）。
- cell VA 仍 0xfffffff007fc884f → **PA 0xA1FC884F**（dump base 0xA1FC8800）。
- **一键发射**（设备恢复后）：
  `bash research/fire_harvest.sh "C:\Users\Cisco He\Desktop\Git\Patch\Xperia\shim-4k-v126-spllo-nopost-m139-gzdtb.img" 0xa1fc8800 "C:\Users\Cisco He\Desktop\Git\Patch\Xperia\k0261-4k-spllo-nopost-m139.macho"`
  脚本会：清 PON（若在 Android）→ 补丁（重试 3 次）→ 发射并验证跳转
  → 最多等 4 分钟回 fastboot → 重新补丁 → 读 DDR cell 并解析
  → reboot 回 Android 读 PON。**不会**在 fastboot 会话里 reboot bootloader。
- **成功判据**：DDR 无新 PARG + PON=0x4B + 长时间（~185s WDT）后才复位。
- 手机当前仍 E1F4；已挂持久 watcher，恢复即通知。
  恢复方式（需人工）：USB 保持连接 → **音量上+电源 长按 ~10s**。

---

## >>> 2026-10-25 03:21 成功判据确认：bsd_init 无根设备会打印并死循环；加 step139 <<<

- 读 `bsd/kern/bsd_init.c`（870-900）：`while (TRUE) { err=vfs_mountroot();
  if (!err) break; printf("cannot mount root, errno = %d\n", err); }`
  —— **没有可挂载根设备时，内核会无限重试并打印 "cannot mount root"**，
  正是目标里的 "waiting for root" 阶段（sagit 的说法）。不会 panic，
  最终由 WDT（shim 180/185）复位。
- 但当前 `startup.c` 的 step 信标只到 **138**（mac_policy 后），
  `140` 在 bsd_init **返回后**（永不返回）→ 到达 bsd_init 时 PON 无
  新标记，无法与更早挂死区分。
- **v126 计划（v125 构建完成后改）**：在 `bsd_init();` 之前插入
  `sweet_early_step_late(139); printf("SWEET STEP 139 (bsd_init)\n");`。
  判据：PON=0x4B（139 编码）+ 之后长时间不复位（WDT 185s 左右）+
  无 panic（DDR 无新 PARG）→ 即到达 bsd_init/等根阶段。
- 注意：mountroot 失败循环会疯狂 printf（console 忙轮询），
  可能让复位更快/更慢；但 PON 139 一旦落下即证明越过全部 startup 初始化。
- 设备仍在 E1F4，等人工恢复；恢复后优先发 v126。

---

## >>> 2026-10-25 03:20 v125（未发射）：SWEET 关闭 XNUPOST；spllo 已恢复 <<<

- 读 `startup.c` 发现：`CONFIG_XNUPOST` **确实打开**（build log 有
  `-DCONFIG_XNUPOST`），而 `kernel_list_tests()`/`kernel_do_post()`
  **无任何 boot-arg 门控**，在 machine_lockdown 之后**开机就跑全量
  XNUPOST**，任一测试失败即 `panic("kernel_do_post: Tests failed...")`。
  这解释了历史上出现的 vm_tests/smr/test_lock 等“测试函数”候选断言。
- **v125 改动**（在 v124 的 spllo 基础上）：
  `#ifdef CONFIG_XNUPOST` → `#if defined(CONFIG_XNUPOST) && !defined(SWEET)`，
  即 SWEET bring-up 不跑测试套件（生产化行为）；spllo 修复保留。
- 构建中（`/tmp/build_v125.log`）；产物将打包为
  `shim-4k-v125-spllo-nopost-gzdtb.img`。
- **手机仍卡 E1F4**，需人工按键恢复；恢复后建议直接发 v125
  （用 `research/fire_harvest.sh` 一键完成 清PON→补丁→发射→
  回 fastboot 读 DDR cell→回 Android 读 PON）。
- 新增工具：`research/fire_harvest.sh`（含 E1F4 防护：在 fastboot
  会话里绝不 `reboot bootloader`）、`research/ddr_cell.py`
  （解析 PARG/caller/字符串并按 macho 映射函数）。

---

## >>> 2026-10-25 03:19 【需人工】手机停在 E1F4 充电模式；v124 已就绪未发射 <<<

- 事故链：03:09 我在 fastboot 会话里读 DDR cell 后，该会话随后
  `xperable -B -U ...` 补丁 0 成功（已知 stall）；执行
  `fastboot reboot bootloader` 刷新会话时设备离开 fastboot，
  再未回来——PowerShell 实锤 **USB\VID_0FCE&PID_E1F4**
  （Qualcomm 充电 UI，USB 大容量存储描述符）。
- **E1F4 无软件出口**（历史结论）：需人工按键。
  **恢复步骤（给用户）**：保持 USB 连接 →
  **音量上 + 电源 长按 ~10 秒** 强制重启；重启瞬间继续按住音量上，
  即直接进 fastboot（蓝灯）。或按电源回 Android 再
  `adb reboot bootloader`。
- **v124 已构建并打包**：`k0260-4k-spllo.macho` +
  `shim-4k-v124-spllo-gzdtb.img`（恢复 EARLY_BOOT 后 `spllo()`）。
  设备恢复后直接：清 PON（Android）→ bootloader → 补丁 2 条 success
  → `xperable -I shim-4k-v124-spllo-gzdtb.img -1 -c boot -1`
  → 等 ~70-200s 回 fastboot → **重新补丁** → 读 DDR cell
  （PA = k0260 的 cell VA - 0xfffffff006000000 + 0xa0000000；
  本次构建 cell VA 仍是 0xfffffff007fc884f → 0xA1FC884F，
  dump base 0xA1FC8800, size 512）→ 解析 `PARG`/caller/字符串。
- **教训**：
  1. 不要在长时间 fastboot 会话里频繁 `fastboot reboot bootloader`；
     补丁 stall 时**先重试一次 -B -U**，仍失败再重启会话；
  2. 读 DDR 后若不再需要该会话，直接用 `fastboot reboot` 回 Android
     （不要 reboot bootloader）；
  3. E1F4 一旦出现，夜间自动化即中断，必须在文档顶部醒目标注。
- 记录：PON 最后一次值 0x70（v123 高字节 run），已过期；
  DDR cell 内容属于 v123（zalloc IRQ-off 断言）。

---

## >>> 2026-10-25 03:09 突破：DDR panic cell 可跨复位读取；真凶=zalloc IRQ-off；恢复 spllo <<<

- **新遥测通道（重大）**：`sweet_ddr_panic()` 写的 `sweet_panic_cell`
  （含 magic `PARG`、caller、panic 字符串）**在复位后仍能从 fastboot 读出**：
  1. `nm` 取 `_sweet_panic_cell` VA（本构建 k0259：0xfffffff007fc884f）；
  2. 由 payload 头得 `virt_base=0xfffffff006000000`、`phys_base=0xa0000000`
     → **PA = VA - virt_base + phys_base**（cell = **0xA1FC884F**）；
  3. 内核死→回 fastboot 后（**先别 boot Android**），当前 ABL 会话
     **重新打补丁**（`-B -U -s 0xf49880 -x -4 -6`，须 2 条 success），
     然后 `xperable.exe -a 0xa1fc8800 -s 512 -r -O ddr.bin`；
     数据在偏移 0x4f 起：`PARG` + caller + 字符串。
  - 注意：读前不要重启 Android（会重新装载内核覆盖该区）；
  - 早前读错地址 0xA0FC884F（漏减 virt_base 0x6000000）得到代码字节，
    已纠正。`xperable -r` 必须配合 `-a/-s/-O`。
- **实锤首次 panic**（v123 构建）：
  - caller = `_zalloc_ext.cold.2`（ra=0x...a87010）；
  - 字符串 = `%s:%d Assertion failed: %s : Calling {k,z}alloc from
    interrupt disabled context isn't allowed`
    → `zalloc.c` 的 `zalloc_ext()` 断言（startup_phase ≥ EARLY_BOOT 后，
    IRQ 关闭时调用 zalloc）。
- **根因**：`startup.c` 在 `kernel_startup_initialize_upto(STARTUP_SUB_EARLY_BOOT)`
  之后原本要 `(void) spllo();` 开中断，但本树之前（v82 排障实验）
  用 `#if !defined(SWEET)` 把 spllo **整段跳过**——从此 IRQ 一直关着，
  后续任何 zalloc 都触发该断言。当时的实验已证明 spllo 不是死因
  （真凶是 console ring），这个 skip 是遗留。
- **v124（编译中）**：去掉 SWEET 的 spllo 跳过，**恢复 stock 行为**
  （EARLY_BOOT 之后开中断）。这是真正的修复而非绕过。
  预期：越过 zalloc 断言；若新 panic，直接按上面的 DDR cell 流程读
  panic 字符串（不再需要 caller/line 单字节拼图）。
- 后续纪律：**每次发射后先留在 fastboot 读一次 DDR cell**（~10s），
  再 reboot 到 Android 读 PON；两者互补。

---

## >>> 2026-10-25 03:00 v122 caller 低字节=0x00（11 个候选）；v123 取高字节 <<<

- v122（caller bits[11:4] 作为最终 PON，带首次锁存）实测 **PON=0x00**
  （Android 恢复后复读仍 00）。
- 用**同一构建**二进制跑 `panic_caller.py k0258 00`：2855 个 panic 调用
  点中 11 个字节为 0x00，含：
  `_ipc_mqueue_receive_results.cold.1+0x2c`（ra=0x...a68000，
  断言冷块，最可疑）、`_mach_syscall+0x2b0`、`_thread_reset_pcs_ast+0x138`、
  `_kqworkloop_get_or_create+0x280`、`_hfs_unlink+0x534` 等。
- 注意两个待排除项：
  1. caller 字节真为 0x00（11 候选中挑）；
  2. **BSS 未清零导致 `sweet_panic_state` 预置非 0**，panic 路径全部
     被锁存跳过 → PON 保持 00。v119–v121 能写说明那几次地址恰好为 0，
     但布局一变就可能踩到脏值。若 v123 仍读 00，优先按 2 处理：
     在早期初始化里显式把 `sweet_panic_state = 0`（debug.c 导出
     setter，在 sweet_early 末尾调用），再重测。
- **v123（编译中）**：最终写改为 `(caller>>8)&0xff`；配合 v122 的
  低字节 0x00，用 `panic_caller.py k0259 00 <hi>` 唯一化。
  k0258 中若某候选 `(ra>>8)&0xff` 互异即可收网。

---

## >>> 2026-10-25 02:51 v121 探针未触发：2579 是假命中，改用 caller 字节法 <<<

- v121（在 `ipc_kmsg.c:2579` 断言前埋 `aux_size<8` 探针，
  命中则写 0x80|aux_size 并锁存）实测 **PON=0xD3**（仍是 line 拼图
  facet），**探针没跑** → 首次 panic 不在 aux 断言处。
- 复核发现：探针插入使该文件行号 +8（断言移到 2587），
  而 (lo=0x13,hi=0x28) 在三份不同构建之间未必同源；**line facet
  跨构建不可靠**（和之前 caller 多字节法失效同因）。
- **改回最可靠的 caller 法（带首次锁存）**：
  - v122：`sweet_panic_assert_hash(str, ap, caller)`，最终写
    `(caller>>4)&0xff`（即 panic 返回地址 bits[11:4]）；
  - 读回后用 `research/panic_caller.py KERNEL.macho <lo>` 在**该构建
    的二进制**里匹配所有 `bl _panic` 调用点；
  - 若候选多，v123 改写 `(caller>>8)&0xff`，用工具的三参模式
    `panic_caller.py KERNEL lo hi` 唯一定位（bits[15:4]）。
- **纪律**：caller 字节必须用**同一构建**的二进制解码（v122 用
  k0258-4k-caller.macho）；line facet 只在单构建内可信。
- v121 的 ipc_kmsg 探针已回退；debug.c 的 `sweet_prepanic_commit`
  保留备用（未调用）。

---

## >>> 2026-10-25 02:43 首次 panic 唯一命中 ipc_kmsg.c:2579（aux_size 断言）<<<

- 锁存后重测：v120 得 **0xD3 → lo=0x13**；配合 v119 两次一致的
  **hi=0x28**，全表 6924 条断言中唯一命中
  **`osfmk/ipc/ipc_kmsg.c:2579`**
  `assert(aux_size >= sizeof(mach_msg_aux_header_t))`
  （函数 `ipc_kmsg_get_body_and_aux_from_user`，调用链
  `mach_msg_trap_send` ← `mach_msg2_trap` 向量发送）。
- **诡异点**：正常路径 `mach_msg_validate_data_vectors()` 已经会拒绝
  `aux_size !=0 && < 8`，理论上到不了这条断言；说明 aux 向量参数
  可能来自未初始化/异常路径（例如 vec_count=1 但代码仍读 aux_vec），
  或存在更早期的内存/参数破坏。另：这条 assert 之后若继续会写
  `aux_header`（8B）而只分配了 aux_size（<8）字节 → 有 4B 级风险，
  不能简单删除断言了事。
- **v121（诊断，编译中）**：在该断言前加 SWEET 探针
  `sweet_prepanic_commit(0x80|(aux_size&0x3f))`：先写 PON，再把
  首次锁存置 2（后续 panic 不再覆盖），断言仍会触发但 PON 保留
  aux_size。预期读回 `0x84` 等（aux_size=4）。这同时独立确认站点
  （只有该站点会在 aux_size∈1..7 时执行）。helper 在 debug.c。
- **若确认 aux_size=4**：修法 = 在该处把 `aux_size < 8` 当作
  `MACH_SEND_INVALID_DATA` 返回（与验证层语义一致），而不是 panic；
  然后继续引导。另需查清"到底谁在发 aux 向量"（早期用户态？内核
  自身 IOKit 消息？还是 kmsg 参数损坏），必要时追 user PC。

---

## >>> 2026-10-25 02:37 首次 panic 锁存成功：PON=0xE8 两次一致；重测 lo/hf <<<

- **v119（首次 panic 锁存）连发两次，PON 都是 0xE8** → `hi=(line>>6)&0x3f=0x28`。
  首次 panic 是**确定性**的；此前 v116/v117 的 lo=0x13 / hf=0x36
  不可信（来自嵌套 panic 的二次覆盖）。**以后所有身份测量必须先带
  锁存**。
- 候选集：全表 6924 条断言中 `hi==0x28` 共 **77 条**；
  6 位 facet 区分度：`lo` 43 桶（最大 5）、`hf` 22 桶（最大 7）；
  `(hf,lo)` 联合 74 桶、最大 2。
- **v120（编译中）**：锁存保持，信标改为 `0xC0|(line&0x3f)`（lo）。
  读回后先在 77 条里筛；若仍 >1，再 v121 测 `0x80|hf`，用
  `(lo,hi,hf)` 三 facet 唯一化（预期最多剩 2 条，再按代码语义判）。
- 备忘：`hi=0x28` 意味着 `line ≡ 0xA13 (mod 4096)` 的行号区间
  （即 259+... 多个 4096 周期），别只盯 2595 附近。

---

## >>> 2026-10-25 02:28 v118=0xE8（hi=0x28）与 v116/v117 不交：疑似嵌套 panic 覆盖 PON <<<

- v118（写 `0xC0|((line>>6)&0x3f)`）实测 **PON=0xE8** → `hi=0x28`，
  即 `line ≡ 0xA13 (mod 4096)`。与 v116（lo=0x13）和 v117（hf=0x36）
  **没有共同候选**：
  - v117+v118 → 6 条，全在 `osfmk/kern/zalloc.c`
    （2573/2593/2614/2615 `zone_depot_*` 断言、6668/6681 `zalloc_ext`）；
  - v116+v118 → 仅 1 条 `osfmk/ipc/ipc_kmsg.c:2579`；
  - 三次交集 = 0。
- **结论（高置信）**：PON 的最终值不是"首个 panic"的名片——首个 panic
  写完信标/保屏后进入 `panic_trap_to_debugger`，**嵌套 panic**（无调试器
  时 panic 路径本身可能再断言）会再次调用 `panic()` 覆盖 0x88e。
  每次构建的代码布局/时序不同 → 嵌套点不同 → 三次读数不一致。
  （也解释了为什么 v114 的"最终写"机制看起来时好时坏。）
- **v119（下一步）**：加 "首次 panic 锁存" `sweet_panic_state`
  （0=未发生，1=已写信标，2=收尾）：
  - `panic()` 只在 state==0 时调用 `sweet_ddr_panic()`（字符串哈希）
    与 `sweet_panic_assert_hash()`+`sweet_panic_hold()`；
  - `panic_with_options()` 同样只在 state==0 时写信标与保屏；
  - 嵌套 panic 不再碰 PON。
  然后**同一 v119 连发两次**：若两次 PON 相同 → 首个 panic 稳定，
  用现有三字节约束联合定位；若仍不同 → 首个 panic 本身不确定，
  需换策略（例如首次 panic 时把身份写进多个可持久位置/改屏显）。
- 全量断言表 `research/assert_all.txt`（6924 条，含 hf/hi/lo 三列）
  已生成，可直接做多维过滤。
- zalloc 六候选语义：`zone_depot_pop_head_full/empty`、`zone_depot_move_full`
  （magazine/depot 记账断言，竞态类）；`zalloc_ext` 的
  `startup_phase < STARTUP_SUB_EARLY_BOOT` 与 Z_NOFAIL 检查（更偏确定性）。

---

## >>> 2026-10-25 02:20 v117 PON=0xB6：文件哈希 0x36，剩两个候选（monotonic/zalloc）<<<

- v117（写 `0x80|(basename 33-hash&0x3f)`）实测 **PON=0xB6** →
  文件哈希 = **0x36**。注意：**不是** 4 个 1235 行候选
  （0x00/0x1B/0x2F/0x03），说明 v116 的 110 条全表里另有命中。
- 交叉 `行号&0x3f==0x13`（v116）与 `文件名哈希==0x36`（v117）：
  全表仅 3 个文件，其中早期可能 2 个：
  - `osfmk/kern/kern_monotonic.c:83` `assert(mtc != NULL)`
    （`mt_fixed_counts_internal`，kpc 固定计数器路径；per-cpu mt_cpu 为空）；
  - `osfmk/kern/zalloc.c:9619` `assert(zone_pva_is_null(z->z_pageq_partial))`
    （`zdestroy()`；zone 销毁时 partial 页队列非空）。
  第三个 `bsd/net/if_fake.c:1619` 在用户态网络，早期不可能。
- **两次测量互相一致**：两候选都满足 v116 的 line%64==0x13，故站点
  在两次构建间稳定；排除 1235 组是数据驱动的，不是假设。
- **v118（下一步）**：写 `0xC0 | ((line>>6)&0x3f)`。预期
  `0xC1` = kern_monotonic.c:83（83>>6=1）；`0xD6` = zalloc.c:9619
  （9619>>6=150, &0x3f=0x16）。随后按结果修：monotonic 查
  `mt_cur_cpu()`/per-cpu 初始化时序（4K/单核启动）；zalloc 查
  zdestroy 时 partial 队列残留（可能 zone_gc/回收路径）。
- 备注：v116/v117 的 PON 0xD3/0xB6 各自都是 helper 直写，说明信标
  链路稳定；每次发射仍按"清 0x88e → 验证 USB 真断开"执行。

---

## >>> 2026-10-25 02:12 v116 成功：确认 assert，line%64=0x13；v117 编码文件名哈希 <<<

- **v116（linger 移到 assert 信标之后）实测 PON = 0xD3** →
  `0xC0|0x13`：**确认是 assert**，且 `行号 & 0x3f == 0x13`。
  信标链路从此可靠（先写 PON 再保屏）。
- **重要更正**：v114 的 0x52 **不是** assert 文件+行哈希，而是通用
  assert 格式串哈希（`panic()` 在 linger 之前写的那一笔）。所以
  "文件+行哈希=0x12" 的假设作废——按该假设筛出的 125 条只用于
  提示，不能当约束。
- **重新枚举**：全树（排除 tests/x86/kdp/san）满足
  `行号&0x3f==0x13` 的 assert 共 **110 条**；按 `(line>>6)&0x3f`
  分 52 桶。桶 0x13 里恰好是 **4 个不同文件的第 1235 行**：
  `bsd/pthread/pthread_workqueue.c:1235`、
  `osfmk/kern/turnstile.c:1235`、
  `osfmk/vm/vm_fault.c:1235`、
  `osfmk/vm/vm_compressor_backing_store.c:1235`。
  因此行号高 6 位无法区分，必须再加**文件名**维度。
- **v117（下一步）**：assert 最终写改为 `0x80|(文件名33-hash&0x3f)`
  （basename 前 24 字符）。四个候选的哈希已算好：
  pthread_workqueue=0x00、turnstile=0x1B、vm_fault=0x2F、
  vm_compressor_backing_store=0x03。若落 turnstile/pthread_workqueue
  即唯一；若落 vm_fault/vm_compressor 需再用 v118 写
  `0xC0|((line>>6)&0x3f)` 区分同文件内的多行。
- **联合唯一性已验证**：110 条候选中
  `(文件名哈希, (line>>6)&0x3f)` 只有 1 组碰撞
  （IOPolledInterface.cpp:979 vs model_dep.c:979，后者才是早期可能）。
- 候选语义备忘：turnstile.c:1235 = `assert(thread->inheritor == NULL)`
  （锁等待唤醒时 PI 记账；turnstile_complete_compact_id，唯一调用者
  lock_mtx.c:583 lck_mtx_lock_wait_done）；vm_fault:1235 =
  `assert(first_m == VM_PAGE_NULL)`；vm_compressor_backing_store:1235 =
  `assert(c_seg->c_bytes_used == 0)`；pthread_workqueue:1235 =
  `assert(wq->wq_creator == NULL)`（早期基本不可能）。
- 工具：`research/assert_enum.py`（按哈希枚举）、
  `research/assert_decode.py`（0xC0+ 解码）、
  `research/assert_line13.txt`（110 条行号&0x3f==0x13 全表）、
  `research/patch_hold_after_hash.py`（v116）。

---

## >>> 2026-10-25 02:04 v115 仍是 0x52：panic linger 挡在 assert 信标前面 <<<

- **v115（最终写 0xC0|(line&0x3f)）两次发射**：第一次 xperable 的 boot 没跳
  （遗留 fastboot，PON 仍 0x00）；第二次确认 USB 3s 内断开（真跳），
  ~73s 回 fastboot，Android 读 0x88e **仍 = 0x52**。
- **结论：assert 信标根本没执行。** 顺序是
  `sweet_ddr_panic()`（写 panic 串哈希 0x40|0x12=0x52，然后 **60s linger**）
  → 返回后才 `va_start` + `sweet_panic_assert_hash()`。**看门狗 ~55-60s
  先把设备复位**，所以 PON 永远停在通用 assert 格式串哈希 0x52。
  这同时说明 v114 的 0x52 也不是"文件+行哈希 0x12"，而是通用 assert 串。
- **仍未定性 assert vs 普通 panic**：两者都可能写 0x52（0x40|0x12）。
  但 assert 概率大。
- **v116（已改，02:04 开始编译）**：把 60s linger 从 `sweet_ddr_panic()`
  移到新函数 `sweet_panic_hold()`，`panic()` 里在
  `sweet_panic_assert_hash()` **之后**才调用；`panic_with_options()` 也在
  trap 前 linger。这样所有 PON 写都先落，再保屏 60s，WDT 收场。
  预期 v116 PON：≥0xC0 = 确认 assert（低 6 位=行号&0x3f）；仍 0x52 =
  普通 panic（字符串哈希 0x12）。
- **发射纪律补充（血泪）**：`xperable -I ... boot` 后必须轮询
  `fastboot devices` 确认 USB **真的断开**（§已知第一发常不跳）；
  本轮回合已按此执行。清 PON 基线用
  `echo "88e 00" > .../regmap/spmi0-00/registers`，读回 00 再发。

---

## >>> 2026-10-25 01:53 v114 结果 PON=0x52（有歧义）；v115 用行号字节消歧 <<<

- **v114（k0250，hash 作为 panic 最终 PON 写）01:48:43 发射，~55s 复位后
  读 0x88e = 0x52。** 0x52 有两种可能：
  1. 通用 assert 格式串 `%s:%d Assertion failed...` 的 24 字符哈希
     （此前 v96 已算得 0x52）——若 `sweet_panic_assert_hash` 未生效，
     就会停在这个中间值；
  2. 已识别的 assert 且其 (文件名, 行号) 哈希 &0x3f == 0x12，
     写得 `0x40|0x12 = 0x52`，与 1 恰好同值。
  另外 79 条普通 `panic("...")` 格式串的前 24 字符哈希也是 0x12
  （`panic_hash.py <tree> 52`），所以仅凭 v114 不能断定是 assert。
- **新工具 `research/assert_enum.py`**（实现 debug.c 的精确哈希：
  basename 前 24 字符 33-hash → 行号低/高字节 → &0x3f）。
  hash 0x12 命中 **125 个 assert 调用点**（其中内核可达 115 个）。
  与早期 pmap 路径相关的两个：
  - `osfmk/arm/pmap/pmap.c:3876` `pmap_tte_deallocate()` —
    `assertf((tte & ARM_TTE_TYPE_MASK) == ARM_TTE_TYPE_TABLE, "%s: invalid TTE ...")`
    （4K 页表释放/降级异常高度可疑）；
  - `osfmk/arm/pmap/pmap.c:11495` `pmap_unpin_kernel_pages()` —
    `assert(!(pp_attr_table[pa_index(pa)] & PP_ATTR_MONITOR))`
    （与 v111 的 pmap_update_cache_attributes 怀疑同属 PPL 属性路径）。
  其余多在 vm/sched/ipc（vm_fault/vm_resident/vm_map/priority/sched_prim…）。
- **v115 消歧（已改 `osfmk/kern/debug.c`，01:53 开始增量编译）**：
  `sweet_panic_assert_hash()` 最终写改为 `0xC0 | (line & 0x3f)`
  （`sweet_early_pon_raw` 原值写，不经 step 编码）。判读：
  - PON ≥ 0xC0 → 确认是 assert，低 6 位=行号&0x3f；与 v114 的
    文件+行哈希 0x12 联合过滤应唯一；
  - PON 仍 0x52 → 不是 assert（普通 panic，字符串哈希 0x12），
    转为按 79 条 panic 字符串候选+调用者方法定位。
- 注意：目标里 "step 174 / commpage assert" 是旧结论；当前实际已越过
  SHA256（PRNG 之后）死在新的 assert/panic 点。SHA256_CTX +512 修复
  生效（v111 后不再出现 SHA256_Init panic）。
- 打包/发射流程（本阶段固定）：Mac 产出 kernel.development.sweet →
  scp 到 WSL `/root/kNNNN.macho` → `tools/mkpayload.py K /root/xnu12.adt
  OUT --cmdline "serial=3 cpus=1 keepsyms=1 -noprogress"` →
  `cd shim && GZ=1 RET=1 bash build.sh /root/payload.bin` →
  `out/shim-boot-gzdtb.img` 拷回 `Patch/Xperia/shim-4k-vNNN-*.img` →
  清 PON → 双 patch → `xperable -I` 一发。

---

## >>> 2026-10-25 00:40 【副线·急】sagit 同 SoC 刚解了 commpage 类坑 + 曝出更致命问题 <<<

`research/ios-sweet-sagit` commit `a3583ff`（10-24，MSM8998 同源）：

1. **commpage panic 真凶（他们的版本）**：
   `pmap_create_commpages()` 把 commpage text 页**随机滑到任意 4K 叶子索引**；
   在 PAGE_RATIO=4（16K VM页/4K PTE）配置下，VM 页必须落在
   4 的倍数叶子上——3/4 概率落在组中 → `write_pte_fast: PTE write
   is unaligned` panic。修复=挑组对齐的 commpage 槽位 +
   `pmap_enter_options` 加 sub-VM-page 守卫 + `pmap_sync_page_attributes_phys`
   改用 `ptoa(pp)`（VM 页号非颗粒页号）。
   **如果我们的 panic 形态吻合（随机性/对齐相关），直接看他的补丁
   `xnu/patches/0118-sagit-16k-vm-pages.patch`。**

2. **更致命——纯 4K 内核跑不了 iOS 用户态**：
   iPhone X 用户态 CodeDirectory.pageSize=14，而
   `cs_validate_codedirectory()` 只接受 12 或 PAGE_SHIFT →
   **4K 内核永远 exec 不了任何签名二进制**（kernel 能 boot 但
   launchd 起不来）。sagit 的解法：`PAGE_SHIFT_CONST = PAGE_MAX_SHIFT`
   （保持 14=user ABI）+ 4K 颗粒 PTE（PAGE_RATIO=4）——
   即"16K VM 页 over 4K PTE"而非纯 4K 内核。
   UniXNU 也有对应补丁 `native-4k-signature-hash-cache`（16K
   CodeDirectory→4K 校验桥）——**我们当前的纯 4K 路线迟早要撞这墙，
   建议主线尽早对照 sagit/UniXNU 选边。**

---

## >>> 2026-10-25 00:30 【副线】前沿物资全到位（不碰手机）<<<

> 副线成果交接，主线调试不受影响。详：`HANDOVER-FRONTIER.md`、
> `RESEARCH-FRONTIER.md`、副线规则 `SUBAGENTS.md`。

- **iOS rootfs 已建成**：Mac `~/Desktop/ios-assets/iosroot-16.7.16.img`
  （17GB UDRW，HFS+J case-sensitive，fsck OK）——iPhone X 16.7.16
  完整 rootfs+cryptex+dyld cache+keybag；写 userdata 前需
  `hdiutil convert -format UDTO` 剥壳。
- **WiFi 修正**：lilac = **QCA WCNSS**（modem 分区 wlanmdsp.mbn），
  非博通 → 无 iOS 原生驱动，长期看 ios-topaz 的 TopazModem 路线。
- **固件仓**：`research/firmware-extract/fw/`（modem FAT 全量+
  BT 全套）；vendor 固件待 adb pull 兜底（手机空闲时）。
- **UFS 保底**：QMP v3 PHY 校准表已提取（sony-4.4 `phy-qcom-ufs-qmp-v3.h:219-306`）。
- **同 SoC 模板**：`research/UniXNU-nash`（MSM8998）board.toml 到手
  （FB 0x9D400000 相同、UEFI G1 已过、目标同为 16.7.16）。
- **参考**：UniXNU 4K 补丁文档 `research/UniXNU/docs/xnu/four-k-page-tables.md`
  对 commpage/4K 问题可能有现成语义。
- 仓库 readme：`ios-xperia-xz1c/README.md` 已重写为 lilac 实况。

---



**续 00:55（副线再推进）**：
- userdata 实拆 21.95 GiB，rootfs 放得下（FR §10）。
- GPU 固件已从 vendor.sin 挖齐（a540_zap/gpmu/cpp/ipa，FR §9）；
  UniXNU GPU 栈=A740 专用，A540 需 zap+CP 直驱自研（FR §11）。

## >>> 2026-10-25 01:44 三字节法失效，改用 panic 字符串哈希（跨构建稳定）<<<

- v113（beacon 写 bits[19:12]）实测 PON=**0x86**；与 v111 hi=0x6E、
  v112 lo=0xEB 联合过滤（lo=EB/hi=6E/b3=86）**0 个匹配**。
  原因：三次测量来自不同构建（即使只改 debug.c 的立即数），link 布局
  或实际 panic 点可能变化，**跨构建拼字节不可靠**。
- **新方案（下一棒第一件事）**：panic 时把 **format string 前 24 字符
  的 33-hash（0x40|(h&0x3f)）作为 PON 的最终写入**（把该写放到 caller
  字节之后），该值跨构建稳定；再用 `research/panic_hash.py` 对源码树
  的 panic 字符串离线匹配。若仍有碰撞，可再引入第二段哈希（如文件名），
  或结合步骤/页信息人工收敛。
- 设备当前在 Android；全链路仍只读 boot。所有代码/脚本/镜像均在
  Patch/Xperia 与 ios-xperia-xz1c 仓库内，AGENTS/NOTES 已同步。

## >>> 2026-10-25 01:40 v111/v112：+512 后 panic 页变为 0x6E，双候选待第三字节 <<<

- v111（SHA256_CTX 余量 +512，全量重编）实测 PON=**0x6E**（hi 字节，布局随
  结构体变大而移动）。v111 二进制 hi=0x6E 的调用点共 12 个，关键两个：
  - `_pmap_update_cache_attributes_locked+0x58c` ra=0x...406eb0（lo=0xEB）
  - `_SHA256_Init.cold.1+0x30` ra=0x...b16eb4（lo=0xEB）
  另有 `_entropy_provide.cold.6`、`_compress_block.cold.1`、
  `_os_ref_panic_retain` 等。
- v112（同布局，仅把 beacon 改回 bits[11:4]）实测 PON=**0xEB**；与 hi=0x6E
  组合后只剩上述两个候选（bits[19:16] 未知）。
- v113（beacon 改为 bits[19:12]）已编译完成（k0249 待打包）；
  读第三个 PON 字节即可唯一确定：pmap 缓存属性路径（bit16-19=0x0）还是
  SHA256_Init（bit16-19=0x1）。
- 若指向 `pmap_update_cache_attributes_locked`：是 4K 块/页属性路径的真实
  早期 bug（panic 信息有 'page is not managed' / 'physical aperture PTE 
  has hint bit set' / 'unsupported VM_WIMG'），按源码与 sagit 对照修。

## >>> 2026-10-25 01:12 v110 仍 SHA：ctx 余量 +128 不够，改 +512 全量重编 <<<

- 清洁全量重编（删除 5 个 DEVELOPMENT 对象目录）后，v110 实测 PON
  仍 **0x6F**；在 v110 二进制里 hi=0x6F 的 17 个候选中，
  `_SHA256_Init.cold.1+0x30` 位于同一页（ra=0x...b16ff4）→
  **SHA256_CTX 的 +128 余量仍不够**（LTC 的 ctx 更大）。
- 已把三份 `sha2.h` 副本（源码 / EXPORT_HDRS / fakeroot）的余量改为
  `CCSHA256_STATE_SIZE + 512`，并再次删除 5 个 DEVELOPMENT 对象目录
  做全量重编（`/tmp/build_sha512.log`）。
- 完成后：清 PON → 打包 v111 → 烧录 → 读 0x88e；若 int 仍 0x6F，
  下一步改用"双字节法"在新二进制上重新取 lo/hi（v111 写 lo，v112
  写 hi），确认是否还有第三个同页 panic。

## >>> 2026-10-25 00:52 v109 未生效：SHA 对象/头副本问题 <<<

- v108/v109 把 `SHA256_CTX` 扩到 `CCSHA256_STATE_SIZE+128` 并同步了
  `build/xnu.obj/EXPORT_HDRS/.../sha2.h` 与 fakeroot 副本（两者原为
  0444，需先 chmod），但实测 PON 仍 **0x6F**（=SHA256_Init 站点高字节）
  → SHA 修复未生效。疑点：`corecrypto_sha2.o` 根本不在内核链接对象列表
  （grep 链接行 0 次），`_SHA256_Init` 可能由别的 TU（如 bsd/crypto 或
  libkern 聚合对象）编译；下一棒先 `nm`/`grep` 找到真正定义 SHA256_Init
  的目标文件与它包含的 sha2.h 路径，再强制重编该对象。

- 设备当前在 Android，v109 已复位；下一发前记得清 PON。
- **v110 进行中**：查明 `corecrypto_sha2.o`（在
  `build/xnu.obj/DEVELOPMENT_ARM64_SWEET/libkern/DEVELOPMENT/`）
  构建于 00:39，早于 00:44 的头文件同步且未被增量重编；由于
  `SHA256_CTX` 是 ABI（大小变了必须全量一致），已删除 osfmk/libkern/
  bsd/iokit/pexpert 的 DEVELOPMENT 对象目录做一次完整内核重编
  （`/tmp/build_clean.log`）。完成后：清 PON → 烧 v110 → 读 0x88e，
  确认 SHA 断言是否消失、下一步 panic 落在哪。

## >>> 2026-10-25 00:37 突破：panic 唯一定位 SHA256_Init（PRNG 阶段）<<<

- **双字节 caller 定位法成功**：v106 写 caller bits[11:4] → PON=0xF5；
  v107（同尺寸布局，仅把移位 4 改 8）写 bits[15:8] → PON=0x6F。
  两者组合成低 16 位 0x6FF?；`panic_caller.py lo=0xF5 hi=0x6F`
  **唯一命中**：
  `_SHA256_Init.cold.1+0x30`  ra=0xfffffff007b16f54。
- **panic 原文**（libkern/crypto/corecrypto_sha2.c:63）：
  `SHA256_Init: inconsistent size for SHA256 context`
  —— `ccdigest_di_size(di) > sizeof(SHA256_CTX)`。
  SWEET 代码把 SHA256 直接指到 XNU 内建 LTC 实现
  （`ccsha256_ltc_di`），其 context 比 SDK 公有 `SHA256_CTX` 大。
- **调用者判断**：启动期第一个 SHA256_Init 来自 SWEET PRNG
  （prng_random.c，"SWEET PRNG: HMAC-SHA256 registered"）。
  → **内核已越过 arm_vm_init、serial_init、kernel_bootstrap，
  进入 EARLY_BOOT/PRNG 阶段**，与 v76 屏上看到的阶段一致。
- **注意**：此前 v101-v104 的 lowglo/arm_debug_set/FF 归属可能都是
  单字节碰撞造成的误判（双字节法才是可靠的）；但那些跳过补丁
  本身无害，保留即可。
- **下一修正**：让 `SHA256_CTX` 足够大（或改 SHA256_Init 用内建
  上下文），使 `ccdigest_di_size(ccsha256_ltc_di) <= sizeof(sha256_ctx)`
  成立。查 `SHA256_CTX` 定义位置（SDK/EXTERNAL_HEADERS）与
  `ccsha256_ltc_di` 的 size 字段，再决定放大结构还是改用内部缓冲。

---

## >>> 2026-10-25 00:29 v106 结果：panic 非 assert；候选出现"测试函数" <<<

- v106（assert 文件/行号哈希）实测 PON=**0xF5**（不在 0x40-0x7f
  范围）→ **本次 panic 不是 assert 格式**，仍是 caller 字节。
  （机制本身已验证：若为 assert 会写 0x40|h。）
- 用 v106 二进制解码 0xF5 候选（17 个），取到的 format string 中
  最值得注意的：
  - **`_pmap_insert_commpage+0x64` → `%s: failed to insert the shared
    page, kr=%d, pmap=%p`**（commpage 在 pmap 里插入失败——说明内核
    可能已走到 task/vm_commpage_enter 相关路径）；
  - `_pmap_remove_options_internal+0xbc` → `Attempt to demote L%u
    block, tte=... target_level=...`（4K 下块级降级异常）；
  - `_lck_rw_lock_shared_internal+0x1c8` → `Assertion failed: ... :
    state=0x%x, owner=%p`（RW 锁状态断言，早启动可疑）；
  - 另外两个候选是**单元测试函数**：`_kmem_realloc_basic_test`
    （generic assert）、以及上一轮的 `smr_hash_basic_test`。
- **新怀疑（下一棒优先验证）**：`CONFIG_XNUPOST` 在编译 flags 里是
  打开的（见 build 日志 -DCONFIG_XNUPOST），`startup.c` include 了
  `tests/xnupost.h`；**如果 XNUPOST 测试在启动时被自动运行**，
  多发的 panic 可能来自测试本身而非 bring-up。查
  `tests/xnupost.c` / `kern_tests.c` 的 STARTUP 触发条件，
  若确认则对 SWEET 关闭 XNUPOST（或移除测试 STARTUP），可能直接
  放行很大一段。
- **当前内核位置推断**：候选中同时出现 commpage pmap 插入、RW 锁、
  vm_purgeable、proc_ro_ref_task 等 → 已远超 arm_vm_init，可能已到
  BSD/task 初始化附近；"waiting for root" 仍有距离但方向明确。
- **状态**：本会话即将 compact；AGENTS.md/NOTES.md 已与实机同步。
  下一棒起手：
  1. bindershell 清 PON（"88e 00"）；
  2. 查 XNUPOST 启动触发；若在跑，SWEET 关闭后重编；
  3. 否则针对 `pmap_insert_commpage` / RW 锁断言做下一轮细分。

---

## >>> 2026-10-25 00:22 assert 定位实验进行中（v106）<<<

- v105/v106 目标：panic 若是通用 assert 格式，则把**断言的文件名+行号**
  哈希写入 PON（最终值 0x40|h），否则保持 caller 字节。
  - 已预计算：`test_lock.c:201`→0x42、`pmap.c:8168`→0x66、
    `lowmem_vectors.c:136`→0x4F、`pcb.c:995`→0x6C、
    `lock_ticket.c:330`→0x68。
  - 实现放在 `panic()` 的 `va_start` 之后（`sweet_ddr_panic` 没有
    va_list）；`sweet_ddr_panic` 只保留格式串哈希+caller 字节。
  - 构建中遇到并已修正：第一次把块插进了 `sweet_ddr_panic`（无
    va_list，编译错误）；回滚并拆出独立 helper。
- **判读 v106**：
  - PON=0x42 → 确实是 test_lock.c:201 的 SMR 测试断言（说明内核
    已进入测试框架/很后期，需查 XNUPOST 为何在启动时运行）；
  - 其他 0x4x-0x7f → 另一条 assert，按同样哈希表反查；
  - PON=0xF3 → 不是 assert，回到 caller 字节候选（thread_setrun /
    proc_ro_ref_task / list corruption 等）。
- **上下文提醒**：本会话即将 compact；所有关键状态已在本文件与
  NOTES.md 中；下一棒从"清零 PON → 烧 v106 → 读 0x88e"继续。

---

## >>> 2026-10-25 00:10 v104 仍 F3；候选指向 test_lock.c:201（待证）<<<

- v103 的 FF 补丁打错了位置（同名代码第一处，另一个函数）；v104 用
  `cff_finish:` 锚点正确 guard 了 `arm_clear_fast_fault`，pmap.o 已重编
  （build_ffc2.log 有 CC pmap.o），但实测 **PON 仍 0xF3**。
- 用 `panic_caller.py`（v104=k0241）列出 0xF3 的 13 个候选，并用
  `panic_strings.py` 批量反汇编取到每条 format string。其中
  - `__smr_hash_basic_test_block_invoke+0x5b4` → generic assert，
    反汇编 x0/x8/x11 得到 **test_lock.c:201
    `assert3u(h->smrh_count, ==, nelems / 2)`**（SMR hash 单元测试）。
  - 其他候选：thread_setrun（scheduler IPI）、proc_ro_ref_task
    （"proc_ro already has an owning task"）、Bad list/tailq head、
    fiq preemption、in6m refcnt 等。
- **矛盾**：test_lock.c 的测试只通过 `debug.test.*` sysctl 注册，
  正常启动不应运行；若它真的在跑，说明内核已进入很后期的测试框架。
  需要下一发确认。
- **关键纪律（已写入本日志）**：
  - 每次测量前必须清零 PON，屏上文字可能是 DSI 面板残留旧帧；
  - 当前可靠遥测 = PON 一个字节（panic caller 地址 bits[11:4]）；
  - `printf/kprintf` 的 os_log、console ring、video console、lowglo
    断言、arm_debug_set flavor、fast-fault 计数 panic 均已在 SWEET
    下绕过/跳过。
- **下一发（v105）计划**：panic 时若格式串以
  `%s:%d Assertion failed` 开头，则把**断言的文件名+行号**（va_list
  前两个参数）哈希进 PON（0x40|h），离线复算匹配，确定到底是哪条
  assert；否则仍写 caller 字节。这样能把 assert 类 panic 精确定位。

---

## >>> 2026-10-24 23:56 连过两道检查：arm_debug_set / fast-fault 计数 <<<

- v101（跳过 lowglo 断言）→ 新 panic PON=0xF5：
  解码 = **`arm_debug_set @pcb.c:995`**（`arm_debug_set()` 收到
  非 NULL 但 flavor 非 32/64 的 debug_state）。SWEET 下改为忽略该
  default 分支（调试寄存器 bring-up 非必需）。
- v102 → 新 panic PON=0xF3：用 `panic_strings.py` 批量反汇编
  12 个候选调用点，唯一符合早期启动的是
  **`arm_clear_fast_fault: first pass (N) and second pass (M)
  disagree on updated mappings`**（pmap.c:8168）。
  - 机制：pass1 用 `ARM_PTE_FF_MARKER`（=ARM_PTE_COMPRESSED 位）
    标记更新过的 PTE，pass2 按该位计数并清位；4K PTE 布局下这个
    软件位没被 `write_pte_fast` 保留 → 计数不等 → panic。
  - pass2 的 TLB flush **不依赖**该计数（每个 PTE 都会 flush），
    因此 SWEET 下跳过此 panic 是安全的（v103）。
- **新增工具**：`research/panic_strings.py`（给定多个 panic 调用点
  返回地址，自动用 llvm-objdump 找 x0 并打印 format string），
  比逐个手查快得多。
- **进展判断**：连续三道都是"检查性 panic"，说明内核已在正常
  bring-up 路径上推进（arm_vm_init 内部 → 线程/调度相关），
  不是硬件死锁。

---

## >>> 2026-10-24 23:42 v100 仍 F5：physmap 重设无效；先跳过 lowglo 断言 <<<

- v100（在 `patch_low_glo_static_region()` 调用前无条件重设
  `physmap_base = phystokv(args->topOfKernelData)`、
  `physmap_end = physmap_base + real_phys_size + PADDING`）实测
  PON 仍 **0xF5** → 同一 cold.2 断言（physmap 为 0）再次触发。
  说明：**写入 physmap_base 后，lowmem_vectors.c 读到的仍是 0**
  （SROL/`__security_const_late` 变量在此构建/加载方式下写入不可见，
  或者读写落在不同符号/映射）。这同时解释了之前
  `commpage_ro_data_kva` 断言读到非 0 的异常。
- **v101 处理**：SWEET 下跳过 `patch_low_glo_static_region()` 的
  两个 assertf（vm_first/last、physmap_base/end），让 arm_vm_init
  继续走完；若后续 VM 依赖 lowGlo.lgPhysMapBase/End，会以新 panic
  暴露，再针对性修。
- **仍待解**：SROL 变量写入不可见是全局性风险（commpage、
  physmap、zone 等都可能有同样问题）。若 v101 后继续前进，应安排
  一次专项验证（在写入点与读取点分别写 PON 标记，确认是否同一地址）。

---

## >>> 2026-10-24 23:34 重大更正：真死点是 arm_vm_init 末尾 physmap 断言 <<<

- **方法升级**：panic 通道现在写"调用者地址字节"到 PON
  （`sweet_early_pon_raw((caller>>4)&0xff)`），配合
  `research/panic_caller.py` 扫描 Mach-O 所有 `bl _panic` 调用点，
  再用 `llvm-objdump` 读该调用点的 format string。
  - v97 PON=0xF6、v98/99 PON=0xF5 → 候选函数交集包含
    `patch_low_glo_static_region.cold.2`；
  - 反汇编 + 字符串读取得到 panic 原文：
    **`lowmem_vectors.c:136 Assertion failed: ... Tried setting the Low
    Globals before arm_vm_init()`**；
  - 进一步反汇编确认比较的是 **physmap_base / physmap_end == 0**
    （0xfffffff007175028/30），失败跳 cold.2。
  - **v99 去掉 `debug=0xc` 后仍同样 panic** → 与 debug 无关，
    patch_low_glo_static_region 在 arm_vm_init.c:2509 无条件调用。
- **结论（推翻之前）**：内核实际死在 **arm_vm_init 末尾**，还没到
  serial_init/console_init，更没到 commpage！此前"屏上 STEP 128-135/
  PON=0xAE"是 **面板残留旧帧 + 未清零的陈旧 PON**（DSI command 面板
  会保留最后一帧）。**所有基于该画面的后期判断作废。**
- **v96 的 PON=0x52 也解释了**：它就是通用 assert 格式串
  `%s:%d Assertion failed: %s (0x%llx %s 0x%llx)` 的 24 字符哈希，
  所有 assert 同哈希，不能区分具体断言。
- **v100 修复尝试**：在 arm_vm_init.c:2509 调用前**无条件重设**
  `physmap_base = phystokv(args->topOfKernelData);`
  `physmap_end = physmap_base + real_phys_size + PADDING;`
  （不再使用随机 slide）。若仍 panic，则说明 SROL 变量写入本身
  不可见/页面只读，需要更深层排查。
- **纪律更新**：
  1. 每次实验前必须先清零 PON（bindershell regmap 写 "88e 00"），
     否则读数可能是上一轮残留；
  2. 屏上文字可能是残留帧，**以 PON + 二进制解码为准**；
  3. 新增 `research/panic_caller.py`（调用者字节 → 函数/偏移）。

---

## >>> 2026-10-24 23:19 panic 真凶：lock_ticket.c:330 owner assert <<<

- **新增诊断通道（重要）**：
  - `sweet_ddr_panic()` 里把 panic 调用者地址的 bits[11:4] 写入 PON
    （`sweet_early_pon_raw((caller>>4)&0xff)`）；离线用
    `research/panic_caller.py KERNEL.macho <byte>` 扫描 Mach-O 中所有
    `bl _panic` 调用点并匹配，可精确到"哪个函数+偏移调用了 panic"。
  - 再用 `llvm-objdump -d --start-address=..` 反汇编该调用点，读它
    加载的 format string → 得到 panic 原文。
  - 沿用：panic 时 `sweet_fbcon_panic_text()` 画底部红条；但 **panic
    路径会把显示关掉**，红条拍照基本不可用，PON 才是主通道。
- **v97 实测 PON=0xF6** → 匹配到 `_lck_ticket_lock+0x104`；
  反汇编读出 panic 字符串：
  **`lock_ticket.c:330 Assertion failed: tlock->lck_ticket_owner == 0`**
  （`tlock_mark_owned()` 里，LOCK_STATS 的 advisory owner 字段非 0）。
  该字段只用于调试记录，FIFO ticket 才是权威；属于 4K/early-boot 下
  owner 字段残留脏值的症状。
- **修复（v98）**：SWEET 下跳过该 `assert3u`，仍写入 owner。
  若之后出现同锁死锁（真递归），会表现为挂死而非 panic。
- **背景**：此前 commpage 的 `assert(commpage_ro_data_kva == 0)`、
  RO global 改写、marker 174/190 等全部是"被这个锁 assert 抢先
  打断"的误判；PON 的 last-step 编码会停在 174 是因为 panic 发生时
  `early_step_id` 就是它（锁 assert 发生在 174 附近的锁操作里）。

---

## >>> 2026-10-24 22:42 v92 仍 174；怀疑 PON 写本身或 alias 被 commpage pmap 破坏 <<<

- v92（globals 改普通 static + 赋值拆 180/181/182）后 PON 仍 **0xAE**
  → 连 180 都没到，panic 落在 174 之后、`phystokv(ro_data_pa)` 之前。
- 两个候选：
  1. `sweet_early_step_late(174)` 的 **PON/SPMI 写自身**触发 data abort
     （alias 槽 20 映射被 `pmap_create_commpages` 的页表操作破坏）；
  2. `phystokv(ro_data_pa)` 崩（但 ro_data_pa==data_pa，刚在 174 前
     成功过一次，理论上不该）。
- **v93 判定实验**：在 `vm_address_t ro_kva = phystokv(ro_data_pa);`
  之前插 `step_late(190)`（PON 0x7E / panic 0xBE）：
  - PON=0xAE → 174 的 PON 写本身崩（alias/SPMI 问题）；
  - PON=0xBE → alias 正常，phystokv(ro_data_pa) 崩。
- 若确认 alias 被破坏：需要查 `pmap_create_commpages` 对 kernel L1
  的副作用（4K 下 commpage pmap 的 nest/enter 是否误写共享表）；
  长期可把 alias 移到不会被页表操作触碰的专用窗口。

---

## >>> 2026-10-24 22:35 v91 去断言仍 panic 174；改为可写 static + 拆赋值 <<<

- v91（删掉 `commpage_ro_data_kva == 0` 两处断言）实测 PON 仍 **0xAE**
  → panic 不是断言，而是紧接着那行赋值
  `*kernel_ro_data_addr = commpage_ro_data_kva = phystokv(ro_data_pa);`
  （marker 175 未执行）。
- **v92 同时做两件事**：
  1. `commpage_ro_data_kva` / `commpage_text_kva` 从
     `SECURITY_READ_ONLY_LATE(static ...)` 改为**普通可写 static**
     （若是 __DATA_CONST 只读写故障即可解）；
  2. 把两条赋值拆成三步并加探针：
     `phystokv(ro_data_pa)`→180、写 global→181、写
     `*kernel_ro_data_addr`→182；text 对应 183/184/185。
- **判读**：
  - 到 180 停 → phystokv(ro_data_pa) panic（ro_data_pa 无效）；
  - 到 181 停 → global 写入仍 fault（只读页/段问题在别处）；
  - 到 182/185 停 → 写入目标 `*kernel_ro_data_addr` 无效；
  - 到 175/176 → commpage 这行已过，panic 在后续
    （`commpage_populate` 的写入或 text init）。
- 在等 v92 构建（约 15-18 分钟）。

---

## >>> 2026-10-24 22:30 panic 文本读到：Assertion failed（commpage_ro_data_kva）<<<

- **panic 红条工具生效**：`sweet_fbcon_panic_text()` 把 panic 格式串
  （前 40 字符）画在 fbcon 底部；面板 BGR 让"红底"在照片里呈蓝色，
  旋转后清晰可读：
  **`%s:%d Assertion failed: %s : Calling th...`**
  （XNU assert 的标准格式串）。
- **结合 PON 0xAE（step 174）**：panic 就是
  `assert(commpage_ro_data_kva == 0)`（174 与 175 之间唯一的代码）。
  该 global 只在 `pmap_create_commpages()` 里写入，正常首次调用应为 0
  → 说明 SWEET 加载器对 `__DATA_CONST`（SECURITY_READ_ONLY_LATE）
  的**初值拷贝不可信**，读到非 0。
- **处理（v91）**：删掉 commpage 的两个"防重复初始化"断言
  （`commpage_ro_data_kva == 0`、`commpage_text_kva == 0`），
  后面本来就会立即赋值；后续若其他 SROL 变量也出现"断言读到脏值"，
  按同一思路处理并记录。
- **注意**：v90 起 panic linger 8e9 次循环（~10s），panic 文本可拍照；
  之后 PSCI reset，185s 狗兜底。摄像头已由用户切**手动对焦**，
  **不要再调 /cam/1/af**。
- **v90 时间线**：step 135 之后在 commpage 里停留 ~30s 才 assert
  （page 分配/pmap 嵌套较慢），panic+linger 约在 boot 后 45-65s，
  抓拍窗口要放到 50-65s。

---

## >>> 2026-10-24 22:20 panic 定位到 commpage：step 174（相护 global 写入）<<<

- **console ring 修好后**，内核过 133/134/135；panic 精确落在
  `vm_commpage_init()` 内。
- **细分探针结果（v88/v89，PON=0xAE）**：
  - `pmap_create_commpages` 入口 179、分配 178、分配结果 168+akr
    （akr 位编码）、`pmap_create_options` 170、`pmap_set_commpage` 171、
    第一次 `pmap_enter_addr` 172、`pmap_update_tt3e` 173、
    `phystokv(data_pa)` 174 全部通过；
  - panic 发生在 **174→175 之间**：即
    `assert(commpage_ro_data_kva == 0);` 或
    `commpage_ro_data_kva = phystokv(ro_data_pa);` 这一行。
  - 说明 commpage 页分配、pmap 建立都成功；问题收窄到
    RO 数据 global 的写/断言（SECURITY_READ_ONLY_LATE 段在 4K
    构建下可能被当作只读，写入触发 data abort）。
- **新工具（v89/v90）**：
  - `sweet_fbcon_panic_text()`（pe_serial.c）：panic 时**直写 fbcon
    底部红底白字**，绕开控制台切换/环形缓冲；`sweet_ddr_panic()`
    里调用。
  - `sweet_psci_halt_restart()` 的 linger 从 4e8 提到 **8e9**
    （panic 文本在屏上停留 ~10s，足够拍照；之后 PSCI reset，
    185s 狗兜底）。
- **注意**：panic 后立即 PSCI 复位时屏幕 0.3s 内断电，
  `fbcon` 红条抓不到；v90 起 linger 加长后再抓。
- **下一棒**：v90 烧录后抓 panic 红条（无需 af，用户已手动对焦），
  读出 panic 字符串：
  - 若是 `commpage_ro_data_kva == 0` 断言 → 检查该 global 是否被
    放到只读段/是否被清零前就被写；可临时把 assert 改成打点或
    换成可写普通变量；
  - 若是 data abort（写 RO 段）→ 在 4K/SWEET 下把 SECURITY_READ_ONLY_LATE
    的段保护推迟（或对 commpage 相关 global 用普通 static）。

---

## >>> 2026-10-24 21:45 panic 元凶：console ring（4K 下写满越界/锁路径）<<<

- **区间探针**：`step_late(150)` 插在 133 的 printf 与 134 之间；PON 仍是
  **0x85** → marker 150 没写 → 崩在 `printf("SWEET STEP 133")` 内部。
- **点差**：`printf 132` 正常（屏上可见），`printf 133` 崩；
  两个 printf 之间只有 **console ring** 的写/刷新。console ring 由
  `console_init()`（step 102）用 `kmem_alloc(KERN_CONSOLE_RING_SIZE +
  2 页, KMA_GUARD_FIRST|LAST)` 建立（4K 下 guard/round 语义与 16K
  假设不一致 → 可用区小于 len，写指针走到未映射页就 data abort）。
  输出量确定 → 每次都在同一 step 崩。
- **修复（v86）**：SWEET 下 **跳过 `console_init()`** →
  `is_console_initialized()` 永远 false → `console_write()` 走
  `_cnputs` 直接写 fbcon（早期一直如此，已验证稳定）。
  控制台文字稍慢/无缓冲，但 bring-up 足够。
- **教训**：`printf` 和 `kprintf` 都走 `console_write`；console ring
  在 4K 移植下必须视为不可靠，除非把 guard 大小按 PAGE_SIZE 修正。
- **后续**：若 v86 越过 133，继续读 PON：
  0x46(134)、0x47..0x4C(135..140)；panic 后 0x80|step。

---

## >>> 2026-10-24 21:30 晚期 panic 精确到 step 133→134；PON 后段编码上线 <<<

- **late-step PON（可用）**：`sweet_early_step_late(n)` 把 n(128..191)
  写为 `0x40|((n-64)&0x3f)`：128→0x40、133→0x45、140→0x4C。
  panic 信标 = `0x80|(last&0x3f)`：last=133→**0x85**、last=134→0x86。
  实测 v80/v82/v83 全部 **0x85** → panic 恰好发生在 step 133 之后、
  step 134 之前。
- **排除**：`spllo()`（中断开启）不是元凶——v82 把 spllo 在 SWEET 下
  整段去掉，PON 仍 0x85。`SCHED_HYGIENE_DEBUG` 未定义
  （`ml_spin_debug_clear_self` 不参与）。
- **区间内只剩**：`printf("SWEET STEP 133\n")` 与
  `sweet_fbcon_beacon(133/134)`（角落信标）。v83 把 PON 写调到
  信标之前，PON 仍 0x85 → 说明 step_late(134) 未执行。v84 已把
  所有 fbcon 角落信标删除，只留 PON+printf，一发判定"信标写挂"
  还是"printf 写挂"。
- **显示**：`video_console.c` **只支持 8/16/30/32bpp**，没有 24bpp。
  shim boot args 里 `v_depth=24` 时，panic 后视频控制台画出来是
  "大块图"。已把 shim `ba->Video.v_depth` 改 **0**（强制 serial/fbcon），
  屏上日志恢复可读。
- **拍照**：用户再次强调对焦 → af 后连拍 2-3 帧挑最清晰。
- **弹药**：k0219(late PON)、k0220(nospl)、k0221(reorder)、
  shim-4k-v80..v83；v84 构建中（no beacon）。
- **下一棒**：v84 若 PON 到 0x46(134) 或更高 → 角落信标是元凶
  （fbcon mapping 晚期失效），改用 PON-only 继续；
  若仍 0x85 → `printf("SWEET STEP 133")` 即 console ring/fbcon
  写路径在此时崩，下一步在 printf 前后细分（kprintf vs printf、
  console ring 状态），或直接用 kprintf 代替 printf。

---

## >>> 2026-10-24 21:05 内核已到 EARLY_BOOT/bsd_init 前夜；os_log 全封 <<<

**当前状态：4K 内核已跑过 `kernel_bootstrap_thread` 的 mapping_adjust
（step 127）、kdp_init、PE_init_iokit，进入 `STARTUP_SUB_EARLY_BOOT`
初始化（PRNG/pthread/AMFI 注册都出现），随后在 ~60s 内复位。**

- **os_log 全封（v76）**：`printf()` 与 `kprintf()` 末尾的
  `os_log_with_args()` 在 SWEET 下全部跳过；
  `libkern/os/log.c:_os_log_with_args_internal()` 在 SWEET 下直接
  return。**这是本阶段最大突破**：否则 firehose 初始化后的第一条
  printf/kprintf 就挂死。
- **屏上实锤**：`SWEET STEP 98/99/100/.../127`、`Firehose configured`、
  `OSLog stream configured`、`Telemetry: Sampling ...`、
  panic_trace/KDP、`kdp_core zlib memory`、`iBoot version: sweet-shim`、
  `SWEET PRNG`、`SWEET pthread`、`SWEET AMFI` 全部打出。
- **PON**：v75=0x64(100)、v76=0x7F(127)。step 127 之后没有 beacon。
  v77/v78/PON 无 panic 标记（0x80+），但 ~60s 必复位。
- **复位不是 185s 狗**：shim 已改 `wdt_arm(180,185)`，仍在 ~60s 内
  复位 → 怀疑晚期 panic（PON alias 在 bsd/VM 后期可能已被复用，
  panic 信标写不进）或另一只看门狗。**下一步用 PON 0x40-0x4C 编码
  step 128-140**（`sweet_early_step_late()`），定位最后一步。
- **显示**：进入 EARLY_BOOT 后 fbcon 的输出变成了"大块/旋转"图像
  （视频控制台/缩放接管），仍能看出是文字但难读。已尝试
  `#if !defined(SWEET)` 跳过 `initialize_screen(NULL,kPEAcquireScreen)`；
  屏上仍出现大块图 → 另有控制台接管路径。**角落信标**
  `sweet_fbcon_beacon()`（2x、红底、右上角）已实现，但照片定位难。
- **拍照纪律**：用户再次提醒对焦 → af 后连拍 2-3 帧挑最清晰。
- **弹药**：k0215..k0218；shim-4k-v75..v79；shim 源码 leash 已 180/185；
  待用 cmdline 无 `-enable_kprintf_spam`（更快）。
- **下一棒**：v80（late PON）烧录后读 0x88e：
  - 0x40..0x4C = step 128..140；
  - 0xC0..0xCC = 对应 step panic；
  - 0x7F = 仍停在 127（说明晚期 PON 写入失败）。
  据此决定：若是 PE_init_iokit/ EARLY_BOOT 内挂死，逐项 stub/修；
  若能进 bsd_init，下一步就是 root/UFS 驱动。

---

## >>> 2026-10-24 20:45 os_log 真凶落网：printf **和 kprintf** 都会调它 <<<

- **v75 第一次发射**：`printf.c` 旁路 os_log 后，屏幕出现
  `SWEET STEP 98 → Firehose configured → SWEET STEP 99 →
  OSLog stream configured → SWEET STEP 100 → Telemetry: Sampling ...`
  —— 内核越过了 firehose，跑到 step 100！
- **PON 仍为 0x64(100)**：说明又挂在 `telemetry_init()` 的
  `kprintf("Telemetry: ...")`。查实 **`kprintf` 自己在末尾也调用
  `os_log_with_args()`**（`pexpert/arm/pe_kprintf.c` 的
  `kprintf()` 两个分支）——文字画完就挂。
- **修复（v76）**：
  1. `pe_kprintf.c`：SWEET 下两处 os_log 调用都跳过；
  2. `libkern/os/log.c`：`_os_log_with_args_internal()` 在 SWEET 下
     直接 `return`（彻底封死所有 os_log 路径，bring-up 期）。
- **教训**：`printf` 和 `kprintf` 是两条独立实现（osfmk/kern/printf.c
  与 pexpert/arm/pe_kprintf.c），都要打补丁。
- **shim 红屏偶发**：v74 第一次、v75 第一次出现
  `PANIC/SLOT STAGE=loader`（ELR 每次同为 0x00B8347C）；同一镜像
  `fastboot reboot bootloader` 后重发即正常。判定 fastboot/ABL
  会话抖动，不是内核回归。**重发前必须重新 patch 两条 success。**

---

## >>> 2026-10-24 20:40 死点细化到 printf→os_log：SWEET 下绕过 <<<

- **真 fbcon 已 RGB888**（`FBCON_BPP=3`、逐字节；`mkadt.py` FB_STRIDE=2160、
  probe 校验 `>= width*3`）。屏上能读真实 kprintf 输出（kmem ranges、
  zone_init、firehose、compressor 等），证明 4K 内核的 VM/zalloc 都通了。
- **单调步骤信标**：`sweet_early_step(n)` 把 n（64-127）写 PON 0x88e，
  同时 `printf("SWEET STEP n\n")`；step 98/99 夹住 firehose 的 printf。
- **panic 信标**：`panic()`→`sweet_ddr_panic()` 里调用
  `sweet_early_panic_beacon()` 写 `0x80|(last_step&0x3f)`。
- **v74 实测（PON 0x62=98，无 panic 信标）**：
  - step 98（firehose printf 之前）已落盘且屏上可见 "SWEET STEP 98"；
  - 随后 `printf("Firehose configured: ...")` 的**文字照常画上屏**
    （说明 console 写成功），但 **`vprintf_internal` 末尾的
    `os_log_with_args()` 挂死**（无 panic、~35s 后复位）。
  - 结论：**不是 panic，是 os_log/msgbuf/firehose 写路径卡死**。
- **修复（已编入 v75）**：`osfmk/kern/printf.c` 在 `#if defined(SWEET)`
  下**跳过 `os_log_with_args`**（保留 console/printf 输出）。后续如果
  os_log 仍被其他路径调用导致挂死，再逐点处理。
- **PON 校验史**：v69 PON=0x14(20)、v70=0x1B(27)、v71=0x64("oslog"
  phase hash)、v72=0x1E(30 firehose done)、v73=0x1E、v74=0x62(98)。
  PON 通道在 OSLOG 之前一直可靠。
- **异常事件**：v74 第一次发射出现 **shim 红屏 PANIC / STAGE=loader**
  （ESR 0x96000000 级、FAR 像 ASCII），随后 `xperable` 也报
  `fbusb_bufcmd send failed`；`fastboot reboot bootloader` 刷新会话后
  重发正常。判断为 fastboot 会话/下载抖动，不是内核问题。
- **复位时间**：多次观测到 ~35-45s 复位（不是 65s WDT）→ 更像
  panic/halt 路径或 APSS WDT 默认 bite；本轮 PON 无 panic 标记，
  说明是纯挂死后被狗咬。
- **弹药**：k0209..k0214；shim-4k-v69..v74；`research/phase_hash.py`、
  `patch_step_beacons.py`、`patch_panic_beacon.py`、
  `patch_printf_no_oslog.py`。
- **下一棒**：v75 发射后看 PON 是否越过 99/100；若继续，重点盯
  `console_init`（kmem_alloc）、`kernel_thread_create`/`load_context`、
  `sched_startup`。

---

## >>> 2026-10-24 20:10 4K alias 修复成功：内核跑到 kernel_bootstrap（stage 27）<<<

**当前状态：4K 内核已过 arm_vm_init 全部阶段、serial_init、kprintf 启动，
进入 kernel_bootstrap，在 `vm_mem_bootstrap()` 之后死亡。早期控制台
（stage 1-20）与 PON 标记均已恢复。**

- **alias 修复（k0209/v69）**：`sweet_early_map_final()` 不再用
  `phystokv(l1 & MASK)` 取 L2 表，而是用 **仍在存活的 TTBR0 恒等映射**
  直接访问最终页表：`l1root = (tt_entry_t*)(cpu_ttep & TTBR_BADDR_MASK)`，
  L2 表 = `(tt_entry_t*)(l1e & ARM_TTE_TABLE_MASK)`（VA=PA），全部
  volatile 写入；`sweet_early_relocate()` 切回 alias 指针（FB=base、
  槽 18 IMEM +0xbf65c/+0xbf000、槽 19 durable +0x1ff000/pstore +0x207000、
  槽 20 PON +0x20000）。
- **实锤**：v69 复位后 PON `0x88e=0x14`（stage 20 = kprintf startup complete）
  ——stage 8/9/10/11/12/13/14/15/16/19/20 全部落盘，TTBR0 teardown
  后的 PON 写入正常（alias 槽 20 有效）。
- **屏上竖条纹谜底**：stage 20 后 `sweet_early_disable()`，kprintf 走
  `pexpert/arm/pe_serial.c` 的真实 fbcon；它按 ADT `sweet-fbcon` 的
  stride=2880、32bpp 画，而 lilac 面板是 **RGB888/2160B 每行** →
  满屏斜条纹（不是崩溃乱码）。已改 **RGB888 fbcon**（FBCON_BPP=3、
  逐字节写）+ `mkadt.py` `FB_STRIDE=720*3=2160`，并把 probe 校验改为
  `stride >= width*3`。
- **新 stage 21-27**：21 PE_init_cpu done、22 arm_init main phase done、
  23 machine_startup entered、24 machine_conf done、25 calling
  kernel_bootstrap、26 kernel_bootstrap entered、27 vm_mem_bootstrap done。
  `sweet_early_mark` 现在允许 stage≤60 且在 `early_disabled` 之后仍写
  PON/IMEM/durable（只是不再画字）。
- **v70 实测**：PON `0x88e=0x1B` = **27** → 内核死在
  `vm_mem_bootstrap()` 之后（`kernel_startup_initialize_upto(OSLOG)`、
  sched_init、mac_policy_init、mapping/clock/IPC 初始化这一段）。
  复位后 ~20s 自动回 fastboot（PON 路由仍有效）。
- **下一步（已编入待发）**：`kernel_bootstrap_log()` /
  `kernel_bootstrap_thread_log()` 每条 message 的 **33-hash**
  写入 PON `0x40|(h&0x3f)`（stage 0x40-0x7f，与 stage≤27/panic 0x80+
  区分）。解码器 `research/phase_hash.py`（对 startup.c 里的字符串
  算同一 hash）。复位后进 Android → bindershell → regmap 读 0x88e
  → 反查最后阶段。
- **重要弹药坑**：`/root/xnu9.adt` 是**过期**的（phys_base
  `0x9f800000`）。每次打包必须
  `python3 tools/mkadt.py /root/xnuXX.adt` 重新生成（0xA0000000 /
  mem 0x5af00000）。已用 `/root/xnu12.adt`（stride 2160）。
- **摄像头**：DroidCam `192.168.68.57:4747` 在 20:05 后**掉线**
  （ping 通但 HTTP 000）；当前主遥测 = PON + bindershell regmap。
- **状态机**：v70 已自动回 fastboot → `fastboot reboot` → Android
  起来 → bindershell root 可读 regmap。每个循环约 2 分钟。

---

## >>> 2026-10-24 19:45 情报补充：MAIR 陷阱 / 树差异 / 参照战国 <<<

**测量陷阱（本会话踩过，勿再踩）**
- **内核 MAIR attr0 = 0xFF = Normal Write-Back（可缓存）**：V=P 块描述符
  （0x601，AttrIndx=0）映射的 RAM 是**可缓存**的。我们早期所有 post-MMU
  的 DDR 标记（0xA3001000/0xA3100000 等）都留在 cache 里、复位即丢，
  造成"内核死在 SCTLR"的误判。**post-MMU 的 DDR 遥测必须 dcache clean
  （dc civac + dsb）或走 Device 映射（SWEET 设备槽 AttrIndx=3）。**
- **panic 路径曾经瞬间 PSCI 复位** → 面板立刻熄灭、DroidCam 拍不到；
  现改为 `sweet_early_panic` spin 保屏（狗 60s 兜底），另有 PANG DDR
  镜像双保险。
- **xperable 卡 `Starting test4`** → `fastboot reboot bootloader` 刷新
  会话后可双成功；**必须**两条 `patched successfully` 才发 `-I`。
- 嵌套 heredoc 传 C 代码给 ssh 时，`'\n'` 会变成真换行 → 用 chr(10)
  或本地写脚本再 scp。

**新旧树的差异（重要）**
- Mac 上 dxb 树原本有上一棒的**本地实验**（0114 无条件跳过 SCTLR、
  0x8f=0x04 路由、panic hook、stage 标记）；我们从补丁系列重建的干净树
  **没有**这些——即 SCTLR 现在真正执行，0x8f 路由靠 shim `RET=1`
  （0x06→fastboot）。不要在干净树上找那些实验代码。
- 干净树上 `sweet_early_mark 1..20` 与 `sweet_early_panic` 是**自带**的，
  early console 直接画 FB（720×1280、RGB888、2160B/行）。

**参照/战略情报（2026-10-24 拉取）**
- `ios-sweet-sagit`（小米6，同 MSM8998）已到 **"KERNEL INIT COMPLETE,
  WAITING FOR ROOT"**（README/NOTES 自述），且用 **phys_base=0xB0000000
  + 4K granule + highmem 可选** —— 本 SoC 最强参照，4K 系列补丁源。
- **UniXNU 社区**：MSM8998 的 OnePlus 5（cheeseburger）仅
  "XNU entry bar observed; MMU and root unverified"；sagit 的 UniXNU
  端口状态未记录；**没有任何 Sony/MSM8998 跑通记录**。结论：继续
  sweet/sagit 路线，暂不切 UniXNU（UEFI port 绕路且同样要解 4K/MMU）。
- sagit 也用 `-mcpu=cortex-a53`（Kryo 280=ARMv8.0，无 LSE/PAN/RCpc）。
- shim 侧工具：`BEACON=93` 打 `ID_AA64MMFR0`/MIDR/MPIDR（验证 granule
  用）；`BEACON=91/92` 是 MMU-on 自测桩（16K 假设，4K 下结果仅存档）。

---

## >>> 2026-10-24 19:41 4K 内核跑到 stage 9；别名窗口确诊为空 <<<

**当前状态：4K 内核已过 stage 7 (arm_vm_init)、8 (TTBR0 teardown)、
9 (kernel framebuffer VA live)；早期控制台仍用 identity 指针，stage 9
之后 TTBR0 消失即无字。别名窗口 L2 槽为 0，是下一个必修点。**

- **屏上实测（k0208 / shim-4k-v68）**：
  ```
  SWEET EARLY 1..7（arm_init → entering arm_vm_init）
  SWEET EARLY 8: leaving bootstrap TTBR0
  SWEET EARLY 9: kernel framebuffer VA live
  ALIAS va  = FFFFFE0002000000
  ALIAS L1e = 0000000027A083
  ALIAS L2s = 0000000000000000
  ALIAS slot18=0000000000000000
  ALIAS kvtp= FFFFFFFF00000000
  ```
- **解读**：
  - `SWEET_EARLY_FB_VA = VM_MIN_KERNEL_ADDRESS + 2MB =
    0xFFFFFE0002000000`（本构建 VM_MIN = 0xFFFFFE0000000000）。
  - L1 项存在（0x27A083：低两位 table|valid，AP 位 0x80），但该 VA 对应
    的 L2 槽读取为 0，slot18 也是 0 → **`sweet_early_map_final()`
    写入的表与 relocate 读到的不是同一张**（cpu_tte 项是 PA 还是 KVA、
    `phystokv()` 用法、或两次的 L1 索引有偏差，需核对）。
  - `kvtophys(va)` 打印 0xFFFFFFFF00000000 → 也印证该别名不可翻译。
- **前序修复（本树已落）**：
  - 4K granule 全套（TCR 实测 0x22a51a251a：T0SZ/T1SZ=26、TG0/TG1=4K）；
  - start.s 设备映射改为复用 DRAM 的 L1[2]/L1[3] 表 + L1[0] 新表；
  - `sweet_early_map_final`：缺 L1→L2 则 `alloc_ptpage`；splash 18×2MiB；
    IMEM/durable/SPMI 槽 18/19/20（PA 0x14600000 / 0xFFA00000 / 0x08400000）；
  - `sweet_early_relocate`：SAGIT 分支暂时**保留 identity 指针**
    （0x9d400000 / 0x146bf65c / 0xffbff000 / 0xffc07000 / 0x8420000），
    以便 stage ≤9 全部可读；
  - `sweet_early_panic`：spin 保屏；panic 文本 DDR 镜像 0x9F700000
    （magic 'PANG'），shim 显示 `PANG`。
- **下一棒（按序）**：
  1. 在 `sweet_early_map_final` 里把 `cpu_tte[...]` 写入后**立刻回读
     slot[0]/slot[18]** 打印（用 identity console），与 relocate 的值
     对比，判定是"写错表"还是"读错表"；核对 `cpu_tte` 项是否物理地址、
     `phystokv()` 是否适用于该表。
  2. 修好后把 relocate 切回别名（FB=base、rr/durable/prz/pon 用槽 18/19/20
     偏移），确认 stage 10-15（pmap_bootstrap 等）可读。
  3. 若别名窗口与 physmap/KVA 冲突（va=0xFFFFFE0002000000 在 4K 布局
     已接近内核 physmap 区），换一个空闲窗口（例如 VM_MIN+8MB 或
     从 cpu_tte 的空槽里挑）。
- **构建/发射要点**：Mac `/usr/local/bin/python` 软链；INCREMENTAL 编译
  ~3-5min；install 阶段 tools/san 报错不影响 kernel 产物；xperable 卡
  `Starting test4` 时 `fastboot reboot bootloader` 可恢复。
  嵌套 heredoc 里写 C 代码注意 `'\n'` 会被变成真换行（用 chr(10)）。
- **弹药**：`k0200..k0208-*.macho`；`shim-4k-v60..v68-gzdtb.img`；
  `research/patch_start4k.py`、`patch_armvminit4k.py`、
  `move_alias_diag.py`。

---

## >>> 2026-10-24 19:10 4K 内核跑进 arm_init：early console 全亮，panic 定位 <<<

**当前状态：4K granule 内核已越过 SCTLR，跑完 arm_init 1-6 阶段，在
`arm_vm_init.c:1842` 的 FB 别名断言处 panic；修复已打上、正在重编。**

- **实锤**：`ID_AA64MMFR0_EL1 = 0x1122`（TGran4=1、**TGran16=0**、
  TGran64=0、40-bit PA），MIDR=0x51AF8014，MPIDR=0。beacon 93 实测。
  Kryo 280 只实现 4KiB granule；16K TCR 开 MMU 后第一拍取指必 fault。
- **4K 移植做法（本树）**：不开 SAGIT 机器配置（否则所有
  `ifeq CURRENT_MACHINE_CONFIG,SWEET` 模块掉架），而是给 SWEET 构建
  加 `-DARM64_BOARD_CONFIG_SAGIT` 并让 SWEET 走 4K 链接布局：
  - `makedefs/MakeInc.def`: SWEET flags 加 SAGIT 宏；
    `ARM64_LINK_ADDR=0xfffffff007004000`、`LARGE_MEMORY_DEFINE=`；
  - `SAGIT.h` 的 `#undef __ARM_16K_PG__` + `__ARM64_PMAP_SUBPAGE_L1__`
    经 board_config.h 生效（TCR 实测 0x22a51a251a：T0SZ/T1SZ=26、
    TG0/TG1=4K ✓）；
  - `BOOTSTRAP_TABLE_SIZE=16 页`、start.s 16 页清零；
  - pmap 8MiB 对齐、compressor 4×RAM、iOS 版 zone/VM 尺寸（sagit 0103/
    0104/0109/0111）。
- **4K start.s 设备映射的三个 L1 冲突（血泪）**：
  - 4K 下一张 L2 表只覆盖 1GiB VA 窗口；splash 0x9d400000 与
    DRAM V=P（phys 0xA0000000）同在 L1[2]，durable/pstore 0xFFA00000
    与 DRAM 上半窗口同在 L1[3]。**不能新建表覆盖，必须复用既有表**
    （`ldr xN,[x25,#idx*8]; and mask`）。
  - IMEM 0x14600000 + SPMI 0x08400000 在 L1[0]（空闲）→ 新建一张表。
  - sagit 用 highmem(0x1_00000000) 正是为了把 DRAM 挪到 L1[4]。
- **屏上证据（4k-v64 发射）**：early console 打印
  `SWEET EARLY 1..7`（arm_init entered / PE_init_platform /
  cpu_topology / cpu_bootstrap / kernel_startup_bootstrap / cpu_init /
  entering arm_vm_init），随后
  `SWEET EARLY PANIC: arm_vm_init.c:1842 Assertion failed:
  (l1 & (ARM_TTE_VALID|ARM_TTE_TYPE_TABLE)) == (...)`
  —— `sweet_early_map_final()` 假设 FB 别名的 L1 表已存在（16K 假设）。
- **已修**：
  - `sweet_early_map_final`：缺失则 `alloc_ptpage` 建 L2；splash 18×2MiB；
    IMEM/durable/SPMI 放独立槽 18/19/20；
  - `sweet_early_relocate`：4K 偏移改为槽 18/19/20
    （rr+0xbf65c、durable+0x1ff000、prz+0x207000、pon+0x20000）；
  - `sweet_early_panic` 改为 **spin（不复位）**，让 panic 文本留在屏上
    （快速 PSCI 复位会让面板瞬间熄灭，DroidCam 抓不到）；
  - panic 文本 DDR 镜像：splash 区 **0x9F700000**（magic 'PANG'），
    shim 下一发显示 `PANG`；
  - 构建补丁 `research/patch_start4k.py`、`patch_armvminit4k.py`。
- **构建坑**：
  - Mac 上 `env python` 与 xcode-select 都找 `python` →
    `ln -sf /usr/bin/python3 /usr/local/bin/python`；
  - `make install` 常在 tools/san 阶段 Error 2，但
    `build/xnu.obj/DEVELOPMENT_ARM64_SWEET/kernel.development.sweet`
    已产出，够用；
  - 冷编译 ~20min（删 build/ 后），改动单文件增量 ~3-5min；
  - sagit 0099 的 arch 条件写反（`ifneq`→`ifeq`），否则 arm64e 报
    ptrauth 编译错误。
- **发射纪律（新增）**：xperable 卡在 `Starting test4` 时，
  `fastboot reboot bootloader` 刷新会话后可双成功；**必须**看到两条
  `patched successfully` 才发 `-I`。
- **下一步**：本次重编（含 alias 修复）→ 两发（一发跑、一发读 PANG）。
  若越过 stage 7，关注 stage 8-15（TTBR0 teardown / pmap_bootstrap /
  early I/O 表），以及 4K 下的 KVA/低全局布局。
- **v65 实测（19:17）**：alias 断言已过、内核持续存活并在画屏，但
  **early console 满屏竖条纹** —— `sweet_early_relocate()` 的
  `early_fb = base + 0x1400000` 沿用 16K 偏移，在 4K 别名布局下
  指向的不是 FB 起点（槽 0 已把 PA 0x9d400000 映射在 `base`）。
  **下一棒第一件事：把 `early_fb` 改成 `base`（或与新槽位一致的偏移），
  并核对 early_rr/imem/durable/prz/pon 是否落在槽 18/19/20 的预期
  偏移上；随后再跑 stage 8-15。** 屏幕仍是最快的遥测（panic spin 保屏）；
  PANG 文本在 0x9F700000（magic 'PANG'），shim 可显示。
- **弹药**：`k0200-4k.macho`（首个 4K）、`k0201-4k-b2.macho`、
  `k0203-4k-pang.macho`、`k0204-4k-spin.macho`；镜像
  `shim-4k-v60..v64-gzdtb.img`。

---

## >>> 2026-10-24 18:20 真凶实锤：MSM8998 只有 4 KiB granule（16K 内核必死）<<<

**当前状态：16K granule 内核在 lilac 上必然死在 MMU-on 的第一拍取指。已切 4K 移植。**

- 实机读 `ID_AA64MMFR0_EL1 = 0x0000000000001122`：TGran4=1、
  **TGran16=0**、TGran64=0、PA 40-bit；MIDR=0x51AF8014（Kryo 280），
  MPIDR=0（beacon 93 实测）。
- 与 sagit（小米6，同 MSM8998）`SAGIT.h` 注释逐字吻合：
  **16K `TCR_EL1.TG0` 在该核不可执行，MMU 打开后第一条取指
  translation fault**。QEMU cortex-a53(0x1122) 可复现，cortex-a76 不复现。
- 这解释了：phys_base 对齐修复后页表 desc/JUMP 全对
  （0xA0000601 / 0xA1497488），却仍在 `msr sctlr_el1` 之后既无异常
  dump、又无 panic、被狗咬复位。**历史上所有"10 秒必死/看门狗"
  的最终底层原因就是这个**。
- 修复路线（移植 sagit 4K 系列）：SAGIT 机器配置
  （`#undef __ARM_16K_PG__` + `__ARM64_PMAP_SUBPAGE_L1__`）、
  4K 链接布局 `0xfffffff007004000`、`-mcpu=cortex-a53`、
  `BOOTSTRAP_TABLE_SIZE = 16 pages`、start.s 16 页清零 +
  **每个 1GiB VA 窗口独立 L2 表**（本树：L1[0]=IMEM 0x14600000 +
  SPMI 0x08400000；L1[2]=splash 0x9d400000 18×2MiB；
  L1[3]=durable 0xFFA00000 + pstore 0xFFC00000）、
  pmap 8MiB 对齐、compressor 池 4×RAM、iOS 版 zone/VM 尺寸。
- **陷阱**：sagit 0099 补丁的 arch 条件写反（`ifneq`），会让 SAGIT
  仍以 arm64e 编译并报 `arm64e LR authentication requires ptrauth`；
  必须改成 `ifeq`（SWEET/SAGIT 保持 plain arm64）。
- 本树 sweet 系列已含 `-mcpu=cortex-a53` 与无 LSE 的 CAS 宏
  （commpage 已是 LDAXR/STXR 循环），0105/0106 可省；
  `__ARM_PAN_AVAILABLE__` 仅 Apple 板定义，无需处理。
- **4K 下 shim 的页表探针失效**：`x25 = phys + 0x117C000` 是 16K
  布局；4K 内核的表在别处，屏幕上的 TPB/Tk2.3 读数只作参考。
- **遗留**：4K 内核的 `bootstrap_pagetables` 新偏移需从符号表重算；
  shim 的 DDR 探针地址若与新布局冲突需调整。

---

## >>> 2026-10-24 16:40 KVA 悬案终结：phys_base 必须 32MB 对齐 <<<

**当前状态：KVA 映射已修复，内核越过 SCTLR；新死点 = 早期 C 路径 panic。**

- **真死因**：`tools/mkadt.py` 的 `PHYS_BASE=0x9f800000` **不是 32MB 对齐**。
  arm_vm_init 用 16K-granule L2 块描述符建 KVA，掩码 `0xFFFFFE000000`
  把 PA 的 bit23/24（0x1800000）静默截掉 → desc=**0x9E000601**，
  KVA 取指落到 PA 0x9F497488（RAM 基址 0x9F800000 之下）→ 指令中止。
- **"0x9BE00601/第三笔写入"是照片误读**：放大后实为 **0x9E000601**
  = `(0x9F800000 & 0xFFFFFE000000) | 0x601`，与 R15=0x9F800000、
  DWR/P33B/Tk2.3 全部自洽。**没有第三个写入者。**
- **实验 A（x21，实锤）**：反汇编 0x495154 `mov x21,#0`，0x4950ec 入口
  路径到 0x4956ac 之间无第二次写 x21；实测 **X21=0** →
  嫌疑②（cpu_ttep 覆写 TTBR1）**排除**。
- **实验 B（p3 整页扫描）**：45 项 0x2000000 递增块 + 6 个快照槽，
  无任何异常项（P3X 只在 0x30+ 显示空洞，符合 45 项长度）。
- **修复**：`PHYS_BASE=0xA0000000`、`MEM_SIZE=0x5B000000`
  （顶部保持 0xFB000000，panic-log carve 不变）。修复后实测：
  desc/Tk2.3=**0xA0000601**、R15=0xA0000000、JUMP=0xA1497488。
- **页表基址随内核基址移动**：`x25 = mach_pa + 0x178000 =
  phys_base + 0x117C000`。shim 旧硬编码 0xA097C000 只匹配旧基址；
  v47 起从 payload `ph[7]`(phys_base) 动态算（magic "SWEETXNU" 校验）。
- **新死点**：内核越过 SCTLR 后 **~10-15s panic → PSCI SYSTEM_RESET**
  （30-40s 回 fastboot，不是 65s WDT）。PON 在 MMU 开启后写不进去
  （a2 及以后从未成功）；内嵌 panic log (0xFB000000) 为空；
  Lexc_dump 镜像到 DDR 0xA3000000（magic 0xFEED8000）**未命中**
  → 不是低向量异常，是 C 路径 panic。屏幕 fbcon 尚未初始化。
- **新工具**：
  - loader 跳转前清零 0xA3000000 异常 dump 单元（shim 先读后清）。
  - loader PON 面包屑 e1(拷贝前)/e2(拷贝后)/e3(bootargs 后)。
  - `research/{dis_kva,patch_ab,patch_x15,patch_lexc_ddr}.py`。
- **拍照纪律（用户纠正）**：先等屏加载，再 **af→frame 连打**
  （af 后不要 sleep，否则对焦失效）；不要 burst 代替对焦。
- **下一棒**：给 arm_init/panic 路径加 DDR 标记（V=P 区是 Device
  映射，写入直落、跨复位存活；避开 0xA0000000-0xA2540000 内核镜像区
  与 0x83400000 ramdisk），定位 panic 前最后一步。

---

## >>> 2026-10-24 16:00 KVA desc 三事实矛盾：交接文档已备 <<<

**当前状态：页表三层全有效，但 p3[3]=0x9BE00601 是"第三笔神秘写入"。
完整交接见 `HANDOVER-KVA-DESC.md`（含全部验证过的偏移/假设/下一步实验）。**

- **修正：15:20 的"KVA L1 失踪"结论作废**——探错索引（真槽位 `0x7E0` 非
  `0x7FE`）。实锤：V=P L1[0]✓、V=P L2[0x50]✓、KVA L1[0x7E0]→p3✓、
  KVA L2[3] 有块描述符。**四层表结构全健康，病灶收窄为"描述符值不对"。**
- **v42 快照三事实互斥**（详表见交接文档）：
  - A) 相位修正 cave **执行了**（R15=0x9F800000 落盘为证）
  - B) KVA 循环写入时 **x15=0xA0804000 未修正**（DWR=0xA0000601）
  - C) p3[3] 最终=**0x9BE00601**（既非修正前也非修正后值）
- **已排除**：SWEET 映射（全写 p2）、外层循环回绕（45<2045）、第二份建表
  代码（fb-movk/ttbr1 全局唯一）、表拷贝循环（只读）。**0x495448 写入后
  到 ret(0x495854) 之间无任何表写入指令**。
- **两大主嫌疑**：① 内核没死在 ret——desc=0xA0000601 把 KVA 引到 PA
  0xA1xxxxxx（内核镜像内部），错代码乱跑进 arm_vm_init 改写表后才死；
  ② `0x4956ac cbz x21`——**x21 从未被快照过**，若≠0 则 TTBR1 被
  cpu_ttep 全局（fileoff 0x173928）覆写 → KVA 取指 L0 地址尺寸错 =
  IFSC=0 完美吻合。
- **下一位 agent 先做**：实验A 抓 x21+cpu_ttep 单元格；实验B p3 整页扫描
  看 0x9BE00601 是否孤例；实验D nop 掉 0x4956bc 的 ttbr1 重载。

---

## >>> 2026-10-24 15:20 MMU 案翻盘：V=P 表是好的，KVA L1 项失踪 <<<

**当前状态：死点定位到 TTBR1/KVA 侧——V=P 映射完好，KVA L1 项没写上。**
- **血泪教训——索引算术别脑算**：`VA>>25` 不等于 `(VA&0xFFE000000)>>25` 之外的什么，**就是同一个数**，但手算时把 `0xA0C97488>>25` 写成 `0x506`（实为 `0x50`）这类错位坑了整个下午三发探针。**凡 L1/L2 索引、imm16 编码（movz 的 imm 在 bits[20:5]，字节序别颠倒）、mask 位域，一律用脚本算/反汇编验证，不要心算**。探针 patch 后必须 decode 验证 imm。
- **正确拓扑**（16K granule，x25=bootstrap_pagetables）：page0=V=P L1（x25+0）、page1=KVA L1（x25+0x4000）、page2=V=P L2（x25+0x8000）、page3=KVA L2（x25+0xC000）。V=P 起点 mach header PA `0xA0804000` → **L2 idx 0x50**（覆盖 0xA0000000-0xA1FFFFFF，含内核 text 0xA0C9xxxx 和 lowvec）；填 ~0x2D1 项至 idx 0x321。KVA `0xfffffe00_07xxxxxx` → **L1 idx 0x7FE**（page1+0x3FF0）、**L2 idx 3**（page3+0x18）。
- v26/v27/v28 判决：`V=P L1[0]=0x03`（表描述符有效，byte1=0x40 → 指向 page2）；`V=P L2[0x50]=0x01` **块描述符有效** → V=P 完好，post-SCTLR PA 取指不应死。`KVA L1[0x7FE]=0`、`KVA L2[0x380]=0` → **KVA 映射根本没建** → `ret` 到 `arm_init@KVA` 时取指中止（吻合 v15 EC=0x21 + v17 FAR 在 KVA 区）。
- 疑问未解：KVA 的 `create_l1_table_entry` 在反汇编里明摆着（`str x12,[x10]` @0x495418 → x25+0x7FF0），却没落。v34 探针=整页扫描 page1 找第一个非零项的索引。
- **DroidCam 对焦**：抓图前一次性 `curl .../cam/1/af` 再 `curl .../cam/1/frame.jpg`（等待只对屏加载，af→frame 要连着打）。

---

## >>> 2026-10-24 13:30 充电图标案侦破：0x88f = PON 软重启原因寄存器 <<<

**当前状态：内核死亡路径自动回 Android 已修复（v12 实锤）。**
- **`0x88f` 不是 scratch 寄存器**——是 PM8998 PON soft-reboot 原因寄存器（bits[7:1]=原因码），**XBL 消费后清零**（屏上永远读 0，不是"没写上"）。已知编码：`0x04`=RECOVERY→Android、`0x06`=BOOTLOADER→fastboot。
- 充电图标根因：0117/0118 把 `0x5e/0x5f` 当标记写进 `0x88f` → 原因码 0x2e/0x2f 是垃圾 → XBL 不路由 → **插着 USB 就进 chargemon**。v9-v11 三发全是自己毒死的。0119 把所有 `0x88f` 写改成 `0x04`（武装 Android 路由），Lexc_dump/shim vectors2/hios_return_stub 同样武装 → v12 内核死后**自动回 Android**，再不用按电源键。
- **IMEM `0x146bf000` 页 TZ 加扰**：写可能进得去，**读回来全是乱码**（`0xABADBABE` 等），`/dev/fmem` 也一样 → **E0/E1/E2/AA01 等 IMEM 标记通道整体作废**，所有"E0≠标记=内核没跑"的结论推翻。真·XBL 路由通道是 `0x146bf764`+cookie `0xDEADD00D`@`0x146bfb14`（XBL 侧读，我们读不出）。
- **内核执行实锤**：`88e=EE` 只能由内核 `Lexc_dump`（0115 自建向量）写入，而它能跑的前提是 VBAR 已装 = 内核已走到 a7 之后 → **内核一直在跑**，死因收敛在 VBAR 安装后/SCTLR 使能窗口。loader 跳前最后写 `0x02`→`0x88e`，屏上 EE 证明是 post-jump 覆盖。
- **payload 全链路已验证无误**：`__start=_start=entry_va=0xfffffe0007497488`，JUMP PA `0xA0C97488` 字节级正确（E0/a0 门铃就在入口前 20 条指令内），loader probe1 还做 64-word 比对，跳错不可能。
- 0120：`Lexc_dump` 改报**向量槽号**（x14>>7）到 `0x88e`——v13 实报 **slot0=EL1t sync**。0121 改报 **ESR.EC**——v15 实报 **0x21 = Instruction Abort（同级 EL）**：**实锤死在 `msr SCTLR_EL1`+`isb` 后的第一条取指，V=P 翻译表有罪**。0122 计划报 ESR.IFSC(bits[5:0]) 定位失败级别。
- **WR 自测判决**（v14）：`0x88e` 写 0x5A 读回 0x5A = **真·scratch**；`0x890/0x8b0-0x8b3` 全死。**唯一可用数据通道 = `0x88e` 一个字节**；`0x88f` 只用于路由（0x04/0x06）。
- **DroidCam 对焦优化**：抓图前先 `curl http://192.168.68.57:4747/cam/1/af` 触发自动对焦，等 ~2s 再 `curl .../cam/1/frame.jpg`——字更清。
- **构建提速**（用户要求）：① 标记/机器码改动 → **直接二进制 patch Mach-O**（秒级，本页有实例：0120 就是 k0119 字节替换出的）② 源码改动 → Mac `INCREMENTAL=1 ./build.sh`（已加开关，~4min，跳过 deps/patches/headers/libs，只走 `build_xnu` 的 make install）。③ 全量 `./build.sh` 只在结构变更时用。
- **增量构建命令**：`ssh ciscohe@192.168.68.59 'cd ~/Desktop/ios-xperia-xz1c/dxb && rm -f build/xnu.obj/kernel.development.sweet && env PATH=... MACOS_VERSION=13.5 KERNEL_CONFIG=DEVELOPMENT ARCH_CONFIG=ARM64 MACHINE_CONFIG=SWEET INCREMENTAL=1 ./build.sh'`（删的是检查哨兵 `xnu.obj/kernel.development.sweet`，真产物在 `xnu.obj/DEVELOPMENT_ARM64_SWEET/`）。
- **PON 门铃协议确认无误**（对比 spmi.c）：`MOV64 x12,0x8420000`（apid4 写通道）→ `[x12,#0x10]`=WDATA0 数据 → `[x12]`=CH_CMD=`(reg&0xff)<<4`（OP_WRITE=0）。`spmi_write_apid(4,addr,val)` 同构。注意 msm8998 有 `enable-ahb-bus-workaround`：写前应先读 `ARB_CNFG+0x6000`，否则事务可能坏——`spmi_write_apid` 没做，目前仍能落。
- 死寄存器（写了读不出，勿再用作标记）：`0x889`-`0x88d`。可用：门铃 `0x88e`、路由 `0x88f`。
- 自动化链路全绿：`adb reboot bootloader`→xperable 双 patch→`xperable -I img -1 -c boot -1`→等 ~10s→`curl http://192.168.68.57:4747/cam/1/frame.jpg` 读屏→内核死→WDT→Android 自动回。**全程零人工**。截图统一存 `C:\Users\Cisco He\AppData\Local\Temp\cam-*.jpg`。

---

## >>> 2026-10-24 11:15 战略转折：adb 闭环恢复 + 卡屏案侦破 + 全量重刷 <<<

**当前状态（重要）：Android 已恢复健康，adb 自助闭环成立。**
- 全新官方 `47.1.F.1.105` 已重刷（system/vendor/kernel/userdata/modem 等 20 sins）。
- **Magisk 被 kernel.sin 覆盖没了**——boot 分区现在是**纯官方 boot**。root 通道暂失（bindershell/magisk 需重装）。
- adb：USB 调试已开 + 本机已授权（`adb devices` → `device`）。
- **`adb reboot bootloader` 实测可用**——**以后进 fastboot 全程远程，不再需要人工按键**。
- `ro.crypto.state=encrypted` / `sys.boot_completed=1` / `zygote=running` 全正常。

**卡"Docomo 说明页"悬案——已侦破：**
- 那页**不是 setup 向导**，是 framework 死亡后的静态残影（charge_service/bootanim 层的两帧教学动画），所以怎么点都没反应。
- 根因链：反复异常复位 → userdata FBE 密钥体系损坏 → vold 无法确定加密态 → **`ro.crypto.state` 永远为空** → init.rc 的 `on zygote-start && property:ro.crypto.state=*` 三个门全不满足 → **zygote 从未启动** → framework 整体缺席。
- `fastboot format userdata` **单独不够**（刷完依旧卡），**newflasher 全量重刷才救回**（system 侧也有损坏）。

**USB 描述符失败案（"未知 USB 设备 VID_0000&PID_0002"）：**
- 不是系统/驱动问题，是**数据线坏握手**——换 LG 的线秒恢复（`VID_0FCE&PID_51F4` = MTP+ADB 复合）。
- 症状时好时坏：刷机全程 OK，之后突然连 fastboot 都枚举不出。**线材列入消耗品监控**。

**newflasher 重刷规程（本次验证）：**
- **必须先隔离 `*.ta`（8 个）和 `partition/` 目录** → `flash/_holdback/`。
  `.ta` 写 Trim Area，`master-reset.ta` 等可能清 unlock 标志 → docomo 重锁=砖。固件自带 boot/xfl 相关 sin 此前已删（O_77 exploit + P_114 ABL 天然不受本次重刷影响）。
- 绿灯 flash mode = 音量下+插 USB；答案序列 `printf 'n\na\nn\nn\nn\n'`（driver=n, reboot=android, dumpTA=n, wipe=n）。
- `kernel.sin` = 官方 boot（会覆盖 magisk）。

**adb-force 镜像配方（`research/boot-adb-force-lilac.img`，已验证 adb 免授权上线）：**
- 拿 `boot-magisk-lilac.img` 解包（header@0 + kernel.gz@0x1000 + ramdisk.gz@0xd3f000，page=4096）。
- ramdisk cpio 注入：`/default.prop` 实体文件（原为→system 软链）：`ro.secure=0` `ro.debuggable=1` `ro.adb.secure=0` `persist.sys.usb.config=mtp,adb`；`/adb_keys`=本机公钥（init.rc 自带 `restorecon /adb_keys` 钩子）；init.rc 尾加 `on boot → setprop persist.sys.usb.config mtp,adb` 兜底。
- **cpio newc 对齐陷阱**：name 填充按 `110+nlen` 对齐到 4，不是 `nlen`——差 2 字节会悄悄污染所有文件数据（每文件头插 2 个 \0）。
- 已知缺陷：fastboot boot 的标准镜像（TWRP/insecure/adb-force）**全在 ~40-90s 内死于充电模式**（kernel+adbd 先活后死）——原因未查明，作为观测事实记录；adb-force 窗口内足够写 /data 抢 logcat。

---

## >>> 2026-10-24 06:35 修正：WDT 判决作废——06:04 起三发全是哑弹 <<<

- **事故**：06:04 起打包改用裸 Mach-O（`k.sweet`）作 ramdisk，
  loader `magic_ok` 拒跳（"no SWEETXNU payload"）→ **06:05/06:08
  两发 XNU 一行代码没跑**。ramdisk 必须走
  `tools/mkpayload.py KERNEL ADT OUT --cmdline 'debug=0xc serial=3
  cpus=1 keepsyms=1 -enable_kprintf_spam -noprogress'`（ADT=
  `/root/xnu9.adt`，SWEETXNU magic 开头才收）。
- **作废结论**：曾以为 `wdt_disarm()` 让 XNU 跑过死亡窗——实际那次
  是 loader 拒跳后 shim 走到底 PSHOLD 落充电模式。**APSS WDT 元凶论
  未证实**（"加探针死点提前"仍是弱旁证，需真弹复测）。
- **残留数据**：`0x88e=a7` 系 03:46 真弹（pon98 内核）遗留，仍有效。
- **现状**：手机在 E1F4 充电模式（`VID_0FCE&PID_01F4`，充电 App 自
  熄屏），远程无法退出——**需按电源键回 Android**。
- **待发弹药**：`shim-dog65-v2-gzdtb.img` = 0111 内核（永不 PSHOLD）
  + SWEETXNU payload + loader `wdt_arm(60,65)` 长绳兜底。
- **新 loader 模式**：`wdt_arm(60,65)` 跳前上 65 秒长绳——内核得足
  时跑/panic，挂死也由狗踢回 Android。
- **0111 补丁**：SweetPlatformExpert halt 也走 SYSTEM_RESET——堵死
  "panic→断电→充电模式"无人值守死路（若 TZ 拒收则 wfe→65s 狗兜）。
- **PON 标记通道**：`0x88e`=门铃（a1 entry→a3 common_start→a4
  pre-SCTLR→a6/a7 mid→a2 post-MMU），`0x88a-d`=异常字节，
  `0xffbff000`=POST V=P 铁证，`0xffc07000`=ramoops prz。

## 发射纪律（血泪教训，每次都要过）

1. `xperable -B -U -s 0xf49880 -x -4 -6` → **必须亲眼看到两条
   `patched successfully`**，缺一条都不发弹。
2. `xperable -I <img> -1 -c boot -1` 只发一发；跳没跳看
   `fastboot devices` 空不空，不看 OKAY。
3. 发射前清 PON 基线（`echo 088e:00 > .../regmap/spmi0-00/registers`，
   `count` 先设 1）。
4. 收割走 bindershell root + regmap debugfs（su 不可用）。
5. **终端安全态排序**：Android(熄屏) > fastboot > 充电模式(自熄屏但要按键) > 裸跑挂死。

---

## >>> 2026-10-23 22:20 重大突破：`fastboot boot` 镜像必须是 Image.gz-dtb <<<

- **根因**：ABL LinuxLoader 只接受 QCOM `Image.gz-dtb`（`gzip(Image)` 后
  **追加原始 DTB**）。只给 `Image.gz` 时日志报
  `DTB offset is incorrect, kernel image does not have appended DTB` +
  `Dtb offset goes beyond the kernel size`，然后**不跳转**、继续处理
  fastboot 命令。
- **假信号**：镜像被拒后 USB 也会消失、手机也会掉到充电画面，和
  “shim 执行后复位”无法区分 → 由此产生的“10 秒必死/看门狗/SPMI 在
  shim 里失效/MMU 被杀”等结论**全部作废**。
- **破局**：用 TWRP（`research/twrp-lilac.img`）做对照——同流程第一发就跳。
- **验证**：beacon65（红/绿/蓝闪屏）用新格式 boot 后屏幕出现 shim
  console（可见 `65` 等文字），长时间不重启。
- **工具**：`ios-xperia-xz1c/shim/build.sh` 现在默认产出
  `out/shim-*-gzdtb.img`（`GZ=1` 时先 gzip 再 `cat ../dtb.img`）。
- **教训**：payload 不执行时，第一步永远是用已知能启动的镜像做对照 +
  读 ABL 日志确认是否真的跳转，不要用“USB 消失”当执行证据。

## 当前状态（2026-10-24 中午）

- **Bootloader：已永久解锁**（`unlocked:yes` / `secure:no`），P_114 ABL 完好。
- 系统：Android 8.0.0 `47.1.F.1.105` —— **10-24 上午 newflasher 全量重刷后健康**：
  `sys.boot_completed=1`、`zygote=running`、`ro.crypto.state=encrypted`，
  正常进桌面。**Magisk/root 已随重刷丢失**（boot 回到官方），adb 免授权
  仍有效（固化的 adb_keys），需要 root 时再补 Magisk。bindershell 备胎。
- **adb 自助闭环打通**：`adb reboot bootloader` → ~6s → `BH905ND69G fastboot`，
  测试全程无需人工按键。USB 数据线曾被判坏（VID_0000 描述符失败），
  **换 LG 线恢复**——枚举异常先换线/换口。
- XFL：当前是 **O_77**（exploit 垫片，原 P_114 配套 XFL 已备份可刷回）。
- SD 卡 `/storage/654F-41A6`：4GB 全量备份完好，**全程未被刷写**。
- 项目阶段：XNU 早期引导排障中——**死点 = a7 门铃之后、post-SCTLR 探针之前**。
  0114 抓到 `0x88e=0xEE`（异常向量首次捕获），但自建向量 dump 溢出
  `.align 7` 槽位踩烂了表 → **0115 已重写为 16 槽×8B 紧凑表 +
  0116 把异常 dump 挪进 IMEM `0x146bf680`（DRAM cell 不跨复位）**，
  v8 弹待射。**DroidCam `192.168.68.57:4747` 直读屏幕 = 全自动收割闭环。**
- **关键案情（勿再走弯路）**：之前"表空"证据全部作废——0x889-0x88d 是
  **死寄存器**（写了读不出，只有 0x88e/0x88f 可写）；0x88c/0x88d 的
  L2/KVA dump 槽位选错（读的是本来就不映射的项）。memSize 布局 shim
  与 XNU 一致（offset 24），x24=0 理论已否。

## 红线（不可违反）

1. **永不格式化/擦写物理 SD 卡**（`/storage/654F-41A6`）—— 用户全部数据备份在上面的 `Backup/`。
2. `flash/fw/tadump/`（TA 分区 dump）和 `unlock/xfl-backup-original.img`（原 XFL）**不得删除**，是救砖材料。
3. 破坏性刷写（userdata、分区覆写）前必须向用户确认，并明确区分：**内部 userdata ≠ SD 卡**。
4. 不删除 `Backup/`、`research/`、`unlock/` 下任何产物。
5. Windows + Git Bash 环境：`adb shell` 里所有以 `/` 开头的路径会被 MSYS 转义成 Windows 路径 —— **必须 `export MSYS_NO_PATHCONV=1`** 或用绝对 Windows 路径。

## 设备档案

| 项 | 值 |
|---|---|
| 型号 | Sony Xperia XZ1 Compact，Docomo 日版 |
| 代号 | `lilac` / `SO-02K`（fastboot product: `SO-02K`） |
| 序列号 | `BH905ND69G`（adb/fastboot `-s` 指定，旁边还有个 `emulator-5556` 干扰源） |
| SoC | 骁龙 835（MSM8998），Kryo 280：Silver `cpu@0-3` + Gold `cpu@100-103` |
| RAM | 4GB；存储 **UFS**（SKhynix H28U62301AMR，`ufshc@0x1da4000` + QMP v3 PHY） |
| 屏幕 | 4.6" **720×1280** `somc,5_panel`，**DSI command 模式**（注意：非 video mode） |
| 触摸 | Synaptics ClearPad `synaptics,clearpad@0x2c`，I²C 控制器 `i2c@c179000`（BLSP/i2c-msm-v2） |
| 按键 | PM8998 PON（`power-on@800`）：电源 linux,code `0x74`，音量 `0x73` |
| PMIC/供电 | `qcom,spmi@0x800f000` pmic-arb，**老 RPM**（`rpm-msg-ram@0x200000`）非 RPMh |
| GIC/Timer | GICv3 `@0x17a00000`（ITS `@0x17a20000`）；armv8-timer 19.2MHz |
| Framebuffer | `splash_region@0x9d400000` 大小 `0x2400000`（ABL continuous splash） |
| 原系统 | Android 9 `47.2.B.5.38`，bootloader `1306-5035_X_Boot_MSM8998_LA2.0_P_114` |
| 解锁前 | `unlocked:no` / `secure:yes` / **`oem_unlock_allowed:0`**（官方解锁对日版关闭） |

## 工作目录布局（`C:\Users\Cisco He\Desktop\Git\Patch\Xperia`）

| 路径 | 内容 |
|---|---|
| `unlock/` | `xperable.exe`、`LinuxLoader-p114.pe`、`bindershell`、`xfl-o77.mbn`、`xfl-backup-original.img`（31,457,280 B，dd 出的原 xfl 分区） |
| `flash/fw/` | Newflasher v58 + 解包的 `SO-02K_47.1.F.1.105` 固件；`tadump/tadump_{1,2}.ta`；`flash_session/` 为临时产物 |
| `research/` | iOS 移植研究：`ios-sweet/`、`ios-topaz/`、`ios-v60/`、`UniXNU/`（均为浅克隆）；`lilac-stock.dtb`（从 kernel.sin 提取的原版设备树，478,430 B） |
| 备份副本 | `backup_apps.ab`（147MB 未加密 adb backup）、`contacts.vcf`、`contacts_raw.txt`、`sms_ie.apk` |
| 其他 | `make_vcf.py`（联系人转换）、`zadig.exe`、各 zip 原始包 |

工具路径：`C:\Users\Cisco He\Desktop\Git\SchoolBox\Tools\platform-tools_ADB\` 下的 `adb.exe` / `fastboot.exe`。

## 已验证的操作手册

**三种 USB 模式**：
- Flash Mode（**绿灯**，VID_0FCE&PID_B00B）：关机 → **按住音量下 + 插 USB**。驱动 = GordonGate（`SOMC Flash Device`，由 newflasher 生成的 MSI 安装，需 UAC）。
- Fastboot（**蓝灯**，VID_0FCE&PID_0DDE）：关机 → **按住音量上 + 插 USB**。
- ADB：需系统里开 USB 调试 + 授权。

**Newflasher 刷机**（`flash/fw/` 下运行，交互输入 `n\na\nn\nn`）：驱动=n，结束重启=a(android)，dump TA=n（已 dump 过），清 userdata=n。**若不想动 bootloader 先删 `boot/` 目录**（本次靠此保住 P_114；固件自带的是 O_82）。

**临时 root（仅 Android 8）**：
```
adb push unlock/bindershell /data/local/tmp/ && chmod 755
{ sleep 4; printf '<命令>\nexit\n'; } | adb shell /data/local/tmp/bindershell
```
拿到 `uid=0(root)` + SELinux permissive（CVE-2019-2215）。注意此 toybox 无 `head -c`，读固定字节用 `dd bs=<N> count=1`。

**xperable 解锁**（蓝灯 fastboot 下，`unlock/` 目录，需 XFL=O_77 已写入）：
```
xperable.exe -v -V -B -U -s 0xf49880 -4     # → 期望 "LinuxLoader @ 0x... patched successfully"
xperable.exe -c "oem unlock Y" -1 -c reboot -1
fastboot getvar unlocked                    # → unlocked:yes
```
解锁后若卡在开锁警告循环重启：`fastboot format userdata`（本次实测修复）。

**写 XFL**（bindershell root 下）：`dd if=/data/local/tmp/xfl-o77.mbn of=/dev/block/bootdevice/by-name/xfl`（分区 30MB=`/dev/block/sda25`）。恢复原 XFL：`dd if=xfl-backup-original.img of=.../xfl` 或解锁后 `fastboot flash xfl`。

## iOS 移植决策（2026-10-20 定案）

- **路线：ios-sweet**（ABL→shim→XNU）。UniXNU 需要先做 UEFI port，绕路且手机侧无先例。
- **版本：iOS 16.7.16 (20H392)**，iPhone X userland —— PAC 锁死 16.x 上限（XS 起 arm64e）。
- **远期课题（iOS 17+）**：SD835 无 PAC 硅，arm64e 用户态不能原生执行；理论路径 = ① 内核 trap 模拟 retab/braa（热点指令，性能不可行）② 离线 PAC 剥离 dyld_shared_cache（机械改写 pacia→nop/retab→ret，巨大但纯机械）③ 换 SD855+ 自带 FEAT_PAuth 的芯片；且 iOS 17 需配套 xnu-10002+ ABI 移植。公开成果中无完成品（QEMU 模拟 PAC 除外，太慢）。列为 v2.0 研究课题，不影响当前 16.7.16 路线。
- **设备身份两步走**：先 `iPhone10,3/D22AP`（继承上游验证过的配置点亮 GUI）→ 再实验 `iPhone10,1/D20AP`（750×1334 ≈ 720×1280 面板绝配；XZ1c 相机键可映射 Home）。

## iOS 移植评估结论（详见研究报告）

- 硬指标**全过**：解锁 ✅、ARM64 ✅、UFS ✅、4GB ✅、splash FB ✅。
- 官网兼容性分级属 **"Hard" 档**：RPM 供电（非 RPMh）、BLSP I²C（非 GENI）、ARMv8.0 老架构。
- 关键继承：`ios-topaz` 刚解决 ARMv8.0 的 LSE/ldapr 用户态模拟（patch 0081）+ Silver 核 16K 页缺失问题；`ios-sweet` 提供 84 个基础补丁 + shim + mkadt。
- 独特优势：lilac 是 Sony Open Devices 官方 AOSP 设备（内核源码/设备树公开）；720×1280 ≈ iPhone 8 的 750×1334；救砖链路已验证。
- **最大未知数**：cmd-mode 面板的 MDP 自动刷新（FB 写入可能不会上屏）、`fastboot boot` 是否被索尼 ABL 支持、Silver 核 TGran16。

## 构建环境（更新 2026-10-24）

**三台机器分工**：Mac = XNU 内核构建，WSL2 = shim/payload/镜像车间，Windows = 刷写台（xperable/adb/fastboot）。

### Mac 构建机（Hackintosh，ssh 可达）

- **地址：`ciscohe@192.168.68.59`**（局域网；`.191.x`/`.137.x` 的旧别名已废）
- 硬件：AMD Ryzen 5825U Hackintosh，Xcode 26.3，KDK `13.5_22G74` 已装 `/Library/Developer/KDKs/`
- 源码树：`~/Desktop/ios-xperia-xz1c/dxb/`（dxb = darwin-xnu-build 包装器，xnu 源码在 `dxb/xnu/`）
- **构建命令**（必须 env 传参跳过 gum TTY 菜单）：
  ```bash
  ssh ciscohe@192.168.68.59
  cd ~/Desktop/ios-xperia-xz1c/dxb
  rm -f build/xnu.obj/kernel.development.sweet   # 触发重编（否则报 already built）
  env PATH=/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin \
      MACOS_VERSION=13.5 KERNEL_CONFIG=DEVELOPMENT \
      ARCH_CONFIG=ARM64 MACHINE_CONFIG=SWEET ./build.sh
  ```
  增量约 5-10 分钟，只重编改动的文件。产物：
  `build/xnu.obj/DEVELOPMENT_ARM64_SWEET/kernel.development.sweet`
- **补丁流**：start.s 改动直接 scp python 补丁脚本上 Mac `python3` 跑，
  或本地改好后 scp 整文件覆盖。**改完必须删 kernel.development.sweet 再 build.sh。**
- 注意：`build.sh` 会重建 PATH（/usr/local/bin 幸存）、幂等防御点：
  `xnu/bsd/pthread/sweet_libpthread`、`xnu/bsd/sweet_hfs`、`xnu/libkern/crypto/sweet_bearssl`

### WSL2 `Ubuntu-22.04`（root）= shim/payload/打包车间

- Windows 工作目录在 WSL 内是 `/mnt/c/Users/Cisco He/Desktop/Git/`
- 已装：`aarch64-linux-gnu-gcc`、`mkbootimg`（`/opt/mkbootimg`）、`img2simg`/`simg2img`、`hfsprogs`、`apfs-fuse`、`rsync`、`dtc`、`gh`、`python3`、`cpio`
- **ADT**：`/root/xnu9.adt`（mkpayload 必需，本机唯一副本）
- **payload 打包**：
  ```bash
  python3 tools/mkpayload.py KERNEL /root/xnu9.adt OUT --cmdline \
    'debug=0xc serial=3 cpus=1 keepsyms=1 -enable_kprintf_spam -noprogress'
  ```
  loader 只认 SWEETXNU magic，裸 Mach-O 会被拒（历史哑弹事故）
- **shim 镜像**：`cd shim && cp <payload> /root/payload.bin && GZ=1 bash build.sh /root/payload.bin`
  → `out/shim-boot-gzdtb.img`（gzip+追加 dtb.img 是 ABL 唯一认的格式）

### Windows 刷写台

- 工具：`C:\Users\Cisco He\Desktop\Git\SchoolBox\Tools\platform-tools_ADB\`（adb.exe/fastboot.exe）
- xperable 在 `Patch/Xperia/unlock/`；发射镜像统一放 `Patch/Xperia/` 根目录（相对路径踩过坑）
- **完整发射序列**（Android→自动 fastboot→patch→boot）：
  ```bash
  adb -s BH905ND69G reboot bootloader && sleep 8
  cd unlock && ./xperable.exe -B -U -s 0xf49880 -x -4 -6   # 两 patch 必见 success
  ./xperable.exe -I ../<image>-gzdtb.img -1 -c boot -1
  ```

### 收割通道（不依赖 root）

- **DroidCam 屏幕直读（全自动，2026-10-24 新增）**：`http://192.168.68.57:4747/cam/1/frame.jpg`
  —— `curl -o /tmp/cam.jpg <URL>` 下载截图直接读屏，无需用户拍照。
  注意：发射后**等 ~5s** 再截图（shim 屏要加载）；屏是蓝底白字横屏。
- **屏幕回读**：shim 开机先把上一轮 PON 门铃（0x88e/0x88f）+ PST1/POST + 0114 起的
  `VEC/ESR/FAR/ELR`（IMEM cell `0x146bf680` 异常 dump，SRAM 跨热复位存活）打印在屏上 → 拍照即收割
- **PON 可写 spare 只有 `0x88e` 和 `0x88f`**；`0x889-0x88d` 写了读不出（全是死寄存器，
  早期 dump 数据全部作废过一轮）
- Android 侧备胎：bindershell root + `/sys/kernel/debug/regmap/spmi*` 读 PON

## 拦路狗档案 —— 已破案（2026-10-23，见 HANDOVER-2026-10-23.md）

**根因**：ABL 的 LinuxLoader 只接受 **QCOM `Image.gz-dtb`**（`gzip(Image)` 后**追加原始 DTB**，DTB 不压进 gz）。格式不对时 ABL 不跳转也不报错，设备悬停至复位——**"~10s 死亡"全是假象，我们的代码从未被执行过**。此前"WDT/PMIC/SPMI/EL2/MMU 杀手"清单全部作废。

**正确流程（每会话）**：
```bash
cd unlock && ./xperable.exe -B -U -s 0xf49880 -x -4 -6   # ABL 运行时补丁，每次复位后必重打
./xperable.exe -I <image-gzdtb.img> -1 -c boot -1         # 只发一发！~8-9s 后才跳转
```
**只发一发 boot**；第二发会崩 ABL 掉电（历史误判来源）。
**⚠️ 发射硬条件（2026-10-24 02:38 实锤）**：补丁 stdout 必须亲眼确认
`patched successfully`（grep 计数>0）才准发射。补丁下载中途 stall
（只有 `Starting test4` 无 patched 行）时**发射未打补丁的弹 = ABL 拒收
→ 直接掉 E1F4 充电模式**。宁可重打补丁或 `fastboot reboot bootloader`
刷新会话，也绝不带着未确认的补丁状态发 `-I ... boot`。

**构建**：`cd shim && GZ=1 BEACON=N bash build.sh /root/stock-ramdisk.img` → `out/shim-beacon-gzdtb.img`。

**已验证能力**：shim 稳定执行（beacon65-78）；shim 内 SPMI 正常（PON 0x88F 写 reason=BOOTLOADER(3)<<1=0x06 → 复位自动回 fastboot，**无人值守循环成立**）；UFS 写仍不落地；屏幕 RGB565 但 ABL EBS 后无新帧（显示通道暂废）。

<details>
<summary>旧版排除清单（历史存档，全部作废勿读）</summary>

`fastboot boot` 自定义 shim **必在 ~10s 落充电图标**；stock/TWRP 同路径存活（TWRP 实测活着 >20min）。以下为 lilac 上已确认存在的看门狗清单与排除/嫌疑状态：

| 狗 | 地址/寄存器 | 超时 | 状态 |
|---|---|---|---|
| **APSS WDT**（`qcom,wdt@17817000` msm-watchdog） | RST=+0x04(写1 pet) EN=+0x08 STS=+0x0C BARK=+0x10 BITE=+0x14，tick=32765Hz | DTB bark=12000ms **pet=10000ms** | ⚠️ 头号嫌疑：pet/disarm 实验均失败，但寄存器读写正确性未验证（pure9 待发） |
| **PMIC PON_WD**（PM8998 `power-on@800`） | 走 SPMI，不可裸 MMIO；`QPNP_PON_WD_RST_S2_CTL2=+0x57` bit7=EN | **S1_TIMER_MAX=10256ms ≈ 10s** | ⚠️ 与处决窗口吻合度最高；PMIC 级复位=断电=充电图标。但 Linux 正常启动并不关它（全树仅 ramdump 路径调用 `qpnp_pon_wd_config(0)`） |
| TZ/Secure WDT | SCM `0x02000107` arg=1 disarm（CAF `SCM_SVC_BOOT/SEC_WDOG_DIS`） | — | ❌ smc5 调用后照死，已排除或调用未生效 |
| Synaptics 触摸狗 | DTB `watchdog_enabled=1, delay=3000` | 3s | ❌ 外设内部狗，不关 CPU 事 |

**已排除的死亡假设**：MMU/cache（pure8）、EL2 入口（pure4）、PSCI 全核签到（pure6）、SMCCC keepalive（pure3）、镜像 ARM\x64 头（本来就有）、FB 画屏越界（0x9d400000 合法 720x1280 已验证）、fastboot 闲置超时（空闲无限活）。

**充电图标新解读（重要）**：不一定是"被谋杀"——可能是 **复位后 XBL/PMIC 对异常 restart_reason 的默认落点**。处决与落地是两件事，死亡时刻需要心跳探针实测（pure9）。

**IMEM 分区图（stock DTB 实测）**：`qcom,msm-imem@146bf000` 4KB 页内含 `tz-log@146bf720`（3KB）。**写标记只能用 <0x720 的区域**；v7 的 0x146bf700/780/800 撞进了 tz-log —— 后续标记一律用 `0x146bf000-0x146bf71f`。

**fastboot boot 怪癖**：同一 fastboot 会话内，第一发 `boot` 常 `OKAY` 但滞留 fastboot（蓝灯未跳），**第二发才真执行**（usb_read failed）。判断真执行看 USB 断连，不看 OKAY。

## 下一步（按序）

1. [ ] 验证 `fastboot boot` 可用（蓝灯后 `fastboot boot <任意img>` 看命令是否被接受）
2. [ ] 活设备树补采：开 USB 调试 → bindershell → `/sys/firmware/fdt` + `/proc/iomem`
3. [ ] 克隆 `sonyxperiadev/kernel` yoshino 分支，查 clearpad 协议/面板 cmd-mode 细节
4. [ ] fork ios-sweet → mkadt.py 填 lilac 地址 → shim framebuffer 打印 = 里程碑 1
5. [ ] （可选）恢复原 XFL / 刷回 Android 9（解锁状态永久保留）

## 变更日志

- **2026-10-24 中午**：**Android 复活 + 异常向量首次捕获 + 向量表 bug 修复。**
  - **系统修复**：FBE 密钥体系损坏（`ro.crypto.state` 空 → zygote 死 →
    "Docomo 使用说明"其实是 chargemon 残影，不是向导）→ `fastboot format
    userdata` 无效 → **newflasher 全量重刷 20 个 .sin 修好**（.ta 和
    partition/ 已隔离防锁）。adb 免授权闭环：`adb reboot bootloader` 全自动。
  - **排障进展**：0112 三探针（PST1/POST/a2）全灭 + `0x88e=A7` → 死点在
    a7→SCTLR。0113 关 MMU 实验跑出 **`0x88e=0xEE`——异常向量首次捕获**，
    证明死亡是真异常非无声挂死。0114 给 EL1h 向量加 FAR/ELR dump 时
    **`.align 7`（128B）槽位被塞爆** → 向量偏移整体错位、表被自己踩烂。
    **0115 修复**：16 槽全部 `mov x14,#off; b Lexc_dump`（8B），共享 dump
    例程写 VEC/ESR/FAR/ELR → DRAM `0xA1D3F000`，shim 屏直接显示。
  - **PON 死寄存器实锤**：`0x889-0x88d` 写不进读不出，所有指向它们的
    dump 数据一律作废；可写 spare 只有 `0x88e`/`0x88f`。
  - **v7 弹药**：`shim-dog65-v7-gzdtb.img`（0115 内核 + 屏显收割 + 125s 狗绳）。

- **2026-10-24 01:xx**：**xperable `-r/-w` 仅限 DDR——碰 MMIO 会崩 ABL（两次实锤）。**
  第二次 fastboot 会话补丁成功（`LinuxLoader @ 0x98db9000` +
  `VerifiedBootDxe @ 0x9b2f0000` patched）后，一条 `-a 0x800f000 -r`（SPMI
  仲裁器探活）立刻 pipe error → 设备复位 → E1F4 充电。此前想走
  `xperable -w 0x84200xx` 从 fastboot 直接 arm PON reason 的方案**作废**；
  arm-pon-fastboot.sh 已停用（改 header 说明）。PON arm 唯一可行路径 =
  shim 内 SPMI（RET=1，`spmi_write_apid(4,0x88f,0x06)`）。
  **fastboot 会话防逃逸只能靠：缩短窗口 + 周期性无害 fastboot 命令保活**
  （ABL 空闲超时会把设备踢出 fastboot）。
  **E1F4 充电模式恢复**：无软件通道，必须按键 —— 插线状态下 **音量上+电源
  长按 ~10s** 强制重启，重启中保持音量上 → 直接回 fastboot 蓝灯；
  或松手进 Android 再 `adb reboot bootloader`。
  **IMEM mortem 取证协议**（shim loader.c 已内置，只欠收割脚本）：
  pre-jump 全态 dump `0x146bf700..77f`（magic `0x77667374`）、
  vectors2 异常 dump `0x146bf780`、XNULDRST 返回记录 `0x146bf800`、
  early stage `0x146bf200/300/400/500`、restart_reason `0x146bf65c`。
  收割：`unattend/harvest.sh` = patch → **单字** xperable -r 各槽
  （禁块读！Android 侧 fmem 块读 IMEM 会 panic，TZ/XPU 保护区）→ `xperable -l`。
  **DDR 复位即死（xnu5/AGENTS 双证）**：early_durable `0xffbff000`、
  prz `0xffc07000`、console prz `0xffcc0000`、`/pram` `0xfaff0000`、
  `sweet_log_ring` 等所有 RAM 通道 = **复位后取证无效**；之前读到的
  "内核代码"实为新一轮 ABL/XBL 映像残留。panic 全文本暂时只能看屏幕，
  标记级死因走 IMEM。
  **>>> 2026-10-24 02:30 标记通道终局定论（实测全家桶）<<<**
  - **IMEM 全灭**：`0x146bf65c`/`0x764`/`0xb14` NS 可写（shim/Android
    fmem 写后回读正确）但 **TZ 在复位路径重写整段 tz-log 区**
    （0x146bf720+ 属 TZ），任何 NS 标记不活到下个 XBL。
    `0x146bf764` 是 20B 记录体（reason+元数据），`0x146bfb14=cookie
    0xDEADD00D`；XBL 只在 cookie 有效时把 `0x764` 搬到 `0x146bfaa4`
    （XResetReasonDxe 读槽）。**写入者 = TZ 复位 teardown，不是 NS。**
  - **PSCI SYSTEM_RESET 从 shim/ABL 上下文被 TZ 拒绝**（HANDOVER 旧
    笔记已记，又忘了一次）→ 落到 wfe → WDT 咬 → crash 复位 →
    `warmboot=0x52d0`。所以 shim 侧做不出"干净 warm reset"。
  - **0x52d0 = "CRASH W/O DUMPER"** = GCC 硬件原因码回退（record
    无效时 XBL 用 `0x161020&0x3ff`）。XNU 死→WDT/crash→恒 52d0。
  - **`warmboot=` 只回显 enum 类**（5500=FASTBOOT/5501-02=NORMAL/
    5503+≈enum10），不透传任意值；`adb/fastboot reboot` 命令路径
    会规范化覆盖。
  - **★ 真通道 = PON spare 寄存器（PMIC 侧，跨一切复位存活）**：
    `0x88e` 实测 **warm reset（sysrq b）和 crash panic（sysrq c）
    都存活**；`0x88f`=SOFT_RB_SPARE 路由字节（reason<<1，RET=1
    机制）。XNU/shim 标记 → SPMI 写 `0x88e` → 收标在 Android：
    ```
    echo 0x88e > /sys/kernel/debug/regmap/spmi0-00/address
    cat /sys/kernel/debug/regmap/spmi0-00/data
    写：echo "88e <hex>" > .../registers   # ASCII "addr val" 对
    ```
    （regmap debugfs 全程无需内核模块；`data` 二进制写会 EINVAL，
    用 `registers` 文本接口。）
  - **自动 panic 提取定案**：XNU stage mark → `0x88e`（1B=128
    标记空间）→ 任意死法存活 → 下个 Android 启动 regmap 直读。
    warmboot= 保留做粗粒度路由旁证。
  - **屏幕叠字病灶定位（02:5x 修正）**：~3 次横重复的旧"2160 列
    GRAM 窗口"假设被 Android fb0 实测**推翻**：`fb0/bits_per_pixel
    =32`、`fb0/stride=2944`、`virtual_size=720,2560` —— MDP 实际
    吃 **XRGB8888 @ 2944B/行**（720px+64B 补齐 128B）。我们写
    RGB565@1440B → 每显示行吞 ~2 行 → 横向重复+纵向压缩。修法 =
    console/XNU fbcon 全改 **32bpp @ stride 2944**（像素 720 不变，
    行内 16px 尾垫）。待 BEACON=80v2 照片终验（图案按 32bpp/2944
    写的，整齐网格=定案）。
  - **xperable -r/-w 仅限 DDR**（IMEM/MMIО 崩 ABL —— 两次充电
    模式事故实锤）；探活只看 patch stdout。
  - **>>> 02:47 PON 通道端到端实锤 <<<**：清 `0x88e=0` → patch →
    发 XNU 弹 → 落 Android → regmap 读回 **`0x88e=0x02`** = loader
    最后标记 `0x77665502` 低字节（D010..D018 全过 + 已跳 XNU）。
    **自动 panic 提取 shim 半段闭环完成**；收割脚本 =
    `unattend/harvest-pon.sh`（adb+bindershell+regmap，已实测：
    读 0x88e/0x88f + warmboot + 解码 + 归档 out/pon-*/）。
  - **0095 XNU 侧 PON 补丁已备**（`xnu/patches/0095-lilac-pon-
    stage-marks.patch`，待 Mac 构建）：start.s 第 4 slot 映
    `0x8000000` block（SPMI arbiter 写通道），arm_vm_init 加
    `slot[3]`，sweet_early.c `early_pon_write()` 裸写
    `0x8420000`（apid4 WDATA0/CMD/STATUS 轮询）。**PON 编码**：
    `0x00-0x7f`=stage 低字节，`0x80|stage`=panic（与 stage 区分）。
  - **BEACON=80 v2**（02:53 发射持屏中）：32bpp XRGB8888 @2944
    写的上半绿/品红横带+下半黑/白竖条。照片判读：整齐网格=
    32bpp/2944 定案；斜切=stride 偏；仍重复=bpp 还不对。

  **不进充电模式的零探测安全循环**（2026-10-24 定型，勿再用内存探测）：
  `xperable patch → xperable -I <img> -1 -c boot -1 → 等 ~54s 自动回
  fastboot（shim RET=1 PON）→ fastboot reboot → Android → cmdline
  warmboot=`。**xperable 探活只用 stdout 验证补丁（"patched
  successfully"），绝不做 -r/-w 探针**；IMEM/MMIО 读取一律走 Android
  root（fmem 单字 / bindershell），不走 xperable。fastboot 会话唯一威胁
  = ABL 空闲超时 → 流程要快或周期 getvar 保活。

- **2026-10-24 00:10**：**panic-hang 根因定位 + 0094 PSCI halt_restart。**
  首射 16bpp 内核后设备没回 fastboot：XNU 晚期 panic 走
  `panic()->halt_all_cpus(TRUE)->PEHaltRestart(kPEPanicRestartCPU)`，
  但 **ARM 平台无人注册 `PE_halt_restart`**（`IOPlatformExpert.cpp:336`
  默认 NULL）→ `haltRestart` 返回 -1 → CPU 进 wfe 死循环 = 黑屏 hang，
  不再 reset，RET 无法接管。期间设备停在 `VID_0FCE&PID_E1F4`
  (USB mass-storage 描述符, XBL 层残留/救援模式)。
  **0094-sweet-arm-psci-halt-restart.patch**：`SweetPlatformExpert::start`
  里 `PE_halt_restart = sweet_psci_halt_restart`（kPEHaltCPU→
  SMC 0x84000008 SYSTEM_OFF；其余→SMC 0x84000009 SYSTEM_RESET）
  —— 所有 panic/halt 经 PSCI 退出 → shim 预置的 PON reason 接管路由。
  kernel.development.sweet.16bpp-psci (17,076,504B) 已编入验证。
  另：stage 脚本幂等防御已修（target 存在则 `sys.exit(0)` 跳过而非
  报错）；deps 启动须显式 `env PATH=/usr/local/bin:...`（install_deps
  跑在 PATH 重建之前）；`MACOS_VERSION/KERNEL_CONFIG/ARCH_CONFIG/
  MACHINE_CONFIG` env 传参跳过 gum TTY 菜单。
- **2026-10-24 凌晨**：**XNU 首次在 lilac 真机运行。**
  `xperable -B -U -s 0xf49880 -x -4 -6` 补丁后 boot `shim-xnu-16bpp-gzdtb.img`
  → shim→loader→XNU 全链通，屏幕出 early console 文本（叠字），
  ramoops `0xffc07000` 读到 `DBGC` 标记（`early_stage_write` 实锤执行）。
  叠字根因：XNU early console 硬编码 32bpp（`EARLY_STRIDE` 按 dword
  计），lilac 面板 RGB565/16bpp/1440B stride。**修复 = `0093-lilac-
  fbcon-16bpp.patch`**（`uint16_t`+`0xffff`+word stride；Stage-0 asm
  band 已被 0092 禁，不动）。注意 dxb 树补丁应用顺序会覆盖原始
  context，补丁须以**打完前序补丁后的树**为准生成（本补丁已验证
  apply）。
  **RET=1 无人值守机制**：`build.sh RET=1` 在 `loader_boot` 前写 PMIC
  `SOFT_RB_SPARE(0x88f)=0x06`（BOOTLOADER reason）→ XNU panic/PSCI/
  WDT 任一复位自动回 fastboot，闭环测试无需人工。
  **Mac 构建环境打通**（AMD Ryzen 5825U Hackintosh, Xcode 26.3）：
  KDK 13.5_22G74 已装 `/Library/Developer/KDKs/`；`dxb/build.sh` 的
  `-j8` 已提 `-j16`；deps 缺 `gum/xcodes/cmake/ninja` → `/usr/local/bin`
  放 stub 但注意 **build.sh 会重建 PATH**（/usr/local/bin 幸存）且
  `install_deps` 跑在重建**之前** → 启动须
  `env PATH=/usr/local/bin:... MACOS_VERSION=13.5 KERNEL_CONFIG=
  DEVELOPMENT ARCH_CONFIG=ARM64 MACHINE_CONFIG=SWEET ./build.sh`
  （env 传参跳过 gum TTY 菜单）；stage 脚本幂等防御 → 重建前删
  `xnu/bsd/pthread/sweet_libpthread`、`xnu/bsd/sweet_hfs`、
  `xnu/libkern/crypto/sweet_bearssl`。
  Git：`617b82c`(beacon 体系)+`54c573a`(upstream panic /pram merge)
  +`94a5874`(0093 补丁)+`97ee29c`(RET=1) 已推 origin/main。

- **2026-10-23 晚**：**突破 2 —— 尾部追加 DTB 导致 ABL 不跳转。**
  `xperable -l` 读 P_114 ABL 日志发现标准 `fastboot boot` 已解压/加载内核
  （Kernel Load 0x80080000），但报 `DTB offset is incorrect` /
  `Dtb offset goes beyond the kernel size` 后不跳转。根因是
  `shim/build.sh` 把 478KB dtb.img 追加到 Image 尾部，与 Image 头
  `image_size`（~250KB）不符。**修复：`NO_DTB=1` 构建**（ABL 用 x0 传
  DTB）。实测无 DTB beacon 用**标准 `fastboot boot` 第一发**即跳转执行
  （PSHOLD 断电）。同时从 RAM dump 了 P_114 VerifiedBootDxe
  （`research/VerifiedBootDxe-P114.pe`，0x25FC 已被 `-6` 改为 `b`），
  `VBDeviceInit` 确认解锁时跳过 AVB。ABL 日志还给出启动标志位表
  （0x4=USB INSERTED、0x8000=HARD RESET……）和 reset-key/shutdown-timer
  逻辑；电池画面 = XBL QcomChargerApp 的充电启动 UI，不是故障。
  新镜像格式：`NO_DTB=1 GZ=1 BEACON=N` + stock ramdisk。
- **2026-10-22 深夜（23:45 追加）**：**"10s 必死"谜底 = `fastboot boot` 需要
  xperable 运行时补丁。** 详见 `HANDOVER-BOOT-KILLER.md` 顶部两节。
  要点：① 每个新 fastboot 会话先 `xperable -B -U -s 0xf49880 -x -4 -6`，
  再用 `xperable -I <img> -1 -c boot -1` 发 boot（标准 fastboot 客户端不行；
  打完补丁后第一条 boot 常不跳转，需发两次）；② 补丁重启即失效；
  ③ shim 确实能执行（PSHOLD/自复位可证）；④ 自复位回 fastboot（RR=
  0x77665500 + PSCI SYSTEM_RESET）可行，可做全自动测试闭环；
  ⑤ start.S 曾有 ICH_EISR_EL2 只读寄存器笔误（已 git 回滚）；空 ramdisk
  会被 ABL 静默拒绝（用 stock ramdisk）；⑥ 待查：delay_ms 是否失效、
  WDT MMIO 是否挂总线、shim 的 SPMI 写为何不落地（PMIC spare 通道已
  验证跨冷启动持久）。手机最终状态：stock boot + Android 8.0.0（充电
  画面按 2 秒电源键回 Android）。
- **2026-10-23**：**BOOT 谜案破案**（HANDOVER-2026-10-23.md）——根因 = ABL
  LinuxLoader 只认 `Image.gz-dtb`（gzip+追加裸DTB），格式错时不跳转不报错，
  悬停复位伪装成"~10s 死亡"。另发现**每会话须先打 xperable ABL 补丁**
  （`-x -4 -6`，复位失效）。shim 已实测稳定执行+SPMI 可控 PON reason
  自动回 fastboot（无人值守循环）。屏幕 RGB565 但 EBS 后无帧，显示暂废。
  下一步：lilac XNU 内核（CI）→ mkpayload → gzdtb shim。
- **2026-10-22 夜**：**`fastboot boot` shim 必死调查 22+ 发全灭，交接 `HANDOVER-BOOT-KILLER.md`**。
  TWRP 同路径存活 >20min（环境放行），但 v10 化石镜像今晚也死（非代码退化）。
  排除：text_offset/gzip/el2全套/MMU/APSS-WDT/TZ-SCM/PSCI/全核/PMIC-PON_WD(SPMI裸驱动)/boot污染/低电。
  关键认知：① 充电图标=PMIC 冷启动（warmboot=0，一切易失证据销毁，死后取证不可能）
  ② DSI cmd-mode 面板 FB 写入不上屏，"看屏幕"通道作废
  ③ `tz-log@146bf720` 占了 IMEM 页 —— 标记限 <0x720
  ④ SPMI v2 裸驱动已实现 `shim/spmi.c`（PON_WD disarm 无效）。
  boot 分区已 `flash` 回 stock（`boot-stock-lilac.img`）。
  **下一棒**：先做 pure12-exec 精确秒表（<3s=代码在跑查定时器；~10s=从未执行查 ABL 路径），
  再考虑 TWRP kernel 缝合（patch code0 跳 payload）。
- **2026-10-21 晚**：**Magisk v30.7 永久 root 上线**。流程：Magisk app 修补
  `research/boot-stock-lilac.img` → `fastboot flash boot magisk-patched-lilac.img`
  → bindershell 最后出力（写 `/data/misc/adb/adb_keys` + `magisk --sqlite`
  灌 uid2000 白名单）→ 重启生效。注意 **magiskd 缓存策略，直改 db 须重启**。
  `/data/adb/service.d/adbalways.sh` 每 boot 兜底 adb_keys。bindershell 降级备胎。
  新增 `DISK-CLEANUP.md`（WSL 虚盘瘦身 ~34G + 可再生 DMG ~7.4G 清单）。
- **2026-10-21 上午**：**shim 真机验证成功 + iOS root 镜像建成**。
  `fastboot boot shim-lilac-v11-ping` 完整跑完（`warmboot=0x77665501`
  只在结尾写 = 全程无崩溃）；v11 起每个 cp_mark 检查点写唯一值
  `0x7766XY` 到 IMEM，卡死也能从下个 cmdline 读出最后位置。
  `ios16-root-lilac.img`（16GiB HFS+）构建+fsck 通过。
  **教训：`devreset`（卸载活设备节点）会把手机侧 USB 打挂** →
  `VID_0000` 描述符失败，软件全救不回，恢复阶梯止步于 `replug`。
  手机在外置 hub `VID_214B` 后面（重启 hub 要针对它而非 ROOT_HUB）。
- **2026-10-21 晨**：**PC 硬件约束 —— ES CPU 稳定性问题，限 70% CPU**。夜间
  mkiosroot 满负荷 rsync 时电脑整体重启（WSL/进程全灭，/tmp 清空）。此后所有
  重负载命令必须套 `taskset -c 0-13`（14/20 核）+ `nice -n10` + `ionice -c2 -n7`。
  hfsplus 模块是运行时装载，重启后需 `insmod /root/wslk/fs/hfsplus/hfsplus.ko`
  重新加载（模块文件持久在 WSL /root/wslk 下）。mkiosroot 已带 CPU 限重启。
- **2026-10-20**：xperable CVE-2021-1931 解锁成功（`unlocked:yes`）。流程：Newflasher 降 47.1.F.1.105（删 boot/ 保 P_114、TA 已 dump）→ bindershell root → dd 写 xfl-o77（MD5 `26bd1901...` 校验过）→ xperable 打 LinuxLoader-p114 补丁 → `oem unlock Y` → `format userdata` 修复解锁后循环重启。SD 卡备份 4GB 完好。提取 `lilac-stock.dtb`，完成 iOS 移植可行性评估并克隆 4 个研究仓库到 `research/`。

## >>> 2026-10-26 12:10【GPU 路线开工：sailfish A530 驱动已还原，lilac 移植要点】<<<

**目标**：CA 的 GLES 渲染器跑在 Mesa freedreno(FD540) 上 → 真实 UI。
参照 sailfish（Pixel/8996/A530，实机 20–40fps 跑通锁屏/主屏/Settings）。
已在 Mac `/tmp/xnu-sf` 用 187 个 sailfish 补丁重建出完整参考树（GPU 文件
全部就位：`bsd/dev/sweet_gpu.c` 84KB、`sweet_gpu.h`、`sweet_gpu_shaders.h`、
`iokit/Kernel/arm/SweetGPU.cpp`、`SweetSurface` 的 GPU 相关改动）。

**lilac(MSM8998/A540) 与 sailfish(MSM8996/A530) 的差异（已核实）**：
- 寄存器：GPU **0x5000000**(0x40000)、SMMU **0x5040000**(64K, CB@0x48000)、
  GPUCC **0x5065000**；sailfish 用 0xb00000/0xb40000 → 必须换。
- zap：PAS id 13，carveout 用 `pil_ipa_gpu_region@95200000`(1MB)。
- 固件：a540_zap.mdt/.b02、a530_pm4/pfp.fw（去 4 字节头）、可选 a540_gpmu.fw2。
- **A540 用 3D0 内 GPMU（Cortex-M3）而不是 A6xx 的 GMU/HFI**；但**硬件可无
  GPMU 启动**，关掉 `ADRENO_A5XX_GPMU` feature 即可走 sailfish 的 CP 直启路线。
- 电源：VDD_GFX = **PM8005 S1（SID 5, base 0x1400，与 S4 两相）**，由 CPR4
  @0x5061000 调压；bring-up 先绕过 CPR 固定 ~0.9V 使能。
- GPUCC GDSC/时钟（下游 gpucc-msm8998.c）：CX GDSCR 0x1004、GX GDSCR
  0x1094、GX_BCR 0x1090、gfx3d RCG 0x1070/分支 0x1098、rbbmtimer 0x10B0/
  0x10D0、rbcpr 0x1030/0x1054、isense 0x1100/0x1124、xo 0x1020；AHB 用
  GCC_GPU_CFG_AHB_CLK；GX 域 0x130 bit0 clamp/bit4 reset。
- CX/MX 电压档走 **RPM-SMD**（pm8998_s1_level / s9_level），不是 RPMh；
  我们树里只有 sweet_rpmh.c，需要从 sailfish 移植 `bsd/dev/sweet_rpm.c`。
- 上电顺序（下游 kgsl）：gfx3d 频率→CX GDSC→GX GDSC（先投 gfx_vreg、
  GX BCR reset 脉冲、清 clamp）→clk 倒序→AXI 票→a5xx_regulator_enable
  (0xA887/0xA88D/0xA881/0xA88B)→VBIF→CP_RB/PFP/PM4→zap PAS→CP_ME_CNTL→
  ME_INIT→非安全模式；GPMU 可跳过。

**打包/用户态缺口**：
- 我们的 `mkpayload.py` 只有 `--ramdisk`，缺 sailfish 的 `--blob
  GPU-PM4/PFP/ZAP-MDT/ZAP-SEG`（写入 `/chosen/memory-map`）；内核 `blob()`
  用 SecureDTLookupEntry 读取。需移植。
- 用户态：`tools/sweetgl`（freedreno GLES + libdrm shim + EAGL 替换，
  `eagl.c`）+ 注入 `sweetenv=backboardd:DYLD_INSERT_LIBRARIES=
  /usr/lib/sweetgl.dylib`。lilac 需按 1125×2436 场景与 FD540 重编。
- 安装 dylib 进根卷 = HFS 元数据变更 → 只能用「整卷离线修改+整卷回刷」
  的一致流程，禁止块级混写（教训见 23:10 条目）。

**下一步（顺序）**：① mkpayload blob 支持 + 内核 blob 访问；② SPMI 写
（PM8005 S1）只读探测先行；③ sweet_gpu 骨架 + lilac 基址 + `sweetgpu=1`
寄存器探测；④ 上电/GDSC/时钟→CP/zap 自检（`sweetgpu=2..8`）；⑤ SMMU+
SweetGPU 用户态 ABI；⑥ Mesa FD540+sweetgl 注入，CA 自动走 GLES。
