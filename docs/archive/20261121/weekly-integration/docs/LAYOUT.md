# 目录与恢复

唯一正式源码ios-xperia-xz1c。gpu/sweetgl用户态，xnu/lilac-current.patch内核，tools/build与tools/maintenance维护工具，reproducibility源码锁和核验，docs当前手册，docs/archive旧handover/notes。

research仅参考Git仓库：Patch86个、Mac3个。deps为固定构建源码，build为输出；assets本地镜像/固件/备份，archive历史实验/证据/产物/旧工具，organization完整迁移与回收记录。

Windows废弃内容原生回收站；WSL旧输出/链接恢复包进Windows回收站；Mac原生Trash。Ubuntu垃圾桶检查为空；代理没有清空Windows/Mac回收站。备份不去重。移入时的回收记录和最终可恢复性分开记录：部分Windows旧条目在最终复核时已经缺失，原因未证明。

第二轮实际回收Patch 45,821项/40.56GB、Windows TEMP 45,585项/34.76GB、Mac1,596项/4.25GB、WSL1,176项/2.07GB及旧xnu-builds/former-artifacts。回收数据仍占磁盘空间，须用户自行决定何时清空回收站。当前恢复、复现和卡点分析的基线/少量日志照片保持；每轮原结果、SHA和计数汇总在reproducibility/evidence-cleanup-index.json，不继续保留成千上万帧的一次性副本。

一次性Windows文件在%TEMP%，禁止C:/Users/Cisco He/Desktop。flash/fw与unlock救砖/XFL资料受保护。Mac ios-assets归入assets/legacy-ios-assets。历史绝对路径只保留证据，不再是当前入口。

当前经过实机验证的内核和RAM库保留正式仓库assets/baselines：kernel-v724-tested.macho、tested-gles-9a59820c/ramfs/sweetgl.dylib和ramfs-render/glram.zblob。Windows另保留v724-private/ready/tested-panel镜像。当前原厂/恢复镜像在Patch/Xperia/assets/recovery，v338打包容器和v648 CPU回退在assets/baselines；archive无.img产物。

Mac当前符号缓存为assets/symbols/ios16-20H392-root.a2s。旧符号化脚本中的archive/experiments/lilac-v655/root.a2s不再是当前位置。各轮结果/SHA清单不重写。
