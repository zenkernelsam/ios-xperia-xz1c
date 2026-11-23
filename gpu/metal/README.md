# 开源Metal用户态前端

当前准确输入、设备与下一步统一看[STATUS](../../docs/STATUS.md)；固定构建方法见[BUILD](../../docs/BUILD.md)。正式源码只维护ios-xperia-xz1c，内核GPU底层、allocator、SCM、PON、RPC、VM及SweetGPU冻结。

当前Metal保留原资源、queue、MTLB／AIR、MRT、sampler、XR、IOSurface、mipmap、缓存，以及Depth32、Stencil8、Memoryless、2／4×MSAA和[原生呈现](../../docs/METAL-PRESENTATION.md)。r11完整native21与PNG6已实际通过；probe自有CA事务闭合后五个呈现阶段分配均0，原pixels、padding、时间与options保持。旧r10残留、附件和安装失败原样保留，不复发。

原UI与功能probe独立验收：原生720×1280／360×640点／scale2、平直状态栏与六行／五行的完整38应用、Dock／Folder保留已有同靴证据。R21标签真实祖先及两处17.5点间距独立通过，原R20 REVISE保留；完整safe-area、黑壁纸、光学、动态UI／FPS、睡眠与其它外设继续推进。原Poster mode／environment只按原请求观察，不能强制unpause或画替代帧冒充原UI；原33方法／deactive结果见[请求专题](../../docs/METAL-REQUEST-OBSERVATION.md)。

GPU对象与呈现契约参考固定[ios_sagit](https://github.com/MC-XiaoXiao/ios_sagit/tree/898998cf1b74ea103e0c16d6dc0016822d8d644b)的BSD-2-Clause方案，Mesa／Freedreno为固定24.3.4；iOS16 ABI用用户提供的准确20H392输入只读核验。Vulkan尚未移植。新库须自身CPU／完整GLES／与能力相符的native／PNG准入，旧proof拒绝新功能。Apple二进制／shader／原帧保持本地，Git保存原创修补、工具和许可来源。

本轮原README与50份手册的准确字节／SHA在[主归档manifest](../../docs/archive/20261123/poster-state-r22-close-out/original-document-manifest.json)，另外三份入口见[补充manifest](../../docs/archive/20261123/project-entry-supplement/original-document-manifest.json)。本周回收路径与文档职责由[ORGANIZATION](../../docs/ORGANIZATION.md)维护。
