# 当前状态 — 2026-11-20

Sony BH905ND69G 当前停在 fastboot，无 controller／串口 worker／in-flight。最新成功进入内核的是r4自身RAMCPU `93a7b9a5851e3ed3`／38.454秒完整0x06回程。随后r4 GPU在传输前USB open拒绝、r5 RAMCPU在guard test4退出，均未启动新内核，原STOP_ERROR保留；失败后独立连续健康30.016秒及一次无bulk传输的接口claim／release观察正常，旧失败包不重发。最近实际GPU仍是场景／队列r3 `3541e07973fe676f`：40请求完整、共享队列与场景有效，但sync／tick／display／nextDrawable／pipeline未发生，150秒灰字锁屏／黑壁纸，原STOP／OBSERVATION_INCOMPLETE保持。前轮`881ce1c5d844e7ba`正常颜色主页证据保留。

前一离线候选为[MSAA附件r1](METAL-MULTISAMPLE.md)：2×／4×颜色-depth-stencil、Private／Memoryless原页、颜色平均及Depth32 Sample0／Min／Max GPU resolve。GLES21de6821／14784032B、Metalb25bfe6c／807568B／source-map149d4577（47源）、reference34219504／238384B、blob4295582B／188cf328；两自身CPU包87c89bcf／0ed40224和386c4f42／4e46f3d6只offline同核private／payload／Mac83源audit通过，未投递。实际作者AIR→ESSL在主机Mesa48draw／5508独立sample／26928B／8resolve通过UBSan；Mac原生对象与18阶段作者probe本体ASan／UBSan通过，90记录／151488B／55098Bguard被独立Python核验。host没有运行FD540 importer；新native20／PNG6手机证明仍待完成，旧native19拒绝。原r4／r5失败和最新用户“暂时不方便，继续离线”保持，本轮download／boot／reset0。见[MSAA offline proof](../reproducibility/multisample-attachment-r1-offline-proof.json)。

当前源码最新候选为[原生presentation-r1](METAL-PRESENTATION.md)：原drawable／framebufferOnly标志、四present方法及GPU原IOSurface synchronize→seed发布；实际操作和页发布后才Scheduled，修正原20H392 ClockKitUI事务等待后直接present的顺序。GLES21de6821保持，Metal8feea459／810304B／source-mapdb1605ef、reference33bf5ee4／256656B、blob4304475B／7a3361f6；新CPU367bebc6／177af34a只offline同核private／payload／Mac83源audit通过，未投递。Mac原生对象／queue FIFO／callbacks／original seed／精确time-options／异常回收及MSAA作者回归ASan／UBSan通过；host drawable fixture不代替原手机对象。native21新Metal_presentation必须实际原CAMetalLayer／CAMetalDrawable／IOSurface像素-padding／import-retire-publish／fence，旧native20拒绝；完整原UI／FPS／光学及其它外设仍缺。用户继续离线，手机download／boot／reset0；静态只读原root已卸载。见[presentation offline proof](../reproducibility/presentation-r1-offline-proof.json)。

已验正式基线为 CA 矩阵修订 r1：

前一离线候选为[Memoryless附件r1](METAL-MEMORYLESS.md)，在Depth32／Stencil8上增加标准Memoryless颜色／深度／模板公开元数据与每encoder独立私有页。GLES6211bac2保持，新Metalf4aa2b35／789024B、source-map31478965、reference observerb083c28f／237216B、RAM blob4280557B／692b62e9。Mac完整真实原生对象ASan／UBSan与iOS ARM64库／探针构建通过；WSL真实Mesa八阶段19draw／10404B颜色-depth-stencil读回UBSan通过。CPU a6ed4392a66f7af4／imagec1539667已offline同核private／payload／Mac83源audit，尚未发送，旧native18不能准入新native19。先前候选及下表已验基线均保留；详见[新offline proof](../reproducibility/memoryless-attachment-r1-offline-proof.json)。

首轮host实际抛isFramebufferOnly缺selector异常，原日志与失败保存；已補标准getter后真实回归通过。公开Memoryless无CPU backing，跨pass Load／persistent Store／sampling／mipmap／IOSurface拒绝；每pass内部原页保留到命令fence，双queue并发页面不同，partial allocation和GPU失败回收实际主机验过。新8phase需19456B／10888Bguard、11原pass页／handle-IOVA与真实backend import-retire核验，不能以host stub或未发送bundle宣称手机Memoryless或壁纸恢复。

正式源码另有尚未手机准入的Stencil8附件r2候选，GLES6211bac2／Metalf40f796b／source-mapb86e29eb，仅离线构建和主机验证通过，未替换下表的已验基线。它在Depth32上增加独立模板页面、准确Mesa separate-z32s8引用转交、独立Load／Clear与front-back状态快照；真实host Mesa2390draw／247860像素UBSan、15类pair回滚和Mac原生对象ASan／UBSan通过，iOS ARM64前端／库／完整native18 probe构建通过。CPU bundle3ec7416adc2cfeea／image5567d656已offline同核private／payload／Mac83源audit，仍未投递。该旧候选当时的MSAA缺口已由新离线r1补窄范围；sampled depth-stencil／depth-stencil-XR有序组合仍缺，见 [METAL-DEPTH-STENCIL](METAL-DEPTH-STENCIL.md) 和 [新offline proof](../reproducibility/stencil-attachment-r2-offline-proof.json)。

本轮WSL ASan停在libasan启动阶段，未进入Mesa，原失败与PC／模块／符号保留；最终host GLES只宣称UBSan通过。11,881,859,070B重复日志已实际进入Windows回收站，首4096B、原SHA及失败元数据保留，回收站未清空；新host运行限45秒、日志1MiB，超量边界实际核验通过。旧Depth32离线证明保持原样，不能拿上轮ASan结果替代本轮。

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

2026-11-20针对串口截图问题，重新从881ce1c5主页与3541e079锁屏的原raw／results／artifact／plan／commands／events独立核验：两个来源、完整像素、PNG SHA和四marker回程均通过，原STOP未提升。图片实际捕获于11月19日；当前无需DroidCam也能继续UI内容和颜色数值取证，LCD光学／背光／物理错行另验。没有为此次复验发送新frame或boot，r4／r5失败后尚待新的物理fastboot会话确认。

## 继续方向与未完成

原模式观察r6只增Vista controller六方法，33方法／264记录／signed NSInteger原位型，Mac ASan／UBSan与ARM64编译通过。observer de4fc867／150304B配旧CA库CPU b2a5bc90在offline源码audit被拒绝：六份已编译GLES／Metal源码与当前Depth32-Stencil8不同，手机下载／boot0。拒绝原样保留，不绕过审计；当前源码匹配完整附件reference重新编译仍7e164ad1、blob4274554B／90dc4934，CPU c10521e2f389104f／image67656595已offline同核private／payload／Mac83源通过，尚未发送。先新附件完整native18／PNG6，随后模式observer必须绑定新proof。详见 [本轮offline](../reproducibility/poster-route-r6-offline-proof.json)。

固定20H392新增[原模式／采样静态分析](../reproducibility/poster-routing-20H392-offline.json)，从准确gl5只读挂载核验9个代码SHA、6个原方法type／IMP和A2S；原root已卸载。applyMode跳表0=active／1=deactive／2=interactive／3=supplemental，active／interactive／supplemental原调用startAnimation，deactive停止。Vista init已置显示bit而初始画布paused，后续动画bit才进入原_startAnimation。静态不能证明本靴是否调用哪个mode，新observer记录实际原参数，不强制改变模式或暂停状态。

原ClockKitUI的options0x08决定4×MSAA，0x01决定depth；独立复验r3十条init实际options0x72，当前该scope两者未请求，不能把4×MSAA缺口当本轮黑壁纸原因。其它配置depth明确252／RenderTarget4／Memoryless3，现候选Memoryless、MSAA及sampling仍未接入。既有shader／GPU／系统基线不因静态分析提升；手机仍fastboot、无worker、待新物理会话确认，本轮所有工作手机download／boot／reset0。

本轮 r1 observer 自身RAMCPU `5754f4626364190d` 通过，GPU `be248be112e4e024` 保持STOP；两条长ABI记录拆分／交错导致原请求核验拒绝，独立截图与39.594秒完整回程仍通过。r2采用短记录／分块ABI，自身RAMCPU `f66de2d4094eec8a` 通过，GPU `61745a1e16c8ab41` 的20条原请求完整：quad_init／quad_init_async／set_device／quad_layout各5，另外9类均0，未见init返回nil。原输入frame375×812、options114／0x72、color_space2、async_nil1。完整来源／截图／回程均复验，原STOP保留；详情与四靴输入在 [METAL-REQUEST-OBSERVATION](METAL-REQUEST-OBSERVATION.md)。

r3仅增12类原场景／队列方法，observer`07d1360de49b9af17f2aad1fed72de5f8db3da92f7526991534aea8664c02a86`／132544B，自身RAMCPU `aa9d5a21aae2f148`通过，38.141秒完整回程。GPU原40请求全部完成，12项ABI／owner／metaclass完整核验；五次共享队列与四次实际队列创建均返回SweetMTLCommandQueue，五次addQuad与五次setScene原参数分别是NUNIQuad／NUNIScene。manager_device与window各8条触及采样上限，不能计成实际总数。paused五条全部YES，disable／enable／sync／tick均0；原postdraw九条仅library与colorsync，无snapshot／PNG。原图SHA`ca2603c424113a64e9ff22d04055e6f0146a2b44b214235949ca0d7e80258e97`，150.063秒捕获、2735块、11.921秒传输，完整来源与保存像素核验通过。见 [route](../reproducibility/v730-quad-route-r3-gpu-route.json) 与 [scanout](../reproducibility/v730-quad-route-r3-gpu-scanout.json)。

r4／r5补原environment／animation观察，源码`6ef54b2`／`2b28508`，Mac ASan／UBSan转发与iOS ARM64编译通过，生产GLES／Metal及kernel保持。r4 observer12773784／149920B自身RAMCPU `93a7b9a5851e3ed3`通过；GPU `21d075ecbe605483` 在guard2/2后USB open报Entity not found，wire明确download0／boot0，没有LIVE／截图。r5 observer9e09ba1d／149984B增加原PRRenderer.environment／return identity，RAMCPU `e6788a5d9d05f334` 在guard test4退出1，无boot sender／wire／LIVE，未取得CPU准入。两次STOP_ERROR完整保留，不能把controller dispatch计数当USB boot写入，也不能发r5 GPU。见 [r4拒绝](../reproducibility/v730-quad-route-r4-gpu-transfer-refused.json)、[r5拒绝](../reproducibility/v730-quad-route-r5-ramcpu-guard-refused.json)。

拒绝后Sony仍0fce:0dde／BH905ND69G、原fastboot健康31.688／30.016秒；描述符配置1、bulk端点1／129、packet512正常，单次接口claim／release正常，bulk读写／download／boot／reset均0。独立 [USB观察](../reproducibility/sony-usb-open-observation-20261120.json)证明当前能打开接口，不能解释首次竞态或提升失败包。正式`tools/device/usb_open_observation.py`记录身份／配置／描述符／claim失败阶段并仅在--claim-once单次打开释放；单worker锁保持，已验boot_once源码SHAe862e430未改。下一步离线核对loader仍处于已patch未boot状态的guard行为及原环境／快照触发，停止用新token重复同一失败。

锁屏文字和主页扫描帧已改善；壁纸仍黑，原画布仅init／layout，未进入display／prewarm／drawable-size-update，原Aegir没有pipeline／snapshot／PNG请求。共享设备与队列原返回已确认nonnull，继续查原environment、动画启动／快照调用及显示触发。只读20H392原Vista init在`0x1ee946f5c`本就传YES给setPaused，原startAnimation才经_updateAnimation判断动画和显示标志；因此五次paused不能独立当作故障或强制解除。`sharedCommandQueue`属于resource-manager class方法，不能误补SweetMTLDevice；layout也不会更新drawable-size，更新在display开头。观察没有新增绘制工作，不强制draw或用旧靴离屏数据补造本靴壁纸成功。当前新scope没有实际缺binary请求，primer只有CPU准入，不能无请求空跑或称已完整硬件预热。

完整动态壁纸、稳定UI／帧率、睡眠唤醒、LCD光学色偏／背光、声音、Wi-Fi／蓝牙，以及广义texture／depth／stencil／MSAA仍未完成；目标保持active。GPU底层／allocator／SCM／PON／RPC／VM及SweetGPU源码冻结。这里只读帧不是LCD光学实测，power_off=0也不证明背光正常。

此前IR3库 `166c08a3` 原 `87397739…` 26条nonnull管线／三次750×1624draw／三次PNG35558B／53完整postdraw样本以及所有cold-link／check-in失败原样保留在 [IR3-VARIANT-CACHE](IR3-VARIANT-CACHE.md) 和对应reproducibility。不能以其中最好样本宣称整体FPS或当前壁纸完成。

旧STATUS／AGENTS／README／BUILD／OPERATIONS原文及SHA保存在 [archive/20261119](archive/20261119/STATUS-before-ca-matrix-actual.md)；历史每个“当前无in-flight”仅描述当时。现行规则见 [OPERATIONS](OPERATIONS.md)，当前构建见 [BUILD](BUILD.md)。镜像、原日志和截图仍本地保留，正式Git提交源码、锁、复现工具和摘要；gl5／userdata／SD／TA／XFL与救砖输入受保护，不永久删除文件。
