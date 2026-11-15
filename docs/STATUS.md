# 当前状态 — 2026-11-16

Sony BH905ND69G 已完成 cache-r2 native GPU，健康fastboot，无in-flight。token6dfc08178ea3da43，raw71901fab8f0f1e2b0365af3c49650ead9ed7ea24ee0f4b12fb280a68e813e238，38.984秒完整四marker/健康30秒/原worker退出。13组原生GPU全部通过；3个fresh pipeline导出state2并恢复state1，sampler147456B/1881pixels原字节与保护区零差异，sequences919/925/929；half131072B和MRT1245184B及其它原组保持。新精确完整proof见reproducibility/v729-cache-r2-native-proof.json。新GLES CPU/GPU完整通过；native CPU独立后处理原失败/10项回放测试和原日志保持，已严格原byte复验、未重发，见ramcpu-partial-replay-r1.json。

cache-r1 原生 GPU 轮已结束，token 57f8b2559214d870，452060B raw SHA c2ded7ab2ab2a6a9d8b05298e2a20070d6947e9bbb1f943425cc11a2953cd16c，39.156 秒完整 ACK/DRAIN/RESET/FINAL_DRAIN、健康30秒、原worker退出。原生 sampler 的程序缓存 first_state=0/restored_state=0，原 REVISE 保留，未准入原壁纸。此前这包 CPU token9e87cbca22c75411已通过，37.172秒完整回程。

cache-r1 精确 GLES e30e7a933497c63f2938f04fa866cc963132c0601bf4fcf9a17e747ecce67a7c 的 CPU/GPU 已通过；原生资源、queue、compute/render/constants、XR、Surface、mipmap、narrow、half131072B和MRT1245184B仍通过，但新增缓存验收失败，不能当完整13组成功。失败证据见 reproducibility/v729-cache-r1-native-gpu-failed.json，CPU证据见同目录 v729-cache-r1-native-ramcpu-actual.json。

已定位 Mesa 24.3.4 st_init_limits：禁用 shader-cache 后 get_disk_shader_cache 返回空，NumProgramBinaryFormats 为0，所以没有导出程序二进制。cache-r2候选仅对真实FD540且提供不可变driver identity的后端启用标准程序二进制；冷链接前设置 retrievable hint，补齐格式/长度拒绝日志。实际构建身份、原source/config和payload摘要均参与校验；旧文件不覆盖不删除。Mac独立新Mesa/前端及native包已构建，新GLES0a2c7e698ae59a1afe8608c884f59e87a344a3952e9b4d52899923b9e295b3db、build identity51e6ba632cbac469f3210833ea01f201305b5ca76f7c37bef04504a1c6c9b22d。精确新GLES CPU/GPU、本包CPU和完整13组native GPU均已通过，下一新proof→原Aegir本包CPU/单发GPU，不能跳过硬件准入。

原 Aegir r7 token5a74bb175409f927仍为链接卡点：原vertex/fragment编译约45–96ms成功，glLinkProgram未在原check-in期限前返回。三条同SHA库的符号化栈均在prelink/gl_nir_opts（nir_opt_access/algebraic/find_array_copies），尚未见IR3。固定基址/返回点验证见 reproducibility/original-link-nir-stack-r1.json；原shader仅本地保留。后台预热必须使用实际设备写出的request，不能用长度/摘要不同的host shader替代。

已验 sampler-r1 基线保持：kernel v729 774b5efd5f715dc40d81e3c207c85b9b690c83fe44f4fd9061a2189ef7db71ec（20648064B、81源）；GLES 1c904bebb1a307d3b5ba55e3406208ec2b8dba854dd736f8a6dca78259e3fd1c；Metal b0b8045bc8b3a8f46e4c67643d3f113d36b8a66b04e0ae4e62dab758e4615636。完整12组后台原始字节、sampler147456B/1881pixels、half131072B及MRT1245184B实机通过，38.719秒完整回程；proof见 reproducibility/v729-sampler-r1-native-proof.json。该旧基线及原证据保持；新cache-r2完整13组已独立通过。

完整锁屏时间/日期/壁纸、系统UI/FPS、睡眠唤醒、声音、Wi-Fi/蓝牙及广义纹理/depth/stencil/MSAA仍未完成，完整目标active。主机/离屏/控制器通过不能推断真实面板或完整系统成功。

操作只BH905ND69G、fastboot -s、COM>=10且VID1209/PID0001、PON只0x06、单download/boot、失败不重发。四回程marker、连续健康30秒及原worker退出后才能下一轮。一个controller持COM/camera；相机override→AF→2秒→MJPEG每5帧抽1→rotate90。gl5/userdata/SD/TA/XFL及救砖输入受保护；allocator/SCM/PON/RPC/VM冻结。Mac旧自动构建器pause保留。

完整历史状态见 docs/archive/20261116/STATUS-before-program-cache.md；既有专题和原失败保留。源码以正式仓库为准，Windows/Mac工具的LF字节和源锁已核对；本轮新修补及证据正提交发布。
