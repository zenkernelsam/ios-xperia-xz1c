# 当前状态 — 2026-11-21

Sony BH905ND69G 健康停在 fastboot；controller和串口worker已退出，无in-flight。本周清理和文档整合已完成。新主机捕获revision1的tiled reference CPU `d0c3a52cd3eb2841` 实机完整通过，一份READY／banner／LIVE及RAM读取、无replay；0x06四marker和健康30秒共38.844秒。旧 `b2f3fc4f69000ff2` 38.594秒回程后的独立启动前缀字节拒绝保持，不重发旧包。完整tiled native21／PNG仍未准入，先核对工作页／GPU copy和MSAA importer证据，见[新CPU原报告](../reproducibility/serial-capture-r1-reference-cpu-20261121.json)。

## 已验基线和候选

| 范围 | 精确身份 | 已验边界 |
| --- | --- | --- |
| 内核v730／83源 | `f258dee9c6d142b90a573f796eeec5e92b39dd65feed6a21b2542d90fb66a88d`，20665112B | private／owner／ready、CPU、串口真实帧、0x06回程 |
| CA基线GLES | `d6d8fcfa27bcac84ab5edb3d2a11c1ed23374e8b491a3e8de093a9a2a819ac2b`，14765296B | 自身CPU／GLES9 |
| CA基线Metal | `5e94ab57401479b42f506884a172a7a839378a82310661a1f66164e99c21f988` | 完整native16＋共享PNG6 |
| CA基线Metal source-map | `7b5a083abdb98fa5d29ef82fbd8e7e68b2a29fd995f16c4163d5c612cb8b9de7` | 与原完整native proof绑定 |
| 固定Mesa／GPU | 24.3.4／`769e51468b49b2a42f0a0eaf71cf9eed5ff4e5de`／FD540 GLES3.1 | 无Vulkan移植 |
| 原附件候选GLES | `21de6821910c452f1bda70720f3573b81ac76483a10f1959ef95e7b0bc9447b4` | 11月20日自身CPU／GLES9通过 |
| tiled附件候选GLES | `c617f2eec2c60dd1b925797f0295815e65e2d705b62817ca1d6053b2743de9aa` | 11月21日自身CPU／GLES9通过，附件未准入 |
| presentation候选Metal／source-map | `8feea4593116ffe9187204f5dd4cf4dd48d3027816d4763ecef20da200f86a27`／`db1605ef8dae8f8ed0a29999dcb2cf7537457aba7ad428677c21e0e914771b78` | 完整native21未通过，生产基线保持 |

新二进制的准入与源码可构建分别记录。旧native16／旧GLES成功不能准入附件候选；当前源中的Depth32、Stencil8、Memoryless、2×／4×MSAA和presentation需完整native21／PNG6。本轮证据见[附件真机报告](ATTACHMENT-HARDWARE.md)、[六靴原报告](../reproducibility/attachment-hardware-r1-20261120.json)、[tiled阶段](../reproducibility/depth-tiled-r1-stage-20261121.json)和[CPU软件拒绝](../reproducibility/depth-tiled-r1-reference-cpu-refused-20261121.json)。

## 实际里程碑

CPU／launchd、F12真实触控与音量键已验。GLES完整750×1624三角形／纹理读回、原IOSurface共享、960000只读像素、动态纹理／上下文、160次relink／320draw／20480像素及资源撤销通过。CA矩阵按准确20H392 QuartzCore UUID、原两空槽、代码锚点安装，GLES第9组六阶段1536B＋192B guard最大差异0。

CA基线native16包括资源、queue、AIR compute／render／constants、XR、IOSurface、mipmap、窄整数、half、MRT、sampler、program-cache、static-half、Clear-cache、IR3-cache；原字节／guard／fence／回收均完整。原token `4946e854d77fe213`／`46747c46785bf85c` 分别39.390／38.375秒完整回程，见[CA-MATRIX](CA-MATRIX.md)与[run-index](../reproducibility/v730-ca-matrix-r1-run-index.json)。

串口已能取真实720×1280／BGR888／stride2160无损PNG。11月19日两张锁屏恢复灰色时间、日期，原 `881ce1c5d844e7ba` 单串口上滑后读到正常颜色主页、文字、小组件及Dock。11月20日 `929a353138003730` 150.047秒再次读到灰字锁屏、绿色电池和白homebar，背景仍黑。日期／捕获token分别保留；合成上滑不等于真人触控，扫描帧不等于LCD光学／背光或FPS。

## 当前卡点

1. **深度／模板布局。** 原附件候选reference GPU `011e16b3caf5b1a6` 与独立页面诊断 `62fb2e559abcbe3d` 均REVISE。颜色原sample与guard正确，D32／S8在非线性位置写入且部分逻辑guard改变；12个原作者页完整取回。固定Freedreno已说明A5xx sysmem深度不能采用线性布局。正式源码新增独立FDL5 tiled工作页及GPU raw-copy，原线性页／offset／stride保持。ARM64构建与其自身GLES通过，但reference CPU的软件日志门槛未通过，完整附件和resolve仍未验，详见[ATTACHMENT-HARDWARE](ATTACHMENT-HARDWARE.md)。
2. **原壁纸请求路由。** 最后实际原场景r3 `3541e07973fe676f` 有40条完整请求，共享设备／queue、NUNIQuad／NUNIScene有效，原paused=YES；display／sync／tick／nextDrawable／pipeline均未出现。原Vista初始化本来暂停，后续模式和环境触发待观察，不能强制解除或画一张替代原请求。33方法模式observer源码已完成，尚无成功实机模式记录，见[METAL-REQUEST-OBSERVATION](METAL-REQUEST-OBSERVATION.md)。
3. **启动日志一致性已独立恢复。** 旧reference CPU READY重放在第3232B首次不同，原CPU拒绝保持。实际v730 USB源显示旧主机DTR提前开启和replay重置游标会交错尚未完成的IN；两段最初3232B完全一致，不是完整启动块一致的证明。新closed-DTR单流捕获及artifact／worker源码SHA绑定有7项模拟故障、10项原字节前缀、6项回程检查，独立CPU `d0c3a52c` 已取得实机准入；worker SHA b37f33c1、raw c638c960、一份READY／banner／LIVE及RAM读取，无replay，38.844秒完整回程。未重发旧包或放宽旧核验，见[捕获研究](../reproducibility/serial-capture-r1-research-20261121.json)。
4. **原生显示比例。** 当前系统375×812点／750×1624像素／scale2，最后映射到720×1280，横纵比例不同。用户要求改720×1280、360×640点／scale2的原生16:9布局。ADT离线profile已补，真实UIScreen／MobileGestalt、状态栏／safe area、触控和原界面须同靴核验；未投递新geometry，见[DISPLAY-GEOMETRY](DISPLAY-GEOMETRY.md)。
5. **其它未完成。** 完整壁纸、动态UI／FPS、睡眠唤醒、LCD光学／背光、声音、Wi-Fi／蓝牙及更广的texture／depth／stencil／MSAA能力保持active。v730实际SweetSurface对共享事件selectors36／37／38／45仍返回零输出，不能当共享事件工作；事件草稿未接入当前库。

早期RPM txh=8／txt=0死锁及全部恢复失败仍是历史证据；本轮GPU启动实际RPM正常。r4 USB-open、r5 guard和本轮失败包不重发。无LIVE的早期失败仍可能需要物理恢复，不能保证所有无人值守情形都可自动回程。

## 整理与后续

本周Mac1713项／34883文件、11,852,238,934逻辑字节已进原生垃圾桶并复核内容，build目录约14GiB降到3.6GiB；垃圾桶未清空，因此系统仍约94%使用／62GiB可用。当前blob／signed库、基线、gl4／gl5／IPSW、实际dxb源码和依赖保持。四条Finder元数据SHA变化单列，用户内容SHA通过。文档原文与SHA先归档，现行入口与回收报告见[LAYOUT](LAYOUT.md)、[ORGANIZATION](ORGANIZATION.md)和[文档索引](INDEX.md)。

整理结束后先解决reference CPU日志门槛，完成tiled附件native21／PNG6，再恢复原模式／壁纸观察；显示比例采用独立geometry试验。只BH905ND69G、COM≥10／VID1209-PID0001、单controller／worker、单download／boot、PON0x06、四marker／健康30秒／worker退出；内核GPU底层／allocator／SCM／PON／RPC／VM冻结。
