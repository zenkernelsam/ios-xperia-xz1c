# AGENTS.md — Xperia XZ1 Compact / lilac

唯一正式源码与操作仓库D:/Desktop/Git/ios-xperia-xz1c；Mac为~/Desktop/ios-xperia-xz1c。Patch/Xperia只作参考／历史入口。先读README.md、docs/STATUS.md、docs/BUILD.md、docs/OPERATIONS.md；当前原结果与进程优先于历史工作日志，专题入口docs/INDEX.md。

## 当前授权与范围

用户授权BH905ND69G无人值守迭代，要求当前轮结束后先整理Mac和本周文档。本轮原生回收与整合按ORGANIZATION及原receipt记录；手机当前状态、controller／worker及in-flight统一以STATUS与实际进程为准。不派子代理。精确基线、候选、原失败与下一步只在STATUS维护，不在各入口复制多轮工作日志。

继续免费开源Mesa／Freedreno GLES及Metal用户态移植，原UI／壁纸、FPS、睡眠、光学／背光、声音及无线保持active。当前HEAD是候选，生产基线不因host／局部byte／GPU Completed／回程成功而提升。原失败包、raw和proof不覆盖或重发；Vulkan尚未移植。

用户原生几何目标720×1280像素／360×640点／scale2已同靴核对，平直状态栏及完整原／拟合应用模型保留已验；标签祖先归属、完整safe area／真人触控／光学尚未完成。现有native1与sf2x1冲突，新ADT、MG／UIScreen、状态栏／safe area和触控需同靴一致，只改MG不足。原Vista paused／mode按原请求观察，不能强制解除或画替代帧冒充UI。

## 硬件、回程与冻结

- 仅BH905ND69G，所有fastboot设备命令带`-s BH905ND69G`。
- COM≥10且VID1209/PID0001，一个controller／serial_worker持有；禁COM3／其它设备／另开串口。
- PON只0x06；单download／boot，失败不重发，不以新token掩饰同一失败。max_cycles1，pass／failure均stop。
- 当前LIVE匹配token才能发控制；ACK／DRAIN／RESET／FINAL_DRAIN、健康30秒、旧worker退出均独立通过后才下一轮。无LIVE早期E1F4仍可能需要物理恢复。
- 不flash／erase gl5、userdata、SD、TA、XFL、校准、救砖固件和备份。
- 内核GPU底层／allocator／SCM／PON／RPC／VM冻结，SweetGPU.cpp SHA必须`aa2c36792a2298b060aa858eabdc039522fcdde3cfdbafd453c5fc6c0683c157`。CPU栈或局部图像失败不授权改底层。几何先用现有native选项，不偷改冻结内核。
- v730实际SweetSurface selectors36／37／38／45零输出；原共享事件port／mapped page未实现。事件未编译草稿移入build/weekly-20261121，未接入当前库，不能宣称shared event成功。

## 准入与证据

新kernel自身private／owner／ready；新RAM自身CPU；新GLES自己的完整9组；新Metal／source-map与能力相匹配完整native组及共享PNG6。生产16、Depth3217、Stencil818、Memoryless19、MSAA20、当前presentation21；旧proof拒绝。short registry需同kernel／GLES／Metal／source-map完整原raw proof，run_trial独立复算payload与Mac实际83源／Mesa归档／用户态编译源。控制器PASS不代替原字节和后处理。

所有GPU探针只用本LIVE→ACK原raw；ACK后退出期单列。host／离屏／缓存／seed／GPU Completed不能代替原UI和LCD光学。原结果／raw／失败不可覆盖。program-cache scope只本build／source-map，1777 sticky；旧cache不改不删，primer只有实际缺bin请求才运行，不空跑冒充预热。

UI优先串口真实720×1280／BGR888／stride2160扫描帧；同核成功private截图先验必须复验。45或150秒单frame，来源／寄存器／CRC／FNV／保存像素绑定本token；缺块失败不补发。power_off不证明背光。scanout_trial_evidence保留原STOP／REVISE。

单次串口上滑只本LIVE／F12就绪后120秒一次，150秒截帧；合成输入不代替真人触控。idle sample仅原library返回／LIVE后180秒一次，和first-clear sample互斥；原90秒周期样本独立，不用单栈推整靴。

相机只用于光学／背光／错行：override→AF(/cam/1/af)→等2秒→MJPEG→每5帧抽1→rotate90。已旋转不再转，grab_cam.py禁用，frame.jpg只视频失败单次兜底。

## 构建、整理与文档

Mac正式~/Desktop/ios-xperia-xz1c，~/xnu-autobuild.pause保持；旧常驻构建器不可恢复工作。核对Mac Git及实际源／归档SHA，临时复制源码不等于同步完成。WSL和Windows同正式源码；新输出build且目录不存在，一次性Windows staging在%TEMP%，禁C:/Users/Cisco He/Desktop。

gpu／xnu／tools／reproducibility进入正式Git；deps固定上游，build产物，assets输入／基线，archive历史实验／证据，organization完整回收记录。research只参考Git仓库，不存新实验。所有救砖／flash/fw／unlock／XFL／备份及基线和当前卡点所需少量原证据受保护。

禁止永久删除。Windows原生Recycle Bin；WSL废弃文件转Windows回收站；Mac NSWorkspace／Finder Trash；不清空Windows／Mac回收站。Ubuntu垃圾桶非空先转Windows回收站再恢复空目录。回收不等于释放磁盘。路径先确认在明确范围内，重复／父子回收路径整批拒绝。

本周及本轮回收实际路径／SHA见docs/ORGANIZATION.md；所有原生回收站未清空。基线、当前失败所需原证据和固定deps／dxb保持受保护。

文档只在STATUS汇总当前状态、BUILD放现行复现、OPERATIONS放规则、LAYOUT放恢复、INDEX串专题。旧里程碑／handover／handout原文和SHA先archive，专题技术证据保留，不把旧“当前in-flight”恢复。每轮更新一份原结果摘要和关键topic；不要在每个入口复制大段工作日志。原文manifest由INDEX／ORGANIZATION索引，本轮整合前50份在docs/archive/20261122/native-full-model-r19-close-out。
