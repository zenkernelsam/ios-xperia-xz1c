# 当前状态 — 2026-11-28

CPU／launchd、触控／音量键、完整GLES9、Metal21／PNG6和原生720×1280几何已有真机证据。当前v732日志队列／r5 GLES／反射Metal组合七轮准入通过且dropped0；原Aegir仍在Earth的NIR链接阶段超时，原UI／FPS／睡眠／光学和其它外设保持未完成。内核GPU／allocator／SCM／PON／RPC／VM冻结。

## 设备与下一轮

2026-11-28用户确认物理会话已恢复；Sony独立只读健康30.75秒通过。新局部SSA组合自身CPU token `92b7017cc30e02bd`已独立准入，日志dropped0、37.641秒完整回程／健康。GLES9 token `999c9b10a80fc321`控制器PASS，但[独立验收拒绝](../reproducibility/nir-callee-ssa-gles9-refused-20261128.json)：本靴CA安装记录在LIVE前，旧revision1不能准入；LIVE后六阶段原像素max_diff0、dropped0及308块扫描另存，不能提升完整GLES。37.922秒完整四marker／健康与旧supervisor／owned workers退出已独立核对，Sony健康fastboot，无在途。native CPU／native21未投递。[本轮结果索引](../reproducibility/nir-callee-ssa-device-round-20261128.json)保持原拒绝；该包不重发。

新CA observation revision2只在probe入口重新读取当前QuartzCore UUID／有界segment／代码锚点／原identity和两个实际已安装pointer／case，日志VERIFY在像素phase0前；不重装、不搬移旧INSTALL日志。原revision1仍要求LIVE内INSTALL，原失败不重新分类。[主机六阶段／只读表项检查](../reproducibility/ca-current-slots-host-20261128.json)与5组decoder、45组准入（2旧输入skip）、native／PNG回归通过。内核冻结保持。

[新SSA／CA候选](../reproducibility/ca-current-slots-SSA-candidate-20261128.json)已实编译、签名及完整打包：GLES `e7cf76d676f9211e3f87bf8ae8c420dc590488cd51e8f711a7c0b66ec01c0c2e`／14801376B，保留已验API和revision4几何helper原字节；两个正确RAM为gles-ram-r3／native-ram-r2。CPU `bffe3867a5cd1b92`、GLES9 `052e16406b2c1148`、native CPU `8a8656341d01c0d0`、native21／PNG6 `66a0459acc20c392`四包均未投递，CPU离线实际86源／compiler overlay审计通过。新完整组合仍依次验自身CPU→GLES9→native CPU→native21／PNG6；9个fixture与AIR object准确SHA复核，inventory只读，无primer。原r1局部结果不跳级；完整回程、健康及worker退出后才下一轮。

2026-11-27用户已重新插拔并确认Sony **BH905ND69G**蓝灯fastboot；本轮先独立只读健康32.750秒通过，随后按下述候选单发。最新原失败token `02758c66f824e349`已完整0x06回程，失败包不重发；准确原记录见下节。

新标准bin预热候选 `build/earth-cache-prime-r1-20261127`已完成离线审计；CPU单发token `3579aca49123dae0`已通过独立准入，39.703秒完整回程／健康且worker退出，日志dropped0；[原CPU报告](../reproducibility/earth-cache-prime-cpu-actual-20261127.json)保留。后台GPU单发 `e7c201e4f261e7ec`捕获在约90秒停止，原controller／worker PID已不存在且无results.json；[原中断观察](../reproducibility/earth-cache-prime-interrupted-observation-20261127.json)保持未准入。当前Sony独立只读fastboot健康31.015秒通过，无在途进程；四回程marker未捕获，不用枚举正常提升原阶段，也不重发该包。当前继续离线处理后台生命周期和NIR链接；下一次实机仍须满足OPERATIONS的会话及完整回程条件。

[Earth离线核对](../reproducibility/earth-link-host-offline-20261127.json)从当前gl5只读核对230044B原库，再用完整旧request与正式HalfESSL／XRBlend重放。重放VS4879B／FNVf2eef44db6a198a7、FS76869B／FNV27c2d57438bf46fe，均与最新LIVE→ACK日志一致；`bytes=74672`是原转换GLSL，原AIR20624B。代表function constants不当真实值使用。

单块NIR内联原型通过真实debug validator、ASan／UBSan及1／64／4096次嵌套／分支图的完整序列化字节对照，未安装生产源。固定Mesa的准确Earth链接阶段2.568→2.345秒、约8.7%改善，但两侧整进程都在LLVM14 host退出期失败；不提升为host GL／真机GPU通过。当前生产Mesa不变。

下一次恢复合法实机会话后，按新GLES RAM自身CPU→完整GLES9→新native RAM自身CPU→完整native21／PNG6顺序单发；各轮四回程／健康／worker退出均通过后才下一轮。新原Aegir观察包须嵌入上述组合自己的fresh proof，不能复用旧r5证明；只有该build实际出现missing request时才另建conditional primer。旧r5原请求／失败保留，不重发。

## 当前编译器与监督进程候选

[独立后台监督进程](../reproducibility/hidden-trial-supervisor-host-20261128.json)经真实两个shell测试：启动0.75秒返回，监督进程／owned worker继续运行20秒，晚到stdout完整；live:false／未知字段在启动前拒绝。正式入口start_trial.ps1只启动一次background_trial.py，后者直接调用原run_trial；没有新USB／串口操作或重试，PON／RPC／kernel保持。中断原GPU不提升，下一轮仍需会话及完整回程准入。

[局部SSA编译器候选](../reproducibility/nir-callee-local-ssa-host-20261128.json)只修改固定免费Mesa的GLSL／NIR四文件，先降低函数局部变量，再在callee递归内联后整理SSA。准确Earth的release host链接阶段2.936→1.383秒，约52.9%减少；原shader长度／FNV匹配关系保持。before／after原NIR结构及支配关系4点检查、768个作者调用／3120字检查／48guard及131072个half／float32边界模式全字节对照通过。原LLVM14 host退出期失败仍保留，不作为完整host GL或真机通过。

Mac已用实际iPhoneOS SDK／ARM64只重编三个compiler objects，制作独立archive overlay；原deps／档案不改，SweetGPU／allocator／SCM／PON／RPC／VM冻结。[原r1 ARM64／guest候选](../reproducibility/nir-callee-local-ssa-ios-candidate-20261128.json)：GLES `dd3ffb371577fc1f158a59c722e5750db24c41511b2f2ac6732e35846bbc8887`／14801296B，RAM blob `30a5bf63ddfa56fb9587c4308c6eb92525616c69ac4d062ada88ce6cb1ff00b7`。CPU单发token `92b7017cc30e02bd`通过；GLES9 `999c9b10a80fc321`原拒绝见上节，没有该组合完整GLES／native／PNG／原UI准入。SSA compiler objects／archive可作新revision2前端链接输入；旧失败镜像不重发。详见BUILD，生产基线保持。

[完整新native候选](../reproducibility/nir-callee-full-native-candidate-20261128.json)已离线准备：Metal仍为19a08083…，new GLES dd3ffb37…；native CPU token `b4b87a99ca59d79e`、native21／PNG6 `53fec6c3efa6d843`均未投递。该observer只读inventory，不运行无缺失请求的primer。43→44项准入测试通过（旧缺输入2项skip）；实际GPU及原UI仍未验。

[实际编译输入投递前审计](../reproducibility/compiler-overlay-actual-audit-20261128.json)已补充4源、3 ARM64对象、24当前／原archive和overlay原manifest核对；Mac正例及source／object／archive／manifest破坏和错误CPU用例通过。live／offline使用同一检查，结果进入payload-audit，所有USB／COM操作之前执行。

## v732日志票号候选

[离线报告](../reproducibility/log-tickets-r1-offline-20261127.json)与[准确内核身份](../reproducibility/log-tickets-linked-candidate-20261127.json)绑定多producer票号队列：8个并发writer／16000条／256000原字节通过WSL／Mac ASan／UBSan及Mac TSan。暂停前缀只等待，不读取未发布字节；超出一整圈的旧writer不能覆盖新generation。旧共享writer锁移除，RPC回调尾部逐字节保持；logger ARM64 object只导入原有printf。

新kernel SHA `f1d485a4f0306bcef003783ea9b2338188a6c8b65a4300fb207acbb15e9b387b`，20681712 B；v731基线重链接除UUID外完全一致，仅替换logger.o，原1112项filelist及4个额外link objects均保持。静态DATA虚拟区增长3MiB，非空vm span低于32MiB；无allocator／VM／GPU／SCM／PON／RPC代码修改。新版本单独为732、86源，不能借731准入。

v732已单发private `cfc7bfafb0c3c165`核对当前VM／SMEM、F12及单次原scanout，180秒一次现有status查询读取log／dropped；随后新732 owner→ready→RAM自身CPU→GLES9→native21／PNG6也已完成。[v732完整实机报告](../reproducibility/v732-log-tickets-full-actual-20261127.json)已验private／owner／ready／GLES RAM CPU／GLES9／native RAM CPU／完整native21及PNG6，七轮都单发完整回程，每次180秒状态dropped0、tx=log。完整native token `1cfd34d92fe4f08d`，raw SHA `697d3124f5888bb8bf53292c41c4fcf83b0fa47b9fe06c6f7d72954f27755025`，回程38.453秒；[native21 proof](../reproducibility/v732-native21-proof-20261127.json)和[PNG6 proof](../reproducibility/v732-PNG6-proof-20261127.json)同一原流，缺失Stencil PAGE／PNG DATA已在新靴完整核对。旧失败不提升、不重发。当前手机安全fastboot，无在途启动。

## 原Aegir下一轮

新`build/aegir-log-tickets-r1-20261127`保持v732／r5 GLES／原反射Metal，short observer SHA `2ec05f92…`、RAM blob `c6aab7f4…`、manifest `48ca18e9…`，嵌入上述新完整native／PNG6证明。原request／renderer／paused／mode保持，旧cache不改删；原snapshot／queue退休、真实原PNG及CG输入仍按原返回观察。

先CPU `f1fd4ad7957b84a5`自身准入，再原GPU `02758c66f824e349`；GPU只在本LIVE／F12120秒一次既定上滑、150秒一次原scanout、180秒一次现有只读status。[原Aegir实机观察](../reproducibility/v732-aegir-earth-link-observation-20261127.json)已单发：CPU通过，回程38.156秒；GPU token02758c66f824e349为REVISE／EXTENSION_CHECKIN_TIMEOUT，原3431块扫描帧及39.844秒完整回程保留。原encoder4212星点draw返回只证明方法返回，尚无该原draw的GPU执行／像素证明；本靴在第三个aegir_earth_fsh pipeline中，VS／FS编译返回后glLinkProgram未返回。可靠sig9样本落在NIR inline_function／block split／reinsert；另两条短栈无唯一base锚点保持未解析。原包不重发，离线准确输入重放已完成，下一步核对现有身份的真实missing request／标准bin预热；原UI／壁纸、FPS、光学／睡眠及其它外设仍未完成。

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

r5四包已各单发一次，原结果保留：

| 顺序／目录 | token | 当前证据 |
| --- | --- | --- |
| CPU／cpu | `549746d4b51a7668` | 完整payload／85源及实机CPU通过，回程38.109秒 |
| GLES9／gles9 | `b5eea38cb400afa4` | 完整GLES9及3445块150秒scanout通过，回程39.172秒 |
| native CPU／native-cpu | `a0c42c1d43ab5734` | native RAM自身CPU通过，回程38.657秒 |
| native21／native21 | `2fb820a1d5198d03` | controller PASS，独立PNG缺块拒绝；无完整native／PNG准入 |

barrier revision4从当前linked fragment program的ImageUnits映射GL binding到dense Gallium槽位，按准确readonly／writeonly coherent RGBA32F资源检查。旧r4人为si[0]／si[8]布局会错走fallback，已用相同真实两槽输入复现，r4保留历史且不作为下一轮候选。新host直接编译锁定Mesa mapper核对255种mask／两种声明顺序，并生成1404次／29489字／117956 B连续CP流，11次模拟批次切换和9次guarded ring扩容通过ASan／UBSan。

优化仍保留每图元XR舍入、首次scratch64 GPU初始化、单调CACHE_FLUSH_TS／WAIT_REG_MEM／UCHE与final fence。异步GL、未知图像、其它stage image、写SSBO／atomic、深度或多样本保持原barrier。主机命令流／映射不证明实际A540缓存、像素、原壁纸或速度；完整native验收须逐项核对优化命中及重叠图元原页。

## 原壁纸与显示卡点

[原星点r5](../reproducibility/aegir-retirement-r5-gpu-refused-20261126.json)token `454044d0bb3177b8`已经进入后端call2：buffers2、count4212／ordered1404，停submit后签到超时3次；原REVISE及39.281秒回程保持。固定Mesa的fd_memory_barrier会fd_context_flush，支持继续验证上述提交优化。[ACK后ring sig11栈](../reproducibility/aegir-star-r5-post-ACK-ring-symbols-20261126.json)单列退出期，不归为本LIVE根因。更早大shader超时、primer缺SHA、旧黑PNG和缺scanout的原结果均保留，由[壁纸专题](METAL-REQUEST-OBSERVATION.md)索引。

原SnapshotManager已按准确库／source-map使用独立cache目录，旧目录不改删；原720×1280加载PNG全黑仍是实际失败证据。只读RBYTES／CGBYTES／queue retirement与无损OPNG只观察原返回，不造像素。尚无完整Earth／post／snapshot恢复证据，不强制解除Vista paused／mode或画替代UI。

[原生几何](DISPLAY-GEOMETRY.md)已核对original六行／fitted五行全部38应用、Dock顺序、Folder内容、平直状态栏及真实标签祖先／两处17.5点间距。完整safe area、真人全屏触控、应用启动和光学仍待验；串口合成上滑不代替真人。原scanout仅720×1280／BGR888／stride2160，每靴一次、45或150秒；缺块失败不补发。扫描帧不证明面板亮度、错行、FPS或睡眠。

## 其它外设

| 外设 | 当前实现／实际证据 | 仍待完成 |
| --- | --- | --- |
| WLED／背光 | v731首帧后只读fault0、mask3、duty1989／1989；非零属性／duty后端65535值及epoch／重入／写失败离线通过，service独立ARM64链接 | 实际注册／SPMI写入、亮度／灭屏／唤醒及光学 |
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

[当前非零背光候选](../reproducibility/backlight-nonzero-duty-backend-20261128.json)已接通可选service后端代码，构造不做硬件IO；成功要完整事务／读回／同epoch。缺实际owner／SPMI／注册，brightness0／fade／辅助level仍Unsupported，无面板调光或睡眠实测。kernel GPU／allocator／SCM／PON／RPC／VM冻结，新service未加入启动。

[背光最新逐IO epoch候选](../reproducibility/backlight-duty-per-io-epoch-20261128.json)已补上每次read／write／wait前后readiness／epoch检查；中途变化及时停止后续IO并保留uncertain。38／6／1故障位置、65535请求映射及重入回归通过，独立ARM64链接已验；没有新kernel投递、真实bus回调或光学，brightness0／fade／level保持待实现。GPU候选包及合法新会话待验条件保持。

[WLED真实SPMI依赖](../reproducibility/backlight-spmi-binding-audit-20261128.json)已核对锁定源码：observer样本不能准入write ownership。现有describe只返回映射数量，真实backend尚缺当前APID owner／unpoisoned／bounded callback验证。未重分配owner或改共享SPMI／PON源码；当前新GPU和WLED候选仍待恢复合法实机会话及完整回程条件。
