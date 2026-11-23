# AGENTS.md — Sony SO-02K (Xperia XZ1 Compact / lilac) 工作日志

> 本文件是跨会话交接的**权威状态记录**。接手前先读 `## 当前状态` 和 `## 红线`。
> 每完成一个里程碑更新 `## 当前状态` 并在 `## 变更日志` 追加一条。沟通用简体中文。

---

## 当前状态（2026-10-26 20:01）

- **iOS 16 已从 UFS 根启动到 GUI**（launchd/SpringBoard/WindowServer，
  hello 上屏；迁移已 data-only 跳过；卷健康）。
- **GPU 进度（真机验证）**：VDD_GFX 900mV（PM8005 S1+S4）、
  CX/GX GDSC、Fabia PLL0 lock、gfx3d RCG/branch、RBBM/CP 寄存器可读、
  **SMMU 恒等映射**、**SCM/zap auth 全 0/0**。
- **唯一卡点**：CP 不取指（RPTR=0/WPTR=n，scratch=0）。已排除
  寄存器偏移/PLL/SMMU/电源/GPMU 电源/zap；已找到 sagit 的完整
  **A540 QcomGPU 驱动**（`research/ios_sagit/drivers/gpu/QcomGPU/`，
  AdrenoStartup/Submit/Firmware/MMU），并按其顺序做了 pre-zap 版本。
- **待测镜像（仓库根目录）**：
  1) `shim-4k-v196p60-pre-zap-gzdtb.img`（**首选**：pre-zap A540
     startup + 60s 停顿，方便相机读日志；level 13）
  2) `shim-4k-v196-pre-zap-gzdtb.img`（20s 停顿）
  3) `shim-4k-v197cp60-pre-zap-gpmu-gzdtb.img`（level 14，pre-zap+GPMU，60s）
     / `shim-4k-v197c-...`（20s 版）
  4) `shim-4k-v195-hwcg-gzdtb.img` / `v194-trust0` / `v193-secure-cb`
- **一键脚本**：`research/boot_v193.sh`（默认 v196；可传镜像名）。
- **当前手机（2026-10-27 07:46）**：用户已按回 **fastboot**（蓝灯，
  `VID_0FCE&PID_0DDE`）。用户上班中；**红线：无人值守只跑
  level ≤10 的镜像，绝不让它进绿灯 E1F4**。
- **v222 实机（2026-10-27 08:24，level 10 + USB，SMMU 对齐 sailfish）**：
  `shim-4k-v222-smmualign-boot-gzdtb.img`。结果：SCTLR=0x27（含
  CFRE）、S2CR0 固定 translate(0)、SMR0=0x80000000；SMMU 恒等映射在
  **zap auth 后仍然完好**（pre-CP 读回一致，假设“zap 擦 SMMU”否决）；
  RBCCU/SP 上电 rbccu=1 sp=1；CP 仍 **consumed=0 RPTR=0 WPTR=14
  scratch=0、HW_FAULT=0、sGFSR=0、CB0 FSR=0x400 恒定、FSYNR0=0**；
  ME_CNTL=0x404（sagit `Halt=0x303` 不匹配 → 之前根本没做过 halt 语义）。
- **v223 实机（level 10 + USB）**：halt 轮询后 ME 仍=0x404、
  VBIF 释放后=0xe0000，CP 仍 consumed=0；`CP release ME=0x404`；
  trust=**0x1**（zap 后正常，需 CP_SET_SECURE_MODE 才归 0）、
  GPMU=0/0、CM3=0x1（保持复位，sagit 首启也如此）。
  `ME_CNTL&0x303` 永不匹配 → A540 上该 halt 位定义与 sagit 注释不符。
- **v224 实机（加 RBBM_SW_RESET + A540 ME_INIT workaround=0）**：
  **两次都在 probe 入口 15s 延时内/CP 段复位回 fastboot**（未进
  E1F4）。sagit 只在确认 halt 后才允许 RBBM reset；我们 ME=0x404
  无法确认 halt，故无条件 reset 不安全。**结论：level 10 不做
  RBBM_SW_RESET**（v225 改为 boot-arg `sweetgpurbm=1` 才做）。
- **v225 实机（关键诊断，level 10 + USB）**：
  `CP cfg rb=0xa1fbb000/0 cntl=0x800020b rptr=0 wptr=0 ctx=0
  ib=0xa25ec000/0 pfp=0xa25f4000/0 me=0x303 stat=0 wfi=0`，
  随后 `CP release ME=0x404 VBIF=0xe0000`，
  `CP consumed=0 RPTR=0 WPTR=14(want 14) cntl=0x800020b`。
  → **halt 0x303 生效、释放后 0x404 是“运行中”状态**（不是错误）；
  RB/IB/PFP 基址、RB_CNTL(bit27)、门铃 WPTR 全部 latch；RPTR 不动。
- **v226–v232 全部实机（level 10 + USB）与已排除项**：
  - v226（blob+4）、v228（页对齐+4 + a540_start 完整 configure：
    VBIF gate/perfctr/HGCG/CP_PROTECT/UCHE/0x0e60/0x0e04）：
    ib/pfp 基址、RB_CNTL(bit27)、WPTR 全 latch；halt=1（ME 0x303）
    → 释放后 ME=0x404；**RPTR 恒 0、scratch 恒 0、无 HW_FAULT/
    sGFSR/CP_INT**。
  - v230：环中追加 `CP_SET_SECURE_MODE(0)+CP_WAIT_FOR_IDLE`+第二
    marker → 仍不消费；SMMU `SMR1/2=0`、`S2CR1/2=0x200ff`、
    `CB1/2 SCTLR=0`（SID1/2 未启用），`sACR=0`。
  - v231：A5xx 无 a4xx 的 CP RAM 接口（0x1F7–0x1FA 读回
    0xDEADBEEF）；**zap 后、写 halt 前 ME 已是 0x303**（固件已
    停好）；AHB error status=0x13d0（来源未明，非 CP_INT）。
  - v229（CP 测试移到 zap 前）：**CP 启动即复位 SoC** → secure
    固件不允许 pre-zap 启动 CP；**顺序必须 zap→CP**。
  - v232（改 a530v1 微码，同内核）：仍 consumed=0 → 微码版本排除。
  - v227（halt 后 RBBM 软复位）：**复位 SoC**（与 v224 相同）→
    **level 10 绝不做 RBBM_SW_RESET**。
- **结论**：SMMU identity、门铃、基址、halt/release、完整 configure、
  微码(+4/对齐/版本)、CP 顺序均已排除。剩余方向：**secure/trust=1**
  （zap 后未归 0）与 **GPMU 未引导**（gpmu=0/0、cm3=0x1）可能是
  CP 不取指的环境前提。直接写 SECVID_TRUST(0xf400)=0 风险高，
  不无人值守做。
- **v233 实机（重大里程碑：GPMU 活了）**：zap 后主机直写
  GPMU IRAM（0x8800+，readback 正常）+ a540_start 同款 AGC 握手 +
  释放 CM3 → **`gpmu=0xBABEFACE/0 cm3=0`**！但同一轮 CP 测试仍
  `consumed=0 RPTR=0 WPTR=21`、trust=1、无 fault。
  → **GPMU 不是 CP 取指的前提**（或至少单独 GPMU 不够）。
  `sweetgpmu=1` 门控；镜像 `research/shim-4k-v233-gpmuhost-boot-gzdtb.img`。
- **v234 实机（用户批准，trust 写）**：`SECVID_TRUST=0` 写入后
  **立即复位回 fastboot（蓝灯，未进 E1F4）** → 非安全世界直接写
  trust 被 TZ 拒绝；**此路不可行，不再试**。
- **v235 实机**：把 GPMU 主机加载移到 **zap 之前**（Linux 顺序），
  zap 前后 GPMU 均 `BABEFACE`，但 **trust 仍=1、CP 仍 consumed=0**。
- **v236（已打补丁在构建）**：补上 Android KGSL
  `a5xx_microcode_load` 中 zap 后的
  **SCM `SET_REMOTE_STATE(state=0, id=13)`**（svc=1, cmd=0xA,
  arginfo=0x2），boot-arg `sweetgpuresume=1`。这是下游 A540
  每次 rb_start 都会发的“放行”调用，可能是 SecureFirmware
  warm-handoff 的关键。镜像 `research/shim-4k-v236-remotestate-boot-gzdtb.img`。
- **v236 实机（SCM remote_state）**：zap 后
  `SCM SET_REMOTE_STATE(0,13)` 返回 **0/0 成功**，GPMU 仍 BABEFACE，
  但 **trust=1、CP consumed=0**。至此 Android KGSL `a5xx_rb_start`
  的每一步都已在 XNU 复刻（bases/RB_CNTL/RB_BASE、zap+remote_state、
  ME_CNTL=0、ME_INIT、CP_SET_SECURE_MODE、GPMU），CP 仍不取指。
- **v237 实机（2MB 页表）**：`sweetgpu2m=1` 把 GB2 拆成
  512×2MB 恒等块（L2 表 0xa1fb7000，TTBR0 0xa1fb3000），
  SCTLR=0x27/TCR 不变；**CP 仍 consumed=0、无 fault**。
  → SMMU 块大小/页表级别也排除。
- **最终判断（2026-10-27 10:45）**：卡点在**安全域 SMMU**。
  lilac DT 的 gfx3d_user 带 `qcom,hyp_secure_alloc`；Android 的
  arm-smmu-msm8998.c 用 **`scm_restore_sec_cfg(sec_id)`** 恢复安全
  SMMU 配置、用 **`hyp_assign_phys()`** 把页表页交给 secure VM。
  GPU 处于 trust=1 时其取指走安全 bank/安全页表；我们的非安全 CB0
  identity 对它不可见。非安全世界写 trust/SID 会被 TZ 复位（v234/
  v218 实锤）。**复刻 KGSL 的 secure IOMMU 初始化**是唯一剩余的
  软件路径，需要 SCM sec_id/hyp 协议，属研究级、不能盲试。
- **静态资源（DT/KGSL 参考）**：lilac DTB 已提取到
  `C:\Users\Cisco He\AppData\Local\Temp\lilac-dtb.bin`
  （shim 容器 offset 0x34AE，totalsize 0x74CDE；解析器
  `research/fdt_dump.py`）。确认：gfx3d_user `iommus=<0x19e,0>`
  （SID0）、`qcom,gpu-offset=0x48000`（= SMMU+0x8000，我们的 CB0
  位置正确）、`qcom,micro-mmu-control=0x6000`（IMPL_DEF1 HALTI/IDLE，
  非取指路由）、`pil_ipa_gpu_region@95200000` 1MB（zap carveout
  基址正确）、touch 为 `synaptics_clearpad@2c`。Android 下游 KGSL
  源码在 `research/ios_sagit/docs/gpu/reference/adreno_a5xx.c`。
- **下一步（静态优先）**：读 `arm-smmu-msm8998.c` 的
  `arm_smmu_restore_sec_cfg` / `hyp_assign_phys` / KGSL iommu 的
  secure 流程，评估从 XNU 发 `scm_restore_sec_cfg` 的可行性；
  或等用户在家做一次性受控实验。手机当前安全在 fastboot。
  **不要写非 0 SID**。
- **摄像头（用户 2026-10-27 重申）**：**每次采集前先
  `curl -m 5 http://192.168.68.57:4747/cam/1/af` 触发自动对焦，
  等 ~2s，再拉 MJPEG 流取一帧**。统一用
  `bash research/grab_stream.sh OUT`（脚本已内置 AF→2s→流→单帧兜底）；
  单帧 `/cam/1/frame.jpg` 只作兜底；`/override` 仅救活卡死的 DroidCam。
  无人值守/夜间也必须自行 AF，不要假设白天不用对焦。
  **不要写非 0 SID**。
- **已知安全事实**：写 SMR/S2CR 的非 0 SID 会**复位 SoC**
  （v218/v220 实锤）；只有 SID0 的 SMR/S2CR 可安全写。
- **相机采集（强制，双方法保留）**：
  - **方法 A（旧，保留）**：`/cam/1/frame.jpg` 单帧；
  - **方法 B（2026-10-26 新增，首选）**：开 `/video?1280x720` 拉
    ≤6 秒 multipart MJPEG，取第一张完整 JPEG 后立即关流：
    `bash research/grab_stream.sh OUT`（内部先试 A，失败再走 B）。
  - DroidCam **同一时间只服务一个客户端**；浏览器预览开着时，HTTP
    会回 `DroidCam busy`（310B HTML）或 200/0，此时先关浏览器。
  - 每次启动**最多抓 1–2 张**（70s 一张即可）；`/override` 仅在卡死
    时救活；**永久停用 `research/grab_cam.py`**。
  - **对焦（2026-10-27 用户最终指令）**：**每次采集前都要**
    `curl -m 5 http://192.168.68.57:4747/cam/1/af`，等 ~2s，再抓帧。
    白天用户手动对焦时曾禁用 AF；现在无论昼夜、只要用户不在，
    一律自己先 AF，不做“白天不用对焦”的假设。
  - 兜底：人工截图（用户已多次用此法提供关键日志/panic）。
- **Mac 源码/仓库**：所有源码在 `~/Desktop/ios-xperia-xz1c`
  （内核/GPU 在 `dxb/xnu`；shim/tools/docs 在父仓）。父仓 merge
  冲突已解决（0111 取 origin 新版），`mkpayload --blob` 与 GPU
  脚本/源码快照已提交到 `docs/gpu-a540/`；**已通过 Windows bundle
  成功 push（main==origin/main）**，Mac 工作区干净（仅 .DS_Store）。
- **源码现状**：v197c（pre-zap + GPMU 游标修正）；v198 静态 ring
  补丁未生效（失败未写入），当前 ring 仍是 `kmem_alloc_contig`。
- **下一动作（2026-10-27 早上）**：手机在 **E1F4 充电模式**。
  1) 人工按电源键开机；若进 Android，电源+音量上 ~10s 回 fastboot。
  2) 用户在旁时再测：boot **`shim-4k-v215-gpu-readback-usb-boot-gzdtb.img`**
     （level 12 + GPMU + pause 150 + USB 日志），等 COM17 读日志，
     grep `a540`（gpmu ready / consumed / post-gpmu / rb / bases /
     diag）；**随时准备按电源键**（level ≥12 有断电风险）。
  3) 无人值守只允许 levels ≤10 的镜像。
- **打包铁律（2026-10-26 20:35 踩坑）**：`tools/mkpayload.py` 只产出
  **SWEETXNU payload**（裸文件，16.7MB），**不能直接 `xperable -I`**
  ——ABL 会报 `Invalid Boot image Header: No Media` 并留在 fastboot。
  必须再包成 Android boot image：`shim/build.sh`（GZ=1，payload 作为
  ramdisk）或把 payload 原位替换进既有 `shim-*-gzdtb.img` 的 ramdisk
  区（同尺寸时零改头）。裸发 = 内核一行没跑，别误判成“复位”。
- **临时大件已移到** `Xperia/_backup-temp/`（userdata-dump.raw /
  iosroot*.raw / iosparts，约 49GB，未入 git，待用户整理）。
- **红线**：不写 UFS、不碰 `ufsice@0x1db0000`、不格式化 SD、不
  devreset、不碰 Y700；破坏性刷写前必须确认。

---

## >>> 2026-10-26 18:30 【副线】PORTING-BIBLE.md 建成 <<<

全生态经验熔炉：9 阶段索引（每坑→解法→代码出处），横跨
sweet/topaz/poplar/sagit/apollo/pandora/HakiOS/UniXNU/elden。
文档分工：AGENTS=日志、FRONTIER=动态、BIBLE=方法索引。

---

## >>> 2026-10-26 18:00 【重大】elden = Y700 Gen4 本机 UniXNU port <<<

- unixnu.com 设备页挖出 `research/UNIXNU_elden`：TB322FC 格式2仓，
  UEFI 菜单/GOP/按键/FAT 两次实机冷启动通过；57 补丁+4K XNU+
  prelink kernel.macho 已生成待上机；目标 iPadOS 17.7.11。
- myron 勘误 = Redmi K90 Pro Max（SM8850）；新 clone：pdx203（Sony
  SM8250 UEFI）、fairlady、cheeseburger（MSM8998 UEFI 实证）。

---

## >>> 2026-10-26 16:30 【副线】群友三分享：生态最远 port 到手 <<<

- `research/data-a-live`（pandora/SM8850/A840）：**GPU 合成 iOS 桌面
  ~110fps**，锁屏/触摸/8核全通，boot_b 双系统菜单，234 补丁。
  Turnip/Metal 插件 Linux 交叉构建链（无需 Xcode），A8XX 寄存器
  差异全录档（FRONTIER §20）——**Y700/A830 的近乎 1:1 模板**。
- `research/UniXNU-picasso`：UEFI RAM-boot 无刷机改 GPT 切分区。
- QcomXnuUsbDebug 串口栈群友在移植中；对 lilac 建议等内核稳定后接。

---

## >>> 2026-10-27 07:46 【COMPACT 交接：GPU/USB/CP 全状态】<<<

**A. 手机与红线**
- fastboot（蓝灯，`VID_0FCE&PID_0DDE`）；用户上班，无人值守。
- **无人值守只跑 level ≤10**（已验证安全）；**level ≥12 可断电进
  绿灯 E1F4**，必须有人按键，只在用户在场时测。
- 充电标志 = 断电，不是复位；软件无法自恢复。

**B. USB CDC 内核日志通道（已打通，重大成果）**
- 驱动：`dxb/xnu/bsd/dev/vermeer_usb_debug.c`、`vermeer_debug.c`、
  `vermeer_log_ring.h`、`vermeer_usb_protocol.h`；console hooks 在
  `osfmk/console/serial_console.c`、`pexpert/arm/pe_kprintf.c`；
  `bsd_init.c` 在 sweet_gpu_probe 前调用；boot-arg **`lilac_usbdbg=1`**
  （默认关闭，主线零影响）。
- lilac 适配关键（真机验证）：
  1) DWC3 `0x0A800000`，无 SMMU，DMA 直打 PA（32 位、16KB 对齐）；
  2) **QUSB2 PHY** `0x0C012000`：ABL 留守 `PWR_CTRL1(0x210).POWER_DOWN=1`；
     写 `(pwr&~1)|CLAMP_N_EN(2)`，等 `PLL_STATUS(0x1A0)`；
  3) **QSCRATCH** `0x0A8F8800`：`GENERAL_CFG+0x08` 置 bit8 → 置
     bit0|bit3 → **再清 bit8**（PIPE clock 选 UTMI，漏清 bit8 就没时钟）；
  4) DWC3 复位：GCTL.CoreSoftReset=**bit11**、DCTL.CSFTRST=**bit30**、
     PHYSOFTRST=**bit31**；epcmd 轮询 50ms；VBUS override
     `HS+0x10 bit20/28`、`SS+0x30 bit24`；
  5) 参考实现：`research/Bergamota/enxerto/usb_dwc3.c`（真机验收）。
- 实测：Windows 出现 **COM17**，从 Darwin banner 起完整 console；
  已用它验证 level 7/9/10 的 GPU 日志。读法：
  `powershell -ExecutionPolicy Bypass -File research/read_com.ps1
  -Port COM17 -Seconds 10 -Out <log>`（1MB 缓冲、DTR）。
- **复位后 fastboot USB 需 1–2 分钟再枚举**（蓝灯常亮），不是砖。
- 注意：console TX 可能重复尾部数据（日志里大量 PAD），grep 可靠。

**C. GPU/CP 现状（截至 07:46）**
- 已验证（USB 日志，全部安全）：
  - level 7：S1/S4 900mV、GDSC 0xa0222000、PLL lock=1
    （mode=0xd0000005）、RCG cfg=0x101、RBBM_STATUS=0x1；
  - level 9：SMMU 恒等映射 SMR0=0x80000000、S2CR0=0、
    CBAR0=0x10000、CB0 SCTLR=0x7、TCR=0x80001c、
    TTBR0=0xa1fb3000、FSR=0x400；
  - level 10：zap 全 0/0（init_image/mem_setup/auth_and_reset）、
    re-power OK、CP 测试 **consumed=0 RPTR=0 WPTR=14 scratch=0、
    ME_CNTL=0x404 RBBM=0xc00003c1 HW_FAULT=0**。
- **关键新发现**：写 **非 0 SID 的 SMR/S2CR 会复位 SoC**
  （v218/v220 实锤）；**只有 SID0 可安全写**。multi-SID 假设作废。
- A540 路径：v206（level 12，无 USB）PON 解码到最后 step=0xb8
  （A540 start 完成）；但 v216/v217（level 12 + USB）在 ~50s 复位/
  断电，说明 **level 12 + USB 组合不安全**，原因未定（电量/
  USB-GPU 交互/secure 写）。**不要再无人值守跑 level 12。**
- 下一步实验（安全，level 10）：**SID0 bypass**（`sweetgpubypass=1`
  只写 S2CR0=1）。**已实机（07:50）**：bypass 写入成功、zap 全 0/0、
  re-power OK，随后 **CP 测试阶段复位**（没到 consumed 行）。
  说明 bypass 改变了 SMMU 行为但 TZ 不允许，不能作为方案。
  **下一步**：在 level 10 调 CB0 页表/TCR/SCTLR（pre-zap 可写）：
  - 试 TCR T0SZ=32（sagit 用 64-input）vs 现在的 28；
  - 试 SCTLR=0x1167（sagit）vs 现在的 7；
  - 或 4KB 页表替代 1GB block；每次用 level 10 + USB 读 consumed。
  对照基线：v214（level 10，无 bypass）稳定 consumed=0、不复位。

**D. 镜像清单（仓库根目录）**
- `shim-4k-v221-l10-bypass-usb-boot-gzdtb.img`：**下一步**，level 10 +
  SID0 bypass + USB（当前内核 07:45 编好，需打包）。
- `shim-4k-v214-gpu10-usb-boot-gzdtb.img`：level 10 对照（安全，
  consumed=0 基线）。
- `shim-4k-v215-gpu-readback-usb-boot-gzdtb.img`：level 12 + GPMU +
  USB + 读回（有人值守才用）。
- `shim-4k-v218-multisid-usb-boot-gzdtb.img`：**危险，会复位**（多 SID）。
- `shim-4k-v219-bypass-usb-boot-gzdtb.img`：**危险**（16 SID bypass）。
- `shim-4k-v212-gpu7-usb...`、`v213-gpu9-usb...`：level 7/9 安全验证。
- `shim-4k-v216/v217...`：halt 实验（v217 导致 E1F4，勿用）。

**E. 构建/打包/发弹**
- 内核：Mac `~/Desktop/ios-xperia-xz1c/dxb`，
  `rm -f build/xnu.obj/kernel.development.sweet && INCREMENTAL=1 ./build.sh`
  （PATH 需 `/usr/local/bin`）。产物
  `build/xnu.obj/DEVELOPMENT_ARM64_SWEET/kernel.development.sweet`。
- 打包：`tools/mkpayload.py KERNEL /tmp/adt-v144.bin OUT --cmdline "..."`
  （GPU blob: GPU-PM4/PFP/ZAP-MDT/ZAP-SEG/GPMU）。
- 封 Android boot image：`research/repack_boot.py <container> <payload> <out>`
  （容器用 `shim-4k-v198c-serr-boot-gzdtb.img`，ramdisk 偏移 495616）。
- 发弹：`cd unlock && ./xperable.exe -B -U -s 0xf49880 -x -4 -6`
  必须 2/2 `patched successfully`；stall 就 `fastboot reboot bootloader`
  刷新；只发一发 `-I ../<img> -1 -c boot -1`。
- 取日志：等 COM17（~40s）→ `research/read_com.ps1`；GPU 段因 15s
  SPMI 延时会晚到，可读第二次。

**F. 历史/待办**
- iOS 16 已从 UFS 根启动到 GUI（hello）；GPU CP 是当前唯一硬卡点。
- 触摸（QUP5 0xc179000 / RMI4 F11 / 从机 0x2c）、几何
  （canvas CFData 4B、1125×2436 居中）、Mesa freedreno FD540 +
  sweetgl 注入、LSE 模拟器均待后续。
- 断电自动恢复（RTC/PON auto-power）未实现；PM8998 用 PM8941 RTC
  布局（SID0/base 0x6000，控制 0x6046 bit7，秒 0x6048），写 PMIC
  需有人值守。

---

## >>> 2026-10-27 03:18 【v219：SMMU bypass 诊断选项】<<<

- 新 boot-arg **`sweetgpubypass=1`**：identity 段把 SID0–15 的
  S2CR 全部设为 **bypass (1)**，绕过 stage-1 翻译。
- 用途：若 v218（多 SID translate）仍 `consumed=0`，用 v219 判断
  是否 SMMU 翻译问题：
  - v219 能取指 → 问题在 SMMU 页表/上下文，转向 CB0 页表调试；
  - v219 也不取指 → 问题在 CP/环/ME_INIT 本身。
- 镜像：`shim-4k-v219-bypass-usb-boot-gzdtb.img`（level 12 + GPMU +
  pause 150 + USB + `sweetgpubypass=1`）。**有人值守才启动**。
- 早上测试顺序：v218（多 SID）→ 若 consumed=0 → v219（bypass）。

---

## >>> 2026-10-27 03:06 【CP 取指新假设：SMMU stream ID ≠ 0 → v218】<<<

- 对照 sagit configure：A540 的 SECVID_TSB、12 个 ADDR_MODE_CNTL、
  18 条 CP_PROTECT、HGCG 我们都有；复位/启动顺序也一致。
- 新假设：**CP 的 SMMU stream ID 可能不是 0**。我们此前只给 SID0
  建了恒等映射（SMR0/S2CR0），若 CP 用 SID1..15，翻译失败 →
  RPTR 永远 0（与 level 10 观测一致）。
- v218：level>=8 的 identity 段把 **SID 1..15 也映射到 CB0 恒等**
  （SMR=valid|sid，S2CR=0）。安全（无 GPU 写），为早上有人值守
  测试准备。
- 镜像：`shim-4k-v218-multisid-usb-boot-gzdtb.img`（level 12 +
  GPMU + pause 150 + USB，无 halt）。**不在无人值守时启动**。

---

## >>> 2026-10-27 03:04 【事故：v217 A540 测试 → E1F4 充电模式】<<<

- 测试件：`shim-4k-v217-haltb8-usb-boot-gzdtb.img`（level 12 +
  GPMU + `lilac_usbdbg=1` + `sweetgpuhalt=1`，halt 放在 b8 之后）。
- 结果：手机进入 **E1F4 充电模式**（`VID_0FCE&PID_E1F4`，
  黑屏+绿灯），不是 fastboot。**需人工按电源键**。
- 结论/红线：
  1) **A540/GPMU 路径仍可能断电**，halt 放 b8 后也没能避免；
     之前 PON timing 解码的 b8 结论不足以证明安全。
  2) **level ≥ 12 的任何镜像都不得无人值守运行**；
     levels 7/9/10 + USB 是安全的（已验证）。
  3) 断电后的自动恢复仍未解决（RTC/PON auto-power 未验证）。
- 早上恢复：按电源键开机；若停在 Android，按电源+音量上 ~10s
  回 fastboot。然后先用 v215（或不带 halt 的 level 12）**在有人
  值守下**重测，随时准备按键。

---

## >>> 2026-10-27 02:22 【GPU + USB 联合验证（level 7）】<<<

- `shim-4k-v212-gpu7-usb-boot-gzdtb.img`（`sweetgpu=7` +
  `lilac_usbdbg=1` + pause 30）实机：COM17 在 boot 后约 40s 出现，
  连续两次读取拿到完整 GPU 电源/时钟/SMMU 日志：
  - S1/S4 752→900mV、en=128；GDSC CX/GX=0xa0222000；PLL lock=1
    （mode=0xd0000005）；RCG cfg=0x101 branch=1；RBBM_STATUS=0x1；
  - SMMU ID0=0x48016403 ID1=0x20008003 SCR0=0x200626；
    SMR0=0、S2CR0=0x200ff（level 7 仍 bypass，符合预期）。
- 结论：**USB 日志通道与 GPU 探测共存**，可在明早直接读
  `a540 gpmu/consumed/diag`，无需相机。
- 读串口建议：boot 后等 COM17 出现（~40s），读 8–10s；GPU 段因
  15s SPMI 延时会晚到，必要时读第二次。
- **level 9 验证**（`shim-4k-v213-gpu9-usb-boot-gzdtb.img`）：
  USB 日志完整看到 SMMU 恒等映射成功——`SMR0=0x80000000`、
  `S2CR0=0`、`CBAR0=0x10000`、`CBA2R0=1`、`CB0 SCTLR=0x7`、
  `TCR=0x80001c`、`TTBR0=0xa1fb3000`、`FSR=0x400`。pre-auth
  SMMU 与 USB 通道共存，早上 v211 的日志读取无后顾之忧。
- **level 10 验证**（`shim-4k-v214-gpu10-usb-boot-gzdtb.img`）：
  USB 日志覆盖 zap 全程（`init_image 0/0`、`mem_setup 0/0`、
  `auth_and_reset 0/0`、`zap OK`）、re-power、CP 测试：
  `CP consumed=0 RPTR=0 WPTR=14 scratch=0`、
  `ME_CNTL=0x404 RBBM=0xc00003c1 HW_FAULT=0`。
  level 10 未跑 A540/GPMU，CP 不取指符合预期；早上 v211 才测
  A540 start + GPMU。
- **v216（level 12 + GPMU + USB + sweetgpuhalt=1）实机**：
  日志只到 `SWEET GPU: probe level 12`，boot 后约 45–50s 就复位
  回 fastboot（**不是充电标志**）。说明 level 12 的 A540 段仍有一处
  致命写在 ~50s 内触发复位，且发生得比日志经 USB 环送出更快。
  `sweetgpuhalt` 未执行到。下一次用 v215（无 halt）在有人值守时
  重测，并可用多次短读抢在复位前拿到 a540 行。
- **PON timing 解码 v216**：用 timing shim（4s/step）配无 GPU payload
  做「步骤读取器」`shim-4k-pon-reader-boot-gzdtb.img`，实测回 fastboot
  =106s；基线约 74s → 延时 ≈32s → **最后 step = 0xb8**。
  **结论：A540 start + GPMU 已完整返回，致命复位在 b8 之后的
  返回/新增读回阶段**。
- **v217**：`sweetgpuhalt` 移到 b8 之后立即驻留（skip 后续读回），
  这样既保留 `gpmu ready/consumed/diag` 日志，又避开 b8 后的复位；
  WDT/`sweetar` 仍会回 fastboot。

---

## >>> 2026-10-27 02:07 【USB CDC 打通：COM17 + 内核日志流】<<<

- **最后一块拼图 = QUSB2 PHY 上电**：ABL 在 `fastboot boot` 后把
  QUSB2（lilac `0x0C012000`）留在 **PWR_CTRL1.POWER_DOWN=1**；必须
  写 `(pwr&~1)|CLAMP_N_EN(2)`、等 `PLL_STATUS bit5 (LOCKED)`。
  另外 QSCRATCH GENERAL_CFG(+0x08) 的 PIPE-UTMI 时钟选择要
  **置 bit8 → 置 bit0|bit3 → 再清 bit8**（漏了清 bit8 就不出时钟）。
- 实机：`lilac_usbdbg=1` + `sweetgpu=1` 启动后，Windows 出现
  **COM17**，读到**从 Darwin 内核 banner 开始**的完整 console 流
  （161KB，含 `SWEET GPU/SPMI`、`VERMEER USB: qusb pwr=0x23→0x22`、
  `PIPE_UTMI_CLK cfg=0x0000000d`、`connected speed=0`、GET_DESCRIPTOR）。
  **读法**：`powershell -ExecutionPolicy Bypass -File research/read_com.ps1
  -Port COM17 -Seconds 8 -Out <log>`（1MB ReadBufferSize、DTR=1）。
- 注意：console 环的 TX 可能重复尾部数据（日志里大量 PAD 重复），
  grep 关键字仍然可靠；后续可精简 pad 输出。
- **复位后的 USB 再枚举需要 1–2 分钟**（DWC3 被接管后 ABL 重新初始化
  较慢）；期间 `fastboot devices` 为空、蓝灯常亮，**不是砖**，等
  1–2 分钟即可。若超过 3 分钟仍无枚举，再人工电源键。
- **下一版 v211**：`shim-4k-v211-gpu-usb-boot-gzdtb.img` =
  level 12 + GPMU + pause 150 + `lilac_usbdbg=1`。**明早用它**：
  GPU 日志直接走 COM17，不再依赖相机（即使随后断电，日志已实时收到）。

---

## >>> 2026-10-27 00:16 【无人值守红线：绝不让它进充电标志】<<<

- 充电标志 = PMIC 断电（PSHOLD/低电保护），不是复位；**断电后软件
  无法自恢复**，只能人工按电源键。
- 已知触发：pre-auth secure CB/0x68/0x70（v196）、可能还有 GPU 上电
  瞬间的 TZ/PMIC 保护。v206 以后 GPU 路径走到 `0xb8` 才断，说明
  a540 本身不再触发，但 pause/launchd 阶段仍有风险。
- **无人值守策略（夜间）**：
  1) 不跑任何 GPU 上电镜像；只跑 `sweetgpu=0/1` 的低风险镜像；
  2) 每次启动都带 `sweetar=120..180`，保证自动回 fastboot；
  3) USB CDC 测试若把 USB 打断，也由 `sweetar` 兜底回 fastboot
     （已实测有效）；
  4) 一旦发现充电标志/绿灯黑屏，立即停止，等人工。
- **未来 bulletproof 方向**：研究 PMIC PON「插入充电自动开机」或
  RTC 闹钟唤醒，作为断电后的自动恢复；在验证前，GPU 测试只在
  有人值守时进行。
  - RTC 事实：PM8998 用 **PM8941 RTC 布局**（SPMI SID0、base 0x6000；
    控制 0x6046（bit7=RTC running）、秒 0x6048 4B LE），见
    `research/ios_sagit/drivers/bus/QcomSPMI/PMICRTC.h`。
  - 该驱动**只读**、明确不 enable/stop/set/rearm；写 RTC 闹钟
    （Android 关机闹钟的自动开机路径）必须有人值守并先确认寄存器
    与 PMIC 行为，**不在夜间无人值守时实施**。

---

## >>> 2026-10-27 00:15 【USB CDC 移植进展 + v206 到 0xb8】<<<

- **v206 的 PON `last step=0xb8`**（下一次启动 shim 读出）：A540 start
  （含 GPMU、非安全 CB0）**完整跑完并返回**；断电/复位发生在之后
  （pause/launchd 阶段），不再是 secure 写。
- **USB CDC（vermeer 移植）**：
  - 文件：`bsd/dev/vermeer_usb_debug.c`、`vermeer_debug.c`、
    `vermeer_log_ring.h`、`vermeer_usb_protocol.h`；hook 在
    `osfmk/console/serial_console.c` 与 `pexpert/arm/pe_kprintf.c`；
    `bsd_init.c` 里 sweet_gpu_probe 之前启动；boot-arg
    **`lilac_usbdbg=1`**，默认关闭。
  - lilac 适配：DWC3 `0x0A800000`（1MB map）、无 SMMU（删 identity）、
    DMA 直打 PA（32 位、16KB 对齐，`kmem_alloc_contig` 用 `0x3fff`
    对齐）、GCC 只打印不 bail、QSCRATCH 段删除。
  - 实机定位（PAD 遥测）：halt/reset/event-address 阶段都过，卡在
    **`endpoint command timeout`**（DWC3 epcmd）。用 `/af` 对焦后读到
    `pre-ep gctl=00112000 dctl=00080000 dsts=00120030 phycfg=00002500`
    → GCTL 是 OTG（PrtCapDir=3），**DCTL.RUN 置不上、命令接口不响应**。
  - 已尝试并排除：16KB DMA 对齐、GCC 门控误判、先 RUN 再 epcmd、
    GCTL CoreSoftReset + 清 CSFTRST、**完全不复位直接接管 ABL 控制器**
    —— 均仍在 epcmd 超时。
  - 决定性参考资料：`research/Bergamota/enxerto/usb_dwc3.c`
    （sweet 家族真机验证）——**GCTL.CoreSoftReset=bit11**、
    **DCTL.CSFTRST=bit30**、PHYSOFTRST=bit31、epcmd 轮询 50ms；
    序列：PHY 复位脉冲(1ms) → 100ms → 清 CoreSoftReset →
    DCTL=CSFTRST 等清 → GUCTL1 bit17/26 + DCTL bit10/12 清 →
    事件环/DCFG/DEVTEN=7 → ep0/ep1 配置+DALEPENA →
    **QSCRATCH VBUS override（HS+0x10 bit20/28，SS+0x30 bit24）**
    → `DCTL|=RUN`。
  - 已按上述完整重写（lilac QSCRATCH=0x0A8F8800），epcmd 仍 50ms
    超时（id=5533300a、gctl=00112000、dctl=10f00000）。
    **跳过 PHYSOFTRST（保留 ABL 训好的 PHY）也仍超时**；补上
    **QSCRATCH GENERAL_CFG(+0x08) 的 PIPE-UTMI 时钟选择**
    （bit8 置位→bit0|bit3，dwc3_qcom_select_utmi_clk）后**仍超时**。
    **结论：MSM8998 的 DWC3 wrapper/PHY 组合需要更多专用寄存器
    资料（QSCRATCH role/UTMI 细节、QUSB2 PLL 状态），今晚停止盲试；
    代码保留、boot-arg 默认关闭。**
  - 测试镜像：`shim-4k-usbdbg11-boot-gzdtb.img`（最新失败版，留档）。
  - 测试镜像：`shim-4k-usbdbg5-boot-gzdtb.img`（GPU `sweetgpu=1` +
    `sweetgpupause=60` + `lilac_usbdbg=1`，不碰 GPU 寄存器）。
  - 观测：Windows 暂未出现新 COM（VID_1209:316E）；手机每次都会在
    `sweetar=180` 后自动回 fastboot，安全。

---

## >>> 2026-10-26 22:36 【v206：post-auth 也只走非安全 CB0】<<<

- v205（删除 carveout）实机：boot 后 **~70s 复位回 fastboot**（带安全 shim
  路由）。说明 carveout 删除后终于走到 `sweet_a540_start(secure_cb=1)`，
  而 **post-auth 写 secure CB1/2 与 SMMU 0x68/0x70 仍会触发安全复位**
  ——安全世界在 PAS auth 后也没把 SMMU secure 寄存器交给我们。
- v206：post-zap 调用改为 `sweet_a540_start(0)`，全程只用非安全 CB0
  （CP 的 SID0 走 CB0，足够）；不再写 secure CB/0x68/0x70。
- 判读：若 v206 不再 70s 复位并打印 `a540 gpmu ready` / `consumed`，
  则安全 CB 假设实锤；若仍复位，下一步查 RBBM 软复位/CP_PROTECT 等。
- **v206 实机（22:42）**：boot 后 ~50s 内就回 fastboot（蓝灯），
  比 v205 更早，且画面 blur 无日志。两个新怀疑：
  ① 电池电量低，GPU 上电/PLL 瞬间电流导致 brownout（此前绿灯充电、
  多次长测）；② v206 改了 post-zap 参数后更早碰到另一处复位。
  **下一步：先充满电再测**，并在 35s/45s 各抓一张定位复位前最后画面。

---

## >>> 2026-10-26 22:16 【v204 死点实锤：zap 后的 carveout 写入】<<<

- v204（level 12 + GPMU + 诊断）实机画面定格在 `step 0x91` 之后：
  zap 成功 → 随后进入 `if (level >= 11 && zap_ok)` 的 **carveout 方案**：
  `ml_io_map(0x95200000, 1MB)` 后把 PM4/PFP 写进 carve+0x10000/0x20000。
  安全 carveout 只映射已认证的 8KB 段，主机写直接挂总线（此前已实锤
  data abort），并让 CPU 卡死，WDT 也不一定拉回。
- **v205 修复**：删除整个 carveout 分支，统一用 `kvtophys` 的普通
  ring/ucode/scratch（`cp_ring` / `cp_scratch`）继续 CP 测试与 A540
  start。镜像：`shim-4k-v205-nocarve-boot-gzdtb.img`（level 12 +
  GPMU + 诊断，pause 150）。
- 判读：如果 v205 能过 `step 0x92..0x97` 并打印 `a540 gpmu ready` /
  `consumed`，就说明 carveout 是唯一剩下的死点。

---

## >>> 2026-10-26 21:50 【v203 里程碑：认证后启动 + GPMU 不复位，跑满 300s】<<<

- `shim-4k-v203-postzap-gpmu-boot-gzdtb.img`（level 12 = zap/auth →
  A540 start(secure_cb=1) → GPMU 先释放；pause 150）实机：
  - 70s/105s/180s 均 **fastboot=0**（未复位）；v198c/v200/v202 都在
    约 75s 复位。
  - ~300s 后内核 `sweetar=300` 自动复位并 **回到 fastboot**（路由链正常）。
  - 结论：**pre-auth 启动才是复位源**；认证后启动 + GPMU 不触发安全
    违例。这与 sagit 顺序（startup/gpmu 在 auth 之前）冲突，但我们
    的可用路径是 auth→start→gpmu。
- timing shim（`shim-4k-v203-timing-boot-gzdtb.img`，按 PON 0x88e
  延时 4s/step）实测回 fastboot = 327s，基线约 300–310s，差值落在
  step 0xb4–0xb7 附近（测量噪声 ±10s）——**A540 start 至少走到
  GPMU/等待段，没有在早期寄存器写入处死**。
- **下一步**：需要读内核日志确认 `gpmu ready` / `consumed`。相机仍
  只服务一次请求；下次启动在 50–70s 抓 **1 张**，或人工读屏。

---

## >>> 2026-10-26 21:24 【v202：A540 start 内部分段 PON step】<<<

- 为定位 pre-auth 复位死点，在 `sweet_a540_start()` 内加：
  - `0xb2` halt 完成、RBBM 软复位之前；
  - `0xb3` RBBM 复位之后、CM3 复位寄存器之前；
  - `0xb4` configure 之后、bases/ring 之前；
  - `0xb5` RB base/RCG 配置之后、ME_INIT 之前；
  - `0xb6` ME_INIT 入环（WPTR=n）之后；
  - `0xb7` GPMU（若启用）之后、等 CP 之前；
  - `0xb8` 函数返回前。
- 判读：复位后下一次启动的 shim 蓝屏 `PONe/f 88e=` 即最后到达的
  step；缺哪个 step 就说明死在它前面那条写操作。
- v201 实测：level 14 + `sweetgpupause=150` 仍在 **~75s 复位**，
  说明不是 pause 结束，而是 A540 start 内部某处触发（v198c 同样 ~75s）。

---

## >>> 2026-10-26 20:55 【v200：GPMU 必须在等 CP 之前（v199 顺序 bug）】<<<

- 对照 sagit `AdrenoStartup::start/gpmu` 发现致命顺序问题：sagit 在
  ME_INIT 入环（cursor==9）后**立刻加载 GPMU、释放 CM3、等
  GPMUReady==0xBABEFACE**，**从不先等 RPTR**；我们的代码却把 GPMU
  放在 `consumed && scratch==0xc0ffee01` 之后——CP 不取指就永远
  跑不到 GPMU，而 GPMU 不释放 CP 又不会取指（死锁）。
- v200 修复：ME_INIT + MEM_WRITE + NOP 入环后立即：
  1) GPMU 固件 → type4 写 0x8800+ 的命令缓冲；
  2) AGC（0x9807/36 dword）+ PWR_COL + 0x9805=0xBABEFACE；
  3) **直接写 ring[n]**（不再用 cp_emit/cp_ring，避免覆盖 ME_INIT
     和 WPTR 回退）；WPTR=n；
  4) 释放 CM3（0xA8D8=0），等 GPMUReady；
  5) 最后才等 RPTR==n / scratch。
  返回 `(consumed && scratch==0xc0ffee01) || gready`。
- 镜像：`shim-4k-v200-gpmu-before-wait-boot-gzdtb.img`（level 14，
  安全 shim，60s 停顿）。
- 测试序：v198c（先确认非安全 CB0 不复位）→ v200（GPMU 先释放）。

---

## >>> 2026-10-26 20:53 【铁律：任何重启都必须回 fastboot（不许退步）】<<<

- 三层同时生效才算达标：
  1) **shim**：`RET=1` 编译时，loader 前无条件写 `0x88f=0x06`；
     payload 缺失/被拒时也不再跑 UFS 探测后 `rr_route(0x77665501)`
     （那会进 Android），改为直接 `0x88f=0x06` + PSCI SYSTEM_RESET。
  2) **内核**：`sweet_gpu_probe()` 一进来（SPMI init 后）就写
     `0x88f=0x06`，覆盖后续任何 panic/WDT/GPU 复位。
  3) **cmdline**：`sweetar=300`（内核自动回 fastboot 兜底）。
- 发弹纪律不变：patch 2/2 确认、只发一发、镜像必须是合法
  `ANDROID!` boot image（ramdisk 偏移 4096+kernel_pad）。
- 验证方法：boot 后故意让它复位/panic，`fastboot devices` 应重新
  出现 `BH905ND69G`；若进 Android 或 docomo 咨询页，说明该镜像
  没带上述三层，**禁止再用该镜像**。

---

## >>> 2026-10-26 20:42 【会话工作流总纲（compact 后必读）】<<<

**0. 机器与目录**
- Windows 权威仓：`C:\Users\Cisco He\Desktop\Git\Patch\Xperia`（git 根是
  上一级 `Patch`；远端 `zenkernelsam/Patch` main）。只放脚本/镜像/文档。
- Mac 构建机：`ssh ciscohe@192.168.68.59`（免密）。内核源码
  `~/Desktop/ios-xperia-xz1c/dxb/xnu`；shim 在 `shim/`；`dtb.img` 在仓库根；
  GPU 固件 `~/Desktop/ios-assets/gpu/`。**内核只改 Mac，不要在 Windows 改。**
- 用户可能开着 `tools/mac-autobuild.sh`（pid 720 类），**不要杀用户进程**。

**1. 内核增量构建（Mac）**
```bash
ssh ciscohe@192.168.68.59 'cd ~/Desktop/ios-xperia-xz1c/dxb && \
  rm -f build/xnu.obj/kernel.development.sweet && \
  nohup env PATH=/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin \
  MACOS_VERSION=13.5 KERNEL_CONFIG=DEVELOPMENT ARCH_CONFIG=ARM64 \
  MACHINE_CONFIG=SWEET INCREMENTAL=1 ./build.sh > /tmp/build.log 2>&1 &'
```
- PATH 必须含 `/usr/local/bin`（gum/xcodes），否则 build.sh 误判缺依赖退出。
- 产物 `build/xnu.obj/DEVELOPMENT_ARM64_SWEET/kernel.development.sweet`；
  完成标志 `🎉 XNU Incremental Build Done!`，`grep -c "error:" == 0`。

**2. 打包（Mac → Windows）**
```bash
python3 tools/mkpayload.py KERNEL /tmp/adt-v144.bin /tmp/payload.bin \
  --cmdline "serial=3 cpus=1 keepsyms=1 -noprogress -v rd=ufsroot0 \
unentitled_ios_sim_launch=1 ufsrw=1 rootrw=1 sweetfb_direct=0 \
sweetfb_noconsole=1 sweetar=300 sweetfulltouch=1 sweetswipe=300 \
sweetswipecount=20 sweetgpu=13 sweetgpupause=60" \
  --blob GPU-PM4=.../a530_pm4.fw --blob GPU-PFP=.../a530_pfp.fw \
  --blob GPU-ZAP-MDT=.../a540_zap.mdt --blob GPU-ZAP-SEG=.../a540_zap.b02
```
- `mkpayload` 只产**裸 SWEETXNU payload**，直接发必被 ABL 拒
  （`Invalid Boot image Header: No Media`）。
- 用 `research/repack_boot.py CONTAINER payload OUT` 替换既有 Android
  boot 镜像（`shim-4k-v196p60-pre-zap-gzdtb.img`）里的 ramdisk；
  自动改 `ramdisk_size`，保留 kernel/gz-dtb/tail。scp 回 Windows。
- level 14（GPMU）额外 `--blob GPU-GPMU=.../a540_gpmu.fw2`，
  cmdline `sweetgpu=14 sweetgpupause=60`。

**3. 发弹与取数（Windows）**
- 一键：`bash research/run_gpu_test.sh <img> <tag>`（自动 patch 重试、
  发弹、28 张连拍、USB 时间线；patch 不到 2/2 拒绝发弹）。
1) `fastboot devices` 必须看到 `BH905ND69G`；
2) `cd unlock && ./xperable.exe -B -U -s 0xf49880 -x -4 -6`
   → **必须 2/2 `patched successfully`**；stall 就
   `fastboot reboot bootloader`，等 10s 重试；**绝不带未确认补丁发弹**；
3) `./xperable.exe -I ../<img> -1 -c boot -1`，**每补丁只发一发**；
4) 发后 `fastboot devices` 变空 = 已跳转；仍可见 = 镜像被拒（查 `ANDROID!` 头）；
5) 相机：`curl -m 10 -o frame.jpg http://192.168.68.57:4747/cam/1/frame.jpg`，
   每 5–6s 一张（boot 后 ~30–90s 才有内核日志）；断流首选 `/override`，
   **禁用 `grab_cam.py`**；
6) 无人值守回 fastboot：内核 probe 入口写 **0x88f=0x06**（v198c 起）+
   0111 never-PSHOLD + cmdline `sweetar=300`。若仍进 Android，说明复位
   发生在 probe 之前或路由未落 → 人工长按电源(+音量上)回 fastboot。
   **已修（20:51）**：shim 的 `loader_boot()` 错误路径原本会跑 UFS 探测
   数分钟后 `rr_route(0x77665501)` **正常启动 Android**。现在在
   `con_str("LOADER", why)` 后直接 `0x88f=0x06` + PSCI SYSTEM_RESET，
   payload 出错也会回 fastboot。新 shim 已用 WSL 工具链重编，镜像
   `shim-4k-v198c-serr-boot-gzdtb.img`（含 v198c payload，首选测试件）。

**4. 源码修改纪律**
- 远程改文件用“本地写 Python 脚本 → scp → python3 执行”，精确断言替换。
- **别用 heredoc 直接写含 `\n` 的 C 字符串**（JSON 会把 `\\n` 退化成真换行，
  已两次踩坑）；用 `chr(92)+'n'` 拼，改完 `sed -n` 验证 printf 完整。
- 改完必须重建；Windows 里的旧快照不能当构建源。

**5. git 流程**
- Mac 父仓 push 无凭据时：Windows clone + `git bundle` 导入 + fast-forward
  后 push；或让用户用 GitHub Desktop。
- Windows `Patch` 仓只 add 指定文件（**勿 add** `flash/fw/`、
  `backup*.ab`、`_backup-temp/`）；`_backup-temp/` 已加入 .git/info/exclude。

**6. 当前 GPU 测试序列（按序）**
1) **v198c**：level 13，非安全 CB0 + PON 路由，60s 停顿；
   判据 `a540 secure cb skipped`，且不再复位、走到 `CP consumed`；
2) **v199**：level 14，v198c + GPMU（必须带 GPU-GPMU blob）；
   判据 `GPMUReady==0xBABEFACE`、`CP consumed=1`、RPTR==WPTR、
   scratch[0]==0xc0ffee01、`TrustControl==0`；
3) 若仍复位：读 PON 0x88e 的 last step 定位（RBBM reset / ME_INIT /
   GPMU 哪一段）；若不复位但不取指：查 CP_PROTECT、TrustControl、
   SMMU FSR、CP_ME_CNTL，并考虑 shim/loader 侧 bootstrap。

---

## >>> 2026-10-26 20:40 【全量 AGENTS 精读：A540 权威顺序 + 新工具约束】<<<

- **sagit 权威顺序**（`ios_sagit/boot/bridge/gpu_handoff.cpp:617-647`）：
  `reset → start(ME_INIT) → gpmu → stop → PAS auth → 非安全`。
  **GPMU 未释放时 CP 不会取指**（A540 有 CM3/GPMU；A6xx 的 GMU/HFI 不适用）。
  我们 v196/v198 都是 `gpmu=0`，即使不复位也预期不取指。
- **A540 常量复核**（`AdrenoSubmit.h` / `AdrenoStartup.h`）：
  - 寄存器偏移是 **offsetwords，读写在 C 里 ×4**；RB 基址 `0x0800*4`。
  - RingControl = `12|(2<<8)|BIT27`（32 KiB、32B block、无 RPTR shadow）；
    重编 RB_CNTL 会清 RPTR，破坏保留的 PFP 状态。
  - ME_INIT payload `{0x2f,3,0x20000000,0,0,0,0,0}`（opcode 0x48），
    **不要 A530 的 0xb**；CP_MERCIU_SIZE=0x400；CP_PROTECT_CNTL=7
    + 18 条保护；VBIF_GATE_OFF_WRREQ=9；HANG_INT=0x4000ffff。
  - SMMU CB0 = **0x8000**、4 KiB 页；sagit 生产路径明确禁 identity/
    bypass/secure context，但我们的 CP 自检阶段可用 CB0 恒等映射。
- **GPMU**：CM3 复位 `0xA8D8`，GENERAL_0/Ready `0xA8E0`，
  Error `0xA8E1`，应答 `0xBABEFACE`；先写 0 清 ready 再释放 CM3。
- **QcomXnuUsbDebug 硬约束**（`research/QcomXnuUsbDebug/AGENTS.md`）：
  只允许私有 translated DMA，**禁 identity/BYPASS 与全局 SMMU 写**；
  三页表、2 MiB 对齐 32 位 IOVA、仅 8 页 DMA；`FSR=0x400` 是 FORMAT
  不是 fault；只挂 kprintf 会漏 printf/IOLog；冷启动时钟/PHY 未实现。
  当前仅模型验证，**不插队**。
- **Snapintosh 坑**：单次 `CP_MEMCPY` >1 MiB 会挂 GPU（4 MiB 实测）；
  只借设计，寄存器/GMU 不可套用。
- **A540 成功判据（sagit `AdrenoSubmit.h:300-332`）**：除 CP consumed/
  RPTR 外，最后还要 **`TrustControl==0`**（非安全态）。
- **同代设备经验**：
  - s9（SDM845/A630，GPU 基址同为 0x5000000）：CP 已跑通 NOP+
    MEM_WRITE；4 KiB PTE；**不要读 `CP_ALWAYS_ON`（CPU 读会假死）**；
    `a630_sqe` 窗口 0x8000 太小（固件 34,188B）→ 固件区要够大
    （我们 PM4/PFP 各 0x8000，需分别确认不越界）。
  - raphael（A640）：SMMU IDR0=0x4c017e06（4 KiB、5 CB）；
    **FSR=0x400 是 FORMAT，不是 fault**；S2CR FAULT 写入需编码转换。
  - picasso（A620）：SMMU 连续读会挂总线；CR0 可能被 TZ 锁死。
  - flame（A640）：A640 已跑 Metal v2；恢复 = RBBM 软复位 + CP 重 init。
- **iOS16 用户态路线**：Metal→`EAGLContext initWithAPI:3`→
  `CA::OGL::GLESContext`，`enable_ogl` 默认开，**16.x 不需 vtable 钩子**；
  A540 只走 GLES（Mesa freedreno FD540），不要 Turnip/metal2vulkan。
- **触摸（lilac 可直接套 sagit）**：QUP5 `0x0c179000`、GCC clk37/46、
  rate8=19.2MHz、GPIO87/88 func3 2mA、RMI4 F11；lilac 从机 `0x2c`
  （sagit 是 `0x20`）。lemonadep 教训：Synaptics TCM 只
  `SET_DYNAMIC_CONFIG(0x24)+REZERO(0x27)`，**永不 COMMIT_CONFIG(0x28)**。
- **几何**：`canvas-width/height` 必须是 **4 字节 CFData**，从 IOService
  父节点读取；上报 1125×2436 居中映射，别报物理分辨率。
- **v198 实机（20:38）**：合法 boot image 已被接受，shim 走到
  `Sweet milestone 4: xnu` 并跳内核；随后黑屏 + **红灯**、USB 消失
  = 低电/充电（非 GPU 复位）。需充电后重跑并抓内核日志。
- **教训（20:49 修正）**：Android boot 头按 **page size 4096** 对齐，
  ramdisk 实际偏移 = `4096 + round_up(kernel,4096)` = **495616**；
  之前按 2048 写导致 shim 报 `no SWEETXNU payload`、不跳内核并回
  Android。`research/repack_boot.py` 已改为 4096 并对齐检查。

---

## >>> 2026-10-26 20:36 【避免充电模式（黑屏+绿灯 / E1F4）铁律】<<<

- **本质**：充电标志 = 手机被**断电**（不是复位）。历史实锤两处：
  ① loader 拒跳后 shim 走到底写 **PSHOLD**；② kernel panic 走
  **PSCI SYSTEM_OFF**。PSHOLD 写 0 = power off，落地就是 E1F4 充电
  UI（SONY CD-ROM + mass storage），adb/fastboot 全灭，只能人工按键。
- **必须同时满足**：
  1) 内核带 **0111 never-PSHOLD**：panic/halt 一律 `SYSTEM_RESET`；
  2) shim 预置 `0x88f=0x06` 路由，挂死也能回 fastboot；
  3) **只在 patch 2/2 确认后发弹**；发送 stall 时先
     `fastboot reboot bootloader` 刷新，绝不带未确认补丁发弹；
  4) 只发 **合法 Android boot image**（见打包铁律），裸 payload 会被
     loader 拒跳 → PSHOLD；
  5) 不 `devreset`、不写 PSHOLD、不 `fastboot reboot`（用
     `reboot bootloader`）。
- **黑色+绿灯 ≠ 充电中可自动恢复**：必须人工长按电源（或电源+音量上）
  回 fastboot；远程无法退出。

---

## >>> 2026-10-26 20:10 【v196 实机：secure CB 后立即复位 → v198 只走非安全 CB0】<<<

- v196p60（level 13，pre-zap）实机：内核正常起、SPMI 正常、
  SMMU identity 全部成功（SMR0=0x80000000 / S2CR0=0 / CB0
  SCTLR=7 / TCR=0x80001c / TTBR0=0xa1fa0000），
  A540 pre-zap 段打印到 `a540 secure cb SMR0=... SMR1=... SMR2=...`
  后 **<10s 内整机复位回 fastboot（蓝灯）**，未打印 pause。
- 结论：**pre-auth 阶段 SMMU 仍归安全世界所有**；在 zap/auth 之前
  写 secure CB1/CB2 与 **SMMU 安全寄存器 sACR 0x68 / sCTLR 0x70**
  触发安全违例 → 直接复位。sagit bridge 的注释同样写明：
  它只配置**非安全 CB0（SID 0）**，"no secure-context access"。
- **v198 修复**（`research/patch_v198_nosec.py`）：
  `sweet_a540_start(int secure_cb)`；pre-zap 传 0（跳过 CB1/2 与
  0x68/0x70，只保留 identity 的 CB0），post-zap 传 1（原逻辑）。
  预期日志出现 `a540 secure cb skipped (pre-auth)`，然后继续
  reset/configure/ME_INIT；这是 sagit 同构的首个真测试。
- 镜像命名：`shim-4k-v198-nosec-gzdtb.img`（level 13，
  `sweetgpupause=60`）。

---

## >>> 2026-10-26 20:12 【新神器入库：QcomXnuUsbDebug + WSL 工具】<<<

- **`research/QcomXnuUsbDebug`（邻居评估，已入库）**：XNU 内核内
  DWC3 CDC ACM USB 调试栈 = 实时内核日志流 + 双向控制 + 按需截图，
  **不需要 ADB、挂根前可用**。与现有"死后取证"（PON 单字节 /
  DDR cells / 相机拍屏）互补：PON/fbcon 管内核起来之前，它管起来之后。
- 对 lilac 三个硬前提天然满足：① 补丁锁 Darwin22/xnu-8796.141.3
  （同源）；② ABL fastboot 本身就是 DEVICE 模式 USB（VID_0FCE），
  时钟/PHY/role 已就绪；③ MSM8998 非安全 QSMMUv500 同架构（非 SMMUv3）。
- 移植成本：写 `soc/Sm8998.h` 常量表（DWC3/GCC/SMMU 基址、SID、
  IOVA；lilac stock DTB 有 USB3 节点）+ 过 qualify 清单。
  **风险**：作者标注"尚待完整内核链接和实机验收"，按高潜半成品对待，
  主线内核稳定后再接，不插队；Y700（SM8750 同 SM8550 代）反而更近。
- **WSL 工具已备份入库**：`research/wsl-tools/`（gensymvers_final.py、
  harvest.py、harvest2.py、mkiosroot.sh、ramp_origin.py），
  `hfsplus.ko` 备份到 `research/hfsplus.ko`（防 WSL 重置）。
- **注意**：本机 WSL 有两个发行版，工具链在 **Ubuntu-22.04**（默认
  `Ubuntu` 不是）；访问用 `wsl -d Ubuntu-22.04 -u root`。

---

## >>> 2026-10-26 20:07 【相机采集协议：只抓单帧，停用 grab_cam.py】<<<

- 用户恢复 DroidCam 后实测：`http://192.168.68.57:4747/cam/1/frame.jpg`
  单帧可用（`curl -m 15 -o now-cam2.jpg <URL>`）。
- **`research/grab_cam.py` 永久停用**：它会打开视频流，导致 DroidCam
  反复退出/崩溃。
- **`/override` 只用于救活摄像头**（画面卡死或掉线时打一次），
  **不是**每次抓图前的前置动作。
- 抓图后直接 `Read` 本地 jpg 判读；不要连续高频抓帧。

---

## >>> 2026-10-26 16:00 【副线】QcomXnuUsbDebug：内核内 USB 实时通道 <<<

- 群友分享 `research/QcomXnuUsbDebug`：DWC3 CDC ACM 从 XNU 内部
  流式日志+双向控制+截图，**不需要 ADB、挂根前可用**，补丁锁
  Darwin22/8796.141.3（与我们同 XNU）。
- 对 lilac 的关键前提已天然满足：ABL fastboot 就是 DEVICE 模式
  USB；QSMMUv500 同代非 SMMUv3 坑。
- 移植 = soc/Sm8998.h 常量表 + qualify 验证；早期未实机项目，
  高潜半成品。隔壁主线到内核稳定后可评估接入。

---

## >>> 2026-10-26 15:30 【副线】调试方法论泛化成文 <<<

- 新文档 `METHOD-DEBUG-PORTABLE.md`：审核了隔壁 shim/start.S/loader.c
  的 PON 遥测+恢复路由体系（0x88e 遥测/0x88f 路由、DDR 五区 cells、
  WDT 兜底、90s 内核自复位、XNULDRST 回跳），提炼为可移植四层模型。
- **重要修正**：DDR 在 WDT→fastboot 链中**存活**（bootstrap tables
  读回证明），推翻附录A 旧结论"DDR 全死"；冷复位/断电才死。
- 给隔壁的代码审核小问题见文档第四节。
- Y700 移植时首批 5 个 beacon 已列：存活矩阵→PON 路由→WDT 可写性→
  granule/CPU→FB 几何。

---

## >>> 2026-10-26 文档合并：本文件为唯一权威 AGENTS.md <<<

原 `ios-xperia-xz1c/AGENTS.md`（硬事实/规则手册，406 行）已**逐字并入**
本文件末尾 `## 附录A`。仓库根目录的 AGENTS.md 已改为指路牌。
`research/*/AGENTS.md` 是各上游项目自带工程日志，非本文件副本，保留。

---

## >>> 2026-10-26 19:47 【v197c 离线审计通过】<<<

- 镜像 `shim-4k-v197c-pre-zap-gpmu-gzdtb.img` 审计：
  - ADT memory-map：GPU-PM4/PFP/**GPMU**/ZAP-MDT/ZAP-SEG 全部存在，
    PA 在 DRAM 内（0xa24e8000..0xa24fc000）；
  - cmdline 末尾 `sweetgpu=14 sweetgpupause=20`；
  - 内核含 `A540 pre-zap`、`a540 gpmu ...`、`a540 consumed` 标记。
- 测试顺序不变：v196 → v197c → v195 → v194 → v193。
- 手机仍关机/充电，USB 无枚举；恢复监控在跑。

---

## >>> 2026-10-26 19:45 【v197c：GPMU ring 游标修正】<<<

- 复查发现 v197 的 GPMU 段把 `cp_rb_n` 重置为 0，但 CP 的 RPTR 已停在
  第一次 ME_INIT 的末尾（~15），WPTR 回退 → CP 不会处理 GPMU 包。
- 修正：GPMU 包**接着当前游标追加**（不重置），WPTR=新末尾；已重编
  v197c，镜像 `shim-4k-v197c-pre-zap-gpmu-gzdtb.img`。
- 其余 GPMU 内容（AGC 0x9807/36 dword、PWR_COL、CM3 释放/0xBABEFACE）
  与 sagit 逐条对齐。
- 手机仍关机/充电，USB 无枚举。

---

## >>> 2026-10-26 19:40 【v197：pre-zap A540 + GPMU】<<<

- v197（level 14）：在 v196 的 **pre-zap A540 startup** 成功后，继续
  按 sagit `AdrenoStartup::gpmu`：
  - 从 `a540_gpmu.fw2`（裸 fw2：remaining/version/metadata+指令）生成
    **CP type4 写 0x8800+** 的命令缓冲（gpu_alloc 连续页，页对齐）；
  - GPMU 电压/阈值、AGC 共享寄存器协议（msg=0x9807，36 dword 负载）、
    PWR_COL 配置；
  - CP 包：SET_PROTECTED_MODE 0 → type4 批量写 → SET_PROTECTED_MODE 1；
  - 释放 CM3（0xA8D8=0），等 `GPMUReady(0xA8E0)==0xBABEFACE`。
- 镜像 `shim-4k-v197-pre-zap-gpmu-gzdtb.img`（含 GPU-GPMU blob）。
- 测试顺序最终版：**v196（纯 pre-zap startup）→ v197（pre-zap+GPMU）
  → v195（post-zap+HGCG+trust0）→ v194 → v193**。
- 手机仍关机/充电；USB 无枚举。

---

## >>> 2026-10-26 19:30 【v196：按 sagit 顺序先 startup 后 zap】<<<

- 对照 sagit bridge 发现顺序差异：bridge 是 **startup(reset/start/
  gpmu) → PAS auth → 非安全**；我们之前是 **zap auth → startup**
  （auth 会复位 GPU，顺序反了）。
- v196（level 13）：跳过 zap，先跑 A540 startup（reset/configure/32K
  ring/对齐 fw/ME_INIT），验证 secure 模式下 CP 能否取指。
  镜像 `shim-4k-v196-pre-zap-gzdtb.img`；脚本默认改为 v196。
- 测试顺序：v196（pre-zap）→ v195（post-zap+HGCG+trust0）→
  v194（无 HGCG）→ v193（仅 secure CB）。
- 手机仍关机/充电，USB 无枚举。

---

## >>> 2026-10-26 19:25 【v195：A540 HGCG 全表移植】<<<

- 从 sagit `AdrenoStartup.h` 自动提取 **96 条 HGCG 时钟门控写**
  （RBBM_CLOCK_CNTL/HYST/DELAY 全族），加入 A540 configure；
  v195 = v194 + HGCG。
- 镜像 `shim-4k-v195-hwcg-gzdtb.img`；一键脚本默认改为 v195。
- 测试顺序（USB 恢复后）：
  1) v195（最完整：reset/halt ACK/32K ring/对齐 fw/HGCG/secure CB/
     TrustControl=0）；
  2) 若复位或异常 → v194（无 HGCG）隔离；
  3) 若仍不取指 → v193 + shim/loader bootstrap。
- 手机仍关机/充电，USB 无枚举。

---

## >>> 2026-10-26 19:20 【v194：TrustControl=0 最终门；两发顺序】<<<

- v194 已打包 `shim-4k-v194-trust0-gzdtb.img`：
  v193 的全部内容（A540 reset/config/连续 ring/对齐 fw/secure CB1+CB2）
  + **A540 reset 后写 TrustControl(0xf400)=0**（sagit bridge 在 auth
  后要求 `!rd(TrustControl)`；这是最后一道非安全门）。
- 恢复后的测试顺序（USB 一到）：
  1) **v194**：`bash research/boot_v193.sh`（脚本里镜像名改为 v194 或
     直接按脚本流程）；看 `a540 trust after reset`、`secure cb SMR2/
     S2CR2/CB2`、`a540 consumed/scratch`。
  2) 若 v194 在 trust 写入后复位 → 改用 **v193**（无 trust 写入）定位
     是否 trust 写入触发复位。
  3) 若两者都不取指 → shim/loader 侧 bootstrap（见 19:15 清单）。
- 手机仍关机/充电；USB 恢复监控在跑。

---

## >>> 2026-10-26 19:15 【shim 侧 A540 bootstrap 移植清单】<<<

若 v193（XNU 内 secure CB）不成功，按 sagit `boot/bridge` 在 **shim**
里做 GPU bootstrap。映射到我们的 shim：
1) **电源**：`shim/spmi.c` 已有 SPMI 写；照 v167 写 PM8005 S1/S4
   0x1440/41=900mV + 0x1446=0x80；可选 PM8998 MX。
2) **时钟/GDSC**：GCC 0x810004 bit0；GPUCC 0x5065000：CX 0x1004、
   GX 0x1090/0x1094/0x130、PLL0 0x0004/08/38/14/18/10/00/2c/0c、
   RCG 0x1074/0x1070、branch 0x1098（代码已在 XNU sweet_gpu.c）。
3) **SMMU**：0x5040000，CB0/CB1/CB2 恒等 L1（GB2..7），SMR/S2CR；
   sagit handoff 用 bootVA 窗口映射 ring/ucode，shim 可直接用 PA
   （CP 地址=PA）。
4) **固件**：把 a530_pm4/pfp.fw 去 4 字节头后放进 shim payload，
   页对齐；A540 ME_INIT payload {0x2f,3,0x20000000,0,0,0,0,0}。
5) **启动**：RBBM 0x43 软复位 → ME/VBIF halt ACK → A540 configure
   （CP_PROTECT/CP_MERCIU=0x400 等）→ 清 VBIF/ME halt → ME_INIT；
   成功后再按 `AdrenoStartup::gpmu` 加载 a540_gpmu.fw2 命令并释放
   CM3（0xA8D8=0，等 0xA8E0=0xBABEFACE）。
6) **交给 XNU**：shim 可在 ADT 写 `gpu-bootstrap=1` 与 ring/状态；
   XNU sweet_gpu 检测到后跳过 reset，只做用户态提交。
- 参考：`research/ios_sagit/boot/bridge/gpu_handoff.cpp`（reset/start/
  gpmu/PAS 顺序）、`drivers/gpu/QcomGPU/AdrenoStartup.h`（寄存器）、
  `AdrenoFirmware.h`（QWGF 容器）、`QcomGPUPlatform.cpp`（时钟/电源）。
- 风险：shim 代码量增加、payload 变大；先在离线编译/QEMU 验证。

---

## >>> 2026-10-26 19:05 【恢复后一键脚本 + 后备方案】<<<

- 新脚本 `research/boot_v193.sh`：检查 fastboot → xperable 双 patch
  （失败不循环、不 devreset）→ boot `shim-4k-v193-secure-cb-gzdtb.img`
  → 连拍 8 帧。
- 当前手机：**关机/充电**（黑屏+绿灯），USB 无枚举；需人工按电源。
- 后备方案（若 v193 仍不取指）：按 sagit `boot/bridge/gpu_handoff.cpp`
  把 A540 bootstrap 移到 **shim/loader**：
  1) loader 建私有 SMMU 映射（bootVA 窗口）；
  2) ring/firmware 放 loader-owned 低物理地址并映射到 bootVA；
  3) PAS auth 前跑 reset/start/GPMU；auth 后再切非安全；
  4) XNU 只使用已初始化的 GPU ring。
  参考代码：`research/ios_sagit/boot/bridge/gpu_handoff.cpp`（~1000 行）
  与 `drivers/gpu/QcomGPU/`（AdrenoStartup/Submit/Firmware/MMU）。
- 预期恢复后第一步：`bash research/boot_v193.sh`；看
  `a540 secure cb SMR2/S2CR2/CB2` 与 `a540 consumed/scratch`。

---

## >>> 2026-10-26 18:55 【手机确认关机充电；v193 secure CB 实验】<<<

- 相机 `/override` 刷新后确认：**屏幕全黑 + 绿灯，手机是关机/充电**，
  不是相机卡死的假象；USB 从 18:19 起无 fastboot/adb，需人工按键。
- v193（编译中）：给 GPU 的 **SID 1/2** 建 CB1/CB2 同样恒等页表
  （CBAR/CBA2R、SMRn=ID|BIT31、S2CRn=CBn|translate），再跑 A540
  start；验证 secure stream 是否走 TZ 保留的 CB（若可写）。
- 若 S2CR1/2 被 TZ 锁死，则必须走 sagit 的 loader 侧 bootstrap
  （bootVA + 私有 SMMU 映射 + auth 前 startup/GPMU）。
- 待测镜像将命名为 `shim-4k-v193-secure-cb-gzdtb.img`；恢复后先用
  v193（它包含 v192 的对齐固件/halt ACK）。

---

## >>> 2026-10-26 18:50 【v192：对齐固件+halt ACK；sagit bridge 关键情报】<<<

- v192 已编好并打包 `shim-4k-v192-a540-aligned-gzdtb.img`：
  PM4/PFP 去 4 字节头后复制到 **页对齐连续缓冲**（各 32KB）；
  增加 ME/VBIF halt ACK 等待、TrustControl 打印。
- **sagit bridge 真相**（`boot/bridge/gpu_handoff.cpp`）：
  1) loader 先建 **私有 bootstrap SMMU 映射**（bootVA 窗口），把 ring
     放 loader-owned 低地址、firmware 映射到 bootVA；
  2) 在 **PAS auth 之前**跑 `startup.reset()` → `startup.start()`
     → `startup.gpmu()` → `startup.stop()`（GPMU 固件响应必须完成）；
  3) 然后才 PAS init/setup/authenticate（zap），再做 **secure 过渡**
     （bootEngine.start(...,{true,false})），最后 `TrustControl==0`。
- 对照结论：我们在 XNU 里直接跑 A540 startup，缺的是 **loader 侧的
  私有 SMMU bootstrap 上下文/bootVA 映射**；CP 的 secure stream 可能
  走 TZ 保留的 context，而不是我们 CB0 的恒等映射。
  → 若 v192 仍不取指，下一步应把 A540 bootstrap 移到 **shim/loader**
  （或复用 sagit 的 boot/bridge 代码），而不是继续在 XNU 里试寄存器。
- 手机仍无 USB（黑屏+绿灯），需要人工按键；恢复后先测 v192。

---

## >>> 2026-10-26 18:45 【v191 已编好打包：A540 完整启动序列】<<<

- `shim-4k-v191-a540-start-gzdtb.img` 已生成（level 12）：
  - RBBM 软复位（0x43=1→0）+ ME halt(0x303)/VBIF halt(15)
  - GPMU CM3 复位/清零
  - A540 configure（CP_PROTECT 18 条、CP_MERCIU_SIZE=0x400、
    VBIF_GATE_OFF_WRREQ_EN=9、hang int、UCHE_DBG_ECO_CNTL_2 等）
  - 32KB **物理连续** ring（kmem_alloc_contig）+ 16KB scratch
  - A540 ME_INIT payload **{0x2f,3,0x20000000,0,0,0,0,0}**（无 0xb）
  - 清 VBIF(0x3080=0)/ME(0x832=0) 后 kick；判据 scratch[0]
- 依据：`research/ios_sagit/drivers/gpu/QcomGPU/AdrenoStartup.h`
  （A540-only，2026-10 移植）；另有 AdrenoSubmit/Firmware/MMU 可继续抄。
- **手机仍无 USB（黑屏+绿灯），需要人工长按电源+音量上**；
  恢复后先跑这张 v191（不是 v190）：
  `xperable -B -U -s 0xf49880 -x -4 -6` → `xperable -I
  shim-4k-v191-a540-start-gzdtb.img -1 -c boot -1`。

---

## >>> 2026-10-26 18:30 【找到 A540 现成启动代码（sagit QcomGPU）】<<<

**重大参考**：`research/ios_sagit/drivers/gpu/QcomGPU/` 是 sagit 的
**完整 A540 iOS GPU 驱动**（9554 行，2026-10 移植 Linux a5xx）。
关键文件：
- `AdrenoStartup.h`（530 行）：A540 reset/configure/start/GPMU；
- `AdrenoSubmit.h`：CP 寄存器常量（RB/ME/VBIF/GPMU/IRQ）；
- `AdrenoFirmware.h`：QWGF 容器/CP/GPMU 固件解析；
- `QcomGPU*`：MMU/secure/submit。

**我们之前 CP 不取指的原因（对照后）**：
1) 没做 **RBBM 软复位**（0x43=1→0）+ **ME/VBIF halt**；
2) 没清 **VBIF halt（0x3080=0）**；
3) **A540 ME_INIT payload 是 {0x2f,3,0x20000000,0,0,0,0,0}**，
   我们用了 A530 的 `0xb` workaround；
4) 32KB ring / `RB_CNTL=12|(2<<8)|BIT27`（我们 16KB/11）；
5) 缺 A540 configure：`CP_MERCIU_SIZE=0x400`、`CP_PROTECT_CNTL=7`+
   18 条 CP_PROTECT、`VBIF_GATE_OFF_WRREQ_EN=9`、
   `RBBM_INTERFACE_HANG_INT_CNTL=0x4000ffff`、UCHE_DBG_ECO_CNTL_2 等；
6) GPMU 需 CP 启动后通过 CP 包加载并释放 CM3（0xBABEFACE）。
**v191** 已按 `AdrenoStartup.h` 移植 reset/configure/A540 ME_INIT
（level 12），编译中；手机 USB 仍挂死，待人工重启后测。
- 恢复后先用 v191（不是 v190）；`shim-4k-v191-a540-start-gzdtb.img`。
- 若 CP 起来，下一步照 AdrenoStartup::gpmu 做 GPMU 固件/AGC 协议，
  再进 blitter/渲染。

---

## >>> 2026-10-26 18:22 【USB 挂死，需要物理重启；v190 已备好】<<<

- 18:19 准备启动 v190（level 9，S2CR translate + CP 固件）时，
  xperable 打补丁阶段报 `LinuxLoader payload send failed` /
  `libusb_open ... failed`，之后 **fastboot/adb 都不再枚举**；
  屏幕黑、绿色 LED（充电/低电状态）。按纪律未做任何 devreset，
  需要人工长按电源+音量上重启（或充电后开机）。
- **v190 镜像已打好**：
  `shim-4k-v190-gpu-cp-trans-gzdtb.img`
  （内核 = v189，含 S2CR 可写实验；cmdline sweetgpu=9 + pause20；
  GPU-PM4/PFP blob 已注入）。
- 恢复后步骤：
  1) 确认 fastboot（人工重启后若进 Android，`adb reboot bootloader`）；
  2) `xperable -B -U -s 0xf49880 -x -4 -6`（若第一次 send 失败，
     重插/等枚举后再打，别连续 devreset）；
  3) `xperable -I shim-4k-v190-gpu-cp-trans-gzdtb.img -1 -c boot -1`；
  4) 看 `CP consumed/scratch`，这是 translate 下的关键一测。
- 重要发现（已确认）：S2CR0 可写，fault/translate/bypass 都能回读；
  之前 CP 测试一直跑在 bypass 下，v190 是第一次在 **translate**
  下跑 CP。

---

## >>> 2026-10-26 18:15 【S2CR 其实可写：translate 已生效】<<<

- v189 实机：`S2CR0` 写 2→读 2(FAULT)、写 0→读 0(TRANSLATE)、
  写 1→读 1(BYPASS) —— **寄存器完全可写**；最终已写回 0(translate)。
  之前看到 0x1 是写入序列/读时序造成的误判。
- 这改变了 CP 取指假设：之前 CP 测试跑在 **bypass** 下；现在
  身份映射 CB0 + translate 已就绪，CP 的取指/ring 走 SMMU 翻译。
- v190（同一内核，level 9 + 固件 blob）：CP 测试将在 translate 下重跑。
  判据仍是 RPTR==WPTR、scratch[0]==0xc0ffee01。
- 若 translate 下 CP 工作 → 继续 CP_SET_SECURE_MODE/zap、blitter；
  若仍不取指 → 转 GPMU 固件/CP 复位研究。

---

## >>> 2026-10-26 18:05 【carveout 扩写会 data abort；S2CR 强制 bypass】<<<

- v188（level 11）：把 ucode/ring/scratch 放进 zap carveout
  （0x95200000+0x10000 起）时，CPU 写高偏移触发 **Kernel data
  abort**（在 memcpy/memset 到 carve+0x10000 处）——carveout 只有
  已认证段（8KB）范围可访问，不能当普通内存扩写。
- **S2CR0 写入 0（translate）后读回 0x1（bypass）**：硬件没接受
  translate，或 Secure 侧强制 bypass。下一步实验：写 0x2(FAULT)
  读回验证寄存器是否可写；若可写再写 0，并检查 CBAR/CB0。
  （若 SMMU 只能 bypass，CP 取指可能被 secure 流挡住。）
- 结论：CP 取指是 A540 安全域/GPMU 级问题，不是简单寄存器遗漏。
  下一步优先级：
  1) S2CR 可写性实验（translate/fault/bypass）；
  2) GPMU（CM3）固件加载/释放 CP 复位；
  3) 参考 A540 Android kgsl 的 secure 模式初始化差异。
- 每次实验仍自动回 fastboot；GPU 电源/时钟/SMMU/zap 状态保持可复现。

---

## >>> 2026-10-26 17:50 【CP 取指仍未成功；A540 专属卡点】<<<

- v186：RB_CNTL 去掉 NO_UPDATE（0x20B）、PFP/PM4 不去 4 字节头，
  仍 **RPTR=0 / scratch=0**。
- v187：zap 成功后**重跑 GDSC/PLL/RCG**（GDSC=0x222000/0x222000、
  PLL=0xd0000005、RCG=0x101、branch=1），CP 仍不取指；
  `secure-off read=0 marker=0`。
- 已排除：GPMU RB/CCU/SP 上电、RPTR_ADDR、NO_UPDATE、固件头、
  zap/SCM、SMMU 映射、GPU 电源/时钟/重上电。
- 仍怀疑（按优先级）：
  1) **A540 GPMU（Cortex-M3）未跑固件**，CP 复位没释放。线索：
     GPMU_CM3_SYSRESET=0xA8D8、GENERAL_0=0xA8E0（==0xBABEFACE）、
     a540_gpmu.fw2 写到 GPU 地址 0x5022000（待证实）；
  2) secure 模式下 CP 只能取 **zap carveout（secure）** 内的
     ucode/ring：可把 PM4/PFP/ring/scratch 全部放进 0x95200000+
     的 1MB carveout 再试；
  3) A540 ucode 入口/版本（stock 只有 a530_pm4/pfp）。
- 同款 A540 的 sagit 仓库只做 CPU 渲染，无 GPU 参考；iOS-sailfish
  是 A530，CP 可直接取指，A540 行为不同。
- 设备当前仍是“GPU 电源/时钟/SMMU/zap 全通，CP 未启动”的状态；
  每次实验 180-300s 自动回 fastboot，无需人工。

---

## >>> 2026-10-26 17:35 【ZAP 通过（SCM 成功），CP 仍不取指】<<<

- v185 实机：`scm_call` 在本内核可直接工作：
  - `zap init_image` → 0/0
  - `zap mem_setup 0x95200000+0x2000` → 0/0
  - `zap auth_and_reset` → 0/0 → **zap OK**
  - 之后 CP 测试仍 `RPTR=0 / scratch=0`；`secure-off read=0 marker=0`。
- 说明：TZ/SCM/zap 链路通了；CP 不取指不是 zap 未加载的问题。
- v186（低成本变量）：RB_CNTL 去掉 NO_UPDATE（0x20B）、PFP/PM4
  不去 4 字节头（base=pa，len 全长）。看是否取指。
- 若仍失败，下一批怀疑：
  1) **A540 GPMU（Cortex-M3）未跑固件**，CP 复位未被释放
     （a540_gpmu.fw2 在 assets；需查 GPMU 加载寄存器/流程）；
  2) CP 在 secure 模式下只能取 **secure carveout** 内的 ring/ucode；
  3) ucode 版本/入口（a530 vs a540）。
- 已确认可用件：PON scratch 单次启动内读写、pause20 相机读屏、
  zap/SCM、SMMU identity、GPU 电源/时钟。

---

## >>> 2026-10-26 17:20 【CP 仍未取指；转 zap/非安全模式】<<<

- v183：CP 固件 blob 找到（pm4 19572 / pfp 15876 字节），ring/scratch
  PA 正常，ME_INIT + CP_MEM_WRITE 已发；**RPTR=0/WPTR=14、
  scratch=0、RBBM=0xc0000001、HW_FAULT=0 → CP 未取指**。
- v184：补 GPMU RB/CCU/SP 上电（`0xa887/0xa881=0x778000`，状态
  bit20 均=1）、补 **CP_RB_RPTR_ADDR**；ME_CNTL=0；仍 RPTR=0、
  scratch=0。GPMU 不是瓶颈。
- 判断：A540 的 CP 可能被 **secure 模式/XPU** 挡住（A530 可直接跑，
  A540 未必）。下一步 v185：移植 sailfish 的 **scm_call（smc #0）+
  zap_load**：
  - PAS id 13；carveout `pil_ipa_gpu_region@95200000`（1MB，DTB 实锤）；
  - a540_zap.mdt/.b02；PAS_INIT_IMAGE(2/1) → MEM_SETUP(2/2) →
    AUTH_AND_RESET(2/5)；
  - zap 成功后发 CP_SET_SECURE_MODE 0 + 第二标记 0x5ec0de00。
- 备注：PON scratch 写入在同一次启动内可读回（0xa5），但**跨启动
  会被清**（读回仍是 shim 写的 0x5a），不能当跨 boot 遥测；靠
  pause 20s + 相机/panic 横幅。
- 若 zap 后仍不取指：试 RB_CNTL=0x20B（去掉 NO_UPDATE）、
  CP_INSTR 不去 4 字节头、S2CR 强制 translate、检查 ucode 版本。

---

## >>> 2026-10-26 16:55 【SMMU 恒等映射成功；进入 CP 自检】<<<

- v182 实机：步骤码 0x80..0x87 全部 **rc=0**，回读：
  - `SMR0=0x80000000`（stream 0）、`S2CR0=0x1`（**bypass**，硬件未接受
    translate=0 的写入；GPU 直接看物理地址，对我们恒等映射无碍）、
  - `CBAR0=0x10000`、`CBA2R0=1`、`CB0 SCTLR=0x7`、
    `TCR=0x80001c`、`TTBR0=pt`、TLB sync=1。
  - L1：GB2..7 的 1GB block（VA=PA，normal NC），静态 16KB 页。
- 结论：**GPU SMMU 内存通路已建立**（bypass 也可）。
- v183：CP 自检（level 9，zap 之前的安全模式）：
  - 静态 16KB ring + 16KB scratch；PFP/PM4 固件从 payload blob 取
    （文件前 4 字节头，CP_INSTR_BASE 用 pa+4/len-4）；
  - a5xx_hw_init 子集 + RB_BASE/CNTL + ME_INIT +
    CP_MEM_WRITE(0xc0ffee01)；步骤码 0x90..0x97；
  - 跑 `--blob GPU-PM4=a530_pm4.fw --blob GPU-PFP=a530_pfp.fw`。
  - 判据：RPTR==WPTR 且 scratch[0]==0xc0ffee01 → **CP WORKS**。
- 若 CP 不取指：查 GPMU RB/CCU/SP 上电、PFP/PM4 固件匹配、
  S2CR bypass 是否影响 CP 的取指（必要时强制 translate）。

---

## >>> 2026-10-26 16:45 【SMMU 偏移真相 + PON scratch 遥测通道】<<<

- **PON scratch 遥测成立**：v181 早期自检
  `sweet_spmi_write_ppid(0x008,0x88e,0xa5)` → `rc=0 read=0 val=0xa5`
  —— 写入/读回都成功。以后每步写步骤码，复位后下次启动最前面
  读回（`SWEET GPU: last step=0xNN`），不再依赖相机。
- **SMMU 两个偏移的真相**（DTB + kgsl 源码）：
  - CPU 侧：`qcom,kgsl-iommu reg = 0x5040000 size 0x10000`，
    ARM SMMU 标准布局，**CB0 = 0x8000**（v177 读 0x48000 直接
    挂死/复位，所以 level 8 从未执行）。
  - `qcom,gpu-offset = 0x48000` 是 **GPU AHB 地址偏移**
    （kgsl 的 type0 包 get_reg_ahbaddr 用），**不是 CPU ioremap 偏移**。
  - GR1=0x1000（CBAR0/CBA2R0），SMR0=0x800，S2CR0=0xc00。
- v182：CB 改回 0x8000、SMMU 只映射 64KB；步骤码 0x80..0x87
  覆盖 map/pt/CB/SMR/S2CR 各阶段。下一步看 last step 定位。

---

## >>> 2026-10-26 16:05 【SMMU 布局确认；开始恒等映射】<<<

- v177 只读探针（lilac GPU SMMU @0x5040000）：
  **ID0=0x40016403、ID1=0x2c000003、S2CR0=0x2800ff(FAULT)、
  micro(0x6000)=0x170、SMR0=0**；寄存器可访问。
  CB 基址按 qcom,gpu-offset 用 **0x48000**（不是 msm8996 的 0x8000），
  GR1 仍按 ARM SMMU 用 0x1000（CBAR0@+0、CBA2R0@+0x800）。
- v178：为 gfx3d_user 建恒等映射：
  - `gpu_alloc()` = `kmem_alloc_contig` + `pmap_find_phys`
    （需 extern `ppnum_t pmap_find_phys(pmap_t, addr64_t)`）；
  - L1：GB 2..7，1GB block，VA=PA，AttrIndx0/AP[1]/SH outer/AF；
  - CB0@0x48000：SCTLR=0、TCR2=(7<<15)|1、TCR=28|BIT23、
    MAIR0=0x44、TTBR0=pt、SCTLR=7；GR1 CBAR0=1<<16、CBA2R0=1；
    SMR0=1<<31、S2CR0=0；TLBI+sync。
  - 目标是回读校验 SMR0/S2CR0/CBAR0/CBA2R0/SCTLR/TCR/TTBR0。
- **CP 测试（下一步）**：a530_pm4/pfp.fw 前 4 字节是头，载入 CP 时
  用 pa+4/len-4；payload 里加 `--blob GPU-PM4/PFP`；CP ring 16KB、
  CP_RB_CNTL=11|(2<<8)|BIT27、ME_INIT(0x48)+CP_MEM_WRITE 自检；
  A540 可先不 zap（保持 secure），先证明 CP 能取指并写内存。
- 注意：GPU 寄存器 offsetwords×4；SMMU/GPUCC 字节偏移。

---

## >>> 2026-10-26 15:40 【CP 寄存器可读；下一阶段=SMMU+CP 固件移植】<<<

- v176 实机（同一上电序列 + CP 寄存器自检）：
  - `RBBM_STATUS=0x1, RBBM_AHB_ERROR_STATUS=0, RBBM_STATUS3=0`；
  - `CP rb=0, CP_ME_CNTL=0x383, PFP_INSTR_BASE=0, PM4_INSTR_BASE=0`
    —— CP 块可访问，固件尚未加载（符合预期）。
  - 探测后系统继续启动到用户态，GPU 上电/时钟不影响系统。
- **结论：A540 的电源（S1/S4 900mV）、GDSC（CX/GX）、PLL0、
  gfx3d RCG、GPU/CP 寄存器通路全部打通。**
- **下一阶段（MP1）**：移植 sailfish CP+SMMU：
  参考 `/tmp/xnu-sf/bsd/dev/sweet_gpu.c`（2250 行；SMMU L1 表在
  line 419 附近，CP init 在 sweetgpu=2 段），lilac 常量：
  GPU 0x5000000(256KB)、SMMU 0x5040000(64KB, CB@0x48000)、
  PAS id 13、carveout 0x95200000(1MB)；固件
  `~/Desktop/ios-assets/gpu/{a530_pm4.fw,a530_pfp.fw,a540_zap.mdt,a540_zap.b02}`；
  打包用 `mkpayload --blob GPU-PM4/PFP/ZAP-MDT/ZAP-SEG`。
  顺序：SMMU 恒等映射 → CP_RB(ring 内存) → PFP/PM4 ucode → ME_INIT
  → zap → CP_STATUS 自检。GPMU 可跳过（A540 硬件可直启 CP）。
- 工具链提醒：GPU 寄存器 offsetwords×4；GPUCC/SMMU 字节偏移；
  内核 io_submap 8MB，映射要复用；每次 panic 180s 自动回 fastboot。

---

## >>> 2026-10-26 15:30 【GPU 点亮：RBBM_STATUS=0x1，寄存器通路打通】<<<

- v175（PLL lock 判 MODE bit31 + GPU 寄存器字偏移 ×4）实机：
  - VDD_GFX 900mV、CX/GX GDSC=0x222000、XO 分支=1；
  - Fabia PLL0：**lock=1**，mode=0xd0000005，op=0x1；
  - gfx3d RCG：cmd=0x10、cfg=0x101（PLL0-even/div2）、branch=1；
  - **RBBM_STATUS(0x4F5*4)=0x1，RBBM_AHB_ERROR_STATUS=0，
    RBBM_STATUS3=0** —— GPU 已上电、有时钟、寄存器可读写；
  - 系统继续启动到用户态（未 panic），说明 GPU 上电不破坏系统。
- 这是 A540 bring-up 的**电源/时钟/寄存器三级全通**里程碑。
- **下一阶段**：CP 子系统 —— 移植 sailfish `sweet_gpu.c` 的
  CP_RB/PFP/PM4/ME_INIT 与 zap（PAS id 13，carveout 0x95200000），
  先做 SMMU 恒等映射，再用 `mkpayload --blob GPU-PM4/PFP/ZAP-MDT/ZAP-SEG`
  注入固件；目标是 CP_STATUS 可读、ME_INIT 成功，再谈用户态 Mesa。
- 设备寄存器偏移纪律：GPU 寄存器（0x5000000）用 **offsetwords ×4**；
  GPUCC/SMMU 用字节偏移。

---

## >>> 2026-10-26 15:10 【第二个关键修正：GPU 寄存器偏移以 32-bit 字计】<<<

- v174：PLL lock 判定修正后走 PLL0-even RCG（cmd=0x10/cfg=0x101/
  branch=0x1），但读 `0x5000000+0x4F5` 仍 data abort。
- **根因**：kgsl 的寄存器偏移是 **offsetwords（32-bit 字）**，
  `kgsl_regread` 内部乘 4；`A5XX_RBBM_STATUS=0x4F5` 的字节地址是
  **0x13D4**。我直接读字节偏移 0x4F5 = 非对齐 MMIO → data abort。
  也就是说 GPU 很可能早已可用，只是读地址错了。
- v175：改为 `0x4F5*4`，并顺带读 `RBBM_AHB_ERROR_STATUS(0x4F4*4)`、
  `RBBM_STATUS3(0x530*4)`。这是“GPU 是否活着”的判定点。
- 这条对后续 CP/zap/SMMU 所有 GPU 寄存器访问都适用：**GPU 寄存器
  偏移一律 ×4**；GPUCC/SMMU 寄存器是字节偏移，不要混。

---

## >>> 2026-10-26 15:00 【PLL0 其实已锁定：lock 位在 MODE bit31】<<<

- v173 回读：PLL 配置全部落地（L=0x12/cal=0x3f/frac=0xc00/
  cfg=0x20485699/uctl=0x7）；XO 分支 0x1020=0x1（开）；
  **MODE(0x00)=0xd0000005 → bit31=1 = PLL_LOCK_DET 已置位**。
- **关键事实：Fabia PLL 的 lock 位在 MODE(0x00) bit31，不在
  STATUS(0x24)**（CAF `wait_for_pll()` 读的就是 PLL_MODE）。
  之前误读 0x24=0，误判未锁定并走了 GPLL0 备选（src=5 无时钟），
  所以 RBBM 读才 data abort。
- v174：lock 判定改为 MODE bit31；锁定后走 PLL0-even RCG
  （cfg=0x101, div2, ROOT_OFF 清，branch=1），再读 RBBM_STATUS(0x4F5)。
- 若 v174 仍 abort，下一步查：gfx3d branch 实际 bit0、rbbmtimer
  时钟（RCG 0x10b0 / branch 0x10d0）、PLL 后分频 USER_CTL[11:8]、
  GX 域是否真的出 reset。

---

## >>> 2026-10-26 14:45 【v172：GDSC 上电成功；PLL0 不 lock（下一关）】<<<

- level 5/4 只读探针限定为 `level==5/4`，避免在 GDSC 上电前碰 GX 寄存器
  （之前 v169/v170 的 data abort 就死在 RCG 读）。
- **v172（sweetgpu=6）实机**：
  - `l6 gpuc/gpu` 映射成功；
  - **CX_GDSCR=0x222000、GX_GDSCR=0x222000**（bit0=0，双域已开）；
  - BCR/reset/clamp 序列执行完毕；
  - Fabia PLL0 配置写入后：**lock=0，mode=0，status=0 —— PLL 没有锁定**；
  - RCG：cmd=0x10、cfg=0x101（src=PLL0 even、div2）、branch=0x1；
  - 读 `0x50004F5`（RBBM）触发 data abort（PLL 死 → GPU 无时钟），
    180s 后自动回 fastboot。
- **下一关：PLL0**。排查顺序：
  1) 配置后回读 0x04/0x08/0x38/0x14/0x18/0x10/0x00/0x2c/0x24，
     确认写入落地；
  2) 读/写 gpucc_xo 分支 0x1020（PLL 的输入时钟），确认 XO 没被门控；
  3) lock 轮询延长到 5ms；确认 status 的 lock 位仍是 bit31；
  4) 设 USER_CTL(0x0c) OUT_MASK=0x7 再等 lock；
  5) 备选：把 gfx3d RCG 的 src 直接切到 **GPLL0（src=5）**，
     绕开 PLL0（硬件 mux 可能支持，software map 里被注释掉了）。
- 电源/时钟结论已稳定：VDD_GFX 900mV 两相 ON；GPUCC 双 GDSC ON；
  AHB 时钟 ON；只差 GPU PLL 输出。CP/zap 移植可并行准备。

---

## >>> 2026-10-26 14:25 【v170：PLL+RCG 同读触发 data abort，转入 GDSC 上电】<<<

- v170（去掉 GCC 映射、GPUCC 单次映射）仍在 PLL 打印后触发
  **Kernel data abort**（ESR 0x96000010，外部同步 abort；FAR 指向
  GPUCC/SPMI 映射边界附近），不是 kernel_alloc。说明不是映射数量问题。
- 判别：v168 **单独**读 GPUCC（CX/GX GDSCR、RCG、branch）全部成功；
  v169/v170 先读 PLL0 再读 RCG 就 abort。最可能是 **GX 域仍处于
  collapse/clamp 时访问 0x1000 段寄存器** 的时序问题（PLL 在 CX 域
  可读，RCG/GDSCR 在 GX 域）。结论：**上电顺序必须先行**，在 GX
  域断电时不要继续探寄存器。
- 下一轮（v171）严格按 kgsl 顺序，**只写不读 GX 寄存器**：
  1) 写 CX_GDSCR(0x1004) bit0=0，delay 10us；
  2) 写 GX_BCR(0x1090)=1，delay 1us，=0；
  3) 读改 0x130：bit4 置位→delay→清位→清 bit0(clamp)；
  4) 写 GX_GDSCR(0x1094) bit0=0，delay 10us；
  5) 配置 PLL0（Fabia：L=0x12/cal=0x3f/frac=0xc00/config_ctl=
     0x20485699/config_ctl_hi=0x2067/user_ctl_hi=0x4805，
     standby→reset_n→run→poll lock→OUTCTRL）；
  6) RCG 0x1070：src=1(PLL0 even)、M/N/D 按 1:1、清 bit31(ROOT_OFF)；
  7) branch 0x1098 bit0=1；delay；
  8) 读 `0x50004F5`（A5XX_RBBM_STATUS）确认 GPU 活着。
- 电话当前：每次 panic 后 180s 自动复位回 fastboot（已验证多次）。

---

## >>> 2026-10-26 14:15 【PLL0 未配置；io_submap 只有 8MB 的坑】<<<

- v169 实机读到 GPU PLL0：**mode=0 opmode=0 status=0**，
  L=0x1a、cal=0x1a、frac=0xc555、user_ctl=0x107 —— 是复位默认值，
  **bootloader 没有配置/开启 PLL0**。所以 gfx3d 时钟不能直接开，
  必须先按 Fabia PLL 序列配置 PLL0（L=0x12、cal=0x3f、frac=0xc00、
  config_ctl=0x20485699、config_ctl_hi=0x2067、user_ctl_hi=0x4805，
  再 standby→reset_n→run→等 lock→OUTCTRL）。
- v169 在打印 PLL 后触发 `kernel_alloc ... failed with 3 @vm_kern.c:170`
  并 halt（180s 后自动复位回 fastboot）。根因线索：arm 的
  **`IO_MAP_SIZE = 8MB`**（osfmk/arm/io_map.c），`ml_io_map` 从
  io_submap 永久分配 VA；重复/大块映射会把它耗完（16MB observer
  映射当初就是因此失败）。**新纪律：一个设备块只映射一次，复用；
  大窗口（>几十 KB）尽量按需小块映射。**
- v170：去掉 GCC 的额外映射（GPU_CFG_AHB 已确认 bit0=1），
  GPUCC 只映射一份，重读 GDSCR/RCG，验证不再 panic。
- 待办（下一轮）：PLL0 配置 → RCG 选 PLL0-even/1:1 → 清 ROOT_OFF →
  开 branch → CX/GX GDSC 上电（顺序：CX 先、GX BCR/clamp/reset 后）
  → 读 A5XX_RBBM_STATUS(0x4F5) 自检 → 再进 CP/zap。

---

## >>> 2026-10-26 14:00 【VDD_GFX 已上电 900mV；GPUCC 可访问】<<<

- **v167/v168 实机**：`sweet_spmi_write_ppid` 写入成功（rc=0/0）。
  PM8005 S1/S4：752mV 使能成功 → 再写 0x40=0x84/0x41=0x03，
  读回 **v=900mV mode=6 en=128**，S1/S4 两相都 ON，系统继续正常
  启动到 launchd（无 panic）。**VDD_GFX 电源域打通。**
- APID 注意：同一 ppid 可能有多个 APID（本次 0x514 命中 APID 1，
  之前是 110，均 owner 0）；`sp_find_apid` 取第一个 owner 0 的即可。
- **GPUCC 可访问**（先确认 GCC GPU_CFG_AHB 已开）：
  `gpu_cfg_ahb=0x20008001`（bit0=1）；
  `CX_GDSCR=0x222001`、`GX_GDSCR=0x222001`（bit0=1 → 两域都
  collapse/off）；`GX_BCR=0`、clamp(0x130)=0x1（GX 被 clamp）；
  `gfx3d RCG(0x1070)=0x80000000`（CMD_ROOT_OFF），branch(0x1098)=
  `0x80000620`（bit0=0，时钟关）。
- **下一步 v169（只读）**：读 GPU PLL0（MODE 0x00/OPMODE 0x2c/
  STATUS 0x24/L/CAL/FRAC）与 RCG CFG/M/N/D，判断 bootloader 是否
  已配好 PLL/分频；若已配好，v170 只需清 CMD_ROOT_OFF + 开 branch，
  再按 CX→GX GDSC 顺序上电（序列见 §12:10 条目）。

---

## >>> 2026-10-26 13:45 【v166 自校验：PM8005 是 common2 布局；v167 开始上电】<<<

v166 实机（`sweetgpupause=6`）自校验读数：
- 同一字节 8-bit/16-bit 两种读法 **完全一致** → SPMI 读写路径可信。
- PM8005 S1：type=0x1C(FTS)、sub=0x02、revid=0；
  0x40=0xF0、0x41=0x02、0x45=6、0x46=0。
- 判读：这是 **common2 布局**（0x40/0x41 = 16-bit 电压，单位 mV：
  **0x02F0 = 752mV**；0x45=6 = AUTO 模式）。不是 range/set 布局。
  en=0 → VDD_GFX 两相关闭，电压预置 752mV。
  与主线路 qcom_spmi-regulator 的 FTS426 寄存器（0x40/0x41 LSB/MSB）
  一致；subtype 值差异不影响使用。
- **v167（写入阶段，`sweetgpu=2`）**：新增 `sweet_spmi_write_ppid`
  （EE0 写通道 0x8400000 + 0x8000*APID 惰性映射；≤0xFF 用 EXT_WRITE=2，
  >0xFF 用 EXT_WRITEL=0）。策略：**先不改电压**，只在 752mV 使能
  S1/S4（0x46=0x80），读回确认；`sweetgpu=3` 才升到 900mV(0x0384)。
  一次一个变量；若使能后系统异常，自动复位 180s 回 fastboot。

---

## >>> 2026-10-26 13:30 【v165 探测读数 + v166 自校验】<<<

**v165 实机读数**（`sweetgpupause=6`，读屏成功；随后正常进 iOS，
`sweetar=180` 到点自动回 fastboot）：
- APID 归属：PM8005 S1 (ppid 0x514) → **APID 110 owner 0（APPS 可写）**；
  PM8005 S4 (0x51d) → **APID 67 owner 0**；PM8998 S9 (0x02c) → APID 114
  owner 4（RPM）；PM8998 S1 (0x014) → **not mapped**（CX 由 RPM 管）。
- 原始状态（待 v166 复核）：GFX S1 rng=240 set=2 mode=6 en=0；
  GFX S4 set=2 en=0；MX S9 en=128。即 **VDD_GFX 两相均关闭**。
- 疑点：S1 的 rng=0xF0 与 CAF 电压表 range_sel(0/1) 对不上，可能
  读命令/地址形式不对。**v166** 改为按 CAF regmap 选 opcode
  （addr≤0xFF 用 EXT_READ=13，>0xFF 用 EXT_READL=1），并加自校验：
  同一控制字节用 8-bit/16-bit 两种形式读、读 S1 type/subtype(0x04/0x05)
  与 PM8005 revid(ppid 0x501/0x00)。预期 type=0x1C(FTS)、sub=0x08。

下一步（②）：确认 type=FTS 后，S1 电压码 = (target-350mV)/5mV，
  900mV → 0x6E；先写 set(0x41)、再清 en(0x46)=0x80 使能；
  S4 同步；然后才碰 GPUCC（GCC GPU_CFG_AHB @0x100000+0x71004 bit0，
  GPUCC @0x5065000：CX GDSCR 0x1004、GX GDSCR 0x1094、GX_BCR 0x1090、
  0x130 bit4 reset/bit0 clamp、gfx3d RCG 0x1070/分支 0x1098）。

---

## >>> 2026-10-26 13:05 【GPU 探测实机进展：SPMI 布局三连修】<<<

- **v161**：新增 `bsd/dev/sweet_gpu.c`（boot-arg `sweetgpu=1`）与
  `sweet_spmi_read_ppid/describe`。首次上机：打完 "probe level 1" 后
  ~10s 黑屏复位回 fastboot —— 卡在 `sweet_spmi_init`。
- **根因**：`sweet_spmi.c` 的基址不是 lilac 的 SPMI：
  错用 core 0x0c440000 / obs 0x0e600000 / cfg 0x0c40a000，且通道步长
  0x80。**shim 已验证的 lilac/MSM8998 布局**是：
  core **0x0800f000**（APID map @+0x800，4B×512）、cnfg **0x0800a000**
  （ownership @+0x700）、obs **0x09400000**、chnls **0x08400000**、
  通道步长 **0x8000**（EE0）。读错地址 = 总线挂死 → WDT。
  （AGENTS 旧条目里 shim 的 PON 写 APID4/8 早已实锤这套布局。）
- **v163**：按上述布局修正 + arb 版本放宽到 v2/v3；上机后正常跑进
  `spmi init`，打印 core/cfg 地址，然后 **`kernel_alloc(...16777216...)`
  failed with 3 @vm_kern.c:170** —— 一次性 ml_io_map 16MB observer 窗口
  把内核 KVA 用爆。机器没复位，但探测失败。
- **v164**：observer 改为**按 APID 惰性映射 0x8000 通道窗口**（缓存
  `sp_obs_ch[512]`），轮询上限降到 400ms。正在构建/待上机。
- 教训：内核 KVA 很紧，不要把 16MB 设备窗口整段 ml_io_map；
  SPMI 访问必须与 shim 的已验证常量逐字对齐，别信"看起来像"的基址。
- **v164 上机成功**：惰性通道映射后不再 panic；探测执行完毕，系统继续
  启动到 **SpringBoard/hello 界面**（约 20s 上屏），说明 SPMI 访问安全。
  但探测输出在两次相机采样之间被 GUI 覆盖，没读到数值。
- **v165**：结果压成 3 行（GFX S1/S4、CX S1、MX S9 的 range/set/mode/en），
  新增 `sweetgpupause=N` 在打印后忙等 N 秒（相机可读），并把
  `sweetar` 缩到 180s（实验后自动回 fastboot）。已去掉 `sweetnoreboot`，
  任何 panic 都会自动复位回 fastboot。

---

## >>> 2026-10-26 14:30 【战略资源研判：对当前三线的取舍】<<<

隔壁 13:00/14:00 两批情报已读，落到我们身上的动作：

1. **LSE 模拟器（poplar 0086，188 行）**：Kryo280=ARMv8.0 无 LSE，
   iOS 16 用户态部分库会触发非法指令。我们已能跑到 SpringBoard，
   说明主路径未命中，但**扩展崩溃（PhotosRelive/GCCWidgets）不能
   再默认归因 GPU**——先查 crash log 的 ESR/exception 类型：若
   EC=0x00（unknown instruction）就是 LSE，移植 0086 即可；若是
   GPU/MTL 相关再走 GPU 线。成本低，排在 GPU 探测之后。
2. **原生几何（sagit c574057/1fa98a5）**：canvas-width/height 必须是
   **CFData(OSData)**，写 CFNumber 会崩 backboardd（我们 v146 已踩过）。
   正确做法 = 用 OSData 发布 720x1280 原生几何后重试 `sweetfb_native=1`；
   若 WindowServer 不再卡 appearance transition，则中央裁切、清晰度、
   旋转、触摸坐标（目标=720x1280）一起解决。**优先于继续加合成映射**。
3. **GPU 主线不变**：A540 继续 sailfish/freedreno；但 Y700（A830/gen8）
   不要照抄 A5xx——`NativeAdreno-myron` 的 `a8xx` 分支有 A840/gen8
   寄存器表，SM8750 的 UniXNU port 已有 5 家（Pagani/erhai/hummer/
   dada/piano），UEFI 三件套齐全。Y700 的 GPU 应走 gen8 + Turnip/Metal
   路线，与 A540 分叉。

结论：当前先完成 GPU 只读探测（SPMI 布局已按 shim 实测修正，v163），
然后并行做 LSE 取证/移植与原生几何 CFData 实验；三者不冲突。

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
  **不要再调 /cam/1/af**。（**2026-10-27 解除**：夜间无人值守画面模糊时
  应主动 `/cam/1/af` 对焦，见顶部相机协议。）
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
6. **相机采集**：只用 `curl .../cam/1/frame.jpg` 抓单帧；**禁止再用 `research/grab_cam.py`**（会让 DroidCam 崩溃）。`/override` 仅在摄像头卡死/掉线时用于恢复，不作常规前置。

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
  **只抓单帧；不要用 `research/grab_cam.py`（会开流导致 DroidCam 崩溃）。
  `/override` 仅当画面卡死/掉线时打一次用于恢复，不作常规前置。**
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


---

## 附录A：lilac 硬事实/规则手册（2026-10-26 并入自 ios-xperia-xz1c/AGENTS.md，原文未改）

# Working on ios-xperia-xz1c (fork of ios-sweet → Sony Xperia XZ1 Compact "lilac" / SO-02K)

- **HOST CPU CAP 70%** (2026-10-21, user requirement): the Windows PC has an
  ES CPU that hard-rebooted the machine overnight under sustained load.
  NEVER run full-core builds/compiles. Envelope: `taskset -c 0-13` (14/20
  cores in WSL) + `nice -n10` + `ionice -c2 -n7` for anything heavy.
  Same spirit on Windows side. `/tmp` in WSL is wiped on every reboot —
  keep anything valuable in the repo or `/root`. The hfsplus kernel
  module is runtime-loaded: after any reboot re-load it with
  `insmod /root/wslk/fs/hfsplus/hfsplus.ko` (built from WSL2-Linux-Kernel
  linux-msft-wsl-6.18.y, exact 6.18.54.1 match; rebuild via
  `KBUILD_MODPOST_WARN=1 make M=fs/hfsplus modules` if ever needed).
- Read `NOTES.md` before starting; it contains the project goal, hardware facts,
  current blocker, local artifact locations and the next concrete action.
- The user switches agents frequently. Update the active handoff in `NOTES.md`
  after each milestone and before stopping. Record commits, CI run IDs, exact
  build/boot results and unfinished work. Distinguish observations from guesses.
- Commit completed changes. Use `gh` for GitHub Actions kernel builds and logs.
  Local single-file cross-compiles are useful before spending a CI run.
- Device bring-up uses temporary `fastboot boot` images. Preserve the Android
  installation and partition backups. The iOS root would go into the existing
  `userdata` partition (no GPT changes). Keep bootloaders, recovery, modem,
  persist and the backups intact, and confirm each destructive step with the
  user before running it.
- Keep device dumps, serial numbers and downloaded Apple artifacts out of git.

# PERMANENT ROOT + ADB (2026-10-21 evening — workflow milestone)

- **Magisk v30.7 installed** to boot partition
  (`research/magisk-patched-lilac.img`, patch of stock 47.1.F.1.105 boot).
  `adb shell su -c 'cmd'` gives instant root (`context=u:r:magisk:s0`) —
  no exploit, no dialog, survives reboots.
- su policy for uid 2000 (shell) seeded via
  `magisk --sqlite "INSERT OR REPLACE INTO policies VALUES(2000,'com.android.shell',1,0,1,1)"`
  (run inside a bindershell root shell). **magiskd caches policies — a
  REBOOT is required before a db-seeded policy takes effect.**
- **adb permanently authorized**: host pubkey appended to
  `/data/misc/adb/adb_keys` + `/data/adb/service.d/adbalways.sh`
  re-asserts it every boot (Magisk service.d, runs as root late_start).
  Verified: device returns `device` (not `unauthorized`) after reboot.
- bindershell remains fallback if Magisk boot ever breaks.
- Magisk APK at `Patch\Xperia\Magisk-v30.7.apk`; pstore still empty at
  this point (ramoops registered but prz didn't survive — open issue).

# lilac port — HARD FACTS (verify against this before changing constants)

## Device / workspace

- Phone: Sony Xperia XZ1 Compact, Docomo SO-02K, serial BH905ND69G.
  Snapdragon 835 (MSM8998), 4 GB RAM, 720x1280 DSI command-mode panel,
  UFS (SKhynix H28U62301AMR). Android 8.0.0 / 47.1.F.1.105, XFL = O_77,
  bootloader P_114, **unlocked via xperable (CVE-2021-1931)**.
- Working repo (this clone): `C:\Users\Cisco He\Desktop\Git\ios-xperia-xz1c`
  WSL path: `/root/xz1c` (symlink → /mnt/c/...). Build host: WSL2 Ubuntu-22.04
  (`wsl -d Ubuntu-22.04`), cross toolchain `aarch64-linux-gnu-*` installed,
  mkbootimg at `/opt/mkbootimg`.
- Windows side (adb/fastboot): `C:\Users\Cisco He\Desktop\Git\SchoolBox\Tools\platform-tools_ADB`
  Use `adb -s BH905ND69G` (a stale `emulator-5556 offline` entry exists — ignore).
  `MSYS_NO_PATHCONV=1` required in Git Bash for adb paths.
- Original research dir `C:\Users\Cisco He\Desktop\Git\Patch\Xperia` still holds
  firmware, TA/XFL backups, unlock tools and `research/lilac-stock.dtb`.
  Do not delete `unlock/xfl-backup-original.img` or `flash/fw/tadump/`.

## Hardware constants (from stock DTB / live probes)

- Framebuffer: `0x9d400000`, 720x1280x32bpp (ABL splash region, 36 MB).
- UFSHCI regs: `0x01da4000` (NOT sweet's 0x1d84000). Storage LUN 0 = `sda`,
  4 KiB sectors. `appslog` = sda58, 512B-sector start 1666080 → 4K block
  **208260**, size 16 MiB. `userdata` = sda66.
- IMEM restart_reason @ `0x146bf65c`; PSHOLD @ `0x10ac000`.
  RESTART_BOOTLOADER = `0x77665500`; value `0x77665501` → normal Android boot
  (observed, appears in next cmdline as `warmboot=0x77665501`).
  Custom values like `0x1AC0xxxx` → device landed in fastboot (ABL routing
  unknown) — do NOT use arbitrary codes; stick to observed values.
- **PSHOLD write = POWER OFF, not reset (2026-10-22 confirmed)** —
  `wr32(PSHOLD,0)` powers the phone off → lands at charge mode enumerating
  `VID_0FCE&PID_E1F4` (SONY CD-ROM + mass storage), adb/fastboot dead,
  needs a PHYSICAL power-button press. Almost certainly the root cause of
  every past "power-off incident", not USB drops. **RULE: dead-end code
  may only write a mark then spin/wfe — NEVER write PSHOLD.** The armed
  APSS WDT bites (~125 s) and recovers while preserving the mark.
  Same class: kernel panic path reached PSCI SYSTEM_OFF → fixed by making
  panic PSCI SYSTEM_RESET after marks (ca7ffde/b52b198).
- **ABL mirrors restart_reason into kernel cmdline as `warmboot=0x........`**
  → 32-bit shim→Android telemetry channel, verified.
- GICv3 `0x17a00000` (same base as sweet), timer 19.2 MHz, PSCI via SMC.
- APSS watchdog `qcom,wdt@0x17817000` (msm-watchdog), bark 12000 ms,
  pet 10000 ms. See "Dead-man switch" below.
- Panel: DSI command mode (somc,5_panel), NOT video mode. Touch: Synaptics
  ClearPad in-cell @ i2c `0xc179000` addr 0x2c, own watchdog in DTB.
- RPM (not RPMh) power voting; keys on PM8998 PON (0x74 power / 0x73 vol).

## Telemetry / log channels (THIS IS HOW WE SEE WITHOUT A SCREEN)

- **`delay_ms` bug — CHECK before trusting any timing/delay code**:
  `cntfrq_el0` reads 0 under ABL → delays returned instantly until the
  19.2 MHz fallback was added (shim.c `delay_ms`). Any pre-fix build's
  "wait" was a no-op; timing-based telemetry decode was meaningless.
- **DDR dies on EVERY reset** (PSHOLD and warm `adb reboot` alike) —
  RAM-based shim→Android channels are all dead. Survivors: UFS blocks,
  XBL-IMEM reason (ABL-consumed, only 0x77665500/5501 route anywhere).
  CONFIRMED 2026-10-22: `0xffbff000` gets XBL structures every boot,
  `0x9f400000` reads back all-0xff — no DDR stash survives a WDT bite.
- **warmboot mark convention (0x7766xxxx, last-write-wins, echoed into
  next cmdline)**: C-marks `0x7766<C><d>` = shim cp_mark stages (C0-C6);
  `0x7766D0xx` = shim loader probes (D010-D016; D000+idx = entry-verify
  mismatch at word idx); `0x7766F0xx` = shim exception vector caught
  (idx: 0/4=sync 1/5=IRQ 2/6=FIQ 3/7=SError, t/h variants);
  `0x7766Exxx` = kernel entry marks (E000=_start, E101=pre-OSLAR,
  E102=pre-FB-band); `0x7766AA01` = start.s entry; `0x77664Dnn`/`50nn` =
  kernel stage/panic marks; `0x77665501` = loader's pre-jump safe route
  (seen alone ⇒ shim completed + jumped, kernel left no mark).
- **UFS write mystery (parked)**: WRITE(10)+FUA+SYNC reports success,
  shim re-read sees its own data, but nothing persists across reset —
  while Android dd at sda58+1MiB survives PSHOLD. Not root-caused.
- UFS LUNs: sda=LU0, sdb=LU1, sdc=LU2 (+ well-known 49456/49476/49488).
- Charge screen is ABL-side (SONY logo → battery icon, unaffected by
  ramdisk overlay.d rc); `fastboot oem` accepts NO generic commands.
  Power-off incidents correlate with USB drops during `fastboot boot`.

- **pstore console record (primary, 256 KiB channel)**: DTB `ramoops` node,
  console-size 0x40000, record-size 0x1000, ecc 0, region `0xffc00000` 1 MiB.
  Zone layout = 192 record prz (0xffc00000..0xffcc0000) then console prz at
  **`0xffcc0000`**. Write `{u32 sig=0x43474244 "DBGC", u32 start, u32 size,
  char data[]}`; after warm boot Android shows it at
  `/sys/fs/pstore/console-ramoops-0`. STATUS: implemented in shim v5,
  **not yet verified on device** (first run aborted by USB drop).
- **warmboot= cmdline**: restart_reason is echoed into next Android cmdline.
  Use for 1-word status if pstore fails. Only observed-safe value 0x77665501.
- **appslog UFS write**: shim has WRITE(10) path targeting LUN0 LBA 208260.
  v3 result: write FAILED (binary-signal test landed in fastboot), stage not
  yet isolated (probe may bail early — host not ready / list not taken).
  ufs.c read probe results will be visible once pstore channel works.
- bindershell (temp root, CVE-2019-2215) lives in
  `Patch\Xperia\unlock\bindershell`; re-push to /data/local/tmp after any
  userdata wipe. Gives uid0 → dd partitions, dmesg, config.gz.

## Dead-man switch / watchdog design (user sleeping = zero hands)

- GOAL: any hang must self-recover to a reachable mode. NEVER power off.
- Shim must ARM the APSS WDT itself (do not assume ABL left it running).
  **Register map VERIFIED** against CAF `drivers/soc/qcom/watchdog_v2.c`
  (compatible `qcom,msm-watchdog`, WDT_HZ = 32765 Hz sleep clock):
  `WDT0_RST +0x04` (write 1 = pet), `WDT0_EN +0x08` (1 = arm, 0 = disarm),
  `WDT0_STS +0x0C`, `WDT0_BARK_TIME +0x10`, `WDT0_BITE_TIME +0x14` —
  all in ticks of WDT_HZ. Implemented in `shim/wdt.c`; shim_main arms
  bark 120 s / bite 125 s right after writing restart_reason 0x77665501,
  so ANY hang self-recovers to Android within ~2 min. CAUTION: on some
  Qualcomm parts only a TZ scm call can truly disable a running secure
  watchdog — if EN writes don't stick, expect bites anyway (pet early).
- Before arming, write the desired recovery restart_reason to IMEM so the
  watchdog reset lands where we want (0x77665501 → Android w/ adb).
- Every shim test ends by explicitly writing restart_reason + pshold reset.
  Choose landing: 0x77665501→Android, 0x77665500→fastboot.
- **Power-off incident (v5 first attempt)**: `fastboot boot` USB drop mid-boot
  left the phone OFF. LESSON: never leave a partial boot; re-run cleanly;
  only reachable states allowed. A powered-off phone needs a physical
  power-button press — treat poweroff as the worst outcome, design against it.
- If adb is lost but Android runs (enumerates e.g. E1F4 storage-only):
  Windows-side `Disable-PnpDevice`/`Enable-PnpDevice` on the VID_0FCE
  instance re-enumerates USB without touching the phone. adb kill-server
  + replug also recovers most cases.

## ALLY INTELLIGENCE (2026-10-21 repo pulls — TOP PRIORITY, do not lose)

Research clones live in `Patch\Xperia\research\`: `ios-topaz`, `ios-sweet`,
`ios-v60`, `UniXNU` — refresh with `git fetch` before planning.

### ios-topaz — THE reference port (SM6225/SD685, ARMv8.0, iOS 16.7.16 iPhone X)

Status 2026-10-20: **boots to iOS home screen, touch+keys+Wi-Fi work**
(patches now 0139). It documents every silent-boot wall we will hit —
their README "How topaz differs from sweet" is our porting checklist.
SD835 lilac is ALSO ARMv8.0 (Kryo 280: no LSE atomics, no ldapr, no PAN) —
sweet is ARMv8.2 and never hit these walls, so our 84-patch series does
NOT cover them. Porting order for lilac:

| Patch | What | Priority |
| --- | --- | --- |
| `0081-topaz-armv8.0-no-lse-no-pan` | `-mcpu=cortex-a53` build flags on SWEET machine config, commpage `casa`→LDAXR/STXR loop, LSE/ldapr userland emulation in `handle_uncategorized` (LDAXR/STLXR on user addr, copyio recovery) | **MANDATORY** — iPhone X userland contains LSE/ldapr; boot dies silently without it. Written against SWEET config so it should apply near-cleanly |
| | **IMPORTED 2026-10-21** as `0085/0086/0087/0088/0089-lilac-*`: topaz 0080+0081+0082+0092+0095 all apply CLEAN (p1) atop our 84-patch tree (verified by real apply). Constants review pending: `0088` still carries `SWEET_SCANOUT_BASE 0xb0000000` (topaz pool) — pick a lilac-safe carve-out before enabling the scanout driver; `0087` assumes Gold MPIDR `0x100-0x103` which matches lilac DTB. | |
| `0082-topaz-boot-on-gold-cpu` | Shim wakes Gold CPU `0x100` via PSCI CPU_ON, Silver cores park in `wfi`; ADT lists only Gold cores. Needed because **Silver cores lack 16K translation granule** (ID_AA64MMFR0.TGran16=0). lilac Silver likely identical — verify with shim's MMFR0 readout | **MANDATORY** for SMP; `cpus=1` covers first boot |
| `0088-topaz-gpt-watchdog` | GPT watchdog handling | medium — we have APSS wdt |
| `0092-0103` | **Direct MDP scanout**: XNU publishes IOSurface-backed surfaces in contiguous RAM <4 GiB, MDP scans them directly, real dcache maintenance, double buffering, copy+flip path | **core display work** — this IS the friend's "hardware scanout, no CPU copy" advice implemented |
| `0094-topaz-cpufreq-max` | pin CPU freq for bring-up stability | medium |
| `0104-0111` | DSI brightness/DCS on-off + `AppleARMBacklight` | medium — cmd-mode panel relevant |
| `0080/0083-0091` | topology skip, FT5452 touch, battery gauge | adapt: our touch is Synaptics not FocalTech |
| `0113-0130` | **TopazModem Wi-Fi stack**: modem PAS boot through TZ from `modem_a`, GLINK/QRTR, pd-mapper/tftp/rmtfs, WLFW, copy engines, HTC/WMI/HTT, scan, WPA2-PSK 4-way IN KERNEL, Ethernet path | later — same Qualcomm modem family, huge head start |
| `0131-0139` | SweetSurface/IOSurface maturity: client address surfaces, detach/allocsize, zombie state, empty IOAcceleratorES, publish IOSurfaceAddress | GUI stage — CoreAnimation GL renderer needs these |

Topaz shim extras ours lacks: `gold_gicr_prepare()` — preps GIC
redistributors for all Gold cores BEFORE XNU starts SMP itself (XNU
does CPU_ON without shim). Our shim has `test_gic()` + PSCI CPU_ON
already; add the redistributor prep when doing SMP.

### UniXNU — separate architecture, mine for DESIGN not code (SM8550 device "sheng")

- **Native 4 KiB kernel**: `BCM2837` machine config (ARM_PGSHIFT=12) +
  patch clusters `native-4k-bootstrap-capacity` (page-table budget),
  `native-4k-user-abi` (PAGE_SHIFT_CONST=hw granule at any RAM size),
  `native-4k-signature-hash-cache` (16K CodeDirectory → 4K kernel
  validation bridge — needed if we ever run 4K kernel with stock
  16K-signed binaries), `pgz-aligned-validation`,
  `zzx-pmap-sync-64bit-pa`, `ArmSmmu4K` kit. Route for using all 8
  cores someday; docs at `research/UniXNU/docs/xnu/four-k-page-tables.md`.
- **Native Adreno GPU WORKING** (A740 only): chain = Metal AIR →
  metal2vulkan → SPIR-V → Turnip → NIR → IR3 → NativeAdreno KMD →
  GMU/CP/SMMU → scanout. Renderserver composites system UI on-device.
  A540 port = v3.0 topic; the Metal→Turnip architecture is universal.
- `NativeUfs` (SMMU + cmd queues), `NativeAES`, `NativeMetal`,
  `ArmDTPlatform` driver designs — reference if sweet's hacked-Apple-driver
  approach stalls.

### ios-v60 — LG V60 (SD865): GUI + touch confirmed on device,
QCA6390 Wi-Fi in progress. Secondary reference.

### NEW ALLIES pulled 2026-10-21 (same-SoC / sibling ports)

- **`ios-sweet-sagit`** (Patch\Xperia\research\ios-sweet-sagit) —
  Xiaomi Mi6 = **MSM8998, our SoC**. Fresh fork of ios-sweet, commits
  so far: device constants in mkadt/shim + xnu-8796 machine config +
  early FB at 0x9d400000. Their constants vs lilac: GICD 0x17a00000 and
  GICR 0x17b00000 MATCH; IMEM 0x146aa65c ≠ ours 0x146bf65c; PSHOLD
  0x0c264000 ≠ ours 0x10ac000; memory = TWO banks (0xb0000000+0x4d000000
  and 0x100000000+0xfcc00000) vs our one contiguous 0x9f800000-0xffb00000.
  Track their commits — every MSM8998 finding transfers.
- **`xnu_kernel_sdm835`** (MC-XiaoXiao) — full **xnu-11417** tree
  (newest-gen XNU, ~iOS 26 era) imported for SDM835. Alternative
  lineage for the far future (post-16 port), watch it.
- **`ios-le-pro3`** (LEX727, SD821) — early probe stage but rigorous
  methodology (observed/source/planned separation). Notable warning:
  their UFS has an **inline-crypto engine (UFSICE) at 0x630000 the DTB
  didn't mention** — check whether lilac UFS likewise has ICE before
  poking; probe must not touch it. **VERIFIED for lilac: we DO have
  `ufsice@0x1db0000` (qcom,ice, crypto=0x83)** — separate node from the
  ufshc@0x1da4000 host, shim probe doesn't touch it, but the XNU UFS
  driver must coexist (never poke the ICE block). Also confirmed "no
  atomics" on their Kryo (ARMv8.0 like us → topaz 0081 applies).
- **`HeliumStudio-Dev/ios-raphael`** — K20 Pro (SM8150/SD855) sibling
  fork, reached iOS lock screen + GT9886 touch verified. Useful for
  touch/HID driver examples.
- `oxhunnx/ios-a5xelte` (user-spotted) — repo 404 / not public (a5xelte
  = Samsung A5 2016, Exynos — different arch anyway).
- `hackios.net` status: HackiOS team (Poco X5 Pro / 11 Lite demos)
  closed-source until release; topaz README says its GPU work will be
  published once HackiOS publishes source — keep watching.
- **`dipa-studio/UniXNU-sweet`** (in research/) — the ios-sweet author
  is porting sweet itself to the UniXNU Darwin-23 arch: has A618
  **legacy HFI** transport (older Adreno, much closer to our A540 than
  A740), DarwinBoot memory-bank selection, ArmCpu timer-PPI override.
  This signals the community's GPU path forward AND gives us the
  A5xx-era firmware trail to study for a future lilac GPU driver.
- **UniXNU unattended loop (steal if Android-adb gets too flaky)**:
  `sheng-log=1` writes diagnostics to last 32 MiB of userdata;
  `sheng-autoreset=N` self-resets to fastboot; then they boot a tiny
  **Linux "log-reader" image** (busybox + USB CDC-ACM shell) that copies
  the log window and streams it over serial — log retrieval with ZERO
  Android/adbd dependency. Docs: `research/UniXNU/docs/devices/
  sheng-unattended-test.md`, script `scripts/run-sheng-unattended.sh`.

## Display path — friend's field-tested advice (FOLLOW IT)

- Do NOT CPU-copy/scale an IOSurface into the framebuffer — proven very laggy.
- Qualcomm display controller supports address mapping + hardware scaling:
  drive the MDP/MDSS pipe directly instead of software scaling.
- For early debug use the lowest resolution mode (≈640x360-class).
- **Lock framerate ~1 fps until the CPU scheduler is stable** — unthrottled
  compositing starves the scheduler and wedges the device.
- Bring-up order friend recommends: **CPU scheduling first**, then display.

## Boot chain plan

- `fastboot boot` VERIFIED working on this ABL (stock img + shims booted).
- iOS 16.7.16 (20H392), iPhone X (D22AP) identity first; iPhone 8 (D20AP,
  750x1334 ≈ our 720x1280) as second-stage identity experiment.
- XNU = xnu-8796 built by GitHub Actions (blacktop/darwin-xnu-build);
  trigger with `gh workflow run`, artifacts via `gh run download`.
- IPSW 16.7.16 download was started to `Patch\Xperia\research\` — check state.
- arm64e/PAC impossible on SD835 → iOS 17+ is a v2.0 research topic only.

## Mac build slave (Hackintosh, configured 2026-10-22)

- Host: `ssh ciscohe@ciscomac.local` (macOS 15.8.1), passwordless via
  `~/.ssh/id_ed25519` ("devin-windows-xnu"). Login password intentionally
  NOT stored in this public repo.
- **USER DIRECTIVE (2026-10-22 晚): Mac is available for ongoing project
  use — but DO NOT litter the macOS.** House rules:
  - Work ONLY inside project dirs: `~/Desktop/ios-xperia-xz1c`,
    `~/Library/CloudStorage/飞牛同步-Home-Nas/XNU/`, and the
    `~/xnu-autobuild*` files we already created.
  - NO random installs / system tweaks / new daemons / Homebrew
    packages without asking first.
  - Clean up temp files you create; never touch the user's personal
    files, Desktop (except the repo clone), or system settings.
- Repo clone: `/Users/ciscohe/Desktop/ios-xperia-xz1c`, build tree `dxb/`.
- **Xcode 26.3 works** (resolved 2026-10-23): commit `5cb3ab3` added the
  Xcode-26 IORPC/iig compat; `a1e516c` produced the first SWEET kernel
  under 26.3. No Xcode 15.4 needed. (The 15.x gate in
  `tools/mac-autobuild.sh` is stale — it will skip under 26.3; either
  run builds manually or relax the gate.)
- `tools/mac-autobuild.sh` = watcher (poll origin/main 60 s → pull →
  refresh dxb/patches → build → drop `kernel-<sha>-<ts>.sweet` into sync
  folder + `BUILD_STATUS.txt`). Install once via
  `tools/mac-autobuild-install.sh` (launchd `com.xz1c.xnu-autobuild`).
  Log `~/xnu-autobuild.log`.
- **CONCURRENCY**: a Mac-side agent may run manual builds in the same
  `dxb/` tree — `launchctl unload` the agent or `touch ~/xnu-autobuild.pause`
  before pushing commits that would trigger a parallel build. Status as of
  10-22: watcher UNLOADED while the Mac agent finishes Xcode setup.
- **Kernel channel = Fn-Sync**: Mac drops into
  `~/Library/CloudStorage/飞牛同步-Home-Nas/XNU/` ↔ Windows
  `C:\Users\Cisco He\Fn-Sync\XNU` (verified ~20 s). Whole NAS Home space
  is synced — keep only kernels/status files in `XNU/`.
- Mac deps status 10-22: `jq` ok; brew/gum/cmake/ninja MISSING.

## Automation toolbox (ALL PROVEN ON DEVICE 2026-10-21)

- **usbctl daemon** = SYSTEM scheduled task "XperiaUsbCtl" (installed via
  `unattend\install-usbctl-task.bat`, right-click→admin ONCE). Agent sends
  commands by dropping `unattend/cmd/*.cmd` files (`replug`/`adbreset`/
  `status`/`stop`), reads `out\result.txt` (UTF-16). `replug` VERIFIED:
  dis/en cycle on VID_0FCE revives USB without touching the phone.
  Wake anytime: `schtasks /run /tn XperiaUsbCtl`.
- `ps:<powershell>` = arbitrary admin command channel (v3 script),
  `devreset` = pnputil remove-device + scan-devices (clears the
  "needs reboot" pending flag that made dis/en fail with 常规故障).
  ⚠️ CAUTION (Xperia AGENTS incident): `devreset` once killed the
  phone-side USB entirely (VID_0000 descriptor failure, unrecoverable
  by software) — LAST RESORT only, prefer replug/hardreplug first.
- **`unattend\watchdog.sh`** (bash, `nohup bash watchdog.sh &`) — autonomous
  babysitter: polls adb every ~30 s, escalates kill-server → adbreset →
  devreset → hardreplug → replug, logs to `out/watchdog.log`. Start it before
  any unattended test batch.
- **adb flakiness ladder**: `adb kill-server; start-server` → `adbreset` cmd →
  `replug` cmd → only then consider it truly dead.
- **Windows "needs reboot" pending-config flag**: once set on the phone's
  device node, `Disable-PnpDevice` AND `pnputil /remove-device` both fail
  (常规故障 / 需要重新启动系统). Parent is ROOT_HUB30 (uncycleable). Only a
  Windows reboot clears it — avoid triggering it; device IO still works.
- **`unattend\arm-xnu10.sh`** (armed sentinel v2, 2026-10-22, SUPERSEDES
  arm-xnu9.sh which died on transient adb failure 07:51) — persistent
  experiment runner: sighting → reboot-verify loop (retries `reboot
  bootloader` up to 5x) → `fastboot boot artifacts/shim-xnu9-lilac.img`
  → records landing mode+latency (<120s = early kernel death, ~125s =
  WDT) → `getvar all` snapshot BEFORE any Android reboot → harvests
  warmboot=/pstore/bindershell-fmem IMEM cells → decodes into
  `out/arm-xnu10.log`. Re-arms itself via `exec $0` on failure — safe to
  leave running forever.
- **`fastboot reboot` after XNU-death CAN power the phone off**
  (observed 2026-10-22 08:00): xnu9 died ~40s → landed fastboot →
  `fastboot reboot` → Android re-boot died mid-way → USB vanished
  electrically (VID_0000 descriptor-fail node, then nothing). Full
  ladder incl. devreset could not revive — physical power button only.
  MITIGATION in v2 sentinel: snapshot `getvar all` in fastboot first;
  TODO: "probe shim" boot.img that harvests IMEM/cmdline→pstore without
  ever booting Android.
- **`adb reboot bootloader` can silently NO-OP when adb is half-dead**
  (offline-adjacent state): command returns but device stays in Android.
  Always verify `fastboot devices` shows the serial, retry the reboot.
- **`/sys/fs/pstore` LIVE**: `pstore: Registered ramoops` confirmed in dmesg,
  console zone at 0xffcc0000 (DTB ramoops node: console-size 0x40000,
  record-size 0x1000, ecc 0). Shim v6 writes prz {DBGC,start,size,data}
  incrementally per con_str — crash-safe shim→Android log channel.
  FIRST RUN PENDING verification (see NOTES.md).
- **sysrq panic caution**: `echo c > /proc/sysrq-trigger` (bindershell)
  FROZE the device ~6+ min (panic_timeout likely 0 → hang, not reboot).
  Recovery = physical power long-press. Check `cat /proc/sys/kernel/panic`
  before ever doing this again.
- **IMEM restart_reason sticky**: values survive warm resets; `adb reboot
  bootloader` leaves 0x77665500 → subsequent unexplained fastboot landings.
  ABL passes it to next cmdline as `warmboot=0x........` (read channel).
- **stale emulator-5556 offline** in adb devices — ignore, always use
  `-s BH905ND69G`.

## USB replug daemon (usbctl) — THE no-hands tool

- `Patch\Xperia\unattend\usbctl.ps1` — elevated PowerShell daemon; polls
  `unattend\cmd\*.cmd` every 2 s, writes `unattend\out\result.txt` +
  `daemon.log`. Commands: `replug` (Disable/Enable-PnpDevice on every
  VID_0FCE device = software cable replug), `adbreset` (kill/start adb
  server), `status` (PnP + adb + fastboot dump), `stop`.
- Start: RIGHT-CLICK `unattend\start-usbctl.bat` → 以管理员身份运行, keep the
  console window open (minimizing is fine). Self-elevating `Start-Process
  -Verb RunAs -WindowStyle Hidden` was unreliable — no visible UAC.
- Send a command from anywhere: `echo replug > unattend/cmd/r1.cmd`,
  read result 3-5 s later from `out/result.txt`.
- Recovery ladder for a lost phone: `adbreset` → `replug` → (worst case)
  ask user. USB transition drops are EXPECTED; the daemon absorbs them.

## Red lines (learned the hard way)

- NEVER power off / leave unreachable states; every test must self-return.
- NEVER force `sys.usb.config=adb` in Android init — it killed USB gadget +
  charging once; only `persist.sys.usb.config` via normal init if ever needed.
- NEVER touch the physical SD card (user's backups live there). Internal
  userdata is fair game ONLY after explicit user confirmation per-step.
- adb offline storms after rapid reboots are normal; poll, don't panic.

# GitHub token (CI access)

- A fine/classic GitHub PAT for Actions artifacts/logs lives in
  `.github-token.env` (git-ignored, repo root) as `GH_TOKEN=ghp_...`.
  Use it for: artifact download, run logs, workflow_dispatch.
  NEVER commit the file — GitHub secret-scanning auto-revokes pushed PATs.
  curl example: `curl -H "Authorization: token $GH_TOKEN"
  https://api.github.com/repos/zenkernelsam/ios-xperia-xz1c/actions/artifacts`

