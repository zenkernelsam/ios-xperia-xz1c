# 串口显示帧截图

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

这类截图证明当前显示缓冲区内的 UI、文字、图标及颜色数值，可以替代 UI 内容观察所需的相机。它不会直接测到 LCD 光学色偏、DSI 是否真正送出该帧、背光或玻璃上的行错位；这些项目仍需相机或用户观察，`optical_panel_verified` 始终单列。完整 UI/FPS、睡眠和其它外设的原目标保持。

设计对照为本机 S2 的 `capture-framebuffer.py`（按实际物理扫描地址／stride 读取并对照序列）和 Y700 的 `debug_screen.py`（分块／帧 CRC 与同一串口会话）；Sony 没有引入其它项目的通用内存命令或 RGB565 色彩降精度。

2026-11-18：整屏 codec ASan/UBSan、ARM64 编译、独立 PNG 像素以及 6 种 USB 拆分／26 项故障拒绝主机检查通过。v730/f258dee9 实际重编／83 源审计通过，private aca5c330 的完整手机扫描帧已传回：2764800B→69523B、544 块、2.906 秒，全部校验通过，38.266 秒四 marker 回程及健康／worker 退出。该 CPU 靴的日期／时间、camera 和 homebar 清晰可读，红色提示／文字与缺壁纸照实保留；其它同 kernel 阶段和原 GPU 界面须独立验收。旧 v729 GPU 先验和相机 STOP 原样保留，见 STATUS 及 reproducibility/v730-screen-private-r1-actual.json。

2026-11-19：owner／ready／RAM CPU 及两轮完整 GLES 八项／readonly 像素也独立通过，六轮均完整读屏及正常回程。最新 5f97bb21 实测在观察 45 秒截一次并复用，38.750 秒回程；该图仍记录 power_off=1，不能推成亮屏 UI 或背光结论。原始命令仅 `frame`、`fastboot <本靴token>` 各一次。全部源从固定 Apple base 在新 TEMP checkout 重建并通过 83 源 SHA；一次性源码检验目录／草稿已进入原生回收站，正式源码和当前证据保持。
