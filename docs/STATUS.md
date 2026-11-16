# 当前状态 — 2026-11-16

Sony BH905ND69G 当前唯一half-selection-r1新包CPU controller in-flight。token `f00d2dbec95bd5bf`、image `0de979b174246125a4f26c49e9c5745ec50a78af977b3b2698104d3b8ad2b2d0`，目录Windows TEMP/lilac-v729-half-selection-r1-native-ramcpu-20261116。须恢复同一worker，完整回程/健康/退出后才发对应新GPU包。新Metal `5e94ab57401479b42f506884a172a7a839378a82310661a1f66164e99c21f988`、observer607bbb63、source-map3e79503f，kernel774b5efd/GLES0a2c7e69保持。

候选[HALF-SELECTION](HALF-SELECTION.md)已做全4294967296种float32、12884901888比较（pack/round等价和round幂等）与65536种unpack，零差异/UBSan；Mac同SHA target ASan/UBSan/65536 literal位型通过，主机GLES131072样本通过。WSL ASan启动失败原日志已原生回收并保留SHA/样本/回收位置，不冒充其通过。新source scope334819af仅创建新shared sticky缓存目录，旧目录/文件保持。GPU完整14组尚待本包CPU后验收，旧be56proof不准入新库。

request-export-r2 token `3fa3a81100888cb9`已完整14组GPU、4454段/142493B真实request、四回程marker、健康30秒及worker exit0通过。raw `4f48814e5e8ca4b53898838a6217acaa99a7218e532fb00133a17d4a50534e2b`。生产XR包装长度/FNV精确匹配原靴；实际原Earth主机旧版两个60秒timeout，新候选6.934/2.096秒。原Apple源码/request只在TEMP；这些不代表手机链接或完整UI成功。r1原长行截断/后处理拒绝及原raw/results/stdout保持。

## 已验基线

- kernel v729 `774b5efd5f715dc40d81e3c207c85b9b690c83fe44f4fd9061a2189ef7db71ec`，20648064B、81源，allocator/SCM/PON/RPC/VM冻结。
- GLES `0a2c7e698ae59a1afe8608c884f59e87a344a3952e9b4d52899923b9e295b3db`，固定Mesa24.3.4/FD540、实际build identity `51e6ba632cbac469f3210833ea01f201305b5ca76f7c37bef04504a1c6c9b22d`。
- Metal static-half-r1 `be56ad130f5a94bb0a1ec4bfabefab52b492a58d1b566222efe7384cac1ae749`；[完整14组proof](../reproducibility/v729-static-half-r1-native-proof.json)，token `58294c8e0de3c139`、raw `07fb2c39f2cef68a6be6dd0ec5054b0780d7a2e1fa51b2ef386315b92003164a`、38.860秒完整回程及worker退出。
- static-half12case/196608B全部原字节、NaN位型、前尾guard/fence及创建后reset的snapshot零差异；原half131072B、MRT1245184B、sampler147456B和其它原组保持。新静态half库旧13组proof拒绝。
- 原Aegir已真正恢复星空89821B、云层84333B和threshold18929B缓存并返回nonnull；见 [PROGRAM-CACHE](PROGRAM-CACHE.md)。缓存成功仍不代表原draw/present或完整UI。

## 原界面卡点

static-half-r1原Aegir CPU token `07450e3a3feff574` 已通过。原GPU token `ea6e7319b920504a`、raw `14ab7641f2b09ed6dbbc647f7ad644350780bf5a87b2a8c5c8eb4c1f8bdb2df0`，40.343秒完整回程，原REVISE保留。half/i16静态常量类型错误已越过，实际地球VS/FS编译成功，XR wrapper后为4879B、89561B/49198B；冷链接跨过原check-in期限。

完整后台primer token `6046e6c880760e21` 随后处理真实VS4840B/FS86136B请求：VS29.994ms、FS651.202ms编译成功，但240秒观察期内链接未返回。40.203秒完整自动回程，原STOP/OBSERVATION_INCOMPLETE保持，[原失败](../reproducibility/v729-static-half-r1-prime-gpu-failed.json)，raw `774d3d67d29da52af22bafbcb919a6fdcbc338d7ff7aac469093b9d3a308471b`。不能把这个CPU链接耗时问题记成RPM硬件死锁。

主机代表shader约1300次half舍入调用，正在比较等位型的无分支整数选择候选。全部4294967296种float32输入的pack/round共8589934592比较，加65536种half unpack均零差异；无sanitizer主机GLES的131072样本通过。WSL的ASan/Mesa进程另有启动崩溃，原失败保留；这些仅为离线候选结果，当前生产HalfESSL和Metal基线尚未更换，仍需真实request耗时和新库14组硬件证明。

完整锁屏时间/日期/壁纸、系统UI/FPS、睡眠唤醒、声音、Wi-Fi/蓝牙及广义texture/depth/stencil/MSAA仍未完成，goal保持active。离屏、主机、缓存或控制器PASS不能替代真实系统与面板。

## 操作与交接

只BH905ND69G；fastboot带-s；COM≥10且VID1209/PID0001；PON只0x06；单download/boot、失败不重发；四回程marker、健康30秒、旧worker退出后下一轮。一个controller持COM/camera，相机override→AF→2秒→MJPEG每5帧抽1→rotate90。gl5/userdata/SD/TA/XFL与救砖输入受保护。Mac旧自动构建器pause保持。

唯一正式源码在ios-xperia-xz1c。Windows/Mac已发布HEAD为`6a4d10f03136278455eb891ec63970f63d91711f`；本轮诊断源码/失败与新的实际结果正在正式仓库维护，完成该轮后提交发布。原完整状态保存在 [归档](archive/20261116/STATUS-before-earth-request-export.md)，更早版本与原失败仍保留。
