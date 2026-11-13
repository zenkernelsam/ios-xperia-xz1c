# 当前状态 — 2026-11-13

Sony BH905ND69G当前健康fastboot，无controller/COM/camera或in-flight。v729/774b5efd+r5/325810 private、延后owner、ready、r5 RAM CPU/GPU和新Metal资源RAM CPU/GPU均实际单发/零重试、四标记和健康30秒完整自动PON06回程通过。Metal资源12组×3buffer的实际分配/CPU字节/原device释放后持有/最终归零通过；资源GPU回程38.657秒。

v729内核774b5efd5f715dc40d81e3c207c85b9b690c83fe44f4fd9061a2189ef7db71ec（20648064B）已编译、81源审计通过，private两次VM/SMEM与36.750秒完整自动返回通过。日志快照改为reader不持writer锁；并发/回绕ASan/UBSan/TSan与arm64通过。仍有少量producer丢弃，不能宣称日志完全无损；GPU完整记录可靠性下一核对。GPUallocator/SCM/PON/RPC/VM原源码保持。

正式Metal前端在gpu/metal，Shared buffer沿用既有Mesa/SweetScatter分配/释放/cache同步，自有limits不改Apple私有ivars。C bridge和Metal对象本体主机ASan/UBSan与iOS arm64编译通过，显式工厂及真实buffer实机12组生命周期通过。没有system Metal device、AIR shader、command queue或render/compute，不发布MetalPluginName。源码与边界见[METAL-RESOURCES.md](METAL-RESOURCES.md)。

实际Metal ABI r3取得12份完整、4份缺项样本：初始化端口uint32，原抽象_MTLDevice未实现featureProfile，前端须自行提供。原Aegir20次_device=nil已明确导致原生Metal壁纸失败；时间/日期是否另有CA文字问题仍待查。

GLES r5签名库325810eabb913247e33707e7e83a69e34052bbb3cbf3de75e3152e32c08f82f4（14676688B）新增三个Metal backing C接口；v728上CPU37.391秒完整通过。GPU原controller汇总PASS、38.766秒回程保持，但独立验收缺60×60初始kernel OPEN；拒绝作完整GPU门槛，未重发同靴。r3同样缺22×27测试结束行，其ABI与回程可以独立验证，绘制不宣称完整通过。

最新完整GPU基线为v729/774b5efd与r5/325810：750×1624、Surface/caller共享、动态纹理、CA、shader生命周期及960000只读像素通过并完整PON06回程，295资源全部释放/撤销，orphan/active/root均0。这一靴之前缺失的OPEN与结束记录完整，原v728失败不改写。时间/日期/壁纸、完整UI/FPS、睡眠唤醒、声音、无线等目标仍active。

下一继续Metal command queue、AIR→Mesa和texture/render接入；资源层通过不代表这些功能通过。所有fastboot操作-s BH905ND69G、COM>=10/VID1209PID0001、单download/boot/retries0、PON仅06；gl5/SD/TA/XFL/备份保留。

旧记录全文在[归档状态](archive/20261113/STATUS-before-metal-resources.md)，早期失败与证据不改。
