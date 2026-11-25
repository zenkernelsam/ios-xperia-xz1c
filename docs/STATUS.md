# 当前状态 — 2026-11-26

反射缓冲选择修补的新Metal已完成自身CPU、完整native21／PNG6及原扫描帧验收；原Aegir第二次星点绘制已进入后端，不再被unused fragment0的EINVAL挡住。原壁纸仍未通过：4212个顶点的星点绘制停在逐图元XR同步提交阶段并触发签到超时。本次修正了批次内同步候选把GL binding当驱动槽位的错误，新r5保持每图元舍入，已完成实际Mesa映射／主机验证和新包离线审计，尚未实机。

Sony BH905ND69G安全停在fastboot，无在途启动。最新native CPU候选的guard没有取得2/2成功，download／boot均0；原失败不重发。之后只读健康连续32.156秒通过，未提升原guard失败；下一次guard前须新物理fastboot会话。当前无iOS运行；本轮已结束实机投递，继续离线收尾。

## 本轮主要结果

| 阶段 | token／原证据 | 原判定与范围 |
| --- | --- | --- |
| 反射Metal RAM CPU | `92cd6155d365cbdd`／[CPU](../reproducibility/buffer-selection-r3-cpu-actual-20261125.json) | PASS／38.812秒完整0x06回程 |
| 反射Metal native21／PNG6 | `1e1bf33f91081ea2`／[完整原字节](../reproducibility/buffer-selection-r3-gpu-actual-20261125.json) | PASS／39.157秒；原unused输入48B、pipeline切换、全部页／guard／退休及150秒扫描3460块 |
| 原壁纸首次新来源 | `6d716e2f8913bb3b`／[原REVISE](../reproducibility/aegir-original-pixels-r4-gpu-refused-20261125.json) | 首次大shader签到超时3次，39.797秒回程；无完整快照／PNG |
| 条件背景预热 | `d3b95d9a5e462ae1`／[原STOP](../reproducibility/buffer-primer-r4-gpu-refused-20261125.json) | 报告53→54文件、27req／26→27bin、prime1；A/index31 SHA缺失且扫描未完成，拒绝／40.282秒回程 |
| 原星点实际进入后端 | `454044d0bb3177b8`／[原REVISE](../reproducibility/aegir-retirement-r5-gpu-refused-20261126.json) | 后端call2 buffers2、count4212／ordered1404；停submit，签到超时3次，39.281秒回程 |
| XR barrier r1 CPU／GLES9 | `abc5af4ed1ac156c`／`36a74003f5e97fff`／[CPU](../reproducibility/ordered-barrier-r1-cpu-actual-20261126.json)、[GLES9](../reproducibility/ordered-barrier-r1-gles9-actual-20261126.json) | PASS／38.563及39.156秒；GLES9含完整720×1280扫描 |
| XR barrier r1 native CPU | `ef161dc3ab9ce723`／[guard拒绝](../reproducibility/ordered-barrier-r1-native-cpu-guard-refusal-20261126.json) | STOP_ERROR／单guard失败，未下载／未boot／不重发 |
| timestamp barrier r2 | [离线候选](../reproducibility/ordered-image-barrier-timestamp-r2-offline-20261126.json)／[主机工具](../reproducibility/ordered-image-barrier-canonical-host-20261126.json) | 21原指令字、monotonic timestamp／WAIT_REG_MEM／UCHE、ASan／UBSan及ARM64构建；尚未实机 |

反射Metal仍`19a08083833851d5a13016473ec3c3db98629d6ce6fc928b70947af114abee49`，source-map `41bdf3e49258c8cf6d57a69beb99d15dec0e1404aec2509725c386941e0f25ee`；它与GLES `6efabfc0…`的精确新[native21](../reproducibility/buffer-selection-r3-native21-proof-20261125.json)／[PNG6](../reproducibility/buffer-selection-r3-png6-proof-20261125.json)只准入该组合。功能通过不提升原wallpaper／FPS／睡眠／光学。

## 当前候选与下一轮

r1 GLES `d1f5075c…`只完成CPU／GLES9，未完成native。r2增加每program独立单调timestamp，r3另明确拒绝异步GLThread，在原context blit scratch offset64等待CACHE_FLUSH_TS写回，再UCHE invalidate；unsupported contract继续原MemoryBarrier。每图元仍单独draw／舍入，没有CPU填预期像素，内核／allocator／SCM／PON／RPC／VM保持。

历史r4 GLES SHA `76f4c32c70aec93954795ed70f48fbc22ef7fd5dd1bc3b0c85eed357b6165e43`，Metal仍`19a08083…`。GLES RAM blob `1408d3012e9449662bcaa66b35bd0cabe0951d86d39c5dfcde8f76dab1b76c30`；native RAM blob `e88507f2743fc2f96479f3890e9cc31812fe7b4b413f36c27d743fa3bf854a1f`／manifest `6b3e5c922530389e815942d2013607c1eb5064525f3861e5a6cb5ced36214735`。[原host与包审计](../reproducibility/ordered-image-barrier-initialized-r4-offline-20261126.json)绑定synchronous GL、第一barrier26指令字／随后21字、GPU初始化scratch0→timestamp1→2、fallback／guard及完整源。四包保持未投递。本次实际Mesa映射复验指出它的sparse mask检查对两槽真实布局会走fallback，旧host把si[0]/si[8]当真实布局的结果不证明优化实际命中；r4不再作为下一轮候选。

当前r5 GLES `dc7113a10542c4b8de2036def8373a4574ab2092d17efd1a39e1c224f1bec308`；Metal仍为已验`19a08083…`，准确source-map保持。GLES RAM blob `31b8ab8196b7bfa84987253b91f8bb7c4b8a3855b37e1eee000dd0f4488cf074`／manifest `ad6365ab02e69674f54e35b7f70895afb63b39b72a848b1bceed028d2f7a94e9`；native RAM blob `75b8f08201a972f9c67ca1a63110131e05286116bfbf47bb1a8fb19548b354f0`／manifest `ae8acd03c0d7e2ef8a0bec81c01a133c73a3e09228f5beb58b534802a8419e08`。[r5完整离线报告](../reproducibility/ordered-image-barrier-dense-r5-offline-20261126.json)绑定barrier revision4及linked GL program的ImageUnits→dense driver slot映射，拒绝未知／重复binding、写SSBO／atomic或其它stage image。直接编译原锁定st_bind_fs_images／st_convert_image验证255个mask和两种声明顺序；同一dense输入复现旧header不发指令。CP初始化、timestamp、每图元舍入和final fence保持，内核／Mesa archive未改。

四包在build/ordered-image-barrier-dense-r5-20261126：CPU `549746d4b51a7668`、GLES9 `b5eea38cb400afa4`、native CPU `a0c42c1d43ab5734`、native21 `2fb820a1d5198d03`，完整payload已复算且均未投递。CPU和native CPU另通过run_trial离线准入、Mac85实际源及准确用户态source审计；GPU还须自己的CPU／GLES／native原字节证明，旧proof不准入。

新物理fastboot会话后依次自身CPU→GLES9→native CPU→完整native21／PNG6，再原Aegir／PNG／scanout。新的GLES不能复用旧native／PNG证明，MRT独立检查所有cached barrier与重叠图元原页。旧guard、缺块和签到超时不重发；新原wallpaper包要有实际功能或观察差别。

主机调度入口wait_trial_slot只等外部工具退出且空闲30秒，再单个controller；可用--wait-before-guard120在任何guard mutation前重复只读进程预检，guard／boot不重复。原Windows路径空格、进程竞态、MemoryError和Mac SSH拒绝均保留；同一native／PNG原run由完整verifier一次派生两份准确证明，未缓存旧验证。原记录见trial-slot／native-png报告与OPERATIONS。

[此前原星点规模主机复验](../reproducibility/ordered-image-barrier-star-load-host-20261126.json)连续生成1404次barrier／29489字／117956 B，逐段核对11次模拟批次切换与9次ring扩容。它验证原CP emitter但使用人为sparse槽位；本次r5在实际dense映射下重跑通过。模拟ring／批次仍不验证实际Mesa调度、A540缓存可见性、XR像素或壁纸耗时。

## 当前输入与准入

| 输入 | 精确身份 | 实际范围 |
| --- | --- | --- |
| v731 只读 WLED 内核 | `15fd8eee111bc880b9b54aeee284f9dc91cd2b7dbbbe302dc944b00d8d2d3e69`，20681752 B／85实际源 | 自身 private／owner／ready；首个完整扫描帧之后只读 WLED |
| v730 基础内核 | `f258dee9c6d142b90a573f796eeec5e92b39dd65feed6a21b2542d90fb66a88d`，20665112 B／83实际源 | 历史完整几何／原UI与诊断基线；底层冻结 |
| Mesa／Freedreno | 24.3.4／`769e51468b49b2a42f0a0eaf71cf9eed5ff4e5de`／FD540 GLES3.1 | 免费开源 GLES 后端；Vulkan 尚未移植 |
| 新 GLES 缓存库 | `6efabfc0627a8818a8c4c45665459cd33d8ee814f2e50a3f3cdc4813bb254325`，14801120 B | 自身 CPU／GLES9；与新 RAM 同靴完整 native21／PNG6 |
| 已验Metal／source-map | `19a08083833851d5a13016473ec3c3db98629d6ce6fc928b70947af114abee49`／`41bdf3e49258c8cf6d57a69beb99d15dec0e1404aec2509725c386941e0f25ee` | 反射buffer组合已完整native21／PNG6；新barrier组合待验 |
| 新 cache build／scope | `16722b126d14e64954fc349d20626f9e639cc726ed1c24856a889b2249efd236`／`9f43d3e226e901f609eeba4b9f348e2050ed7e1bd790ef8f7b422dc31eec892a` | 该基线靴24对／48文件、missing0；后续真实请求扩充清单，原记录保持 |
| R21 geocompat revision4 | `ee7a3207f26af5e9106cbd98c74392153b11a2d345386964db131110a2a6153c`，106544 B | 原生几何、完整应用模型、真实祖先和17.5点标签间距 |
| 历史原壁纸观察 RAM | blob `e98ed2cd62595b2b414ef2d04ce1cfa93af34041a99fd6b38691f004314eb83b`／manifest `55eee7cb15985a1e27c939ae4601ceb104d9d43ae760edb7c6191f450899ec98` | 自身 CPU 通过；原 GPU 观察 STOP，失败包不重发 |

旧 GLES `0766d5c7…` 的生产／原UI和缓存记录仍保留。新库的功能probe成功不自动提升为完整原UI／壁纸、FPS、光学或睡眠基线；不同 RAM／库仍须自己的准入，旧proof不能准入新库。

## 最近完成的实机阶段

| 阶段 | token | 独立结果 | 完整返回含健康30秒 |
| --- | --- | --- | ---: |
| 新 GLES RAM CPU | `77f81204d559eb64` | PASS，完整库读入 | 37.844秒 |
| 新 GLES9 | `ce5c76be554da1aa` | PASS，150秒扫描3417块、像素和资源退休 | 39.156秒 |
| 新 native RAM CPU | `8078d5413d8da5c4` | PASS | 39.078秒 |
| 条件预热 RAM CPU | `1fdd166cf693cf15` | PASS | 39.375秒 |
| 新 native21／PNG6 | `2d1794b225c1a6c1` | PASS，150秒扫描3422块、缓存missing0 | 39.235秒 |
| 原壁纸 RAM CPU | `48d8ab64336f5abf` | PASS | 40.094秒 |
| 原 Aegir 状态观察 | `a4ef7f6bb4537a5e` | STOP／OBSERVATION_INCOMPLETE | 39.438秒 |

[新 GLES 原字节](../reproducibility/poster-cache-budget-gles9-actual-20261124.json)复验完整9组。纯 GLES RAM 只含 GLES／libglapi／geocompat，不需 exception observer。后续独立只读健康观察另通过31.968秒。

[新 native21／PNG6 原字节](../reproducibility/poster-cache-budget-native21-prime-actual-20261124.json)绑定 raw `abd6cd25…`／2917448 B、COM17 单流、完整payload、四段回程、健康检查及worker退出。全部作者页、guard、fence、资源退休、PNG像素／ICC／metadata和720×1280 BGR888／stride2160扫描帧通过。缓存实际24对／48文件、pending0／orphans0／missing0，条件预热实际skip。新的 [native21](../reproducibility/poster-cache-budget-native21-proof-20261124.json) 与 [PNG6](../reproducibility/poster-cache-budget-png6-proof-20261124.json) 只用于精确新库组合。

[缓存末槽修复](../reproducibility/poster-cache-budget-offline-20261124.json)按实际发布占用计数，macOS32对／64名字和WSL含staging的16对／64名字边界均经ASan／UBSan通过；已有文件不覆盖／不删。超过预算仍拒绝。跨进程并发发布尚未独立验证。

## 原壁纸卡点

原bounds服务与磁盘PNG复用已实机观察，原SnapshotManager的限定来源目录让pipeline重新执行，旧文件保持；早期720×1280加载PNG全黑的原证据保留。反射输入修补之后，星点count4212／1404图元进入后端；固定Mesa fd_memory_barrier会fd_context_flush，原call2停submit与批次创建栈支持继续优化该用户态提交开销。尚不能证明完整Earth／post／快照或颜色已恢复。

SnapshotReadbackObserve只读取本次原getBytes已经返回的XR数据；OriginalCGInputObserve在PNG前取原CGImage provider bytes，记录logical FNV、code范围和三个原word；OriginalPNGExport无损zlib导出返回字节。OriginalQueueRetirement另核对Completed队列的原fence和operations。所有观察均有host准确ABI／边界／原decoder，新RAM仍须自己的CPU；缺块失败不补造。

原无pipeline STOP、旧原队列EINVAL、R4／R5签到超时、primer缺SHA／scanout、host路径／内存／SSH和guard失败均保持日期／token／raw／结果，详细由[原壁纸专题](METAL-REQUEST-OBSERVATION.md)与归档索引，不再从旧“下一步”恢复执行。

## 显示及其它外设

[R21原生几何](DISPLAY-GEOMETRY.md)同靴ADT／MG／UIScreen／F12／scanout为720×1280像素、360×640点、scale2。original六行和fitted五行的38个应用、Dock顺序及Folder内容完整对照；真实祖先、两处17.5点间距已验，平直状态栏移除rdar占位。完整safe-area、真人全屏触控、真实应用启动与光学继续验收；合成上滑不代替真人。

v731多靴在首帧后读到WLED模块启用、双灯串／调制器mask3、fault0、duty1989／1989；未连接硬件写入，不证明亮度或睡眠恢复。背光service及非零调光事务已离线编译／故障测试，详情见[Lilac外设](LILAC-PERIPHERALS.md)。

ADSP stage3只核验固件和AP端准备，没有启动DSP。ASM PCM／映射／APR／ADM路由与COPP层已完成离线模拟和ARM64编译；[本次主机复验](../reproducibility/cache-budget-peripheral-host-validation-20261125.json)保持实际源码。实际传输、AFE、Tasha／Tavil选用和声音未验。Wi-Fi／蓝牙也没有扫描、连接或传输证据。shared-event selectors36／37／38／45仍零输出，presentedTime仍0，未实现项保持明确。

[统一PCM会话](../reproducibility/lilac-pcm-session-offline-20261126.json)把已锁定Sony AFE／ADM／ASM组件串到同一个epoch：开流／格式／映射→AFE配置／启动→COPP／路由→Run，全部准确回复后才允许PCM写入。关流先排空四个在途缓冲，再分别等待EOS普通回复与RENDERED_EOS，最后Close／Unmap／关闭COPP／AFE Stop；WRITE_DONE不冒充声音已播放完。WSL和Mac同源七组ASan／UBSan、两种EOS到达顺序及全部13步命令错误通过，iOS ARM64对象已编译但未链接内核。实际DSP／codec／APR传输和物理声音仍未接通，固件token回显与schema／channel map仍需实机确认。

## 复现、整理和操作

正式源码、构建与工具只在本仓库；[BUILD](BUILD.md)给固定Mesa、AIR重建／168个LLVM导入、RAM／原snapshot／AFE复现，[OPERATIONS](OPERATIONS.md)给准入与返回，[LAYOUT](LAYOUT.md)给本地SDK／gl5／救砖输入与恢复。全新gl5首次启动仍未独立验收。各失败包raw／完整payload和只读source-map保留。

本轮[Mac349项／631文件／1070475061 B](../reproducibility/render-buffer-close-out-macos-native-trash-20261125.json)进原生Trash，967保护文件一致，28份HFS从保留blob复算；[Windows7项／10文件／4780776 B](../reproducibility/render-buffer-close-out-windows-native-recycle-20261125.json)进原生回收站。两套[Ubuntu垃圾桶为空](../reproducibility/render-buffer-close-out-wsl-trash-audit-20261125.json)，Windows／Mac未清空。10份整合前原文与SHA在[本轮归档](archive/20261125/render-buffer-selection/original-document-manifest.json)；更早整理与可恢复Git stash由[ORGANIZATION](ORGANIZATION.md)索引。

本轮[Mac311项／521文件／1019811972 B](../reproducibility/ordered-XR-macos-native-trash-20261126.json)进原生Trash，731保护文件保持；[Windows9项／21文件／585102 B](../reproducibility/ordered-XR-windows-native-recycle-20261126.json)进回收站。[Ubuntu Trash为空](../reproducibility/ordered-XR-wsl-trash-audit-20261126.json)，未清空Windows／Mac。新候选、原失败／日志／proof和signed库保留，源代码与复现工具只进入正式Git。

PCM与原星点规模主机收尾另回收[Mac51项／97文件／3430430 B](../reproducibility/pcm-session-and-XR-load-macos-native-trash-20261126.json)、[Windows31项／33文件／45242472 B](../reproducibility/pcm-session-and-XR-load-windows-native-recycle-20261126.json)，WSL共享build测试程序进入Windows回收站。目标和保护文件SHA一致，原构建／test日志、manifest、源码备份及r4候选保留；[两套Ubuntu Trash为空](../reproducibility/pcm-session-and-XR-load-wsl-trash-audit-20261126.json)。未永久删除或清空垃圾桶，五份入口原文先归档，由ORGANIZATION／INDEX索引。

PCM／星点主机补充已在正式主提交`d42c453…`推送origin/main，[Mac同源同步](../reproducibility/pcm-session-and-XR-load-formal-git-sync-20261126.json)24份变更逐字节匹配，source验证通过；Mac预同步源保存在未drop的stash`6b2990c0…`，pause及冻结内核保持。本轮没有新的guard／download／boot，新的r5须新物理fastboot会话后走原准入顺序。

r5收尾[Mac54项／78文件／204301653 B](../reproducibility/ordered-image-barrier-dense-macos-trash-20261126.json)进原生Trash，236保护文件SHA保持；六份重复HFS从保留blob逐字节复算后回收。[Windows一个TEMP包／两文件／328704 B](../reproducibility/ordered-image-barrier-dense-windows-recycle-20261126.json)进原生回收站，外层误读旧退出码后只核验原journal、未重复回收。[两套Ubuntu Trash为空](../reproducibility/ordered-image-barrier-dense-wsl-trash-audit-20261126.json)，Windows／Mac未清空；五份整合前入口原文与SHA已archive。

正式补充主提交`4dc552dba56d156496fea0d0b1919a0de2635893`已推送origin/main，[r4 Mac同步](../reproducibility/ordered-XR-initialized-git-sync-20261126.json)18份变更逐字节一致、source验证通过，原Mac修改以未删除stash`caea8028…`保留。此前[94份变更同步](../reproducibility/ordered-XR-formal-git-sync-20261126.json)原记录保持。Windows／WSL使用同一正式目录，原sampler fixture字节保持。反射Metal组合已完整验收；当前HEAD还有待验的XR barrier候选，不冒充完整生产UI。更早同步与stash由ORGANIZATION索引。

只操作BH905ND69G，fastboot设备命令显式`-s BH905ND69G`；COM≥10且VID1209／PID0001、一个controller／worker；PON只0x06，单download／boot、失败不重发。ACK／DRAIN／RESET／FINAL_DRAIN、健康30秒及旧worker退出后才下一轮。无本token LIVE的早期E1F4仍可能需物理恢复。SweetGPU SHA `aa2c3679…`及内核GPU底层／allocator／SCM／PON／RPC／VM冻结；gl5／userdata／SD／TA／XFL、校准、备份和救砖输入不flash／erase。Mac旧autobuild保持pause；废弃文件只进原生回收站。
