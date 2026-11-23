# 真机、串口与相机

当前设备／候选和本轮结果统一看[STATUS](STATUS.md)，源码与构建见[BUILD](BUILD.md)。用户授权无人值守；每轮完整回程、健康30秒及worker退出后再下一轮，按约定先收尾回收与文档。历史失败不重发。

## 单发和回程

只BH905ND69G，所有fastboot设备命令带`-s BH905ND69G`；不碰其它设备。串口COM≥10且VID1209/PID0001，只一个controller／serial_worker持COM，禁COM3和旁路串口。守护和单发发送器核对资产SHA及身份。未建立本token LIVE的早期E1F4可能需要物理恢复，不能假设所有失败都能自动返回。

每包单download／boot、max_cycles1、on_pass／on_failure均stop，失败不重发，不换token重复同一失败。PON只0x06。ACK／DRAIN／RESET／FINAL_DRAIN、fastboot连续健康30秒、旧worker退出逐项通过后才下一轮。无匹配LIVE不发回程命令；回程成功不改变GPU的REVISE／STOP。gl5／userdata／SD／TA／XFL／固件／校准／备份不flash／erase。

新包绑定serial_capture_revision1及实际worker源码SHA。worker先构造关闭的Serial，DTR=false后打开唯一COM，再以xb新建原raw文件，DTR=true后仅发一次loop token／status，连续读取当前靴cursor0流，不发replay／二次loop。Windows驱动打开期间会清RX，DTR提前开启和随后replay会交错启动字节。独立后处理核对顺序、COM身份、单READY／banner／LIVE、原RAM读字节和完整回程；旧exact replay核验保留，只供原历史证据。旧包缺新绑定不准实机；原失败不重分类。见[捕获研究](../reproducibility/serial-capture-r1-research-20261121.json)。

新的物理会话确认之后，可先独立只读健康观察：

```powershell
python -B tools/device/observe_fastboot.py --output "$env:TEMP\新的健康目录"
```

只有本轮完整returned health和worker退出后继续，不能用枚举正常提升旧失败包。

## 实际准入

新kernel自身private／owner／ready；新RAM包自身CPU；新GLES自己的完整9组；新Metal／source-map自己的完整能力组与共享PNG6。生产native16、Depth3217、Stencil818、Memoryless19、MSAA20、当前presentation21；旧proof不准入新功能库。short registry必须完整原raw、同kernel／GLES／Metal／source-map精确先验，并与嵌入proof一致。

run_trial在手机USB／COM命令前复算完整payload、Mac83实际源、Mesa归档和已编译用户态源码。默认offline，加--live才投递；每包／运行目录必须不存在，运行目录是Windows TEMP子目录。例如，实际路径按已审核的包与同版本原raw填写：

```powershell
python -B tools/device/run_trial.py --bundle build/本轮CPU包 --directory "$env:TEMP\新的CPU目录" --private-evidence "$env:TEMP\同核private原目录" --live
```

GPU还需--owner-evidence、--ready-evidence、--rpm-evidence、--cpu-evidence；有Metal再--gles-evidence，有short registry再--native-evidence，有PNG encoder再--png-evidence。原artifact／plan／result／raw／wire、command和回程全部保留。独立后处理拒绝即没有准入，不能用controller PASS绕过；本轮启动日志案例见[CPU拒绝](../reproducibility/depth-tiled-r1-reference-cpu-refused-20261121.json)。

内核GPU底层／allocator／SCM／PON／RPC／VM、SweetGPU冻结。Mac保持~/xnu-autobuild.pause，旧常驻构建器不恢复工作。工具fastboot／bash由LILAC_FASTBOOT／LILAC_BASH或PATH选，xperable由使用者提供assets/device-tools/xperable.exe且SHA精确；boot_once只单发，不能用xperable自带boot重试。

## UI截图和输入

UI／文字／图标／颜色优先串口720×1280／BGR888／stride2160原扫描帧。同kernel完整成功private截图先验先复验；GPU包--screen-evidence默认45秒，--screen-after150为150秒单帧，artifact／plan同值，观察期限内执行。每靴只发一次frame，缺块／失败不补发，最终取证复用本帧。

一个worker排队命令和读取原raw，来源、实际扫描寄存器、逐记录／块／流CRC、FNV和保存BGR／PNG像素全部绑定本token，见[SCANOUT-SCREENSHOT](SCANOUT-SCREENSHOT.md)。不将750×1624 IOSurface／shadow当原扫描。原截图无需rotate90；旧raw可离线重解，明确原日期／token，不能冒充当前实时。fastboot期间没有iOS帧可读，不为截图复发失败boot。

--swipe-once仅本LIVE／F12实际就绪后120秒一次固定上滑，150秒frame；和sampling独立。核对本靴command／tx／一次注入，合成输入不替代真人触控。--idle-sample仅原Aegir library返回后180秒单sample，与--thread-sample原first-clear触发互斥。原90秒周期样本另列，不能从一次CPU栈推全部耗时或改冻结组件。

扫描帧证明缓冲区内容，不能证明LCD发帧、光学色偏、物理错行、背光／睡眠或FPS；power_off是系统请求，也不是背光实测。原失败提前frame记录其真实时机，不冒充150秒UI图。

## 附件失败页面和相机

新AttachmentPageSnapshot只在作者附件字节检查失败、GPU command实际Completed并持有fence后读已有page。24B每块、每页≤4096B、前后FNV稳定，原handle／GPU／LIVE→ACK绑定。独立decoder拒绝缺块／乱序／重复／损坏／foreign slot；它保留原REVISE，不能成为native准入proof。它不读原UI或CPU修补数据，见[ATTACHMENT-HARDWARE](ATTACHMENT-HARDWARE.md)。

DroidCam只用于LCD光学／背光／物理错行，唯一流程override→AF(/cam/1/af)→等2秒→MJPEG(/video?1280x720)→每5帧抽1→rotate90。已旋转不再转，grab_cam.py禁用；frame.jpg只能视频失败单次兜底。日常UI取证不用等待相机服务。

## 原／拟合原生布局对照

同一个精确新helper／RAM先自身CPU，完整回程后单发original（GEO_GRID0），完整Root／Dock／Folder模型及原字节准入后才单发fitted（GEO_GRID1）。两靴均同kernel、同helper，120秒一次上滑、150秒一次原scanout，包和run目录各自新建；缺原完整基线、slot／LINK／child、实际mode或回程就停止。

独立native_grid_profile_evidence同时检查完整应用multiset、Dock顺序、Folder身份／list内容、原生窗口及标签间距。未显示页从model读取，旧15个第一页reference及DefaultIconState不作为全基线。Root允许原system分页重排，不据view数量判应用丢失。

原同名标签候选多于一个、祖先未观察或缺记录仍保留REVISE；不得按坐标／字号挑预期候选或补造旧raw。下一轮补限界只读归属capture后，新helper／RAM仍需自身CPU及新original基线。controller PASS和完整模型保留分别不提升原UI／光学。当前结果只看STATUS，旧失败包不重发。

## USB拒绝和保存

独立usb_open_observation.py只查Sony身份、配置和bulk描述符；--claim-once只单次claim／release接口，无bulk读写／download／boot／reset，不set_configuration或detach驱动。输出新TEMP目录、单device lock；成功不解释前次竞态或授权重发旧失败。

原失败／raw／退出期记录不覆盖；one-time staging在TEMP、Mac build。所有废弃文件进原生回收站，WSL废弃内容转Windows回收站，Windows／Mac都不清空，路径先核对。救砖／XFL／flash/fw／unlock／备份和当前基线证据受保护。回收／恢复见[LAYOUT](LAYOUT.md)，旧操作手册原文与SHA见[本周归档](archive/20261121/weekly-integration/original-document-manifest.json)。
