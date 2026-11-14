# 固定源码与构建

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
