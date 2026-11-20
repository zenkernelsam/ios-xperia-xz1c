# 串口显示帧截图

2026-11-20重新从881ce1c5主页与3541e079锁屏原串口日志独立核验全部源／寄存器／CRC／FNV／保存PNG像素，均通过，未发送手机命令。两张图实际捕获于11月19日；日常UI／文字／颜色数值已经可直接取串口无损720×1280 PNG，不必等待DroidCam。复杂主页原传输30.311秒、锁屏11.921秒，当前为单帧取证，不能当实时录屏或FPS；LCD实际发光、光学色偏、错行及背光仍单独验收。原raw、失败判定和已验基线保持。

2026-11-19现行CA矩阵GLESd6d8fcfa／v730已通过自身GLES9组／完整native16／PNG6组；cb9a39f8及206fb768两张150秒图恢复灰色锁屏时间日期，881ce1c5在120秒单串口上滑后150秒读到完整正常颜色主页。该主页帧2764800B→925068B／7228块，传输30.311秒／PNG301942B，power_off0；所有来源和像素校验通过，壁纸仍黑且原Aegir无pipeline，controller STOP保持。最新原请求r2／61745a1e16c8ab41在150.031秒再次保存灰字锁屏，2764800B→349908B／2734块，PNG52248B／SHA39025591；来源／保存像素／完整回程均通过，原STOP仍保留。详见 [CA-MATRIX](CA-MATRIX.md)、[METAL-REQUEST-OBSERVATION](METAL-REQUEST-OBSERVATION.md)、STATUS及对应证据，下面r6缺字为旧库比较。

v730 增加完整、无损的显示帧读取。现有 USB CDC `frame` 命令在 `sfscreen=1` 时只排队，显示 workloop 在完成本次呈现后复制扫描缓冲区，独立线程压缩并分块发送。v729 的 `frame` 仍只有 ASCII／缩略图，不能当作完整截图。

当前支持的来源是已经验证的 lilac VIG0：720×1280、RGB888（内存 BGR 顺序）、stride 2160、单管线、不裁剪、不使用 QSEED3。读取地址必须同时匹配实际 MDP 寄存器和现有 panel memory descriptor；原四字节 IOSurface、逻辑 750×1624 canvas 或旧 shadow 不作为该截图来源。其它扫描契约明确拒绝，不能用重采样猜测面板输出。

捕获期间 workloop 串行，复制后重新读取源像素 FNV64 和扫描寄存器。私有副本不会再被绘制。PRLE1 保存全部三字节颜色；每条元数据和数据记录有 CRC32，每块有独立 CRC32，整段压缩流有 CRC32，解压后再核对原像素 FNV64。协议绑定本靴 token、递增 frame id、vsync、捕获时间和 power request 状态；缺块、重排、重复、越界、来源变化或传输失败均拒绝，不补图。每靴最多四次请求。

Windows 仍由一个 controller／serial_worker 持有 COM≥10、VID1209/PID0001。截图包强制 `sweetfb_noconsole=1`，停止面板文字绘制，完整串口日志继续保留。截图工具只读取该 worker 的原始日志，不另开串口。电脑端将真实 BGR 像素转存 RGB PNG，同时保存 `.bgr`、寄存器、完整源码／镜像绑定和 SHA。无需对截图 rotate90；相机的旋转规则只适用于相机帧。

新 kernel 仍依次需要自身 private／owner／ready、新 RAM CPU 和精确 GLES／native 先验。首次 private 可用截图做通道验收；其它使用截图的阶段必须重新核验同 kernel 成功的 private 截图。旧 CAMERA_EVIDENCE_MISSING／STOP 不能因新增截图工具而变成通过。四回程 marker、PON 0x06、连续健康 30 秒及旧 worker 退出的要求保持。

在已按版本顺序安装 v725..v729 增量、通过源码审计的 Mac 构建树上，v730 的入口为：

```sh
python3 tools/build/check_screen_snapshot.py --output build/scanout-snapshot-v730/host-checks
python3 tools/build/install_screen_snapshot.py
python3 tools/build/build_diagnostic_kernel.py --version 730
```

主机检查需要 Pillow（仅用于独立解码核对 PNG），且不能以主机 fixture 代替手机截帧。`xnu/lilac-scanout-snapshot.patch` 含完整增量及两个新增 header，`surface-diag-v730.json` 绑定全部 83 个实际源输入。`prepare_screen_snapshot.py` 仅从保留的精确 v729 `SweetFramebuffer.cpp` 重建增量，不修改旧版本记录。冻结的 GPU／allocator／SCM／PON／RPC／VM 源码均独立保留并逐源复算。

从固定上游完整重建源输入可运行 `python3 tools/build/verify_diagnostic_reconstruction.py --version 730`。基线的三个 LF/CRLF 差异由 `tools/baseline_source_inputs.py` 按固定 SHA 和行尾索引物化；源码文本、现行内核输入与旧实机记录保持。它在 TEMP 中创建新的 checkout，不覆盖现行构建树。

Windows 的单轮打包和投递示例：

```powershell
python tools/device/package_trial.py --mode private --version 730 --kernel build/surface-diag-v730/kernel.v730.macho --output build/screen-v730/private --screen-evidence
python tools/device/run_trial.py --bundle build/screen-v730/private --directory "$env:TEMP\lilac-screen-v730-private" --live
```

输出在本轮 `cycle-01/screens/scanout-00000001.png`，对应 `.bgr`、`.json` 和 `capture.json`。后处理会从原串口字节重新解码，并逐字节比对保存的 BGR 与 PNG；离线恢复工具为 `tools/device/scanout_snapshot.py --raw ... --artifact ... --output ...`，输出必须使用新目录。手机端失败不重发旧 boot bundle。

GPU 的新包在观察开始 45 秒、已确认本靴 LIVE 后截一次，随后继续全部 GPU／native 检查。最终取证复用这张已校验原帧，不再发送第二次 `frame`；失败也不补发。`power_off` 是系统的显示电源请求，截图数据不能用来推定背光。旧 r2 的截图取于该请求已为 1 时，照实保留，不能拿它判断正常亮屏期的时钟／壁纸或颜色。

2026-11-19电脑端新增`--screen-after 150`。GPU默认仍45秒，新包允许独立选择150秒，plan／artifact同值且位于原观察期限内；一个worker／每靴单frame／完整校验与回程保持，kernel／GPU库及原extension期限不改。完整原GPU draw／PNG已在87397739实际返回，但其45秒帧仍logo，较晚捕获用于补足UI内容，不能从离屏PNG推扫描画面。示例使用已独立准入的v730／原RAM包，输出必须为新目录：

```powershell
python -B tools/device/package_trial.py --mode gpu --version 730 --kernel build/surface-diag-v730/kernel.v730.macho --ram-package build/ir3-cache-r1/original-ram --output build/ir3-cache-r1/my-late-ui --screen-evidence --screen-after 150
```

随后`run_trial.py`继续绑定同kernel private／owner／ready、该RAM自身CPU、精确GLES、完整native16／同共享源PNG、真实RPM及Mac源输入；只在全部门槛通过后单发。若原失败要求提前回程，保留失败并在既有最终取证位置捕获一次，不称其为150秒样本。输出路径仍为本轮`cycle-01/screens/scanout-00000001.png`；无须摄像头服务或rotate90。

独立核验入口`scanout_trial_evidence.py`同时读取本靴原results／artifact／plan／commands／events、完整raw和保存帧，检查单frame／单token回程、完整payload、150秒实际观察记录与四marker／健康；它保留PASS／REVISE／STOP原判定，单列缺失的原目标日志，不改变GPU准入结果：

```powershell
python -B tools/device/scanout_trial_evidence.py --directory "$env:TEMP\lilac-v730-ir3-cache-r1-original-late-screen-gpu-r6-20261119" --output build/ir3-cache-r1/my-late-screen-evidence.json
```

该实际r6 token4dd119fa在150.047秒捕获了完整锁屏缓冲区：2764800B→325269B、2542块、10.862秒，power_off=0。33600B PNG的SHA256为`f91e2ff89351927915c45c608cb1b55e9c2702fe79113a4f9601ea2d80e0079f`。来源／寄存器／整帧校验及保存像素均通过；绿色电池、相机、白色homebar清晰，背景仍黑，时间／日期／壁纸缺失。原GPU登记窗口为STOP／OBSERVATION_INCOMPLETE（未见pipeline请求），保留原STOP，38.172秒完整回程／健康／worker退出。该帧只证明真实扫描缓冲区内容，power_off=0也不是LCD背光实测。

新`--swipe-once`只在本LIVE／F12就绪后120秒一次既有touch线程上滑、150秒单frame，与采样独立；raw必须有一次request与一次注入，command／events／回程都核验，合成手势不当物理触控。`--idle-sample`本原library返回后180秒单sample，原周期样本单列；独立scanout工具允许精确绑定的这些额外命令，缺／错／重发仍拒绝。两者不改变原kernel／GLES／期限或覆盖旧试验结果。

这类截图证明当前显示缓冲区内的 UI、文字、图标及颜色数值，可以替代 UI 内容观察所需的相机。它不会直接测到 LCD 光学色偏、DSI 是否真正送出该帧、背光或玻璃上的行错位；这些项目仍需相机或用户观察，`optical_panel_verified` 始终单列。完整 UI/FPS、睡眠和其它外设的原目标保持。

设计对照为本机 S2 的 `capture-framebuffer.py`（按实际物理扫描地址／stride 读取并对照序列）和 Y700 的 `debug_screen.py`（分块／帧 CRC 与同一串口会话）；Sony 没有引入其它项目的通用内存命令或 RGB565 色彩降精度。

2026-11-18：整屏 codec ASan/UBSan、ARM64 编译、独立 PNG 像素以及 6 种 USB 拆分／26 项故障拒绝主机检查通过。v730/f258dee9 实际重编／83 源审计通过，private aca5c330 的完整手机扫描帧已传回：2764800B→69523B、544 块、2.906 秒，全部校验通过，38.266 秒四 marker 回程及健康／worker 退出。该 CPU 靴的日期／时间、camera 和 homebar 清晰可读，红色提示／文字与缺壁纸照实保留；其它同 kernel 阶段和原 GPU 界面须独立验收。旧 v729 GPU 先验和相机 STOP 原样保留，见 STATUS 及 reproducibility/v730-screen-private-r1-actual.json。

2026-11-19：owner／ready／RAM CPU 及两轮完整 GLES 八项／readonly 像素也独立通过，六轮均完整读屏及正常回程。最新 5f97bb21 实测在观察 45 秒截一次并复用，38.750 秒回程；该图仍记录 power_off=1，不能推成亮屏 UI 或背光结论。原始命令仅 `frame`、`fastboot <本靴token>` 各一次。全部源从固定 Apple base 在新 TEMP checkout 重建并通过 83 源 SHA；一次性源码检验目录／草稿已进入原生回收站，正式源码和当前证据保持。
