# iOS on Xperia XZ1 Compact — lilac / SO-02K

Sony Xperia XZ1 Compact（Snapdragon835／Adreno540）的开源XNU、Mesa／GLES和Metal用户态移植。唯一正式源码与操作仓库是ios-xperia-xz1c；Patch/Xperia保存参考Git仓库和本地历史。固定Mesa24.3.4／Freedreno FD540 GLES3.1，尚未移植Vulkan。

已验v730／CA矩阵基线可显示灰色锁屏时间日期，以及上滑后的正常颜色主页。CPU、launchd、F12触控、音量键、完整GLES9、native Metal16与六组PNG已验；壁纸仍黑，动态UI／FPS、睡眠、LCD光学、声音和无线未完成。

最新tiled r2深度／模板候选自身CPU／GLES9／reference CPU通过，串口单流捕获已实机恢复。完整native21仍在GPU布局复制路径返回-45，未准入附件／呈现／PNG；原页面字节失败和日志拒绝均保留。本周Mac整理及47份文档整合已完成。手机健康fastboot、无worker，精确SHA与后续入口见[STATUS](docs/STATUS.md)。

| 入口 | 内容 |
| --- | --- |
| [文档索引](docs/INDEX.md) | 当前手册、能力专题、历史证据 |
| [构建](docs/BUILD.md) | 固定OSS源、Mac／WSL、RAM与新产物准入 |
| [操作](docs/OPERATIONS.md) | 单发、串口截图、0x06返回、相机 |
| [附件验收](docs/ATTACHMENT-HARDWARE.md) | 真机错误、原页面、tiled候选边界 |
| [显示比例](docs/DISPLAY-GEOMETRY.md) | 原生720×1280／360×640计划与离线ADT |
| [目录与恢复](docs/LAYOUT.md) | 输入／基线／archive、回收与复原 |
| [本周整理](docs/ORGANIZATION.md) | Mac垃圾桶实际记录与文档归档 |

所有新源码、工具和锁只在本仓库提交；SDK和构建时间可改变产物SHA，新库必须自己的实际CPU／GLES／完整native／PNG证明。iPhoneX iOS16.7.16／20H392与Sony固件、镜像、备份由使用者提供，不进入Git；从零重建gl5首次启动尚未独立验收。

只操作BH905ND69G，fastboot必须-s；COM≥10且VID1209/PID0001、单worker、单download／boot、失败不重发、PON仅0x06。四回程marker、健康30秒和worker退出后才下一轮。gl5／userdata／SD／TA／XFL及救砖输入受保护；废弃文件进原生回收站，不永久删除或清空。
