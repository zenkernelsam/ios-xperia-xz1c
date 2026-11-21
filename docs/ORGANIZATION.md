# 本周整理与文档整合 — 2026-11-22

本周及追加Sony项目内容已通过macOS NSWorkspace原生Trash或Windows原生Recycle Bin回收；每轮均在完整回程、健康30秒及worker退出后处理。全部用户内容按原路径／目标路径／SHA复核，未永久删除或清空垃圾桶，未修改其它手机项目。

| Mac累计 | 实际数量 |
| --- | ---: |
| 原生Trash项 | 1873 |
| 文件 | 35381 |
| 逻辑字节 | 13510835475（约12.58GiB） |
| 最新正式build | 4362504KiB（约4.16GiB） |
| 最新系统可用空间 | 98031190016B（约91.3GiB） |

垃圾桶仍占磁盘；这些数字表示从工作目录移入垃圾桶的内容量，不表示释放量。旧报告中的94%／62GiB等是当时快照，当前值来自r15收尾实测。Mac范围仅`~/Desktop/ios-xperia-xz1c`，`~/xnu-autobuild.pause`保持。

## 原生回收记录

| 阶段 | Mac项／文件／逻辑字节 | Mac原路径、Trash路径及SHA |
| --- | --- | --- |
| 本周主体 | 1713／34883／11852238934 | [主体记录](../reproducibility/weekly-macos-native-trash-20261121-final-host.json) |
| depth-tiled r2 | 15／33／134708787 | [记录](../reproducibility/depth-tiled-r2-macos-native-trash-20261121.json) |
| 中途重复／离线产物 | 9／147／460725414 | [记录](../reproducibility/round-progress-macos-native-trash-20261121.json) |
| stencil r7 | 21／29／285078590 | [记录](../reproducibility/stencil-r7-macos-native-trash-20261121.json) |
| stencil r8 | 15／25／150861981 | [记录](../reproducibility/stencil-r8-macos-native-trash-20261121.json) |
| stencil r9 | 15／25／150862932 | [记录](../reproducibility/stencil-r9-macos-native-trash-20261121.json) |
| stencil r10 | 15／25／150868099 | [记录](../reproducibility/stencil-r10-macos-native-trash-20261121.json) |
| presentation r11 | 14／28／95169204 | [记录](../reproducibility/presentation-lifecycle-r2-macos-native-trash-20261122.json) |
| original-mode r12 | 25／47／76896058 | [记录](../reproducibility/original-mode-r12-macos-native-trash-20261122.json) |
| owner r13 | 26／132／78473846 | [记录](../reproducibility/original-owner-r13-macos-native-trash-20261122.json) |
| native-flat r15 | 5／7／74951630 | [记录](../reproducibility/native2x-flat-r15-macos-native-trash-20261122.json) |

回收包括可由保留SGLZ0001 blob逐字节恢复的HFS raw／dmg、重复765MB符号表、已结束OSS checkout、旧库展开副本、host／dSYM、一次性tar和日志。原失败raw／包／proof及当前需要的少量页面保留。主体四项Finder元数据变化及旧partial journal原样保留，未当用户内容SHA或整批成功；正式Mac工具在首个移动前拒绝重复／父子嵌套路径。

r15五项为两份精确可重建HFS容器、传输tar、host程序和dSYM；signed helper／GLES、blob／manifest、准确A2S、v730 kernel、SweetGPU及pause的指纹全部保持。Windows本轮传输staging及重复CLI共3项／9文件／26918845B进入[原生回收站](../reproducibility/native2x-flat-r15-windows-native-recycle-20261122.json)，目标全部SHA核验。r14重复CLI记录见[Windows r14](../reproducibility/native2x-r14-windows-native-recycle-20261122.json)；其它Windows／WSL实际记录保留在相应阶段reproducibility。

gl4／gl5、IPSW、Sony固件、SD／TA／XFL／校准／救砖与备份、当前内核、固定deps／dxb及准确符号输入受保护。WSL废弃内容走Windows回收站；Ubuntu的root／cisco及Ubuntu-22.04的root实际垃圾桶均为空，见[本轮整合审计](../reproducibility/native-r15-organization-20261122.json)。Windows／Mac垃圾桶未清空，Ubuntu若有废弃内容先转Windows回收站后恢复空目录。

## 现行文档与历史

[STATUS](STATUS.md)只写实际设备、准确输入和下一步；[BUILD](BUILD.md)给现行复现；[OPERATIONS](OPERATIONS.md)给单发／回程／截图／相机；[LAYOUT](LAYOUT.md)给目录和恢复；[INDEX](INDEX.md)联技术专题。旧版本、token、失败和原byte留在专题／原报告，不再在入口复制每轮工作日志。

本周47份原文及SHA在[最初manifest](archive/20261121/weekly-integration/original-document-manifest.json)，r7与r10阶段编辑前快照保留原目录。r15整合前50份现行文档原文及SHA新增[完整快照](archive/20261122/native-r15-close-out/original-document-manifest.json)。过时handover／handout和in-flight记录只作archive历史；此次移除入口中互相矛盾的“未准入／正在测试”描述，保留每个旧失败的准确判定。

当前原生几何／平直状态栏与r11完整native21／PNG6有各自实机报告；小组件重叠、黑壁纸、光学、FPS、睡眠、声音和无线未扩大为完成。回收不会重新分类旧GPU失败。

源码／工具／锁只在正式Windows与Mac的ios-xperia-xz1c维护并同步；Patch/Xperia是参考与历史入口，research只参考Git仓库，不存新实验。用户输入与二进制本地保留，Git里保存免费OSS方案、精确构建和验证方法；新构建产物须独立准入。
