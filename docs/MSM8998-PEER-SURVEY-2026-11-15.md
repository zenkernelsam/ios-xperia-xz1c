# MSM8998 同行进度与可复用代码 — 2026-11-15

> 历史技术记录：下文的版本、结果、“当前”和“下一步”对应原实验日期。现行产物及设备状态见[STATUS](STATUS.md)，构建与单发规则见[BUILD](BUILD.md)／[OPERATIONS](OPERATIONS.md)。原文与SHA见[本轮归档](archive/20261123/poster-state-r22-close-out/original-document-manifest.json)。

这是2026-11-15的固定调查快照；后续实际借用sampler／MSAA／presentation实现与本机真机结果见[STATUS](STATUS.md)和各能力专题。下文旧行号／待实现描述适用于当时源，不代表最新代码或已验硬件。

最值得借用的是 MC-XiaoXiao 的 `ios_sagit`：同为 Adreno 540，公开了原生 Metal、AIR→NIR→IR3、IOSurface 与显示交接代码。我们已使用其 2026-10-23 的部分前端；最新远端增加了 42 个提交，包含系统合成、反馈采样和锁屏／唤醒修复。建议恢复工作后先补原系统 shader 的常量 sampler 与首次管线编译，再核对显示交接；直连 NIR 作为后续编译性能方向。

本次只核对公开网页、Git 远端、源码及现有本地记录，未编译、启动或操作手机。长期实机目标保持暂停，registry-short-r6 未投递。上游的真机叙述在下文标作“上游报告”，不算本项目重新验收。

## 官方名单的范围

读取 [UniXNU Device List](https://unixnu.com/devices/) 及其源仓库 `3aee8d0a2b1daa7e2ac2cb029804db7c663fe245`。当次名单的 MSM8998 项共 **5 款机型、9 个社区仓库记录**：cheeseburger、chiron、dumpling、nash、sagit。官方维护的设备只有 sheng / SM8550；名单收录不表示 UniXNU 组织维护这些 835 项目。

官网有些页面只有链接或较旧阶段摘要，故下面同时核对维护者当前分支。日期为本次读到的提交日期，具体时区及完整 SHA 在 [调查输入](../reproducibility/unixnu-msm8998-survey-20261115.json)。

| 机型／维护仓库 | 本次核对提交 | 公开进度与边界 | GPU／显示复用价值 |
| --- | --- | --- | --- |
| 一加 5 / [DinnerABC](https://github.com/DinnerABC/UniXNU-cheeseburger) | `199a50c2`，11-13 | 上游报告 iPadOS 17.7.11 桌面、真实触控、8 核与持久 Data。桌面候选使用 GOP CPU 呈现；系统 Metal 开启时 backboardd 曾 SIGABRT。 | A540 BO／SMMU／提交／fence、受控 AIR 四帧读回已实现；系统 GPU 合成未完成。官网 GPU 的 `implemented` 也没有被记录为 `device-verified`。 |
| 一加 5 / [lss717](https://github.com/lss717/UniXNU-cheeseburger) | `4f533e88`，11-15 | 已迁移官方 format 2、共享 MSM8998 profile；Mac 内核、payload、EFI 与 boot image 构建通过。迁移后真机尚未重新验收，正式 UFS root 路径待 A/B/C/D。 | 当前 GPU 为 planned。共享 SoC／构建角色整理可参考，暂无可直接移入的完整 GPU 方案。 |
| 一加 5 / [maozedongae](https://github.com/maozedongae/UniXNU-cheeseburger) | `4b43cc63`，10-25 | 板级配置与预链接 roots 骨架；未找到新的设备运行记录。 | 未找到 A540 加速、系统合成或显示唤醒实现证据。 |
| 小米 MIX 2 / [to-mid](https://github.com/to-mid/ios-party) | `6147e4c3`，10-29 | 当前独立 HackiOS 8796 / iOS 16.5 路线有桌面、真实触控与八核的现场成功反馈，仍卡顿；系统卷来自驻留 loader 的 RAM。官网另述的最小 4 KiB XNU＋PID1 是较早的独立路线。 | 触控／SMP／运行记录有用；没有可复用的已验证 GPU 合成后端。不能把 QEMU 桌面或 RAM root 记成原生 UFS。 |
| 小米 MIX 2 / [sbccc1888](https://github.com/sbccc1888/ios-party) | `977861ad`，11-08 | 继承上述桌面基线；新增 A540 fw2、GPUCC、GPMU、SMMU、CP 微码／ring 的主机模型与探针。11-08 专题明确“没有一行在手机上跑过”。 | 固件结构、寄存器来源和 SMMU 参数校核可参考；不把模型通过或 probe 打包当 GPU 上机通过。 |
| 一加 5T / [The-Sunder-islands](https://github.com/The-Sunder-islands/UniXNU-dumpling) | `23832188`，11-10 | 11-09 上游报告 UEFI＋UniXNU 菜单、DumplingInfo 与实体按键成功。XNU 尚未启动，root／launchd／原生触控待测。 | GOP 参数与固件集成参考；暂无 XNU GPU 驱动。 |
| Moto Z2 Force / [fcxuhaq3](https://github.com/fcxuhaq3/UniXNU-nash) | `e06dcbc8`，10-30 | nash 板级配置、早期 ARM／panic 区域补丁；目标设备的启动证据尚待发布。README 仍写 OnePlus 5 并引用 cheeseburger 的 G1，不能据此授予 Moto 实机资格。 | 暂无目标设备 GPU 加速证据；只参考通用平台描述。 |
| 小米 6 / [dmnakzkahziamnk-a11y](https://github.com/dmnakzkahziamnk-a11y/UniXNU-sagit) | 无可访问提交 | 官网提示公开访问不可用；本次 GitHub API 返回 404。无法区分改名、私有或移除。 | 当前无法审计代码及运行进度。 |
| 小米 6 / [ilertnost](https://github.com/ilertnost/unixnu-sagit) | `e219b06b`，11-11 | porting.toml 报告 boot ladder 到 checkpoint 35、卡 STARTUP_SUB_KPRINTF；最新提交修 ADT 的 interrupt-controller/master，但没有新真机完成记录。官网暂未接受其 OS profile 不匹配的记录。 | GPU 分拆到 Adrenoline；当前只有 stub，无功能后端。启动诊断可参考，不能作为完成的 GPU 作业。 |

名单页入口：[一加 5](https://unixnu.com/devices/models/cheeseburger)、[MIX 2](https://unixnu.com/devices/models/chiron)、[一加 5T](https://unixnu.com/devices/models/dumpling)、[Moto Z2 Force](https://unixnu.com/devices/models/nash)、[小米 6](https://unixnu.com/devices/models/sagit)。

## 名单外的重要参考

| 仓库 | 当前提交 | 查到的实际价值与限制 |
| --- | --- | --- |
| [MC-XiaoXiao/ios_sagit](https://github.com/MC-XiaoXiao/ios_sagit) | `673f85070fe9f7872793015eb230a5c3987d4a8c` | 主线为 XNU 11417 / Darwin 24.6、iPadOS 18.7.10 / 22H374。README 报告 UFS/APFS、桌面／触控、A540 Compute/Render、IOSurface 回读和系统合成；标作 Partial，API 和长期稳定性仍不足。最新提交还报告锁屏／唤醒 2096 个 native jobs 无失败。当前公开 tree 没有 docs 原日志，也没有 GitHub Release；上述数量是维护者提交叙述，本次仅核对到配套源码。 |
| [nftuoa/ios-sweet-sagit](https://github.com/nftuoa/ios-sweet-sagit) | `c00c0a817f9fe3d4123fe0cb0b287befbe7f181d` | 最新 0143 是 A540 SMMU 只读探针／SID0 页表；注释记录 0142 CP 首次 fetch 停滞。QUP/F12 和同代 XNU 适配仍可参考。README 大部分还是 sweet / SM7150 的原文，其桌面声明不能直接归给 Mi6。 |
| [wakanameko/ios-poplar](https://github.com/wakanameko/ios-poplar) | `b056ffb3f506d190f00b1f30a7235b99816cf608` | Sony XZ1 同平台的 ARMv8.0／commpage／早期映射参考。README 仍大量保留 sweet 的机型和成果，未找到可授予 poplar 的 GPU 合成记录。 |
| [baozi977/xzp-ios](https://github.com/baozi977/xzp-ios) | `f5aa4a306f0dfc5f164b4a1fc2a981ef80ba5bc6` | Sony XZ Premium / SO-04J；上游阶段四摘要报告 APFS、原始 launchd／notifyd／cfprefsd、IOSurface 接口验证。README 明确锁屏／SpringBoard、物理呈现、触控和持久 Data 后续再做；摘要没有提供完整原日志。 |
| [ilertnost/Adrenoline](https://github.com/ilertnost/Adrenoline) | `e9da6e76bfecf17b16efb7602ca95a2bdcc1645e` | README 描述 Metal→NIR→PM4／Gallium 的目标架构。实际 tree 只有 `backends/stub`，明确 “No backend is functional”。其 20H392 CA shader 清单及 MTLB 工具适合调查输入，不是可工作的替代驱动。 |

## 优先借用的实现

### 1. AIR 常量 sampler：已经定位到本项目的真实缺口

我们 [metal_render.c](../gpu/sweetgl/metal_render.c) 的 `bind_samples`（313 行起）只允许普通 `_qsampleN_M` 与 `_level`，328 行拒绝其它尾缀，329 行拒绝 `sampler>=8`。现有 AIR 前端则已生成 `_c_...` 常量状态和逻辑 slot≥8。即使星空管线成功编译，原采样调用仍可能在这里被拒绝。

可借用的上游契约：

- [Semantic.h:156](https://github.com/MC-XiaoXiao/ios_sagit/blob/673f85070fe9f7872793015eb230a5c3987d4a8c/tools/gpu-compiler/Semantic.h#L156)：解析 Apple 一字／两字 constexpr 编码，校验 filter、LOD、address、anisotropy、border、normalized。
- [Interface.h:409](https://github.com/MC-XiaoXiao/ios_sagit/blob/673f85070fe9f7872793015eb230a5c3987d4a8c/tools/gpu-compiler/Interface.h#L409)：完整解析 `_c_` 七字段，检查同一逻辑 literal 的一致性，并独立分配真正硬件 sampler。

应把语义状态映射到当前 GLES sampler／纹理绑定，保留实际纹理单元、LOD 和格式限制；上游的 NIR 虚拟 binding 编号不能直接当 GL 单元。该缺口与 r5 首管线编译超时是两个问题，不能声称补 sampler 就已修复超时。

### 2. 冷编译：先做真实变体缓存／预热，再评估 AIR→NIR

本项目当前原 starfield 的 host 无缓存链接由 5.267 降至 1.991 秒，但它不是手机时耗或系统 UI 成功。优先把原 shader、真实 function constants、顶点布局、两个颜色附件与 blend 状态组成缓存身份，记录 AIR lowering、GL compile/link、等待各自耗时；预热必须覆盖实际使用的变体。

最新 [AdrenoMetal 的 priming 文档](https://github.com/UniXNU/AdrenoMetal/blob/19bb66cb25f9a25b983bd7be15ea697ac7ed5203/README.md#shader-caches-and-priming) 和 `PrimeShaders.h`、`SharedShaderCache.h` 已使用阶段标签／反射常量／vertex inputs，分别记录成功、拒绝及中断函数。可参考清单与缓存身份、失败记录和后台调度；其 Vulkan pipeline cache 与 A740 的精度策略需要另作 GLES 适配。

同 A540 的中期方向是：

- [Compiler.cpp:93](https://github.com/MC-XiaoXiao/ios_sagit/blob/673f85070fe9f7872793015eb230a5c3987d4a8c/tools/gpu-compiler/Compiler.cpp#L93)：优先直接 NIR，失败缓存按原 AIR、entry、constants、registry 区分，失败后完整回退，计时分别记录。
- [AIRNIR.cpp:10](https://github.com/MC-XiaoXiao/ios_sagit/blob/673f85070fe9f7872793015eb230a5c3987d4a8c/tools/gpu-compiler/AIRNIR.cpp#L10) 与 [AIRNIRLowering.h](https://github.com/MC-XiaoXiao/ios_sagit/blob/673f85070fe9f7872793015eb230a5c3987d4a8c/tools/gpu-compiler/AIRNIRLowering.h)：通过 LLVM C API 直接生成 NIR，交给 `qgpu_compile_nir`／IR3。

这条路径可减少 AIR→ESSL→GLSL linker 的重复工作，源码已有实现；对接我们的 Gallium/winsys、反射／资源和 fence 仍需要独立工程与实机验证。没有本机性能数值支持“换上就更快”。当前 kernel/SweetScatter 接口维持冻结，先研究纯用户态／Mesa 对接。

### 3. 显示与锁屏：借用 Surface 等待和新帧亮屏的状态模型

- [673f8507](https://github.com/MC-XiaoXiao/ios_sagit/commit/673f85070fe9f7872793015eb230a5c3987d4a8c)：原 GPU 因尚未提交的 IOSurface reader 等待跨过熄屏，被 admission age 判为全设备故障。修复保留该等待，watchdog 继续约束真正执行的命令。上游报告控制中心和 30 秒锁屏／唤醒循环，2096 jobs、0 failure/recovery。
- [4f738d1](https://github.com/MC-XiaoXiao/ios_sagit/commit/4f738d1030dc03b5c69c8784d346f272503003d7)／[DisplayWakeGate.h](https://github.com/MC-XiaoXiao/ios_sagit/blob/673f85070fe9f7872793015eb230a5c3987d4a8c/drivers/display/QcomDisplay/DisplayWakeGate.h)：唤醒 barrier、之后提交的新帧、真实显示完成三者同时满足才亮背光；旧 retained frame／旧完成中断不足以亮屏。

这与我们曾见旧帧、黑屏唤醒失败的现象相关，但尚不能认定是本机根因。恢复后要分别看原 draw／fence、Surface reader retirement、MDP 完成和背光 ACK。iOS18 的私有 framebuffer 状态偏移、Mi6 面板和 WLED 接线不能搬到 lilac；设备回程 deadline 也不能因 Surface 等待而取消。

### 4. 合成负载：按能力参考反馈采样和资源复用

[9d49c2d](https://github.com/MC-XiaoXiao/ios_sagit/commit/9d49c2d5e838efd49efa72beace2034e0df8c2e6) 在单采样 UNORM line draw 中使用按图元顺序的 barrier，减少控制中心边缘的逐线 framebuffer snapshot；提交报告 24 个 pixel cases，IB 435→31。相关实现是 `AdrenoPipeline.h`。这是 native PM4 路径的优化；当前 FD540 GLES 没有已验证的 framebuffer-fetch／float blend 能力，不能直接用未支持的 GL barrier，也不能把上述 IB 数推成我们手机的 FPS。

当前纹理解码、mip 与 MRT 已有明确字节证明；后续优化仍需保持原像素、低位、保护区、生命周期和多客户端 Surface 交接。先使真实系统 shader 与 present 工作，再扩大这些优化。

## Vulkan 与其它外设的结论

[Mesa 官方 Freedreno 文档](https://docs.mesa3d.org/drivers/freedreno.html#turnip) 对 Turnip 明确写道：**“There are no plans to port to a5xx or earlier GPUs.”** Freedreno GL/GLES 已支持 A540；这与 Turnip 的硬件范围不同。最新 UniXNU AdrenoMetal 的实机合成验证在 sheng / A740；它不是现成的 835 Vulkan 驱动。现阶段借用 Metal/AIR/NIR/缓存代码继续现有 Freedreno 路径最切合已有验证。

名单内未查到可直接复用的完整 Wi-Fi、蓝牙、音频、深睡唤醒成果。MC-XiaoXiao 的表也把 Wi-Fi／蓝牙／声音标为不可用或未验证；其原生背光、SDR EDR、PMIC 读数／通知、RTC 和电源状态模型有较明确源码可参考。Sony 需要自己的 PMIC、面板、无线固件和路由事实，尤其充电写控制不能仅因 SoC 相同就照搬。

## 本地参考与恢复后的顺序

`research` 新增三个纯 Git 参考仓库：`UniXNU-dumpling`、`unixnu-sagit-ilertnost`、`Adrenoline`，总计 89 个；网页快照与一次性输入在 Windows TEMP。既有参考的远端 refs 已 fetch，原固定 checkout、私有输入和旧结果保留。UniXNU 参考工作分支本来已有本地提交，未 pull／reset；`NativeMetal` 上游地址当前重定向到 `AdrenoMetal`，两个目录原 checkout 均保留。

正式源码仍在 ios-xperia-xz1c。恢复研究或测试后优先：

1. 核对原 starfield shader 的编译阶段和真实 variant 缓存／预热；补 constant sampler 的生产绑定契约。
2. 用相同版本完整 GPU 先验验证该改动，再验原 Aegir 的实际纹理／壁纸／完整 UI；保存原失败，不以离屏通过代替系统结果。
3. 跟踪 Surface reader 与显示完成／亮屏状态，验证锁屏和真实唤醒。
4. 独立评估直连 NIR、反馈采样和资源池的收益；不先改已冻结内核或整包切到 iOS18。

手机授权与操作继续遵守 OPERATIONS；本报告没有恢复 paused 目标或创建新的测试轮。
