# 当前状态 — 2026-11-13

最新r2实际CPU/GPU诊断均PASS并完整自动PON06返回37.188/37.641秒，Sony健康fastboot，无controller/COM/camera/in-flight。Aegir原始Metal `_device` 20次真实入口观测全nil，真实index ABI=Q，17完整栈/3缺帧保留；原GLES、全尺寸/共享/lifecycle与960000只读像素仍通过，最后308资源全释放/撤销。面板80帧MJPEG仍缺时间/日期/壁纸，完整目标active。

已开始gpu/metal正式源码移植：iOS16 limits264B的64字段布局与独立元数据逐项原生/arm64编译通过；修正参考iOS18多出maxAccelerationStructureLevels导致maxConstantBufferArguments偏移252/实际248的问题。尚无Metal device/command/render，不能把header或GLES自检通过当完整UI。r2两靴均单download/boot/retries0，回程允许另一台fastboot设备同时连接，所有实际端点命令仍Sony-s限定。

最新 Aegir 诊断已定位原生 Metal uniform buffer 的 nil 返回，详见 AEGIR-RENDERING.md。20次异常有18条完整栈、2条缺帧，缺帧不补猜；原生动态壁纸使用 CLKUIMetalQuadView/NanoUniverse。GLES/Surface通过不代表 Metal device 已提供，实际 `_device` 状态下一直接观测。时间/日期/壁纸仍未恢复，完整目标active。

token9b22a5a27265fbe1 的原controller STOP_ERROR保留：另一台手机接入导致旧全局唯一库存规则中断回程计时。原始四标记完整，独立Sony USB/序列号/端点连续31.422秒健康通过，没有再download/boot/reset。多设备库存已改为严格选择Sony，缺失/重复/非法Sony仍拒绝；所有操作保持-s BH905ND69G。当前Sony健康fastboot，无controller/COM/camera/in-flight。

Sony BH905ND69G当前健康fastboot，无device/COM/camera worker或in-flight。Mac当前源/成功内核为v728/335804584b9e593ffb1b9c153b6309cdd4b4602e8e0b980757aac82647faa3eb（20,664,448B），当前前端r4库b035a887832513f977d8857c4224419b409ee04b64cf90d69ad4a5adba4ce117（14,676,400B）。v728 private/延后owner/ready/RAM CPU(r4)/GPU(r4)均实际单发、四标记及完整自动PON06回程通过。

r4生产bind_surface路径实机通过：只读400×400源offset32走seed-aware复制上传，offset64/128直接共享；每种160000像素两次（包括原VM解除后），总960000像素零差异，CPU视图prot1/max1且写锁返回NotWritable。原750×1624三角形/纹理/Surface双向/caller持有、动态纹理、CA隔离、160relink/320draw和名字复用的独立哈希仍通过。最后281个scatter对象全撤销，caller active/outstanding/retained均0，回程38.25秒含健康30秒。logSHAb2c6af88bf61f7d96e1993ffe2ef305bb9ab5fbdb023b75b440bdbea2c39f7ab。

本靴真实UI有11次400×400成功创建、0个VMError、0条MEMREF ERROR；原权限冲突恢复。AegirPoster仍有6次sig6，面板仍缺时间/日期/壁纸；相机107帧MJPEG，完整底部可见，右侧边缘仍截断。不能把Surface数据PASS当完整视觉完成。下一定位AegirPoster异常名/原因和CA文字渲染路径；声音/无线/睡眠唤醒仍未完成。

r3偏移对比原REVISE保持：Mesa BO IOVA保留完整offset，offset32的GPU哈希cb385e665dfd88dd独立重算等于从页首读取（漏32B），offset64/128两次直接采样与原VM解除后持有均PASS。r4只为base/stride非64B对齐接入既有seed刷新上传，不改原源权限/页面持有或硬件时钟。布局算法全部16384偏移ASan/UBSan通过；36项传输/绑定/独立验收故障回放通过。

首次v728 owner原日志缺snap2/slot2，完整回程但独立验收拒绝；没有补猜/修改原结果。以相同内核另做90秒采样时机对照（watchdog420s）取得两次完整四STREAM/根表哈希一致并37.86秒完整回程。后续准入只用该完整证据，控制器已在PASS前检查完整owner元数据。private37.281、ready38.828、r4 RAM CPU37.125秒完整回程。关键报告见reproducibility/readonly-r4-*-actual.json，所有源码及输入绑定在正式仓库；大镜像/原日志仅本地build/archive。

以下为原v727/v726记录：

v727 private37.438/owner38.422/ready37.625/RAM CPU37.328秒均完整PON06回程。GPU实际REVISE：caller layout失败，旧vm_region_recurse_64包装对4GiB以上地址因VM_MAX_ADDRESS=0xF0000000拒绝；全尺寸绘制/普通Surface先通过，readonly未执行。原失败logSHAb7ec970b/镜像95397c91/单发与完整38.687秒回程记录保留。v728只修该接口，不把v727的四阶段通过当GPU完成。详细解释见SURFACE-READONLY.md。

本轮先在v726上新region-r1库18a97e2c完成CPU/GPU真实单发并完整自动返回（37.735/38.672秒），实证12个400×400源prot1/max1/tag51，与kIODirectionInOut冲突；所有可写自检数据仍PASS，最后287对象撤销。记录见region-r1-*-actual.json。v727据此增加只读共享方向/CPU映射/GPU叶权限，主机真实方向/mapper/原5/12MiB回归ASan/UBSan通过。首编失败保存后修订公开VM包装接口编译成功；全目标仍active，详见SURFACE-READONLY.md。

以下为上一轮结束时v726记录：

Sony BH905ND69G 当前健康fastboot，无device/COM/camera worker或in-flight。Mac当前源和成功内核均v726，kernel cda82b99792d4e6ae7c93ef57481f44e69e39f577b4369a3856a889380a484bf（20,647,936B）。v726五阶段private/owner/ready/RAM CPU/GPU均实际单download/boot、retries0并完整PON06返回；回程含健康30秒分别37.297/37.766/38.125/37.688/38.375秒。

v726明确捕获真实故障：24条memory-entry采样全部KERN_PROTECTION_FAILURE=2，prot0x100003要求读写、map page_mask0x3fff为16KiB，descriptor flags0x110913。关联400×400/640000B源，地址偏移32B，页覆盖655360B；本靴完整范围共1127次400×400创建失败、20次AegirPoster sig6。用户态observe_init原样转发属性，未额外注入地址。源区域的protection/max_protection和精确ObjC异常原因仍未观测，尚不能将错误2直接等同于某个已证明的只读映射契约。

原GLES9a59820c所有独立像素/双向共享/生命周期核验仍通过，最后283个scatter对象全部撤销，caller active/outstanding/retained均0。相机MJPEG73帧，保存的画面仍无时间/日期/壁纸，右侧/底部截断；数据PASS与视觉INCOMPLETE分开。GPU logSHA19d54d4f6371df498aaa0e00a604a9af91d531b079a64b872cb1c78681c37ee4。

版本化79源/编译内核校验与完整payload复算已完成，32项故障回放通过。mkpayload支持内存字节输入，package_trial不再落盘一次性提取blob。最新证据报告在reproducibility/v726-*-actual.json，必要原日志/单张面板图仅本地archive/evidence/v726-20261113。下一在用户态观察器记录创建前的源VM区域保护权限、范围及完整属性名/类型，沿用已通过的v726内核，仅重走新库RAM CPU/GPU准入；还需将打包/验收中的RAM库输入、SHA/大小/FNV参数化，拒绝借用旧库CPU证据。之后实现正确的Surface创建/共享路径。完整UI/FPS、睡眠唤醒、声音与无线目标继续active。

以下v725及v724记录保持原结论：

v725诊断内核3a9a27b36139423fe8ef0ef1f03f8b47a22bf38e7d4557b3f537cf8099547dad已完成private、owner、ready、RAM CPU、GPU五阶段单发验收。仅增加Surface有界日志，既有分配/pin/map/release不变。全部正常PON06返回，无人工；目前Sony健康fastboot、无worker/in-flight。

GPU仍为原已验证GLES9a59820c：750×1624三角形/纹理、Surface双向、caller原VM解除后持有、小纹理动态刷新、CA隔离、160次relink/320draw与名字复用全部独立哈希通过。最后194个scatter对象全部释放/撤销，caller active/outstanding/retained均0；GPU回程38.938秒含健康30秒。原始logSHAff863fac9e36d7dee6ea0ad9b1b84f26b528fd262204ea1e47ce93282787695e。

本轮约180秒观察仍未见时间/日期/壁纸。相机MJPEG可用，取景右侧/底部截断，不能据此核验完整面板或底部导航条。出现69×69/alloc19044与67×67/alloc17956两个caller创建失败：均在withAddressRange阶段返回NULL，对应32KiB覆盖；日志0xe00002bd是包装层占位值，底层实际失败码尚未知。两次acquire=SCA_ALLOCATION_FAILED，没有到prepare/map。此次未复现旧400×400大量失败/AegirPoster退出，不能视为已解决。

正式run_trial准入已补真实RPM三ACK、同内核private/owner/ready/CPU门槛、Mac78实际源及编译内核SHA。28项离线测试全部通过，包含原始RPM/owner/GPU日志故障回放；v725构建/打包/验收入口见SURFACE-DIAGNOSTICS.md。下一追查memoryReferenceCreate的底层错误/范围与权限，另核对真实CA文字/壁纸路径；不凭NULL猜改方向或扩大页面。

v726构建过程：IOMemoryDescriptor.cpp固定Apple前源9963ef1a→7d377fb6，只加memory-entry失败与init失败两处日志（各24条）。实际插入体的成功/flags/task/NULL-map/计数边界ASan/UBSan通过，去除日志后原文件逐字节一致。首次构建因SSH环境和jq缺项停在依赖提示，已只终止原进程并保留日志/旧对象/内核；jq已补齐，第二次实际编译及79源/字符串链接核验通过，session14180已结束。最终实机结果见本页顶部。

以下为保留的v724基线记录：

BH905ND69G健康fastboot，无device/COM/camera worker。内核v724 SHAda6ddc44fc074eb393f135e1395798483fe2a52eb420dfecea883e0833136c9a；GLES库SHA9a59820c7e54b1626cacd49acb70a7d460be3c01c415a17429a2464bb5b11571（14,675,424B）。

GPU核心、Surface双向1,218,000像素、caller创建后修改/GPU写回/原VM解除后持有、动态35,600像素、CA双context384像素、shader/program20,480像素与真正名字复用实机通过。

180秒面板实际182.823s，38.547s完整PON06自动回程含健康30秒。三批照片仍只见充电标志、相机按钮、导航条；时间/日期/壁纸缺。关闭前1129次400×400 Surface VMError，20次AegirPoster退出，符号栈为ObjC/C++terminate与FBSSerialQueue，原因尚未捕获。

原controller数据PASS与视觉INCOMPLETE分别保存，失败不改写。logSHA182d3e4b1e335565320c6e8fa78d6cad3d0d28b7cbd80e5de871a4d0d4224e1b；原包418文件/248,653,297B的结果与SHA清单保持，当前关键原日志和三张面板照片保留，其余一次性副本已进回收站。最后277对象全部释放/撤销，orphan0/active0/root0。

正式仓库当前源已重新编译XNU、GLES与shim，结果见reproducibility/build-results.json和shim-build-results.json。新内核e0eb264c/GLES f22dc3d0/shim77d425b0未上机，不代替此前实机通过的da6ddc44/9a59820c基线。

下一步在整理/复现完成后核对caller acquire/prepare/map与AegirPoster异常，不先猜改色。声音CORE APR/AVS2.8已过，实际播放录音/codec/iOS16前端未过，无线同样未完成；完整目标active。

整理已完成目录分类和实际回收，正式源码推送并在Windows/Mac同步，Patch/Xperia待提交项归零。经过实机验证的内核/库/打包blob已移入正式assets/baselines，SHA核对与上述基线一致；整理报告记录了部分Windows旧回收条目的当前缺失，未将历史移入验证当作当前恢复保证。
