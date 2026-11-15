# AGENTS.md — Xperia XZ1 Compact / lilac

唯一正式源码仓库是ios-xperia-xz1c。所有修补、工具、源码锁、证据与复现步骤在这里维护并提交；Patch/Xperia只作参考/历史入口。开始先读README.md、docs/STATUS.md、docs/BUILD.md、docs/OPERATIONS.md。

## 当前基线

2026-11-16 static-half-r1：kernel v729/774b5efd保持81源、GLES0a2c7e69保持，Metalbe56ad13完整后台14组实机通过。新增完整2B half/i16常量内存双向按位读写，12case共196608B原输出/前尾guard/NaN位型/fence/创建pipeline后reset的snapshot零差异，序列850..872递增、最终allocated0。原half131072B、MRT1245184B、sampler147456B及3次fresh program binary restore等原13组全部保持。38.860秒完整PON06回程、健康30秒、原worker退出，proof见reproducibility/v729-static-half-r1-native-proof.json；旧13组proof不能准入新静态half库。

StaticHalfBits.h和独立patch仅处理完整binary16标量的half/i16等宽访问，LLVMConstBitCast保留原位型；非零offset/不同size/类型/vector等拒绝，原bounds/跨member/padding检查保持。固定BSD-2-Clause ios_sagit引用源码不改。实际LLVM/ASan/UBSan全65536位型×4路径262144检查通过；原earth/post代表常量AIR/ESSL和作者fixture已主机通过，不能推断运行时原常量/面板。详见docs/AIR-STATIC-HALF.md。

cache-r2已在原Aegir实际恢复星空89,821B并返回nonnull，云层84,333B与threshold18,929B也成功创建/恢复。原r2 REVISE/raw保持，完整UI仍因earth/post static initializer memory type mismatch而未完成。该错已精确复现为half→i16、offset0、size2，现经14组硬件验证；下一新原Aegir包CPU/单发GPU，嵌入新14组raw proof。真实cache/prime流程见PROGRAM-CACHE；仅实际runtime request、同build/source/config/payload身份，旧文件不覆盖不删，不延长check-in。

CPU前缀回放校验继续核验原bytes/唯一banner/current token/明确marker/单次库读与完整回程。原postprocessing失败、3,081B partial replay证据和原raw/results/stdout保留，不重发旧包；见ramcpu-partial-replay-r1.json。实际token/in-flight以STATUS为准。

完整壁纸/时间日期/UI/FPS/睡眠/声音/无线及广义texture/depth/stencil/MSAA继续active。主机/离屏/缓存/控制器PASS不能替代原系统绘制与真实面板。历史AGENTS/STATUS在docs/archive/20261116及更早目录，原失败/基线保持。

## 硬件与返回

- 只操作Sony BH905ND69G，所有fastboot操作带-s BH905ND69G。
- 串口必须COM>=10且VID1209/PID0001；禁止COM3及其它设备。
- PON只0x06，单download/boot，失败不重发。完整ACK/DRAIN/RESET/FINAL_DRAIN、连续健康30秒、旧worker退出后才能下一轮。
- 不flash/erase gl5/userdata、SD、TA、XFL、救砖固件和备份；早期无LIVE的E1F4可能需要人工恢复。
- allocator/SCM/PON/RPC/VM既有约束不变；SweetGPU.cpp冻结SHA aa2c36792a2298b060aa858eabdc039522fcdde3cfdbafd453c5fc6c0683c157。

## 目录与回收

gpu/sweetgl和gpu/metal是用户态，xnu/lilac-current.patch是完整内核修补；tools/build/device/maintenance和reproducibility在正式仓库维护。deps固定上游源码，build仅构建输出。research只放参考Git仓库，archive/assets/organization只放本地资料；不得提交镜像、备份或私有设备数据。一次性Windows文件放%TEMP%，禁止C:/Users/Cisco He/Desktop。Mac工作路径~/Desktop/ios-xperia-xz1c。

禁止永久删除。Windows用原生Recycle Bin，WSL废弃文件转Windows回收站，Mac用NSWorkspace/Finder Trash。不得清空Windows/Mac回收站；Ubuntu垃圾桶非空时先转Windows回收站再恢复空目录。flash/fw、unlock救砖/XFL资料、基线和当前卡点证据受保护。保留原结果和失败记录，不覆盖旧试验输出。

## 相机与证据

相机只能override→AF(/cam/1/af)→2秒→MJPEG→每5帧抽1→rotate90；已旋转照片不得再转，grab_cam.py禁用，frame.jpg仅视频失败后单次兜底。一个controller持有COM/camera。

新包必须同kernel/private/owner/ready、对应新包CPU及精确GLES先验，完整payload和Mac源输入复算。主机stub不是硬件验收；原页面、fence、生命周期和真实面板分别验证。ACK前任一native probe失败必须拒绝，回程后的退出期记录另列。不得把离屏PASS当时间/日期/壁纸、FPS、睡眠、声音或无线完成。遵循无人值守授权，不派子代理。
