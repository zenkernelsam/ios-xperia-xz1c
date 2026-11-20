# Native AIR/Metal rendering — 2026-11-14

v729/774b5efd与GLES render-r4/7944112d、Metal1b8d1f5d已完成标准MTLB/AIR→system LLVM C→开源lowering→ESSL/Mesa FD540→原Metal texture页面的实际绘制验收。10完整样本、0缺项；每样本一次清屏和三次draw，四个phase共检查17152B逻辑范围（含6560B未写保护/填充），另检查getBytes输出stride和保护区。四个目标hash按独立Python配方重算，全部零差异。不能据此宣称系统Metal、完整壁纸/时间/日期、FPS或其它外设已完成。

本靴token27ed79126f7e4b51，镜像SHA5427210db145a792477b8026c57935652b348669c451dff7633e67cbe84111ae，原日志535739B/SHA00280f14b472aa87c731a176bd9c21fa974b346d49246c9018eedfadcc0681e4。ACK/DRAIN/RESET/FINAL_DRAIN、连续健康30秒和旧worker退出均核对；完整PON06回程38.594秒。CPU新GLES37.281/GPU38.750秒、含Metal的新包CPU37.891秒也已实际通过。最终830资源allocated/released/unmapped，orphan/active/root0；原GLES全尺寸绘制、共享/刷新/CA/lifecycle和960000只读像素仍通过。原生buffer/queue/compute同靴各10份完整证据保持。

目标纹理33×19 BGRA8，bufferOffset64、bytesPerRow192、逻辑buffer3776B；源纹理7×5 RGBA8，offset128、pitch64、buffer512B。phase0红色清屏；phase1实际AIR position/color带viewport(3,2,27,15)、scissor(5,4,23,11)和背面剔除；phase2完整纹理采样；phase3上传新源并仅在scissor(3,4,23,11)更新。所有phase包括行填充和前后保护区，每次GPU fence严格递增；目标和源IOVA不变。源getBytes使用44B输出stride并检查所有240B输出。

| phase | 目标buffer FNV64 | 源buffer FNV64 |
|---|---|---|
|0|20c43a448b59e713|be2eff39d5ab1ebb|
|1|89e37fb6d82caf5b|be2eff39d5ab1ebb|
|2|7dd04d4bf540ea7d|be2eff39d5ab1ebb|
|3|8c2bcd5b14b5f392|55dedfef614ffdd0|

三次失败原样保留在v729-render-r1/r2/r3-gpu-revise.json。r1因Mesa a5xx对顶点SSBO报告0而拒绝链接，38.109秒返回。r2链接成功，但属性检查将gl_VertexID误判，38.250秒返回。r3实际清屏完成，FD5 fast clear/GMEM resolve向每行padding及尾部写入1204B，38.828秒返回。r4用受scissor约束的GPU shader draw实现清屏，原页面无需CPU回填，全部保护区通过。没有重发失败靴或修改既有结果。

顶点buffer通过原页面上的R32_UINT整数纹理取数，4096word/row，16KiB行，逻辑offset/word-count在texelFetch之前检查；片元仍走独立SSBO binding8..15。AIR varying由语义hash连接，刻意交换的顶点/片元声明顺序已验证；Metal y/clip-depth适配、背面剔除、纹理slot13与sampler3、纹理宽高query和动态上传均实测。原参考文件12个仍为ios_sagit898998cf原字节。未使用Vulkan，GPU allocator/SCM/PON/RPC/VM不变。

主机ASan/UBSan运行实际Metal方法体，验证布局/区域/保护、对象持有和encoder snapshot、factory失败回滚、队列终态；生成的真实ESSL另经WSL llvmpipe/Mesa23.2.1/GLES3.2编译/链接/四phase绘制，10032颜色分量零差异。主机结果与A540硬件证明分开。iOS arm64库链接成功，原AIR object168个LLVM C导入审计保持。日志仍可能因并发console打印有损，不能宣称lossless。

当前支持边界：单level2D Shared纹理，显式支持R8/RG8/RGBA8/BGRA8/sRGB/RGBA16Float的储存布局；本轮硬件像素仅核验RGBA8/BGRA8与nearest/clamp。单color attachment、非indexed draw、整数且目标内viewport、smooth/flat/centroid smooth语义；可编码基本blend但本轮未单独验收。其它formats、linear/mirror/repeat、depth/stencil、MSAA、mips/arrays/cube、constant sampler、function constants、vertex attributes、IOSurface texture与system Metal发现继续待实现/验收，拒绝不支持的接口，不冒充完整协议。

复现入口均在正式仓库。Mac先build_frontend_trial.py与package_glram.py产生新GLES；新库先RAM CPU→GPU。make_air_fixture.py --fixture render在WSL用llvm-as-14组装本仓库render.air.ll，3104B bitcode/96a7f5e2和3657B MTLB/c8991170。Mac build_metal_library.py --ram-package新GLES包 --air-package既有audited AIR包，再build_exception_observer.py --metal-resources新Metal目录 --air-fixture compute目录 --render-fixture render目录和package_glram.py --presigned创建新包。再RAM CPU→GPU，run_trial.py --gles-evidence必须是精确同GLES SHA的原完整实机证据。

本机目录build/render-r4/{frontend,gles-ramfs,library,observer,ramfs}；大二进制/镜像不入Git。源码/recipe/源锁、当前独立验收在reproducibility/v729-render-r4-*-actual.json。相机按override→AF→2秒→MJPEG→每5帧→rotate90，仍见充电图标/相机/home indicator，缺时间/日期/壁纸；不把离屏成功代替面板验收。下一接系统Metal发现及原生AIR覆盖，同时保留完整UI和其它外设目标。

本轮收尾回收Windows160项/956681002B、Mac207项261文件/826687085B，均逐项进入原生回收站/Trash并核对源路径消失；未清空回收站。17轮完整serial原日志/结果和每轮关键原照片保留，旧成功stage镜像/重复HFS/raw/MJPEG/多余帧和host对象缓存回收；失败镜像、最新包/库、AIR/LLVM输入与原始失败日志保持。WSL两个Ubuntu实例垃圾桶为空，本轮输出位于/mnt/d的直接走Windows回收站。只读root在Mac已正常detach，source镜像未改。计数和计划/日志SHA见reproducibility/render-cleanup-2026-11-14.json。
