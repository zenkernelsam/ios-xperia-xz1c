# 固定源码与构建

完整无损串口截图的新 v730 诊断增量、主机检查与独立实机准入见 [SCANOUT-SCREENSHOT](SCANOUT-SCREENSHOT.md)。它只扩展显示观察；既有 GPU、源码冻结及同 kernel 各阶段的验证继续保持，实际状态以 STATUS 为准。

固定基线另保留三个源文件的精确行尾物化规则，见 `baseline-source-newlines.json`；`prepare_sources.py` 自动应用，与已验内核的源 SHA 一致。代码文本保持相同。`verify_diagnostic_reconstruction.py --version 730` 在新的 TEMP checkout 中从 Apple 固定提交重新应用完整补丁及 v725..v730 增量；旧／浅缓存缺少固定 base 时只向该 TEMP checkout 拉取。原构建树保持，报告不代表新的二进制或设备已通过。

当前实机入口为v729、XR Clear优化GLES72d39da5和Metal half-selection-r1/5e94ab57，完整14组raw proof见STATUS与XR-CLEAR。下方各版本的构建/失败记录保留其原适用范围；不能以旧版本proof准入当前新库。仅observer诊断变更的最新构建、同库CPU准入和原PNG时耗观察见[POSTDRAW-OBSERVATION](POSTDRAW-OBSERVATION.md)，不重编或改动已验kernel/GLES/Metal。

2026-11-19最新实际入口为v730/f258dee9、原阶段观察GLESa7bad22c及Metal5e94ab57，已完整14组／六组共享PNG和原RAMCPU准入；原UI仍REVISE。该新GLES的真实scope预热与阶段观察见[RENDER-STAGES](RENDER-STAGES.md)。上段旧v729／72d39da5是保留的比较入口，不能准入新库。`validate_current.py`现在同时核对v730补丁及新增截图／行尾物化工具锁；`render_stage_evidence.py`从单轮原raw验证阶段和保存截图，不开启串口。

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
