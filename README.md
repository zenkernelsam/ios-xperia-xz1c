# iOS on Xperia XZ1 Compact — lilac / SO-02K

Sony Xperia XZ1 Compact（Snapdragon835／Adreno540）的开源 XNU、Mesa／Freedreno GLES 和 Metal 用户态移植。唯一正式源码与复现入口是本仓库；Patch/Xperia 保存参考 Git 仓库和历史。固定 Mesa24.3.4／FD540 GLES3.1，Vulkan 尚未移植。

CPU、launchd、F12 触控、音量键、完整 GLES9、native Metal21 及 PNG6 已有真机证据。原生720×1280像素／360×640点／scale2和平直状态栏已验；原六行与拟合五行的全部38个应用、Dock顺序及Folder内容已完成独立对照。小组件标签的真实祖先和两处17.5点间距已实机验收。完整safe-area、黑壁纸、动态UI／FPS、睡眠唤醒、光学／背光、声音和无线继续推进。

当前候选和生产基线分开验收。每个新产物按实际编译源、签名、完整payload及自己的CPU／GLES／native／PNG原字节准入；功能probe通过不会自动提升为完整原UI。精确SHA、设备状态、原失败和下一步只在[STATUS](docs/STATUS.md)维护。

| 入口 | 内容 |
| --- | --- |
| [文档索引](docs/INDEX.md) | 现行手册、技术专题与历史原文 |
| [构建](docs/BUILD.md) | 固定OSS源、Mac／WSL、RAM与新产物准入 |
| [操作](docs/OPERATIONS.md) | 单发、串口截图、0x06返回、相机 |
| [显示比例与模型](docs/DISPLAY-GEOMETRY.md) | 原生尺寸、六行／五行对照与标签归属 |
| [目录与恢复](docs/LAYOUT.md) | 受保护输入、归档、回收与复原 |
| [本周整理](docs/ORGANIZATION.md) | 原生回收路径／SHA和文档整合 |

源修补、工具、许可来源及源码锁均进入本仓库。Apple SDK、iPhoneX iOS16.7.16／20H392与Sony固件、镜像、校准和备份由使用者提供，二进制与原帧保持本地。SDK／签名变化可能改变新产物SHA；从零重建gl5首次启动尚未独立验收。

只操作BH905ND69G；fastboot设备命令必须-s，COM≥10且VID1209/PID0001、一个worker、单download／boot、失败不重发、PON只0x06。四回程marker、连续健康30秒及worker退出后才下一轮。gl5／userdata／SD／TA／XFL及救砖输入受保护。废弃内容进入原生回收站，不永久删除或清空。
