# AGENTS.md — Xperia XZ1 Compact / lilac

唯一正式源码仓库是ios-xperia-xz1c。所有修补、工具、源码锁、证据与复现步骤在这里维护并提交；Patch/Xperia只作参考/历史入口。开始先读README.md、docs/STATUS.md、docs/BUILD.md、docs/OPERATIONS.md。

## 当前基线与真实边界

2026-11-16 half-selection-r1：kernel v729/774b5efd、20648064B、81源保持；固定Mesa24.3.4/FD540 GLES0a2c7e69保持；Metal5e94ab57的完整后台14组实际GPU/原bytes/guard/fence/lifecycle已通过，token45a839fc4c105e01、39.062秒完整回程，proof见reproducibility/v729-half-selection-r1-native-proof.json。旧Metalbe56或旧13组proof不能准入新库。half整数选择仅折叠已证等价路径，全部4294967296种float32/12884901888检查及65536 unpack、Mac target ASan/UBSan、131072个host GLES样本通过；WSL ASan基础设施失败单独保留，不能声称其通过。

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
