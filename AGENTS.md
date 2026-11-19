# AGENTS.md — Xperia XZ1 Compact / lilac

唯一正式源码仓库是 D:/Desktop/Git/ios-xperia-xz1c。所有源码、工具、锁和复现步骤只在这里维护并提交；Patch/Xperia只作参考／历史入口。开始先读 README.md、docs/STATUS.md、docs/BUILD.md、docs/OPERATIONS.md，当前进程／结果优先于历史状态。

## 当前基线与目标

2026-11-19现行 v730/f258dee9（20665112B、83源）、GLES CA-matrix-r1/d6d8fcfa（14765296B）、Metal5e94ab57／source-map7b5a083a、固定Mesa24.3.4/769e5146/FD540。新库自身CPU／GLES原8＋CA矩阵1组、referenceRAMCPU／完整native16／共享PNG6组、原RAMCPU／primerRAMCPU均已独立通过。完整native token46747c46，38.375秒回程；旧166c08a3或旧15组proof不能准入新库。

新矩阵适配只在准确20H392 QuartzCore UUID／范围／两代码锚点／原表达式／原空槽全部匹配后补0x14、0x2d，原方程保持；其它模糊槽未安装。生产GLI矩阵6阶段1536B／192B guard实际最大差异0／context释放；不能把host测试当手机像素。

原cb9a39f8／206fb768两张150秒完整scanout已恢复正常灰色锁屏时间日期，背景黑；原881ce1c5经一次串口上滑出现完整主页／正常多彩图标／文字／widgets／dock，39.688秒四marker／健康30秒／worker退出。最新原请求observer-r2 token61745a1e16c8ab41已39.375秒完整回程／健康30秒／worker退出，当前健康fastboot／无in-flight。r2全部13方法／10条ABI inventory完整，20条原请求完成：quad_init／quad_init_async／set_device／quad_layout各5，五次画布初始化nonnull，其余9类0。150秒锁屏扫描帧仍灰字／黑壁纸，无display／nextDrawable／pipeline请求，原STOP／OBSERVATION_INCOMPLETE保持；仅改变observer，生产GLES／Metal／内核未变。r1两条长ABI记录破损仍拒绝，不能补造或重发旧包。详情见STATUS／METAL-REQUEST-OBSERVATION和对应reproducibility。原idle sample与周期样本独立，不凭一次栈推全部耗时。

完整壁纸／稳定UI与FPS／睡眠／光学／声音无线及广义texture-depth-stencil-MSAA目标仍active。原IR3库26pipeline／三次draw／PNG返回及旧check-in／cold-link／STOP失败完整保留专题和docs/archive/20261119；不从过时工作日志恢复已结束in-flight。

2026-11-20最新quad-route-r3／3541e07973fe676f已39.109秒四marker／健康30秒／worker退出，当前fastboot／无in-flight。observer07d1360d／132544B只增12原场景／队列方法观察，生产库及kernel保持；自身RAMCPU aa9d5a21通过／38.141秒。原40请求完整：共享队列5次nonnull、device_queue4次nonnull，NUNIQuad加入5次／NUNIScene设置5次，paused5次全YES；sync／tick／disable／enable均0，manager_device与window各8条达到采样上限，不能当总调用数。原13类20请求仍完整但display／nextDrawable／pipeline0，150秒灰字锁屏／黑壁纸，STOP保留。原Vista init本就setPaused:YES，不据此判驱动错误；下一步查原environment／startAnimation／snapshot触发。见METAL-REQUEST-OBSERVATION与v730-quad-route-r3证据。

## 硬件与返回

- 仅Sony BH905ND69G；所有fastboot设备命令必须带 `-s BH905ND69G`。
- 串口COM>=10且VID1209/PID0001，一个controller／serial_worker持有；禁止COM3／其它设备和另开串口。
- PON仅0x06；单download／boot，失败不重发。完整ACK／DRAIN／RESET／FINAL_DRAIN、连续健康30秒、旧worker退出后才下一轮。
- 不flash／erase gl5／userdata／SD／TA／XFL／救砖固件及备份；无LIVE早期E1F4可能仍需人工物理恢复。
- allocator／SCM／PON／RPC／VM与GPU底层冻结。SweetGPU.cpp SHA必须 `aa2c36792a2298b060aa858eabdc039522fcdde3cfdbafd453c5fc6c0683c157`；不得因采样栈修改它们。
- 用户授权无人值守持续推进，不派子代理。单轮结果完成后按实际证据再制定下一发，不用新token掩饰重发失败。

## 准入与证据

新kernel自身private／owner／ready；新RAM包自身CPU；新GLES完整自身GPU；新Metal／source-map完整native16，当前还需GLES矩阵第9组和共享PNG精确先验。short registry必须同kernel／GLES／Metal／source-map的完整原raw proof，run_trial独立复算完整payload与Mac实际源码／归档。ACK前任一GPUprobe失败拒绝，ACK后退出期记录单列；不以host／离屏／缓存／controller PASS代替原UI。

程序cache仅本实际build／Metal source-map scope；1777 sticky，原文件／旧scope不改不删。primer只读本scope实际缺bin请求；标准NIR binary不等于异步IR3完全预热，旧三轮prime_begin→ACK没有IR3日志事实保留。新库无实际请求时不得空跑primer冒充改善。

串口截图优先。v730同核完整private先验必须复验；只读真实720×1280／BGR888／stride2160扫描缓冲区，来源／寄存器／CRC／FNV／保存像素绑定本token。45或150秒单frame，缺块或失败不补发，原图片无需rotate90。`scanout_trial_evidence.py`只能独立验证截图并保留STOP；power_off元数据不能当背光／LCD光学验收。

`--idle-sample`只有原library返回／LIVE后180秒单sample，原内核90秒周期样本独立保留；与`--thread-sample`首次Clear互斥。`--swipe-once`只在本LIVE／F12就绪后120秒一次串口上滑、150秒截帧，和采样独立；合成输入不代替真人触控。一个worker持COM，命令及响应都保留。

相机用于光学／背光时只能 override→AF(/cam/1/af)→等2秒→MJPEG→每5帧抽1→rotate90；已旋转图片不再转，grab_cam.py禁用，frame.jpg仅视频失败单次兜底。

## 目录与回收

gpu/sweetgl、gpu/metal、xnu完整patch及tools/reproducibility在正式Git；deps固定上游，build输出。research仅参考Git仓库，archive／assets／organization本地资料；不得提交镜像、备份、原私有帧或Apple shader请求。一次性Windows文件放%TEMP%，禁止C:/Users/Cisco He/Desktop。Mac正式路径~/Desktop/ios-xperia-xz1c，~/xnu-autobuild.pause必须保持，不启动旧常驻构建器。

禁止永久删除。Windows用原生Recycle Bin，WSL废弃文件转Windows回收站，Mac用NSWorkspace／Finder Trash；不得清空Windows／Mac回收站。Ubuntu垃圾桶非空先转Windows回收站再恢复空目录。救砖／XFL／flash/fw／unlock、基线和当前卡点证据受保护。过时文档原文与SHA先归档，不覆盖旧raw／结果／失败。
