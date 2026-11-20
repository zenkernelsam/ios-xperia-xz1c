# 整理与文档整合 — 2026-11-21

按用户要求，在设备轮完整回程／健康30秒／worker退出后清理Mac Sony项目的一次性和重复产物。所有回收使用macOS NSWorkspace原生Trash；没有永久删除或清空垃圾桶，没有改其它手机项目。详细原路径／垃圾桶路径／SHA见[实际回收报告](../reproducibility/weekly-macos-native-trash-20261121-final-host.json)。

| 本周Mac实际 | 数量 |
| --- | ---: |
| 原生Trash项 | 1,713 |
| 包含文件 | 34,883 |
| 逻辑字节 | 11,852,238,934（11.04 GiB） |
| 原分配字节 | 11,933,851,648（11.11 GiB） |
| 正式build目录 | 约14 GiB → 3.6 GiB |

回收含64组HFS builder `.dmg/.raw`、另7个可重建raw、四个准确重复765MB符号表、两份已结束OSS复现checkout、旧signed库展开副本、过时host／dSYM、一次性传输tar及Finder元数据；另689项已结束x86_64主机程序和342MB旧内核构建日志保留SHA后进垃圾桶。RAM都先由保留SGLZ0001 blob逐字节解码与raw／container对比；旧signed库按manifest raw_offset从blob恢复再比SHA。目录回收也记录全部原文件／SHA。原失败日志、精确blob／manifest、当轮signed库、原shader研究和所需少量证据保留。

原生Trash目标存在、内容逐SHA复核。四项`.DS_Store`在Finder操作前后发生变化，单列为系统元数据，未混入用户内容验证。第三批计划包含两个重复路径，577项移入后工具因同源已不存在停止；旧partial journal原样保留，只续剩余221个唯一路径。正式工具现在在第一次移动前拒绝重复／父子嵌套清单，避免重复发生。没有把partial失败当整批完成。

gl4、gl5、IPSW、本机固件／SD／TA／XFL／校准／救砖及备份保持受保护；当前v730／SweetGPU、固定deps／dxb、CA完整基线、最新tiled／原失败包blob及A2S符号表重新核验。Mac `~/xnu-autobuild.pause`保持，旧常驻构建器未恢复工作。

**垃圾桶仍占磁盘**，整理后Mac系统仍约94%使用／62GiB可用；未将移入量称为释放量。用户之后自行决定何时清空。此次Mac范围仅~/Desktop/ios-xperia-xz1c；同盘其它UniXNU_elden项目较大，只读盘点后未改。Windows一次性staging4项／16文件／163,611,132B已走[Windows原生Recycle Bin](../reproducibility/weekly-windows-native-recycle-20261121.json)，WSL没有新增内部持久产物，两个Ubuntu垃圾桶另复核。

## 文档整合

本周AGENTS、README、44份docs和Metal README共47份原文先按原字节和SHA保存到[归档manifest](archive/20261121/weekly-integration/original-document-manifest.json)。过时handover／handout历史仍在docs/archive；旧raw／proof／token／失败不覆盖。

当前入口收敛：[STATUS](STATUS.md)只写实际当前设备／基线／候选，[BUILD](BUILD.md)只放现行复现，[OPERATIONS](OPERATIONS.md)只放单发／回程／截图／相机，[LAYOUT](LAYOUT.md)放目录／恢复，[INDEX](INDEX.md)联专题。移除入口重复的几十轮工作日志；技术专题保留实际代码契约、精确证据和复现，顶部标明历史scope，不能盖过当前STATUS。

旧整理报告20261112／13／15保留原事件和数量，作为历史；当前汇总在本页。原参考仓库索引和MSM8998调查保持来源，当前Patch/research89个Git参考仓库。源码／锁／工具和整合文档只提交正式ios-xperia-xz1c，不在Patch维护新修补。

本轮GPU结果独立保留：GLES21de与c617两库各自CPU／GLES9通过；原附件reference GPU两次REVISE；tiled reference CPU原controller PASS但启动日志独立核验拒绝，native21未投递。整理没有改变这些结果或证明完整UI完成。显示比例720×1280／360×640计划与离线ADT放[DISPLAY-GEOMETRY](DISPLAY-GEOMETRY.md)。
