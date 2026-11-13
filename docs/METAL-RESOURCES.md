# 原生 Metal 前端与资源 — 2026-11-13

MetalResource代码现位于gpu/metal，使用同一进程的Mesa/SweetScatter设备。MetalDeviceLimits16.h按原版20H392全部64字段编译核对；MetalResources.mm继承真实_MTLDevice，自有limits存储不写Apple私有ivars，显式SweetMTLCreateDevice工厂提供Shared buffer。还没有command queue、AIR shader、render/compute或系统MetalPluginName，不能据此称完整Metal驱动。

实际r3 ABI观测取得12份完整、4份缺记录样本，保留缺项。设备初始化签名是@20@0:8I16，端口uint32；buffer contents为^v、length/gpuAddress为Q，queue commandBuffer返回对象，commit/wait为void，回调为block。原抽象_MTLDevice没有featureProfile实现，自有前端必须提供该SPI getter。原r3 controller汇总PASS保持，但独立GPU验证因22×27动态纹理结束行缺失而拒绝；正常38.500秒完整PON06回程。报告metal-abi-r3-actual.json区分ABI通过与完整绘制未通过。

Metal backing API只调用既有分配/释放/cache同步。logical byte与16KiB页跨度分开，支持432、16384、16385等非整页请求；检查GPU窗口0x40000000..0x80000000、映射对齐、大小与所有权。Metal对象持有设备，设备原引用释放后buffer仍有效；currentAllocatedSize按实际页跨度记账。释放超时保留底层所有权，不能假称页已释放。当前mapper是cached CPU页，WriteCombined仅作优化请求，API如实报告DefaultCache；以后再做缓存策略优化。

真正C bridge和Metal对象本体的主机ASan/UBSan覆盖了非法长度/选项、错误映射、非Metal句柄、释放超时、cache同步失败、设备生命周期及记账；iOS arm64编译通过，metal-resource-host-check.json记录原输入SHA。build_metal_library.py构建独立资源库；package_glram.py可把它与相同GLES及观察器打入只读RAM，Metal真实探针的GPU准入额外要求完整、同库的GLES实机证据。主机模型不是实际GPU backing通过。

GLES r5新增三个C资源接口，正式源码和固定Mesa依赖已重编，签名库325810eabb913247e33707e7e83a69e34052bbb3cbf3de75e3152e32c08f82f4。v728上CPU37.391秒完整通过；GPU虽有各项像素输出、controller汇总PASS和38.766秒正常回程，但独立验收缺60×60初始kernel OPEN记录，拒绝作完整GPU证据。原日志保留，不重发同靴，不以另一个surface的记录补缺。

为提高无人值守验收可靠性，v729只改console mirror：旧USB reader/copy和writer共用trylock，读期间新输出直接计入dropped。新reader用版本号和原子字节快照，不持writer lock、不等待；并发写期间快照丢弃并不推进cursor。writer继续非阻塞，不改USB DMA、GPU allocator/SCM/PON/RPC/VM。C原源的CRLF副本逐SHA保留，再按明确LF配方应用增量；此路径也适用于从正式仓库重建的LF源。ASan/UBSan/TSan与arm64编译通过，console-snapshot-host-check.json记录输入，尚需实机证明记录可靠性。

v729 kernel774b5efd5f715dc40d81e3c207c85b9b690c83fe44f4fd9061a2189ef7db71ec已成功编译，81源审计通过。先同内核private、延后owner、ready、相同r5库CPU/GPU完整返回，再单发Metal资源探针。手机操作始终Sony BH905ND69G、COM>=10、PON06、一次download/boot且失败不重发；实际buffer、原壁纸、时间/日期、完整UI与外设分别验收。
