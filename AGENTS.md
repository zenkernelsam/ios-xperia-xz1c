# AGENTS.md — Xperia XZ1 Compact / lilac

唯一正式源码仓库是ios-xperia-xz1c。所有修补、工具、源码锁、证据与复现步骤在这里维护并提交；Patch/Xperia只作参考/历史入口。开始先读README.md、docs/STATUS.md、docs/BUILD.md、docs/OPERATIONS.md。

## 当前基线

2026-11-15 sampler-r1：kernel v729/774b5efd保持81源，GLES1c904beb、Metalb0b8045b完整后台12组实机通过。原全部资源/queue/compute/render/constants/XR/Surface/mipmap/narrow/half/MRT保持，新增原constexpr字807bff0000080a49、两个独立sampler状态、显式LOD/slot5，147456B原target/source/vertex/全padding、1881pixels零差异，序列913/919/923递增、最终分配0。half131072B与MRT1245184B仍逐原byte/NaN类别/raw payload/fence复核。38.719秒完整PON06回程、健康30秒、原worker退出；新完整native-proof含AIR_sampler。原half/MRT失败和旧成功输入保留。

新增sampler语义来自固定BSD-2-Clause ios_sagit；GLES逻辑8..23与真实单元区分，实际border/aniso能力检查，unnormalized/shadow/array/cube等未适配时拒绝。function不可变AIR/constant快照的成功翻译按对象寿命缓存；精确half整数偏置使host无缓存原starfield链接1.991→1.122秒，131072输入通过。native half packing实验14.212秒已撤回、未发设备，不作为加速证据。新库实机与原UI分别验，不把host/离屏当完整UI。

原Aegir r6首starfield仍check-in timeout，原REVISE保留；下一r7使用同kernel/GLES/Metal/source-map的完整原raw证明准入，CPU/单发GPU/完整回程不可省略。新builder/prerequisite/native-proof要求AIR_sampler自身GPU证据；旧库证明不能替代。短登记不重跑constructor长probe，也不要求原成功应用必须抛异常；原异常如有仍完整核验。实际token/in-flight以STATUS为准。完整壁纸/时间日期/UI/FPS/睡眠/声音/无线等继续active。

历史AGENTS/STATUS全文归档docs/archive/20261115，不继续叠加过时in-flight。原失败和日志不得改写。

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
