# 原始快照耗时观察 — 2026-11-16

原Aegir r2在五个实际管线恢复后已出现两次750×1624 GPU draw，但扩展仍在check-in期限内退出。当前靴的三个采样分别处于malloc释放、ColorSync ICC变换初始化、ImageIO PNG过滤/deflate/finalize。采样位置只能说明退出时正在执行的路径，不能据此认定VM、色彩配置或PNG实现有故障。

postdraw-r1仅在原Aegir的短登记observer中添加四个标准API的入口/返回观察：UIImagePNGRepresentation、CGImageDestinationAddImage、CGImageDestinationFinalize、ColorSyncTransformCreate。与已有IOSurface观察相同，使用独立dylib的标准dyld interpose section；不修改原系统映像，不调整图像属性、压缩参数、颜色配置、像素或check-in期限。每次原调用保持参数及返回对象/布尔值、nil、异常和errno。记录PNG返回长度与CGImage尺寸/stride/颜色模型等metadata，不读取原像素页或PNG内容。

每进程最多记录64次调用。pid/tid/call/root/depth区分重启与嵌套，monotonic wall和线程CPU时耗分别记录；入口/metadata日志在计时开始前，子调用的日志和时耗仍被父调用包含。不能把嵌套时耗相加当成总耗时，极短调用的线程CPU读数还包含时钟采样开销。CPU时钟失败明确cpu_valid=0；缺少某API记录不证明该API没有执行，尤其shared-cache内部调用可能无需通过被观察的绑定。

独立Mac ASan/UBSan验证保持原返回/nil/异常/errno和64条日志界；真实ImageIO对sRGB、DisplayP3及750×1624图像的编码字节、解码像素、ICC、metadata和原输入逐一相等。iOS-only PNG API在host使用明确标注的转发fixture，不能当成手机API或硬件通过。目标arm64编译对四个函数指针作静态ABI核验；实际Mach-O的__DATA,__interpose为64B、四对条目。只有实际手机的BEGIN/END才能证明观察到了原调用。

构建保持kernel774b5efd、GLES0a2c7e69、Metal5e94ab57及其完整14组native proof；不改allocator/SCM/PON/RPC/VM或SweetGPU.cpp。例：

    python3 tools/build/build_exception_observer.py --output build/my-postdraw-observer --metal-resources build/half-selection-r1/metal-resources --registration-only --register-device --native-proof build/half-selection-r1/native-proof.json --pipeline-observe --postdraw-observe
    python3 tools/build/package_glram.py --presigned --frontend build/half-selection-r1/registry-ram-r1 --observer build/my-postdraw-observer --metal-resources build/half-selection-r1/metal-resources --output build/my-postdraw-ram

新RAM先做独立CPU实际单发/完整回程，再以完全相同RAM和native45a证明单发原GPU。tools/device/postdraw_observer_evidence.py逐条核验绑定、嵌套、原返回及原raw/checkpoint/完整回程；保留未返回调用与原REVISE，不用诊断成功把UI失败改为PASS。

原A2S v2符号表来源另补完整SHA5bf04681…、765432322B、10898361个排序记录；reader用12B header和12B address/offset，原shared-cache slide按当前靴记录核对。私有symbol map和原镜像保持在assets，不进入Git。原分析见reproducibility/original-aegir-r2-postdraw-analysis.json。

本包CPU872bd5d6已38.328秒完整回程通过；原GPUd25c7eea完整39.985秒回程/健康/退出，原REVISE保持。可见两次transform create约0.4ms，外层PNG均未返回。当前靴栈仍有ImageIO内部ColorSync初始化和原Metal快照等待，说明内部调用可能绕过标准绑定；这些读数只限定两个已见调用，不排除其它ICC工作，也不证明PNG过滤选项可解决UI。真实面板仍缺壁纸/时间日期，原结果与该限制保留。

snapshot-r1另增加八类原方法的精确ABI计时：真实pipeline/library factory、CLKUIMetalQuadView snapshot、command wait/execute、两个texture getBytes。只在类自身拥有且返回/参数编码匹配时安装，拒绝不匹配、继承和重复安装；CGRect32B/CGFloat8B/MTLRegion48B/NSUInteger8B目标ABI检查及host原参数/返回/异常/errno/read字节/guard通过。实际目标覆盖由逐进程metadata决定，host不能替代。对象标签关联wait/execute与读回，不读取原像素。计时从observer初始化起，不能当完整extension启动预算。

原postdraw-r1的GLES八项结束标记在2893行，首次Aegir exec在3964行；没有直接自检/应用启动重叠，未关闭或修改自检。该排序不能排除其它后台CPU工作，继续测原快照自身。

snapshot-r1实际CPU de5cda38已38.797秒回程通过；原GPU426db2f1已38.625秒完整回程/健康/退出，原REVISE保持。28个完整/6个未完成时耗：pid290原快照3.307秒，包含wait2.807秒、execute2.809秒（线程CPU0.605秒）及read21.222ms；随后原PNG3.911秒（线程CPU3.896秒），返回32179B。这些是原API真实返回，仍不能代替完整UI。wait与execute重叠，PNG含ICC/编码，不相加混算，不归咎冻结VM。原面板仍只有暗背景/电池/camera/homebar，继续优化两段真实耗时。

## PNG输入契约观察 — 2026-11-17

XR Clear和实际Luna请求预热后，原GPU c32999dff23480c9仍未完成扩展check-in。按该靴实际slide 0x4ed00000解析的三个ACK前sig9分别处于快照队列等待、ColorSync内部CMM转换序列初始化，以及PNGWritePlugin写出/memmove。它们是退出时采样，不证明这些组件有故障；完整记录见reproducibility/v729-xr-clear-r1-after-luna-cache-crashes.json。两次PNG入口仍未返回，原四个API绑定也未观察到内部addImage/finalize，所以旧IMAGE记录不能证明PNG输入格式。

postdraw observer revision2在原UIImagePNGRepresentation入口读取UIImage/CGImage metadata：点尺寸、scale、orientation、实际像素尺寸、bpc/bpp/stride、原bitmapInfo/alpha/renderingIntent、颜色模型/分量、是否广色域或扩展范围，以及有界色彩空间名称。只调用返回编码匹配的原getter；nil、缺getter或getter异常明确记录，再继续原PNG API。读取不创建图像、不读取像素/provider页或ICC内容、不调整压缩/色彩配置；原参数、返回、异常和errno保持。getter耗时单独记录，在原PNG计时之前；父快照的含义和原check-in期限保持。

仍为每进程64调用、四个interpose配对；新增PNG_CONFIG、PNG_INPUT、PNG_UI、PNG_CG、PNG_COLOR记录按当前token/pid/call绑定。revision1原证据兼容保留；revision2拒绝缺失、截断、混call或未声明的metadata。实际nil/error不会被改写为成功图像。

tools/build/test_png_input_observe.m使用明确标注的Mac UIKit转发fixture，同时用真实CoreGraphics/ImageIO检查8/16/32位sRGB/P3/扩展线性图像。provider读计数验证getter不读像素，原PNG字节、解码像素、ICC/metadata和输入保持；nil、getter ABI不匹配、getter异常、原异常和errno分别检查。这些主机结果不替代原手机输入或UI验收。

新包保持kernel774b5efd/GLES72d39da5/Metal5e94ab57及native4bcd完整14组先验，只重新构建observer与RAM。示例：

    python3 tools/build/build_exception_observer.py --output build/png-input-r1/observer --metal-resources build/xr-clear-elision-r1/metal-resources --registration-only --register-device --native-proof build/xr-clear-elision-r1/native-proof.json --pipeline-observe --postdraw-observe --snapshot-observe
    python3 tools/build/package_glram.py --presigned --frontend build/xr-clear-elision-r1/original-ram --observer build/png-input-r1/observer --metal-resources build/xr-clear-elision-r1/metal-resources --output build/png-input-r1/ram

该新RAM必须自身CPU完整回程后，再以同RAM和native4bcd原目录准入原GPU。手机实测结果仍以STATUS与独立reproducibility记录为准；不能把输入观察或主机PNG通过当成完整壁纸/时间日期/UI/FPS完成。

新RAM CPU ce96235e已38.594秒完整回程通过，原GPU c45fec49e9327a92已39.578秒完整回程/健康/退出，原REVISE/check-in超时保持。两个原750×1624 Clear/draw成功；snapshot3.369/2.146秒，两个PNG均未返回。首次取得真实UIImage输入：750×1624、10bpc/32bpp、stride3000、bitmap0x42000、alphaNone、ExtendedSRGB、scale2/orientation0；getter耗时2.408ms/40.677µs。0x42000对应RGBCIF10与32Little；公共CGImage头声明其最高两位应为1，但本轮没有读取原像素，不能据此推定真实最高两位或改GPU位型。

Mac为AMD Ryzen7 5825U/x86_64、macOS15.8.1。独立profile_cif10_png.m对自建CIF10色阶/混色/750×1624图像调用默认ImageIO，输出为16bpc RGB PNG。8位及16位简化算术转换均有像素/metadata差异，未准入或安装；ASan/UBSan通过只证明该主机工具的边界，不能把AMD/x86颜色结果当成835/iOS的数值参考。全量失败对照与初次编译错误保留reproducibility/cif10-png-host-r1-rejected-predictions.json，未修改原PNG参数或数据。

## iOS后台CIF10参考

新增--cif10-png-reference只允许完整backboardd suite。它在原GLES及全部14组native成功后，创建五组作者自建的CIF10图像，使用原UIImagePNGRepresentation，再用原ImageIO解码。四组1024×1色阶/混色/最高位对照仅导出少量自建像素和第一组ICC；750×1624整屏只记录长度/位深/位型/FNV/耗时。最高位为0的一组明确是文档范围之外的对照。原Aegir及其启动期限不改，不读取原UI像素、不安装替代编码器。

CIF10PNGReferenceProbe.h与test_cif10_png_reference.m分别记录目标API与明确的Mac UIKit转发fixture。host对有界数据/导出、原输入不变、真实ImageIO/nil/异常检查通过；这些不能代替iOS参考。cif10_reference_evidence.py按当前token验证全部原输入、连续导出/长度/FNV、五组原API返回及完整回程，拒绝截断/混case/失败，解码数据只能输出到Windows TEMP。参考PASS不能准入尚未实现的优化器，也不能当原系统UI/FPS成功。

新reference RAM依旧只变observer，须自身CPU准入，再同GLES72d39da5/Metal5e94ab57单发GPU，完整四回程/健康30秒/旧worker退出后继续。任何新颜色/PNG优化先取本机参考并核对像素、profile、metadata及返回行为；主机更快而输出不同的候选保持拒绝。

后台参考已实机完成：CPU2ad33c28/38.015秒回程，GPUff064390/完整14组+五组原PNG/37.813秒完整回程与健康、controller退出。五组输出均为16bpc/48bpp/alphaNone、kCGColorSpaceSRGB，ICC3144B/SHA2b3aa164…；整屏750×1624作者混色PNG4805449B、8.467秒/CPU6.069秒。该高熵样本不代表原Aegir时耗。灰阶在本组最高位0/3对照字节相同；真实混色仅98/1024符合灰阶逐通道查表，单红输入639个码值出现非零G/B，不能用分量查表替代原转换，也不能据此擅改GPU位型或宣称硬件色彩故障。原UI像素仍未读取。完整原数据与范围见v729-cif10-reference-r1-gpu-actual、native14-actual及cif10-reference-r1-native-analysis。

本次构建首先拒绝旧sampler-fixture-wsl的CRLF源码SHA072c376d；保留该失败目录后，以Ubuntu-22.04原LLVM14/f9315b42从正式LF源7b849ba2重新组装，bitcode/library/header三份字节与原先验完全相同，新目录build/cif10-reference-sampler-r1。未改旧manifest或弱化源审计，证据见cif10-reference-sampler-canonical-r1.json。重建入口：

    python3 tools/build/make_air_fixture.py --llvm-as /usr/bin/llvm-as-14 --fixture sampler --output build/cif10-reference-sampler-r1

后台参考probe保留原GLES/native14之后运行的位置；下一步原转换/PNG过滤与byte-swap开销须以原iOS函数和当前源码/库准入，不用AMD/x86快路径或该参考PASS替代原界面验收。

## 公开转换对照 revision2

reference revision2仍使用五组原UIImage PNG；另外以vImageBuffer_InitWithCGImage和vImageConverter_CreateWithCGImageFormat/vImageConvert_AnyToAny比较整图转换。目标颜色空间、位深、stride、bitmap及decode来自该组原PNG的真实CGImage解码。两条路径逐字节核对原解码像素，保留API错误和不相等候选；64B前后guard、成功返回时的buffer身份必须保持。未安装图像转换器或PNG替代实现，参考PASS不能准入数值不等候选。

小图还导出原PNG完整字节，PNG/像素/ICC均有界且只在Windows TEMP落地。cif10_reference_evidence.py的revision2检查全部PNG chunk CRC、IHDR、bounded zlib解压、五种filter和16位网络序→CG原端序像素一致；旧revision1兼容保留。原PNG参数、返回和check-in期限保持。

主机修正前的kvImageDoNotTile误用于InitWithCGImage被真实API以-21775拒绝，未投递设备；正确地把该flag用于直接converter路径后，host ASan/UBSan与输入/guard/nil/异常检查通过。AMD/x86上两条路径均与原ImageIO像素不等，保持拒绝；后续须本包CPU→835 GPU/参考取得真实值，不能以host速度替代本机色彩语义。

revision2 CPU047696d9已38.234秒完整回程；GPU57b6e1ce的controller记GOALS_STABLE/PASS并40.672秒完整回程，但独立gpu_pixels验收把候选VIMAGE的mismatches字段匹配为GPU像素失败，进程exit1。这轮保留独立REJECTED，不能准入新包或改称GPU PASS。独立作者PNG参考/CRC/解码数据有效，单列reference-only；两条公开vImage在835上仍与原像素不同，候选不安装。原错误、raw和完整回程见cif10-reference-r2-postprocessor-rejection。

revision3将候选差异记为differences，保留原GPU全局错误guard；新包仍须自身CPU→GPU单发复验。增加ImageIO baseline、PNG_FILTER_NONE和PNG_FILTER_UP三种作者图像对照：三者都使用原CGImage/原ImageIO颜色转换，DPI144与原scale2一致，逐组核验原UIImagePNG的像素、ICC和完整metadata。仅压缩过滤策略变化，不安装替代编码器。另增加750×1624作者平滑渐变，与原混色样本区分；六组原PNG及filter结果不能当完整原UI验收。

AMD/x86 host ASan/UBSan与六组原输入/guard/nil/异常检查通过；三种ImageIO策略在host像素/ICC/metadata相同，NONE/UP的压缩字节不同。当前尚无本包实机filter准入，目标侧是否保持原UIImage metadata与真实提速须独立验证。旧revision1/2证据兼容，PNG小图CRC/IDAT/filter/端序验证和altered-export拒绝保持。

reference3已完整实机验收：CPU21e8cfa3/47.156秒回程，GPU5221b885/原14组+六组原PNG/过滤对照、38.172秒四回程/健康/controller退出。三种ImageIO策略在全部六组原16位像素、ICC与metadata相同；UP作者混色2.104秒/原8.531秒，作者渐变0.965秒/原4.563秒。这些是作者参考，不能当原界面FPS或check-in成功。公开vImage仍数值不等，拒绝；r2的独立拒绝记录没有改写。原GPU错误guard未修改，新候选字段differences避开歧义。结果见v729-cif10-reference-r3-gpu-actual、native14-actual与cif10-reference-r3-filter-analysis。

下一实现应把相同的ImageIO/UP编码部分抽到正式源码的受约束兼容层，保留原色彩转换、scale2/DPI144、16位/profile/metadata，未支持UIImage输入走原API。仅在原真实GPU snapshot之后编码完整PNG，不能用nil/stub、主机或后台参考PASS替代原Aegir与面板验收；本次尚未把新编码器装到原UI。

## 共享编码实现与原界面准入

PNGImageIOEncoder.h将同一ImageIO编码部分供作者参考与原界面使用，原CGImage不复制、不降位深、不改颜色空间，仍由ImageIO执行色彩转换；UP只改变压缩过滤，scale2对应DPI144。PNGImageDispatch.h只接受精确UIImage类、原CIF10/ExtendedSRGB/alphaNone/bitmap0x42000、scale2/orientation0、raw CGImage、非动画及有界几何。其它输入直接走原UIImagePNGRepresentation；编码失败或异常也用原对象回退，原nil/异常/errno保持。主机真实ImageIO/RGB16像素/ICC/metadata、原输入不变、类型/方向/scale/动画拒绝和原调用身份的ASan/UBSan检查已通过，不能替代原手机验收。

新--png-filter-encoder只允许Aegir短登记及postdraw观察，必须给--png-proof。该proof从新共享函数的后台原UIImage PNG/raw重新核验，绑定kernel/GLES/Metal、共享编码源码SHA、六组UP原像素/ICC/metadata、当前token及完整回程。旧reference3虽通过，但没有新共享源码绑定，不能直接准入新实现。之后仍须新原界面RAM自身CPU，再同库native/PNG先验单发原GPU。

快路径BEGIN明确original_forwarded=0，PNG_ROUTE标明ImageIO_UP；原路径/失败回退仍original_forwarded=1，END对应原行为标记。相机和原Aegir check-in期限保持，不能把兼容编码当成原API实际转发，不能仅凭后台PASS改写原UI失败。最终完整UI/FPS/睡眠/声音/无线目标保持。
