# 当前状态 — 2026-11-24

R25节流版GPU63168a33181e8a4f已独立PASS／37.812秒：完整native21／PNG6、原生几何／scanout和缓存58文件／29请求／29bin、missing0／实际skip全部原字节复算通过。原raw bcbf6bba／2935059B，COM17单流、四marker／PON0x06、健康30秒与worker退出通过；CPU84d6d2af0f2f72f9自身PASS／39.000秒。R23／R24失败与缺块保留。

[R26 原壁纸诊断 RAM](../reproducibility/poster-cache-r26-offline-20261123.json)自身 CPU `38e5e7e0a91b3fbd` 已独立 PASS，COM17 单流，四段 0x06 回程及连续健康 30 秒共 38.078 秒。后续实机状态见下方 R26 GPU 与 v731 记录。[首次 GPU 打包拒绝](../reproducibility/poster-cache-r26-stage-20261124.json)保留：616 字节超过 607 字节上限，当时未生成镜像。

[修正后的 R26 GPU 包](../reproducibility/poster-cache-r26-fitted-default-offline-20261124.json)为 605 字节，保留三路日志与原生几何，只对固定 helper 在超长时省略默认的 `GEO_GRID=1`。整包、同 RAM CPU、R25 原 native21／PNG6 与 Mac 83 份实际源码均通过离线准入。下次真机验收必须在本次 `LIVE→ACK` 内验证 SpringBoard 的拟合模式记录、Aegir 原调用和扫描帧。随后已按原包单发，见[本次原始证据复算](../reproducibility/poster-cache-r26-gpu-actual-20261124.json)：SpringBoard 实际报告拟合 `value=1`，Aegir 三次确认同一缓存范围，记录 17 次二进制导入命中，但 `LIVE→ACK` 内仍有两次扩展签到超时。失败触发的唯一扫描帧缺至少一块 128 B 数据，未到计划 150 秒，也没有完整 END。原结果 `STOP / SCREEN_EVIDENCE_MISSING`，不重发；四段 `0x06` 回程、健康 30 秒及串口关闭已核验，共 38.937 秒。原壁纸和真实扫描帧仍未验收。

[R27 缓存背景复验](../reproducibility/poster-cache-r27-inventory-actual-20261124.json) `74d17a254c08c57c` 独立 PASS，完整 native21／PNG6 与扫描帧均通过。R25 的 58 个文件逐项保持原 SHA，新清单 62 文件／31 请求／31 二进制，缺失 0，因此未运行预热。新增两对的源码长度与二进制负载大小和 R26 原日志吻合，见[对照](../reproducibility/poster-cache-r26-r27-comparison-20261124.json)；不能据此确认原调用使用的精确键或壁纸已恢复。四段回程与健康检查共 38.797 秒，采集已退出；手机最后停在健康 fastboot。

[Lilac 外设](LILAC-PERIPHERALS.md)的 WLED 只读候选已登记为独立 v731／`15fd8eee…`。[四阶段包](../reproducibility/lilac-wled-v731-prerequisites-20261124.json)均通过完整 payload 重建；private 单帧包通过标准离线准入和 Mac 85 份源码核验。背光服务及注册入口通过[独立链接验证](../reproducibility/lilac-backlight-service-linkcheck-20261124.json)。非零[调光事务](../reproducibility/lilac-wled-duty-transaction-20261124.json)已通过模拟总线故障测试，未连接硬件写接口。[v731 首轮只读实机](../reproducibility/lilac-wled-v731-private-actual-20261124.json)已独立 PASS：完整 720×1280 BGR888 原扫描帧（535 块）之后，读取到模块启用、两条灯串和调制器均启用、故障位 0，两路占空比均为 1989。回程四段、健康 30 秒、worker 退出通过，共 38.859 秒。该阶段完成后，同核 [owner／ready／RAM CPU 三阶段](../reproducibility/lilac-wled-v731-admission-actual-20261124.json)均已完成各自原字节复验、四段回程、健康 30 秒及采集退出；完整回程分别为 41.453、40.234、38.594 秒。[v731 自身 GLES9](../reproducibility/lilac-wled-v731-gles9-actual-20261124.json) `5003b7449459d175` 已独立 PASS：GLES 专用 RAM 先通过自身 CPU，GPU 靴复验全部九组、像素及资源退休，150 秒原扫描帧完整（3405 块），随后读取到同样的双串占空比 1989、故障位 0。完整回程 38.281 秒，采集退出；手机最新停在健康 fastboot。随后 [v731 背景 Metal21／PNG6](../reproducibility/lilac-wled-v731-metal21-actual-20261124.json) `97fa008727dbc047` 从原日志独立通过，已生成同核 [native21](../reproducibility/lilac-wled-v731-metal21-native-proof-20261124.json) 和 [PNG6](../reproducibility/lilac-wled-v731-metal21-png-proof-20261124.json) 准入证据。150 秒扫描帧完整（3420 块），同靴 WLED 再次读出两串 1989、故障位 0。R27 的 62 个缓存文件记录保持一致，缺失 0，预热跳过。完整回程 38.828 秒，采集退出。原壁纸、实际调光及睡眠唤醒仍待验。

[v731 原壁纸启动诊断](../reproducibility/lilac-wled-v731-poster-startup-actual-20261124.json) `f8d225d5316dfafc` 未见签到超时，复算了三份非空原快照及三次 PNG 返回。本轮只采串口，无扫描或相机证据。四段 `0x06` 标记完整，但 fastboot 枚举曾中断，原结果 `STOP_RESET_UNCONFIRMED` 保留，不重发。随后独立只读健康检查通过 31.265 秒，手机最新停在 fastboot。缓存写入 `-28` 与 64 文件预算吻合，仍需验证实际清单。

[缓存末槽修复](../reproducibility/poster-cache-budget-offline-20261124.json)已通过 macOS 与 WSL 边界测试：请求为配对文件留位，二进制按实际发布占用计数，macOS 可写满 32 对并拒绝第 65 个文件。签名 ARM64 新 RAM 已完成自身 CPU `77f81204d559eb64` 和 [v731 GLES9 实机复验](../reproducibility/poster-cache-budget-gles9-actual-20261124.json) `ce5c76be554da1aa`，均独立 PASS。CPU 原 raw 为 `4642eca0…`／335851 B，完整回程 37.844 秒；GLES 原 raw 为 `2d6cf85f…`／1819660 B，全部九组、像素及资源退休通过，150 秒原扫描帧完整（3417 块），同靴 WLED 双串占空比 1989、故障位 0，完整回程 39.156 秒。COM17 单流、四段 `0x06` 回程、连续健康 30 秒和 worker 退出均通过；后续独立健康观察另通过 31.968 秒，手机最后停在健康 fastboot。新库生成缓存身份 `16722b12…`；新 Metal／source-map 尚待构建及完整 native21／PNG6 验收，原设备缓存清单和壁纸仍待独立观察，旧 Metal 证明不用于新库。纯 GLES 包按规范只含 GLES／libglapi／geocompat，不需要 exception observer。

新库的 Metal 资源已构建，AIR object 从正式源码重建后与原审计 SHA 完全一致（[重建记录](../reproducibility/poster-cache-budget-air-reconstruction-20261124.json)）。第一份新 native RAM [自身 CPU](../reproducibility/poster-cache-budget-native21-cpu-actual-20261124.json) `8078d5413d8da5c4` 独立 PASS，39.078 秒完整返回；其 GPU 包 `bb157ab341d460d3` 在 [Windows 进程预检超时](../reproducibility/poster-cache-budget-native21-host-refusal-20261124.json) 后 STOP_ERROR，download／boot 均未发生，不重发。后续独立健康检查通过 32.391 秒。新的 [条件预热 native21／PNG6 包](../reproducibility/poster-cache-budget-native21-prime-offline-20261124.json) 加入实际缺 bin 才运行的有界 primer，与前次失败包具有实际差别；自身 CPU `1fdd166cf693cf15` 独立 PASS、39.375 秒完整返回。本次 GPU `2d1794b225c1a6c1` 正在原单流中观察，最终准入仍待原字节、150 秒扫描帧、四段回程、健康 30 秒和 worker 退出，不能以局部日志提升。

R22已结束：新RAM自身CPU通过，原GPU为REVISE／EXTENSION_CHECKIN_TIMEOUT；单download／boot、PON0x06、四回程marker、连续健康30秒及worker退出通过。失败包不重发。该轮 Mac 回收与文档整合已执行，CPU／GPU完整payload、signed先验及Mac实际83源的收尾离线复验已通过。

音频方面，[R25／R26 RAM CPU 原日志复算](../reproducibility/lilac-adsp-prepare-20261124.json)确认 ADSP 固件核验和 AP 端准备完成；两靴均未启动 DSP。PCM 路由现绑定[ADM 输出对象的打开回复与句柄](../reproducibility/lilac-adm-playback-device-offline-20261124.json)，打开、路由、播放及关闭的模拟测试与 ARM64 编译通过。实机传输、AFE／编解码器配置和物理声音仍未验。

R22在四个原AegirPoster进程解出24条完整environment状态；两份check-in超时栈定位到Mesa的GLSL→NIR链接优化，第三份栈截断，未补造。原route有两条ID缺失，严格decoder拒绝保留。后续 R23–R25 已核对同来源缓存并补全三个缺 bin；不复发 R22、不强制原mode或paused。见[R22 原阶段](../reproducibility/poster-state-r22-stage-20261123.json)、[壁纸专题](METAL-REQUEST-OBSERVATION.md)。

## 准确输入与已验范围

| 输入 | 精确身份 | 实际范围 |
| --- | --- | --- |
| v730 kernel／83实际源 | `f258dee9c6d142b90a573f796eeec5e92b39dd65feed6a21b2542d90fb66a88d`，20665112B | private／owner／ready、CPU、原scanout及0x06回程；底层冻结 |
| Mesa／Freedreno | 24.3.4／`769e51468b49b2a42f0a0eaf71cf9eed5ff4e5de`／FD540 GLES3.1 | 免费开源后端；Vulkan尚未移植 |
| 当前GLES | `0766d5c713a883426e801e6d162bbdc46df79c5bc094ca77a7b6d6462599d607`，14801120B | 自身CPU、完整GLES9；R22也有本LIVE内GLES9成功记录 |
| 当前Metal／source-map | `8feea4593116ffe9187204f5dd4cf4dd48d3027816d4763ecef20da200f86a27`／`db1605ef8dae8f8ed0a29999dcb2cf7537457aba7ad428677c21e0e914771b78` | R11完整native21及PNG6；库字节保持 |
| R21 geocompat revision4 | `ee7a3207f26af5e9106cbd98c74392153b11a2d345386964db131110a2a6153c`，106544B | 原Class树／最近祖先、完整模型、17.5点标签间距 |
| R21 RAM blob | `4277931a612742a2eb2e02f67af8c2083116a13686c7594e7d0a3b5e87f0cf27`，3920885B | 自身CPU及original／fitted两靴独立通过 |
| R22 observer revision1 | `e501c4be6e75658ae2ccb32513e39be641c8dd15dd04b466bc6f777cf658f019`，150688B | host ASan／UBSan、ARM64签名和24条原返回状态；GPU未准入 |
| R22 RAM blob | `84994aed23f0c460c68babb2eb5d83ebf5c215a654b5c4931ba011763ff08e97`，4269268B | 新RAM自身CPU通过；原GPU失败保留 |

R22 observer实际编译源03c1c4d，打包源0f59e90；当前Git HEAD含后续证据与整理，不提升生产基线。每份manifest锁定实际guest／host源和signed文件，Mac／Windows来源一致。SDK／签名变化仍需新二进制自己的准入。生产CA复现见[run-index](../reproducibility/v730-ca-matrix-r1-run-index.json)。

## 最新单发结果

| 阶段 | token | 原结果 | 完整返回含健康30秒 |
| --- | --- | --- | ---: |
| R21新RAM CPU | `960521e563270307` | PASS | 39.266秒 |
| R21 original／GEO_GRID0 | `3e28639a5012be41` | GLES9、scanout、完整祖先／模型基线PASS | 39.438秒 |
| R21 fitted／GEO_GRID1 | `41797ddbf4c8b947` | 完整38应用／Dock／Folder及17.5点间距PASS | 38.859秒 |
| R22新RAM CPU | `61e1bcd69b12bd13` | 独立PASS，COM17单流、原字节及回程复算通过 | 38.078秒 |
| R22原environment GPU | `68f6a0c0d6068c37` | REVISE／EXTENSION_CHECKIN_TIMEOUT | 39.906秒 |

R22 GPU原image SHA为`c20d6e5db81e60367910b8fef76b19d67fdfe5c916e197ee8b87ce5d9f2f0d43`；原raw1905932B／`7491cb2e6ad9292b562f4d9646ce3ca1627c3276a4da968107c025fa2ce45263`。run位于Windows TEMP/lilac-v730-poster-state-r22-gpu-live-20261123。三次check-in超时在LIVE→ACK内，第四次在退出期，单列排除。失败时唯一原scanout的来源、寄存器、3431块、BGR／PNG像素均独立通过；它未到计划150秒，不作为上滑或home验收，14.781秒是请求后单帧捕获、解码与保存耗时。

首次R22 GPU打包在607B命令区被主机拒绝，没有设备启动；[原拒绝](../reproducibility/poster-state-r22-command-refusal-20261123.json)保留。固定v730的sf2x初始0经实际consumer／parser SHA核对，限定本kernel的默认0省略与已验numeric-one拼法后，gpu-r2为604B。完整payload和六个篡改拒绝通过；未改shim、内核或运行语义。[离线记录](../reproducibility/poster-state-r22-offline-20261123.json)保持投递前范围。

## 显示、GPU和其它外设

R21两靴各927 TREE、158对VIEW-RECT、14 OWNER、3组grid、4模型／41icon／38应用。完整父边、最近原Class祖先、应用multiset、Dock顺序及Folder内容独立一致；根层29、Dock4、Folder内5。两处widget标题到下一排图标都17.5点，refused／truncated／write_failures均0。[原阶段](../reproducibility/native-label-ownership-r21-stage-20261122.json)、[original基线](../reproducibility/native-label-ownership-r21-original-baseline-20261122.json)、[fitted profile](../reproducibility/native-label-ownership-r21-profile-20261122.json)保持原token和字节。

同靴ADT、MobileGestalt、UIScreen、真实扫描与F12范围为720×1280像素／360×640点／scale2，面板BGR888／stride2160；圆形Clock及图标比例保持，LegacyPhone平直状态栏已移除rdar占位。完整safe-area、真实应用启动及真人全屏触控仍需独立验收；120秒一次合成上滑不替代真人。

R11完整[native21 proof](../reproducibility/current-native21-proof.json)与[PNG6 proof](../reproducibility/current-PNG6-proof.json)绑定同kernel／GLES／Metal／source-map。五个呈现阶段在各自CA事务闭合后allocated均0、最终0。presentedTime仍0；shared-event selectors36／37／38／45仍零输出、未实现。旧R10残留11520B与原失败保留。

R13已验33个原方法和mode路由，原applyMode1→deactive本来就是初始化策略；原paused保持。黑壁纸、原动态UI／FPS、睡眠唤醒、LCD光学／背光、声音、Wi-Fi及蓝牙继续推进。CPU／launchd、真人F12及音量键有历史实机证据，R22不新增外设通过。局部probe、GPU Completed、seed、原snapshot、power_off和扫描缓冲区分别保持证据范围。

## 操作与本周整理

只BH905ND69G；fastboot设备命令-s，COM≥10且VID1209/PID0001、一个controller／worker、单download／boot、失败不重发、PON只0x06。四marker、连续健康30秒及worker退出后才下一轮。无本token LIVE的早期E1F4仍可能需物理恢复；gl5／userdata／SD／TA／XFL、救砖／校准／备份受保护。SweetGPU及内核GPU底层／allocator／SCM／PON／RPC／VM冻结，当前GLES／Metal字节保持。

R22 Mac178项／4097文件／179699518B与Windows4项／16文件／29061727B已进入原生回收站，1282个Mac及447个Windows保护文件SHA一致。Mac旧research的18项非Git内容已归档／移入assets，38份历史文档与源文件及53份现行文档原文可按SHA恢复；Windows research89个、Mac正式research3个参考Git仓库。Ubuntu两套垃圾桶无内容。原raw／失败包／signed库／blob／manifest／固定deps和pause保留，所有回收站未清空。

Mac回收快照build约3.72GiB、系统可用约92.32GiB；移入垃圾桶仍占磁盘，历史累计不能当释放量。[ORGANIZATION](ORGANIZATION.md)给原生路径与整合索引，[BUILD](BUILD.md)给复现，[OPERATIONS](OPERATIONS.md)给单发／截图／相机，[LAYOUT](LAYOUT.md)给恢复，专题及历史由[INDEX](INDEX.md)串联。新修补与证据只在正式ios-xperia-xz1c维护并同步Mac。
