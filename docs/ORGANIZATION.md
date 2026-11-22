# 本周整理与文档整合 — 2026-11-22

本轮在R19／R20三靴完整0x06回程、连续健康30秒及worker退出后执行。Mac813项／815文件／552748850B和Windows4项／10文件／27009609B实际进入各自原生回收站，所有用户内容SHA、原路径／目标路径及保护指纹通过。未永久删除或清空回收站，未修改其它手机项目。

| Mac本周累计／最新快照 | 数量 |
| --- | ---: |
| 原生Trash历史移入项 | 2843 |
| 文件 | 36359 |
| 逻辑字节 | 14289537269（约13.31GiB） |
| 最新正式build | 3973140KiB（约3.79GiB） |
| 最新系统可用空间 | 93958021120B（约87.51GiB） |

移入垃圾桶仍占磁盘；历史累计是各次原生回收记录的数量，不表示释放量，也不保证用户之后未自行回收的旧条目仍存在。本轮813项的垃圾桶目标已逐文件复核。系统可用空间为本轮收尾快照，旧94%等数字只在原历史报告中保留。

## 本轮与保护范围

Mac回收两份可由精确SGLZ0001 blob恢复的32MiB HFS容器、重复传输tar、host程序／dSYM、35个一次性Mach-O包装及已结束构建的.o中间文件。固定deps／Mesa归档／dxb实际83源、v730内核、所有signed库／observer、RAM blob／manifest、原失败包／raw／proof、gl4／gl5／IPSW、准确A2S、校准／救砖／XFL／备份及pause保持。

104个Mac保护文件已在回收前后SHA核对，包括12.2GB gl5原sparse输入；HFS没有挂载，保留原bin／txt／地址范围和Folder静态报告。Windows只回收重复staging和三份重复CLI，358个保护文件核验一致，完整三靴原run目录、像素及REVISE报告保留。

[本轮Mac原路径／Trash路径／SHA](../reproducibility/native-full-model-r19-r20-macos-native-trash-20261122.json)和[Windows原路径／Recycle Bin路径／SHA](../reproducibility/native-full-model-r19-r20-windows-native-recycle-20261122.json)用于查找或复原；恢复办法见[LAYOUT](LAYOUT.md)。

research实查仅89个参考Git仓库、非仓库0；Ubuntu的root／cisco和Ubuntu-22.04的root垃圾桶均无内容。WSL如有废弃内容须先移至Windows TEMP再进Windows原生回收站；不会永久删除或直接清空Ubuntu垃圾桶。[整合审计](../reproducibility/native-full-model-r19-r20-organization-20261122.json)记录原文、SHA、链接与本地盘点。

## 原生回收历史索引

| 阶段 | Mac项／文件／逻辑字节 | 原路径、Trash路径及SHA |
| --- | --- | --- |
| 本周主体 | 1713／34883／11852238934 | [记录](../reproducibility/weekly-macos-native-trash-20261121-final-host.json) |
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
| native-layout r16 | 5／7／75067648 | [记录](../reproducibility/native-layout-r16-macos-native-trash-20261122.json) |
| native-density r17 | 51／53／75314075 | [记录](../reproducibility/native-grid-density-r17-macos-native-trash-20261122.json) |
| native-model r18 | 101／103／75571221 | [记录](../reproducibility/native-layout-model-r18-macos-native-trash-20261122.json) |
| full-model r19／r20 | 813／815／552748850 | [记录](../reproducibility/native-full-model-r19-r20-macos-native-trash-20261122.json) |

主体四项Finder元数据差异与partial journal按原记录保存；不算用户内容SHA一致或整批成功。其它Windows／WSL历史回收记录仍在各阶段reproducibility。每轮新回收的最终目标核验与旧历史记录分开保留。

## 文档整合与正式源码

现行状态只由[STATUS](STATUS.md)维护；[BUILD](BUILD.md)放现行复现，[OPERATIONS](OPERATIONS.md)放单发／回程／截图／相机，[LAYOUT](LAYOUT.md)放目录与恢复，[INDEX](INDEX.md)串联技术专题及历史。R19／R20完整应用身份的通过与标签归属REVISE分别保留，没有提升旧GPU失败。

| 整合前原文 | 数量 | 原字节与SHA |
| --- | ---: | --- |
| 本周最初手册 | 47 | [manifest](archive/20261121/weekly-integration/original-document-manifest.json) |
| r15入口整合前 | 50 | [manifest](archive/20261122/native-r15-close-out/original-document-manifest.json) |
| 本轮整合前／6a9efe8 | 50 | [manifest](archive/20261122/native-full-model-r19-close-out/original-document-manifest.json) |

本轮审核47个docs页面及README／AGENTS／Metal入口，过时的2026-11-12／13／15整理报告原文归档，旧链接保留历史指引。旧handover／handout、编号实验、in-flight与失败只作历史；技术证据保留，入口不再复制多轮工作日志。原文快照不改字节，恢复到原路径后使用其相对链接。

所有新源修补、工具、许可来源与源码锁仅在正式ios-xperia-xz1c提交并同步Mac；Patch/Xperia只作参考和历史。Apple输入、signed二进制、原帧和救砖资料保持本地，Git保存免费开源方案、构建和独立验证方法。
