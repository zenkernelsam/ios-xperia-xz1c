# AGENTS.md — Xperia XZ1 Compact / lilac

唯一正式源码仓库是ios-xperia-xz1c。所有修补、工具、源码锁、证据与复现步骤在这里维护并提交；Patch/Xperia只作参考/历史入口。开始先读README.md、docs/STATUS.md、docs/BUILD.md、docs/OPERATIONS.md。

## 当前基线

2026-11-15：v729/774b5efd保持81源审计，GLES499bc6e9、Metal3b4a4082已完整backend实机通过原生资源/queue/compute/render/constants/XR/Surface/mipmap及dynamic8/16窄整数和half存储/逐结果舍入/顶点片元。AIR_half1完整/0缺项、131072B原字节/NaN位置与实际payload/全部保护区通过，303全released/unmapped、active/orphan/root0，38.282秒完整PON06回程。主机half全部65536码+65536float32边界和原Aegir顶点也通过，但各自证据区分。

原Aegir短登记已与长probe分离，--native-evidence必须重新完整验证原raw log并与嵌入kernel/GLES/Metal/source-map/token/image/log SHA一致；新库narrow/half/MRT缺任何独立GPU证据不准入。registry-short-r4已让原threshold管线实机编译成功，请求375×812/3mip R8 Private纹理；原starfield输出2附件，XR star/earth/post还有blend需求。原应用随后check-in timeout，原REVISE不提升UI通过，37.453秒完整回程；364allocated/released、363unmapped、active/orphan/root0。多附件/FP32 image混色正在准入，原constant sampler、原壁纸和完整UI/FPS/睡眠/声音/无线继续active。设备实时状态、当前token与in-flight以STATUS为准。详见AIR-HALF.md、AIR-NARROW.md、METAL-REGISTRY-SHORT.md和STATUS。

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
