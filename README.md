# iOS on Xperia XZ1 Compact — lilac / SO-02K

Sony Xperia XZ1 Compact（Snapdragon835 / Adreno540）的开源XNU、shim与Mesa/GLES移植。本仓库是唯一正式源码及操作入口；Patch/Xperia保存参考Git仓库、受保护输入与本地历史。GPU后端使用固定Mesa/FD540 GLES3.1。

实机已验证8核CPU、launchd、F12触控/音量键和以下GPU基线。最新设备状态、当前token与未完成项集中在[STATUS](docs/STATUS.md)，原失败及历史里程碑保留各专题证据。

| 范围 | 已验证内容 |
| --- | --- |
| GLES/CA | 750×1624绘制/读回、IOSurface共享、caller页面持有、动态纹理、上下文隔离；160次relink/320draw/20480像素零差异 |
| [只读Surface](docs/SURFACE-READONLY.md) | 原只读页面及非64B对齐刷新；三个400×400偏移共960000像素零差异 |
| [原生Metal后台](docs/METAL-BACKBOARD.md) | 资源/queue、MTLB/AIR compute、vertex/fragment、function constants、四XR格式、单plane IOSurface、六级mipmap及LOD/trilinear |
| [窄整数](docs/AIR-NARROW.md)/[half](docs/AIR-HALF.md) | 动态signed/unsigned8/16和binary16存储/逐结果舍入/顶点片元；原字节、NaN类别/payload及保护区分别复核 |
| [常量 sampler](docs/METAL-SAMPLERS.md) | 原constexpr字、一字/两字global、两种独立状态、显式LOD；147456B输入/结果/顶点/全部padding实机零差异 |
| [程序二进制缓存](docs/PROGRAM-CACHE.md) | 标准Mesa binary在3个fresh pipeline恢复；sampler147456B/1881pixels、half131072B与MRT1245184B同轮保持，完整13组原生GPU通过 |
| [静态half位型](docs/AIR-STATIC-HALF.md) | 常量内存half/i16四种原位型读写、12case/196608B输出与guard、创建后reset的snapshot完整实机通过 |
| [half链接优化](docs/HALF-SELECTION.md) | 全float32/binary16等位型验证；新Metal完整14组实机通过，原真实request标准binary已预热 |
| [XR Clear优化](docs/XR-CLEAR.md) | 保留原Clear/位型写回与Load，省去覆盖旧色的解码；新GLES完整14组通过，真实750×1624 Clear/draw已出现 |
| [原Aegir短登记](docs/METAL-REGISTRY-SHORT.md) | 原默认device返回真实A540，threshold管线编译成功；必须有同kernel/GLES/Metal/source-map的完整实机先验 |

[多颜色附件与XR混色](docs/METAL-MRT.md)现已完整实机通过：8附件输出、四XR与BGRA独立blend/writeMask、FP32 image按图元有序混色；1245184B原页面/低位/保护区零差异，523全撤销并38.562秒完整自动回程。当前half-selection-r1已完整14组/39.062秒回程验证；原Aegir现已恢复五条真实管线并出现两次750×1624 GPU draw，check-in超时和不完整面板仍保留。新的[原快照耗时观察](docs/POSTDRAW-OBSERVATION.md)已取得一次32179B真实PNG返回；快照约3.31秒（含队列等待2.81秒）、PNG/ICC约3.91秒，原check-in失败及不完整UI仍保留。未完成真实锁屏时间/日期/壁纸、完整UI/FPS、睡眠唤醒、声音、Wi-Fi/蓝牙及广义texture/depth/stencil/MSAA。离屏GPU通过不代表完整系统完成。

内核修补为xnu/lilac-current.patch及版本诊断增量，用户态源码在gpu/sweetgl与gpu/metal，Mesa修补在gpu/mesa-lilac.patch。固定源码锁、逐源重建和实际输入/结果在reproducibility；当前v729保持81源实际审计，allocator/SCM/PON/RPC/VM冻结。构建/验收入口见[BUILD](docs/BUILD.md)，root配方见[ROOT-GL5](docs/ROOT-GL5.md)。SDK/构建时间可改变产物UUID/SHA，源码重建与新二进制实机准入分别验证。

实机仅BH905ND69G、fastboot -s、合格COM>=10、PON只0x06。单download/boot、失败不重发；每轮完整ACK/DRAIN/RESET/FINAL_DRAIN、连续健康30秒及旧worker退出后继续。相机只用override→AF→2秒→MJPEG每5帧抽1→rotate90。完整规则在[OPERATIONS](docs/OPERATIONS.md)。

iPhoneX iOS16.7.16/20H392镜像、Sony原厂固件、设备备份由使用者提供，不进入Git。gl5/userdata/SD/TA/XFL与救砖资料保持受保护。目录/归档/原生回收规则见[LAYOUT](docs/LAYOUT.md)及整理记录[2026-11-12](docs/ORGANIZATION-2026-11-12.md)、[2026-11-15](docs/ORGANIZATION-2026-11-15.md)。

2026-11-17当前GLES为72d39da5，Metal5e94ab57和内核v729保持；新库完整14组及真实请求预热通过。单个原界面样本的队列wait由约2.807秒降至1.067秒，但CPU管线开销和完整UI仍未解决，总快照改善不作推断。原数据与限制见STATUS/XR-CLEAR。

最新原UIImage输入已确认为RGBCIF10/ExtendedSRGB，PNG仍未在原期限内返回；AMD/x86主机的8/16位简化候选不等，未作为设备修复。作者图像的iOS后台原PNG参考与准入边界见POSTDRAW-OBSERVATION及STATUS，完整UI和其它外设继续推进。

作者CIF10图像的iOS后台原PNG参考已实机完成：新RAM CPU及完整14组GPU/五组PNG均自动回程通过。真实输出16位sRGB且有跨通道转换；原界面的PNG期限、完整UI/FPS和其它外设仍继续。详细边界与复现入口集中在STATUS/POSTDRAW-OBSERVATION。
