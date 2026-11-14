# 当前状态 — 2026-11-14

Sony BH905ND69G健康fastboot，无controller/COM/camera或in-flight。v729/774b5efd+GLES render-r4/7944112d与Metal79eb8207已实际完成原生function constants快照/AIR specialization/GPU原buffer运算：14完整/0缺项、3phase×64invocation，每份240word含48保护word全部零差异；name/index/range/reset与required/type失败真实核对，创建后改99仍按13/29/37运算。原compute/render/queue各14完整，38.750秒完整PON06回程。详见[函数常量](METAL-CONSTANTS.md)和[原生渲染](METAL-RENDER.md)。

最新discovery-r3已取得28完整原MTLAddDevice→MTLCreateSystemDefaultDevice的真实SweetMTLDevice/SPI/kernelRegistryID证据，同进程先验AIR/GPU全通过，1925资源allocated/released/unmapped、active/orphan/root0，38.375秒完整回程。原controllerSTOP保持，原Aegir已越过nil设备而卡CAMetalLayerInvalid/unsupported extended range format，尚非全系统开机plugin。详见[原版登记](METAL-DISCOVERY.md)。

内核仍v729 SHA774b5efd5f715dc40d81e3c207c85b9b690c83fe44f4fd9061a2189ef7db71ec（20648064B），81源审计保持。新GLES SHA7944112d3478c5717d5ec5f35061d9fb722c154ec8a401a4059843dce63bc292（14711200B）及其CPU/GPU门槛保持，新constants包CPU37.734秒完整返回。Metal SHA79eb8207b62fc04bb54f7368ed7ae49942a04d91da77078f29c19b4db8362924，observer72ee469b。原GLES全尺寸/Surface/caller动态共享/CA/lifecycle及960000只读像素仍PASS；allocated=released1110/unmapped1105、orphan/active/root0，差5的早期具体路径未记录，不称1110全部map/unmap。资源probe14完整/1缺项保留。

r1实际拒绝vertex SSBO，r2误判内建gl_VertexID，r3清屏覆盖1204B padding/尾部，均单发并约38秒完整自动回程，失败记录不改写。r4顶点通过逻辑边界检查的原buffer R32整数纹理取数，清屏用GPU shader draw保护原页面padding，全部实机通过。真实方法体ASan/UBSan及生成ESSL的主机Mesa四phase10032颜色分量均通过。

当前未发布全系统MetalPluginName；Aegir进程内原system默认device登记已实际通过，下一实现其要求的XR10格式，再接一般AIR/vertex attributes、IOSurface texture、depth/stencil/mips/MSAA等。标准native function constants和local/bundle library入口已接，但原system shader/general UI还需验收。面板仍缺时间/日期/壁纸、完整FPS，睡眠唤醒、声音和无线等完整目标active，不能将离屏PASS当完整UI。

下一接系统Metal发现和原生AIR接口覆盖，分别验收原壁纸/时间/日期/主页与其它外设。所有fastboot操作-s BH905ND69G，COM>=10且VID1209PID0001，PON只06，单download/boot、失败不重发；完整四标记回程、连续健康30秒和旧worker退出后下一轮。gl5/SD/TA/XFL/备份不重刷；相机override→AF→2秒→MJPEG→每5帧→rotate90。

[AIR compute](AIR-COMPUTE.md)与前状态全文在[渲染前状态](archive/20261114/STATUS-before-Metal-render.md)。当前源码/输入锁/实际证据均在正式仓库，大镜像和原日志本地保留；本轮冗余媒体/构建缓存按原生回收站整理。
