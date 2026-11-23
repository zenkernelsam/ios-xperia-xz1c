# 当前状态 — 2026-11-23

R23有界缓存清单／条件primer候选已完成真实C文件测试和12项独立解码故障拒绝，准备Mac完整native21／PNG6签名包及自身CPU。当前无in-flight；新的Sony只读健康观察连续30秒通过并退出，设备download／boot／reset均0。现有kernel／GLES／Metal保持准确字节，R22原失败保留。

R22已结束：新RAM自身CPU通过，原GPU为REVISE／EXTENSION_CHECKIN_TIMEOUT；单download／boot、PON0x06、四回程marker、连续健康30秒及worker退出通过。手机最后实机状态为健康fastboot，无in-flight。失败包不重发。Mac本轮回收与文档整合已执行，CPU／GPU完整payload、signed先验及Mac实际83源的收尾离线复验已通过；完整GPU／UI／外设目标仍active。

R22在四个原AegirPoster进程解出24条完整environment状态；两份check-in超时栈定位到Mesa的GLSL→NIR链接优化，第三份栈截断，未补造。原route有两条ID缺失，严格decoder拒绝保留。下一轮先核对本来源scope的实际请求／缺失bin，再准备有界背景primer的新RAM自身CPU；不复发本轮、不强制原mode或paused。见[本轮原阶段](../reproducibility/poster-state-r22-stage-20261123.json)、[壁纸专题](METAL-REQUEST-OBSERVATION.md)。

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
