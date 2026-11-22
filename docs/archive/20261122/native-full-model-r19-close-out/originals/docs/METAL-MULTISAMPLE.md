# Metal 2×／4× MSAA候选 — 2026-11-20

最新tiled GLES c617f2ee已通过自身CPU／GLES9，reference CPU b2f3fc4f因独立启动日志字节不一致拒绝，native21未投递；本页此前离线及失败记录保持，准入以[STATUS](STATUS.md)为准。详见[ATTACHMENT-HARDWARE](ATTACHMENT-HARDWARE.md)。

2026-11-20恢复新蓝灯会话后的最新实际验收与12个失败页面见[ATTACHMENT-HARDWARE](ATTACHMENT-HARDWARE.md)。当前GLES21de自身CPU／GLES9已通过；完整Metal21的深度／模板／Memoryless／MSAA首阶段原字节失败，原presentation／PNG未执行，不能提升此页旧离线结果为整组通过。全部0x06回程／健康／worker退出完整，失败包保留不重发。

此页保存MSAA候选的原离线结果；当前源码另增[原生presentation-r1](METAL-PRESENTATION.md)，同GLES配新Metal须完整native21，不能以此页旧native20或旧Metal-source-map准入。

正式源码已接入颜色／Depth32Float／Stencil8的2×和4×附件、Private与Memoryless存储，以及颜色平均／深度Sample0、Min、Max的GPU resolve。iOS ARM64构建、原生对象ASan／UBSan、实际AIR→ESSL主机Mesa绘制及独立字节核验通过；没有投递手机。已验基线仍为v730／CA矩阵GLESd6d8fcfa／Metal5e94ab57，完整壁纸、UI／FPS、睡眠、LCD光学和其它外设继续。

## 开源契约与页面

对象与resolve语义参考[ios_sagit QcomMTL](https://github.com/MC-XiaoXiao/ios_sagit/blob/898998cf1b74ea103e0c16d6dc0016822d8d644b/drivers/gpu/QcomMTL/QcomMTL.mm)，BSD-2-Clause。固定[Mesa24.3.4 Freedreno](https://gitlab.freedesktop.org/mesa/mesa/-/tree/769e51468b49b2a42f0a0eaf71cf9eed5ff4e5de)的A5xx资源布局、纹理采样及独立Z32／S8扫描规则保持。新增代码与作者AIR fixture进入正式仓库；无Vulkan实现。

新`MetalMultisampleAPI.h`独立C ABI保持旧单采样API，`MultisampleLayout.h`按原A5xx每像素交错samples布局计算stride／offset／容量。导入的`pipe_resource.nr_samples`和`nr_storage_samples`都是真实2或4；实际FD资源cpp、stride、offset及样本数必须吻合，补external importer未初始化的fdl样本字段。`st_context_teximage`原本初始化单采样颜色元数据，随后在同一保留资源上调用`_mesa_init_teximage_fields_ms`描述真实多采样颜色／深度／Z32S8图像，不另拷像素。

仅2DMultisample、单mip、RenderTarget；Private原页和每encoder独立Memoryless页分别保持。Depth32与独立Stencil8页面采样数相同，Mesa separate-z32s8只转交一个原引用。所有纹理、管线、附件采样数在录制与执行两侧重新核对，宽高和范围检查先于窄整数转换。公开Private／Memoryless纹理不允许CPU访问；诊断租约只借已有作者测试页面，原UI不用它。

`MetalMultisample.inc`保留不可变源纹理／resolve目标／请求，`endEncoding`在所有绘制之后追加同queue的resolve。Clear-only先清后resolve，Load-only可resolve前一pass保存的样本。目标与本pass所有颜色／深度／模板源和其它resolve目标有任何重叠均拒绝，原descriptor不被改写。Memoryless只允许DontCare或MultisampleResolve，不允许跨pass Load或StoreAndMultisampleResolve；私有页一直保留到resolve的fence或错误回收。

实际GPU shader读取每个原样本并写单采样目标。颜色在shader中平均；Depth32支持Sample0、Min、Max，Metal枚举ABI0／1／2由编译静态检查绑定。resolve关闭dither、sample mask、stencil和不相关blend状态；没有CPU平均或repack。原sysmem／nofp16策略保持，未改Mesa归档、内核、allocator／SCM／PON／RPC／VM或SweetGPU。

XR多采样、stencil resolve、sampled-depth／stencil、数组／多mip／IOSurface多采样和depth-stencil与XR有序image blend仍缺。当前program-cache请求没有sample字段，所以MSAA管线不读取／写入旧单采样binary缓存；普通管线缓存策略保持。缺功能明确拒绝，不通过伪造sampleCount或忽略resolve放行。

## 实际离线结果

| 输入 | 精确身份 |
| --- | --- |
| 同核v730 | `f258dee9c6d142b90a573f796eeec5e92b39dd65feed6a21b2542d90fb66a88d`／20665112B |
| 候选GLES | `21de6821910c452f1bda70720f3573b81ac76483a10f1959ef95e7b0bc9447b4`／14784032B |
| 候选Metal | `b25bfe6c61bb0b5c7dd625e930762ea1e002290e1555e407fff85d4ca136e2ee`／807568B |
| Metal source-map | `149d4577effba966ca00f16b999256650252f07b6c9f3673c9fc33618c0e23d9`／47源 |
| 完整reference observer | `34219504ad427b5923df4a7ed1d83bee4e52f285f2680de3b3aa7920174aa067`／238384B |
| RAM blob | `188cf3289587da4e98c54dd869d9099c93ff2f931fc94375d758c77bcd7db30d`／4295582B |
| GLES自身CPU包 | token`87c89bcff0f44fed`／image`0ed4022472b4c95ba702b61855e5d3218b6c7edd1ff91760cdfc24f880b00b85` |
| reference自身CPU包 | token`386c4f42212f7715`／image`4e46f3d68cd5839b9a6354d0eaf02235a0075665bfa9f302783f5488a64f2f12` |

实际主机Mesa llvmpipe／23.2.1执行新AIR→ESSL shader，48draw、5508独立颜色／深度／模板sample、26928B和8resolve通过UBSan；非单调深度让Sample0／Min／Max产生不同结果，resolve后复读所有原样本。它没有执行FD540 external importer，不能称手机GPU通过。原生对象、所有单采样回归、新MSAA的sample-mask快照、Clear-only／Load resolve、重叠拒绝、失败回收和Memoryless双queue页面隔离通过Mac ASan／UBSan。主机测试map加锁，避免两个queue的host model自身竞争。

18阶段实际作者probe本体在原生对象host model中执行，90记录／151488B／55098Bguard被独立Python复算；C配方的逐页hash也与Python相同。host model明确不是硬件证据。CPU包已offline同核private原截图先验、完整payload和Mac83实际源audit通过，download／boot／reset均0；未打包或重发新的GPU boot。

原失败保留：reserved GLSL变量`sample`、错误Mesa include路径、局部`samples`重名、旧单采样host误走MSAA入口、Mac无LLVM14可执行文件、CLI错误输入MTLB、主机整数vertex texture少NEAREST导致全黑。最终使用Ubuntu-22.04的LLVM14、正确AIR输入与生产一致vertex绑定后通过。失败／旧产物来源SHA保留，不覆盖旧raw；废弃重复容器与一次性staging按原生回收规则处理。

## 正式源码复现

Mac和Windows只在正式仓库操作，输出必须新建，固定Mesa／AIR准备见[BUILD](BUILD.md)及[IR3](IR3-VARIANT-CACHE.md)。Mac保持`~/xnu-autobuild.pause`。

```sh
python3 -B tools/build/build_frontend_trial.py --mesa-build build/mesa-ir3-cache-r1 --output build/my-msaa/frontend
python3 -B tools/build/package_glram.py --frontend build/my-msaa/frontend --output build/my-msaa/gles-ram
python3 -B tools/build/check_metal_compute.py --output build/my-msaa/host-native --llvm-headers deps/llvm-c-14
python3 -B tools/build/build_metal_library.py --ram-package build/my-msaa/gles-ram --air-package build/static-initializer-r1/frontend --output build/my-msaa/library
```

新作者fixture使用现有Ubuntu-22.04 LLVM14；不是要求Mac安装不存在的路径。输出在正式Windows build可被Mac复制同字节后使用。

```powershell
wsl.exe -d Ubuntu-22.04 -- python3 -B /mnt/d/Desktop/Git/ios-xperia-xz1c/tools/build/make_air_fixture.py --llvm-as /usr/bin/llvm-as-14 --fixture multisample --output /mnt/d/Desktop/Git/ios-xperia-xz1c/build/my-msaa/fixture
python -B tools/build/check_multisample_gles.py --distribution Ubuntu --output build/my-msaa/host-gles
python -B tools/build/check_multisample_recipe.py --distribution Ubuntu --output build/my-msaa/host-recipe
python -B -m unittest discover -s tools/device -p test_multisample_evidence.py
```

AIR→ESSL使用`build_graphics_cli.py`重建实际CLI，然后以`gpu/metal/air/fixtures/multisample.air.ll`为输入生成vert／frag；CLI读取AIR，不读取MTLB容器。`check_multisample_gles.py --generated-shaders build/...`执行这对真实generated shader，manifest绑定其SHA。`check_metal_multisample_probe.py --native-host ... --fixture ... --llvm-headers deps/llvm-c-14 --output ...`执行实际probe本体和独立日志核验。

完整observer命令保留所有19组前序和PNG6，并新增`--multisample-fixture`：

```sh
python3 -B tools/build/build_exception_observer.py --output build/my-msaa/reference-observer \
  --metal-resources build/my-msaa/library --target-process backboardd --gles-ram-package build/my-msaa/gles-ram \
  --air-fixture build/air-r1/fixture --render-fixture build/render-r1/fixture --constants-fixture build/constants-r1/fixture \
  --xr-probe --surface-probe --mip-probe --narrow-fixture build/air-conversion-r1/narrow-fixture \
  --half-fixture build/air-half-r1/fixture --mrt-fixture build/metal-mrt-r2/fixture-final \
  --sampler-fixture build/link-cache-r1/sampler-fixture-lf --static-half-fixture build/static-initializer-r1/fixture \
  --clear-cache-probe --ir3-cache-probe --depth32-probe --stencil8-probe --memoryless-probe \
  --multisample-fixture build/my-msaa/fixture --cif10-png-reference
python3 -B tools/build/package_glram.py --presigned --frontend build/my-msaa/gles-ram \
  --observer build/my-msaa/reference-observer --metal-resources build/my-msaa/library --output build/my-msaa/reference-ram
```

首次pull可用`make_air_fixture.py`分别生成basic／render／constants／narrow／half／mrt／sampler／static-half；路径按实际输出传入上述选项，不要求复制旧build。SDK／时间会改变签名库SHA，所有新二进制需自己的实际准入。

## 未来手机准入

新`MetalMultisampleProbe.h`共18阶段：2／4采样分别Clear-only→独立sample-mask绘制→另一pass只改sample0→颜色平均和depth Sample0／Min／Max→默认all-samples绘制及resolve；另有2／4各两queue共享公开Memoryless纹理、不同私有页及不同结果。所有源及目标的前后guard／row padding、原handle／IOVA／pass身份、实际import、separate-z32s8配对、每次退休、queue fence／顺序和final allocated0均从本LIVE→ACK原raw核验。原样本在resolve前后保持，全帧数值由作者recipe独立计算。新日志最长147B，低于190B界限。

自身GLES CPU→新GLES9→reference CPU→完整native20／PNG6→重新生成native／PNG proof→原poster CPU／GPU及150秒串口截图。`native_multisample_attachments`要求`Metal_multisample`，旧native19在build／RAM／run准入拒绝。probe错误在本LIVE后走现有0x06回程并保留REVISE；无LIVE不发控制。

用户本轮明确要求继续离线；r4／r5失败后尚未确认新物理fastboot会话，枚举健康不等于恢复，不能因新token重发。仅BH905ND69G、COM≥10／VID1209-PID0001、单download／boot、PON0x06、四回程marker／健康30秒／旧worker退出规则保持。串口720×1280 PNG用于UI，DroidCam仅光学／背光验收。

全部精确输入、已执行host结果和离线audit见[MSAA offline proof](../reproducibility/multisample-attachment-r1-offline-proof.json)。此前七份手册原字节和SHA保存在[archive](archive/20261120/multisample-r1/original-document-manifest.json)。

本轮实际回收Windows20项／54文件／71492607B，Mac35项／878文件／378478631B，全部原生回收并核对目标。两Ubuntu垃圾桶为空；Windows／Mac未清空。当前CPU包、raw／zblob、精确输入和原失败保留。详情见[回收清单](../reproducibility/multisample-attachment-r1-native-recycle.json)。
