# 文档入口

当前设备、准确SHA、候选、独立判定和下一步统一看[STATUS](STATUS.md)。历史专题中的“当前／下一步／in-flight”只指原实验日期，不覆盖现行操作规则。

| 现行手册 | 唯一职责 |
| --- | --- |
| [STATUS](STATUS.md) | 实际设备、已验输入、候选、卡点 |
| [BUILD](BUILD.md) | 固定OSS源、Mac／WSL、新RAM与产物复现 |
| [OPERATIONS](OPERATIONS.md) | 单发、0x06回程、串口扫描、相机 |
| [LAYOUT](LAYOUT.md) | 正式目录、受保护输入、归档和恢复 |
| [ORGANIZATION](ORGANIZATION.md) | 本周原生回收路径／SHA与文档整合 |
| [DISPLAY-GEOMETRY](DISPLAY-GEOMETRY.md) | 原生几何、original／fitted全模型对照及标签归属 |

| 当前技术专题 | 内容 |
| --- | --- |
| [附件验收](ATTACHMENT-HARDWARE.md) | D32／S8／Memoryless／MSAA原字节、作者page与tiled工作布局 |
| [原壁纸请求](METAL-REQUEST-OBSERVATION.md) | 原33方法／mode、磁盘PNG复用、原队列EINVAL、开源反射绑定修补与原拒绝 |
| [串口截图](SCANOUT-SCREENSHOT.md) | 无损720×1280、来源／CRC／FNV／保存像素与光学范围 |
| [Lilac 外设](LILAC-PERIPHERALS.md) | 固定 Sony WLED 来源、背光服务及当前音频／无线日志边界 |
| [CA矩阵](CA-MATRIX.md) | 完整GLES第9组、准确20H392绑定 |
| [完整native呈现](METAL-PRESENTATION.md) | R11完整native21／PNG6、CA生命周期与原GPU范围 |

| 历史能力实现／固定原证据 | 专题 |
| --- | --- |
| 资源／queue／AIR／图形 | [资源](METAL-RESOURCES.md)、[队列](METAL-QUEUES.md)、[AIR](AIR-COMPUTE.md)、[Render](METAL-RENDER.md)、[constants](METAL-CONSTANTS.md) |
| XR／共享页／mipmap／MRT／sampler | [XR](METAL-XR.md)、[IOSurface](METAL-IOSURFACE.md)、[mipmap](METAL-MIPMAPS.md)、[MRT](METAL-MRT.md)、[sampler](METAL-SAMPLERS.md) |
| AIR位型 | [narrow](AIR-NARROW.md)、[half](AIR-HALF.md)、[static-half](AIR-STATIC-HALF.md)、[selection](HALF-SELECTION.md) |
| 缓存和原耗时 | [program-cache](PROGRAM-CACHE.md)、[IR3](IR3-VARIANT-CACHE.md)、[Clear-cache](CLEAR-CACHE.md)、[XR-clear](XR-CLEAR.md)、[render-stage](RENDER-STAGES.md)、[PNG／postdraw](POSTDRAW-OBSERVATION.md) |
| 新附件与呈现 | [Depth32／Stencil8](METAL-DEPTH-STENCIL.md)、[Memoryless](METAL-MEMORYLESS.md)、[MSAA](METAL-MULTISAMPLE.md)、[presentation](METAL-PRESENTATION.md) |
| 显示服务／原登记 | [Surface诊断](SURFACE-DIAGNOSTICS.md)、[只读Surface](SURFACE-READONLY.md)、[原登记](METAL-DISCOVERY.md)、[短登记](METAL-REGISTRY-SHORT.md)、[原Aegir](AEGIR-RENDERING.md)、[早期native基线](METAL-BACKBOARD.md) |
| 原输入和OSS参考 | [gl5配方](ROOT-GL5.md)、[参考仓库](REFERENCE-REPOSITORIES.md)、[2026-11-15 MSM8998调查](MSM8998-PEER-SURVEY-2026-11-15.md) |

reproducibility保存原结果摘要、源码锁、精确proof和回收记录；镜像、signed产物、原日志／扫描帧／失败page和光学媒体保持本地。host或离屏结果、局部字节、seed、GPU Completed及回程各自验收。

| 当前离线实现 | 准确证据／范围 |
| --- | --- |
| GPU r5 dense image映射 | [离线报告](../reproducibility/ordered-image-barrier-dense-r5-offline-20261126.json)、[CPU／GLES9实机](../reproducibility/dense-r5-CPU-GLES9-actual-20261127.json)、[MRT子集及PNG原拒绝](../reproducibility/dense-r5-native21-PNG-refusal-20261127.json)；完整native／PNG和原壁纸待验 |
| PCM运行时／C11接口／clock／cache交接 | [C接口报告](../reproducibility/lilac-pcm-c-abi-offline-20261126.json)：十二组回归、六个ARM64对象、稳定caller storage／重入和销毁门槛、复制→cache交接→publication／完成→acquire；真实mapping／cache指令／DSP／codec／声音待接 |
| ADSP启动依赖与实际生命周期 | [源码审计](../reproducibility/lilac-adsp-boot-source-audit-20261126.json)：固定Sony Git blob和准确formal源，RPM前置／stage4～7最终shutdown；当前DSP状态与声音未验 |
| console日志完整性 | [当前writer争用报告](../reproducibility/console-log-current-writer-contention-20261127.json)、[新native原拒绝](../reproducibility/cif10-paced-r3-native-stencil-log-refusal-20261127.json)：v729源／真实链接object已核对；当前writer竞争待处理 |
| v732票号日志／完整新GPU组合 | [完整实机报告](../reproducibility/v732-log-tickets-full-actual-20261127.json)、[native21](../reproducibility/v732-native21-proof-20261127.json)、[PNG6](../reproducibility/v732-PNG6-proof-20261127.json)；原壁纸／FPS／睡眠仍待验 |
| v732原Earth链接卡点 | [原失败及方法返回范围](../reproducibility/v732-aegir-earth-link-observation-20261127.json)、[准确库锚点符号](../reproducibility/v732-aegir-earth-link-symbols-20261127.json)；NIR inlining样本，原壁纸未通过 |
| SSA设备轮／CA当前表项 | [原阶段索引](../reproducibility/nir-callee-ssa-device-round-20261128.json)、[CA原拒绝](../reproducibility/nir-callee-ssa-gles9-refused-20261128.json)、[只读表项及六阶段host](../reproducibility/ca-current-slots-host-20261128.json)；新revision2实际准入见STATUS |
| 新SSA／CA完整组合 | [实际产物](../reproducibility/ca-current-slots-SSA-candidate-20261128.json)、[四轮实机](../reproducibility/ca-current-slots-SSA-device-round-20261128.json)、[native21](../reproducibility/ca-current-slots-SSA-native21-proof-20261128.json)、[PNG6](../reproducibility/ca-current-slots-SSA-png6-proof-20261128.json)；自己的完整准入通过，原壁纸／FPS未提升 |
| 新原Aegir触发观察 | [原STOP与启动记录](../reproducibility/aegir-callee-ssa-original-observation-20261128.json)、[原扫描／回程](../reproducibility/aegir-callee-ssa-scanout-20261128.json)；本靴未见Aegir执行，不作Earth链接结论 |
| 原壁纸配置只读采集 | [CPU准入](../reproducibility/poster-config-cpu-actual-20261128.json)、[完整native21／PNG6与采集准入](../reproducibility/poster-config-gpu-actual-20261128.json)、[两份实际偏好](../reproducibility/poster-config-current-observation-20261128.json)；当前选中provider仍未识别 |
| 有界MCM容器只读候选 | [固定OSS身份方案](../reproducibility/poster-container-OSS-schema-reference-20261128.json)、[完整gl5静态核对](../reproducibility/poster-container-gl5-static-20261128.json)、[11组host／8类损坏拒绝](../reproducibility/poster-container-host-20261128.json)；未接入observer或投递 |

原[PCM session七组](../reproducibility/lilac-pcm-session-offline-20261126.json)、[GLINK适配八组](../reproducibility/lilac-pcm-glink-transport-offline-20261126.json)和[人工sparse槽位barrier流](../reproducibility/ordered-image-barrier-star-load-host-20261126.json)保持历史边界，不作为不同新产物的实机准入。现行步骤只由STATUS／BUILD维护。

[具体FIFO十组／2048环位置](../reproducibility/lilac-pcm-smem-fifo-offline-20261126.json)是当前运行时保留的底层证据；实时clock与PCM跨度管理的新边界由上述runtime报告验证。

runtime的[EOF收尾前报告](../reproducibility/lilac-pcm-runtime-cache-offline-20261126.json)保留原编译源身份，最终报告另绑定去除EOF空行后的准确源码及完整复验；不覆盖旧记录。

## 关键原阶段

| 阶段 | 原报告 |
| --- | --- |
| 完整native21／PNG6、CA lifecycle归零 | [R11](../reproducibility/presentation-lifecycle-r2-stage-20261122.json) |
| 原壁纸33方法及deactive策略 | [R13](../reproducibility/original-mode-swift-owner-r13-stage-20261122.json) |
| 原生尺寸／平直状态栏 | [R14](../reproducibility/native2x-r14-stage-20261122.json)、[R15](../reproducibility/native2x-flat-r15-stage-20261122.json) |
| 布局重叠、缺记录与旧参考范围 | [R16](../reproducibility/native-layout-r16-stage-20261122.json)、[R17](../reproducibility/native-grid-density-r17-stage-20261122.json)、[R18](../reproducibility/native-layout-model-r18-stage-20261122.json) |
| 完整38应用／Dock／Folder对照，标签归属REVISE | [R19／R20](../reproducibility/native-full-model-r19-r20-stage-20261122.json)、[归属卡点](../reproducibility/native-label-ownership-r20-blocker-20261122.json) |
| 完整原Class祖先／38应用及两处17.5点间距PASS | [R21](../reproducibility/native-label-ownership-r21-stage-20261122.json)、[新original](../reproducibility/native-label-ownership-r21-original-baseline-20261122.json)、[新fitted profile](../reproducibility/native-label-ownership-r21-profile-20261122.json) |
| 新RAM CPU通过、原environment24条、Mesa链接超时REVISE与完整返回 | [R22阶段](../reproducibility/poster-state-r22-stage-20261123.json)、[原状态](../reproducibility/poster-state-r22-gpu-original-state-20261123.json)、[原失败](../reproducibility/poster-state-r22-gpu-failure-20261123.json)、[链接栈](../reproducibility/poster-state-r22-gpu-link-symbols-20261123.json)、[route缺ID拒绝](../reproducibility/poster-state-r22-route-original-rejection-20261123.json) |

## 历史原文

| 原文快照 | manifest |
| --- | --- |
| 本周47份手册 | [20261121](archive/20261121/weekly-integration/original-document-manifest.json) |
| r15整合前50份入口 | [native-r15](archive/20261122/native-r15-close-out/original-document-manifest.json) |
| R19整合前50份／6a9efe8 | [native-full-model-r19](archive/20261122/native-full-model-r19-close-out/original-document-manifest.json) |
| 本轮50份现行手册／入口 | [poster-state-r22](archive/20261123/poster-state-r22-close-out/original-document-manifest.json) |
| NOTES／XNU／AIR三份补充入口 | [project-entry-supplement](archive/20261123/project-entry-supplement/original-document-manifest.json) |
| 缓存库完整验收整合前9份入口 | [cache-budget 20261124](archive/20261124/cache-budget-close-out/original-document-manifest.json) |
| 原快照来源整合前4份入口 | [snapshot-scope 20261125](archive/20261125/poster-snapshot-scope/original-document-manifest.json) |
| 原队列停点与外设整合前10份入口 | [render-buffer 20261125](archive/20261125/render-buffer-selection/original-document-manifest.json) |
| 新Metal完整验收整合前5份入口 | [reflected-render 20261125](archive/20261125/reflected-render-acceptance/original-document-manifest.json) |
| XR timestamp候选整合前8份入口 | [ordered-XR 20261126](archive/20261126/ordered-XR-barrier/original-document-manifest.json) |
| PCM会话／星点规模主机复验整合前5份入口 | [pcm-session 20261126](archive/20261126/pcm-session-and-XR-load/original-document-manifest.json) |
| Mesa dense image修补整合前5份入口 | [dense-image 20261126](archive/20261126/dense-image-barrier/original-document-manifest.json) |
| PCM GLINK适配整合前5份入口 | [pcm-glink 20261126](archive/20261126/pcm-glink-transport/original-document-manifest.json) |
| SMEM FIFO实现及STATUS整合前5份入口 | [pcm-smem 20261126](archive/20261126/pcm-smem-fifo/original-document-manifest.json) |
| PCM运行时／缓存交接整合前5份入口 | [pcm-runtime 20261126](archive/20261126/pcm-runtime-cache/original-document-manifest.json) |
| Mac旧38份文档与源文件 | [macos-patch-entry](archive/20261123/macos-patch-entry/original-document-manifest.json) |

旧handover／handout与编号手册在docs/archive；原字节与SHA保持。旧整理报告入口：[2026-11-12](ORGANIZATION-2026-11-12.md)、[2026-11-13](ORGANIZATION-2026-11-13.md)、[2026-11-15](ORGANIZATION-2026-11-15.md)。回收与复原统一由ORGANIZATION／LAYOUT说明，不从旧in-flight恢复执行。

原Earth当前输入重放与未采用NIR原型见[离线报告](../reproducibility/earth-link-host-offline-20261127.json)；本轮入口原文／SHA见[归档manifest](archive/20261127/earth-host-link-close-out/original-document-manifest.json)。

2026-11-28局部SSA／独立后台监督进程：见[host编译器证据](../reproducibility/nir-callee-local-ssa-host-20261128.json)和[生命周期证据](../reproducibility/hidden-trial-supervisor-host-20261128.json)；五份入口原文／SHA见[本轮归档](archive/20261128/callee-ssa-and-background/original-document-manifest.json)。

2026-11-28完整native候选／投递前实际compiler审计见[候选](../reproducibility/nir-callee-full-native-candidate-20261128.json)和[审计](../reproducibility/compiler-overlay-actual-audit-20261128.json)；入口原文／SHA见[归档](archive/20261128/compiler-overlay-audit/original-document-manifest.json)。

2026-11-28非零backlight property→duty候选：见[Lilac外设](LILAC-PERIPHERALS.md)与[离线证明](../reproducibility/backlight-nonzero-duty-backend-20261128.json)；六份入口原文／SHA见[归档](archive/20261128/backlight-duty-backend/original-document-manifest.json)。

逐IO背光epoch候选见[新证据](../reproducibility/backlight-duty-per-io-epoch-20261128.json)，五份入口原文／SHA见[归档](archive/20261128/backlight-per-io-epoch/original-document-manifest.json)。

WLED实际SPMI接入依赖审计见[报告](../reproducibility/backlight-spmi-binding-audit-20261128.json)，四份入口原文／SHA见[归档](archive/20261128/wled-spmi-binding/original-document-manifest.json)。

配置读取轮整合前五份入口原文／SHA见[归档manifest](archive/20261128/poster-config-reader/document-manifest.json)。实际设备／下一步只由STATUS维护，读取及严格解码复现由BUILD说明。

容器采集源码接入revision2：[最终host／准入边界报告](../reproducibility/poster-container-source-integration-r2-20261128.json)。整合前五份入口原文／SHA见[本轮归档](archive/20261128/poster-container-integration/document-manifest.json)；旧revision1 host报告保留，不作为新guest准入。

WLED只读lane接口：[准确源码／原选择器／14+7组host证据](../reproducibility/spmi-wled-lane-snapshot-host-20261128.json)。五份整合前入口原文／SHA见[归档](archive/20261128/spmi-lane-snapshot/document-manifest.json)；候选未加入当前kernel，写入及光学未验。

WLED首帧lane记录：[生产器／原流解码证明](../reproducibility/wled-lane-screen-host-r2-20261128.json)，五份整合前入口原文／SHA见[归档](archive/20261128/wled-lane-screen/document-manifest.json)。实际编译依赖当前由[Mac阻断审计](../reproducibility/mac-build-blocker-audit-20261128.json)记录。
