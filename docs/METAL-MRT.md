# 多颜色附件与 XR 混色

> 历史技术记录：下文的版本、结果、“当前”和“下一步”对应原实验日期。现行产物及设备状态见[STATUS](STATUS.md)，构建与单发规则见[BUILD](BUILD.md)／[OPERATIONS](OPERATIONS.md)。原文与SHA见[本轮归档](archive/20261123/poster-state-r22-close-out/original-document-manifest.json)。

此页保留该能力的原版本实现与实机证据；每个旧“当前”仅指当次日期。现行精确基线／候选／设备只见[STATUS](STATUS.md)，复现与单发分别见[BUILD](BUILD.md)／[OPERATIONS](OPERATIONS.md)。历史阶段结果保留其准确边界，完整原UI／光学需独立验收。

本轮实现最多8个颜色附件、各附件独立clear/blend/writeMask、不可变命令快照及原页面逐附件fence/撤销。XR552..555使用FP32暂存与GPU写回，保留未画区域和未改通道的原始码及低位。正式源码在gpu/metal/MetalRender.inc、gpu/sweetgl/metal_render.c、MultiTargetShader.h与XRBlendShader.h。

原Aegir的starfield AIR实际输出两个render_target，star/earth/post请求XR混色。r1新增双附件后，XR552清屏65536B通过，第一混色phase原返回-EIO；没有记录GL错误值，因此不把原因猜测当原实机GL证据。tokencaf703b6a68559d4的REVISE/BACKGROUND_METAL_SUITE_FAILED和完整40.032秒PON06回程保留reproducibility/v729-mrt-r1-gpu-revise.json。

固定Mesa769e51468的A5xx screen不声明PIPE_BIND_BLENDABLE，state_tracker不会暴露EXT_float_blend，GLES draw_validate会拒绝启用blend的FP32附件。该screen还不提供framebuffer fetch。r2增加实际screen/context能力记录与prepare/blend/image-bind/draw的GL错误定位，保持原sysmem,nofp16；不改能力标志、GMEM、内核allocator/SCM/PON/RPC/VM。

XRBlendShader保留高精度AIR输出，在fragment内从readonly RGBA32F image读取目标、按原descriptor混色、逐图元XR舍入后写到同纹理的writeonly image。GLES3.1对RGBA32F的访问限定由两个独立image绑定满足，暂存使用immutable TexStorage。XR image从FBO脱离；只有XR的pass使用实际支持的无附件FBO。每次只派发一个图元和一个实例，下一图元前执行image barrier，保持重叠顺序；vertexStart仍是原值，instanceID通过独立base uniform保留。无CPU生成结果，未依赖fragment interlock。

末尾GPU写回与原XR整数码快照比较，仅更新实际改变的可写通道；未画区域和masked通道保持完整原码。每个原目标独立获取提交sequence并同步，全部资源随队列快照撤销。BGRA/RGBA等普通目标继续使用实际Mesa indexed entry bodies的固定混色和遮罩。

作者AIR fixture包含7个函数。四XR各4phase验证清屏、alpha混色、R/B累计遮罩及实际采样到BGRA/RGBA，另验8个同时输出。追加四XR的两图元×两实例重叠绘制，vertexStart=3、负色和大于1色值及原保护区；完整组检查1245184B。C与Python独立复算原缓冲hash、码值、低位、附件fence和finalAllocated。任何ACK前失败或缺项均拒绝，不用主机stub、另一靴或既有成功补记录。

完整r3现在已实机通过全部原native组和新增MRT：四XR各4phase、8个同时输出、四XR的两重叠图元×两实例/vertexStart3/负色与大于1色值，1245184B全部原byte/低位/保护区零差异。GLESa4632b32、Metal2afbde84、token8cb8d1e96216ee1f；523allocated/released/unmapped、active/orphan/root0，38.562秒完整PON06回程。547262B原raw SHAe8b754efb565d29486c9323bb800d4a59af34d3255229c43cac934549cefa6aa，独立重核的报告及native-proof在reproducibility/v729-mrt-r3-*。

共享RenderUniformRules补齐生产bind_samples对内部uniform/image的类型及显式绑定位置检查；同一规则也在主机实际GLES uniform枚举中执行，并拒绝错误image binding。r2原-45/ENOTSUP和39.891秒回程保留，未派发draw，不提升原失败为通过。真实screen为8目标、FP32 render/image支持、FP32 blend/EXT_float_blend/fbfetch=0、无附件FBO=1、24 fragment images/192 image units。主机ASan/UBSan原生对象/队列快照/同BO非重叠附件/释放保持。完整UI/时间/日期/壁纸/FPS、其它图元的GPU验收、一般blend因素/constant sampler、depth/stencil/MSAA及外设仍待推进。

主机复现入口：

    python3 tools/build/make_air_fixture.py --fixture mrt --llvm-as /usr/lib/llvm-14/bin/llvm-as --output build/my-mrt-fixture
    python tools/build/check_mrt_gles.py --fixture build/my-mrt-fixture --graphics-cli build/air-half-r1/graphics-cli --output build/my-mrt-host
    python3 tools/build/check_metal_compute.py --llvm-headers deps/llvm-c-14 --output build/my-mrt-native-host

check_mrt_gles在Windows运行，重新调用实际Mac AIR frontend生成全部7个shader，再在WSL执行生产混色/写回shader；它保留源/fixture/CLI/shader/log SHA，明确不证明手机原页面或回程。AIR frontend的固定开源来源和许可见AIR-HALF.md与AIR-NARROW.md；XR表的固定LGPL来源/原通知见METAL-XR.md。

新GLES按BUILD.md构建/打包后先自身CPU→完整GLES GPU，再使用build_metal_library.py --air-package构建Metal。build_exception_observer.py --mrt-fixture同时要求完整compute/render/constants/XR/Surface/mip/narrow/half fixture、--target-process backboardd和--gles-ram-package精确GLES包。原GLES字节保留，添加新Metal/observer后再CPU→完整GPU。短登记只有在native_prerequisite从当前完整raw重新验Metal_MRT及全部原组后准入；不在Aegir启动路径重跑长probe。设备单发与完整回程规则见OPERATIONS.md。
