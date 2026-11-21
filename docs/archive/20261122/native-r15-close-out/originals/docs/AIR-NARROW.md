# Dynamic packed integer AIR conversion — 2026-11-15

此页保留该能力的原版本实现与实机证据；每个旧“当前”仅指当次日期。现行精确基线／候选／设备只见[STATUS](STATUS.md)，复现与单发分别见[BUILD](BUILD.md)／[OPERATIONS](OPERATIONS.md)。新附件native21尚未准入，旧阶段成功不扩展为完整UI。

原NanoUniverse/aegir_pass_vsh在动态short2顶点输入调用air.convert.f.f32.s.i16，开源前端仅允许常量8/16位conversion，因而实际返回nil pipeline。本机原库拆出的AIR 4928B SHA22b2b082c18cdf49a02bdbd4aa19702ae3e07b4cb4da5c7f8b9d795102ce796e，旧编译器和真实手机均复现同一错误。原库从使用者镜像只读挂载后提取，/dev/disk12正常detach，无镜像修改；不上传原系统库。

独立适配补丁gpu/metal/air/lilac-air-conversion.patch应用于materialized build副本，保留固定898998cf开源参考字节及声明。接受准确i8/i16 scalar/vector签名，按intrinsic的s/u在32位carrier先执行符号扩展/掩码，再数值转换；仍拒绝错suffix、错shape和未知sign。target narrow整数结果按位掩码，原半精度不冒充支持。

原aegir_pass_vsh现完成AIR→ESSL，并由主机Mesa transform feedback校验6个动态short2位置、2种UV/uniform字段偏移、48个结果分量与8保护分量（最大误差1e-6）。独立作者fixture narrow.air.ll同时覆盖8/16位signed/unsigned、scalar和2-lane vector，每invocation输出12个实际float bits，64invocation×2phase完整input/output/16384B allocation padding检查。主机98304B全零差异，ASan/UBSan通过；错误签名三组均拒绝。主机结果不等同FD540实机。

make_air_fixture.py --fixture narrow使用固定LLVM14生成作者MTLB；header/bitcode/source/compiler SHA独立记录。build_exception_observer.py --narrow-fixture在完整后台native组追加该probe，不缩减既有compute/render/constants/XR/Surface/MIPS。AIR frontend-r2新object SHA073bb66b26f13586d617faef26051a854949c6d341c82552e01311e2efbae54a，168个LLVM导入仍经20H392固定symbol核对。

CPU新包37.204秒完整回程，GPU整轮已独立验收通过：两phase98304B全结果/input/保护区零差异、423资源全released/unmapped、active/orphan/root0，38.469秒完整回程。实际源码/host manifest在reproducibility/air-narrow-*.json；整机UI仍待短登记后的原壁纸验收。

复现主机入口check_air_narrow.py：--air-cli/--graphics-cli给Mac当前修订编译器，--original-air给本机从NanoUniverse metallib拆出的aegir_pass_vsh，--gles-source给生成输出。入口实际再生成ESSL并逐字节匹配、运行WSL主机GLES、拒绝错签名。air_extract_cli.cpp沿固定OSS MTLB parser和每AIR SHA拆原库。原输入仅本地build；报告只记录hash/bytes，作者fixture及修补可开源复现。

下一完整native GPU通过后，以当前新库proof再短登记到原Aegir，逐步处理其下一真实AIR/texture/管线要求，完整壁纸时间日期/FPS/睡眠/声音/无线继续active。

原aegir_pass_vsh后续实机越过dynamic conversion；registry-short-r3进入half扩展错误，air-half-r1实机通过后r4原threshold管线编译成功。下一multi-target/XR混色，不把原单管线成功代替壁纸完整验收。
