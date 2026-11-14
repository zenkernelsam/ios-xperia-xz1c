# AGENTS.md — Xperia XZ1 Compact / lilac

唯一正式源码仓库是ios-xperia-xz1c。所有修补、工具、源码锁、证据与复现步骤在这里维护并提交；Patch/Xperia只作参考/历史入口。开始先读README.md、docs/STATUS.md、docs/BUILD.md、docs/OPERATIONS.md。

## 当前基线

本轮mipmap WIP：原生多级/Private/逐级区域/GPU生成和LOD/trilinear已实现且主机通过，r2精确GLES470c5643的CPU/GPU与新Metal包CPU实机通过。Aegir ctor完整四格式探针仍被原extension check-in timeout打断；r1 STOP、r2 REVISE保留，r2 38.969秒完整回程。Sony健康fastboot无worker/in-flight，不能称完整mipmap/UI通过。下一将完整诊断组移出Aegir启动关键路径，所有原字节/fence/重生成/LOD/保护区检查保留，再验原壁纸；详见METAL-MIPMAPS.md与STATUS。

2026-11-14：内核v729/774b5efd保持81源审计；GLES4241a020、Metal f8d78396已实机完成原生AIR compute/render/constants/XR及IOSurface。surface-r1的13完整/1未完成样本核对194688B零差异，包含原64/128offset、创建者VM/CF释放后GPU绘制/采样和padding，45.859秒完整PON06回程、1652全释放/unmapped、active/orphan/root0。

surface-discovery-r1另有10完整原system Metal登记/IOSurface样本；原Aegir已到SweetMTLBlitEncoder generateMipmapsForTexture:缺方法。原controller PASS只对应原生探针目标，原应用仍异常退出，不能称完整UI通过。1442allocated/released、1441unmapped、active/orphan/root0，38.282秒完整回程。Sony健康fastboot，无controller/COM/camera或in-flight。完整GPU/显示/外设目标active；下一实现真正mipmap。历史全文已归档docs/archive/20261114/AGENTS-before-XR-and-IOSurface.md，以STATUS为准。

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
