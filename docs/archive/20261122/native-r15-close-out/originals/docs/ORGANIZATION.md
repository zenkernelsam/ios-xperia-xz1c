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

## 本轮追加收尾

主体整理后，capture revision1恢复真实单份启动流，tiled r2新GLES自身CPU／GLES9／reference CPU通过；完整native21复制路径-45拒绝，40秒回程后停止。原四靴及截图／失败报告见[本轮阶段](../reproducibility/depth-tiled-r2-stage-20261121.json)，未提升基线，原failed package／raw保持。

本轮Mac另15项／33文件／134708787B通过NSWorkspace进原生垃圾桶，包括两组可由保留blob精确重建的raw／dmg、已完成host程序／dSYM及未编译成功的旧CRLF-fixture观察器目录。所有用户内容SHA复核，当前signed库、blob／manifest、内核及SweetGPU指纹保持。实际记录见[Mac追加回收](../reproducibility/depth-tiled-r2-macos-native-trash-20261121.json)。主体和追加合计1728项／34916文件／11986947721B（11.16GiB），垃圾桶未清空；最新Mac94%使用／59GiB可用、build3.7GiB，其它项目未清理。

Windows本轮传输及mock fixture3项／37文件／174001078B另进原生回收站，目标内容SHA通过，见[Windows追加回收](../reproducibility/serial-capture-r1-windows-recycle-20261121.json)。实际设备raw／失败、作者页面和少量必要扫描图受保护，临时传输已回收。现行文档已更新到r2真实边界；47份本周原始归档manifest保持逐字节一致。

## r6／r7追加与当前汇总

本轮四靴完整结束后，Mac又有21项／29文件／285078590B经NSWorkspace进原生Trash：r6／r7八份可由保留blob逐字节恢复的HFS raw／dmg、已完成observer host程序／dSYM、两个在编译前停止的空目录和一次性传输tar。目标内容SHA全部复核；当前signed库、blob／manifest、v730与SweetGPU指纹保持。见[本轮Mac原生记录](../reproducibility/stencil-r7-macos-native-trash-20261121.json)。

前一批r3／r4重复容器及仅离线r5目录另9项／147文件／460725414B已经原生回收，记录现已提交，见[过程Mac回收](../reproducibility/round-progress-macos-native-trash-20261121.json)。主体1713项、旧追加15项、过程9项和本轮21项合计1758项／35092文件／12732751725B（约11.86GiB），保持原事件分别记录。

Windows本轮两个完成传输目录、8个WSL在Windows挂载盘生成的ELF及4份重复CLI输出，共14项／38文件／96769680B进原生Recycle Bin，所有目标SHA复核。原四靴raw／commands／result／source audit／return proof和五个作者失败page保留，见[Windows本轮记录](../reproducibility/stencil-r7-windows-native-recycle-20261121.json)。此前3个stage传输目录98.71MB回收见[过程Windows记录](../reproducibility/round-progress-windows-native-recycle-20261121.json)。

两个Ubuntu按实际passwd读取root及cisco的垃圾桶，均无files／info条目。Windows／Mac垃圾桶没有清空，仍占磁盘；最后Mac91%使用、91GiB可用，正式build4094896KiB（约3.91GiB），同盘其它项目未改。

现行50份文档统一由STATUS给事实、BUILD给复现、OPERATIONS给规则、LAYOUT给恢复、INDEX给专题。修改前10份原文与SHA保存在[本轮归档manifest](archive/20261121/stencil-r7-integration/original-document-manifest.json)；本周47份原归档逐字节保持。过时handover／handout仍归入archive，原失败不重新分类。正文只维护现行入口，技术证据留专题／原报告；当前[GPU四靴](../reproducibility/stencil-sample-r7-stage-20261121.json)仍REVISE，整理没有改变验收结论。

r8结束后另15项／25文件／150861981B入Mac原生Trash，9项／22文件／56556776B入Windows回收站，含4个WSL ELF；目标SHA与保护指纹通过，原四靴及拒绝转储raw保留。见[Mac r8](../reproducibility/stencil-r8-macos-native-trash-20261121.json)／[Windows r8](../reproducibility/stencil-r8-windows-native-recycle-20261121.json)。回收站未清空。

r9轮后15项／25文件／150862932B进入Mac原生Trash，5项／18文件／56212424B进入Windows回收站；目标SHA和保护指纹复核，垃圾桶未清空。原四靴、拒绝和当前少量有效页面保留，见[Mac r9](../reproducibility/stencil-r9-macos-native-trash-20261121.json)／[Windows r9](../reproducibility/stencil-r9-windows-native-recycle-20261121.json)。

## r10收尾与本周累计

r10四靴完整返回后，15项／25文件／150868099B进Mac原生Trash；Windows6项／19文件进原生回收站，含最后一个WSL日志writer ELF，目标SHA／保护指纹全部通过。[Mac r10](../reproducibility/stencil-r10-macos-native-trash-20261121.json)／[Windows r10](../reproducibility/stencil-r10-windows-native-recycle-20261121.json)保存路径和原内容SHA。

主体及全部追加Mac共1803项／35167文件／13185344737B（12.28GiB）；回收站未清空，最新91%使用／92GiB可用、build4281044KiB（4.08GiB），其它项目未改。现行50份文档按STATUS／BUILD／OPERATIONS／LAYOUT／INDEX／专题职责整理；本轮编辑前STATUS／ORGANIZATION原文和SHA另归入[r10归档](archive/20261121/r10-close-out/original-document-manifest.json)。GPU完整附件证明和presentation拒绝分开保留，源修补只在正式仓库。

2026-11-22 lifecycle-r2轮两靴完整结束后：Mac14项／28文件／95169204B及Windows3项／11文件／29514408B全部原生回收、目标SHA复核，保留完整native21／PNG6新包、proof和原raw。没有清空垃圾桶，旧R10失败与基线未删。[Mac记录](../reproducibility/presentation-lifecycle-r2-macos-native-trash-20261122.json)／[Windows记录](../reproducibility/presentation-lifecycle-r2-windows-native-recycle-20261122.json)。

2026-11-22 原mode-r12轮结束后，Mac25项／47文件／76896058B与Windows3项／11文件／29021080B已原生回收；目标SHA验证、失败包／raw／native21 proof和冻结内核保留，未清空。[Mac](../reproducibility/original-mode-r12-macos-native-trash-20261122.json)／[Windows](../reproducibility/original-mode-r12-windows-native-recycle-20261122.json)。

2026-11-22 known-owner r13轮结束，Mac26项／132文件／78473846B（含仅离线失败夹具构建）及Windows3项／11文件／29022694B均原生回收，目标SHA通过、未清空；原STOP、source33／home scanout及完整native21 proof保留。[Mac](../reproducibility/original-owner-r13-macos-native-trash-20261122.json)／[Windows](../reproducibility/original-owner-r13-windows-native-recycle-20261122.json)。
