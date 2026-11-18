# 当前状态 — 2026-11-17

Sony BH905ND69G当前安全停在fastboot，无controller/串口worker。共享ImageIO/UP编码函数已在正式源码，调用分派与postdraw组合的host ASan/UBSan、real RGB16 PNG/像素/ICC/metadata、原nil/throw/errno与不支持输入回退检查通过。新共享参考CPU49625fe2/37.812秒通过；GPU110c23cd8f167e3c的六组原PNG/源码SHA61031999…/ImageIO策略数据已返回，但因DroidCam拒绝连接缺相机证据记STOP/CAMERA_EVIDENCE_MISSING，38.250秒完整回程。此STOP保留，不准入新PNG encoder；源码先验工具确认拒绝，未投递原Aegir新编码路径。用户表示稍后开启服务。既有reference3完整14组/六组filter先验及原Aegir超时保持，kernel774b5efd/GLES72d39da5/Metal5e94ab57不变。当前继续离线检查；相机与新共享源码完整准入恢复后再新包CPU→原GPU/面板，不重复旧STOP bundle。完整UI/FPS/其它外设目标仍未完成。

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
