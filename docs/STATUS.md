# 当前状态 — 2026-11-15

Sony BH905ND69G健康fastboot，无controller/COM/camera/in-flight。新AIR dynamic8/16转换和half存储/运算/顶点片元已完整后台GPU验收；原Aegir越过两处编译错误，threshold管线实机编译成功，星空/地球/post仍需多颜色附件和XR混色，随后原extension check-in timeout。完整时间/日期/壁纸/UI/FPS及睡眠/声音/Wi-Fi/蓝牙仍未完成，完整目标active。

固定内核v729 SHA774b5efd5f715dc40d81e3c207c85b9b690c83fe44f4fd9061a2189ef7db71ec（20648064B）及81源实际审计保持；GLES499bc6e9ddcf8143a0cc5578a3b8dd9c6ce3396f660917b7ac5655aaedf25cf3（14711888B），当前Metal3b4a4082b5d5b2cc4ff93197451b1ad4d7dba07a42405cea00c2900ef54cd9e8。原allocator/SCM/PON/RPC/VM不改。

[half验收](AIR-HALF.md)：air-half-r1的1完整/0缺项、两phase compute与half vertex/fragment，131072B全部原byte/input/padding、有限half bits精确，NaN位置/类别/raw payload分别复算。303allocated/released/unmapped、active/orphan/root0，38.282秒完整PON06回程。主机全部65536half patterns及65536float32half-ties/边界/random，以及实际AIR half buffer/graphics分别通过；主机不冒充手机。

[窄整数验收](AIR-NARROW.md)：air-narrow-r1 dynamic signed/unsigned8/16 scalar/vector64invocation×2phase，98304B全部原结果/input/保护区零差异；423全released/unmapped、active/orphan/root0，38.469秒完整回程。原aegir_pass_vsh曾因动态short2转换被拒绝，原shader在主机transform feedback的位置/UV/uniform偏移通过。独立适配patch仅应用构建副本，固定开源参考和MIT/BSD声明完整。

[短登记](METAL-REGISTRY-SHORT.md)：不在Aegir ctor重跑长GPU组；派发前native_prerequisite重新验实际backboard完整raw而非manifest摘要，嵌入kernel/GLES/Metal/source-map/token/image/logSHA严格全匹配。新窄整数/half库要求额外自身GPU验证。registry-short-r1/r2/r3原登记成功及原错误保持，r2动态short2错误、r3half扩展错误各有完整原栈和pipeline metadata。

registry-short-r4原threshold的aegir_pass_vsh+aegir_threshold_fsh实机pipeline返回nonnull，linked=1 robust=1，使用原half/shader成功。原Aegir请求375×812/3mip Private R8用于下一pass。其它原XR pipeline返回nil，starfield实际AIR有两个render_target输出而backend现只接单attachment；star/earth/post实际要求blend，XR仍需float shadow混色。原REVISE/EXTENSION_CHECKIN_TIMEOUT保留：token58ea25e5de40fb92，镜像d54783e2d5c22b59a5dd3802213b76ee9369e3e9c48855f05d173d7318002c23，439514B日志SHAfcba01a6f35cdaf1ad6b8b9078c0ba8f1bbbf98b8297d8eb9fb74d869e16de9c；37.453秒完整回程，364allocated/released、363unmapped、active/orphan/root0，不称364全部map/unmap。这不是完整UI通过。

全部原GLES/CA lifecycle/960000只读像素及Metal compute/render/constants/queue/XR4format/Surface/mip完整backend门槛保持。原native system登记只在Aegir，开机全系统plugin尚未发布。广义half/math、奇数尺寸/其它mip级数/Surface mip、多附件/constant sampler/XR混色、array/cube/YUV/depth/stencil/MSAA和实际壁纸/完整UI/外设继续推进。

下一优先实现原multi-target输出与XR混色，分别实际GPU原字节/保护区/fence/释放验证，再当前完整native proof准入短登记和原壁纸。保持只BH905ND69G、fastboot -s、COM>=10且VID1209/PID0001、PON只06、单download/boot、失败不重发、ACK/DRAIN/RESET/FINAL_DRAIN、连续健康30秒和旧worker结束；gl5/userdata/SD/TA/XFL/备份不重刷。

相机override→AF→2秒→MJPEG→每5帧抽1→rotate90；已转照片不再转，grab_cam.py禁用，一个controller。原源码/复现工具/实际证据在正式仓库，大库/镜像/完整raw本地，原系统shader只保留本地并提供提取hash配方。

历史状态在docs/archive/20261115与更早archive；本轮重复HFS/媒体/传输按原生回收站回收，原raw和最佳照片、失败镜像/当前压缩包保留，不清空Windows/Mac回收站。
