# Background native Metal acceptance — 2026-11-14

backboard-r1已实机完成整组原生资源/queue/AIR compute/render/constants/XR/IOSurface/mipmap，全部1完整/0缺项。新诊断在backboardd独立线程等待本进程原GLES八项全部通过后才开始，constructor不等待诊断；没有在此轮给Aegir注入或登记device。原Aegir check-in timeout保持不变，原UI仍未验收。

mipmap四格式70/80/81/553、六级、8次生成（包含上传变化后重生成）、24个LOD/trilinear采样通过。独立验证917504B完整原buffer/级间/padding/输出保护区、40个生成fence和24个采样fence，所有规定结果匹配；UNorm/XR精确，sRGB仍明确1LSB量化界。mipmap探针耗时2360ms。原资源/Surface/XR、960000只读GLES像素和完整回程分别验证，没有以主机或部分样本补足。

固定内核v729 SHA774b5efd5f715dc40d81e3c207c85b9b690c83fe44f4fd9061a2189ef7db71ec、81源审计保持。GLES499bc6e9ddcf8143a0cc5578a3b8dd9c6ce3396f660917b7ac5655aaedf25cf3（14711888B），新包完整CPU/GPU门槛均通过。本靴token42634887bc169c5f、镜像c00fb1729dff0fa583e77ffe3e3eeb34071737425ff53c383b07e69584ea4c65；506647B原串口SHA5759b7f2f8b37bc35f7571fa0abcc603c2fa4b2947bf5b495437ffb4bbe4f98c。431allocated/released/unmapped、active/orphan/root0，37.797秒完整PON06回程（健康30秒及旧worker退出）。Sony健康fastboot，无controller/COM/camera/in-flight。

sweetgl_selftest_state只表示本进程作者GLES自测的0未请求/1进行中/2全通过/3失败状态。原八项函数顺序保留，成功记录先写出再release发布；背景线程acquire读取该信号。新增MetalProbeStartup.h在真实host worker验证等待、成功、失败、未知状态和deadline，不能在失败或超时后运行GPU。后台调度记录先发布，再放行线程，保证独立串口排序验收可靠。

新observer固定允许AegirPoster或backboardd；后台目标必须完整资源、队列和所有AIR/XR/Surface/MIPS fixture，禁止系统登记。命令路由backboardd:DYLD_INSERT_LIBRARIES=/dev/sglram/m:/dev/sglram/e，保留SWEETGL_TEST=1和原SpringBoard几何库；不向Aegir加e。原异常preprocessor继续原链，但后台验收无需制造异常或renderer记录，使用真实启动/GLES/全suite标记和各独立字节/fence verifier。

构建使用build_exception_observer.py --target-process backboardd --gles-ram-package指定精确已测GLES，--metal-resources及固定compute/render/constants fixtures，--xr-probe --surface-probe --mip-probe。后台新state API必须显式链接该GLES；首轮漏直接符号链接的编译失败保留observer/ios-compile.log，修订observer-r2通过。package_glram和package_trial依目标构造路由，新包继续先CPU后GPU及精确GLES先验。backboard_probe_evidence.py拒绝缺原GLES成功信号、调度顺序错误、漏任一探针、原Aegir注入/登记或任一失败；test_backboard_probe.py故障回放通过。

正式实际证据reproducibility/v729-backboard-r1-*-actual.json；原日志和最佳相机帧本地保留。上一轮r1/r2的Aegir启动超时不改写，本轮补上完整GPU证据。下一将原Aegir设备登记与长诊断分离，严格绑定当前完整native suite/GLES/Metal字节先验，再观察真实壁纸的请求；奇数尺寸/更多level数量、Surface mip硬件验收及完整UI/FPS/睡眠/声音/无线继续active。
