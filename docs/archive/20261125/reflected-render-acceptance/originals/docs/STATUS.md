# 当前状态 — 2026-11-25

原 Aegir 黑壁纸已定位到命令队列的第二次绘制：原编码器保留上一管线的片元缓冲0，后端却要求全部绑定与新管线反射集合完全相等。新 Metal 用户态已按固定开源参考修正选择，主机 ASan／UBSan 通过；新库尚未完成自身CPU／native21／PNG6，不提升为生产基线。最后实际GPU靴原REVISE与四段0x06回程保持，Sony BH905ND69G停在fastboot，无在途启动。

两次新库CPU投递都在ABL guard前遇到其它设备的fastboot进程，download／boot均0；原失败不重发。独立Sony健康观察通过31.359秒。当前准备候选是带真实管线切换与未使用缓冲原字节核验的r3，后续先重新查进程冲突，再自身CPU，完整回程后native21／PNG6，最后原壁纸。

## 本轮关键结果与候选

| 阶段 | 原结果与证据 | 边界 |
| --- | --- | --- |
| 原快照服务 | [22完整调用／4完整回调](../reproducibility/poster-snapshot-gate-gpu-actual-20261125.json) | 原bounds与view创建已返回，未改mode／paused |
| 原磁盘PNG复用 | [8次非空load／8次setImage](../reproducibility/poster-snapshot-cache-gpu-actual-20261125.json) | 整组缺无关BEGIN仍拒绝；图片路径独立验证 |
| 新来源原快照 | [31完整pipeline／64完整postdraw](../reproducibility/aegir-warm-snapshot-r3-logwrite-gpu-actual-20261125.json)，`01fa0a69a8ce868f` | 原观察PASS／回程40.312秒；壁纸仍黑，不能当UI通过 |
| 原加载PNG | [无损压缩导出](../reproducibility/aegir-loaded-png-compressed-r2-gpu-actual-20261125.json)，`15480a7a8a23a9ed` | PNG27208B／720×1280／921600个像素全黑；原STOP保持，回程39.219秒 |
| 原GPU队列 | [5次编码／仅第一项后端执行](../reproducibility/aegir-render-queue-r4-gpu-refused-20261125.json)，`5e64e7240b822797` | 原status5／error9／`AIR render failed result=-22`；REVISE含签到超时，回程38.110秒 |
| 新Metal r1／r2 CPU | [r1](../reproducibility/buffer-selection-r1-cpu-host-refusal-20261125.json)、[r2](../reproducibility/buffer-selection-r2-cpu-host-refusal-20261125.json) | guard前冲突，未下载／未boot，不重发 |
| 新Metal r3 | [主机与包审计](../reproducibility/buffer-selection-r3-switch-offline-20261125.json) | 待自身CPU／完整native21／PNG6／原UI |
| AFE SLIMBUS协议 | [双平台主机与ARM64对象](../reproducibility/lilac-afe-slimbus-playback-reproducible-20261125.json) | 只协议／状态，无DSP／codec／声音准入 |

新Metal SHA `19a08083833851d5a13016473ec3c3db98629d6ce6fc928b70947af114abee49`；source-map `41bdf3e49258c8cf6d57a69beb99d15dec0e1404aec2509725c386941e0f25ee`。GLES仍`6efabfc0…`、内核仍`15fd8eee…`。r3 RAM blob `91ec02d744ce8e645a138005da39a015bfc351f4159a9f74454ca169710e7eaf`／manifest `ffd427114c90a76cc16ad496fe56ad28b5024ff58ab3bea0d3e454fae67ec8d6`；CPU token `92cd6155d365cbdd`、GPU token `1e1bf33f91081ea2`，尚未实机。新shader-cache来源`b1aa0e0a…`，不预热空请求、不删除旧cache。

## 当前输入与准入

| 输入 | 精确身份 | 实际范围 |
| --- | --- | --- |
| v731 只读 WLED 内核 | `15fd8eee111bc880b9b54aeee284f9dc91cd2b7dbbbe302dc944b00d8d2d3e69`，20681752 B／85实际源 | 自身 private／owner／ready；首个完整扫描帧之后只读 WLED |
| v730 基础内核 | `f258dee9c6d142b90a573f796eeec5e92b39dd65feed6a21b2542d90fb66a88d`，20665112 B／83实际源 | 历史完整几何／原UI与诊断基线；底层冻结 |
| Mesa／Freedreno | 24.3.4／`769e51468b49b2a42f0a0eaf71cf9eed5ff4e5de`／FD540 GLES3.1 | 免费开源 GLES 后端；Vulkan 尚未移植 |
| 新 GLES 缓存库 | `6efabfc0627a8818a8c4c45665459cd33d8ee814f2e50a3f3cdc4813bb254325`，14801120 B | 自身 CPU／GLES9；与新 RAM 同靴完整 native21／PNG6 |
| Metal／source-map | `8feea4593116ffe9187204f5dd4cf4dd48d3027816d4763ecef20da200f86a27`／`db1605ef8dae8f8ed0a29999dcb2cf7537457aba7ad428677c21e0e914771b78` | 库字节及 source-map 保持；新 GLES 组合已重新完整验收 |
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

## 原壁纸卡点与下一步

原SnapshotManager复用旧PNG；限定原Aegir UUID／caller的来源目录让原snapshot／pipeline重新执行，旧文件保持。随后背景只预热真实缺bin请求，原r2 primer的PNG6缺块拒绝及第一次namespace签到超时保留。r3四份snapshot非空／三份PNG返回的原PASS只说明API观察完成；后续queue终态确认原第二draw仍返回EINVAL。原bounds服务和场景并未被强制解除paused。

黑PNG与新队列结果是本轮主要停点。下一阶段先用r3新RAM完成自身CPU，完整回程后验全部native21／PNG6，特别核对真实pipeline切换、unused fragment0／slot7与48B原输入；合格后为新Metal重新生成source-map绑定的native／PNG proof，再运行原Aegir队列和一次scanout。只有原五项draw均实际完成、PNG和扫描真实像素出现，才提升原壁纸；FPS、LCD光学与睡眠另验。

早期无pipeline调用的状态／scanout／单次上滑原STOP、r24／r2的缺块、r26签到超时和旧STOP_RESET_UNCONFIRMED均保持；原token、raw、proof与完整回程在[原壁纸专题](METAL-REQUEST-OBSERVATION.md)和本轮归档索引。新包不得借用旧证明或用新token复发失败包。

## 显示及其它外设

[R21原生几何](DISPLAY-GEOMETRY.md)同靴ADT／MG／UIScreen／F12／scanout为720×1280像素、360×640点、scale2。original六行和fitted五行的38个应用、Dock顺序及Folder内容完整对照；真实祖先、两处17.5点间距已验，平直状态栏移除rdar占位。完整safe-area、真人全屏触控、真实应用启动与光学继续验收；合成上滑不代替真人。

v731多靴在首帧后读到WLED模块启用、双灯串／调制器mask3、fault0、duty1989／1989；未连接硬件写入，不证明亮度或睡眠恢复。背光service及非零调光事务已离线编译／故障测试，详情见[Lilac外设](LILAC-PERIPHERALS.md)。

ADSP stage3只核验固件和AP端准备，没有启动DSP。ASM PCM／映射／APR／ADM路由与COPP层已完成离线模拟和ARM64编译；[本次主机复验](../reproducibility/cache-budget-peripheral-host-validation-20261125.json)保持实际源码。实际传输、AFE、Tasha／Tavil选用和声音未验。Wi-Fi／蓝牙也没有扫描、连接或传输证据。shared-event selectors36／37／38／45仍零输出，presentedTime仍0，未实现项保持明确。

## 复现、整理和操作

正式源码、构建与工具只在本仓库；[BUILD](BUILD.md)给固定Mesa、AIR重建／168个LLVM导入、RAM／原snapshot／AFE复现，[OPERATIONS](OPERATIONS.md)给准入与返回，[LAYOUT](LAYOUT.md)给本地SDK／gl5／救砖输入与恢复。全新gl5首次启动仍未独立验收。各失败包raw／完整payload和只读source-map保留。

本轮[Mac349项／631文件／1070475061 B](../reproducibility/render-buffer-close-out-macos-native-trash-20261125.json)进原生Trash，967保护文件一致，28份HFS从保留blob复算；[Windows7项／10文件／4780776 B](../reproducibility/render-buffer-close-out-windows-native-recycle-20261125.json)进原生回收站。两套[Ubuntu垃圾桶为空](../reproducibility/render-buffer-close-out-wsl-trash-audit-20261125.json)，Windows／Mac未清空。10份整合前原文与SHA在[本轮归档](archive/20261125/render-buffer-selection/original-document-manifest.json)；更早整理与可恢复Git stash由[ORGANIZATION](ORGANIZATION.md)索引。

正式主提交`7e18b58a4e0a7f675682ff10efb7986f82210ecc`已推送origin/main，[Mac同步](../reproducibility/render-buffer-formal-git-sync-20261125.json)112份变更逐字节一致、source验证通过，原Mac修改以未删除stash`a408edc6…`保留。Windows／WSL使用同一正式目录；原sampler fixture字节保持。当前HEAD包含待实机验收的新Metal，不冒充旧生产基线。

只操作BH905ND69G，fastboot设备命令显式`-s BH905ND69G`；COM≥10且VID1209／PID0001、一个controller／worker；PON只0x06，单download／boot、失败不重发。ACK／DRAIN／RESET／FINAL_DRAIN、健康30秒及旧worker退出后才下一轮。无本token LIVE的早期E1F4仍可能需物理恢复。SweetGPU SHA `aa2c3679…`及内核GPU底层／allocator／SCM／PON／RPC／VM冻结；gl5／userdata／SD／TA／XFL、校准、备份和救砖输入不flash／erase。Mac旧autobuild保持pause；废弃文件只进原生回收站。
