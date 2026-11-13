# 当前状态 — 2026-11-14

Sony BH905ND69G健康fastboot，无controller/COM/camera或in-flight。v729/774b5efd+r8/c84403c1与Metal/575e0764已实际完成MTLB bitcode→system LLVM C→开源AIR lowering→ESSL/Mesa→FD540→原Metal buffer共享页面的非空compute。22完整/0缺项样本，2phase×64invocation，每phase80word全检含16保护word，两哈希独立重算一致。892资源全部released/unmapped、orphan/active/root0；单download/boot/retries0、四标记/健康30秒与38.343秒完整PON06自动回程通过。详见[AIR compute](AIR-COMPUTE.md)。

内核仍v729 SHA774b5efd5f715dc40d81e3c207c85b9b690c83fe44f4fd9061a2189ef7db71ec（20648064B），81源审计保持。r8 GLES库c84403c11f8a1141e5072e9e2ae8805f61ee175aea25e568a4ee83613bb9a680（14694112B），原GLES CPU37.454/GPU38.609和新AIR包CPU37.625秒完整回程均通过。GLES全尺寸/Surface/caller/动态纹理/CA/lifecycle与960000只读像素仍PASS。

原生MTLDevice子类、Shared buffer持有、command queue FIFO/GPU copy/fill、MTLLibrary/Function/ComputePipeline/ComputeEncoder非空计算已通过对应实机门槛。未发布system MetalPluginName，原Aegir的system device仍nil；原生texture/render/IOSurface、function constants及一般graphics shader未完成。面板仍缺时间/日期/壁纸、完整FPS，睡眠唤醒、声音和无线等目标继续active。

开源AIR源固定ios_sagit898998cf，12文件原字节/许可证/SHA保留。168最终LLVM C导入在原20H392 cache逐项找到；host强制旧API/ASan/UBSan、MTLB hash/stage与真实方法体测试通过。不是Apple私有C++ ABI或Vulkan方案，host与硬件证据分开。

后续沿既有资源/队列/compute接native Metal texture/render和system device discovery，分别验收原壁纸、时间/日期、主页与其它外设。所有fastboot操作-s BH905ND69G，COM>=10且VID1209PID0001，单download/boot且失败不重发，PON只06，完整回程/健康30秒/旧worker退出后下一轮；gl5/SD/TA/XFL/备份不重刷。

旧记录全文在[前状态](archive/20261114/STATUS-before-AIR-compute.md)，原队列r2缺kernel日志及其它失败不改写。
