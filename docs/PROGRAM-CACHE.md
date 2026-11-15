# FD540 程序二进制与后台预热

本轮沿固定 Mesa 24.3.4 的 GLES 后端实现程序缓存，参考已固定的 UniXNU AdrenoMetal shader priming 思路。原 AIR→ESSL 翻译及真实GPU绘制仍执行；缓存仅保存标准 glGetProgramBinary 输出，并在相同构建/上下文/原始source/config下通过 glProgramBinary 导入。

## 原卡点与一次失败

原 Aegir r7 的 vertex/fragment 编译约45–96ms成功，随后 glLinkProgram 未在原扩展check-in期限前返回。三条同SHA库的精确符号化栈均在 GLSL/NIR prelink/gl_nir_opts，未见IR3阶段。tools/build/symbolicate_link_crash.py 和 reproducibility/original-link-nir-stack-r1.json 保留证据；原Apple shader/binary只存本地。

cache-r1 GLES CPU/GPU及native包CPU通过，native GPU token57f8b2559214d870在sampler phase0出现 first_state=0/restored_state=0，原 REVISE 不改。其余原生GPU组仍通过。原raw452060B、SHA c2ded7ab2ab2a6a9d8b05298e2a20070d6947e9bbb1f943425cc11a2953cd16c，39.156秒完整0x06回程/健康/worker退出；独立失败记录见 reproducibility/v729-cache-r1-native-gpu-failed.json。

Mesa st_init_limits 仅在 get_disk_shader_cache 非空时给 NumProgramBinaryFormats=1；本构建固定 shader-cache=disabled，导致格式数为0。另一处 st_get_program_binary_driver_sha1 调用的禁用cache stub不写输出，原20B栈身份未初始化。完整Mesa修补修正这两处：无disk-cache时用平台实际构建/上下文身份；只对平台明确支持的FD540开放标准程序二进制。普通无hook后端行为保持；不伪造disk-cache指针。冷链接前设置 GL_PROGRAM_BINARY_RETRIEVABLE_HINT，格式/长度不可用均保留明确日志。

## 身份和记录

make-program-cache-identity.py 对全部当前frontend源、实际链接的Mesa归档和编译defines建立SHA256身份，再加入真实context API/version及screen名称。缓存key另绑定原VS/FS实际字节、长度与整个SweetRenderPipelineConfig。外层136B little-endian记录包含kind、format、长度、完整identity/key和payload SHA256；Mesa内层继续核对driver身份及CRC。

记录保存在 /private/var/tmp/sgpc-<identity前16字节hex>-<完整key>.req/.bin，单payload最多8MiB，按identity前缀限制64文件/32MiB。request在实际冷链接前持久化，保存原始source/config；bin仅由成功链接的真实Mesa程序导出。独占发布，不覆盖或删除已有文件。不同构建、source/config、损坏/截断记录拒绝。缓存不是私有程序二进制的分发渠道，原shader/设备bin不提交Git。

无有效cache时仍走原冷编译。prime只处理本identity缺bin的真实request，最多8项；已有bin保持原样。遇既有无效bin会拒绝并冷编译，当前primer不改写/删除该文件，应保留该失败后另处理，不能把存在文件当有效导入。

## 验收门槛

新库必须顺序完成精确GLES CPU/GPU、本native包CPU、全部13组backboard原生GPU，并保持四回程marker、健康30秒及原worker退出。新Program_cache组要求3个新建pipeline均实际 glProgramBinary restore，结合sampler147456B全部source/target/vertex/padding和1881pixels复核；half131072B、MRT1245184B及其它原组同轮保持。native_prerequisite.py从原raw重新验证，并把内核/GLES/Metal/source-map、token/image/raw和13组绑定进新proof。旧12组proof不能准入缓存库。

原Aegir短登记必须再使用同库新proof、本包CPU和单发GPU；不在扩展constructor重跑长probe，不延长原check-in期限。首次若仍冷链接超时，其request已保存真正runtime输入；原GPU失败完整回程后，另建完整backboard observer加 --program-cache-prime 做后台预热。后台成功写bin/完整回程后，重新准入原Aegir，分别看cache hit、原pipeline返回、真实draw/present及面板。写request、shader成功或离屏PASS都不能推断壁纸/完整UI已完成。

## 构建入口

Mac在正式仓库执行，每个新输出目录必须不存在：

    python3 tools/build/build_gles.py --mesa-build build/my-cache-mesa --frontend-output build/my-cache-base
    python3 tools/build/build_frontend_trial.py --mesa-build build/my-cache-mesa --output build/my-cache-frontend
    python3 tools/build/package_glram.py --frontend build/my-cache-frontend --output build/my-cache-gles-ram

完整native构建按 BUILD、METAL-BACKBOARD、METAL-SAMPLERS，build_metal_library.py绑定新GLES，build_exception_observer.py根据resource manifest自动开启Program_cache探针。--program-cache-prime只允许完整backboard sampler/MRT流程。原Aegirobserver用 --registration-only --register-device --pipeline-observe --native-proof。

package_trial.py --version729分别打包ramcpu/gpu；run_trial.py继续强制private/owner/ready、对应包CPU、RPM、精确GLES与native实机先验、Mac实际源和完整payload审计。具体本轮SHA/token以 STATUS 和 reproducibility 的原结果为准；本轮目标缓存/全部13组硬件证明已完成：GLES0a2c7e69、Metal87b91b14，token6dfc08178ea3da43、raw71901fab…、38.984秒完整回程。3个fresh pipeline恢复、147456B/1881pixels零差异，half/MRT和其它原组保持。原Aegir壁纸/完整UI仍需单独验收。

固定kernel v729/774b5efd及81源，allocator/SCM/PON/RPC/VM和SweetGPU冻结。仅BH905ND69G、合格COM>=10、PON0x06、单download/boot、失败不重发、单controller。gl5/userdata/SD/TA/XFL和救砖输入受保护；历史/失败/输出保留，不永久删除。

本包CPU还验证了真实部分CDC回放：第二个READY只重复第一段3081B的完整行，随后明确replay marker→LIVE，库只在LIVE后读一次。旧后处理错误和原controller PASS/完整回程分别保留；新ramcpu_prefix_evidence.py核验原字节相等、唯一banner/current token、marker边界、精确库长度/FNV/只读状态及唯一读，10项测试含损坏/重复/无marker/多banner拒绝。reproducibility/ramcpu-partial-replay-r1.json记录独立复验，没有改原raw/results/stdout，也没有再次投递该靴。
