# Scoped original Metal registration — 2026-11-15

原Aegir登记已与长native GPU诊断分离。registry-short-r1/r2只登记原MTLAddDevice→MTLCreateSystemDefaultDevice并观察原factory/异常；不会在扩展constructor重跑全部GPU探针。单发前native_prerequisite.py从前一完整backboard实际results/raw log重核内核/GLES/Metal/源码、全套byte/fence和回程，不能用摘要或模拟结果准入。

固定native-proof的版本、内核/GLES/Metal/source-map SHA、前轮token/image/raw SHA与必需探针列表嵌入observer并写短串口记录。RAM包校验schema/header，run_trial --native-evidence再独立重验原日志并与嵌入proof全匹配后才派发。新AIR窄整数补丁的库须额外AIR_narrow完整实机证据，旧库证据不能替代。

r1在原default得到真实SweetMTLDevice后，越过mipmap而到setRenderPipelineState:nil。原controller技术目标PASS、原应用异常仍保留。tokenb94918360aedfdee，585402B原串口SHAbc7b3b6cfbba400aac2baa3d8386015906a423bced8d291cb89e94b36925f498，37.640秒完整PON06回程。回程ACK后一次R8 Surface拒绝不作为LIVE期间功能失败。

r2新增只读pipeline/library factory观察，保留原对象、NSError**地址、nil和原异常。真实host方法测试覆盖超过32条日志界、返回nil/nonnull、原错误和throws，ASan/UBSan通过；原接口按ObjC参数数和类型验证后才观察。target只Aegir，原Metal/GLES库不变。

r2真正管线错误为aegir_pass_vsh的AIR JIT: AIR conversion type disagrees with its intrinsic name。原描述符使用555/BGR10_XR_sRGB，以及R8 threshold；sample1、无depth/stencil、原function有效且无vertexDescriptor。实际short2通过buffer0读出，再使用动态signed16→float intrinsic；固定开源AIR前端此前只允许常量narrow，导致管线nil。对应原库和字节只读提取、修补与数值门槛见AIR-NARROW.md。

构建新短observer：build_exception_observer.py --registration-only --register-device --metal-resources指定已验库 --native-proof指定复验输出，可加--pipeline-observe。先本包CPU，后GPU并同时给精确--gles-evidence与--native-evidence；所有0x06/单发/健康/原worker退出规则保留。registry_proof_evidence.py与pipeline_observer_evidence.py只证明原技术登记/错误 metadata，完整原壁纸/UI/FPS和外设仍需实机验收。

r3的新narrow库原Aegir越过short2错误，转到half扩展不支持；r4在half完整实机proof准入后，原threshold实际返回nonnullpipeline。其它XR管线与原app check-in timeout仍未解决，具体边界见AIR-HALF.md；原REVISE保持。

r5绑定新的完整MRT/FP32 image硬件proof（原raw重验），记录每个真实颜色附件/因素/遮罩。原starfield描述符为555与R8，后者writeMask0；新后端不再立即因单附件拒绝，但进入Mesa create能力日志后一直未返回，随后原check-in timeout。tokend766c9ef26cfb30b的399240B原raw SHA77e87e68966b91464b3cb754cb7ed74de8ba68584e805e3b7aa131509d7be920，38.687秒完整PON06回程/健康30秒/原worker退出。完整MRT原证据仍有效；原shader编译时耗与constant sampler及真实UI继续分析，不用离屏结果补原应用成功。
