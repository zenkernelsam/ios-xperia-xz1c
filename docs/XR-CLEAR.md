# XR Clear 旧颜色解码优化 — 2026-11-17

此页保留该能力的原版本实现与实机证据；每个旧“当前”仅指当次日期。现行精确基线／候选／设备只见[STATUS](STATUS.md)，复现与单发分别见[BUILD](BUILD.md)／[OPERATIONS](OPERATIONS.md)。历史阶段结果保留其准确边界，完整原UI／光学需独立验收。

原snapshot-r1实际快照约3.307秒，其中队列等待2.807秒，随后PNG/ICC约3.911秒。等待包含驱动准备、绘制及同步，不能全部当成GPU硬件时间。检查固定Mesa24.3.4/FD540与现行GLES源码时发现：XR附件在loadAction=Clear时仍先将旧XR颜色解码到FP32，再由原Clear整块覆盖。

xr-clear-elision-r1仅在这个Clear分支创建相同的RGBA32F临时目标，省去被覆盖的旧颜色解码。原始整数纹理和raw位型快照继续保留，原bounded_clear、写回、channel mask、padding/低位和fence/生命周期逻辑保持。Load和DontCare继续原解码路径，采样/mipmap路径不改。没有修改half数值转换或关闭原GLES自检。

GLES源metal_render.c由627eb73b…变为7ddadbe2…；原24个Mesa链接输入保持。签名前端736475e8…、打包GLES72d39da5…，内核仍v729/774b5efd。独立新包CPU d7e6a8b4已37.781秒完整自动回程通过，八项GLES GPU877aa6b1已39.593秒完整回程通过。这些基础检查不能替代14组native MRT/XR原字节验证或完整面板。

frontend build binding标记XR_clear_decode_elision，新Metal资源manifest传播该特征，RAM及native proof继续强制精确GLES身份。完整MRT验证除1245184B原输出、mask、低位、padding、8目标、HDR、fence和final_allocated=0外，还要求552..555四种Clear的decode_skipped/original_code_snapshot/original_clear实际记录，以及两个Load阶段各自的解码记录。旧库即使字节测试通过，也不能提供新优化的准入证明。

构建沿正式入口：build_frontend_trial.py指定已审计的build/mesa-program-cache-r2；package_glram.py创建独立GLES RAM；build_metal_library.py绑定该精确新GLES与原static-initializer-r1/frontend；完整background observer使用当前源码/库/头文件都匹配的作者fixture。旧sampler输入一处源码哈希不符已由guard拒绝，改用link-cache-r1/sampler-fixture-lf，未覆盖失败构建。

完整native RAM CPU a0907d08已37.468秒完整回程通过；14组GPU4bcd571c已39.125秒完整回程通过，实际Clear/Load及全部原字节/guard/fence/lifecycle保持。新proof在reproducibility/v729-xr-clear-r1-native-proof.json；原Aegir实机性能与完整UI仍待验收。只操作BH905ND69G，单download/boot，PON只0x06，四回程marker/健康30秒/worker退出后下一轮。原UI超时、PNG真实返回及不完整面板仍保留在STATUS；不能把编译、基础GLES或离屏通过写成完整UI完成。

真实Earth请求73444B在完整14组primer中已按原wrapper长度/FNV配对，42.657秒链接并保存747773B标准binary，39.078秒完整回程。随后原Aegir新包e4eb78cb实际750×1624 Clear省去解码并完成GPUdraw。单个完整wait为1.067315729秒，旧snapshot-r1样本2.806619062秒，约少62%；当前总snapshot3.718521719秒/线程CPU2.570029323秒仍未缩短，缓存/管线CPU负载不同，不能拿wait单项推断总性能。原超时与不完整面板保持，40.172秒完整回程。
