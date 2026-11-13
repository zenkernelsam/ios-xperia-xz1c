# iOS16 Metal 移植

这里维护本机Metal前端。GLES/Mesa路径在gpu/sweetgl；自有_MTLDevice、Shared buffer生命周期及原生command queue的GPU copy/fill已实机核验。队列20份完整FIFO/包/fence/660480B/回调通过；尚无system Metal/AIR/texture/render，不发布MetalPluginName。边界见docs/METAL-QUEUES.md及docs/METAL-RESOURCES.md。

MetalDeviceLimits16.h 从 BSD-2-Clause 的 ios_sagit NativeMetal.h 改编，并对实际20H392 Metal `_MTLDevice._limits` 的全部64字段核对布局。两版大小都264B，iOS16没有 maxAccelerationStructureLevels；maxConstantBufferArguments 位于248，iOS18位于252，maxBufferLength均256。tools/build/check_metal16_projection.py 已用独立原始元数据生成每字段offsetof断言，原生和iOS arm64编译均通过。其余方法签名、基类初始化和资源/队列接口尚需本机验证。

参考固定为 [ios_sagit](https://github.com/MC-XiaoXiao/ios_sagit) 898998cf1b74ea103e0c16d6dc0016822d8d644b。QcomMTL.mm 的 nativeMetalABI 明确仅接受iOS18.6/18.7，不能只删版本判断。原GPUUserProtocol使用独立ConnectionType0x47505502、协议2和GetInfo/CreateContext/CreateQueue/CreateBuffer等selector；本机SweetScatter是另一组10..16选择器和16KiB页面，必须实现明确适配，不能把该插件直接交给当前服务。

真正目标仍是系统Metal device、AIR着色器、command queue、渲染/采样、IOSurface共享与原生壁纸/UI。GPUallocator/SCM/PON/RPC保持；header和主机资源测试通过不作为硬件驱动通过。原Aegir `_device=nil` 的20次实际入口观测与自动回程见reproducibility/aegir-r2-gpu-actual.json，完整锁屏仍缺时间/日期/壁纸。
