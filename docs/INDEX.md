# 文档入口 — 2026-11-22

当前设备／基线／候选只看[STATUS](STATUS.md)。[BUILD](BUILD.md)放现行复现，[OPERATIONS](OPERATIONS.md)放手机与回程，[LAYOUT](LAYOUT.md)放目录／恢复，[ORGANIZATION](ORGANIZATION.md)放整理报告。历史专题保留其原版本、token和失败边界；旧页面中的“当前”指当次日期，不覆盖STATUS。

| 当前专题 | 内容 |
| --- | --- |
| [附件验收](ATTACHMENT-HARDWARE.md) | D32／S8／Memoryless／MSAA原byte、模板Clear、作者page与tiled候选 |
| [显示比例](DISPLAY-GEOMETRY.md) | 已验720×1280／360×640点／scale2及平直状态栏，小组件布局待修 |
| [原壁纸请求](METAL-REQUEST-OBSERVATION.md) | 原33方法／mode已观察，deactive／黑壁纸仍STOP |
| [串口截图](SCANOUT-SCREENSHOT.md) | 无损720×1280、CRC／FNV／保存像素、光学边界 |
| [CA矩阵](CA-MATRIX.md) | 已验GLES第9组、准确20H392绑定 |
| [完整原生基线](METAL-BACKBOARD.md) | backboard suite、完整proof与实际GPU边界 |

| 能力实现／固定证据 | 专题 |
| --- | --- |
| 资源／queue／AIR／图形 | [资源](METAL-RESOURCES.md)、[队列](METAL-QUEUES.md)、[AIR](AIR-COMPUTE.md)、[Render](METAL-RENDER.md)、[constants](METAL-CONSTANTS.md) |
| XR／共享页／mipmap／MRT／sampler | [XR](METAL-XR.md)、[IOSurface](METAL-IOSURFACE.md)、[mipmap](METAL-MIPMAPS.md)、[MRT](METAL-MRT.md)、[sampler](METAL-SAMPLERS.md) |
| AIR位型 | [narrow](AIR-NARROW.md)、[half](AIR-HALF.md)、[static-half](AIR-STATIC-HALF.md)、[selection](HALF-SELECTION.md) |
| 缓存和原耗时证据 | [program-cache](PROGRAM-CACHE.md)、[IR3](IR3-VARIANT-CACHE.md)、[Clear-cache](CLEAR-CACHE.md)、[XR-clear](XR-CLEAR.md)、[render-stage](RENDER-STAGES.md)、[PNG／postdraw](POSTDRAW-OBSERVATION.md) |
| 新附件与呈现契约 | [Depth32／Stencil8](METAL-DEPTH-STENCIL.md)、[Memoryless](METAL-MEMORYLESS.md)、[MSAA](METAL-MULTISAMPLE.md)、[presentation](METAL-PRESENTATION.md) |
| 显示服务／原登记 | [Surface诊断](SURFACE-DIAGNOSTICS.md)、[只读Surface](SURFACE-READONLY.md)、[原登记](METAL-DISCOVERY.md)、[短登记](METAL-REGISTRY-SHORT.md)、[原Aegir](AEGIR-RENDERING.md) |
| 原输入和OSS参考 | [gl5配方](ROOT-GL5.md)、[参考仓库](REFERENCE-REPOSITORIES.md)、[MSM8998调查](MSM8998-PEER-SURVEY-2026-11-15.md) |

reproducibility保存原结果摘要、源码锁、精确proof和回收记录；原日志／镜像／作者失败page／光学媒体保持本地。任何host、局部byte、offscreen、seed或回程成功都不扩大为整机完成。

过时handover／handout／工作日志和本周47份入口原文均归入[archive](archive/20261121/weekly-integration/original-document-manifest.json)，原字节与SHA保持。阶段专题保留复现信息，旧编号实验不再是当前执行入口。文档链接以正式仓库相对路径为准。

最新原结果：[完整native21／PNG6](../reproducibility/presentation-lifecycle-r2-stage-20261122.json)、[原壁纸mode](../reproducibility/original-mode-swift-owner-r13-stage-20261122.json)、[原生尺寸](../reproducibility/native2x-r14-stage-20261122.json)、[平直状态栏](../reproducibility/native2x-flat-r15-stage-20261122.json)。旧r10 presentation拒绝与全部失败原样保留，当前边界统一由STATUS解释。

此次[50份修改前原文／SHA](archive/20261122/native-r15-close-out/original-document-manifest.json)保留旧in-flight和重复工作日志，现行入口已收敛。实际回收与所有阶段记录由[ORGANIZATION](ORGANIZATION.md)汇总。

[r16原布局取证](../reproducibility/native-layout-r16-stage-20261122.json)定位小组件label bbox与下一排图标重叠；窗口／grid配置差异仍待追查，GLES9通过不扩大为完整原UI成功。

[r17原生密度／独立UI拒绝](../reproducibility/native-grid-density-r17-stage-20261122.json)保留原setter与实图，以及缺VIEW653的拒绝；[原27方法及cache范围](../reproducibility/native-layout-20H392-static-r17.json)由直接HFS/sparse reader取得，不展开大镜像。
