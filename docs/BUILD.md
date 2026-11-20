# 固定源码与构建

当前正式基线为v730/f258dee9、CA矩阵GLESd6d8fcfa、Metal5e94ab57／source-map7b5a083a。它已通过自己的RAM CPU、GLES原8组＋矩阵第9组、完整native16／六组共享PNG及原RAMCPU；锁屏文字／主页扫描帧有实机改善，黑壁纸／稳定UI／FPS／其它外设仍继续。实际输入／结果见 [STATUS](STATUS.md)、[CA-MATRIX](CA-MATRIX.md) 和reproducibility，不用下方旧14／15组专题证明准入当前库。

最新源码为[原生presentation-r1](METAL-PRESENTATION.md)，GLES21de6821保持、新Metal8feea459及reference33bf5ee4构建／原生host／CPU offline audit通过，完整native21／PNG6原drawable像素及原UI仍待手机。新库不接受旧20组proof；完整重建和原20H392 ABI静态检查见专题。

前一离线源码为[2×／4× MSAA附件r1](METAL-MULTISAMPLE.md)：独立原页／resolve ABI和作者AIR／18阶段probe均已构建，完整native20／PNG6手机准入未发。新前端必须重编当前源，GLES21de6821／Metalb25bfe6c只作offline候选；旧native19不放行。完整重建命令与本轮输入SHA集中在该专题。

2026-11-20此前源码增加尚未手机准入的Depth32＋Stencil8附件r2候选，完整构建／源锁／host证明与新增native18门槛见 [METAL-DEPTH-STENCIL](METAL-DEPTH-STENCIL.md)。新包自身CPU、GLES9、referenceCPU、native18原depth／stencil页面及PNG6均需独立验收，旧库native16／17不能准入；不将离线candidate或host像素称为当前实机基线。WSL当前GLES host证明只用UBSan，ASan启动失败另存；host runner限45秒／日志1MiB。

随后原模式r6 observer／33方法只改观察源码，当前准确原调用／byte锚点工具和构建见 [METAL-REQUEST-OBSERVATION](METAL-REQUEST-OBSERVATION.md)。旧CA基线配新observer即使所有host测试通过，仍因当前六份GLES／Metal源码不一致被offline run_trial拒绝；不跳过源码audit。新附件GLES6211bac2／Metalf40f796b保持，按当前source重编完整reference observer仍7e164ad1，重新打包blob4274554B／90dc4934；CPU c10521e2已offline audit，完整native18／PNG6实机验收后才能用新proof构建模式observer原GPU包。

最新Memoryless单采样候选GLES6211bac2保持，Metalf4aa2b35／source-map31478965／referenceb083c28f；正式源码、host、构建与native19门槛见[METAL-MEMORYLESS](METAL-MEMORYLESS.md)。它沿完整native18全部fixtures再加`--memoryless-probe`，reference包必须自己的RAMCPU后native19／PNG6原实机验收；CPU a6ed4392仅offline通过，旧native18及host不能放行原GPU。

当前CA矩阵前端沿用已审计的IR3 Mesa归档，仅重编用户态，全部新目录必须不存在：

```sh
python3 -B tools/build/build_frontend_trial.py --mesa-build build/mesa-ir3-cache-r1 --output build/my-ca-matrix/frontend
python3 -B tools/build/package_glram.py --frontend build/my-ca-matrix/frontend --output build/my-ca-matrix/gles-ram
```

从零重建当前Mesa／IR3归档使用 [IR3-VARIANT-CACHE](IR3-VARIANT-CACHE.md) 的 `build_gles.py --mesa-build ... --frontend-output ... --ir3-cache`，固定Mesa base／完整patch／独立variant增量与实际source/header锁均核验。SDK／时间可改变产物UUID／SHA，新二进制必须重新实机准入。既有kernel／allocator／SCM／PON／RPC／VM／GPU底层不因CA适配重编。

完整native后台observer当前必须给全basic／render／constants／narrow／half／MRT／sampler／static-half fixture和`--clear-cache-probe --ir3-cache-probe`，共享PNG再`--cif10-png-reference`；正式命令见 [CA-MATRIX](CA-MATRIX.md)。自身CPU→精确GLES→完整native16／PNG→native_prerequisite／png_encoder_prerequisite生成新proof→原Aegir short observer自身CPU→原GPU，严格顺序保持。

v730完整扫描帧reader及主机／83源重建见 [SCANOUT-SCREENSHOT](SCANOUT-SCREENSHOT.md)。GPU包可`--screen-evidence --screen-after 150`，单帧150秒；`--swipe-once`为本LIVE／F12后120秒一次原上滑、150秒截帧；`--idle-sample`单独观察原metallib返回后180秒线程栈。它们只改变电脑端有界观察，不改kernel／原check-in／回程；当前包只在独立新bundle发一次。

原Metal／画布请求r2构建与四靴实际结果见 [METAL-REQUEST-OBSERVATION](METAL-REQUEST-OBSERVATION.md)：生产kernel／GLES／Metal保持，`--request-observe`增加13类原方法转发及分块ABI元数据，自身RAMCPU后再GPU。r2原20请求完整，但未出现display／pipeline，STOP保持；r1长ABI记录拒绝照实保留。当前UI截图通道已实机通过，不依赖DroidCam，LCD光学仍独立验收。

旧BUILD内容的各版本精确复现及失败记录在 [archive/20261119](archive/20261119/BUILD-before-ca-matrix-actual.md) 与各专题。下方公共构建手册继续保留，其历史库／组数只适用于该版本，不能提升当前库为已验。

reproducibility/source-lock.json锁定Apple XNU xnu-8796.141.3/f625db25f47631424dc62ac1c3414b6e530f7e8a、darwin-xnu-build基础663906528d1cd5ea8fde7de6f16de73db9511549、Mesa769e51468b49b2a42f0a0eaf71cf9eed5ff4e5de/24.3.4及全部修补。source-reconstruction.json记录逐源重建核验。

GLES在Mac构建，需Xcode/iphoneos SDK。脚本创建固定Python依赖环境，下载固定Mesa到deps，输出build/mesa-ios，再编译gpu/sweetgl：

    python3 tools/build/build_gles.py

内核需KDK13.5/22G74、Xcode及jq/gum/xcodes/cmake/ninja。使用已安装KDK：

    export KDKROOT="$HOME/KDKs/KDK_13.5_22G74.kdk"
    bash tools/build/build_kernel.sh

固定builder和依赖进入build/dxb；XNU先应用完整lilac-current.patch，不混用旧编号系列。当前实机未裁剪产物来自dxb/build/xnu.sym/kernel.development.sweet，不冒充xnu.obj的另一份strip产物。

Linux/WSL shim需aarch64-linux-gnu工具链，使用本仓库shim源码和本机Sony stock DTB：

    python3 tools/build/build_shim.py --dtb dtb.img

输出新的 build/shim-日期目录；仅编译，不发送手机。Linux Image头的 _end 是链接时尺寸字段，运行时指针不得有绝对重定位。该脚本不依赖旧mkbootimg临时目录。

当前 SpringBoard 几何兼容库也进入正式源码，编译入口：

    bash tools/build/build_geocompat.sh

验证入口为 python3 tools/validate_current.py。GLES新输出在 build/sweetgl；内核新输出在 build/dxb/build/xnu.sym。已记录的源码与编译验证见 reproducibility/build-results.json、source-reconstruction.json、shim-build-results.json；新二进制仍需经过实机准入，未替换已通过的v724基线。

合法提供iPhoneX_16.7.16_20H392_Restore.ipsw、Sony SO-02K原厂47.1.F.1.105固件及本机备份。root/gl5与一轮投递的正式可复现入口仍需补齐验证，旧绝对路径脚本仅保留历史。SDK/构建时间可能改变UUID与产物SHA，源码输入和实机验收分别记录。

RAM库打包入口已完成主机校验：

    python3 tools/build/package_glram.py

它直接使用 build/sweetgl 与 build/geocompat 的正式输出，生成新的 build/glram-日期目录，验证只读HFS、m/g硬链接、库SHA和压缩blob。render ADT可由 tools/build/make_render_adt.py 从本机 board ADT调整，仅四个显示属性，输出750×1624、375×812点、scale2。

用户镜像基线配方见 ROOT-GL5.md。原root脚本和新的源码构建已进入正式仓库；全新gl5首次启动与新编译二进制尚未实机验收，边界见STATUS.md。

v725/v726诊断增量及单次投递入口见 SURFACE-DIAGNOSTICS.md；当前v727/v728只读背板与64位VM修订见 SURFACE-READONLY.md。按固定基线应用各增量并构建，源码与实机结果分别校验。当前成功输入kernel33580458/r4库b035a887，不替换保存的旧基线。

当前前端可在既有Mesa归档已校验的Mac上单独重编并打包：

    python3 tools/build/build_frontend_trial.py --output build/my-frontend
    python3 tools/build/package_glram.py --frontend build/my-frontend --output build/my-ramfs

输出目录必须不存在，旧输出保留。--ram-package将新RAM包提供给tools/device/package_trial.py --version 728；同一新库先RAM CPU完整回程，再GPU。完整payload复算和Mac实际源/Mesa归档校验保持。全新Mesa与root构建仍按前述入口分别验收。

当前v729日志快照增量和r7/Metal队列的构建/准入见METAL-QUEUES.md；最新版package_trial支持--version729。在同版本全GLES库实机通过后，build_metal_library.py --ram-package指定该包构建原生库，build_exception_observer.py --metal-resources指定新原生库目录。package_glram.py --presigned --observer --metal-resources保持原GLES字节并添加两个库，先新包RAM CPU再GPU，run_trial的--gles-evidence还要求该精确GLES库的完整先验。命令/源/包/fence与原日志分别验证，禁止复用不同kernel或library的前序。

原生AIR texture/render的最新实际入口与限制见METAL-RENDER.md。make_air_fixture.py --fixture render创建独立作者AIR，build_exception_observer.py --render-fixture添加标准render encoder探针；compute fixture和原引用保持。check_metal_compute.py同时覆盖真实计算/渲染对象，air_graphics_cli.cpp与test_air_gles.c把生成ESSL交给主机Mesa实际编译绘制。主机测试不代替手机像素/fence/共享页面/完整回程验收。

Native function constants的实际门槛见METAL-CONSTANTS.md：make_air_fixture.py --fixture constants、build_exception_observer.py --constants-fixture和check_metal_compute.py的原生常量测试。新Metal包继续绑定完全相同的已测GLES、先RAM CPU后GPU；host ABI/ASan/UBSan与目标GPU实际字节分别核对。

原native系统登记探针使用build_exception_observer.py --register-device，仅Aegir且同进程先验GPU都通过。构建/独立STOP证据和真实格式卡点见METAL-DISCOVERY.md；回程四标记/健康/旧worker退出仍为强制条件，不能用原设备登记通过代替shader/完整面板验收。

XR格式的固定参考、LGPL源与原GPU字节见METAL-XR.md。单plane原IOSurface的构建/验收见METAL-IOSURFACE.md；build_exception_observer.py --surface-probe需--render-fixture，可同时添加--xr-probe。新GLES库先CPU/GPU，添加新Metal/observer包后再CPU/GPU，精确--gles-evidence仍强制。当前只支持已界定的单mip线性2D，并未实现generateMipmapsForTexture:。

mipmap最新实现见METAL-MIPMAPS.md，取代上段单mip限制：源码已支持界定的多级线性2D、Private、逐级region和GPU生成/LOD，硬件整组验收仍未完成。build_exception_observer.py --mip-probe使用固定render fixture，check_mip_gles.py从生产源码提取XR shader并在WSL Mesa验六级/层间采样。r1/r2同步Aegir ctor测试被原扩展check-in timeout打断；保持失败记录和同库准入，下一调整诊断执行位置后继续完整覆盖。

最新后台整组实机已通过，见METAL-BACKBOARD.md。build_exception_observer.py --target-process backboardd必须给--gles-ram-package精确GLES、--metal-resources及完整compute/render/constants/XR/Surface/MIP fixture；此模式禁止Aegir登记。背景线程只在原GLES八项成功后执行，不改变check-in。新库CPU/GPU及新包CPU/GPU/完整回程均须验收，目标路由和字节在package_trial/ram_library中强制。

当前dynamic narrow与half适配/验收见AIR-NARROW.md、AIR-HALF.md。固定OSS源保留，materialize只对build副本顺序应用独立patch；half MIT源/notices在air/half/reference。make_air_fixture.py --fixture narrow|half，build_exception_observer.py --narrow-fixture/--half-fixture加入完整backboard验证，新包CPU/GPU实际通过后才registry-only。短登记必须--native-proof及run_trial --native-evidence从原raw重新完整核验，库/源码/内核/日志token全匹配。

当前target helper变更可独立重编实际host AIR→ESSL工具，不重用包含旧HalfESSL的CLI：

    python3 tools/build/build_graphics_cli.py --air-package build/air-half-r1/frontend --llvm-headers deps/llvm-c-14 --output build/my-graphics-cli

该工具验证固定AIR包原source-map，以当前ESSLTarget/HalfESSL重新编译graphics-cli，记录完整源/工具SHA。check_air_half.py验当前helper的全部half位型/float32边界及实际AIR compute/vertex/fragment；profile_gles_shader.c可独立测原shader的host compile/link，不绘制、不作为手机时耗或UI证据。half-link优化保持GLES字节，重新构建Metal/observer/RAM包，沿同GLES先验→新包CPU→完整backboard GPU→新native-proof→原Aegir短登记流程。

当前sampler输入及自身硬件准入见METAL-SAMPLERS.md；build_exception_observer.py --sampler-fixture只用于完整backboard suite并要求MRT/half原先验配置，native-proof增加AIR_sampler。新GLES1c904beb与Metalb0b8045b的完整CPU/GPU/回程已验证，原Aegir仍需独立验收。

程序二进制与后台预热的最新实现/证据见 [PROGRAM-CACHE](PROGRAM-CACHE.md)。build_gles.py --mesa-build/--frontend-output在新目录重建完整Mesa，build_frontend_trial.py --mesa-build记录实际归档；RAM绑定及run_trial审计使用该安全相对目录。缓存库增加Program_cache自身实机证明，完整native-proof共13组；只保存实际runtime request，在完成原GPU回程后才单独 --program-cache-prime，不能以host或旧12组证明准入原Aegir。

静态half常量内存适配/全位型及14组原raw验收见 [AIR-STATIC-HALF](AIR-STATIC-HALF.md)。materializer在原引用副本顺序应用lilac-air-static-half-bits.patch并复制StaticHalfBits.h；build_air_frontend.py记录新输入/真实iOS LLVM导入。make_air_fixture.py --fixture static-half、build_exception_observer.py --static-half-fixture添加自身GPU组，新库旧13组proof拒绝。

XR Clear优化的最新GLES构建/原位型门槛与真实性能证据见[XR-CLEAR](XR-CLEAR.md)。build_frontend_trial以实际源码锁和同一Mesa归档生成新库，再依次新GLES CPU/GPU→完整native包CPU/14组GPU→新native proof→原包CPU/GPU。CPU与全GPU库身份须完全相同，旧0a2c7e69不能准入72d39da5。
