# 当前状态 — 2026-11-14

Sony BH905ND69G健康fastboot，无controller/COM/camera或in-flight。最新surface-discovery-r1已让原Aegir越过XR格式和IOSurface入口，卡在SweetMTLBlitEncoder的generateMipmapsForTexture:缺方法。原controller PASS对应原生探针目标；原应用异常原因和16帧栈已完整保存，不代表壁纸或完整UI通过。

原生Surface验收surface-r1：13完整/1未完成样本，两种64/128字节逻辑origin各三phase；原IOSurface GPU清屏、创建者VM/CF释放后scissor AIR绘制、再GPU采样到原buffer，194688B包含84240B行尾padding全零差异。1652allocated/released/unmapped、active/orphan/root0，45.859秒完整PON06回程。详见[IOSurface](METAL-IOSURFACE.md)。

最新原系统登记轮还有10完整/0缺项登记与IOSurface样本；compute/render/constants/queue各12完整，XR10完整/2缺项。原Aegir已进入纹理mipmap准备阶段。1442allocated/released、1441unmapped，active/orphan/root0，不称1442全部map/unmap；38.282秒完整回程。原PASS和原missing-selector异常分别保留在reproducibility/v729-surface-discovery-r1-gpu-actual.json。

XR格式552/553/554/555的独立xr-r1实机16完整/1缺项样本、559104B原码/采样/padding零差异；按固定LGPL开源参考的原表和编码实现。只移植颜色表示，没有导入Vulkan驱动。GPU解码到float shadow后真实AIR采样，XR混色和部分通道写仍拒绝。详见[XR格式](METAL-XR.md)。

固定内核仍v729 SHA774b5efd5f715dc40d81e3c207c85b9b690c83fe44f4fd9061a2189ef7db71ec（20648064B），81源实际审计。当前GLES SHA4241a0200f974dffe3513c9ff225fdd2f4c8833530d94167d2884d61e1d429d1（14711552B），Metal SHAf8d7839652c898938f04b8a1dcb0d80414fd3847b5dbfaabf79bbb5529b77365。新GLES先CPU/GPU，Metal Surface及登记包分别再CPU/GPU，单发和完整回程均已验证。GLES全尺寸/CA lifecycle及960000只读像素保持通过。

下一实现真正mipmap：先确认原请求的texture和levels，再实际GPU生成/采样并验证每级原字节与保护区，不能用空方法或支持查询掩盖缺功能。系统发现当前仍限Aegir内原MTLAddDevice；全系统开机MetalPluginName、一般AIR/attributes/half、广义IOSurface/YUV、多mip/array/cube/MSAA/depth/stencil、完整时间/日期/壁纸/UI/FPS与睡眠/声音/Wi-Fi/蓝牙仍未完成，完整目标active。

所有操作只BH905ND69G，COM>=10且VID1209/PID0001，PON只06；单download/boot、失败不重发，完整四标记回程/健康30秒/旧worker退出后下一轮。gl5/SD/TA/XFL/备份不重刷。相机override→AF→2秒→MJPEG→每5帧→rotate90。

历史状态完整保留[XR/Surface之前](archive/20261114/STATUS-before-XR-and-IOSurface.md)及更早archive。正式源/输入锁/独立验收在本仓库，大镜像和原日志本地保留；本轮重复HFS/媒体通过各自原生回收站清理，不清空回收站。
