# 当前状态 — 2026-11-20

Sony BH905ND69G 当前健康停在 fastboot，无 controller／串口 worker／in-flight。最新场景／队列观察 r3 靴 `3541e07973fe676f` 已完成 PON 0x06 的 ACK／DRAIN／RESET／FINAL_DRAIN、连续健康30秒及 worker 退出，回程39.109秒。40条原场景／队列请求完整返回：共享队列5次nonnull，场景加入／设置各5次，paused5次全YES，sync与tick仍0。原13方法20请求也完整，但display／nextDrawable／pipeline未发生。150秒完整扫描帧仍灰色锁屏时间日期与黑壁纸，原GPU登记窗口保持 STOP／OBSERVATION_INCOMPLETE。前轮单次上滑 `881ce1c5d844e7ba` 的正常颜色主页证据保留，截图成功没有改判GPU或壁纸阶段。

当前正式基线为 CA 矩阵修订 r1：

| 输入 | 精确身份 |
| --- | --- |
| 内核 v730／83源 | `f258dee9c6d142b90a573f796eeec5e92b39dd65feed6a21b2542d90fb66a88d`，20665112B |
| GLES／CA 矩阵 r1 | `d6d8fcfa27bcac84ab5edb3d2a11c1ed23374e8b491a3e8de093a9a2a819ac2b`，14765296B |
| Metal／AIR | `5e94ab57401479b42f506884a172a7a839378a82310661a1f66164e99c21f988` |
| Metal source-map | `7b5a083abdb98fa5d29ef82fbd8e7e68b2a29fd995f16c4163d5c612cb8b9de7` |
| 固定 Mesa／GPU | 24.3.4／`769e51468b49b2a42f0a0eaf71cf9eed5ff4e5de`／FD540 GLES3.1 |
| Shader cache build ID | `a042e5d1ad72d2cc0180c564ee682b02027194b0e95cfb459b0474a3672fee46` |

当前源码、严格准入与全部9靴输入／原判定在 [run-index](../reproducibility/v730-ca-matrix-r1-run-index.json)，主机／构建／实机边界集中在 [CA-MATRIX](CA-MATRIX.md)。旧库 `166c08a3` 的16组证明不能准入 `d6d8fcfa`；当前库已取得自己的新证明。

## 本轮已验证

旧 EAGL 纹理补全使用另一版 QuartzCore 地址，实际20H392 的表／代码相差0x4000，原保护检查一直拒绝安装。现行适配按准确 image UUID `9e40b2eb260c3af1ae45a1a551a8c1b9`、Mach-O范围、两个实际代码锚点、原矩阵表达式和两个空槽核验后，补齐0x14／0x2d矩阵函数；不修改原矩阵方程。其它模糊槽未在这轮安装，未知布局拒绝。生产GLI source rewrite／link／draw／framebuffer-fetch新增第9组，六阶段检查1536B与192B guard，实际FD540最大差异0、全部guard一致、context释放。

| 独立实机门槛 | token | 回程（含健康30秒） |
| --- | --- | --- |
| GLES RAM CPU | `908bb4fa328d5cbf` | 38.359秒 |
| GLES原8组＋CA矩阵第9组 | `4946e854d77fe213` | 39.390秒 |
| reference RAM CPU | `59cca10ed12add74` | 38.610秒 |
| 完整native16＋6组共享PNG | `46747c46785bf85c` | 38.375秒 |
| primer RAM CPU | `4ee0c210f31ec8d5` | 38.437秒 |
| 原UI RAM CPU | `e28b2aea410763cd` | 37.906秒 |

完整native16包括资源／queue／AIR compute-render-constants／XR／IOSurface／mipmap／窄整数／half／MRT／sampler／标准program-cache／static-half／Clear-cache／IR3-cache；原字节／padding／guard／fence／撤销都独立核验。IR3 specialize13／29／13实际hits5／4／5、960B原结果和guard一致，最终分配0；进程统计不逐条归属某个kernel。六组共享PNG策略保持原16位像素／ICC／metadata。新proof见 [native](../reproducibility/v730-ca-matrix-r1-native-proof.json) 与 [PNG](../reproducibility/v730-ca-matrix-r1-png-proof.json)。

## 原界面与截图

原 `cb9a39f8708f1faf`／150.032秒和 `206fb7688e2dacf9`／150.047秒两帧均有正常灰色时间、日期、绿色电池、相机和白色homebar，背景仍黑。相比旧 `4dd119fa` 同范围完全无字，新图时钟区域16403个中性灰像素／17018个亮像素，红像素0；该比较证明这两张扫描帧的内容改善，不证明绝对时间准确或所有配置都正常。见 [前后比较](../reproducibility/v730-ca-matrix-r1-clock-scanout-comparison.json)。

原 `206fb768…` 的唯一串口 sample 在确认 metallib 返回后180.047秒执行；同靴slide0x31644000与固定A2S符号图核验，9个线程样本中主线程在 mach_msg→CFRunLoop→UIApplication／ExtensionKit事件循环，另一线程在UIEventFetcher。样本里没有正在编译或draw的GPU调用，不能据此推整靴或所有线程无GPU耗时。原周期初始化样本独立保留。两靴分别38.500／38.860秒完整回程，原STOP未提升，见 [threads](../reproducibility/v730-ca-matrix-r1-original-idle-r2-threads.json)。

原 `881ce1c5…` 120.078秒经同一worker发送一次swipe，内核F12线程确认一次注入；150.031秒捕获720×1280／BGR888／stride2160完整主页。2764800B→925068B／7228块，传输30.311秒，301942B PNG SHA `19fb266d1d4637945eb4ecbae123f40d0fafa13f24f1eb95b8313f5468a1ea30`。所有來源／寄存器／记录-块-整帧CRC／FNV及保存像素复算，power_off=0照实记录。合成手势可验证无人值守UI路由，但不能替代手指触控验收。见 [home](../reproducibility/v730-ca-matrix-r1-home-scanout-observation.json) 与 [scanout](../reproducibility/v730-ca-matrix-r1-original-swipe-r3-scanout.json)。

## 继续方向与未完成

本轮 r1 observer 自身RAMCPU `5754f4626364190d` 通过，GPU `be248be112e4e024` 保持STOP；两条长ABI记录拆分／交错导致原请求核验拒绝，独立截图与39.594秒完整回程仍通过。r2采用短记录／分块ABI，自身RAMCPU `f66de2d4094eec8a` 通过，GPU `61745a1e16c8ab41` 的20条原请求完整：quad_init／quad_init_async／set_device／quad_layout各5，另外9类均0，未见init返回nil。原输入frame375×812、options114／0x72、color_space2、async_nil1。完整来源／截图／回程均复验，原STOP保留；详情与四靴输入在 [METAL-REQUEST-OBSERVATION](METAL-REQUEST-OBSERVATION.md)。

r3仅增12类原场景／队列方法，observer`07d1360de49b9af17f2aad1fed72de5f8db3da92f7526991534aea8664c02a86`／132544B，自身RAMCPU `aa9d5a21aae2f148`通过，38.141秒完整回程。GPU原40请求全部完成，12项ABI／owner／metaclass完整核验；五次共享队列与四次实际队列创建均返回SweetMTLCommandQueue，五次addQuad与五次setScene原参数分别是NUNIQuad／NUNIScene。manager_device与window各8条触及采样上限，不能计成实际总数。paused五条全部YES，disable／enable／sync／tick均0；原postdraw九条仅library与colorsync，无snapshot／PNG。原图SHA`ca2603c424113a64e9ff22d04055e6f0146a2b44b214235949ca0d7e80258e97`，150.063秒捕获、2735块、11.921秒传输，完整来源与保存像素核验通过。见 [route](../reproducibility/v730-quad-route-r3-gpu-route.json) 与 [scanout](../reproducibility/v730-quad-route-r3-gpu-scanout.json)。

锁屏文字和主页扫描帧已改善；壁纸仍黑，原画布仅init／layout，未进入display／prewarm／drawable-size-update，原Aegir没有pipeline／snapshot／PNG请求。共享设备与队列原返回已确认nonnull，继续查原environment、动画启动／快照调用及显示触发。只读20H392原Vista init在`0x1ee946f5c`本就传YES给setPaused，原startAnimation才经_updateAnimation判断动画和显示标志；因此五次paused不能独立当作故障或强制解除。`sharedCommandQueue`属于resource-manager class方法，不能误补SweetMTLDevice；layout也不会更新drawable-size，更新在display开头。观察没有新增绘制工作，不强制draw或用旧靴离屏数据补造本靴壁纸成功。当前新scope没有实际缺binary请求，primer只有CPU准入，不能无请求空跑或称已完整硬件预热。

完整动态壁纸、稳定UI／帧率、睡眠唤醒、LCD光学色偏／背光、声音、Wi-Fi／蓝牙，以及广义texture／depth／stencil／MSAA仍未完成；目标保持active。GPU底层／allocator／SCM／PON／RPC／VM及SweetGPU源码冻结。这里只读帧不是LCD光学实测，power_off=0也不证明背光正常。

此前IR3库 `166c08a3` 原 `87397739…` 26条nonnull管线／三次750×1624draw／三次PNG35558B／53完整postdraw样本以及所有cold-link／check-in失败原样保留在 [IR3-VARIANT-CACHE](IR3-VARIANT-CACHE.md) 和对应reproducibility。不能以其中最好样本宣称整体FPS或当前壁纸完成。

旧STATUS／AGENTS／README／BUILD／OPERATIONS原文及SHA保存在 [archive/20261119](archive/20261119/STATUS-before-ca-matrix-actual.md)；历史每个“当前无in-flight”仅描述当时。现行规则见 [OPERATIONS](OPERATIONS.md)，当前构建见 [BUILD](BUILD.md)。镜像、原日志和截图仍本地保留，正式Git提交源码、锁、复现工具和摘要；gl5／userdata／SD／TA／XFL与救砖输入受保护，不永久删除文件。
