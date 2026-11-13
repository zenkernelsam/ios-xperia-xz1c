# Aegir 壁纸与 Metal — 2026-11-13

v728/r4 已修复只读 LayerKit Surface 和 FD540 非64B对齐纹理输入。它没有提供 Metal device；内核既有 IOAcceleratorES 占位服务没有 MetalPluginName，CA 沿 GLES 路径合成。不能据 GLES 像素通过宣称 Metal 或完整锁屏已完成。

独立异常观察库只在 AegirPoster 运行，使用 objc_setExceptionPreprocessor 记录16帧以内的异常，再转发原异常链，不吞异常。原生 macOS ASan/UBSan 已验证异常身份、20次原链调用和16条记录上限。打包使用 --presigned 保留实测 r4 GLES 和几何库全部字节，另加只读 e 硬链接。GPU 时显式 sweetnull=backboardd,AegirPoster 才能看到该进程输出；默认仅转发 backboardd/SpringBoard 的首次诊断未捕获异常，原 STOP 保留。

第二次实际单发 token9b22a5a27265fbe1 捕获20次异常：NSInvalidArgumentException，原因为 `*** -[__NSArrayM insertObject:atIndex:]: object cannot be nil`。18条栈完整，2条缺帧，缺帧不从另一轮补写。同进程 pid289 原 crash 的 slide0x44630000 用于符号化，核对每帧实际 image，结果在 reproducibility/aegir-r1-symbols.json。

真正调用链为 NUNIAegirRenderer 的 getOrCreateUniformBufferforFrameBufferIndex: → _renderOffscreenBackgroundWithRenderEncoder:frameBufferIndex: → _renderOffscreenSceneWithScene:viewport:commandBuffer:frameBufferIndex:drawableTexture: → renderWithScene:viewport:commandBuffer:passDescriptor: → NUNIQuad → CLKUIMetalQuadView snapshot → AegirPoster。

同版 NanoUniverse 的 0x1ee956b00..0x1ee956b24 指令从 `_device` 读取接收者，调用 `newBufferWithLength:options:`（长度0x1b0=432，options1），立即 addObject:。实际 nil 异常证明加入数组的返回值为空，但仅凭这条栈不能区分 `_device` 本来为空与设备分配返回空。初始化函数调用 CLKUIMetalResourceManager sharedDevice 并存入该 ivar；下一观测直接记录原方法入口的实际设备与 ABI，保留原方法和异常行为。

此次控制器原 STOP_ERROR 是回程健康窗口中另一台 HA2A1PT2 接入，旧全局唯一 fastboot 规则拒绝了库存。原始四标记 ACK/DRAIN/RESET/FINAL_DRAIN 按当前token和顺序完整，原日志446273B/SHAf07ecde88295d60371127e95e8f22bcd088566f1f97f620e8a97dcba3bd11046。没有重发启动，也没有改写原 PASS。

现修订按 Sony 目标行验证，其他序列号可以留在库存中；缺 Sony 不能算返回，重复/格式异常 Sony 行拒绝，所有端点操作仍带 -s BH905ND69G。独立 USB0FCE:0DDE/序列号/端点连续健康31.422秒，零download/boot/reset。完整补充报告在 reproducibility/aegir-r1-interrupted-actual.json，原结果仍 STOP_ERROR。手机健康fastboot，无controller/COM/camera/in-flight。

复现入口：tools/build/build_exception_observer.py、package_glram.py 的 --observer/--presigned、tools/device/exception_evidence.py、interrupted_trial_evidence.py、observe_fastboot.py 与 tools/build/symbolicate_aegir_exception.py。原始 root 只读挂载，ipsw符号缓存写 build，禁止写源镜像。原始证据只保存在本地 archive/evidence；Git仅包含来源、SHA、源码与可复现校验。

后续完整 Metal 支持优先审计 [ios_sagit](https://github.com/MC-XiaoXiao/ios_sagit) 的 QcomMTL/A540 原生路径（当前参考898998cf1b74ea103e0c16d6dc0016822d8d644b）。其 iOS18 ABI、AIR编译和 GPU 私有接口不能直接套用本机 iOS16/SweetScatter。另 [AdrenoMetal](https://github.com/UniXNU/AdrenoMetal) 的 README 明确是 metal2vulkan+Turnip，公开说明的硬件验证在A740，不作为835 Vulkan驱动已存在的证据。保持当前 GLES 路径，补真正 Metal 支持；不以伪造 buffer、吞异常、禁用原壁纸或画占位文字代替完整目标。
