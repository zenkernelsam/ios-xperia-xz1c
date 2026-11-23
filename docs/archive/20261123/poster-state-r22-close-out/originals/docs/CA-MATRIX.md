# CA矩阵补全与原界面 — 2026-11-19

本页保留该专题的实现与分阶段原证据；旧“当前／下一步”只指原实验日期。准确候选、生产基线和准入统一看[STATUS](STATUS.md)，现行构建与单发见[BUILD](BUILD.md)、[OPERATIONS](OPERATIONS.md)。

当前GLES `d6d8fcfa…` 在Sony FD540实机补回锁屏时间、日期；一次串口上滑后的完整720×1280主页有正常应用图标、文字、widgets和Dock。壁纸仍黑，原Aegir在这几靴没有pipeline请求，GPU登记窗口STOP如实保留。真实图片仅在TEMP；原raw、输入SHA、完整回程和限制见 `v730-ca-matrix-r1-run-index.json` 和STATUS。

## 具体变更

现有 `sgl_install_texfns` 使用另一版QuartzCore固定地址，20H392的texture-table、create_shader与uniform-switch相差0x4000，原保护检查拒绝安装。只读本地20H392 HFS得到准确QuartzCore header `0x188284000`、UUID `9e40b2eb260c3af1ae45a1a551a8c1b9`、table `0x1d5b48a10`、switch `0x18855e1e4`；A2S原symbol与实际create_shader两组ADR-P／ADD／table-load／branch字节匹配。

新版只在image UUID／Mach-O segment范围／两组实际24B代码锚点、slot0x13原表达式`colorMatrix4x4Bias(s0)`和0x14／0x2d原空表项／0xde分支全部核验后，补齐两个矩阵槽和其matrix uniform case。回写后核对实际值，失败尝试恢复原空项；未知布局返回拒绝。原vcm／cm4方程逐字节保持，只把源码提取至CAMatrixShaders.h。其它模糊槽0x26／0x27未在这轮安装，不能借“开源适配”扩大成模糊已完成。

`ca_matrix_probe.c`沿生产GLI source rewrite→compile→link→draw／framebuffer-fetch，独立作者source/target像素和CPU矩阵公式核验。六阶段包括identity、通道交换、bias／clamp与三个destination-mix，检查1536B及192B保护区；量化界1LSB，实际手机六组最大差异0。新GLES suite为原8＋matrix1共9组，stage/native准入必须读新组，旧8组不能通过新库。

主机WSL／Mesa真实shader运行和UBSan通过，UUID／code-anchor／expression／nonempty-slot错误拒绝；iOS目标编译／签名通过。主机结果不代替手机安装与像素。原kernel、Mesa编译器／归档、IR3 serializer、allocator／SCM／PON／RPC／VM保持，实际源锁见ca-matrix-r1-host-proof、layout-proof和build-inputs。

## 实机结果

GLES CPU908bb4fa／38.359秒、GLES9组4946e854／39.390秒、reference CPU59cca10e／38.610秒与完整native16／共享PNG6组46747c46／38.375秒均独立回程通过。资源／compute／render／constants／XR／IOSurface／mip／half-narrow／MRT-sampler／program-static-half-Clear-IR3与原guard、fence、撤销全部核验。Metal二进制5e94ab57及source-map7b5a083a保持，但精确GLES改变，完整proof重新生成。

原RAM CPUe28b2aea／37.906秒、primer CPU4ee0c210／38.437秒也通过。primer尚未GPU投递：新scope未出现实际缺binary请求，不能空跑冒充预热。旧IR3 scope的真实request／failure／成功draw和PNG记录完整保留。

原cb9a39f8与206fb768两张150秒扫描帧都有正常灰色时间／日期、绿色电池、相机／white homebar，黑壁纸。前后时钟区域比较由原图字节／SHA绑定，旧帧0个亮像素，新帧17018个；只证明这些样本内容，不能判绝对时间／所有锁屏配置完成。206fb768的唯一串口sample在本metallib返回后180.047秒，固定symbol-map与真实slide复核9线程：主线程UIKit事件循环、UIEventFetcher，未观察到正在compile／draw的GPU栈；原90秒初始化样本单列，不能推整靴无GPU耗时。

原881ce1c5本LIVE／F12后120.078秒一次swipe，touch线程确认一次注入；150.031秒真实完整主页帧2764800B→925068B／7228块／30.311秒，PNG301942B／SHA `19fb266d1d4637945eb4ecbae123f40d0fafa13f24f1eb95b8313f5468a1ea30`。Apps、文字、天气／日历小组件、Search和四项Dock颜色正常；壁纸黑、Aegir pipeline仍缺，原STOP／OBSERVATION_INCOMPLETE保留，39.688秒完整四marker／健康30秒／worker退出。合成gesture不代替手指触控；scanout不代替光学／背光／sleep／FPS。

## 构建与下一步

Mac按BUILD重编当前前端、包只读RAM。完整后台参考在已安装fixture的主机可用：

```sh
python3 -B tools/build/build_metal_library.py --ram-package build/ca-matrix-r1/gles-ram --air-package build/static-initializer-r1/frontend --output build/my-ca/library
python3 -B tools/build/build_exception_observer.py --output build/my-ca/reference-observer --metal-resources build/my-ca/library --gles-ram-package build/ca-matrix-r1/gles-ram --target-process backboardd --air-fixture build/sampler-reuse-r1/fixtures/basic --render-fixture build/sampler-reuse-r1/fixtures/render --constants-fixture build/sampler-reuse-r1/fixtures/constants --xr-probe --surface-probe --mip-probe --narrow-fixture build/sampler-reuse-r1/fixtures/narrow --half-fixture build/sampler-reuse-r1/fixtures/half --mrt-fixture build/sampler-reuse-r1/fixtures/mrt --sampler-fixture build/cif10-reference-sampler-r1 --static-half-fixture build/static-initializer-r1/fixture --clear-cache-probe --ir3-cache-probe --cif10-png-reference
python3 -B tools/build/package_glram.py --presigned --frontend build/ca-matrix-r1/gles-ram --observer build/my-ca/reference-observer --metal-resources build/my-ca/library --output build/my-ca/reference-ram
```

fixtures都是仓库作者AIR，经make_air_fixture从固定LLVM生成；旧路径仅现有构建输入，源／bitcode／header／library SHA必须重新核验，不能缺文件时随便换参考。新frontend／Metal构建的任何新SHA须自身完整准入，顺序见BUILD／OPERATIONS。Windows每个新mode用package_trial --version730和对应run_trial前序，原Aegir short使用新native／PNG proof并自身RAM CPU。

下一步继续原Poster／scene请求与完整壁纸；当前没有触发实际pipeline，不能把GPU硬件更改当已证根因。动态UI／FPS、sleep、音频无线和广义texture／depth／stencil／MSAA仍active。只有对应真机结果完成原目标后才关闭；保留当前可返回fastboot基线和全部旧失败。
