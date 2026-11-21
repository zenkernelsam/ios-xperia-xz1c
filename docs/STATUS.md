# 当前状态 — 2026-11-22

当前r11新RAM CPU／native21与PNG6已独立通过并完成回程；最后66715b9659b47bc1健康返回38.656秒，Sony健康fastboot、无worker。下方r10为历史失败，当前下一步为原poster模式／界面与原生几何。
Sony BH905ND69G健康fastboot。本轮四靴已结束，无controller／串口worker／in-flight；最后native21 c75b0e696cfee083为REVISE／BACKGROUND_METAL_SUITE_FAILED，40.812秒完成0x06四marker、连续健康30秒和worker退出。GPU四组附件和当前r11完整native21／PNG6已独立通过；当前下一步转为原poster模式／UI和原生显示比例，原失败不重发。

## 已验基线与当前候选

| 范围 | 精确身份 | 实际边界 |
| --- | --- | --- |
| 固定v730 kernel／83源 | f258dee9c6d142b90a573f796eeec5e92b39dd65feed6a21b2542d90fb66a88d，20665112B | private／owner／ready、CPU、原scanout及0x06回程 |
| 生产CA GLES | d6d8fcfa27bcac84ab5edb3d2a11c1ed23374e8b491a3e8de093a9a2a819ac2b | 完整自身CPU／GLES9 |
| 生产CA Metal／source-map | 5e94ab57401479b42f506884a172a7a839378a82310661a1f66164e99c21f988／7b5a083abdb98fa5d29ef82fbd8e7e68b2a29fd995f16c4163d5c612cb8b9de7 | 完整native16＋共享PNG6 |
| 固定Mesa／GPU | 24.3.4／769e51468b49b2a42f0a0eaf71cf9eed5ff4e5de／FD540 GLES3.1 | 无Vulkan移植 |
| 当前r10源码 | 35990ab，正式Windows／Mac同步 | 本次实际编译源 |
| 当前r10 GLES | 0766d5c713a883426e801e6d162bbdc46df79c5bc094ca77a7b6d6462599d607 | 自身CPU／GLES9／reference CPU及四组附件通过 |
| 当前已验Metal／source-map | 8feea4593116ffe9187204f5dd4cf4dd48d3027816d4763ecef20da200f86a27／db1605ef8dae8f8ed0a29999dcb2cf7537457aba7ad428677c21e0e914771b78 | r11完整native21＋PNG6已准入，原UI／光学另验 |

CPU／launchd、F12真实触控和音量键已验。生产CA基线GLES9、Metal16与PNG6包括原资源／queue／AIR／XR／IOSurface／mip／MRT／sampler及缓存的原byte、guard、fence、释放；精确证据见[CA-MATRIX](CA-MATRIX.md)与[基线run-index](../reproducibility/v730-ca-matrix-r1-run-index.json)。

原720×1280／BGR888／stride2160扫描帧可经串口取得。灰色锁屏时间日期、正常颜色主页已有基线证据；壁纸仍黑。合成上滑不替代真人触控，扫描帧不证明LCD光学、背光、FPS或唤醒，见[SCANOUT-SCREENSHOT](SCANOUT-SCREENSHOT.md)。

## r10实机结果

| 单发阶段 | token | 结果 |
| --- | --- | --- |
| 自身CPU | f292f8e49ffb1204 | PASS，原RAM和启动流独立核验 |
| GLES9 | 5191d205f7d9a072 | PASS，完整原9组及单次150秒扫描帧 |
| reference CPU | a47eac3cd446ab81 | PASS，37.765秒完整回程 |
| 完整native21 | c75b0e696cfee083 | REVISE，presentation final_allocated11520，40.812秒完整回程 |

以下四组已逐项独立核验本LIVE→ACK原raw，不以END或GPU Completed代替结果：

| GPU附件 | 完整阶段 | 已验内容 |
| --- | ---: | --- |
| Depth32 | 29 | 原byte／guard／getBytes、work IOVA／CPP／BOX／copy fence／释放 |
| Stencil8 | 99 | front/back全部比较／操作／mask、Clear／Load、原独立Z32/S8、字节及guard |
| Memoryless | 8 | 原私有pass页、CPU backing拒绝、双queue隔离、模板单独使用及释放 |
| 2×／4×MSAA | 18 | 原逐sample色／depth／stencil、resolve、guard、copy／fence与两queue隔离 |

r8 GPU清零draw＋八个bitplane覆盖draw使S8推进到phase96；剩下stencil-only分支中unused depth primary仍linear。r9普通创建器未得到所需布局并明确-45拒绝。r10复用真实private FDL5 allocation／alias，dummy与S8均TILE5_3，实际独立handle／IOVA／cpp／pitch／size及dummy退役全部核验，四组现已完整通过。无caller depth、无CPU seed／linear shadow／depth发布；内核GPU底层、allocator、SCM、PON、RPC和VM保持冻结。

日志writer的有界EINTR／部分写处理、串行fd初始化及errno保存已通过10个真实helper故障测试。MSAA核验按原作者local0／1／6首操作Clear，并以真实独立resolve src/dst/samples/filter/fence绑定；旧缺失或坏记录不补造。原r7／r8／r9失败、r8缺块拒绝和r9缺退休日志拒绝保留。见[r10阶段与完整附件证明](../reproducibility/stencil-private-r10-stage-20261121.json)、[r8](../reproducibility/stencil-bitplane-r8-stage-20261121.json)／[r9](../reproducibility/stencil-only-r9-stage-20261121.json)及[附件专题](ATTACHMENT-HARDWARE.md)。

## r10历史卡点与后续范围

1. **Presentation资源生命周期。** 五个真实CAMetalDrawable阶段的原IOSurface像素、padding、fence与原present方法成功；END result1、final_allocated11520，等于五个2304B surface。native presentedTime仍0且光学未验；未出现IOSurface release_failed日志。原queue已在Completed前移除operation／presentation引用，下一步查原生layer／drawable pool和CoreAnimation事务持有及释放。不能忽略分配计数、注入didPresentAtTime或伪造completion。完整native21与PNG6尚未准入，见[METAL-PRESENTATION](METAL-PRESENTATION.md)。
2. **原壁纸／动态UI。** 完整native21＋PNG6后恢复原poster mode／environment观察。原queue／scene有效而display／tick／pipeline未出现，原Vista paused=YES；不强制解除暂停或画替代帧，见[原请求专题](METAL-REQUEST-OBSERVATION.md)。
3. **原生16:9。** 目标720×1280像素／360×640点／scale2，当前设备375×812／750×1624。ADT离线profile已完成；package_trial几何集成及同靴MG／UIScreen／ADT／状态栏／safe area／触控还未验。既有native1与sf2x1冲突，见[DISPLAY-GEOMETRY](DISPLAY-GEOMETRY.md)。
4. **其它外设。** FPS、睡眠唤醒、光学／背光、声音、Wi-Fi／蓝牙保持active。v730共享事件selectors36／37／38／45仍零输出，草稿未接入编译。

本轮RPM／GPU时钟正常；历史txh8／txt0死锁及所有恢复失败保留。无LIVE早期E1F4仍可能需物理恢复。只BH905ND69G、COM≥10／VID1209-PID0001、单发、PON0x06、四marker／健康30秒／worker退出。

## 整理与复现

本周及追加Mac累计1803项／35167文件／13185344737B（约12.28GiB）已原生Trash并逐用户内容SHA复核，未清空。最新Mac91%使用、92GiB可用，正式build约4.08GiB。续轮Windows已回收传输、重复CLI及WSL ELF，原签名库／blob／manifest／失败包／raw／所需少量页面保留，两个Ubuntu实际垃圾桶已复核为空。

50份现行文档统一入口，修改前原文／SHA和过时handover／handout在docs/archive；[ORGANIZATION](ORGANIZATION.md)给实际回收与整合记录，[LAYOUT](LAYOUT.md)给恢复，[INDEX](INDEX.md)联专题。所有源码／工具／锁在正式仓库提交并同步Mac；HEAD是当前候选，生产基线按原source manifest与准确Git版本复现，新库需要自己的实机准入。

## 当前离线候选：presentation lifecycle revision2

重新读取r10原证据确认：queue在Completed前已移除own operation/presentation引用，五surface没有release错误，device残留11520B，native presentedTime0。新候选只给后台探针自己创建的native layer补显式CA begin／commit／flush及每phase autoreleasepool关闭后的实际分配日志；原present四mode／time／options、GPU像素／padding和最终零分配门槛保持，不能造completion。四项证据故障检查通过，ARM64构建／新RAM CPU和native21还未准入，见[离线记录](../reproducibility/presentation-lifecycle-r2-offline-20261121.json)。

## r11完整native21／PNG6已准入

新探针own native layers的显式CA begin／commit／flush和pool关闭后，五阶段各自actual allocated0，最终0；原765pixels／padding／四present modes／transaction_wait保持。完整21组和六PNG原byte／ICC／metadata均经原raw独立验证，原r10残留与probe线程CA事务边界有关，不重新分类旧失败。完整[阶段](../reproducibility/presentation-lifecycle-r2-stage-20261122.json)、[native21 proof](../reproducibility/current-native21-proof.json)和[PNG6 proof](../reproducibility/current-PNG6-proof.json)已保存，必须同kernel／GLES／Metal／source-map绑定。原始production UI／光学／wallpaper／FPS／geometry／其它外设未扩大为完成。

## r11最新下一步

新包自身CPU 6dc5ffb5fd270602／完整native21与PNG6 66715b9659b47bc1均独立通过，GPU回程38.656秒含健康30秒，手机健康fastboot无worker。原R10_11520B失败保留；新probe只对自己创建的native layer闭合CA事务并在pool关闭后核对各phase0分配，原native presentedTime仍0，不能称LCD显示completion。下一包按新完整proof构建原poster33方法mode／environment观察、自己的CPU→GPU及150秒扫描帧，保持原策略，无额外draw或强制active。原生16:9、FPS、睡眠、光学、声音和无线保持完整目标。
