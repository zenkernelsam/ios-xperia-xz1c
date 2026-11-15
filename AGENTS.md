# AGENTS.md — Xperia XZ1 Compact / lilac

唯一正式源码仓库是ios-xperia-xz1c。所有修补、工具、源码锁、证据与复现步骤在这里维护并提交；Patch/Xperia只作参考/历史入口。开始先读README.md、docs/STATUS.md、docs/BUILD.md、docs/OPERATIONS.md。

## 当前基线

2026-11-16 cache-r2：kernel v729/774b5efd保持81源，GLES0a2c7e69、Metal87b91b14完整后台13组实机通过。原资源/queue/compute/render/constants/XR/Surface/mipmap/narrow/half/MRT/sampler全部保持，新增标准glProgramBinary在3个fresh pipeline中实际restore。sampler147456B的原source/target/vertex/全部padding、1881pixels零差异，序列919/925/929递增、最终分配0；half131072B与MRT1245184B仍逐原byte/NaN类别/payload/fence核验。38.984秒完整PON06回程、健康30秒、原worker退出。精确完整proof见reproducibility/v729-cache-r2-native-proof.json；旧12组proof不能准入缓存库。

Mesa禁用disk-cache时原程序二进制格式数为0，且driver SHA输出空stub未初始化；新平台hook使用实际frontend源/Mesa归档/defines与context/API/FD540身份，仅本FD540开放标准binary。原source/config/key/payload均校验；缺失request在实际冷链接前保存，已有文件不覆盖不删除。完整backboard primer最多8项，只处理本identity真实缺bin请求，不替换原绘制或延长原check-in。原cache-r1失败/raw及源hash保留；详情见docs/PROGRAM-CACHE.md。

本包CPU的独立后处理原失败保留：两个READY中第二段只重放3081B完全相同的启动前缀，库只在当前LIVE后实际读取一次。新validator要求原byte相等、完整行/明确marker边界、唯一banner/current token/精确长度FNV只读状态与单次读；10项故障/正常回放测试通过，原raw/results/stdout不改、不重发。独立记录见reproducibility/ramcpu-partial-replay-r1.json。

原Aegir仍需独立验收。r7原vertex/fragment compile成功后在GLSL/NIR prelink/gl_nir_opts超出check-in，尚未见IR3；原REVISE不改，符号化见original-link-nir-stack-r1.json。下一先本包CPU/原Aegir单发GPU，用同kernel/GLES/Metal/source-map的13组raw证明准入。必要时以真实runtime request后台预热，再验原cache hit/pipeline/draw/present与面板。完整壁纸/时间日期/UI/FPS/睡眠/声音/无线等继续active，不能用离屏或缓存shader通过替代系统完成。实际token/in-flight以STATUS为准。

历史AGENTS/STATUS全文归档docs/archive/20261116及更早日期，不改原失败和日志。sampler-r1旧基线和原proof保持，sampler语义来源固定BSD-2-Clause ios_sagit；half RTE和原constexpr语义继续以原字节验证。

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
