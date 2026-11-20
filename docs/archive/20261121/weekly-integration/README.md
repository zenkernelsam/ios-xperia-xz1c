# iOS on Xperia XZ1 Compact — lilac / SO-02K

2026-11-20用户明确确认重新按音量上插线并亮蓝灯，物理新fastboot会话恢复。新GLES21de自身CPU87c89bcf与完整GLES9／929a3531实机通过，CA矩阵6阶段max_diff0，150.047秒720×1280扫描图仍灰字／黑壁纸；Metal8feea459 reference CPU367bebc6通过，完整native21 GPU011e16b3却在Depth32／Stencil8／Memoryless／MSAA首phase原字节失败，保留REVISE／DEPTH32_PROBE_FAILED，presentation与PNG未执行。独立失败页observer7fdb9f19自身CPU b798b65b通过，GPU62fb2e55读回12页完整FNV／SHA；颜色页与guard精确正确，深度和模板写入非线性位置。各轮单download／boot／0x06四marker／健康30秒／worker退出均完整，最近回程38.922秒，当前健康fastboot无in-flight。旧r4／r5及本两失败包不重发，生产CA基线16组保持，不能用局部成功放行当前native21。实际报告见ATTACHMENT-HARDWARE及reproducibility/attachment-hardware-r1-20261120.json。

Sony Xperia XZ1 Compact（Snapdragon835／Adreno540）的开源XNU、shim和Mesa/GLES移植。唯一正式源码／操作仓库是ios-xperia-xz1c，Patch/Xperia只保存参考Git仓库与本地历史。GPU后端为固定Mesa24.3.4／Freedreno FD540 GLES3.1，尚未移植Vulkan。

当前v730／CA矩阵修订r1已实机恢复灰色锁屏时间和日期，单次串口上滑后的完整720×1280扫描帧显示正常主页图标、文字、小组件和Dock。黑壁纸、动态UI／FPS、睡眠唤醒、LCD光学／背光、声音和Wi-Fi／蓝牙仍未完成；原Aegir未发出pipeline请求的STOP如实保留。最新状态集中在 [STATUS](docs/STATUS.md)，图像／阶段成功不代表整机已完成。

| 已验证范围 | 实机内容 |
| --- | --- |
| CPU／系统与输入 | 8核CPU、launchd、F12触控和音量键；合成上滑另列 |
| [GLES／Surface](docs/SURFACE-READONLY.md) | 750×1624绘制读回、原IOSurface共享／持有、960000只读像素、动态纹理／上下文、160relink／320draw／20480像素零差异 |
| [原生Metal](docs/METAL-BACKBOARD.md) | 资源／queue、MTLB／AIR compute-vertex-fragment、constants、XR、IOSurface、mipmap／LOD |
| [窄整数](docs/AIR-NARROW.md)／[half](docs/AIR-HALF.md)／[static-half](docs/AIR-STATIC-HALF.md) | 原输入／位型／结果／guard、specialization与资源撤销 |
| [MRT与XR](docs/METAL-MRT.md)／[sampler](docs/METAL-SAMPLERS.md) | 八附件／独立blend-writeMask、1245184B原页及147456Bsampler完整检查 |
| [程序cache](docs/PROGRAM-CACHE.md)／[IR3cache](docs/IR3-VARIANT-CACHE.md) | fresh标准binary与原硬件variant恢复，当前完整native16重新准入 |
| [CA矩阵补全](docs/CA-MATRIX.md) | 准确20H392绑定0x14／0x2d，生产GLI六阶段1536B／192Bguard最大差异0；GLES原8＋新1组 |
| [串口截图](docs/SCANOUT-SCREENSHOT.md) | 无损真实720×1280／stride2160 PNG；锁屏和上滑主页可直接取证，光学另验 |
| [绘制／PNG观察](docs/POSTDRAW-OBSERVATION.md) | 原IR3库曾26nonnull管线／三次draw／PNG返回，新矩阵库完整native16／六组PNG证明 |

当前精确GLES为d6d8fcfa、Metal5e94ab57／source-map7b5a083a、内核v730/f258dee9。构建入口 [BUILD](docs/BUILD.md)，实机／无人值守回程 [OPERATIONS](docs/OPERATIONS.md)，原root配方 [ROOT-GL5](docs/ROOT-GL5.md)。所有新库须自己的CPU／GPU／native／PNG原始实机证明，源码可重建与新二进制准入分别验证。

2026-11-20正式源码新增[Depth32／Stencil8附件候选](docs/METAL-DEPTH-STENCIL.md)：独立原页面、front-back比较／操作／掩码、独立Load／Clear和引用回收已通过离线构建与主机验证，新库完整native18／PNG6实机准入仍待完成，已验基线保持。串口720×1280无损截图已实机可用，日常UI取证不依赖摄像头，面板光学另验。

后续[Memoryless附件候选](docs/METAL-MEMORYLESS.md)已补颜色／深度／模板临时存储、每渲染通道独立页面和错误回收；原生对象、主机Mesa像素与iOS ARM64构建通过，新库完整native19／PNG6仍待真机验收，生产基线保持。

最新[2×／4× MSAA候选](docs/METAL-MULTISAMPLE.md)补多采样原页与GPU颜色／深度resolve，原生ASan／UBSan和实际作者AIR主机Mesa逐sample读回通过；完整native20／PNG6实机验收尚未发送。所有候选不替换已验CA基线。

最新[原生呈现候选](docs/METAL-PRESENTATION.md)补原drawable标志、四种present和真实GPU页发布后的Scheduled，适配原事务呈现路径。原生ASan／UBSan、ARM64构建与CPU包offline audit通过，完整native21／原UI与光学仍待手机验收。

仅操作BH905ND69G，fastboot必须-s；COM>=10且VID1209/PID0001，单controller／worker、PON仅0x06、单download／boot、失败不重发；四回程marker、健康30秒及旧worker退出后继续。内核GPU底层／allocator／SCM／PON／RPC／VM冻结；不重刷已有gl5／userdata／SD／TA／XFL。

iPhoneX iOS16.7.16／20H392、Sony原厂固件和备份由使用者提供，不进入Git；回收／归档规则见 [LAYOUT](docs/LAYOUT.md)。此前README及旧里程碑原文在 [archive](docs/archive/20261119/README-before-ca-matrix-actual.md)，各专题和reproducibility保留全部旧失败与适用范围。
