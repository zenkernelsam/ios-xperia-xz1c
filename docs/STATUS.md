# 当前状态 — 2026-11-25

新缓存库已在 v731 完成自身 CPU、GLES9、完整 native Metal21／PNG6 及真实扫描帧验收；原壁纸还未恢复。最近一次原 Aegir 单次上滑后的主页扫描观察为 STOP／OBSERVATION_INCOMPLETE，150秒主页扫描完整，已完成四段 0x06 回程、连续健康30秒和串口退出，Sony BH905ND69G 安全停在 fastboot，无在途启动。

## 当前输入与准入

| 输入 | 精确身份 | 实际范围 |
| --- | --- | --- |
| v731 只读 WLED 内核 | `15fd8eee111bc880b9b54aeee284f9dc91cd2b7dbbbe302dc944b00d8d2d3e69`，20681752 B／85实际源 | 自身 private／owner／ready；首个完整扫描帧之后只读 WLED |
| v730 基础内核 | `f258dee9c6d142b90a573f796eeec5e92b39dd65feed6a21b2542d90fb66a88d`，20665112 B／83实际源 | 历史完整几何／原UI与诊断基线；底层冻结 |
| Mesa／Freedreno | 24.3.4／`769e51468b49b2a42f0a0eaf71cf9eed5ff4e5de`／FD540 GLES3.1 | 免费开源 GLES 后端；Vulkan 尚未移植 |
| 新 GLES 缓存库 | `6efabfc0627a8818a8c4c45665459cd33d8ee814f2e50a3f3cdc4813bb254325`，14801120 B | 自身 CPU／GLES9；与新 RAM 同靴完整 native21／PNG6 |
| Metal／source-map | `8feea4593116ffe9187204f5dd4cf4dd48d3027816d4763ecef20da200f86a27`／`db1605ef8dae8f8ed0a29999dcb2cf7537457aba7ad428677c21e0e914771b78` | 库字节及 source-map 保持；新 GLES 组合已重新完整验收 |
| 新 cache build／scope | `16722b126d14e64954fc349d20626f9e639cc726ed1c24856a889b2249efd236`／`9f43d3e226e901f609eeba4b9f348e2050ed7e1bd790ef8f7b422dc31eec892a` | 新范围实际24对／48文件、missing0，前后记录 SHA 一致 |
| R21 geocompat revision4 | `ee7a3207f26af5e9106cbd98c74392153b11a2d345386964db131110a2a6153c`，106544 B | 原生几何、完整应用模型、真实祖先和17.5点标签间距 |
| 原壁纸观察 RAM | blob `e98ed2cd62595b2b414ef2d04ce1cfa93af34041a99fd6b38691f004314eb83b`／manifest `55eee7cb15985a1e27c939ae4601ceb104d9d43ae760edb7c6191f450899ec98` | 自身 CPU 通过；原 GPU 观察 STOP，失败包不重发 |

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

[本次原观察](../reproducibility/poster-cache-budget-wallpaper-observation-20261125.json)raw `c338c9b3…`／702521 B完整保留。Aegir只出现一次exec，登记证明准确，库bundle／URL非空返回；20条原request、128条原route和99条原environment记录均完整，未观察到Aegir签到超时。唯一缺失目标是原pipeline第1次返回；本LIVE内没有pipeline BEGIN、snapshotImage调用、PNG返回或原绘制API。

原环境返回preview／snapshot均true，backlight／linear_backlight均1.0，unlock均0.0；五次原paused=true，applyMode1沿原deactive分支返回。无调用不能推断整个系统没有渲染，亦不授权强制mode／paused。该轮只有串口，没有扫描或光学帧；原STOP不提升、不重发。随后同靴扫描与较小观察包已完成，见下一段；原STOP均保留。

随后 [原 Aegir 同靴扫描](../reproducibility/poster-cache-budget-wallpaper-scanout-actual-20261125.json) `035142bfce21f792` 再现唯一pipeline目标缺失，原STOP保持。150秒扫描完整3431块，原raw `802745c3…`／1928425 B；9条postdraw、20条request、126条route及99条environment记录均完整，没有原快照／PNG／draw。四段回程、健康30秒及worker退出38.469秒。真实扫描中时间、电池和底条正常，背景仍黑。相同v731编译consumer中sf2x初值0已核验，新的省略绑定保持本kernel SHA和同consumer／parser；不同kernel／显式sf2x仍拒绝。后续较小观察包的 [CPU](../reproducibility/poster-cache-budget-minimal-cpu-actual-20261125.json) `445e94deb747f6f7` 独立PASS／38.047秒；[GPU](../reproducibility/poster-cache-budget-minimal-gpu-actual-20261125.json) `502518c1cac3a3a4` 仍STOP／同一pipeline目标未出现。150秒扫描完整3426块、raw `9d77d6af…`／1791188 B，7条postdraw完整、无原snapshot／PNG。回程、健康30秒及worker退出38.468秒。减少request／route包装未在本靴恢复快照，未把诊断干扰认定为主要原因；下一步独立查PosterBoard→Aegir原快照请求入口，原mode／paused保持。

按现行规则加入一次上滑的 [主页同靴验证](../reproducibility/poster-cache-budget-minimal-swipe-actual-20261125.json) `6b7a6729c50d0b51` 已结束，原结果仍STOP／同一pipeline目标缺失。当前LIVE／F12就绪后120秒只发一次swipe，150秒扫描7980块完整，原raw `a2b7dc9a…`／3484551 B。真实帧已进入主页，图标／小组件／Dock色彩与比例看起来正常；背景仍黑，7条postdraw完整、原snapshot／PNG／pipeline均0。四段回程、连续健康30秒及worker退出38.766秒。合成输入不代替真人触控，扫描不证明LCD光学／FPS；没有强制原mode／paused，失败包不重发。下一步针对PosterBoard发给扩展的原快照请求与回调做有界只读观察。

完整native背景包的扫描图中，锁屏文字、电池和底条颜色正常，背景仍黑。它没有原Aegir登记，因此该图不证明壁纸、LCD实际颜色或亮度。真实UI截图优先串口，光学／背光另用相机。

此前 `bb157ab341d460d3` 的GPU包在[Windows主机预检](../reproducibility/poster-cache-budget-native21-host-refusal-20261124.json)超时，download／boot均未发生，原STOP_ERROR与失败包保留。冲突筛选保持，CIM内部20秒／受控子进程外层30秒；后续独立预检及健康32.391秒通过。新条件预热包具有实际功能差别，没有重发该包。

此前旧库原壁纸 `f8d225d5316dfafc` 的[STOP_RESET_UNCONFIRMED](../reproducibility/lilac-wled-v731-poster-startup-actual-20261124.json)保留；之后的健康31.265秒不重分类原失败。R22／R23／R24／R26的原超时和缺块、R25／R27的准确缓存证据在[壁纸专题](METAL-REQUEST-OBSERVATION.md)及历史原文中保留。

## 显示及其它外设

[R21原生几何](DISPLAY-GEOMETRY.md)同靴ADT／MG／UIScreen／F12／scanout为720×1280像素、360×640点、scale2。original六行和fitted五行的38个应用、Dock顺序及Folder内容完整对照；真实祖先、两处17.5点间距已验，平直状态栏移除rdar占位。完整safe-area、真人全屏触控、真实应用启动与光学继续验收；合成上滑不代替真人。

v731多靴在首帧后读到WLED模块启用、双灯串／调制器mask3、fault0、duty1989／1989；未连接硬件写入，不证明亮度或睡眠恢复。背光service及非零调光事务已离线编译／故障测试，详情见[Lilac外设](LILAC-PERIPHERALS.md)。

ADSP stage3只核验固件和AP端准备，没有启动DSP。ASM PCM／映射／APR／ADM路由与COPP层已完成离线模拟和ARM64编译；[本次主机复验](../reproducibility/cache-budget-peripheral-host-validation-20261125.json)保持实际源码。实际传输、AFE、Tasha／Tavil选用和声音未验。Wi-Fi／蓝牙也没有扫描、连接或传输证据。shared-event selectors36／37／38／45仍零输出，presentedTime仍0，未实现项保持明确。

## 复现、整理和操作

[AIR固定原object复现](../reproducibility/cache-budget-verified-air-tool-20261124.json)从正式materialize源码得到原`10df4bf0…`，168个原LLVM导入一致；[完整参考manifest](../reproducibility/air-static-half-object-reference-20261124.json)已进入Git。不同object／target不允许复用原审计，新的20H392 exports仍需重新提取。新源码和工具只在正式仓库维护；构建及新包准入见[BUILD](BUILD.md)、[OPERATIONS](OPERATIONS.md)，用户提供SDK／固件等原输入见[LAYOUT](LAYOUT.md)。全新gl5重建首次启动尚未独立验收。

本轮[Mac原生Trash](../reproducibility/cache-budget-macos-native-trash-20261124.json)54项／238文件／287602294 B，233保护文件SHA保持；六份重复HFS从保留blob逐字节复算，原失败包、raw、proof、signed库、AIR、deps和pause保留。两套Ubuntu Trash为空，Windows／Mac垃圾桶未清空；回收仍占磁盘。整合前9份入口原字节及SHA见[本轮归档](archive/20261124/cache-budget-close-out/original-document-manifest.json)，完整历史由[INDEX](INDEX.md)／[ORGANIZATION](ORGANIZATION.md)索引。

本次原观察收尾另把 [Mac46项／88文件／136572265 B](../reproducibility/poster-observation-macos-native-trash-20261125.json) 送入原生Trash，111保护文件一致；[WSL生成的9个主机程序／13062696 B](../reproducibility/cache-budget-host-windows-recycle-20261125.json) 从共享Windows build直接进Windows回收站。原编译失败、源码、固定APR头文件、signed库、原结果／raw／proof都保留。正式主提交`7c02219`已推送origin/main，Mac已同步且135变更文件逐字节一致，旧Mac修改保存在[可恢复stash](../reproducibility/cache-budget-formal-git-sync-20261125.json)。

只操作BH905ND69G，fastboot设备命令显式`-s BH905ND69G`；COM≥10且VID1209／PID0001、一个controller／worker；PON只0x06，单download／boot、失败不重发。ACK／DRAIN／RESET／FINAL_DRAIN、健康30秒及旧worker退出后才下一轮。无本token LIVE的早期E1F4仍可能需物理恢复。SweetGPU SHA `aa2c3679…`及内核GPU底层／allocator／SCM／PON／RPC／VM冻结；gl5／userdata／SD／TA／XFL、校准、备份和救砖输入不flash／erase。Mac旧autobuild保持pause；废弃文件只进原生回收站。
