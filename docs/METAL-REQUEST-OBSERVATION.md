# 原 Metal 画布请求观察 — 2026-11-20

此页r1..r6是原请求观察历史，最后有实机route仍r3／3541e079；33方法mode观察没有成功手机记录。用户蓝灯会话已恢复，最新附件验收／软件拒绝与当前设备见[STATUS](STATUS.md)，不从旧“待物理确认”恢复旧任务。

当前壁纸卡点已缩小到原画布初始化后的显示路由。最新r2的五次`CLKUIMetalQuadView`初始化均返回nonnull，原layout执行；观察窗口内没有display、prewarm、drawable-size-update、nextDrawable或pipeline请求。完整720×1280扫描帧仍是灰色锁屏文字与黑壁纸。原GPU结果保持STOP／OBSERVATION_INCOMPLETE，尚未证明壁纸或完整UI完成。

最新r3继续原调用观察，实机确认共享设备与共享队列均nonnull，五次NUNIQuad加入和NUNIScene设置完成；随后原画布五次paused=YES。sync／tick均0，图仍灰字锁屏／黑壁纸。原Vista初始化本来就暂停画布，不能将这一点直接认作故障；下一观察应定位原环境、动画启动或快照路由缺少的触发。生产内核／GLES／Metal保持，r3原STOP与39.109秒完整回程单列。

生产输入保持v730／`f258dee9c6d142b90a573f796eeec5e92b39dd65feed6a21b2542d90fb66a88d`，GLES／`d6d8fcfa27bcac84ab5edb3d2a11c1ed23374e8b491a3e8de093a9a2a819ac2b`，Metal／`5e94ab57401479b42f506884a172a7a839378a82310661a1f66164e99c21f988`，source-map／`7b5a083abdb98fa5d29ef82fbd8e7e68b2a29fd995f16c4163d5c612cb8b9de7`。完整GLES9／native16／PNG6原raw先验来自`4946e854d77fe213`／`46747c46785bf85c`。这轮仅更新诊断observer及其RAM包，没有新增GPU绘制或改变原请求的参数、返回、异常、error和errno。

## 两版观察与四靴

| 独立试验 | token | 原判定 | 回程含健康30秒 |
| --- | --- | --- | --- |
| r1自身RAMCPU | `5754f4626364190d` | PASS | 38.547秒 |
| r1原GPU | `be248be112e4e024` | STOP／OBSERVATION_INCOMPLETE | 39.594秒 |
| r2自身RAMCPU | `f66de2d4094eec8a` | PASS | 38.906秒 |
| r2原GPU | `61745a1e16c8ab41` | STOP／OBSERVATION_INCOMPLETE | 39.375秒 |

r1的两条长ABI记录被拆分／交错，原请求后处理的记录计数拒绝。原raw、拒绝及原STOP全部保留，不补造缺失ABI，不重发旧bundle。独立scanout和四marker回程成功仍单列；见[拒绝](../reproducibility/v730-request-observe-r1-request-postprocessor-rejected.json)。r1主机转发通过只证明主机行为，不能解除手机记录拒绝。

r2改为有界短记录及分块ABI元数据，observer SHA为`4c8e6b2608d39f411c22549437becf621961e94004915680bc5dde249c6a909c`／131520B。全部13方法安装、10条ABI inventory完整，20条请求全部完成，0条未完成；核验仅统计本token的LIVE至ACK，退出期排除。

| 原API | 实际调用数 |
| --- | --- |
| quad_init | 5 |
| quad_init_async | 5 |
| set_device | 5 |
| quad_layout | 5 |
| function、function_constants、function_names、next_drawable、set_drawable_size | 各0 |
| quad_display、quad_prewarm、quad_drawable_size、quad_update_size | 各0 |

五次外层init与五次其转发的async-init均返回原nonnull画布，不能把它们计成十个独立画布。原frame为375×812、options114／0x72、color_space2、async_nil1。没有观察到初始化nil或正在编译阻塞，也不能凭这些缺少调用证明所有GPU线程都没有工作。完整请求与ABI见[原请求](../reproducibility/v730-request-observe-r2-gpu-requests.json)，原draw／PNG观察见[postdraw](../reproducibility/v730-request-observe-r2-gpu-postdraw.json)。

## 实图与原方法关系

r2在观察150.031秒请求一次frame，真实BGR888／stride2160缓冲区2764800B压缩为349908B／2734块，PNG52248B／SHA`3902559110ffee1182d7590f7feec0cbd94a91919e1334853556f0851597ef51`。灰色日期时间、绿色电池、相机和白色homebar可见，壁纸仍黑。来源寄存器、块／整帧CRC、FNV、保存BGR与PNG像素以及39.375秒回程均通过，`power_off=0`照实记录；见[scanout](../reproducibility/v730-request-observe-r2-gpu-scanout.json)。私有图像保留在本轮TEMP，不提交Git，LCD光学／背光／睡眠／FPS未据此验收。

同机20H392原方法的只读研究显示：init会设置CAMetalLayer、frame并调用resource-manager共享队列；layout只更新layer frame和viewport；`_updateDrawableSizeIfNecessary`在display开始才调用。display随后才准备quads、commandBuffer、offscreen、nextDrawable、render、present与commit。当前实际只出现init／layout，因此不能把没有size-update单独认作驱动错误，也不能强行调用draw代替原UI请求。

`sharedCommandQueue`实际是`+[CLKUIMetalResourceManager sharedCommandQueue]`，原地址`0x1c1719a48`。它通过原class锁与weak cache、`[self sharedDevice]`、`[device newCommandQueue]`创建或获取队列。它不是SweetMTLDevice的方法，尚未作生产修改。下一步若观察共享device／queue，应绑定原class／metaclass、owner及完整ABI，分别记录原返回，而不是在错误类上添加同名方法。

本机A2S为iOS16.7.16／20H392，SHA`5bf04681af1819e2379319942a97be635100d6ca3844c5a22e372ce78a83b83d`。原切片／反汇编仅本地TEMP／Mac build，不提交Apple字节。参考`ios_sagit`的iOS18 ABI不能直接套用。

## 构建与复验

源码提交`a9853e1`／`e34ef65`分别记录r1／r2。r2以当前精确GLES RAM与Metal资源重编observer，使用`tools/build/build_exception_observer.py --request-observe --registration-only --postdraw-observe`及所需原native／PNG proof，随后`package_glram.py --presigned --observer --metal-resources`保持生产库字节。主机ObjC转发ASan／UBSan与ARM64编译通过，主机证明仍单列；见[host-r2](../reproducibility/request-observe-r2-build-host-proof.json)。新RAM包自身CPU后再GPU，投递前继续复算payload、Mac实际源码与归档；旧失败包不重发。

原日志复验不打开COM、不重启手机、不覆盖结果，输出必须是新文件：

```powershell
python -B tools/device/metal_request_evidence.py --directory "$env:TEMP\lilac-v730-request-observe-r2-gpu-20261119" --output "$env:TEMP\lilac-request-r2-recheck.json"
python -B tools/device/scanout_trial_evidence.py --directory "$env:TEMP\lilac-v730-request-observe-r2-gpu-20261119" --output "$env:TEMP\lilac-scanout-r2-recheck.json"
```

下一步继续查原Poster／scene如何触发画布显示及resource-manager原返回。内核GPU底层／allocator／SCM／PON／RPC／VM与SweetGPU冻结，单worker、仅BH905ND69G、PON0x06及完整回程约束保持。

## r3共享资源与场景 — 2026-11-20

源码`7a6fc1e`增加`QuadRouteObserve.h`，观察12类原方法，准确owner／class或metaclass／返回及参数ABI核验后安装，每类最多8条、总96条、记录小于190B。原nil、异常、errno、BOOL／double／block参数保持；字符串暂停原因只读且观察getter异常不改变原调用。真实Mac ASan／UBSan、ARM64及原observer组合构建通过。observer为`07d1360de49b9af17f2aad1fed72de5f8db3da92f7526991534aea8664c02a86`／132544B，GLES／Metal源码及二进制未变。构建初次因native-proof位于reproducibility被拒绝，原拒绝保留；精确proof复制到新build目录后构建成功，未有手机重发。

| 试验 | token | 原判定 | 完整回程含健康 |
| --- | --- | --- | --- |
| 新RAM自身CPU | `aa9d5a21aae2f148` | PASS | 38.141秒 |
| 原GPU／route | `3541e07973fe676f` | STOP／OBSERVATION_INCOMPLETE | 39.109秒 |

GPU共40条完整原route请求，0未完成；manager_device／window各8条及明确LIMIT说明采样已封顶，不能作为实际总数。manager_queue5／device_queue4均返回SweetMTLCommandQueue，add_quad5的原参数为NUNIQuad，scene5为NUNIScene，paused5全YES；add_quads／disable／enable／sync／tick均0。原13类request20条也完整，display／nextDrawable仍0，原postdraw9条只有library_bundle／library_url／colorsync，无snapshot／PNG。详见 [route](../reproducibility/v730-quad-route-r3-gpu-route.json)、[requests](../reproducibility/v730-quad-route-r3-gpu-requests.json)、[postdraw](../reproducibility/v730-quad-route-r3-gpu-postdraw.json)、[CPU](../reproducibility/v730-quad-route-r3-ramcpu-actual.json)。

150.063秒捕获真实BGR888／2160扫描帧，2735块／11.921秒，PNG SHA`ca2603c424113a64e9ff22d04055e6f0146a2b44b214235949ca0d7e80258e97`；灰色时间日期、绿色电池、相机与白homebar可见，背景仍黑，原STOP保留。[来源／像素／回程](../reproducibility/v730-quad-route-r3-gpu-scanout.json)全部复验，光学／睡眠／FPS未据此验收。

原方法元数据来自本机20H392 method-list的type／IMP，并与固定A2S的准确symbol start交叉核对，[元数据](../reproducibility/quad-route-20H392-methods.json)不含原二进制。一个初始relative-selector解码器得到无效名称，保留原输出且未用于绑定；准确A2S与实际runtime ABI核验共同准入。原Vista init的`0x1ee946f5c`本就设置setPaused:YES；startAnimation经_updateAnimation检查`_isAnimating`／`_isDisplayOn`才可能解除，故不能把五条YES直接当驱动错误。继续观察原环境更新、原startAnimation／stopAnimation与snapshotImage触发，避免强制绘制改变原策略。

新增构建选项`--quad-route-observe`要求原`--request-observe --snapshot-observe`及其完整先验，保持新包自身CPU→GPU。复验入口只读已有raw：

```powershell
python -B tools/device/quad_route_evidence.py --directory "$env:TEMP\lilac-v730-quad-route-r3-gpu-20261119" --output "$env:TEMP\lilac-quad-route-r3-recheck.json"
```

## r4／r5环境观察与投递拒绝

r4／`6ef54b2`把观察扩展到26类：原`PRPosterEnvironmentImpl`的preview／snapshot／low-luminance、backlight／linear-backlight／unlock double、content CGSize，原Vista start／stop／update／snapshotImage以及Aegir delegate init／update。每类8条／208条总上限，receiver／argument对象ID绑定实际原请求，scalar／size日志保存原64位位型。r5／`2b28508`再增`-[PRRenderer environment]`和返回对象ID，27类／216条上限；用来区分renderer getter与delegate环境是否一致，不额外调用原getter／动画或快照。Mac原ObjC方法、nil／异常／errno／BOOL／double／CGSize／block／环境对象转发ASan／UBSan及ARM64构建通过。准确20H392元数据在[environment](../reproducibility/poster-environment-20H392-methods.json)，host结果与手机观察分别保存。

r4 observer SHA`12773784797c95b0418f5e25ef5bd40ceb96223438f41bed5ddd814172976784`／149920B，自身RAMCPU `93a7b9a5851e3ed3`通过，38.454秒四marker／健康／worker退出；[CPU](../reproducibility/v730-quad-route-r4-ramcpu-actual.json)。其GPU `21d075ecbe605483` guard2/2后USB open报`[Errno 2] Entity not found`，wire实际download0／boot0，未有LIVE／屏幕或环境观察。原controller STOP_ERROR与原wire保留，dispatch1只表示调用发送器，不能计为写了一次USB boot；[拒绝](../reproducibility/v730-quad-route-r4-gpu-transfer-refused.json)。

r5 observer SHA`9e09ba1d9cb0b16ee33348289fa59a330fb6a4921ac7ef54e7b91b75fc676d5e`／149984B，其新RAMCPU `e6788a5d9d05f334`在guard test4退出1，未到boot sender、无wire／LIVE，因此未取得CPU先验，不能发GPU；[拒绝](../reproducibility/v730-quad-route-r5-ramcpu-guard-refused.json)。两版旧失败bundle未重发，原内核／GLES／Metal保持。

之后独立fastboot健康31.688／30.016秒通过，Sony身份／现有配置／bulk描述符正常，单次claim／release成功，期间download／boot／bulk读写／reset均0；[USB](../reproducibility/sony-usb-open-observation-20261120.json)。这些只读观察不解释前次竞态，也不提升失败试验。原已patch未boot的loader状态与guard交互需离线核对，不能继续换token重复失败。已验`boot_once.py`SHAe862e430保持，独立`usb_open_observation.py`提供准确打开阶段诊断。手机当前fastboot，无in-flight。

## r6原模式分支与准确采样选项

2026-11-20对准确gl5原root只读重建／挂载，固定A2S、原Aegir与cache header逐SHA核对，原输入无修改，分析后已卸载。新`inspect_poster_routing.py`从实际20H392读九处代码byte锚点、原relative method list六项type／IMP、A2S精确函数起点及四字节跳转表。工具只提交源码、SHA、metadata与结论，原Apple切片／反汇编保持本地build，不提交；实际[static proof](../reproducibility/poster-routing-20H392-offline.json)。

`applyMode:`／`animateTransitionToMode:`是signed NSInteger／`v24@0:8q16`，原跳表明确0=activeMode、1=deactiveMode、2=interactiveMode、3=supplementalMode。active／interactive／supplemental调用Vista startAnimation，deactive调用stopAnimation；Vista init已置_isDisplayOn bit0x02，start置_isAnimating bit0x04，_updateAnimation同时检查两bit才进入_startAnimation并setPaused:NO。原Aegir自己选择模式，不能让observer强制active或取消paused。

ClockKitUI `_msaaCount`本身返回4，但init只有options0x08置位才使用它；没有该bit时sample1。options0x01另决定depth，实际depth descriptor是252／RenderTarget4／Memoryless3。独立从r3原raw核验十条quad_init／async_init全部options114／0x72、color_space2，因此当前scope没有请求MSAA或depth；这些是更广的功能缺口，不能解释这个黑壁纸。Memoryless／MSAA实作继续，静态不冒充本靴运行参数或面板背光。

r6 route revision4再增加Vista controller apply_mode／transition_mode／active_mode／deactive_mode／interactive_mode／supplemental_mode，共33方法、每类8条／264上限。所有NSInteger保留原64位位型，无额外mode getter／scene／GPU调用；原owner、ABI、nil-return、throw和errno逐项转发。Mac真实ObjC ASan／UBSan、signed极值与无符号ABI拒绝测试通过；Python10项原记录／缺块／signed-bits／ACK后拒绝通过，旧r1..r3仍可独立复验。

编译observer`de4fc867883c1d731fe2b2819646d17fb5b38dc9faa4d550e3bb204e86ddb500`／150304B ARM64完成，与旧CA基线配CPU b2a5bc908eec01c2／image7fa57a71只进行offline。run_trial严格拒绝六份compiled源码与当前Depth32-Stencil8源码不一致，下载／boot均0，原拒绝保持。不能绕过当前source audit来测试旧库；当前新附件完整reference与新CPU c10521e2已offline源码审计通过，先其自身CPU→GLES9→referenceCPU→native18／PNG6→新proof，再构建原模式包自身CPU／GPU。新包源码可构建和真机准入分别验，见[offline proof](../reproducibility/poster-route-r6-offline-proof.json)。

Mac原模式observer构建沿完整本版本native／PNG proof，`--registration-only --register-device --pipeline-observe --postdraw-observe --snapshot-observe --request-observe --quad-route-observe --png-filter-encoder`；输出必须新建，绑定当前准确Metal库，不能拿旧16组proof准入新18组库。静态复现入口，在已经只读挂载的准确原gl5上运行：

```sh
python3 -B tools/build/inspect_poster_routing.py --root build/my-original-root/root \
  --symbols assets/symbols/ios16-20H392-root.a2s --ipsw /path/to/local/ipsw \
  --output build/my-poster-routing-proof
```

本轮使用blacktop/ipsw3.1.749的普通macOS x86_64公开release，archive b141b419／binarye90ac1e0与release checksum匹配，只有本地静态读取。该r6离线轮结束时手机fastboot／无worker、尚待物理会话确认；后续会话恢复和附件结果见STATUS，不复发r4／r5；完整壁纸／UI／FPS／睡眠／其它外设目标保持。
