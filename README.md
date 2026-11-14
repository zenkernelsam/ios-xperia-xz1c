# iOS on Xperia XZ1 Compact — lilac / SO-02K

Sony Xperia XZ1 Compact（Snapdragon835 / Adreno540）的开源XNU、shim与Mesa/GLES移植。本仓库是唯一正式源码；Patch/Xperia只保存参考仓库和本地历史。

已验证8核CPU、launchd、F12触控与音量键、FD540/Mesa GLES3.1硬件绘制、全750×1624读回、IOSurface/GPU共享、caller页面持有、动态纹理、CA上下文隔离与shader/program生命周期。160次relink/320draw的20,480像素零差异，单发测试经PON0x06自动回fastboot。

v728内核与r4前端已恢复只读LayerKit Surface创建：只读CPU/GPU背板持有及非64B对齐纹理刷新上传，三个400×400偏移的960000像素实机零差异，原可写共享/生命周期仍通过。真实UI的400×400 VMError本靴已消失。

最新v729/render-r4保持完整GLES通过，原生Metal资源、GPU队列、MTLB/AIR compute及顶点/片元绘制已实机核验。10组清屏/viewport/scissor/cull/动态纹理结果与原页面保护区零差异，830资源全撤销并38.594秒完整PON06回程。system Metal、一般AIR/广义IOSurface texture与完整UI仍未完成，边界见[原生渲染](docs/METAL-RENDER.md)、[AIR计算](docs/AIR-COMPUTE.md)、[队列](docs/METAL-QUEUES.md)和[资源](docs/METAL-RESOURCES.md)。

最新[native function constants](docs/METAL-CONSTANTS.md)也已实机通过14完整样本：name/index/range/reset和冻结AIR实际GPU运算，所有结果/保护区零差异，38.750秒完整回程。system Metal设备发现与原生UI继续推进。

最新[原生设备登记](docs/METAL-DISCOVERY.md)在Aegir进程内实机通过28完整样本；原system默认device现在返回真实A540设备，原壁纸推进到XR10 extended-range格式要求。XR10四格式和单平面IOSurface的真实GPU验收现已完成，详见[XR格式](docs/METAL-XR.md)与[IOSurface纹理](docs/METAL-IOSURFACE.md)。原壁纸继续到blit encoder的mipmap方法缺失；全系统自动plugin与完整UI仍未完成。

当前仍未完成：真实锁屏时间/日期/壁纸、完整UI/FPS、睡眠唤醒、声音、Wi-Fi/蓝牙等。AegirPoster仍重复异常退出；180秒面板仍缺文字与壁纸。不能把离屏PASS当完整系统完成。

[当前状态](docs/STATUS.md) · [固定源码与构建](docs/BUILD.md) · [操作规则](docs/OPERATIONS.md) · [目录与恢复](docs/LAYOUT.md)

整理与回收记录：[2026-11-12](docs/ORGANIZATION-2026-11-12.md)。正式源码、构建入口和旧文档归档均在本仓库；大文件仅本地保存。

内核修补在xnu/lilac-current.patch，GLES源码在gpu/sweetgl，Mesa修补在gpu/mesa-lilac.patch。固定版本重新检出并应用后，404个XNU源码变更路径、78个审计源和7个Mesa变更源与实机编译输入一致；旧41个补丁残片和Finder元数据已排除。

iPhoneX iOS16.7.16/20H392镜像、Sony原厂固件及本机备份由使用者提供，不在Git仓库。构建不自动刷机，不删除现有资料。
