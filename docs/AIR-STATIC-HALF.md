# AIR 常量内存 binary16 按位访问

此页保留该能力的原版本实现与实机证据；每个旧“当前”仅指当次日期。现行精确基线／候选／设备只见[STATUS](STATUS.md)，复现与单发分别见[BUILD](BUILD.md)／[OPERATIONS](OPERATIONS.md)。新附件native21尚未准入，旧阶段成功不扩展为完整UI。

原Aegir在cache-r2/r2已实际恢复星空89,821B、云层84,333B和threshold18,929B的程序，原pipeline返回nonnull；完整UI仍因earth/post管线nil而失败。39.516秒完整0x06回程，原REVISE/raw保留。实际错误为AIR specialization: static initializer memory type mismatch。

固定BSD-2-Clause ios_sagit的常量内存解释器按LLVM类型递归匹配。主机用原earth/post AIR和有明确标注的代表常量重现为source=half/target=i16、offset0、allocation/access均2B。新StaticHalfBits.h与独立lilac-air-static-half-bits.patch只接受一个完整binary16标量的half/i16双向按位读取或替换；使用LLVMConstBitCast，不做数值转换。原引用源码/校验不改；非零偏移、不同尺寸、不同位宽、向量、越界、跨成员、padding等仍拒绝。

实际LLVM/ASan/UBSan全部65,536位型和4条读写路径共262,144项通过，包含正负零、subnormal、infinity及所有NaN payload；错误偏移/尺寸/类型/向量拒绝。原Earth/Post代表常量AIR和ESSL转换通过，Windows/Mac原AIR摘要匹配；此主机数据不是原运行时常量或整机结果。记录见reproducibility/static-half-r1-host.json。

作者fixture static-half.air.ll从半精度与ushort两个必需原MTLFunctionConstantValues经全局constructor分别half→i16存储、i16→half存储及反向读取，GPU每case输出4个原字。12组位型覆盖正负零/最小与最大subnormal/min normal/1/max finite/正负infinity/quiet与signaling NaN；原16KiB输出页含64B前guard和16304B尾guard逐byte/RLE/hash核验，共196608B。创建pipeline后reset/mutate源constants验证不可变snapshot。

MetalStaticHalfProbe.h作为完整backboard新增第14组AIR_static_half；native-proof、RAM builder与runtime verifier要求新库自身原字节证据，旧13组proof拒绝。保持原half131072B、MRT1245184B、sampler147456B和缓存3次fresh restore等全部原组。新库先本包CPU/完整native GPU/新proof，再原Aegir包CPU/单发GPU；当前kernel v729、GLES0a2c7e69保持，静态别名目标硬件验收已完整通过：新Metal be56ad13、GLES0a2c7e69，token58294c8e0de3c139/raw07fb2c39f2cef68a6be6dd0ec5054b0780d7a2e1fa51b2ef386315b92003164a，38.860秒完整四marker/健康/worker退出。12case共196608B逐byte及全部padding/fence/snapshot保持，完整14组原始结果通过；原Aegir仍需独立验收。

主机CLI为tools/build/air_specialize_cli.cpp，输入本地AIR与entry，以required/all两种代表常量模式输出诊断，SWEET_AIR_OUTPUT只将私有生成源写到调用者指定本地TEMP。LLVM14 fixture构建用make_air_fixture.py --fixture static-half；当前build_air_frontend.py和materializer记录额外helper/patch，编译与iOS16.7.16真实LLVM导入分别审计。

原失败/输入/缓存及救砖资料保留，所有新源在正式仓库，非Git输入和镜像不提交。不永久删除；一个controller、仅BH905ND69G/合格COM/PON0x06、单download/boot、完整回程/健康30秒/旧worker退出后继续。
