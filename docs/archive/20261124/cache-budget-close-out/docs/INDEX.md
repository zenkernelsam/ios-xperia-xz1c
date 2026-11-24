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
| [原壁纸请求](METAL-REQUEST-OBSERVATION.md) | 原33方法／mode、R22有界environment、Mesa链接超时与原拒绝 |
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
| Mac旧38份文档与源文件 | [macos-patch-entry](archive/20261123/macos-patch-entry/original-document-manifest.json) |

旧handover／handout与编号手册在docs/archive；原字节与SHA保持。旧整理报告入口：[2026-11-12](ORGANIZATION-2026-11-12.md)、[2026-11-13](ORGANIZATION-2026-11-13.md)、[2026-11-15](ORGANIZATION-2026-11-15.md)。回收与复原统一由ORGANIZATION／LAYOUT说明，不从旧in-flight恢复执行。
