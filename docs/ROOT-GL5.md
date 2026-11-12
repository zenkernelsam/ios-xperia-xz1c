# gl5 用户镜像配方与输入

设备当前 gl5 不需要重刷。这里记录复现当前用户镜像的必要变更；镜像本身不进入 Git。

输入为 iPhone X iOS16.7.16/20H392 的 OS、SystemOS、AppOS；使用者从 Apple 提供的 IPSW 中按 BuildManifest 获取。旧 gl4/5 本地基线的 sparse 镜像分别约12.2GB，原 gl5 MD5 e9bf1547b6a7e9a33b51c2749c643bc2。具体原文件保留在 assets/root-build/，勿用其它机型镜像替代。

基础 HFS+ 构建脚本 tools/mkiosroot.sh、tools/mkiosroot-mac.sh 和 tools/sweet-root-tweaks.py 保留原源码。历史脚本含旧临时清理动作，当前直接运行前必须按回收站约束检查，不自动发送设备。它们合并 cryptexes、保留所有者/权限/链接、创建 keybag/log 存储与容器。

gl5 相对 gl4 的决定性差异是：不预先设置 com.apple.migration.plist 来伪称迁移已完成，让 datamigrator 和 migrationpluginwrapper 在首次启动正常建立数据库。恢复迁移标记和启动完成标记必须根据实机结果判断；不能把 API 读回成功当数据库已经完整。

用户态 GPU 从 boot payload 的只读 RAM HFS 镜像加载：backboardd 注入 /dev/sglram/m（sweetgl.dylib），SpringBoard 注入 /dev/sglram/g（几何辅助库）并设 GEO_FIX=1。工具 tools/build/package_glram.py 从正式编译输出生成32MiB只读HFS镜像和SGLZ0001压缩blob，m/g为硬链接，manifest包含库/辅助库/hash/FNV；已完成主机生成及只读校验。

原用户root中的 CPU/GLES选择、launchd配置、iOS16 ABI适配属于本地用户镜像配方。当前尚未从 Apple 原镜像重建并实机验证一个全新的 gl5，因此“源码可重建”不能扩大为任意新root首次启动已通过。本仓库记录这个边界；在清理后继续补齐用户镜像全流程验证。

禁止把“复现验证”当作授权重刷本机 userdata：任何 root 重建输出使用新文件，现有 gl5、原厂固件、校准和备份保持；发设备仍需完整包审计与对应新kernel/新库基线门槛。
