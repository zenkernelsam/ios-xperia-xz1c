# 本机 Metal 前端

现行基线、当前候选、原结果与后续统一在[STATUS](../../docs/STATUS.md)。本轮r7自身CPU／GLES9／reference CPU通过，完整native21仍S8／Memoryless失败，严格work日志也有长度截断；原失败页和回程见[附件专题](../../docs/ATTACHMENT-HARDWARE.md)。生产CA基线保持。

当前已验基线是v730／CA矩阵r1：原资源、queue、MTLB／AIR、MRT／sampler、XR／IOSurface／mipmap与缓存已实机通过；锁屏文字及上滑主页有真实720×1280串口截图。完整壁纸／动态UI／FPS／睡眠／光学仍未完成。现行精确身份与所有阶段以[STATUS](../../docs/STATUS.md)为准。

最新源码候选包含Depth32／Stencil8／Memoryless／2×4×MSAA及[原生呈现](../../docs/METAL-PRESENTATION.md)。呈现保留原CAMetalDrawable／IOSurface纹理、descriptor flags、精确时间与options，实际GPU页发布后才Scheduled；原生host ASan／UBSan和ARM64构建通过，native21／PNG6与原UI仍待手机，不能复用旧native20。所有修补在正式仓库，底层／allocator／SCM／PON／RPC／VM／SweetGPU冻结。

开源对象契约参考固定ios_sagit／898998cf（BSD-2-Clause），iOS16 ABI由本机20H392独立核验；用户提供的shader／镜像／原帧不进Git。此前README原文已归档于docs/archive/20261120/presentation-r1。
