# AGENTS.md — Xperia XZ1 Compact / lilac

唯一正式源码仓库是ios-xperia-xz1c。所有修补、工具、源码锁、证据与复现步骤在这里维护并提交；Patch/Xperia只作参考/历史入口。开始先读README.md、docs/STATUS.md、docs/BUILD.md、docs/OPERATIONS.md。

## 当前基线与真实边界

2026-11-17 XR Clear优化：kernel v729/774b5efd、20648064B、81源保持；固定Mesa24.3.4/FD540新GLES72d39da5及Metal5e94ab57已独立GLES/完整后台14组GPU/原bytes/guard/fence/lifecycle通过，token4bcd571c9e12b34d、39.125秒完整回程，proof见reproducibility/v729-xr-clear-r1-native-proof.json。旧GLES0a2c7e69的完整14组45a839fc及原失败/样本仍保留作比较，不能准入不同GLES。旧Metalbe56或旧13组proof不能准入新库。half整数选择仅折叠已证等价路径，全部4294967296种float32/12884901888检查及65536 unpack、Mac target ASan/UBSan、131072个host GLES样本通过；WSL ASan基础设施失败单独保留，不能声称其通过。

原Aegir r2 token6d17bdf97a8fc7c8已恢复五条nonnull管线并实际出现两次750×1624 GPU draw result0；原extension check-in timeout三次/REVISE与不完整面板保持。新的PNG/ColorSync/释放采样不是旧glLink卡点的同一栈，不凭采样推定VM或颜色故障。postdraw-r1只增加原快照API耗时观察，保持原参数、返回、nil、异常、errno、颜色配置及像素；host真实PNG字节/解码/ICC保持不代替原手机验收。当前唯一controller/token、实际回程和本包CPU门槛以STATUS为准。

程序cache按实际Metal source-map与GLES派生scope，目录1777 sticky；旧文件/旧scope不改不删。新primer必须只读本scope的实际缺bin请求。完整14组primer8abc57d8已实际编译三个原request；真实request byte/key/identity按原raw匹配，不能用host或不同token替代。原失败、长行截断、partial CPU prefix及后处理拒绝保持，见各reproducibility记录和专题文档。

完整壁纸/时间日期/UI/FPS/睡眠/声音/无线及广义texture/depth/stencil/MSAA继续active。主机/离屏/缓存/控制器PASS不能替代原系统绘制与真实面板。原AGENTS/STATUS/README已逐字归档docs/archive/20261116，历史source/proof及失败边界保持。

snapshot-r1已完成同RAM CPU与原GPU单次回程：CPU de5cda38/38.797秒，GPU426db2f1/38.625秒。八类原方法两个进程均实际安装8/8，28完整/6未完成时耗；pid290原snapshot3.307秒（含wait2.807秒）、read21.222ms，原PNG/ICC3.911秒并返回32179B。原首次进程check-in超时与不完整面板保持，不把重叠wait/execute相加，不把等待全部归为GPU硬件时间，不凭栈修改冻结VM。详见POSTDRAW-OBSERVATION与STATUS。

## 硬件与返回

- 只操作Sony BH905ND69G，所有fastboot操作带-s BH905ND69G。
- 串口必须COM>=10且VID1209/PID0001；禁止COM3及其它设备。
- PON只0x06，单download/boot，失败不重发。完整ACK/DRAIN/RESET/FINAL_DRAIN、连续健康30秒、旧worker退出后才能下一轮。
- 不flash/erase gl5/userdata、SD、TA、XFL、救砖固件和备份；早期无LIVE的E1F4可能需要人工恢复。
- allocator/SCM/PON/RPC/VM既有约束不变；SweetGPU.cpp冻结SHA aa2c36792a2298b060aa858eabdc039522fcdde3cfdbafd453c5fc6c0683c157。

## 目录与回收

gpu/sweetgl和gpu/metal是用户态，xnu/lilac-current.patch是完整内核修补；tools/build/device/maintenance和reproducibility在正式仓库维护。deps固定上游源码，build仅构建输出。research只放参考Git仓库，archive/assets/organization只放本地资料；不得提交镜像、备份或私有设备数据。一次性Windows文件放%TEMP%，禁止C:/Users/Cisco He/Desktop。Mac工作路径~/Desktop/ios-xperia-xz1c，~/xnu-autobuild.pause保持，不启动旧常驻构建器。

禁止永久删除。Windows用原生Recycle Bin，WSL废弃文件转Windows回收站，Mac用NSWorkspace/Finder Trash。不得清空Windows/Mac回收站；Ubuntu垃圾桶非空时先转Windows回收站再恢复空目录。flash/fw、unlock救砖/XFL资料、基线和当前卡点证据受保护。保留原结果和失败记录，不覆盖旧试验输出。

## 相机与证据

相机只能override→AF(/cam/1/af)→2秒→MJPEG→每5帧抽1→rotate90；已旋转照片不得再转，grab_cam.py禁用，frame.jpg仅视频失败后单次兜底。一个controller持有COM/camera。

新包必须同kernel/private/owner/ready、对应新包CPU及精确GLES先验，完整payload和Mac源输入复算。短登记需完全相同kernel/GLES/Metal/source-map的完整native raw proof；新Metal/GLES需自身完整GPU准入，新observer RAM须自身CPU准入。ACK前任一native probe失败必须拒绝，回程后的退出期记录另列。不得把离屏PASS当时间/日期/壁纸、FPS、睡眠、声音或无线完成。遵循无人值守授权，不派子代理。

XR-CLEAR仅在Clear省掉即将覆盖的旧XR颜色解码，原整数位型快照、原Clear/写回、Load/DontCare/采样及half数值保持。真实请求primerd4b6b70f再次完整14组并配对73444B实际Earth请求成功。新原GPUe4eb78cb实际750×1624 Clear/draw通过，单样本wait1.067秒相对旧2.807秒少约62%；总snapshot3.719秒/CPU2.570秒未改善，不能据此宣称完整UI/FPS完成。原check-in超时/不完整面板保留；当前无in-flight，以STATUS为准。

2026-11-17原PNG输入c45fec49已确认RGBCIF10/ExtendedSRGB（10bpc/32bpp/stride3000/bitmap0x42000/scale2），两次原draw通过但PNG仍未返回，39.578秒完整回程。Mac是AMD/x86，不能拿其CIF10色值当835/iOS参考；8/16位简化候选均拒绝。新增后台CIF10参考只创建作者图像、调用原UIImagePNGRepresentation，不读取原UI像素或安装编码器；先自身RAM CPU后GPU，原GLES/native14、启动期限、四回程和全部红线保持，当前投递状态以STATUS为准。

2026-11-17后台CIF10参考ff064390已完整14组+五组原PNG通过、37.813秒回程/健康/worker退出；自身CPU2ad33c28/38.015秒保持。真实输出16位sRGB且有跨通道关系，逐分量查表候选不准入；作者整屏8.467秒不代表原UI时耗。当前健康fastboot/无in-flight，原Aegir超时及完整UI/外设目标保持；下一步原转换/filter/byte-swap位型保持优化。旧CRLF sampler包原样保留，实际LLVM14重编LF源的bitcode/library/header完全相同，当前参考使用build/cif10-reference-sampler-r1。

2026-11-17 reference3完整14组+六组原PNG/filter已独立通过，CPU21e8cfa3/47.156秒、GPU5221b885/38.172秒回程。ImageIO/UP保留原16位像素/ICC/metadata，作者混色8.531→2.104秒、渐变4.563→0.965秒，不代表原UI时耗。新编码器尚未安装；原Aegir/UI/外设目标继续。r2独立REJECTED保留，r3改诊断字段differences且GPU guard不变。当前无in-flight、健康fastboot，以STATUS为准。
