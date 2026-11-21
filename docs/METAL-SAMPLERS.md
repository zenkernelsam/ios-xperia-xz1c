# AIR 常量 sampler 与编译复用 — 2026-11-15

此页保留该能力的原版本实现与实机证据；每个旧“当前”仅指当次日期。现行精确基线／候选／设备只见[STATUS](STATUS.md)，复现与单发分别见[BUILD](BUILD.md)／[OPERATIONS](OPERATIONS.md)。历史阶段结果保留其准确边界，完整原UI／光学需独立验收。

生产 GLES 绑定器已适配 ios_sagit 的 constexpr sampler 契约，处理普通与常量逻辑 slot、完整 `_c_` 七字段／`_level`、同一阶段同一 literal 的状态一致性。逻辑8..23映射到真正纹理单元，避开顶点buffer的8..15和XR LUT的16；单位总数按真实GL能力检查。普通 mirror-clamp 的坐标／导数标志同步到原AIR辅助uniform。

来源固定为 MC-XiaoXiao/ios_sagit `673f85070fe9f7872793015eb230a5c3987d4a8c` 的 BSD-2-Clause `Interface.h` 与 sampler state 校验，逐文件 SHA 在 reproducibility/sampler-reference.json。适配只进入正式仓库，不更换当前 Kernel/SweetScatter ABI。

当前范围为 normalized 2D：filter、LOD、寻址、border、anisotropy 依真实 GLES 能力执行；未提供 unnormalized／shadow／array/cube 适配时明确拒绝。主机实际GLES/ASan/UBSan 25检查、11精确像素通过，包括无需外部sampler对象的literal、两个寻址状态、显式LOD、min/max LOD、border与保留纹理单元。

原生GPU探针是作者 `sampler.air.ll`，直接使用原壁纸的 `0x807bff0000080a49`、一字和两字global constexpr、两个独立采样状态与显式LOD。源纹理真实slot5、两mip、33×19目标，不调用setFragmentSamplerState；三个16384B原始buffer各三phase，共147456B，包括原输入／顶点／前后及行padding。实际字节以完整覆盖的原data/run记录保存，独立验收展开后逐byte、FNV、fence、状态和最终0分配复算；失败／缺块／重复／越界／变更输入都拒绝。

新库只有同kernel/GLES/Metal/source-map的完整backboard硬件证明中包含 `AIR_sampler`，才准入原Aegir短登记。probe不在原应用constructor运行，新builder也拒绝把sampler探针放入Aegir。原生fixture的编译与主机检查不能代替手机结果。

编译端缓存同一原生function的成功AIR翻译，依其不可变library slice和function-constant快照区分，按对象寿命释放，用每function互斥保护；不同specialization不共享entry，失败不缓存。raw NSMutableData也先复制为library所有的不可变输入。host原生资源／渲染／常量／mipmap／MRT对象ASan/UBSan已通过。

half舍入维持原零／subnormal／overflow／NaN规则，用 `4095 + retained_low_bit` 的整数偏置实现最近偶数舍入，减少normal与subnormal尾数比较分支。主机全部65536half位型＋65536float32 tie/边界/random通过；原starfield仅换helper、无缓存host链接1.991→1.122秒。native packing实验14.212秒已撤回，未发设备。当前结果不是手机编译时耗；生产端新增vertex/fragment/link独立begin/end、wall与thread CPU计时。

可复现入口：

    python3 tools/build/build_frontend_trial.py --output build/my-sampler/frontend
    python3 tools/build/package_glram.py --frontend build/my-sampler/frontend --output build/my-sampler/gles-ram
    python3 tools/build/build_metal_library.py --ram-package build/my-sampler/gles-ram --air-package build/air-half-r1/frontend --output build/my-sampler/metal
    python3 tools/build/make_air_fixture.py --fixture sampler --llvm-as /usr/lib/llvm-14/bin/llvm-as --output build/my-sampler/sampler-fixture

先新GLES CPU/GPU，再新Metal包CPU和完整backboard GPU（basic/render/constants/XR/Surface/mip/narrow/half/MRT＋sampler）；builder增加 `--sampler-fixture`，其余完整fixture仍必需。root首次配置依BUILD.md，AIR固定来源按AIR-HALF.md物化／编译。Mac原生clang/Xcode负责目标编译，作者AIR使用固定WSL LLVM14组装后按SHA转入Mac。

状态与实际token见STATUS.md；源、host及原r6失败和回程证据在reproducibility。原r6首starfield仍check-in超时，不改写为UI通过。完整壁纸／时间日期／系统UI/FPS／睡眠／声音／无线仍待验收。

实机sampler-r1已完成新GLES CPU/GPU、新Metal包CPU/完整GPU：sampler147456B三phase/1881pixels全部原byte/输入/顶点/padding零差异；序列913/919/923递增、最终分配0。原全部组、half131072B与MRT1245184B保持。完整native-proof含AIR_sampler，38.719秒四标记/健康/原worker退出。原壁纸需另行验收，不由后台通过推断UI。

原Aegir r7已实测：vertex/fragment compile成功且各约45–96ms，link_begin后没有link_end/原pipeline返回，原check-in timeout。38.484秒完整回程。当前卡点为GL链接，完整原壁纸/UI仍未通过；不得把sampler完成表述成壁纸已修好。
