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
