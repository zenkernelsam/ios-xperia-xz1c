# 当前状态 — 2026-11-15

Sony BH905ND69G 已完成 sampler-r1 registry-r7，健康fastboot，无in-flight。r7 token5a74bb175409f927、raw662ccb2ca942e834036b940deb4453c8c44d9c7d6d57d7e0d1de54ac473f2441，38.484秒完整回程。原vertex/fragment compile约45–96ms成功，glLinkProgram未在原check-in期限前返回，REVISE保留；精准卡点是link。r7 CPU tokenfb388608de24d74f完整38.188秒回程。

历史registry-short-r6：token b41991ebc88e302b，358260B raw SHA a0eb1b78d888649ca6e339ed3fdde4ada6b9093fb75a192d5d0719056e3316c4；原 starfield 首个 pipeline 未返回，Aegir check-in timeout，原 REVISE 保留。38.579 秒完整 ACK/DRAIN/RESET/FINAL_DRAIN＋连续健康30秒，单 download/boot，原 worker 已退出。相机 MJPEG 成功采到本轮画面，尚未把它作为完整 UI 通过。

r6 CPU token 9b34249a82fa2d1f，358745B raw SHA 7b746ce0ec6e501513203027743c47c38916109a3764a25008a56128cf49a6dd；37.25 秒完整回程。串口主动 replay 形成两个完全相同的2938B READY→库读片段；新独立验收要求原raw、明确replay标记、唯一kernel banner和相同片段，并保留两条原读记录，已重核通过。首次 WSL bash 路径错误、GPU预检遇另一fastboot设备均在任何download/boot前停止，原记录保留。

正在直接适配开源 sampler 语义及冷编译路径。RenderSamplerRules/生产绑定器已支持AIR constexpr逻辑8..23、普通sampler、LOD/寻址/边界颜色、同一literal状态一致性及避开内部8..16纹理单元；host实际GLES/ASan/UBSan 25检查、11精确像素通过。新GLES 1c904beb及新Metal已编译；新GLES CPU token48fcf66d763eec3d完整37.532秒回程。GLES GPU token2499ad4d57eca2b7完整37.891秒回程；新原生包CPU tokenc49f90b6a9836e67完整37.000秒回程。新Metal后台GPU token2df735bb72466378已完整通过，所有原native组、sampler147456B/1881pixels、half131072B及MRT1245184B复核通过，38.719秒完整回程，旧worker已退出。原Aegir r7 CPU/GPU均已完成，原GPU REVISE/链接卡点保留；无in-flight。普通mirror-clamp元数据同步，精确half舍入使用4095＋保留低位的整数偏置，host全部131072输入通过，原starfield冷链接1.991→1.122秒。normal-half native packing实验冷链接14.212秒已撤回，未发手机。下一完善原AIR sampler后台硬件探针并重建同源包，按精确新GLES CPU/GPU、新Metal包CPU/完整native GPU→原Aegir顺序验收。

控制器现在选原生Git Bash；针对已授权Sony选择，ABL guard要求可读BH905ND69G USB身份及固定二进制SHA，允许其它设备保持连接。没有放宽单次boot或0x06回程门禁。

当前已验基线：kernel v729 774b5efd5f715dc40d81e3c207c85b9b690c83fe44f4fd9061a2189ef7db71ec，20648064B及81源；GLES 1c904bebb1a307d3b5ba55e3406208ec2b8dba854dd736f8a6dca78259e3fd1c（旧a463基线保留）；Metal b0b8045bc8b3a8f46e4c67643d3f113d36b8a66b04e0ae4e62dab758e4615636（新完整12组已通过，旧28e6基线保留）。此前half-link-r1完整后台native GPU、131072B half含NaN/保护区、1245184B MRT/XR及其它全部组保持，404全released/unmapped、active/orphan/root0，37.281秒完整回程。新源码候选不替换该已验基线。

完整锁屏时间/日期/壁纸、系统UI/FPS、睡眠唤醒、声音、Wi-Fi/蓝牙、更多Metal纹理/depth/stencil/MSAA仍未完成，完整目标active。离屏、主机与真实面板证据分别验，不把shader编译或控制器PASS当整机完成。

操作继续只BH905ND69G、fastboot -s、COM>=10/VID1209/PID0001、PON只06、单download/boot/失败不重发，完整四回程标记/健康30秒/旧worker退出。gl5/userdata/SD/TA/XFL/救砖输入受保护。相机override→AF→2秒→MJPEG每5帧抽1→rotate90，一个controller。

GPU专题及原结果见 METAL-BACKBOARD/METAL-REGISTRY-SHORT/AIR-HALF/AIR-NARROW/METAL-MRT；本次同行来源见MSM8998-PEER-SURVEY-2026-11-15.md。旧STATUS原文在docs/archive/20261115/STATUS-before-sampler-reuse.md与更早archive，原失败/raw不改写。

下一离线对照固定Mesa GLSL/NIR链接与IR3路径，定位具体耗时或锁等待；保持已验sampler/half/MRT字节与内核冻结，修改用户态/Mesa后沿完整准入继续。

2026-11-15 cache-r1候选：既有3个Aegir崩溃栈已按同SHA库和唯一glLinkProgram返回点符号化，均处于prelink/gl_nir_opts（nir_opt_access/algebraic/find_array_copies），未见IR3阶段。固定Mesa禁用shader-cache后driver SHA空实现/未初始化；新补丁引入实际构建/上下文身份。新私有程序缓存按实际source/config/digest校验，原文件不覆盖不删除，缺失请求供backboard预热；host128pixel/576guard bytes新上下文程序二进制往返通过。iOS Mesa变体与GLES已编译，目标缓存GPU证明尚待新库完整CPU/GPU/13组；目前不发原壁纸缓存准入。
