# 真机、串口与相机

用户授权无人值守持续推进。正式单轮入口为tools/device/package_trial.py与run_trial.py，源码／操作只在ios-xperia-xz1c；当前基线与实际controller/token以 [STATUS](STATUS.md) 为准。历史工具不作为当前可重发入口。

## 设备与回程

仅Sony BH905ND69G，所有fastboot操作必须-s BH905ND69G。COM>=10且VID1209/PID0001，禁止COM3／其它设备；一个controller／serial_worker持COM和相机，不能另开串口。早期没有LIVE的E1F4仍可能需物理恢复，不能凭过期观察重发正在运行的boot。

单download／boot，失败不重发，max_cycles=1、on_pass/on_failure=stop。PON仅0x06；每轮ACK／DRAIN／RESET／FINAL_DRAIN、连续健康30秒、旧worker退出必须独立通过，才可下一轮。无匹配token LIVE不发送回程控制；失败或缺证据保持原STOP／REVISE，自动回程成功不提升GPU阶段。

gl5已在设备，不flash／erase userdata、SD、TA、XFL、救砖固件／校准／备份。新kernel自身private／owner／ready，新RAM自身CPU，新GLES自身全部组、新Metal精确能力组／source-map及共享PNG先验。生产CA基线native16，Depth32候选17，Depth32＋Stencil8候选18，现行Memoryless候选19；新Memoryless库不接受旧18组proof。run_trial投递前复算整个payload和Mac实际源码／Mesa归档；短登记嵌入proof必须与原raw复验完全一致。当前CA矩阵库还有GLES第9组，旧库或漏组proof拒绝。

## UI取证与单次输入

优先串口真实720×1280／BGR888／stride2160扫描帧，前序同kernel完整private截图必须成功。新GPU包 `--screen-evidence` 默认观察45秒，`--screen-after 150`在150秒单frame，以免只见logo；只能45／150且plan／artifact同值、位于原观察期限内。最终取证复用这张帧，缺块／失败不补发。若原失败要求提前回程，保留真实捕获时机，不冒充150秒样本。详细协议与独立核验见 [SCANOUT-SCREENSHOT](SCANOUT-SCREENSHOT.md)。

`--swipe-once --screen-evidence --screen-after 150`仅用于独立原UI观察：本LIVE／F12实际就绪后120秒发送一次swipe，内核触控线程原固定上滑；150秒捕获。它与采样独立，完整准入／原观察期限／回程保持。必须核对本靴command／tx／一次注入；合成手势不等于物理触控。最新881ce1c5主页内容已保存，controller仍STOP，因为Aegir无pipeline请求。

`--idle-sample`在原Aegir metallib实际返回／本LIVE后180秒经同一worker发送一次sample；原内核90秒周期样本单列。`--thread-sample`原首次Clear触发与它互斥。保持已验内核采样器，十二个数字1的无=1拼写必须绑定已测试parser；不重发sample，不能从单栈推全部GPU耗时或修改冻结组件。

截图证明缓冲区内容／数值，不能证明LCD真正发帧、光学色偏、物理错行、背光／睡眠。power_off是系统请求，0也不是背光实测；相机光学验收仍需单列。

日常GPU／UI迭代直接用已验v730串口图，不必等待DroidCam。复杂主页原传输约30.3秒，锁屏原传输约11.9秒，适用单帧而非实时录屏／FPS。每轮仍通过原同一worker排队`frame`，手机fastboot期间没有iOS扫描帧可读，不为看图重发失败boot。已有raw可以离线复解码，图片注明原捕获日期／token，不能冒充新的实时画面。

## 相机与工具

相机只用override→AF(/cam/1/af)→等2秒→MJPEG(/video?1280x720)→每5帧抽1→rotate90；已旋转照片不再转。grab_cam.py禁用，frame.jpg只在视频失败后单次兜底。UI内容采用已验串口图时无需DroidCam服务，旧CAMERA_EVIDENCE_MISSING／STOP原样保留。

fastboot／bash来自LILAC_FASTBOOT／LILAC_BASH或PATH；xperable由使用者放assets/device-tools/xperable.exe，guard核对SHA且只操作Sony。boot使用boot_once无重试，不能用xperable内置boot重试。读取现有raw的后处理工具不打开COM；原结果／raw不覆盖，退出期记录单列。

USB投递前拒绝可独立使用`usb_open_observation.py --output "$env:TEMP\新的观察目录"`读取准确Sony身份／现有配置／bulk描述符；`--claim-once`只单次claim／release原接口。该工具不发bulk读写／download／boot／reset，也不set_configuration或detach驱动，一个device lock且输出目录必须新建。它记录失败所在阶段，成功不授权重发旧失败试验。r4 USB open拒绝与r5 guard退出均未boot，原记录见STATUS；实际单发发送器SHAe862e430保持。

Mac正式仓库~/Desktop/ios-xperia-xz1c，~/xnu-autobuild.pause保持，旧常驻构建器不启动。每轮核对Mac实际Git和源码／归档SHA，单独复制源码不当作同步完成。当前镜像／原输入／失败证据受保护；回收站规则和历史原文见 [LAYOUT](LAYOUT.md) 与 [archive](archive/20261119/OPERATIONS-before-ca-matrix-actual.md)。不永久删除，不清空Windows／Mac回收站。
