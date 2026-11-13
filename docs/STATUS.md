# 当前状态 — 2026-11-14

Sony BH905ND69G健康fastboot，无controller/COM/camera或in-flight。v729/774b5efd+GLES render-r4/7944112d与Metal1b8d1f5d已完成标准MTLB/AIR→system LLVM C→开源lowering→ESSL/Mesa FD540→原Metal texture页面的实际vertex/fragment渲染。10完整/0缺项、4phase清屏/viewport/scissor/cull/纹理宽高query及动态上传；171520B全逻辑范围含65600B保护区零差异，独立hash一致，原页面IOVA不变。830资源全部released/unmapped，orphan/active/root0；38.594秒完整PON06自动回程。详见[原生渲染](METAL-RENDER.md)。

内核仍v729 SHA774b5efd5f715dc40d81e3c207c85b9b690c83fe44f4fd9061a2189ef7db71ec（20648064B），81源审计保持。新GLES SHA7944112d3478c5717d5ec5f35061d9fb722c154ec8a401a4059843dce63bc292（14711200B），CPU37.281/GPU38.750秒和含Metal新包CPU37.891秒均完整返回。Metal SHA1b8d1f5da0463abd280176c8f1db15073c14506ead19e8279b43fd42249279bf，observer e38330b3。原GLES全尺寸/Surface/caller动态共享/CA/lifecycle及960000只读像素仍PASS；本靴原compute/queue/resource各10完整。

r1实际拒绝vertex SSBO，r2误判内建gl_VertexID，r3清屏覆盖1204B padding/尾部，均单发并约38秒完整自动回程，失败记录不改写。r4顶点通过逻辑边界检查的原buffer R32整数纹理取数，清屏用GPU shader draw保护原页面padding，全部实机通过。真实方法体ASan/UBSan及生成ESSL的主机Mesa四phase10032颜色分量均通过。

当前仍未发布system MetalPluginName，原Aegir system device仍nil；function constants、一般AIR/vertex attributes、IOSurface texture、depth/stencil/mips/MSAA等继续待完成。面板仍缺时间/日期/壁纸、完整FPS，睡眠唤醒、声音和无线等完整目标active，不能将离屏PASS当完整UI。

下一接系统Metal发现和原生AIR接口覆盖，分别验收原壁纸/时间/日期/主页与其它外设。所有fastboot操作-s BH905ND69G，COM>=10且VID1209PID0001，PON只06，单download/boot、失败不重发；完整四标记回程、连续健康30秒和旧worker退出后下一轮。gl5/SD/TA/XFL/备份不重刷；相机override→AF→2秒→MJPEG→每5帧→rotate90。

[AIR compute](AIR-COMPUTE.md)与前状态全文在[渲染前状态](archive/20261114/STATUS-before-Metal-render.md)。当前源码/输入锁/实际证据均在正式仓库，大镜像和原日志本地保留；本轮冗余媒体/构建缓存按原生回收站整理。
