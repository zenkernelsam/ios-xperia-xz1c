# 当前状态 — 2026-11-14

Sony健康fastboot，无controller/COM/camera/in-flight。backboard-r1整组native资源/queue/compute/render/constants/XR/Surface/mipmap已实机1完整/0缺项；后台等待原GLES八项成功后运行，原Aegir未注入或登记。四格式六级、重生成、24次LOD/trilinear和917504B全原字节/保护区通过，MIPS耗时2360ms。431全释放/unmapped、active/orphan/root0，37.797秒完整回程。完整UI仍未验收，详情见METAL-BACKBOARD.md。

原生Surface验收surface-r1：13完整/1未完成样本，两种64/128字节逻辑origin各三phase；原IOSurface GPU清屏、创建者VM/CF释放后scissor AIR绘制、再GPU采样到原buffer，194688B包含84240B行尾padding全零差异。1652allocated/released/unmapped、active/orphan/root0，45.859秒完整PON06回程。详见[IOSurface](METAL-IOSURFACE.md)。

最新原系统登记轮还有10完整/0缺项登记与IOSurface样本；compute/render/constants/queue各12完整，XR10完整/2缺项。原Aegir已进入纹理mipmap准备阶段。1442allocated/released、1441unmapped，active/orphan/root0，不称1442全部map/unmap；38.282秒完整回程。原PASS和原missing-selector异常分别保留在reproducibility/v729-surface-discovery-r1-gpu-actual.json。

XR格式552/553/554/555的独立xr-r1实机16完整/1缺项样本、559104B原码/采样/padding零差异；按固定LGPL开源参考的原表和编码实现。只移植颜色表示，没有导入Vulkan驱动。GPU解码到float shadow后真实AIR采样，XR混色和部分通道写仍拒绝。详见[XR格式](METAL-XR.md)。

固定内核仍v729 SHA774b5efd5f715dc40d81e3c207c85b9b690c83fe44f4fd9061a2189ef7db71ec（20648064B），81源实际审计。当前GLES499bc6e9ddcf8143a0cc5578a3b8dd9c6ce3396f660917b7ac5655aaedf25cf3（14711888B），Metal a0b58e86b8f65a4541a56bf901c9606def3f72a34730c98ed3a4256d4275588f。新GLES先CPU/GPU，Metal Surface及登记包分别再CPU/GPU，单发和完整回程均已验证。GLES全尺寸/CA lifecycle及960000只读像素保持通过。

下一把Aegir原设备登记与长诊断组分离，并严格使用当前完整backboard native/GLES/Metal字节先验；再验原壁纸真实请求。奇数尺寸/不同级数/Surface mip硬件检查也待完成。系统发现仍限Aegir内原MTLAddDevice；全系统开机MetalPluginName、一般AIR/attributes/half、广义IOSurface/YUV、array/cube/MSAA/depth/stencil、完整时间/日期/壁纸/UI/FPS与睡眠/声音/Wi-Fi/蓝牙仍未完成，完整目标active。

r2绑定GLES470c56438bda618cc40521140601dae124b8cc88e1333ea8735a3a52438c4425、Metal a0b58e86b8f65a4541a56bf901c9606def3f72a34730c98ed3a4256d4275588f；内核仍v729/774b5efd、81源审计。token673c15c98ea11c4e，镜像98fb4a499094a1dc631b89ca5da4b724808f673f3335ae7bf9aa47504ea15d1f，631519B原日志SHA541f3b50696dc85bd09fbcc9d76edbc1b7ef26045856d6407e331acf2eb88699。r1/r2原结果及完整回程保留，不提升为完整mipmap或UI验收。

所有操作只BH905ND69G，COM>=10且VID1209/PID0001，PON只06；单download/boot、失败不重发，完整四标记回程/健康30秒/旧worker退出后下一轮。gl5/SD/TA/XFL/备份不重刷。相机override→AF→2秒→MJPEG→每5帧→rotate90。

历史状态完整保留[XR/Surface之前](archive/20261114/STATUS-before-XR-and-IOSurface.md)及更早archive。正式源/输入锁/独立验收在本仓库，大镜像和原日志本地保留；本轮重复HFS/媒体通过各自原生回收站清理，不清空回收站。
