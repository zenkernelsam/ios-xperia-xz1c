# 当前状态 — 2026-11-15

当前half-link-r1优化包完整native GPU已通过，token236ebb8e9a66435b，479968B raw SHA3322990b0cc465701fc9be75b4341663bb3b50d97952aa5cdd5b45d8a3538db8；37.281秒完整PON06回程/健康30秒/原worker退出，Sony健康fastboot无in-flight。GLESa4632b32保持，新Metal28e6a132。原全部native组、half131072B含NaN/保护区与MRT1245184B通过，404allocated/released/unmapped、active/orphan/root0。新half解包findMSB及直接位型舍入主机131072用例与实际AIR验证通过；原星空主体不变，无缓存host链接5.267→1.991秒。正在构建绑定新完整proof的registry-short-r6；r5原check-in超时保留，原UI/FPS/睡眠/声音/无线继续active。

同一新GLESa4632b32的CPU/GPU及Metal包CPU分别37.828/38.531/37.531秒完整回程。共享RenderUniformRules在生产区分真实sampler与内部uniform/image，类型/位置检查及实际主机枚举/错误image绑定拒绝通过；此次完整GPU补上了r2遗漏的生产绑定器验收。

mrt-r2完整Metal GPU原REVISE并39.891秒完整回程保留，token402f3ed8ec46f88a。真实caps为8目标、FP32 render/image=1、blend/EXT_float_blend/fbfetch=0、无附件FBO=1、fragment images=24/image units=192。XR552清屏65536B通过，首次混色返回Darwin -45/ENOTSUP，未派发draw；原bind_samples拒绝新增内部image/uniform，r3已修订。全部原native组先前通过，r2不提升完整MRT/UI通过。详见METAL-MRT.md。

同轮新GLES43ed6ab8的CPU/GPU及Metal2afbde84+新observer包CPU单发分别通过，token6d0d914413af4b4d/402b719e0aa9cc52/fedd266853ff5966，37.000/37.390/37.968秒完整回程；GLES八项、960000只读像素与203全部released/unmapped保持。主机四XR/八附件/遮罩/原低位及四重叠图元、两个实例、非零顶点起点、负色/大于1色值、原生对象ASan/UBSan通过；主机测试未覆盖真机生产绑定器的这一拒绝。

mrt-r1的tokencaf703b6a68559d4、原REVISE/BACKGROUND_METAL_SUITE_FAILED保留。XR552双附件清屏65536B通过，第一混色phase返回-EIO且未记原GL错误码；40.032秒完整PON06回程。固定A5xx screen未暴露PIPE_BIND_BLENDABLE/EXT_float_blend，且无framebuffer fetch；r2增加实际能力和逐阶段GL错误日志，不提升r1为通过。新AIR dynamic8/16转换和half存储/运算/顶点片元已完整后台GPU验收；原Aegir越过两处编译错误，threshold管线实机编译成功。完整时间/日期/壁纸/UI/FPS及睡眠/声音/Wi-Fi/蓝牙仍未完成，完整目标active。

固定内核v729 SHA774b5efd5f715dc40d81e3c207c85b9b690c83fe44f4fd9061a2189ef7db71ec（20648064B）及81源实际审计保持；当前GLESa4632b328945416bc0db01e4f6fa0f18cf12adc25d40d4918a6338dcebe009ed（14729008B）、Metal2afbde84976310911b2274577d0dbbceef5455143f3630d48f407af6402247a9。原allocator/SCM/PON/RPC/VM不改；旧half基线与原失败输入仍保留。

[half验收](AIR-HALF.md)：air-half-r1的1完整/0缺项、两phase compute与half vertex/fragment，131072B全部原byte/input/padding、有限half bits精确，NaN位置/类别/raw payload分别复算。303allocated/released/unmapped、active/orphan/root0，38.282秒完整PON06回程。主机全部65536half patterns及65536float32half-ties/边界/random，以及实际AIR half buffer/graphics分别通过；主机不冒充手机。

[窄整数验收](AIR-NARROW.md)：air-narrow-r1 dynamic signed/unsigned8/16 scalar/vector64invocation×2phase，98304B全部原结果/input/保护区零差异；423全released/unmapped、active/orphan/root0，38.469秒完整回程。原aegir_pass_vsh曾因动态short2转换被拒绝，原shader在主机transform feedback的位置/UV/uniform偏移通过。独立适配patch仅应用构建副本，固定开源参考和MIT/BSD声明完整。

[短登记](METAL-REGISTRY-SHORT.md)：不在Aegir ctor重跑长GPU组；派发前native_prerequisite重新验实际backboard完整raw而非manifest摘要，嵌入kernel/GLES/Metal/source-map/token/image/logSHA严格全匹配。新窄整数/half库要求额外自身GPU验证。registry-short-r1/r2/r3原登记成功及原错误保持，r2动态short2错误、r3half扩展错误各有完整原栈和pipeline metadata。

registry-short-r4原threshold的aegir_pass_vsh+aegir_threshold_fsh实机pipeline返回nonnull，linked=1 robust=1，使用原half/shader成功。原Aegir请求375×812/3mip Private R8。其它XR管线在当时单附件/无XR混色后端返回nil；当前r3多附件与混色已通过须再验证原应用，不补写r4成功。原REVISE/EXTENSION_CHECKIN_TIMEOUT保留：token58ea25e5de40fb92，镜像d54783e2d5c22b59a5dd3802213b76ee9369e3e9c48855f05d173d7318002c23，439514B日志SHAfcba01a6f35cdaf1ad6b8b9078c0ba8f1bbbf98b8297d8eb9fb74d869e16de9c；37.453秒完整回程，364allocated/released、363unmapped、active/orphan/root0，不称364全部map/unmap。这不是完整UI通过。

全部原GLES/CA lifecycle/960000只读像素及Metal compute/render/constants/queue/XR4format/Surface/mip/narrow/half/MRT完整backend门槛保持。原native system登记只在Aegir，开机全系统plugin尚未发布。广义half/math、奇数尺寸/其它mip级数/Surface mip、更多blend因素与constant sampler、其它图元/array/cube/YUV/depth/stencil/MSAA和实际壁纸/完整UI/外设继续推进。

下一以当前完整native proof准入原Aegir短登记，记录真实各颜色附件/函数/原错误，再验原壁纸/时间日期/完整UI。保持只BH905ND69G、fastboot -s、COM>=10且VID1209/PID0001、PON只06、单download/boot、失败不重发、ACK/DRAIN/RESET/FINAL_DRAIN、连续健康30秒和旧worker结束；gl5/userdata/SD/TA/XFL/备份不重刷。

相机override→AF→2秒→MJPEG→每5帧抽1→rotate90；已转照片不再转，grab_cam.py禁用，一个controller。原源码/复现工具/实际证据在正式仓库，大库/镜像/完整raw本地，原系统shader只保留本地并提供提取hash配方。

历史状态在docs/archive/20261115与更早archive；本轮重复HFS/媒体/传输按原生回收站回收，原raw和最佳照片、失败镜像/当前压缩包保留，不清空Windows/Mac回收站。
