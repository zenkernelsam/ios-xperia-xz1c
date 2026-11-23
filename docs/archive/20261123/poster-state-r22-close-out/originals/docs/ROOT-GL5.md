# gl5 用户镜像配方与输入

本页保存受保护输入的配方与恢复方法；当前设备、准确基线及操作状态以[STATUS](STATUS.md)为准，单发规则见[OPERATIONS](OPERATIONS.md)。

设备当前 gl5 不需要重刷。这里记录复现当前用户镜像的必要变更；镜像本身不进入 Git。

输入为 iPhone X iOS16.7.16/20H392 的 OS、SystemOS、AppOS；使用者从 Apple 提供的 IPSW 中按 BuildManifest 获取。旧 gl4/5 本地基线的 sparse 镜像分别约12.2GB，完整gl5 SHA256见下文。具体原文件保留在 assets/root-build/，勿用其它机型镜像替代。

2026-11-15磁盘复核：保留的gl5原文件SHA256为
`e9bf1547b6a7e9a33b51c2749c643bc2eff0134c31cc3a63f9435e4cea1f9b29`。
按Android sparse规则解码（DONT_CARE在这个已核文件中恢复为零），得到17179869184字节，
SHA256 `4c90d65aa82c72aec8930feb2d9914b07750d879e6003dcae88884e74f63e631`，
与原Mac ios16-root-lilac.img全SHA一致。因此该raw副本可原生回收，仍保留gl5和gl4原文件。
需要raw时，从仓库根目录用以下工具恢复到新路径；它核对原sparse、所有chunk、decoded
指纹，以及实际输出的完整读回，不挂载或发送设备，也不覆盖已有文件：

```sh
python3.12 tools/maintenance/restore_gl5_raw.py --sparse /path/to/ios16-root-lilac-gl5.simg --verify-only
python3.12 tools/maintenance/restore_gl5_raw.py --sparse /path/to/ios16-root-lilac-gl5.simg --output /path/to/new/ios16-root-lilac.img
```

IPSW解包缓存的83件原始成员也已与留存IPSW逐件SHA一致；需要时可从IPSW重解。
原unzip.log独立保留。这里证明已存gl5的逐字解码，不扩大为从Apple原镜像重建新gl5
已完成实机验证。详细回收记录在工作台maintenance/cleanup-20261115-lilac-followup。

基础 HFS+ 构建脚本 tools/mkiosroot.sh、tools/mkiosroot-mac.sh 和 tools/sweet-root-tweaks.py 保留原源码。历史脚本含旧临时清理动作，当前直接运行前必须按回收站约束检查，不自动发送设备。它们合并 cryptexes、保留所有者/权限/链接、创建 keybag/log 存储与容器。

gl5 相对 gl4 的决定性差异是：不预先设置 com.apple.migration.plist 来伪称迁移已完成，让 datamigrator 和 migrationpluginwrapper 在首次启动正常建立数据库。恢复迁移标记和启动完成标记必须根据实机结果判断；不能把 API 读回成功当数据库已经完整。

用户态 GPU 从 boot payload 的只读 RAM HFS 镜像加载：backboardd 注入 /dev/sglram/m（sweetgl.dylib），SpringBoard 注入 /dev/sglram/g（几何辅助库）并设 GEO_FIX=1。工具 tools/build/package_glram.py 从正式编译输出生成32MiB只读HFS镜像和SGLZ0001压缩blob，m/g为硬链接，manifest包含库/辅助库/hash/FNV；已完成主机生成及只读校验。

原用户root中的 CPU/GLES选择、launchd配置、iOS16 ABI适配属于本地用户镜像配方。当前尚未从 Apple 原镜像重建并实机验证一个全新的 gl5，因此“源码可重建”不能扩大为任意新root首次启动已通过。本仓库记录这个边界；在清理后继续补齐用户镜像全流程验证。

禁止把“复现验证”当作授权重刷本机 userdata：任何 root 重建输出使用新文件，现有 gl5、原厂固件、校准和备份保持；发设备仍需完整包审计与对应新kernel/新库基线门槛。
