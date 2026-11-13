# 当前状态 — 2026-11-13

Sony BH905ND69G当前健康fastboot，无controller/COM/camera或in-flight。v729/774b5efd+r7/db47937b与自有Metal库a2b11a46的队列r3实机通过：20份完整/0缺项的FIFO、80次chunk提交/等待、660480B全区域字节/保护与scheduled/completed回调一致；38.454秒四标记/健康30秒完整PON06自动返回。原GLES全链/960000只读像素与503资源全部撤销仍通过，system Metal/AIR/texture/render和完整UI未完成。详见[原生队列](METAL-QUEUES.md)。

v729内核774b5efd5f715dc40d81e3c207c85b9b690c83fe44f4fd9061a2189ef7db71ec（20648064B）已编译、81源审计通过，private两次VM/SMEM与36.750秒完整自动返回通过。日志快照改为reader不持writer锁；并发/回绕ASan/UBSan/TSan与arm64通过。仍有少量producer丢弃，不能宣称日志完全无损；GPU完整记录可靠性下一核对。GPUallocator/SCM/PON/RPC/VM原源码保持。

正式Metal前端在gpu/metal，Shared buffer沿用既有Mesa/SweetScatter分配/释放/cache同步，自有limits不改Apple私有ivars。显式工厂、真实buffer生命周期和原生command queue的GPU copy/fill已实机通过。尚无system Metal device、AIR shader或texture/render/compute，不发布MetalPluginName。资源边界见[METAL-RESOURCES.md](METAL-RESOURCES.md)。

实际Metal ABI r3取得12份完整、4份缺项样本：初始化端口uint32，原抽象_MTLDevice未实现featureProfile，前端须自行提供。原Aegir20次_device=nil已明确导致原生Metal壁纸失败；时间/日期是否另有CA文字问题仍待查。

GLES r5签名库325810eabb913247e33707e7e83a69e34052bbb3cbf3de75e3152e32c08f82f4（14676688B）新增三个Metal backing C接口；v728上CPU37.391秒完整通过。GPU原controller汇总PASS、38.766秒回程保持，但独立验收缺60×60初始kernel OPEN；拒绝作完整GPU门槛，未重发同靴。r3同样缺22×27测试结束行，其ABI与回程可以独立验证，绘制不宣称完整通过。

最新完整GLES基线为v729/774b5efd与r7/db47937b：750×1624、Surface/caller共享、动态纹理、CA、shader生命周期及960000只读像素通过并完整PON06回程，281资源全部释放/撤销；同库队列r3包含503资源全撤销。原队列r2因kernel日志32次上限没有完整fence证据，原记录不改。时间/日期/壁纸、完整UI/FPS、睡眠唤醒、声音、无线等目标仍active。

下一继续AIR→Mesa和Metal texture/render、系统device发现接入；copy/fill队列通过不代表原生shader或完整UI通过。所有fastboot操作-s BH905ND69G、COM>=10/VID1209PID0001、单download/boot/retries0、PON仅06；gl5/SD/TA/XFL/备份保留。

旧记录全文在[归档状态](archive/20261113/STATUS-before-metal-resources.md)，早期失败与证据不改。
