# 当前状态 — 2026-11-13

v725诊断内核3a9a27b36139423fe8ef0ef1f03f8b47a22bf38e7d4557b3f537cf8099547dad已完成private、owner、ready、RAM CPU、GPU五阶段单发验收。仅增加Surface有界日志，既有分配/pin/map/release不变。全部正常PON06返回，无人工；目前Sony健康fastboot、无worker/in-flight。

GPU仍为原已验证GLES9a59820c：750×1624三角形/纹理、Surface双向、caller原VM解除后持有、小纹理动态刷新、CA隔离、160次relink/320draw与名字复用全部独立哈希通过。最后194个scatter对象全部释放/撤销，caller active/outstanding/retained均0；GPU回程38.938秒含健康30秒。原始logSHAff863fac9e36d7dee6ea0ad9b1b84f26b528fd262204ea1e47ce93282787695e。

本轮约180秒观察仍未见时间/日期/壁纸。相机MJPEG可用，取景右侧/底部截断，不能据此核验完整面板或底部导航条。出现69×69/alloc19044与67×67/alloc17956两个caller创建失败：均在withAddressRange阶段返回NULL，对应32KiB覆盖；日志0xe00002bd是包装层占位值，底层实际失败码尚未知。两次acquire=SCA_ALLOCATION_FAILED，没有到prepare/map。此次未复现旧400×400大量失败/AegirPoster退出，不能视为已解决。

正式run_trial准入已补真实RPM三ACK、同内核private/owner/ready/CPU门槛、Mac78实际源及编译内核SHA。28项离线测试全部通过，包含原始RPM/owner/GPU日志故障回放；v725构建/打包/验收入口见SURFACE-DIAGNOSTICS.md。下一追查memoryReferenceCreate的底层错误/范围与权限，另核对真实CA文字/壁纸路径；不凭NULL猜改方向或扩大页面。

以下为保留的v724基线记录：

BH905ND69G健康fastboot，无device/COM/camera worker。内核v724 SHAda6ddc44fc074eb393f135e1395798483fe2a52eb420dfecea883e0833136c9a；GLES库SHA9a59820c7e54b1626cacd49acb70a7d460be3c01c415a17429a2464bb5b11571（14,675,424B）。

GPU核心、Surface双向1,218,000像素、caller创建后修改/GPU写回/原VM解除后持有、动态35,600像素、CA双context384像素、shader/program20,480像素与真正名字复用实机通过。

180秒面板实际182.823s，38.547s完整PON06自动回程含健康30秒。三批照片仍只见充电标志、相机按钮、导航条；时间/日期/壁纸缺。关闭前1129次400×400 Surface VMError，20次AegirPoster退出，符号栈为ObjC/C++terminate与FBSSerialQueue，原因尚未捕获。

原controller数据PASS与视觉INCOMPLETE分别保存，失败不改写。logSHA182d3e4b1e335565320c6e8fa78d6cad3d0d28b7cbd80e5de871a4d0d4224e1b；原包418文件/248,653,297B的结果与SHA清单保持，当前关键原日志和三张面板照片保留，其余一次性副本已进回收站。最后277对象全部释放/撤销，orphan0/active0/root0。

正式仓库当前源已重新编译XNU、GLES与shim，结果见reproducibility/build-results.json和shim-build-results.json。新内核e0eb264c/GLES f22dc3d0/shim77d425b0未上机，不代替此前实机通过的da6ddc44/9a59820c基线。

下一步在整理/复现完成后核对caller acquire/prepare/map与AegirPoster异常，不先猜改色。声音CORE APR/AVS2.8已过，实际播放录音/codec/iOS16前端未过，无线同样未完成；完整目标active。

整理已完成目录分类和实际回收，正式源码推送并在Windows/Mac同步，Patch/Xperia待提交项归零。经过实机验证的内核/库/打包blob已移入正式assets/baselines，SHA核对与上述基线一致；整理报告记录了部分Windows旧回收条目的当前缺失，未将历史移入验证当作当前恢复保证。
