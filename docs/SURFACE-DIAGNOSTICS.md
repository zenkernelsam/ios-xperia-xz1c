# v725 Surface 诊断与单次准入

v725 只在 SweetSurface.cpp 增加有界日志：request、allocate、prepare、kernel_map、acquire。成功最多8条、失败最多24条；分配、pin、map、complete/release 的原操作保持。成功额度可能被启动探针用完，不能用缺少成功日志推断失败。

基线完整修补仍为 xnu/lilac-current.patch；诊断增量为 xnu/lilac-surface-diagnostics.patch。reproducibility/surface-diag-v725.json 固定增量与前后源码SHA。实际构建内核为3a9a27b36139423fe8ef0ef1f03f8b47a22bf38e7d4557b3f537cf8099547dad，源码输入与重新构建的二进制SHA分别验收。

先按 BUILD.md 完成基线内核构建，再在Mac正式仓库执行：

```bash
python3 tools/build/update_surface_diagnostics.py
bash tools/build/build_diagnostics.sh
```

安装脚本只接受固定基线或已安装的诊断源，审计全部78源。构建脚本保留旧日志、对象、内核及顶层链接，移开 .cpo/.cpd、顶层 kernel 链接及其实际目标后才重编。它要求日志中出现 SweetSurface.cpp 且内核含诊断字符串，防止构建返回0但跳过链接。

Windows打包使用 assets/baselines 中已验证的v724 private/ready/panel镜像、对应内核与shim。package_trial.py 在内存中提取ADT/firmware/RAM库后，先逐字节重建原完整payload，再替换新内核、token、UTC及对应模式。新包仅保存在build：

```powershell
python -B tools/device/package_trial.py --mode private --kernel build/surface-diag-v725/kernel.v725.macho --output build/surface-diag-v725/private
```

owner、ready、ramcpu、gpu 同理使用独立输出目录；现有目录不得覆盖。当前测试依赖本机已验证基线，不能把它当作全新gl5首次启动已经复现。

run_trial.py 默认只验证包、前序实机证据和Mac实际78源，不发送设备。加 --live 才单次运行。前序证据要求：

| 模式 | 必需实机证据 |
| --- | --- |
| private | 无前序，同一新内核的只读VM/SMEM/F12与严格回程 |
| owner | 同内核private、真实RPM三ACK |
| ready | 同内核private/owner、真实RPM三ACK |
| ramcpu | 同内核private，交付已验证RAM GLES库 |
| gpu | 同内核private/owner/ready/ramcpu、真实RPM三ACK |

RPM证据使用 clock_evidence.py 读取原v676完整CPU试验日志，核对SHA、当前token、三个不同reply、正常回程与健康30秒；这是历史时钟证据，下一靴仍须成功完成正常GPU初始化。不能只提供一份声称PASS的JSON。v676当时尚无FINAL_DRAIN，新试验一律要求四标记。

owner_metadata.py 独立验证两次30寄存器/512根表元数据，重算根表FNV，检查TTBR/流绑定/空闲及顺序。gpu_pixels.py 从原v724验收代码整合，核对750×1624逐像素结果、caller别名与原VM解除后持有、小纹理刷新/不变seed、CA上下文隔离、160次relink/320draw及真正名字复用，并检查回程前最终scatter/caller全部撤销。GPU观察至少180秒，相机按OPERATIONS.md的唯一流程捕获。

每轮完整ACK/DRAIN/RESET/FINAL_DRAIN、健康30秒、原进程退出和独立核验后才下一轮。run_trial目录必须为不存在的TEMP子目录。真实面板、颜色、UI/FPS及声音/无线仍单独验收，不能由离屏数据推断完成。

## v726 离线草案

prepare_memref_diagnostics.py 在已安装v725诊断源的固定Apple IOMemoryDescriptor.cpp 上生成两处日志，默认保持当前编译源不变。请求地址/大小、返回大小、prot、task map页掩码和真实错误码均为失败元数据；不读用户缓冲区内容，成功路径不增加日志/页面操作。原文件去掉插入块后必须逐字节相同，实际插入块在独立C++程序中经ASan/UBSan验证每处24条上限及NULL map防护。

输出位于新的build/surface-diag-v726，固定增量和SHA另保存xnu/lilac-surface-memref-diagnostics.patch与reproducibility/surface-diag-v726.json。现有草案目录不会覆盖；后续使用 install_memref_diagnostics.py 安装已验证的草案：

```bash
python3 tools/build/install_memref_diagnostics.py
bash tools/build/build_diagnostics.sh 726
```

diagnostic_candidate.py 将固定基线、两个诊断增量与版本绑定，并审计79源和真实编译内核SHA。构建入口先检查jq/gum/xcodes/cmake/ninja，再归档失效对象与旧内核；先前失败已经移开顶层链接时，直接重编。v726打包使用package_trial.py --version 726，投递入口从包版本读取配置，拒绝缺失/错版本源码绑定及任何旧内核的前序证据；仍重走private→owner→ready、RAM CPU→GPU五阶段。所有上机命令单发，失败不重发。

当前mkpayload也接受不可变字节输入，提取blob只保存在进程内存。prove_payload在每次投递前及独立验收中，从固定基线ADT/firmware/RAM库和对应编译kernel重建整个Android镜像，要求与实际发送文件逐字节一致；镜像内核绑定覆盖payload内容。

surface_permission_evidence.py读取实际GPU完整返回包，核对原始log/image/kernel、当前token及四标记后提取memory-entry/init错误和对应Surface尺寸。v726观察到24条错误2、1127次400×400创建失败和20次AegirPoster sig6；此报告明确保留源区域实际保护权限及精确异常原因仍未知的边界。
