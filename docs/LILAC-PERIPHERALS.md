# Lilac 外设：背光接口与现有日志

[R25 外设证据](../reproducibility/r25-peripheral-observations-20261123.json)仅取自原串口当前 `LIVE` 到 `ACK`，并复验原完整 payload、返回及扫描帧。R25 的 GPU 阶段通过不代表外设通过。

| 现象 | R25 记录 | 证据边界 |
| --- | --- | --- |
| 显示逻辑状态 | `on → off → on`，行 2405、2893、9935 | `AppleCLCD` 消费系统请求，更新软件标志和重绘；未读取 WLED 或测量 LCD |
| 系统背光接口 | `backlight-control` 匹配四次无结果，`IODeviceTree:/backlight` 查找一次无结果 | 缺失的是这些请求所找的服务或节点，不能据此判断 PMIC 是否已点亮面板 |
| Wi-Fi | SpringBoard 的 `wlan` 服务查找无结果 | 无扫描、连接或传输证据 |
| 声音、蓝牙 | `mediaserverd` 一次、`bluetoothd` 十六次 `exec` | 进程活动不能证明音频路由、蓝牙控制器初始化或连接 |

## 音频启动边界

[ADSP 原始证据](../reproducibility/lilac-adsp-prepare-20261124.json)复核了 R25、R26 两次 RAM CPU 启动的相同 stage 3：固件核验通过，PIL 接口 1、2、5 可用，AP 端共享内存准备完成。两靴的日志均明确未加载、认证或启动 DSP。R25 GPU 启动参数未启用该探针，镜像内包含固件也不能算音频启动。

固定 Sony 开源参考中，`msm8998.c` 为 Tasha 和 Tavil 分别选择编解码器，均有 `SLIMBUS_0_RX` 回放路径；`q6afe.c` 和 `q6asm.c` 分别连接 ADSP 的 AFE 端口与 ASM 音频流。原厂 DTB 同时列出两套候选，因此不能直接套用某一编解码器的初始化。下一步要单独验证 DSP 启动、APR 响应与实际编解码器，之后才验证 PCM 播放和物理输出。stage 3 不作为这些功能的准入。

[启动依赖源码审计](../reproducibility/lilac-adsp-boot-source-audit-20261126.json)从锁定formal patch恢复准确`sweet_sample.c`／`sweet_rpm.c`，对照Sony固定commit的PIL loader、LPASS DT、GCC clock和调压宏。上游LPASS代理资源为vdd_cx TURBO384和19.2MHz XO，后者的parent是RPM SMD；PIL init／auth另先申请SCM带宽。当前cold pilot依次要求RPM电压active／sleep、XO／CE1／aggre2 helper和master／slave带宽，失败分别返回43／44／45，尚未进入PIL init。因此新增PCM接口不绕过RPM前置依赖，也不能从源码推断当前RPM实机健康。

现有stage4～7在Auth／SMP2P ready和可选GLINK／APR观察后调用PIL shutdown；成功返回是一次已收尾的诊断，不是供PCM长期使用的DSP实例。生产音频还需要明确的DSP供电／启动／ready／播放／关闭生命周期及失败清理，不能把前一probe的ready或静态auth标志作为本运行时的当前readiness。LPASS节点只列XO代理时钟，当前复用GPU helper所带CE1／aggre2不构成它们是ADSP必需资源的证明；本次没有修改冻结RPM／SCM或尝试删减时钟。

固定Git中`arch/arm64/boot/dts/qcom`是symlink；审计直接读取`arch/arm/boot/dts/qcom`的实际Git blob，并保存链接blob／target，避免把Windows展开路径误当独立上游源码。复现入口见BUILD。审计只读取Git和本地文件，没有内核构建、PIL调用、设备启动或声音证据。

[AFE SLIMBUS协议](../reproducibility/lilac-afe-slimbus-playback-reproducible-20261125.json)继续使用同一固定Sony开源源。`LilacAfeSlimbusPlayback.h`只构造SLIMBUS_0_RX配置／start／stop与管理原回复状态，没有接入硬件。原q6afe发送36B `union afe_port_config`，其中SLIMbus member24B、余12B清零；V2／V3总包84／92B，start／stop24B，APR源／目标端口均0，AFE port0x4000在payload。模式需调用方提供已验证的DSP参数schema和codec channel map，不能凭Tasha／Tavil节点选值。Accepted不提升为Running；错误／不确定后拒绝late reply和后续发包，stop须先确认播放detached。

正式可复现主机工具从`xnu/lilac-current.patch`和source-lock重建两份APR头，原CRLF只在准确SHA匹配时恢复，不依赖旧build目录。WSL／Mac实际ASan／UBSan、原包／guard／token／epoch／错误边界通过，ARM64对象也已编译；DSP、codec、实机包回复及物理声音仍未验。

```powershell
python -B tools/build/check_lilac_afe_slimbus.py --output build/新的-afe-host
```

[统一播放会话](../reproducibility/lilac-pcm-session-offline-20261126.json)使用同一Sony固定提交`33dc678e…`，把AFE、ADM输出／路由和ASM开流／映射组合成一条有明确确认顺序的协议流程。`LilacPcmPlaybackSession.h`只拥有协议状态；调用方另行提供已经验证的DSP、codec、topology、SLIMbus channel map、参数schema和4KiB对齐36位地址区域。准备好本epoch后依次确认开流、S16／48k格式、映射、AFE配置／启动、COPP、路由及Run，之后才允许四个互不重叠的PCM缓冲。

关流须先停止新写入，等待全部匹配WRITE_DONE，再发送`ASM_DATA_CMD_EOS`（0x10bdb）。Sony `msm-pcm-q6-v2.c`把`ASM_DATA_EVENT_RENDERED_EOS`（0x10c1c）作为独立播放收尾信号；普通EOS回复或`ASM_DATA_EVENT_EOS`（0x10bdd）均不替代它。会话允许准确普通回复和rendered事件先后两种顺序，两个条件都满足才Close→Unmap→关闭COPP→AFE Stop。全部停止确认后才发布region_release_confirmed；发送状态不明、超时或DSP错误后保留区域所有权，不信任晚到成功。这里验证的是协议条件，尚未听到实际声音，固件EOS token回显也仍需核验。

新正式工具`check_lilac_pcm_session.py`从锁定patch重建APR头，WSL和Mac同源七组ASan／UBSan已通过，覆盖准确包／guard、epoch／token、Accepted／typed回复、四缓冲排空、EOS两种顺序、全部13步命令错误及在途WRITE错误。Mac `--arm64-object`另编译`compile_lilac_pcm_session.cpp`的全部入口，产物为ARM64 Mach-O，未链接内核或接通传输。复现命令见[BUILD](BUILD.md)，新音频会话和流support保留GPL-2.0-only标识，固定参考Git blob与COPYING列于报告。

[串行GLINK适配](../reproducibility/lilac-pcm-glink-transport-offline-20261126.json)将该session接到原正式patch的ADSP::Link，未改底层传输实现。直接session sink在Map普通ACK后返回NotReady，原Link会fault；对照测试已复现。`LilacPcmPlaybackTransport.h`把合法待完成回复转换成Link可消费的OK，session继续等准确最终回复。分段GLINK包只有拼成完整APR后才处理，不接受未发布控制包或WRITE的伪完成。

适配层在调用方确认本epoch的DSP／codec／link就绪后串行推进；待发APR字节和token保持，只有Link返回Busy（保证未发布）允许再发送相同包。成功publication后记录控制／PCM deadline，Accepted／普通Map ACK不刷新它；IO错误可能发生在FIFO／head已发布后，绝不重试或释放区域。四个已发布WRITE分别限时，session排空／rendered EOS／Stop完整成功后还要等GLINK CloseAck和远端Close，最终才确认region可释放。超时、时钟倒退、epoch或readiness变化保留所有权，需要调用方独立恢复，适配层不启动或复位DSP。

原revision2复现编译八组ASan／UBSan，直接使用锁定的GLINK／intent／RX ACK代码和模拟FIFO，WSL／Mac准确源一致；session与transport两个ARM64 Mach-O通过。其transport对象留下四个FIFO外部函数未定义，是原接入边界，不算实际SMEM IO。四份APR头来自正式patch，混合行尾按锁定before／after SHA复建；首次仅CRLF重建的拒绝保留，未改原内核源。

[具体SMEM FIFO](../reproducibility/lilac-pcm-smem-fifo-offline-20261126.json)实现了发送、available、peek和consume，沿用正式sweet_adsp_apr_ops回调。固定Sony `glink_smem_native_xprt.c`与原SweetADSPAPR代码均为来源：16KiB双环、8 B对齐、TX留16 B；AP只写descriptor+4的TX head和descriptor+8的RX tail，不写远端cursor、RX数据或reset。发送时先复制并检查完整GLINK frame，空间不足只返回Busy且无IO写入；写完数据并fence后才发布head和notify。发布cursor或通知失败保持不确定，不能重试。

RX使用有界snapshot，全部word read成功才输出，保留调用方外围guard；只准consume最近成功观察的完整frame大小和同一tail。部分／损坏prefix可交给原Link拒绝，不能提前消费。主机验证覆盖全部2048个TX／RX环位置、最大4112 B包、partial／格式错误、逐word IO失败、head／tail可能已更新后的失败和通知错误。完整PCM→原Link→具体FIFO在模拟SMEM中走两种EOS顺序及四个缓冲乱序完成，最后双向Close通过。

原check_lilac_pcm_session.py revision3从正式patch复建五份准确APR／GLINK／回调ABI头，WSL／Mac同源十组ASan／UBSan通过。三个ARM64对象已编译；新增具体SMEM对象只留下正常stack guard符号，无四个未实现FIFO函数导入。代码没有加入内核，真实SMEM映射、uncached／屏障语义、mailbox回调和DSP／codec准入仍待连接。没有实际音频播放、DSP启动或手机IO证据；当前运行范围见STATUS，现行复现命令见BUILD。

[保留运行时与缓存交接](../reproducibility/lilac-pcm-runtime-cache-final-20261126.json)把FIFO、原ADSP::Link和transport放入单个60,352 B对象，由owner在内核栈外保留并串行调用。readiness回调每次给出准确epoch、独占endpoint、uncached SMEM、DSP及codec状态；PCM CPU映射／device地址、字节长度、明确WriteCombined或CacheMaintenance策略和cache-line边界在任何FIFO IO前检查。构造不会映射／分配／复位硬件。

queue_pcm在准确Run确认后检查四份独立cache-line跨度，复制实际双声道S16字节，然后完成release-to-device回调，才排入同一WRITE。旧span仍在途时拒绝覆盖；成功WRITE_DONE还要完成acquire-from-device回调才复用。回调失败或readiness／epoch变化保留跨度与整段region。给出的sync回调必须完成真实clean／barrier或write-combined发布语义，当前测试用独立CPU／DSP字节视图验证顺序，没有执行实际cache指令。

transport新增当前clock hook，在receive处理前、poll后、send／close后及cache回调后重采样。原publication／reply／WRITE／关闭deadline不延长；IO内迟到回复、迟到CloseAck和cache回调阻塞超过另一在途WRITE期限均拒绝。原工具revision4编译十一组ASan／UBSan和四个ARM64对象，WSL／Mac源逐字节一致；原peer提取为共享测试头，重叠源数据、外来token和callback失败均检查。具体runtime对象无未实现FIFO导入，未加入当前kernel音频service，也没有DSP／codec／实际声音证据。

[C11运行时接口](../reproducibility/lilac-pcm-c-abi-offline-20261126.json)提供construct／start／advance／queue／drain／status／uncertain／destroy，实际实现是`xnu/support/LilacPcmRuntimeAPI.cpp`，不分配内存。当前ARM64／host context为60,624 B、8 B对齐；调用方以context_bytes／context_alignment查询需求，在内核栈外提供清零、不可搬移的存储。配置和回调表复制入context，PCM mapping和两类backend context继续由owner保留；复制出的context因地址身份不符被拒绝。构造纯验证全部FIFO和PCM布局／schema／policy／回调，任何失败不执行IO。

start先核对准确readiness和真实clock，之后由同一个外部串行owner调用advance推进原GLINK／会话；queue只在Run确认后交接实际PCM字节。回调中重入所有C接口返回Busy，不能把此检查当跨线程锁。token及snapshot输出不能覆盖context或PCM、不得未对齐。只有未启动，或完整排空／EOS／Close／Unmap／COPP／AFE和双向GLINK关闭且region可释放时，destroy才结束并清零对象；任何在途／uncertain状态继续保留，不释放物理页。

现行工具revision5以独立C11 caller和实际C++实现链接复验十二组ASan／UBSan，Mac另编译六个ARM64对象；具体API只留下stack guard导入，C caller只导入准确公开C符号。八种policy／schema／EOS顺序的四缓冲完整流程、表复制、错误配置、输出alias、context复制、重入及发布／cache失败保留均通过。真实mapping／cache指令／mailbox／DSP／codec及audio service没有接通，声音仍待真机验证。

[PCM 负载核验](../reproducibility/lilac-asm-pcm-offline-20261124.json)实现了无时间戳、双声道 16 位 PCM 的 `WRITE_V2` 负载编码。`WRITE_DONE_V2` 必须匹配端口、token、连接代次、缓冲区地址和映射句柄，且状态为零，才能判为 DSP 消费完成。主机测试覆盖准确字节、越界和外来回复。当前代码不映射内存或发包。复验可在 WSL 用 `cc -std=c11 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -fno-sanitize-recover=all -no-pie tools/build/test_lilac_asm_pcm.c -o build/新的-pcm-test` 编译并运行。

[PCM 在途队列](../reproducibility/lilac-asm-pcm-queue-offline-20261124.json)最多保留四个待确认缓冲区。DSP 地址范围重叠时拒绝再次提交；匹配的成功完成只释放一次，旧连接或重复完成不会释放新提交。DSP 错误、超时或发送状态不明后，连接应停止接收新请求并保留待确认缓冲区，直到独立确认 DSP 不再访问它们。主机复验沿用上面的编译选项，将输入换成 `tools/build/test_lilac_asm_pcm_queue.c`，输出使用新路径。

[APR 适配层](../reproducibility/lilac-asm-pcm-apr-offline-20261124.json)复用现有 XNU APR 编解码器，构造 52 字节写包，并在来源域、服务和完成事件格式通过校验后才把 36 字节完成包交给队列。接入时须由传输层提供连接代次、串行调用，并实机验证异步 token 回显及 DSP 映射。离线复验可用准确重建的 v730 源码树，在 WSL 编译 `tools/build/test_lilac_asm_pcm_apr.cpp`：用 `c++ -std=c++17` 代替前面的 `cc -std=c11`，保留 sanitizer 选项，并以 `-I <v730-xnu>/iokit/Kernel/arm` 引入已锁定的 APR 头文件。

[PCM 映射管理](../reproducibility/lilac-asm-pcm-map-offline-20261124.json)为一段连续、4 KiB 对齐的物理内存构造 Sony ASM 映射和解除映射请求。只有匹配的回复返回非零映射句柄后，才允许生成该区间内的 PCM 写入请求。在途队列排空前拒绝解除映射；映射状态不明或解除失败后保留所有权，不接受晚到的成功回复。离线复验沿用 APR 测试的编译参数，改用 `tools/build/test_lilac_asm_pcm_map.cpp`。实际页面分配、缓存一致性、连接串行化和固件回复仍由后续硬件接入验证。

[PCM 流状态](../reproducibility/lilac-asm-pcm-stream-offline-20261124.json)把开流、双声道 48 kHz S16 格式、映射和启动串到同一个离线流程。[ADM 路由确认](../reproducibility/lilac-adm-playback-route-offline-20261124.json)须属于本连接和会话，才能生成 `Run`。一份确认只允许启动一次；失去路由确认后停止提交新缓冲区。匹配的 `Run` 成功回复到达后才允许写入。停止提交后须等在途缓冲区消费完，再分别确认关闭与解除映射，才能释放该区域。发送超时或状态不明时，调用方须标记状态不确定并保留所有权。复验沿用 APR 测试的编译参数，改用 `tools/build/test_lilac_asm_pcm_stream.cpp`。

[ADM 输出对象](../reproducibility/lilac-adm-playback-device-offline-20261124.json)根据调用方提供的已核验 AFE 端口和拓扑生成 OPEN_V5 请求。只有匹配的专用回复才能提供 COPP 句柄；普通确认不算打开。路由绑定这次打开的连接和请求，关闭或重新打开后旧确认失效。关闭请求沿用 Sony 的 AFE 源端口和 COPP 目标端口，须先由调用方确认播放已脱离输出。实际 AFE／编解码器配置、固件回复端口和声音仍待真机验证。

[2026-11-25 正式源码复验](../reproducibility/cache-budget-peripheral-host-validation-20261125.json)重新编译了 APR／映射／流、WLED 读数／调光事务和背光参数主机用例，ASan／UBSan 通过。首次命令遗漏了已在上文说明的 APR include 路径，原编译失败保留；随后按 `source-lock.json` 核对两个 APR 头文件 SHA，再以原 `-I` 参数复验。它们来自正式 `xnu/lilac-current.patch` 的准确展开，不是新源码修补或独立来源。

复验命令：

```powershell
python -B tools/device/adsp_prepare_evidence.py --directory "$env:TEMP\lilac-poster-cache-r26-ramcpu-20261124" --output build/新的-adsp-prepare.json
```

## Sony WLED 来源

参考已有 `sonyxperiadev/kernel` 开源检出固定提交 `5772572ccdfbc16c270d33f8fa6b55d33d27709c`，路径和 Git blob 如下。这里记录硬件契约，未复制 GPL 驱动代码。

| 固定文件 | Git blob | 从文件核对的契约 |
| --- | --- | --- |
| `arch/arm64/boot/dts/qcom/dsi-panel-somc-lilac-base-sde.dtsi` | `b892f5fda9e9ef7cb9d3ea3ddb1952ae8f5f1358` | Lilac 面板选 `bl_ctrl_wled`，非零亮度范围 1–4095 |
| `arch/arm64/boot/dts/qcom/msm8998-yoshino-lilac_common.dtsi` | `305969261e67929fc51d278c73304771f7282a44` | 对 PMI8998 WLED 有 Lilac 独立电流及亮度表覆盖，不能套用其它 Yoshino 机型的值 |
| `arch/arm64/boot/dts/qcom/pmi8998.dtsi` | `0479c46fa7364e7cfa995e80068076c9f7df0a55` | `qcom,pmi8998-wled` provider，含独立控制和输出资源及保护中断 |
| `arch/arm64/configs/aosp_yoshino_lilac_defconfig` | `95e808cf82b53cd53b15e54e4285a572a8b41091` | 选用 `CONFIG_BACKLIGHT_QCOM_WLED=y` |
| `drivers/video/backlight/qcom-wled.c` | `743d3d21e611979f039817463f31c3d481b1b6d9` | PMI8998 匹配 WLED 4.1；驱动在 backlight update 中处理亮度和消隐 |
| `drivers/gpu/drm/msm/dsi-staging/dsi_panel.c` | `786d2a63598b2521cf8a2a5a83aae49b8caabc1d` | WLED 由 raw backlight provider 提供；找不到时延后 probe，而非改用 DCS 亮度路径 |

[原厂 SO-02K DTB 报告](../reproducibility/lilac-dcm-stock-peripherals-47.1.F.1.105.json)从固定 `kernel.sin` 的 `boot.000` 内核段核对同一 DTB，复验报告逐字节一致。五个面板候选均为 720×1280、WLED 非零范围 1–4095，启用的 PMI8998 provider 绑定 strings 0、1。实机选中的面板变体仍待确认。WLAN 需先对照 `qcom,icnss` 路径；原厂 DTB 的 `qcom,wcn3990-wifi` 替代节点明确禁用。蓝牙有 `qca,wcn3990` 节点，音频有 Tasha 和 Tavil 两套候选，节点存在不能证明已初始化或实际选用。

[固定 20H392 背光发现核验](../reproducibility/backlight-discovery-20H392.json)确认 `CBDisplayContaineriOS` 以 `IOPropertyMatch: {backlight-control: true}` 枚举服务，并检查 `AppleARMBacklight` 类。这与 R25 查找失败的条件一致。初始化与到达回调都在 `IOService` 平面查 `kUSBContainerID`，选项值为 3；原代码检查结果是否为 `NSString`，对应分支调用 `initWithUUIDString:`。还不能判定该属性对内置 LCD 是否必需。

[同版本旧式亮度接口核验](../reproducibility/backlight-brightness-20H392-offline-20261124.json)核对了 10 个原函数的字节、调用目标和常量。初始化从背光服务的 `IODisplayParameters` 读取 `brightness.value/min/max`，另查 `new-backlight-architecture`。旧式 setter 把 32 位整数 `brightness` 和 `FadePeriod` 写到背光服务，条件性携带 `backlight-level`；其中 `FadePeriod` 来自原浮点参数乘 65,536 后截断。旧式 getter 只读取缓存浮点值。这个缓存与 `fPowerOff` 软件状态均不能代替 WLED 读数或光学验收；也不能据此把 Apple 亮度整数直接当成 Sony 的 12 位占空比。

现有 XNU 的注册表属性写入走目标服务的 `setProperties`，默认实现返回 `kIOReturnUnsupported`。独立的 `xnu/support/AppleARMBacklight.cpp` 实现了亮度属性字典和请求校验，并使用现有 IOKit 可写键列表。未知键、错误类型及越界数值返回 `kIOReturnBadArgument`；有效请求返回 `kIOReturnNotReady`，不会把未执行的调光写成成功。[属性对象记录](../reproducibility/lilac-backlight-service-properties-20261124.json)保存初版独立编译结果。

注册入口要求有效 provider 和明确亮度范围，成功时返回调用者持有的服务引用。服务和入口已作为单独对象[链接进 v731 的离线验证副本](../reproducibility/lilac-backlight-service-linkcheck-20261124.json)，没有加入开机注册调用，也没有绑定实机亮度范围。Mac 正式仓库具有原 v731 构建输入时，可用 `python3 -B tools/build/link_lilac_backlight_service.py --output build/my-backlight-linkcheck` 复算；该产物没有真机准入。

在 WSL 可直接以 ASan／UBSan 编译运行 `tools/build/test_lilac_backlight_properties.cpp`，核对数值边界和截断拒绝。Mac 验证复用已核验的 `wled-compile.argv.json`，只替换输入源、对象和依赖输出的路径，在新的 `build` 目录编译。后续须先取得 v731 的总线读数，再结合实际面板响应决定后端范围；不把 Apple 亮度整数当作 WLED 占空比。

离线复验使用 `tools/build/inspect_lilac_stock_dtb.py --firmware <原厂ZIP> --dtb <原DTB> --output <新报告>`（需要 `fdtget`）、`tools/build/inspect_backlight_contract.py --sparse <gl5> --symbols <固定A2S> --output <build下新报告>`、`tools/device/peripheral_log_evidence.py --directory <原运行目录> --output <新报告>`。前两者按固定 SHA 拒绝其他版本，三者都不打开串口或读写手机。

亮度接口复验使用 `tools/build/inspect_backlight_brightness.py --sparse <gl5> --symbols <固定A2S> --output <build下新报告>`；只用本地镜像和符号，报告不包含原函数字节。

## 原厂背光只读快照

原厂 `qcom,qpnp-wled` 采用 SID 3、控制窗口 `0xd800` 和灯串窗口 `0xd900`。对应参考是本地 `research/lilac-kernel-8.4` 的 `leds-qpnp-wled.c` 和设备树绑定，并非上表新版通用驱动的节点名。[离线证据](../reproducibility/lilac-wled-snapshot-offline-20261124.json)固定了两个参考文件的 Git blob。

`xnu/support/LilacWledSnapshot.h` 只接受读取回调，两遍核对故障、模块使能、灯串启用和两串 12 位占空比。读失败或两遍不一致时，不发布部分亮度。`CONFIGURED` 只表示寄存器配置一致，不能证明面板亮度或亮灭。

[占空比计划](../reproducibility/lilac-wled-duty-plan-20261124.json)复现开源驱动对已启用两条灯串的非零调光顺序：低字节、高字节依序更新，再按原厂 DTB 等 1,000 微秒完成同步。它要求调用方提供已验证的上限，并拒绝故障、两串不一致、模块关闭及零值请求。零值和唤醒涉及另一路模块电源与保护流程，不由占空比计划处理。计划只返回数据，未接入背光服务或 SPMI 写接口。

主机复验：在 WSL 正式仓库用 `cc -std=c11 -O1 -g -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -fno-sanitize-recover=all -no-pie tools/build/test_lilac_wled_duty_plan.c -o build/test-wled-duty` 编译到新路径，再运行该文件。

[占空比事务](../reproducibility/lilac-wled-duty-transaction-20261124.json)用同一份计划驱动模拟总线，先取稳定快照并检查同步寄存器空闲，执行后复核两串占空比、故障及同步寄存器。写入开始后若任一步失败，或读回无法确认结果，该事务状态拒绝后续请求，不自动重试或回滚。硬件接入前，调用方须负责串行化访问、验证同步读回语义和限定调光范围；寄存器一致不能证明物理亮度。主机复验沿用上面的编译选项，把输入换成 `tools/build/test_lilac_wled_duty_transaction.c`，输出用新的路径。

[WLED 观察源码候选](../reproducibility/lilac-wled-observer-overlay-20261124.json)以实际 v730 编译输入为基准。当前 `LIVE` 下的首个完整扫描帧写出 `END` 后，沿现有 SPMI observer 读取固定两个 PPID 的九个寄存器，最多 18 次读；任一读失败即停止，失去 `LIVE` 后不发布样本。此候选不创建新串口命令、不初始化总线、不改 owner 或 PMIC 寄存器。[v731 私有诊断实机](../reproducibility/lilac-wled-v731-private-actual-20261124.json)已验证这两个页可读；唯一一次 `SAMPLE` 位于完整 535 块扫描帧的 `END` 之后、本次 `ACK` 之前。状态 `CONFIGURED`、故障位 0、模块启用、灯串／调制器掩码均为 3，两串占空比为 1989。未执行背光写入，不能由此推导亮度范围、物理亮灭或唤醒结果。

`tools/device/lilac_wled_evidence.py` 要求精确镜像 SHA、完整扫描帧与回程、同靴 `END→SAMPLE→ACK` 顺序及样本 CRC。即使原试验为 `REVISE`，只要扫描帧和回程可复验，就能分别记录 WLED 状态并保留原判定。光学、亮度和睡眠恢复另行验收。

在 Ubuntu 22.04 的正式仓库根目录可复验：

```sh
out=build/my-wled-host
mkdir "$out" &&
  cc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    -fno-sanitize-recover=all -no-pie tools/build/test_lilac_wled_snapshot.c \
    -o "$out/test_lilac_wled_snapshot" &&
  "$out/test_lilac_wled_snapshot"
```

用已重建并核验的 v730 XNU 源码树生成隔离源码，随后在 WSL 验证生成的头文件：

```sh
out=build/my-wled-overlay
python3 -B tools/build/prepare_lilac_wled_observer.py \
  --v730-tree /path/to/exact-v730-xnu --output "$out" &&
  c++ -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    -fno-sanitize-recover=all -no-pie -I "$out/overlay/iokit/Kernel/arm" \
    tools/build/test_lilac_wled_screen_probe.cpp -o "$out/test_lilac_wled_screen_probe" &&
  "$out/test_lilac_wled_screen_probe"
```

生成器只输出源码和清单。在保留原 v730 编译日志、对象、链接列表和工具链的 Mac 正式仓库中，可使用下列命令独立链接：

```sh
python3 -B tools/build/link_lilac_wled_candidate.py \
  --repository . --overlay build/my-wled-overlay --output build/my-wled-link
```

脚本要求原对象重链接除 `LC_UUID` 外逐字节一致，再只替换 `SweetFramebuffer.cpo`，保留 1,116 条原链接输入 SHA。登记后的候选为 v731，使用自己的内核身份和准入证据；不能挂到 v730 下投递。

在 Windows 正式仓库离线重建并审计 v731 的首个采样包：

```powershell
python -B tools/device/package_trial.py --mode private --version 731 --kernel build/lilac-wled-isolated-link-r3-20261124/kernel.wled-observer.macho --screen-evidence --output build/my-wled-private
python -B tools/device/run_trial.py --bundle build/my-wled-private --directory "$env:TEMP\my-wled-private"
```

主机在首帧结束后最多等两秒，并受原帧截止时间约束，不补发截图。未收到样本时仍保存已验扫描帧，读数单列为不可用。回程后的 WLED 核验使用 `tools/device/lilac_wled_evidence.py --directory <原运行目录> --image-sha256 <该包SHA> --output <新报告>`。

v731 的两种图形验收包均使用 `xz1c-native-2x` 拟合布局和 150 秒单帧。固定 R21 RAM 只跑 GLES；固定 R25 RAM 在 backboardd 运行完整 Metal21／PNG6 背景测试，另须提供同核 GLES9 先验。使用各自的 RAM 先跑同镜像内容的 v731 CPU 准入，再依操作手册提供 v731 的 private／owner／ready／CPU 和保存的 RPM 时钟证据。参见 [GLES9 实测](../reproducibility/lilac-wled-v731-gles9-actual-20261124.json)和 [Metal21／PNG6 实测](../reproducibility/lilac-wled-v731-metal21-actual-20261124.json)。原壁纸启动诊断是独立的串口试验，不取扫描帧或 WLED 样本。

## 非零属性到占空比的候选后端

[本轮离线证明](../reproducibility/backlight-nonzero-duty-backend-20261128.json)补充旧属性service：`LilacBacklightDutyBackend.h`把已验证、明确advertised min/max的非零整数按整数ceiling归一到owner给定的12位上限，再复用原4/1000下限和六步占空比事务。固定原厂DTB没有`qcom,wled-brightness-map`或stepper，SYNC延迟1000us；21项somc区域电流表仍单列，不当作256项亮度LUT或光学线性曲线。caller仍须验证物理范围／回调／串行所有权。

IOService新增可选duty-backend factory，旧无backend入口仍NotReady。try-lock拒绝并发／重入，backend在readiness回调前设置busy；要求准确同epoch，事务后复核epoch和总线读回。只有VERIFIED／稳定UNCHANGED才更新IODisplayParameters.value并返回成功。属性dictionary提前分配，后写registry失败使backend不确定；任何已开始写入的失败保持锁定，不重发／回滚。backend及bus context由owner保留到service detach，构造不初始化或映射SPMI。

brightness0、非零FadePeriod及辅助backlight-level需要各自真实电源／渐变／等级路径，目前明确Unsupported且无IO。此候选只完成已启用模块的非零即时占空比通路，不证明灭屏、唤醒、光学亮度或全背光API。65535个属性值以独立long-double ceiling对照、极值、六个写失败、epoch变化与回调重入通过ASan／UBSan。新service以实际v731 recipe独立ARM64编译并追加链接，原1116链接输入保持；没有开机注册调用或实际bus回调，没有新设备内核投递。

```powershell
python -B tools/build/check_lilac_backlight_duty_backend.py --reference D:/Desktop/Git/Patch/Xperia/research/lilac-kernel-8.4 --output build/新的-duty-backend-host
```

Mac独立链接使用现行`link_lilac_backlight_service.py`，输出新build目录；原kernel和GPU／allocator／SCM／PON／RPC／VM不改。下一步仍是带真实owner/readiness的backlight注册／非零调光验收及光学，再单独处理模块电源与睡眠恢复。

2026-11-28：[逐IO epoch边界修正](../reproducibility/backlight-duty-per-io-epoch-20261128.json)复用原事务，但read／write／wait回调两侧均检查当前readiness与同epoch。原先只在事务首尾复核，IO中域变化后仍可能继续后面的写入；现在任何边界丢失都停止剩余IO，锁定不确定状态。38个read、6个write及1个wait中途epoch变化位置逐一验证，失败后不重发、不回滚、不更新属性cache；65535映射值／重入／原写故障回归保持。独立ARM64 service编译和v731链接重现通过，原输入与当前kernel不变，无新的boot／真实SPMI或光学证据。渐变、电源、辅助level及注册仍待各自实现／验收。
