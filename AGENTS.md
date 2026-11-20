# AGENTS.md — Xperia XZ1 Compact / lilac

唯一正式源码仓库D:/Desktop/Git/ios-xperia-xz1c；Patch/Xperia只作参考／历史入口。先读README.md、docs/STATUS.md、docs/BUILD.md、docs/OPERATIONS.md。当前进程和原结果优先于旧工作日志，专题入口见docs/INDEX.md。

## 当前状态与授权

2026-11-21 Sony BH905ND69G 健康fastboot、无controller／串口worker／in-flight。用户已确认新蓝灯会话并授权无人值守；用户最新要求完成当前轮后先整理Mac和本周文档，当前设备轮及本周整理已收尾，后续依据新证据继续。不派子代理。

已验生产基线v730/f258dee9（20665112B、83实际源）、GLESd6d8fcfa（14765296B）、Metal5e94ab57／source-map7b5a083a、固定Mesa24.3.4/769e5146/FD540；自身CPU／GLES9、referenceCPU／native16／PNG6完整通过。灰色锁屏时间日期与单串口上滑后正常多彩主页有720×1280原扫描帧，壁纸、动态UI／FPS、睡眠、光学／背光、声音和无线仍active。

原附件GLES21de6821自身CPU／GLES9实机通过；reference Metal8feea459／source-mapdb1605ef完整native21在D32／S8／Memoryless／MSAA首phase字节失败，GPU011e16b3／62fb2e55均REVISE，原presentation／PNG未执行。独立诊断保留12页原作者byte／hash／handle／fence，颜色和guard正确，depth／stencil非线性写入。新DepthTiledPlane.h仅用户态FDL5工作页及GPU raw-copy；GLES c617f2ee自身CPU122537a0／GLES9 16c49c82通过，reference CPU b2f3fc4f原controller PASS却被独立启动前缀字节核验拒绝，完整回程38.594秒；未发该GPU。不要生成native21 proof、提升基线或重发旧包。见STATUS、ATTACHMENT-HARDWARE及depth-tiled-r1-stage-20261121.json。

原壁纸r3／3541e07940请求完整，共享queue／scene有效，但display／sync／tick／nextDrawable／pipeline0，灰字／黑壁纸，STOP保持。原Vista init本来paused=YES；模式／环境33方法observer只离线完成，不能强制取消paused或draw。r4 USB-open／r5 guard失败不复发。

用户新增原生比例要求：未来720×1280像素／360×640点／scale2、取消750×1624横纵不等缩放。make_render_adt.py已支持xz1c-native-2x离线profile；设备geometry未变，需同靴UIScreen／MobileGestalt、ADT、状态栏／safe area及触控证据，见DISPLAY-GEOMETRY。只改MG不足以称完成。

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

本周Mac1713项／34883文件／11,852,238,934B已原生Trash，build约14GiB→3.6GiB，系统仍约94%／62GiB可用；4条Finder .DS_Store哈希变化另记，用户内容复核。gl4／gl5／IPSW／基线／原failed包blob／固定deps／dxb保存。后续避免堆重复HFS容器、符号表、测试可执行文件和传输tar。

文档只在STATUS汇总当前状态、BUILD放现行复现、OPERATIONS放规则、LAYOUT放恢复、INDEX串专题。旧里程碑／handover／handout原文和SHA先archive，专题技术证据保留，不把旧“当前in-flight”恢复。每轮更新一份原结果摘要和关键topic；不要在每个入口复制大段工作日志。原AGENTS／README等47份本周原文见docs/archive/20261121/weekly-integration/original-document-manifest.json。
