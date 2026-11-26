# 当前状态 — 2026-11-27

CPU／launchd、触控／音量键、完整GLES9、Metal21／PNG6和原生720×1280几何已有真机证据。原Aegir壁纸仍未通过：星点4212顶点／1404图元停在逐图元同步提交。当前GPU r5已修正Mesa图像binding与紧凑驱动槽位的对应，完整主机检查和四份新包离线审计通过，尚未实机。音频现已具备AFE／ADM／ASM会话、GLINK适配及具体SMEM FIFO回调代码；真实DSP／codec／声音仍未接通。

## 设备与下一轮

最后实机观察为Sony **BH905ND69G**安全停在fastboot，无在途启动。原native CPU token `ef161dc3ab9ce723`的guard未取得2/2成功，download／boot均0；[原拒绝](../reproducibility/ordered-barrier-r1-native-cpu-guard-refusal-20261126.json)保留且不重发。随后只读健康32.156秒通过，不能提升guard失败。[本轮只读PnP](../reproducibility/pcm-c-abi-pnp-observation-20261126.json)仍显示到达时间02:05:52，早于02:18:31失败guard；未确认新的物理会话，本轮没有guard／串口／download／boot。

新的物理会话确认后依次：r5自身CPU→GLES9→native RAM自身CPU→完整native21／PNG6，再按新证明构建原Aegir／PNG／150秒scanout包。每轮ACK／DRAIN／RESET／FINAL_DRAIN、连续健康30秒及旧worker退出均独立通过才下一轮。新GLES不能借旧native／PNG proof准入；失败包不换token重发，缺扫描块不补发。

## 已验基线

| 输入 | 精确身份 | 已验证范围 |
| --- | --- | --- |
| v731只读WLED内核 | `15fd8eee111bc880b9b54aeee284f9dc91cd2b7dbbbe302dc944b00d8d2d3e69`，20681752 B／85源 | 自身private／owner／ready，完整首帧后只读WLED |
| v730基础内核 | `f258dee9c6d142b90a573f796eeec5e92b39dd65feed6a21b2542d90fb66a88d`，20665112 B／83源 | 历史几何／原UI和诊断基线，底层冻结 |
| Mesa／Freedreno | 24.3.4／`769e51468b49b2a42f0a0eaf71cf9eed5ff4e5de`／FD540 GLES3.1 | 免费开源GLES后端；Vulkan未移植 |
| 已验GLES | `6efabfc0627a8818a8c4c45665459cd33d8ee814f2e50a3f3cdc4813bb254325`，14801120 B | 自身CPU、GLES9，与下列Metal完整native21／PNG6 |
| 反射Metal | `19a08083833851d5a13016473ec3c3db98629d6ce6fc928b70947af114abee49` | reflected buffer选择、unused绑定保留及pipeline切换 |
| Metal source-map | `41bdf3e49258c8cf6d57a69beb99d15dec0e1404aec2509725c386941e0f25ee` | 完整先验只准入准确GLES／Metal／kernel组合 |
| R21几何helper revision4 | `ee7a3207f26af5e9106cbd98c74392153b11a2d345386964db131110a2a6153c`，106544 B | ADT／MG／UIScreen／F12／scanout同靴720×1280像素、360×640点、scale2 |
| 已验cache build／scope | `16722b126d14e64954fc349d20626f9e639cc726ed1c24856a889b2249efd236`／`9f43d3e226e901f609eeba4b9f348e2050ed7e1bd790ef8f7b422dc31eec892a` | 原24对／48文件、missing0；后续真实请求扩充，不删除旧cache |

反射Metal完整原字节token **`1e1bf33f91081ea2`**、raw SHA `882969267ed97fd71130e785bad05497b8eac636597aed5cba6120e4dc5d4bdd`、39.157秒完整回程：[实际验收](../reproducibility/buffer-selection-r3-gpu-actual-20261125.json)、[native21 proof](../reproducibility/buffer-selection-r3-native21-proof-20261125.json)、[PNG6 proof](../reproducibility/buffer-selection-r3-png6-proof-20261125.json)。完整页／guard／fence／退休、unused输入48 B及150秒扫描3460块通过。该组合功能通过不提升原壁纸、FPS、光学或睡眠。

## 当前GPU候选 r5

GLES SHA **`dc7113a10542c4b8de2036def8373a4574ab2092d17efd1a39e1c224f1bec308`**，Metal及source-map仍为上表准确身份。生产源与Mesa归档均绑定[r5离线报告](../reproducibility/ordered-image-barrier-dense-r5-offline-20261126.json)。

| 产物 | SHA256 |
| --- | --- |
| GLES RAM blob | `31b8ab8196b7bfa84987253b91f8bb7c4b8a3855b37e1eee000dd0f4488cf074` |
| GLES RAM manifest | `ad6365ab02e69674f54e35b7f70895afb63b39b72a848b1bceed028d2f7a94e9` |
| native RAM blob | `75b8f08201a972f9c67ca1a63110131e05286116bfbf47bb1a8fb19548b354f0` |
| native RAM manifest | `ae8acd03c0d7e2ef8a0bec81c01a133c73a3e09228f5beb58b534802a8419e08` |

四包在`build/ordered-image-barrier-dense-r5-20261126`，均**未投递**：

| 顺序／目录 | token | 当前证据 |
| --- | --- | --- |
| CPU／cpu | `549746d4b51a7668` | 完整payload和run_trial离线门槛、Mac85源及新用户态审计通过 |
| GLES9／gles9 | `b5eea38cb400afa4` | 完整payload通过；等待自身CPU实机先验 |
| native CPU／native-cpu | `a0c42c1d43ab5734` | 完整payload和run_trial离线门槛、Mac85源及新用户态审计通过 |
| native21／native21 | `2fb820a1d5198d03` | 完整Metal21／PNG6包，等待新CPU／GLES9先验 |

barrier revision4从当前linked fragment program的ImageUnits映射GL binding到dense Gallium槽位，按准确readonly／writeonly coherent RGBA32F资源检查。旧r4人为si[0]／si[8]布局会错走fallback，已用相同真实两槽输入复现，r4保留历史且不作为下一轮候选。新host直接编译锁定Mesa mapper核对255种mask／两种声明顺序，并生成1404次／29489字／117956 B连续CP流，11次模拟批次切换和9次guarded ring扩容通过ASan／UBSan。

优化仍保留每图元XR舍入、首次scratch64 GPU初始化、单调CACHE_FLUSH_TS／WAIT_REG_MEM／UCHE与final fence。异步GL、未知图像、其它stage image、写SSBO／atomic、深度或多样本保持原barrier。主机命令流／映射不证明实际A540缓存、像素、原壁纸或速度；完整native验收须逐项核对优化命中及重叠图元原页。

## 原壁纸与显示卡点

[原星点r5](../reproducibility/aegir-retirement-r5-gpu-refused-20261126.json)token `454044d0bb3177b8`已经进入后端call2：buffers2、count4212／ordered1404，停submit后签到超时3次；原REVISE及39.281秒回程保持。固定Mesa的fd_memory_barrier会fd_context_flush，支持继续验证上述提交优化。[ACK后ring sig11栈](../reproducibility/aegir-star-r5-post-ACK-ring-symbols-20261126.json)单列退出期，不归为本LIVE根因。更早大shader超时、primer缺SHA、旧黑PNG和缺scanout的原结果均保留，由[壁纸专题](METAL-REQUEST-OBSERVATION.md)索引。

原SnapshotManager已按准确库／source-map使用独立cache目录，旧目录不改删；原720×1280加载PNG全黑仍是实际失败证据。只读RBYTES／CGBYTES／queue retirement与无损OPNG只观察原返回，不造像素。尚无完整Earth／post／snapshot恢复证据，不强制解除Vista paused／mode或画替代UI。

[原生几何](DISPLAY-GEOMETRY.md)已核对original六行／fitted五行全部38应用、Dock顺序、Folder内容、平直状态栏及真实标签祖先／两处17.5点间距。完整safe area、真人全屏触控、应用启动和光学仍待验；串口合成上滑不代替真人。原scanout仅720×1280／BGR888／stride2160，每靴一次、45或150秒；缺块失败不补发。扫描帧不证明面板亮度、错行、FPS或睡眠。

## 其它外设

| 外设 | 当前实现／实际证据 | 仍待完成 |
| --- | --- | --- |
| WLED／背光 | v731首帧后只读fault0、enable／strings mask3、duty1989／1989；属性service与非零调光事务离线通过 | 实际注册／SPMI写入、亮度／灭屏／唤醒及光学 |
| ADSP／音频 | 实机只stage3固件核验和AP准备；AFE／ADM／ASM、GLINK／FIFO及PCM C接口完成主机和ARM64验证；启动源码审计确认RPM前置及stage4～7最终shutdown | 持续DSP供电／启动／关闭生命周期、endpoint／schema／token回显、Tasha／Tavil选用、实际mapping／cache回调、codec和物理声音 |
| Wi-Fi／蓝牙 | 原DTB和开源路径已核对，进程有启动日志 | 控制器初始化、扫描／连接和真实数据传输 |
| shared event／presentedTime | selectors36／37／38／45仍零输出，presentedTime0 | 端口／mapped page及实际呈现时间未实现；旧草稿不算通过 |

音频当前入口为[C ABI／运行时离线报告](../reproducibility/lilac-pcm-c-abi-offline-20261126.json)：60,624 B／8 B对齐的C接口context内含60,352 B运行时，拥有具体FIFO、原Link及transport。owner提供内核栈外、清零且不搬移的稳定存储，串行驱动；配置和回调表复制，backend contexts与PCM mapping继续保留。构造先纯验证FIFO／PCM／schema／policy和回调，不执行IO；复制的context、重入操作及覆盖PCM／context的输出指针均拒绝。准确epoch／DSP／codec／SMEM readiness、CPU映射／device地址和cache-line边界先验证；输入字节复制及release-to-device回调完成才允许APR排队，成功匹配WRITE_DONE和acquire-from-device回调都完成才复用跨度。四份跨度不能相交；在途／uncertain context拒绝销毁，只有未启动或完整会话及双向GLINK关闭后才能结束对象。真实cache指令／mapping／mailbox／驱动注册仍未连接，未启动DSP或产生物理声音。

WSL／Mac同源十二组ASan／UBSan及六个ARM64对象通过，C11 caller和C++实现独立编译／链接；具体ARM64 C接口只留下正常stack guard导入，没有分配器或未实现FIFO符号。保留原2048环位置／逐字故障／EOS／关闭测试，另核对PCM字节和guard、重叠复制、外来token、缓存交接顺序与IO／cache回调内部过期。真实时钟在receive、poll后、publish后和cache回调后重采样；轮询入口的旧时间不能准入迟到回复或CloseAck。旧仅手动时间的transport接口仍供离线测试，实际运行时始终绑定clock hook。GPU r5源码与包未改。

原session／GLINK适配保证Busy只发同一未发布包、成功publication才开始回复deadline，Accepted／typed等待不延长；控制与WRITE完成必须匹配已发布token／epoch／端口。四WRITE排空、EOS普通回复及RENDERED_EOS都确认，再Close／Unmap／COPP／AFE Stop和双向GLINK关闭后才确认region可释放。细节与历史协议证据由[LILAC-PERIPHERALS](LILAC-PERIPHERALS.md)索引。

[ADSP启动源码审计](../reproducibility/lilac-adsp-boot-source-audit-20261126.json)确认当前cold pilot先要求RPM电压／时钟／带宽；stage4～7成功后也最终PIL shutdown，不能给后续PCM runtime提供持续运行的DSP。真实音频接入须另定义保留的DSP生命周期与当前readiness；本次没有改冻结RPM／SCM或调用PIL，不把源依赖当当前RPM故障／健康证据。

## 正式复现与规则

正式源码、增量、工具、锁和报告只在本仓库。Windows／WSL使用同一正式目录，Mac为`~/Desktop/ios-xperia-xz1c`且旧autobuild保持pause。[BUILD](BUILD.md)给准确源码复现，[OPERATIONS](OPERATIONS.md)给准入与回程，[INDEX](INDEX.md)串技术和历史，[ORGANIZATION](ORGANIZATION.md)维护回收与可恢复stash凭据。所有已验binary仅准入自己的准确组合；HEAD候选不冒充生产UI。SDK／签名变化须新产物实机验证，全新gl5首次启动仍未独立验收。

本轮C接口源码与准确编译报告只在正式仓库维护，WSL与Mac的40份编译输入逐字节一致；提交、Mac正式同步和回收凭据由[ORGANIZATION](ORGANIZATION.md)索引。前轮[运行时同步](../reproducibility/pcm-runtime-formal-git-sync-20261126.json)与原stash继续保留。GPU候选及冻结内核保持。 [最新Mac核验](../reproducibility/adsp-boot-audit-pause-recovery-20261127.json)确认源码审计同步及24份Mesa归档／v731／冻结SweetGPU SHA；发现缺失的旧autobuild pause已恢复，检查时无相关构建进程。

只操作BH905ND69G，fastboot设备命令显式`-s`；COM≥10、VID1209/PID0001且一个worker；PON只0x06，单download／boot、失败不重发。内核GPU底层／allocator／SCM／PON／RPC／VM冻结，SweetGPU.cpp SHA **`aa2c36792a2298b060aa858eabdc039522fcdde3cfdbafd453c5fc6c0683c157`**。gl5／userdata／SD／TA／XFL／校准／备份／救砖输入受保护。

本轮整合前五份入口与原SHA在[原文archive](archive/20261126/pcm-c-abi/original-document-manifest.json)。废弃host产物进入原生回收站，WSL共享build产物走Windows回收站，准确receipt由ORGANIZATION索引；不永久删除或清空Windows／Mac垃圾桶。原失败、准确候选、少量所需raw／proof和受保护输入保留。
