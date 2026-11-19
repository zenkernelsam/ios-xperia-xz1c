# 当前状态 — 2026-11-19

最新原GPUe662710b1d6a0bb4已完成39.172秒PON06四marker／连续健康30秒／worker退出；Sony BH905ND69G当前健康fastboot，无in-flight。新绘制观察GLES a7bad22c（只加原阶段计时）、v730/f258dee9与Metal5e94ab57／source-map3e79503f已取得自身GLES CPU／GPU、完整14组native／六组同源PNG及原observer CPU的独立准入，不能使用旧72d39da5的proof替代。完整结果见[RENDER-STAGES](RENDER-STAGES.md)与v730-render-stage-r1各记录。

三轮后台预热均完整14组通过，并实际编译同scope缺bin的原request：15521／5005B、4840／73444B、4840／9711B，link分别3.147／37.936／2.872秒。原失败／partial-builder失败均保留；最后五条原pipeline命中64421／64317／747773／17437／44221B binary并nonnull，约195／80／236／59／72ms返回，随后首个真实750×1624 draw进入。`targets`完整1.523800秒／线程CPU0.225859秒，后续buffer／sample约1.006／0.504ms；`submit`只有BEGIN，原extension仍check-in超时，未见本次draw返回或PNG入口，REVISE保持。不得把draw进入当原UI／FPS通过，也不把stage墙钟直接归为GPU硬件时间。下一步细分原两个附件的目标准备／clear与首次提交；不据退出采样修改冻结VM／SCM／PON／allocator。

本轮串口原scanout截图均完整复算；CPU靴power_off=0，原GPU45秒图仍power_off=1，仅用于缓冲区UI内容，不证明正常亮屏／LCD光学／睡眠。`render_stage_evidence.py`按原进程／pid／call核对阶段、退出期及缺失；最后r2报告保留submit pending并独立核验原PNG／BGR。正式工具锁已补齐v730截图／构建／行尾物化依赖，`validate_current.py`当前170个Python源语法／锁及v730增量通过。所有源码在正式仓库，旧raw和失败未覆盖。

以下保留v730前序截图／编码器与旧库比较事实，当前可准入的库及下一步以上述最新记录为准。

Sony BH905ND69G 当前健康停在 fastboot，无 controller／串口 worker／相机 worker。v730/f258dee9（20665112B、83 源）已通过自身 private、owner、ready、精确 GLES RAM CPU、两轮完整 GLES 八项／readonly 960000 像素与六张完整无损扫描帧，六轮均单 download/boot、PON06 四 marker、连续健康 30 秒和 worker 退出。最新 GPU token 5f97bb217a29d17a，38.750 秒完整回程，45 秒观察点的帧只请求一次并在最终取证复用；完整来源／块／记录／帧 CRC、原像素 FNV 和保存 PNG／BGR 均复算通过。固定 Apple base + 完整补丁 + v725..v730 已在新的 TEMP checkout 重建，83 源逐字节匹配；三个 LF/CRLF 物化规则已锁定，现行内核输入不变。详见 SCANOUT-SCREENSHOT 与对应 reproducibility 记录。

读屏分辨率为真实 720×1280、BGR888/stride2160、2764800B；本轮截帧约 1.7..10.9 秒。CPU private 图确有日期／时间、camera／homebar、红色提示和红字；两轮 GPU 图捕获时系统 power_off 请求均为 1，即使取证前移也保持原事实，不能据此判断正常亮屏时的时钟／壁纸或面板色彩。UI 内容可用已校验串口帧替代相机，背光／光学色偏／面板真正送帧仍另验。完整 UI/FPS、睡眠和其它外设继续 active。

v730 共享编码参考自身 RAM CPU4ea81eaf／38.140秒和 GPUd1a2778c／37.578秒完整回程通过，原八项 GLES／全部14组 native／六组原 UIImage PNG 与同源 ImageIO 策略均已独立复算。共享源61031999…的六组 UP 原16位像素、ICC、metadata一致；作者混色7.009→2.253秒、渐变4.459→1.098秒，不代表原 UI 时耗。新 v730 native／PNG proof 已生成，旧 v729 camera STOP保持拒绝。原编码库becbb403（113200B）host组合／参数／nil／异常／errno、像素／ICC／metadata检查及自身RAM CPUe9731f67／37.829秒通过。

原 Aegir 编码器 GPU75bd2dd959bcdac5 已单发并完整回程40.406秒，目前健康fastboot／无worker／无in-flight。编码器在ACK前pid276实际安装并绑定d1a2778c原先验；五条原pipeline均nonnull，Clear／纹理／sampler绑定正常，但第一次原snapshot等待执行队列时扩展仍check-in timeout，原draw返回及PNG入口均未观察到，结果REVISE保持。当前实际slide0x3324000的独立栈落在 `___psynch_cvwait`→CLKUIMetalQuadView snapshot等待，只是退出位置采样，不据此判定GPU／VM故障；不把编码器已安装当提速成功。下一步分解原首次draw的提交／等待／写回耗时，保留原参数／位型／期限，继续原UI/FPS及外设目标。完整原失败、postdraw和符号栈见v730-png-encoder-r1-original各记录。

此前 v729 的共享 ImageIO/UP 参考记录：CPU49625fe2/37.812秒通过；GPU110c23cd8f167e3c六组原 PNG／源码 SHA61031999…数据返回，但相机拒绝连接而记 STOP/CAMERA_EVIDENCE_MISSING，38.250秒完整回程。此 STOP 保留、源码先验拒绝，未投递原 Aegir 新编码路径。原 host ASan/UBSan、RGB16 PNG／像素／ICC／metadata、nil/throw/errno／回退检查及 reference3 完整14组／六组 filter 先验仍保留，均不能替代新的同 kernel 准入。

## 当前新GLES验证

GLES72d39da52e5673755b9419b2038b5db5b876e073fe96b52cae20f4c5944c72b4，Metal5e94ab57及其source-map3e79503f保持。独立GLES八项及完整14组native4bcd571c9e12b34d通过；实际四种XR Clear跳过旧色解码，原Clear/位型/padding/mask/fence/lifecycle及两个Load解码保持。实际原界面单样本数据和不完整UI见顶段与XR-CLEAR，不能以旧GLES proof替代。

## 原比较基线（0a2c7e69）

- kernel v729：774b5efd5f715dc40d81e3c207c85b9b690c83fe44f4fd9061a2189ef7db71ec，20648064B、81源；allocator/SCM/PON/RPC/VM冻结。
- GLES：0a2c7e698ae59a1afe8608c884f59e87a344a3952e9b4d52899923b9e295b3db；固定Mesa24.3.4/FD540，build identity51e6ba632cbac469f3210833ea01f201305b5ca76f7c37bef04504a1c6c9b22d。
- Metal half-selection-r1：5e94ab57401479b42f506884a172a7a839378a82310661a1f66164e99c21f988；source-map3e79503f6f2856fca12eb3a2095a9d4631098e9c0be4f92eda876abe20f1457b。
- 完整14组native token45a839fc4c105e01，rawf9b13434f7207510434b8d0b73d2522883ae07f8b38336788da234d3964da7c6，39.062秒完整回程/健康30秒/worker退出。half131072B、MRT1245184B、sampler147456B、static-half196608B及全部guard/fence/lifecycle保持，三次fresh binary恢复。见[完整proof](../reproducibility/v729-half-selection-r1-native-proof.json)。

## 原GPU和新卡点

snapshot-r1 pid290的完整原snapshot为3.307421094秒，其中wait2.806619062秒、execute2.808915156秒/线程CPU0.604673906秒、read0.021221823秒；随后原PNG为3.910680521秒/线程CPU3.895532344秒并返回32179B。管线创建约31..417毫秒。wait/execute重叠，snapshot包含其子路径，不可相加当独立工作；PNG包含ICC及编码。八类方法的实际覆盖和早期失败完整保留，下一步聚焦队列/渲染与PNG/ICC，不据此推定冻结VM故障。见[本轮结果](../reproducibility/v729-snapshot-r1-gpu-revise.json)。

postdraw-r1又出现两次750×1624原GPU draw result0、sequence612/663、original_pages1。两个可见ColorSyncTransformCreate调用分别437604ns/405625ns，原PNG外层各未返回；addImage/finalize内部调用没有经过该标准绑定。新的实际slide0x26fa4000栈显示：第一进程在CLKUIMetalQuadView快照等待，另两进程仍在ImageIO内部CMM profile/转换序列初始化。不能拿可见0.4ms调用排除未被观察的内部ICC开销，也不能直接把PNG过滤器当成已验证修复。下一步覆盖原pipeline/快照/readback/wait及进入PNG前的预算。见[本轮原结果](../reproducibility/v729-postdraw-r1-gpu-revise.json)、[时耗](../reproducibility/v729-postdraw-r1-gpu-observation.json)及[当前靴栈](../reproducibility/v729-postdraw-r1-cache-crashes.json)。

原Aegir r2 token6d17bdf97a8fc7c8、raw98bcc37136ebaf6fce95b5a0529152c695bf94d20bf2f174bb806ccc59443e4a，39.578秒四回程marker/健康30秒/worker exit1。五条真实nonnull管线与星空/云层/地球等cache hit保持，750×1624原GPU draw两次result0、sequence619/678、original_pages1。面板仍只有暗背景、camera、homebar和电池，完整壁纸/时间日期/UI未完成；原extension check-in timeout三次与REVISE保留。

该靴三个ACK前sig9栈用实际shared-cache slide0x3c5bc000解析：pid285在malloc free_large→mach_vm_deallocate；pid288在ColorSync/CMM profile transform→ImageIO PNG writer；pid291在PNG deflate/filter→CGImageDestinationFinalize→UIImagePNGRepresentation。只是退出时的位置采样，不据此认定VM、CMM或PNG故障，不改变冻结组件。原ACK后退出期sig11另列。见[原栈](../reproducibility/original-aegir-r2-postdraw-stack.json)和[完整symbol map来源分析](../reproducibility/original-aegir-r2-postdraw-analysis.json)。

[postdraw-r1](POSTDRAW-OBSERVATION.md)新observer b88aec05a5aefec2bca6cafb381773f8b90f3392e07b2a8fae85d8ce6e1884cc只观察原PNG/addImage/finalize/ColorSync入口与返回。Mac ASan/UBSan验证原参数/返回/nil/throw/errno、嵌套和64条日志界；实际sRGB、DisplayP3及750×1624 ImageIO编码字节、解码像素、ICC/metadata和原输入保持。目标arm64 ABI检查与64B/four-pair interpose section已通过。这些仍是host/编译结果，本包CPU及原手机时耗须独立记录，不改原失败或check-in期限。

## 链接优化和实际预热

half-selection-r1的全部4294967296种float32/12884901888比较与65536 unpack零差异/UBSan；Mac同SHA target ASan/UBSan及131072个host GLES样本通过。WSL ASan启动失败单独保留SHA/样本/原生回收位置，不冒充通过。原Earth两份真实shader主机旧版各60秒timeout，新候选6.934/2.096秒，不能推成手机时耗或UI成功。见[HALF-SELECTION](HALF-SELECTION.md)。

原Aegir r1 token478fb7bafbe5df03仍REVISE：星空实际cold link1.565914秒/64421B，云层1.888484秒/64317B，返回nonnull；原check-in timeout三次，39.047秒完整回程。后续primer8abc57d8已完整14组和三个真实request匹配：FS26549/38924/73444分别链接13.039/16.140/39.007秒，保存227981/375861/747773B标准binary，38.532秒完整回程。按新scope334819af只读本机实际缺bin请求，旧cache保持。

request-export-r2 token3fa3a81100888cb9已完整接回两个actual request共142493B/4454段，record/payload/key/identity及生产XR wrapper长度/FNV匹配原靴，完整14组与回程通过。原r1长行截断及后处理拒绝保留。原Apple shader/request只在TEMP，Git保留工具和摘要。[PROGRAM-CACHE](PROGRAM-CACHE.md)及对应reproducibility记录说明各边界。

## 未完成与操作

完整锁屏时间/日期/壁纸、系统UI/FPS、睡眠唤醒、声音、Wi-Fi/蓝牙及广义texture/depth/stencil/MSAA仍未完成，goal保持active。离屏、主机、缓存、draw或控制器PASS不能替代真实系统与面板。

只BH905ND69G；fastboot带-s；COM≥10且VID1209/PID0001；PON只0x06；单download/boot、失败不重发；四回程marker、健康30秒、旧worker退出后下一轮。一个controller持COM/camera，相机override→AF→2秒→MJPEG每5帧抽1→rotate90。gl5/userdata/SD/TA/XFL与救砖输入受保护；Mac旧自动构建器pause保持。新修补只在正式仓库，git log和reproducibility记录发布/实际输入。

旧static-half/be56、旧cold link 240秒失败、旧partial CPU和当时状态完整保存在[本轮原文归档](archive/20261116/STATUS-before-postdraw-r1.md)及各原失败记录。文档整合没有覆盖原日志/结果，也没有永久删除文件。
