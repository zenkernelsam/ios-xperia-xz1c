# 当前状态 — 2026-11-21

r9四靴已结束，CPU／GLES9／reference CPU通过，native21为REVISE，40.016秒完整0x06回程／健康30秒，Sony健康fastboot、无worker。r10仅离线源码／核验就绪，尚未投递。
当前r8四靴已结束：CPU／GLES9／reference CPU通过，native21在Stencil8 phase96为REVISE，39.391秒完整0x06回程／健康30秒，Sony健康fastboot、无worker。r9仅离线源码／核验完成，尚未投递。
Sony BH905ND69G健康fastboot；本轮四靴已结束，无controller／串口worker／in-flight。Stencil8 r7完整native21为REVISE／STENCIL8_PROBE_FAILED，40.813秒完成0x06四marker、连续健康30秒和worker退出。自身CPU、GLES9、reference CPU均独立通过；S8仍phase2失败，Memoryless新清屏路径也在phase2失败。MSAA／presentation在ACK前没有完整END，PNG6未准入。生产CA基线保持，原失败不重发。

## 已验生产基线

| 范围 | 精确身份 | 已验边界 |
| --- | --- | --- |
| v730 kernel／83源 | f258dee9c6d142b90a573f796eeec5e92b39dd65feed6a21b2542d90fb66a88d，20665112B | private／owner／ready、CPU、原scanout、0x06回程 |
| CA基线GLES | d6d8fcfa27bcac84ab5edb3d2a11c1ed23374e8b491a3e8de093a9a2a819ac2b | 完整自身CPU／GLES9 |
| CA基线Metal | 5e94ab57401479b42f506884a172a7a839378a82310661a1f66164e99c21f988 | 完整native16＋共享PNG6 |
| CA Metal source-map | 7b5a083abdb98fa5d29ef82fbd8e7e68b2a29fd995f16c4163d5c612cb8b9de7 | 与完整原native proof绑定 |
| Mesa／GPU | 24.3.4／769e51468b49b2a42f0a0eaf71cf9eed5ff4e5de／FD540 GLES3.1 | 固定开源归档；Vulkan尚未移植 |

CPU／launchd、F12真实触控及音量键已验。GLES包括完整750×1624三角形／纹理、原IOSurface、只读像素、动态纹理／上下文及完整CA矩阵第9组。Metal16含原资源／queue／AIR compute与render、constants、XR／IOSurface／mip、位型、MRT／sampler及缓存；原byte、guard、fence和释放完整验证。见[CA-MATRIX](CA-MATRIX.md)及[基线run-index](../reproducibility/v730-ca-matrix-r1-run-index.json)。

串口能取真实720×1280／BGR888／stride2160扫描帧。基线有灰色锁屏时间日期与单串口上滑后的正常颜色主页；合成输入不替代真人触控。本轮GLES9的150秒原帧仍是灰字黑壁纸锁屏、绿色电池和白homebar，PNG SHA 4eb49f297fe8fcc0af02a6f0ea104f01382421754650cb9105d7cd9b765f0c97。扫描帧不证明LCD光学／背光、FPS或唤醒。

## r7历史候选与四靴结果

| r7身份／阶段 | 实际结果 |
| --- | --- |
| 源码 | b9afd391682a48591ef07c6a2e24aa88ffd684a4，正式Windows／Mac已同步 |
| GLES | 681c4a8c409106443746d88f1b5f86c98e62b3e72d13b5707b70ecb18bb5b2c6，14800944B |
| Metal／source-map | 8feea4593116ffe9187204f5dd4cf4dd48d3027816d4763ecef20da200f86a27／db1605ef8dae8f8ed0a29999dcb2cf7537457aba7ad428677c21e0e914771b78 |
| 自身CPU 790ba0f8fd381508 | PASS，38.859秒完整回程 |
| GLES9 6a7d1537e9ce3f33 | PASS，38.828秒完整回程及150秒真实帧 |
| reference CPU da3da9d8d13b9192 | PASS，原RAM／启动流独立通过 |
| native21 71d9ff6329643ac7 | REVISE，40.813秒完整回程 |

r7按Gallium逐位模板fallback思路实现原integer页→逐sample／bit stencil draw→实际stencil纹理Store，禁止CPU重排。主机162copy／54native clear、任意256种字节模式、深度／未选样本／外围保护通过；另60次depth MSAA及2390次scalar状态回归通过。llvmpipe请求2×实际分配4×，空fragment scissor边界异常单列；不作为FD540证明。见[主机报告](../reproducibility/stencil-sample-r7-host-20261121.json)。

实机五个原作者page共5312B，与本LIVE→ACK raw逐SHA重新解码一致。S8 phase2的color／depth／guard正确，153个逻辑stencil值全0，预期3。Memoryless phase2颜色正确，stencil出现0／17，原值仍不符。Depth32完整29阶段的byte／guard／getBytes通过，但严格work协议拒绝：32条BLIT／COPY完整，32条BOX全部被256B tee分块拆开字段，不补造记录。完整附件、native21、PNG与原UI未准入，见[四靴原报告](../reproducibility/stencil-sample-r7-stage-20261121.json)及[附件专题](ATTACHMENT-HARDWARE.md)。

r4完整Depth32 29／Memoryless8原byte、work／copy／fence证明保留。r6的MSAA phase1已过、phase2仅55个stencil字节错误，Memoryless8通过；Depth29 byte通过但缺一条BLIT，严格work协议不完整。FD5 gen_dirty／gen_all均0符合固定上游，common dirty flags仍严格要求。历史见[r4](../reproducibility/depth-tiled-r4-stage-20261121.json)／[r6](../reproducibility/depth-msaa-state-r6-stage-20261121.json)，不提升基线。

## 下一步与其它目标

1. 核对原stencil Clear的FBO／Mesa／FD5实际状态及清屏前后页面；已从固定bsd/dev/mem.c确认单write每256B分块tee并加进程前缀，源码恢复独立BLIT／BOX记录，后续需新产物自己的完整实机核验。再投递有新证据目的的新候选。不改冻结内核／allocator／SCM／PON／RPC／VM，不重发r7。
2. 完整native21＋PNG6后继续原poster模式／壁纸。原queue／scene有效，但display／tick／pipeline未出现，原Vista paused=YES；33方法模式observer仅离线完成。不能强制解除暂停或画替代帧，见[原请求专题](METAL-REQUEST-OBSERVATION.md)。
3. 独立推进720×1280像素／360×640点／scale2原生16:9。设备仍375×812／750×1624；ADT离线profile已完成，package_trial几何集成未接入。MG／UIScreen／ADT／safe area／状态栏／触控需同靴一致，见[DISPLAY-GEOMETRY](DISPLAY-GEOMETRY.md)。
4. 原UI动态FPS、睡眠唤醒、光学／背光、声音、Wi-Fi／蓝牙保持active。v730共享事件selectors36／37／38／45仍零输出，未实现草稿不接入库。

RPM历史txh=8／txt=0死锁及恢复失败保持；本轮GPU时钟正常。无LIVE早期E1F4仍可能需物理恢复。只BH905ND69G、COM≥10／VID1209-PID0001、单发、PON0x06、四marker／健康30秒／worker退出。

## 本周整理与复现

本周及追加共1758个Mac项目项／35092文件／12732751725B（约11.86GiB）已原生Trash，用户内容SHA复核。最新21项／29文件／285078590B；Windows本轮14项／38文件／96769680B进回收站，含8个WSL ELF。两个Ubuntu实际root／cisco垃圾桶均为空。垃圾桶未清空；最后Mac系统91%使用／91GiB可用，正式build约3.91GiB，其它项目未改。

50份现行文档链接审计并统一入口；本周47份与本轮10份修改前原文／SHA分别archive。精确清单在[ORGANIZATION](ORGANIZATION.md)，目录与恢复在[LAYOUT](LAYOUT.md)，专题在[INDEX](INDEX.md)。源码／工具／锁只在正式仓库；HEAD是当前候选，已验基线按原manifest和对应源码版本复现，新产物仍需自己的准入。

日志修补已在Mac离线编译：68份实际frontend源、24份未改Mesa输入，库SHA 172c7de2036f467ebb9432e795e9b3d0497a0f84a14ce2d14a8a5ce56f8ed9b1；kernel console实际源SHA 3c840194226099216d7b0177c97b87740f6f6ed889328cfb6b7e863fbf6bd22c。尚未打新RAM或实机，旧r7 proof不准入此新库，见[编译原报告](../reproducibility/stencil-logging-bounds-offline-20261121.json)。

r8候选已完成离线验证：固定fd5_clear对Z32硬件快清返回false；改为一次GL_ZERO绘制加八个GPU bitplane绘制，Load清零也用GPU draw，原depth禁写。主机162次Load／Store、54次逐位清屏、深度／其它sample／外围guard通过，另2390scalar draw和60depth MSAA回归通过；25项证据故障测试通过。ARM64新库与实机准入尚未完成，r7原失败保持，见[r8主机与固定上游依据](../reproducibility/stencil-bitplane-r8-host-20261121.json)。

## r8当前结果与r9候选

r8 GLES dab61a49d275b2f4326f37fbec7f3360179e42e3304475e535f7f8400f87d743／Metal 8feea4593116ffe9187204f5dd4cf4dd48d3027816d4763ecef20da200f86a27，Stencil8 phase0..95共96个阶段的color／depth／stencil原FNV与独立recipe、guard、getBytes全部一致；严格Depth32 29 work／copy／STATE与byte／guard通过。整体仍在phase96失败，Memoryless在phase2失败，两者都是stencil-only。原失败转储缺块／乱序被decoder拒绝，没有填补或重新分类，见[r8原报告](../reproducibility/stencil-bitplane-r8-stage-20261121.json)。

prepare_depth_tiled在无caller depth时把unused primary显式强制DRM_LINEAR，实际S8 work为TILE5_3。r9改用固定Mesa原生resource_create，并要求actual dummy tile3／cpp／pitch／size和caller_depth0／禁depth test-write日志，未知布局明确拒绝。不新造或发布caller depth页，不改内核／allocator／SCM／PON／RPC／VM。新增dummy缺失／线性／depth写入故障核验通过，完整native21／PNG仍待新库。

r9原结果见[r9阶段](../reproducibility/stencil-only-r9-stage-20261121.json)：严格D32全29通过，MSAA18原byte／guard／FNV复算一致，但phase16／stencil206退休记录缺一条，完整work协议保持拒绝。核验同时按原作者encode纠正local0／1／6的首操作Clear，depth resolve按原独立src/dst/samples/filter/fence返回记录绑定，不构造不存在的通用retired日志。

r10临时depth采用已验private FDL5 allocation＋原Z32 alias，不依赖返回linear的普通创建器；独立work／IOVA／tile／cpp／pitch／size／禁depth写及释放必须实际记录。无caller depth、无linear shadow或CPU seed、无depth发布。日志write剩余字节／EINTR和串行fd初始化／errno保存加入，10个真实helper故障测试通过；原缺块／缺失记录不补发，见[r10离线核验](../reproducibility/stencil-private-r10-offline-20261121.json)。
