# 本周整理与文档整合 — 2026-11-26

PCM保留运行时／实时clock和cache交接收尾：[Mac54项／98文件／7524860 B](../reproducibility/pcm-runtime-macos-trash-20261126.json)进入原生Trash，[Windows25项／26文件／37697590 B](../reproducibility/pcm-runtime-windows-recycle-20261126.json)进入原生回收站，包含WSL共享build已结束主机程序、两套ARM64 object／dSYM、TEMP传输tar和原peer重构的一次性源码副本。全部目标SHA一致，98个Mac和65个Windows保护文件保持；原test／build日志、准确manifest、src备份及GPU r5／冻结kernel保留。

[两套Ubuntu全部用户Trash为空](../reproducibility/pcm-runtime-wsl-trash-audit-20261126.json)，Windows／Mac未清空。整合前[五份入口原文](archive/20261126/pcm-runtime-cache/original-document-manifest.json)原字节与SHA已保存；现行音频复现更新为十一组／四对象，不把host缓存回调成功当物理声音。新代码只在正式仓库维护，设备操作0。

提交diff检查首次拒绝共享peer头的EOF空行，修正后按最终字节重做WSL／Mac十一组和四个对象验证，ARM64 runtime代码SHA未变。[前一报告](../reproducibility/lilac-pcm-runtime-cache-offline-20261126.json)原文保留，[最终准确源报告](../reproducibility/lilac-pcm-runtime-cache-final-20261126.json)记录两者身份。补回收[Mac27项／50文件／3722214 B](../reproducibility/pcm-runtime-eof-macos-trash-20261126.json)及[Windows12项／13文件／17818937 B](../reproducibility/pcm-runtime-eof-windows-recycle-20261126.json)，包含复验host产物和一次性传输副本；原日志／manifest／旧源码备份保留，native目标SHA一致，垃圾桶未清空。

主提交`fd89415286bbcd2599e49d7d59fbeb758c4e9fb7`已推送origin/main，[Mac源同步凭据](../reproducibility/pcm-runtime-formal-git-sync-20261126.json)28份变更与commit逐字节一致，实际runtime／clock／cache源与正式source验证通过。预同步原文先保存，再进未drop的可恢复stash`220b8c174f04ecc5d9db8e45444e265983b7d28e`；Mac旧builder保持pause，GPU r5／冻结SweetGPU及Windows原sampler保持。

具体SMEM FIFO收尾：[Mac24项／44文件／2801648 B](../reproducibility/pcm-smem-fifo-macos-trash-20261126.json)进入原生Trash，[Windows23项／23文件／35077856 B](../reproducibility/pcm-smem-fifo-windows-recycle-20261126.json)进入原生回收站，包含WSL在正式共享build的已结束主机程序、三个ARM64 object／dSYM和TEMP传输tar。回收目标逐字节SHA一致，45个Mac和61个Windows保护文件保持；准确源码、原编译／test日志、五份展开头文件、manifest、源码备份和GPU r5候选保留。

[两套Ubuntu全部用户Trash为空](../reproducibility/pcm-smem-fifo-wsl-trash-audit-20261126.json)，Windows／Mac未清空。整合前[五份入口原文](archive/20261126/pcm-smem-fifo/original-document-manifest.json)原字节／SHA保存；STATUS从20354 B整理为约10KiB，只保留设备、已验准确组合、r5候选和外设待验门槛，旧逐轮回程／回收／同步日志由topic／原报告和本页索引。GPU候选与失败证据未改，设备操作0。

本轮主提交`68c00352548785fd76ad892336c5136d3b01bfd0`已推送origin/main，[Mac准确同步](../reproducibility/pcm-smem-fifo-formal-git-sync-20261126.json)22份变更与commit逐字节一致，实际FIFO／PCM编译源和正式source验证通过。pre-sync原字节先保存，再进未drop的stash`db9a7ff895dce3e87ac7fd1e39317713846c34d6`；Mac旧builder保持pause，GPU r5候选、冻结SweetGPU及Windows原sampler字节保持。

PCM／GLINK适配收尾：[Mac38项／70文件／3412804 B](../reproducibility/pcm-glink-transport-macos-trash-20261126.json)进入原生Trash，[Windows26项／27文件／38847808 B](../reproducibility/pcm-glink-transport-windows-recycle-20261126.json)进入原生回收站，含WSL共享build已结束主机程序、两套ARM64 object／dSYM及TEMP传输tar。目标逐字节SHA核验，69个Mac和73个Windows保护文件保持；原失败输出、混合行尾头文件、构建／test日志、manifest、源码备份、r5 GPU包与冻结内核保留。

[Ubuntu／Ubuntu-22.04全部用户Trash为空](../reproducibility/pcm-glink-transport-wsl-trash-audit-20261126.json)，未清空Windows／Mac垃圾桶。整合前[五份现行入口](archive/20261126/pcm-glink-transport/original-document-manifest.json)已按原字节／SHA归档，音频技术与现行八组复现入口归入LILAC-PERIPHERALS／BUILD；源码只在正式仓库维护，设备操作0。

本轮主提交`67ffb766bb14831a4759bfaf8d38d6ee46fdb999`已推送origin/main，[正式Mac同步](../reproducibility/pcm-glink-transport-formal-git-sync-20261126.json)21份变更与commit逐字节一致，实际PCM／GLINK编译源和source验证通过。pre-sync源原字节先保存，再进未drop的可恢复stash`a22a59a207754ef6c538b1d5db85f84b233be3da`；Mac旧builder继续pause，冻结GPU和Windows原sampler保持。

本次Mesa dense image修补收尾：[Mac54项／78文件／204301653 B](../reproducibility/ordered-image-barrier-dense-macos-trash-20261126.json)进入原生Trash，236保护文件SHA一致；六份重复HFS先从保留blob逐字节复算，主机程序／dSYM／frontend object和TEMP传输tar已结束后回收。新r5 signed库、RAM blob／manifest、源码备份、原test／build日志及固定内核保留。

[Windows一个TEMP传输包／两文件／328704 B](../reproducibility/ordered-image-barrier-dense-windows-recycle-20261126.json)进入原生回收站，外层检查误用了此前native命令退出码，随后只读验证原journal和目标全部SHA，未再次回收。[Ubuntu和Ubuntu-22.04全部用户Trash为空](../reproducibility/ordered-image-barrier-dense-wsl-trash-audit-20261126.json)；Windows／Mac未清空。整合前[五份入口原文](archive/20261126/dense-image-barrier/original-document-manifest.json)原字节保持。本轮没有guard／download／boot，原r4／失败与历史证据保持，新r5只在正式仓库维护。

本轮主提交`887348ba6f6cf80d8ad23a5b0a9adb6620d17a7b`已推送origin/main，[Mac同步](../reproducibility/ordered-image-barrier-dense-git-sync-20261126.json)28份变更与commit逐字节一致，实编译新GLES源码和正式source验证通过。pre-sync修改原字节先保存，再进可恢复stash`58e25e8e4922d3d43d27ada1947e6c46eb6684df`，未drop；冻结SweetGPU与pause保持，Windows原sampler未覆盖／提交。

本次PCM统一会话与原星点规模主机验证收尾：[Mac51项／97文件／3430430 B](../reproducibility/pcm-session-and-XR-load-macos-native-trash-20261126.json)移入原生Trash，[Windows31项／33文件／45242472 B](../reproducibility/pcm-session-and-XR-load-windows-native-recycle-20261126.json)移入原生回收站，包含WSL在正式共享build生成的已结束测试程序和TEMP源码传输包。回收目标逐文件SHA一致，Mac114个和Windows9个保护文件保持；候选签名库／准入包、冻结内核、原manifest／编译／test日志和源码备份保留。移动仍占磁盘，未清空垃圾桶。

两套[Ubuntu Trash实查为空](../reproducibility/pcm-session-and-XR-load-wsl-trash-audit-20261126.json)，root和所有/home用户均核对。Windows回收凭据首次核验遇TEMP短路径别名，按准确父目录解析后逐字节确认；首次默认WSL用户无法读root Trash，随后只读root审计通过，均没有重新执行回收或永久删除。整合前[五份入口原文](archive/20261126/pcm-session-and-XR-load/original-document-manifest.json)原字节／SHA已保存。音频协议、验证工具、GPU命令流测试及报告只维护在正式仓库；本轮设备操作0。

本轮主提交`d42c4539716a556b9b5b1dfd4067a3940ff1b2f4`已推送origin/main，[Mac同步凭据](../reproducibility/pcm-session-and-XR-load-formal-git-sync-20261126.json)确认24份变化与commit逐字节一致、正式source验证通过。Mac实编译源先按原字节保存，再进可恢复stash`6b2990c0521f51a574bdb566cf13eeab43abc36a`，未drop；旧pause／冻结SweetGPU保持。Windows原sampler fixture未提交或覆盖，WSL使用同一正式源。

反射Metal新库完成native21／PNG6、原壁纸失败与XR同步候选这一轮收尾追加[Mac311项／521文件／1019811972 B](../reproducibility/ordered-XR-macos-native-trash-20261126.json)，731保护文件SHA保持，30份HFS从保留blob完整重建。signed库、候选／失败包、raw／proof、固定Mesa归档、AIR对象／恢复配方和pause保持；回收host程序／dSYM／object及重复传输材料。

[Windows9项／21文件／585102 B](../reproducibility/ordered-XR-windows-native-recycle-20261126.json)包括TEMP传输包和本轮host fixtures，逐SHA核对原生回收位置；两套[Ubuntu Trash为空](../reproducibility/ordered-XR-wsl-trash-audit-20261126.json)，Windows／Mac未清空，回收仍占磁盘。新增host fixture从TEMP分配，完成后由原生回收流程处理。整合前5份[reflected-render原文](archive/20261125/reflected-render-acceptance/original-document-manifest.json)与8份[ordered-XR原文](archive/20261126/ordered-XR-barrier/original-document-manifest.json)字节与SHA保持。

主提交`876f73e`已进入正式origin/main，[Mac同步记录](../reproducibility/ordered-XR-formal-git-sync-20261126.json)核对94份变更与commit原字节及source-lock；旧Mac修改进可恢复stash`1c50dc5b9b64269bb299439be5f2fdf1e8a188b4`，未drop。Mac旧builder继续pause，Windows／WSL使用正式源，不再在Patch目录维护修补；新候选binary仍须自己的实机准入。

最终scratch初始化修补收尾另回收[Mac43项／59文件／202825088 B](../reproducibility/ordered-XR-initialized-macos-native-trash-20261126.json)，207个保护文件一致；新r4签名库、blob／manifest、原host构建日志和回收路径保留，垃圾桶未清空。首次26字／后续21字的CP初始化／timestamp同步host检查和四份未投递准入包由STATUS索引。

该补充主提交`4dc552d`已推送，[r4同步凭据](../reproducibility/ordered-XR-initialized-git-sync-20261126.json)核对18份Mac源／文档／report原字节，source-lock验证通过。pre-sync修改保留在未drop的stash`caea8028862c3829135a2a9f2f311f617bc611d8`；当前Mac和Windows／WSL正式源码一致，旧autobuild继续pause。

原快照和GPU队列停点这一轮收尾追加[Mac原生Trash349项／631文件／1070475061 B](../reproducibility/render-buffer-close-out-macos-native-trash-20261125.json)，967个保护文件SHA逐一保持。28份HFS重复容器从保留blob逐字节复算后回收，另有已结束host程序／dSYM／object和传输tar；新Metal签名库、当前kernel、原失败包／raw／proof及AIR恢复材料保留。[Windows7项／10文件／4780776 B](../reproducibility/render-buffer-close-out-windows-native-recycle-20261125.json)包括WSL在共享Windows build的主机程序和TEMP传输副本，全部SHA核验原生回收位置。

两套[Ubuntu Trash实查为空](../reproducibility/render-buffer-close-out-wsl-trash-audit-20261125.json)，Windows／Mac垃圾桶未清空；回收仍占磁盘。10份整合前入口原字节和SHA先[归档](archive/20261125/render-buffer-selection/original-document-manifest.json)，原快照服务、黑PNG和命令队列错误整合入METAL-REQUEST-OBSERVATION，AFE协议入LILAC-PERIPHERALS，当前候选和进程冲突只在STATUS维护。

源码、112份文档／证据与工具已在正式主提交`7e18b58`推送；[Mac同步凭据](../reproducibility/render-buffer-formal-git-sync-20261125.json)核对相同commit和逐文件原字节，pre-sync全部本地修改进可恢复stash`a408edc63f471884b037943d0364599a9d236230`，未drop。Mac旧builder仍pause；原sampler fixture字节及本地输入／binary保持。原生回收helper和本次plan／journal留在build，便于复核回收路径。

收尾补回收[Mac AFE对象／host四项、六文件、127067 B](../reproducibility/render-buffer-afe-macos-native-trash-20261125.json)及[Windows七个原布局scratch目录、六文件、144803 B](../reproducibility/render-buffer-scratch-windows-native-recycle-20261125.json)。可复現源码、manifest、ASan／ARM64日志和每次原失败仍可恢复，未清空任何原生回收站。本轮合计Mac353项／637文件／1070602128 B；Windows14项／16文件／4925579 B。

本轮在R22新RAM CPU与原GPU均完成0x06回程、连续健康30秒及worker退出后执行。Mac178项／4097文件／179699518B及Windows4项／16文件／29061727B实际进入各自原生回收站；全部原内容SHA和1282个Mac／447个Windows保护文件一致。没有永久删除或清空回收站，R22原REVISE与少量必要证据保持。

2026-11-24，R26 回程后追加 [Mac 原生回收凭据](../reproducibility/poster-cache-r26-macos-native-trash-20261124.json)：28 项、52 文件、135635716 B。R25／R26 的四份重复 RAM 容器均从保留的 blob、manifest 和库范围逐字节复算；另移入已结束的 R26 主机测试可执行文件与 dSYM。候选内核、签名库、构建源、原结果及暂停标志保留。目标逐文件复核，未清空垃圾桶。

2026-11-24 缓存库验收收尾追加 [54 项／238 文件／287602294 B 的 Mac 原生 Trash 记录](../reproducibility/cache-budget-macos-native-trash-20261124.json)。六份 HFS 重复容器、已结束的 frontend object 和 host fixture／dSYM 已回收；233 个保护文件复算一致。首次调用旧 helper 因计划 schema 不同在任何移动前拒绝，随后从正式 Swift 源码编译当前 helper 并逐项目标验证。签名库、原失败包、raw／proof、manifest、固定 Mesa 输入和 AIR 重建材料均保留；新 [9 份入口原文快照](archive/20261124/cache-budget-close-out/original-document-manifest.json) 保存整合前字节。两套 Ubuntu Trash 实查无内容，Windows 和 Mac Trash 未清空；回收的内容仍占磁盘。

2026-11-25 原壁纸两种观察包收尾追加 [Mac46项／88文件／136572265 B](../reproducibility/poster-observation-macos-native-trash-20261125.json)，111保护文件SHA一致；四份HFS可从保留blob重建，host程序／dSYM已结束后回收。另有 [9个WSL生成的主机程序／13062696 B](../reproducibility/cache-budget-host-windows-recycle-20261125.json) 从共享Windows build进入Windows原生回收站。原失败编译日志、实际源、固定APR头、signed库、manifest、原结果／raw／proof保留。所有垃圾桶未清空。主提交7c02219已推送正式origin/main，[Mac完整135文件同步](../reproducibility/cache-budget-formal-git-sync-20261125.json)通过，原本地修改保存在未删除的stash。

## 本轮原生回收

| 范围 | 项／文件／逻辑字节 | 原路径、原生目标及SHA |
| --- | --- | --- |
| Mac HFS、传输、host、Mesa object、一次性cache fixture与Finder元数据 | 177／4096／179624318 | [主记录](../reproducibility/poster-state-r22-macos-native-trash-20261123.json) |
| Mac已退出的一次性回收可执行文件 | 1／1／75200 | [helper记录](../reproducibility/poster-state-r22-macos-helper-native-trash-20261123.json) |
| Windows重复staging与成功CPU／health CLI | 3／14／29060777 | [Recycle Bin记录](../reproducibility/poster-state-r22-windows-native-recycle-20261123.json) |
| Windows两份Git行尾转换的一次性副本 | 1／2／950 | [原件恢复与回收](../reproducibility/poster-state-r22-archive-checkout-native-recycle-20261123.json) |

Mac只回收已结束构建的object-only目录、host程序／dSYM、可从保留blob逐字节重建的两份32MiB HFS及重复tar。46个host fixture内的测试符号链接随父目录移入Trash，逐一核对原link target、未跟随；Swift原生URL使用/tmp别名的记录与/private/tmp计划按实际规范路径核对，两种原路径都保留。

固定deps、Mesa归档／生成header、dxb实际83源、v730内核、signed库／observer、RAM blob／manifest、所有原失败包／raw／proof、gl4／gl5／IPSW、A2S、校准／救砖／XFL／备份及xnu-autobuild.pause保持。Windows原GPU失败CLI及所有本轮原结果、字节和像素也保持。回收后CPU／GPU的完整payload、signed先验和Mac实际83源离线复验均通过：[原离线记录](../reproducibility/poster-state-r22-post-recycle-offline-20261123.json)。设备操作0，原GPU失败判定保持。

## 磁盘快照与历史数量

| Mac本周历史移入累计／本轮快照 | 数量 |
| --- | ---: |
| 历史原生Trash移入项 | 3031 |
| 历史文件 | 40470 |
| 历史逻辑字节 | 14544832393（约13.55GiB） |
| 本轮正式build | 3905268KiB（约3.72GiB） |
| 本轮系统可用空间 | 99122552832B（约92.32GiB） |
| 本轮主回收后实际Trash | 184936KiB；随后追加helper75200B |

移动到垃圾桶仍占磁盘，不能把历史累计当释放量。上述可用空间是主回收核验时快照；系统其它活动会改变数字。本次开始时Trash只有约1MiB，历史各次移入内容是否后来由用户回收不作推断。当前新目标均已逐文件核验，Windows／Mac垃圾桶本轮都没有清空。正式Sony build约3.72GiB，系统整体占用约90%；其它手机和应用项目不属于本轮移动清单。

## Mac旧入口与参考目录

Mac旧~/Desktop/Patch/Xperia的18项非Git research内容已移出；38份历史文档／源文件按原路径与SHA归档到正式docs/archive/20261123/macos-patch-entry，包含旧AGENTS、handover、unattend及原实验脚本。旧脚本只作历史，不恢复旧in-flight。全部38份历史文本已进入正式Git，目录中被research ignore规则过滤的21份已按准确文件清单纳入。

Magisk APK、magisk-patched-lilac.img和hfsplus.ko三份本地恢复／构建输入移至正式assets/legacy-macos-patch-inputs-20261123并逐SHA验证，保留保护身份。Mac旧AGENTS现在只指向正式README、STATUS、BUILD与OPERATIONS；旧research为空，正式research只剩3个参考Git仓库。[完整原路径与目标](../reproducibility/macos-legacy-patch-integration-20261123.json)／[历史原文manifest](archive/20261123/macos-patch-entry/original-document-manifest.json)。

Windows Patch/Xperia/research实查仍为89个参考Git仓库、非仓库0。Ubuntu及Ubuntu-22.04的root／cisco Trash均空或不存在：[本次只读审计](../reproducibility/poster-state-r22-wsl-trash-audit-20261123.json)。若以后出现废弃WSL内容，先移至Windows TEMP再进Windows原生回收站；不直接永久清空。

## 文档职责与复现

现行设备、准确SHA、候选、原结果和下一步只在[STATUS](STATUS.md)汇总；[BUILD](BUILD.md)放固定源与现行复现，[OPERATIONS](OPERATIONS.md)放单发／回程／串口／相机，[LAYOUT](LAYOUT.md)放目录与恢复，[INDEX](INDEX.md)串技术与历史。本轮原environment与链接耗时整合到[壁纸专题](METAL-REQUEST-OBSERVATION.md)，原REVISE及缺route ID拒绝保持。

| 整合前原文 | 数量 | 原字节与SHA |
| --- | ---: | --- |
| 本周最初手册 | 47 | [20261121](archive/20261121/weekly-integration/original-document-manifest.json) |
| R15入口原文 | 50 | [native-r15](archive/20261122/native-r15-close-out/original-document-manifest.json) |
| R19整合前／6a9efe8 | 50 | [native-full-model-r19](archive/20261122/native-full-model-r19-close-out/original-document-manifest.json) |
| R22本轮手册与入口 | 50 | [本轮主manifest](archive/20261123/poster-state-r22-close-out/original-document-manifest.json) |
| NOTES／XNU／AIR补充入口 | 3 | [补充manifest](archive/20261123/project-entry-supplement/original-document-manifest.json) |
| Mac旧文档与源文件 | 38 | [Mac旧入口](archive/20261123/macos-patch-entry/original-document-manifest.json) |

53份现行入口和技术文档均审核；历史能力页标明原实验边界，过时的XNU远程artifact指令与AIR旧进度已整合到现行入口。原文先按SHA保存，原失败与历史内容没有重写为通过。Git已对所有originals禁用行尾转换；Windows两份.bat检出转换已从与Mac留存原件SHA一致的原Git blob恢复，转换副本进入原生回收站。Apple输入／shader／原帧、signed二进制与救砖资料保持本地；正式Git保留免费开源源码、修补、锁、构建与验证方法。

## 原生回收历史索引

| 阶段 | Mac项／文件／逻辑字节 | 原记录 |
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
| ancestry r21 | 10／14／75595606 | [记录](../reproducibility/native-label-ownership-r21-macos-native-trash-20261122.json) |
| environment r22 | 178／4097／179699518 | 本页主记录与helper记录 |

主体四项Finder元数据差异与partial journal仍按原记录保存，不提升为整批SHA通过。复原方法见LAYOUT；当前新回收目标的复核与历史累计保持各自范围。
