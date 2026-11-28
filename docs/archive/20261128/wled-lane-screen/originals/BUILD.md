# 构建与复现

先读[STATUS](STATUS.md)确认已验基线和当前候选，[OPERATIONS](OPERATIONS.md)负责手机准入与单发。唯一正式源码D:/Desktop/Git/ios-xperia-xz1c，Mac为~/Desktop/ios-xperia-xz1c；Patch只作参考。输出只在新的build子目录，不能覆盖原库、manifest、失败包或日志。Mac `~/xnu-autobuild.pause` 保持，不恢复旧常驻构建器。

## 固定源与平台

| 输入 | 固定身份／要求 |
| --- | --- |
| Apple OSS XNU | xnu-8796.141.3／`f625db25f47631424dc62ac1c3414b6e530f7e8a` |
| darwin-xnu-build | base `663906528d1cd5ea8fde7de6f16de73db9511549`＋tools/build/dxb-local.patch |
| Mesa | 24.3.4／`769e51468b49b2a42f0a0eaf71cf9eed5ff4e5de`＋gpu/mesa-lilac.patch／IR3独立增量 |
| Mac | Xcode／iphoneos SDK、KDK13.5／22G74、jq／gum／xcodes／cmake／ninja |
| WSL | Ubuntu／Ubuntu-22.04、LLVM14、aarch64-linux-gnu工具链；host Mesa读回使用UBSan |
| 本地原输入 | iPhoneX16.7.16／20H392 IPSW、Sony SO-02K47.1.F.1.105、DTB、gl5／校准／备份，使用者提供，不进Git |

全部源、增量、实际header／archive哈希由reproducibility/source-lock.json、current-device-tools.json及各build manifest绑定。SDK／构建时间可能改变签名／UUID／产物SHA；源码复现不等于新二进制实机通过，更不等于重建全新gl5首次启动已验。原root配方见[ROOT-GL5](ROOT-GL5.md)。

## 内核和shim

现行设备内核是v730/f258dee9／20665112B、83实际编译源；内核GPU底层／allocator／SCM／PON／RPC／VM冻结。日常用户态迭代复用它，不重编内核。固定源重建入口：

```sh
export KDKROOT="$HOME/KDKs/KDK_13.5_22G74.kdk"
bash tools/build/build_kernel.sh
python3 -B tools/build/verify_diagnostic_reconstruction.py --version 730
```

完整lilac-current.patch先应用，再顺序应用v725..v730诊断增量。后三版分别保持只读Surface／64位区域、console snapshot及真实scanout；准确路径、行尾和83份输入见tools/diagnostic_candidate.py及各surface-diag-v*.json。上述reconstruction创建新的TEMP checkout，验证后保留输出，不改变当前dxb。

需要从现存准确树重建时，按专题先host验对应增量，再安装、强制失效相关.cpo/.cpd和顶层链接，使用build_diagnostic_kernel.py。不能只凭build.rc0确认新代码已链接；实际kernel和83份源码都要audit。build/dxb/build/xnu.sym/kernel.development.sweet是原非strip来源，勿混作xnu.obj的另一份产物。旧版本手册见[SURFACE-DIAGNOSTICS](SURFACE-DIAGNOSTICS.md)和[SURFACE-READONLY](SURFACE-READONLY.md)，v730见[SCANOUT-SCREENSHOT](SCANOUT-SCREENSHOT.md)。

WSL shim仅编译，不发送：

```sh
python3 -B tools/build/build_shim.py --dtb /path/to/verified/dtb.img
```

Image头_end是链接时尺寸；运行时指针不得生成绝对重定位。使用本仓库工具，新输出build/shim-日期；原已验shim身份由每个trial artifact绑定。

## GLES前端

从零准备固定Mesa和IR3归档：

```sh
python3 -B tools/build/build_gles.py --mesa-build build/my-mesa --frontend-output build/my-gles/frontend --ir3-cache
```

既有Mac已审计build/mesa-ir3-cache-r1时，只重编当前用户态前端：

```sh
python3 -B tools/build/build_frontend_trial.py --mesa-build build/mesa-ir3-cache-r1 --output build/my-gles/frontend
python3 -B tools/build/package_glram.py --frontend build/my-gles/frontend --output build/my-gles/ram
```

工具验证所有当前前端源和固定Mesa依赖，manifest保存实际source／archive／build-ID。已有Mesa归档不因用户态调整重编。32MiB HFS RAM包保持只读、m／g硬链接、签名／SHA／FNV；GLES包须先自身CPU再完整GLES9。CA矩阵安装必须准确20H392 UUID、两原空槽、原表达式与代码锚点全匹配，见[CA-MATRIX](CA-MATRIX.md)。

最新用户态包含Depth32／Stencil8、Memoryless、MSAA和presentation。FDL5附件work用固定fd5_tile_mode的sysmem TILE5_3；原linear offset／stride、真实样本、work IOVA、GPU copy fence和释放全部绑定manifest。depth MSAA用实际sample shader bitcast交接，stencil候选用GPU逐位Load／实际stencil纹理Store。完整native21／PNG6原字节门槛保持，当前实机结论只看STATUS。

HEAD是当前候选源码。稳定基线按reproducibility/v730-ca-matrix-r1-run-index.json、对应manifest的source_files及准确Git版本复现；SDK／签名／UUID变化仍需新产物自己的准入。用户提供IPSW／固件输入保持本地，不能把HEAD新库视为已验基线。

附件GL语义主机检查：

```powershell
python -B tools/build/check_stencil_sample_copy_gles.py --output build/my-stencil-sample-host
python -B tools/build/check_depth_multisample_copy_gles.py --output build/my-depth-sample-host
python -B tools/build/check_depth_stencil_gles.py --sanitizers undefined --output build/my-depth-stencil-host
```

这些检查直接调用生产helper并保留原字节／外围guard。llvmpipe可能把请求2×分配为4×，以manifest实际样本数为准；host不证明FD540导入器、原native或UI。帧缓冲清屏及BLIT／BOX日志必须在真机完整核对；单条日志的实际长度上限需从原源码确认，缺失／截断不补造。

## AIR、Metal和完整作者probe

固定AIR reader及许可见gpu/metal/air/reference、reproducibility/air-reference.json。materialize_air_sources.py只在build副本顺序应用narrow／half／static-half独立修补，原上游字节保留。libLLVM的实际170→168导入适配按20H392原导出审计，不能用Mac函数存在冒充iOS目标支持。

```sh
python3 -B tools/build/build_air_frontend.py --llvm-headers deps/llvm-c-14 --target-symbols /path/to/verified/20H392-libLLVM-exports.txt --output build/my-air/frontend
python3 -B tools/build/check_metal_compute.py --llvm-headers deps/llvm-c-14 --output build/my-metal/host-native
python3 -B tools/build/build_metal_library.py --ram-package build/my-gles/ram --air-package build/my-air/frontend --output build/my-metal/library
```

旧 AIR 目标文件已经回收、但完整原构建记录仍保留时，可按 [本次重建核验](../reproducibility/poster-cache-budget-air-reconstruction-20261124.json) 从正式源码重新 materialize，再以同 LLVM14 header 和 ARM64 参数编译。只有新 object SHA 与 [原完整 AIR manifest](../reproducibility/air-static-half-object-reference-20261124.json) 完全相同，且 `nm -u` 的 168 个 LLVM 导入逐项一致，才可复用原导入审计；该路径不准入新导入或不同目标系统。20H392 原 exports 文件未在现存目录找到，重新审计不同 object 前仍需重新提取并核对它。

同一完整参考已进入 Git，可用正式工具复现已回收的 object（仅准入固定原 manifest／原目标／相同 object）：

```sh
python3 -B tools/build/rebuild_verified_air_object.py \
  --reference-manifest reproducibility/air-static-half-object-reference-20261124.json \
  --llvm-headers deps/llvm-c-14 --output build/my-air/identical-object
```

[工具实编译记录](../reproducibility/cache-budget-verified-air-tool-20261124.json)验证正式 materialize 后 object SHA `10df4bf0…`、168 原导入全部一致。任何 source／object／manifest 差异均拒绝，改动后的库仍须自己的完整 GPU 验收。

v731原壁纸的同靴150秒扫描包仍绑定精确新RAM／native21／PNG6，完整源码和payload复算后单发。command超过607 B时，sf2x默认0的省略只准入精确v731内核与和v730相同的SweetFramebuffer.cpp／parser SHA；numeric_one及helper拟合默认也逐项验证。不能把换token视为新实验，新的扫描取证改变实际串口命令和观察范围。

作者MTLB／AIR fixture均由正式源和LLVM14生成，不提交原Apple shader。basic使用`--fixture basic`，render／constants／narrow／half／mrt／sampler／static-half／multisample使用对应参数；具体choices以make_air_fixture.py为准：

```powershell
wsl.exe -d Ubuntu-22.04 -- python3 -B /mnt/d/Desktop/Git/ios-xperia-xz1c/tools/build/make_air_fixture.py --llvm-as /usr/bin/llvm-as-14 --fixture render --output /mnt/d/Desktop/Git/ios-xperia-xz1c/build/my-fixture/render
```

完整backboard observer保留所有前序16组并加depth32、stencil8、memoryless、multisample、presentation及六组PNG：

```sh
python3 -B tools/build/build_exception_observer.py --output build/my-metal/reference-observer \
  --metal-resources build/my-metal/library --target-process backboardd --gles-ram-package build/my-gles/ram \
  --air-fixture build/my-fixture/basic --render-fixture build/my-fixture/render --constants-fixture build/my-fixture/constants \
  --xr-probe --surface-probe --mip-probe --narrow-fixture build/my-fixture/narrow --half-fixture build/my-fixture/half \
  --mrt-fixture build/my-fixture/mrt --sampler-fixture build/my-fixture/sampler --static-half-fixture build/my-fixture/static-half \
  --clear-cache-probe --ir3-cache-probe --depth32-probe --stencil8-probe --memoryless-probe \
  --multisample-fixture build/my-fixture/multisample --presentation-probe --cif10-png-reference
python3 -B tools/build/package_glram.py --presigned --frontend build/my-gles/ram \
  --observer build/my-metal/reference-observer --metal-resources build/my-metal/library --output build/my-metal/reference-ram
```

首次pull各路径使用自己生成的fixture。tiled候选仍要求真实GPU load／store复制、完整原页／guard、fence／回收；颜色／D32／S8等未实现组合必须明确拒绝，不CPU填预期。host Mesa不是FD540 importer，host drawable fixture不是手机原CAMetalDrawable。全部21组通过后才生成native_prerequisite和PNG proof，再构建[原poster模式observer](METAL-REQUEST-OBSERVATION.md)，短登记不得构造函数内重复长probe或使用旧16／20组proof。

## 显示比例和现行验证

现行native几何包已实机核对720×1280像素／360×640点／scale2、MG／UIScreen和真实扫描尺寸；完整模型、标签祖先及两处17.5点间距已验，完整safe-area、真实应用与光学继续验收，见[DISPLAY-GEOMETRY](DISPLAY-GEOMETRY.md)。make_render_adt.py的xz1c-native-2x只改四个CFData属性，原node、内存映射和其它输入保持字节一致。package_trial.py --display-profile xz1c-native-2x绑定native1／sf2x0和整个payload；旧750×1624配置按原manifest保留。

原生状态栏helper必须单独构建并绑定实际source／signed library，不能把临时修改的库放进RAM：

```sh
python3 -B tools/build/build_geocompat_trial.py --output build/my-native/helper
python3 -B tools/build/package_glram.py --presigned --frontend build/my-gles/ram \
  --geocompat-package build/my-native/helper --output build/my-native/ram
```

builder执行真实host四参数ObjC转发／ASan／UBSan及ARM64签名；新helper包必须自身CPU后再GLES9／原UI。run_trial在USB前核对Mac实际helper源码及签名SHA。精确native配置的nil／unsupported provider使用已有LegacyPhone，其它有效返回保持；safe-area只读，详见几何专题。新包仍需要自己的CPU，不能沿用旧helper RAM证明。

Windows源码核验和当前相关单元验证：

```powershell
python -B tools/validate_current.py
python -B -m unittest discover -s tools/build -p test_render_adt.py
python -B -m unittest discover -s tools/device -p test_attachment_snapshot_evidence.py
```

手机实际入口是package_trial.py／run_trial.py；缺精确private／owner／ready／RAMCPU／GLES／native／PNG先验即拒绝，步骤见[OPERATIONS](OPERATIONS.md)。新包还绑定serial capture revision1／worker SHA，DTR低电平打开后建raw、一次单流读取，无replay；原失败核验保持。run_trial默认offline，--live才单发，controller及原字节后处理均通过才准入。

## 原壁纸返回状态观察包

在同kernel／GLES／Metal／source-map的完整native21与PNG6原字节准入后，构建只观察原请求的包。下面的my-*须替换为自己已验的对应产物；short registration不重复长probe，quad-route-observe同时编译有界PosterStateObserve：

```sh
python3 -B tools/build/build_exception_observer.py --output build/my-poster/observer \
  --metal-resources build/my-metal/library --gles-ram-package build/my-gles/ram \
  --registration-only --register-device --native-proof build/my-native21/proof.json \
  --pipeline-observe --postdraw-observe --snapshot-observe --request-observe --quad-route-observe \
  --png-filter-encoder --png-proof build/my-png/proof.json
python3 -B tools/build/package_glram.py --presigned --frontend build/my-gles/ram \
  --observer build/my-poster/observer --metal-resources build/my-metal/library \
  --geocompat-package build/my-native/helper --output build/my-poster/ram
```

builder执行真实ObjC转发、原位值缓存／errno的ASan／UBSan和ARM64签名；manifest绑定全部实际源、库及先验。新RAM先自身CPU，完整回程后才准入新的GPU包。R22原失败及本轮捕获范围见[壁纸专题](METAL-REQUEST-OBSERVATION.md)，不用旧失败包继续投递。

## 原快照与绘制失败复现

原快照来源／bounds回调／PNG／命令队列观察均是独立的observer flag：`--poster-snapshot-gate`、`--aegir-snapshot-cache-scope`、`--original-snapshot-png-export`、`--loaded-snapshot-png-export`、`--snapshot-render-queue`。loaded与original PNG export互斥，loaded要求gate；queue要求short registration／snapshot／namespace。builder对真实ObjC ABI、原返回／errno／throw、有界日志及host原decoder验证后编译签名，RAM manifest绑定实际源与完整先验。PNG transport只无损zlib＋48B分块；缺块原失败不补发。

`--snapshot-readback --original-cg-input --original-queue-retirement`分别观察原getBytes已返回字节、PNG前原CIF10 CGImage provider与同一Completed队列fence／operations。readback要求queue，CG input要求readback，retirement要求queue；namespace绑定实际观察源，旧黑PNG保持。每源每进程一次，有界像素／provider，不增加draw／转换／回写。新RAM仍自身CPU，不重发旧失败包。

## A5xx ordered XR同步候选

当前源码以每图元XR舍入和严格同步为前提，只有精确A5xx／单样本sysmem／RGBA32F原图像契约采用批次内timestamp wait与UCHE invalidate，其它保持GL MemoryBarrier。host工具直接编译固定Mesa emitter，验证21个指令字、monotonic timestamp和溢出拒绝；host不替代GPU。Mac用现存准确Mesa归档构建，源／archive SHA与新program-cache identity全部绑定：

最新候选同时拒绝异步GLThread，原GL barrier负责已排队的前端命令。原context blit scratch64与固定Mesa使用的offset0独立；未知image access／layer／format、非sysmem／gen5、深度或多样本保持原同步路径。r4首次同步先用CP_MEM_WRITE／WAIT_MEM_WRITES初始化scratch，再等待非零单调timestamp，避免新context复用BO的旧值。第一次barrier26指令字、以后21字。r2／r3／r4 signed产物各自绑定源，旧host／device proof不能提升新产物。

当前barrier revision4另从实际linked fragment program读取ImageUnits数组，把GL binding映射为dense Gallium槽位。Mesa的driver image0可能来自binding8，不能按GL binding直接索引si。host工具直接include准确锁定的st_atom_image.c，核对255种mask与两种dense顺序，并编译旧header复现错误fallback；连续1404次指令和ring扩容检查保留。其它stage image、atomic或写SSBO保持原barrier。r5的准确signed库和四份未投递包由STATUS索引，旧r4保持历史。

```sh
python3 -B tools/build/check_ordered_image_barrier.py --mesa-build build/mesa-ir3-cache-r1 --output build/my-ordered-XR/host
python3 -B tools/build/build_frontend_trial.py --mesa-build build/mesa-ir3-cache-r1 --output build/my-ordered-XR/frontend
python3 -B tools/build/package_glram.py --frontend build/my-ordered-XR/frontend --output build/my-ordered-XR/signed-gles
python3 -B tools/build/package_glram.py --presigned --frontend build/my-ordered-XR/signed-gles --geocompat-package build/my-native/helper --output build/my-ordered-XR/gles-ram
```

builder增加固定generated compiler／NIR和Freedreno IR3 header include，不更改Mesa archive。新GLES自身CPU→完整GLES9，新的native RAM自身CPU→完整native21／PNG6。独立MRT除所有原页／mask／低位／guard／fence，还要求12条实际优化barrier记录，其中四种XR各四重叠图元／instances／HDR全部原字节正确。r1 CPU／GLES9不能准入r2，旧proof不能准入新GLES。精确候选与原guard失败见STATUS。

新Metal反射绑定候选复用精确GLES及已核验AIR object；新Metal自身source-map变化，旧native／PNG proof不准入。构建完整作者observer时保留全部21组／PNG6，额外probe在真实encoder pipeline切换后保留fragment0与unused slot7，核对原48B position buffer、target／guard、fence和最终退休：

```sh
python3 -B tools/build/check_metal_compute.py --llvm-headers deps/llvm-c-14 --output build/my-buffer-selection/host
python3 -B tools/build/build_metal_library.py --ram-package build/my-gles/ram --air-package build/my-air/identical-object --output build/my-buffer-selection/library
```

完整observer沿用前述native21参数、`--cif10-png-reference --program-cache-inventory`；只读inventory不启用空primer。新RAM自身CPU、同GLES完整9组先验、完整native21／PNG6通过后，再构建同库／新proof的short原Aegir队列观察。这里的host和新signed产物不自动获得实机准入。固定原调用点可用`inspect_aegir_render_boundary.py --sparse <gl5> --symbols <A2S> --output <build下新JSON>`复验，不展开16GB镜像，不发布原字节。

## 多写入方日志票号候选

```powershell
python -B tools/build/check_log_tickets.py --output build/my-log-tickets-host
```

源`SweetLogTickets.h`预留唯一字节ticket，atomic32 cell同时发布generation和byte；reader只接受匹配的ticket。Producer保留128次原子预留上限，发布过程不持互斥锁；pending前缀返回已就绪的原字节，旧writer被整圈超越后不覆盖新数据。Tag horizon远大于每靴128MiB捕获上限。原型为caller-retained静态存储，尚需新kernel完整准入。

Mac现行v731源码准备与隔离链接：

```sh
python3 -B tools/build/prepare_log_tickets.py --tree build/dxb/xnu --output build/my-log-tickets/overlay
python3 -B tools/build/compile_log_tickets.py --candidate build/my-log-tickets
python3 -B tools/build/link_log_tickets.py --candidate build/my-log-tickets --output build/my-log-tickets/link
```

当前已审核输出在build/log-tickets-r1-20261127：基线v731重链接除UUID外一致、仅logger object替换，原树不改。来源、实际ARM64／host检查及完整链接object SHA由专题报告绑定。新732记录必须包含自己的source binding和180秒一次当前LIVE status查询；查询走现有唯一serial worker，不改内核命令或回程。private仍要求完整scanout，四回程marker／健康30秒／worker退出均保持。

## 当前Earth link栈解析

symbolicate_link_crash.py新增可选`--token`裁定本LIVE→ACK及`--retain-unresolved`保留无唯一锚点样本。当前r5函数已拆出_render_create_samples，工具从实际符号选择该共同创建函数的glLinkProgram返回点；旧库仍按原函数锚点。库SHA、唯一16K对齐base和精确return offset均必须成立；无锚点不能猜位置，原raw不重写。原输入shader／shared-cache范围只保留本地，Git只放诊断元数据及源。报告由METAL-REQUEST-OBSERVATION索引。

## 当前console logger并发复验

```powershell
python -B tools/build/check_log_snapshot.py --output build/my-console-check
```

工具从锁定base patch及v729增量复建两版logger，准确核对v729源SHA；控制读取／写入暂停的hook仅属host测试。当前版本的writer争用丢记录与旧base读锁争用分别重现，外部串行writer原型另核对10000条并发字节和cursor，不能宣称多writer设备修复。Mac可加`--sanitizer thread`；WSL TSan若出现unexpected memory mapping即保留运行时失败，不作为通过。当前ARM64头编译与v731对象代码段核验见专题报告，未链接新内核。

## 作者PNG导出日志候选

`CIF10ExportWait.h`将每份48 B原导出后的等待改为15ms、按nanosleep剩余时间处理EINTR且最多64次。错误保留errno并使reference失败；诊断写入错误另计数，新background准入必须有零错误summary。原PNG decoder不放宽。构建完整作者observer沿用全部21组／PNG6参数；`build_exception_observer.py`同时执行真实ImageIO fixture、等待故障ASan／UBSan、日志错误与原exception ABI检查。

从已有presigned RAM生成新包时，必须显式传入已验`--geocompat-package`，使复制的helper与manifest绑定一致。Mac和Windows的fixture路径不保证相同，应按header／metallib／当前源SHA全部一致选择，不能只按header挑旧CRLF副本。精确新候选和实机顺序由STATUS维护，失败包不重发。

## 音频协议离线复现

ADSP实际启动前置和诊断生命周期可独立核对：

```powershell
python -B tools/build/inspect_lilac_adsp_boot.py --reference D:/Desktop/Git/Patch/Xperia/research/lilac-kernel-8.4 --output build/my-adsp-boot-audit
```

工具要求Sony固定commit `33dc678e7692f04ecc1eeddeb1cca8a809bc1727`、当前formal patch与source-lock准确SHA，直接读取上游Git blob，不依赖参考工作树中的Windows symlink展开副本。新输出只含manifest／源码片段及SHA；不编译／修改内核、映射寄存器、调用RPM／PIL或连接设备。stage4～7成功诊断最终shutdown，不能作为持续运行的DSP准入；详见LILAC-PERIPHERALS。

`check_lilac_afe_slimbus.py --output build/新的-afe-host`从正式patch／source-lock materialize原APR头，再编译运行真实ASan／UBSan包字节与状态用例；Windows用WSL Ubuntu-22.04，Mac直接host clang。它不连接传输或启动DSP。实际codec channel map、参数schema与物理声音另行验收，见[Lilac外设](LILAC-PERIPHERALS.md)。

统一会话工具在同一套锁定APR／GLINK／回调ABI头下编译PCM负载、队列、APR、映射、流、AFE、session、transport、FIFO、SMEM链路、保留运行时及C接口十二组用例，不依赖历史build展开树。C11 caller、C++ peer测试与实际API实现分成三个translation unit再链接。新输出目录须不存在。Windows调用WSL Ubuntu-22.04；Mac可同时用实际iphoneos SDK编译会话、GLINK适配、SMEM链路、运行时、实际C接口与C11 caller的六个ARM64对象：

```powershell
python -B tools/build/check_lilac_pcm_session.py --output build/my-pcm-session
```

```sh
python3 -B tools/build/check_lilac_pcm_session.py --arm64-object --output build/my-pcm-session-arm64
```

manifest revision5绑定全部40份真实support／test／builder／锁定header源、共享模拟peer fixture、命令／日志SHA及可选ARM64对象。实际C接口的ARM64未定义符号只能为正常stack guard／内存原语，C11 caller只能导入准确lilac_pcm_runtime_*符号；没有kernel链接或设备启动。BasebandGLINK的混合CRLF／LF按source-lock中准确before／after SHA和三个LF行位置复建，字符不变；首次全CRLF假设失败的输出保留。会话要求独立确认本epoch的DSP／codec就绪，EOS普通回复与RENDERED_EOS均到达后才关流。

`LilacPcmPlaybackTransport`只拥有一份104 B待发APR包和四个已发布WRITE时间记录，实际Link仍由调用方持有。admit之后只让advance轮询／send／close该Link；Busy保留同一token和字节，成功发布才开始回复deadline；Accepted及typed回复等待不延长deadline。queue_write只预留，不等于DSP收到。失去readiness／epoch、超时、时钟倒退或发布状态不明都会uncertain，物理页保留。session停好之后另确认原GLINK的双向关闭。

具体`LilacAprSmemFifo`沿用正式patch的sweet_adsp_apr_ops回调，先只读验证布局与四个cursor，并由调用方确认独占／uncached／DSP就绪；不会映射内存、清cursor或初始化硬件。16KiB双环和8 B对齐保持，TX保留16 B，发送数据→fence→TX head→fence→notify。RX先复制完整有界prefix，所有read成功才交给Link；consume须与当前观察的完整frame大小／tail一致，再RX tail→notify。逐字故障、cursor／通知失败使FIFO不确定，不能重发。工具直接测试全部2048环位置和完整PCM／GLINK／FIFO链路，ARM64具体SMEM对象不再留下未实现FIFO函数导入。

调用方仍需连接真实read32／write32／fence／notify到已准入SMEM映射与mailbox，串行调用、提供monotonic时间、管理PCM页面／缓存并确认DSP／codec。该工具不连接硬件、不分配／释放物理页、不加入内核，主机／ARM64结果不准入实际声音。

`LilacPcmPlaybackRuntime`拥有上述FIFO、Link与transport及四份PCM跨度。构造不做硬件IO；start只在准确readiness和mapping契约通过后开始一次Link协商，advance有独立协商deadline。保留对象60,352 B须由owner放在内核栈外，所有调用和回调串行、非重入。queue_pcm只在准确Run回复后复制双声道S16帧字节，检查DSP32 B对齐和owner提供的cache-line跨度，release回调完成后才生成可发布WRITE。只有匹配的已发布WRITE_DONE加acquire回调成功才交回CPU；失败／unknown跨度持续保留。

实际运行时绑定当前clock hook；transport在接收、poll结束、send／close结束以及cache回调后验证原deadline。旧两参数transport用传入时间进行确定性离线回放，不作为阻塞真实IO的时钟保证。当前十一组回归涵盖回调内部时间推进、迟到普通回复／CloseAck、PCM输出byte guard、缓存释放／收回失败、外来token和重叠源数据，准确证据由LILAC-PERIPHERALS索引。实际owner仍需验证readiness／cache回调的硬件语义和PCM物理映射，不能由host callback成功推导声音。

## 产物和历史手册

本周Mac重复HFS容器／raw、符号表、旧展开库、主机产物、临时检出及传输tar已进入原生垃圾桶；留存压缩blob／manifest／signed库可按SHA重建原RAM。当前kernel／deps／dxb、gl4／gl5／IPSW和失败证据保持。[LAYOUT](LAYOUT.md)给恢复方法，[ORGANIZATION](ORGANIZATION.md)给实际清单。回收不清空，不把移入垃圾桶称为释放全部空间。

旧BUILD原全文、编号版本复现和失败历史在[本周归档](archive/20261121/weekly-integration/original-document-manifest.json)及各能力专题；原source-lock／build-map／raw不重写。当前状态只写STATUS，不再在构建手册重复每轮工作日志。

## 原／拟合完整模型对照

新helper revision3／4读取原Root、Dock及Folder.lists／uniqueIdentifier，main queue在constructor后120秒捕获一次。revision4另发完整TREE与label最近祖先，由独立decoder复算；builder还把真实C++ host捕获交给同一decoder，核对嵌套wrapper、同坐标外部标签及opaque子类。builder执行真实ObjC ABI、factory／cache／errno／opaque返回、完整model graph和GEO_GRID控制的ASan／UBSan，再编译ARM64、签名并绑定实际五个guest构建输入和三个host decoder源。guest库和RAM manifest不可在实机后改写为已验；实际准入另存原run报告。

使用同一个精确新RAM准备自身CPU、original和fitted三包；各输出目录必须不存在。下面只打包，投递仍由OPERATIONS规定的run_trial先验控制：

```powershell
python -B tools/device/package_trial.py --mode ramcpu --version 730 --kernel build/surface-diag-v730/kernel.v730.macho --ram-package build/my-native/ram --display-profile xz1c-native-2x --output build/my-native/cpu
python -B tools/device/package_trial.py --mode gpu --version 730 --kernel build/surface-diag-v730/kernel.v730.macho --ram-package build/my-native/ram --display-profile xz1c-native-2x --native-grid-mode original --screen-evidence --screen-after 150 --swipe-once --output build/my-native/original
python -B tools/device/package_trial.py --mode gpu --version 730 --kernel build/surface-diag-v730/kernel.v730.macho --ram-package build/my-native/ram --display-profile xz1c-native-2x --native-grid-mode fitted --screen-evidence --screen-after 150 --swipe-once --output build/my-native/fitted
```

original绑定GEO_GRID0，fitted绑定1；整个SpringBoard env、control源／值、plan和完整payload独立复算。R21三包命令区均607B，R22 CPU607B／GPU604B，具体以新artifact为准；限定本v730实际consumer的sf2x默认0省略保持原语义，shim参数不压缩。先新RAM自身CPU，再合格的原完整模型baseline，完整回程后才投递fitted。

若壁纸观察包保留全部进程日志后超长，打包器只对已核验的 revision 4 helper 使用拟合默认值，记录 helper 二进制和源码 SHA，并在独立复验中要求本靴 SpringBoard 实际报告 `value=1`。原始六行模式仍须显式传 `GEO_GRID=0`。复现 R26 主机打包（输出目录须新建）：

```powershell
python -B tools/device/package_trial.py --mode gpu --version 730 --kernel build/surface-diag-v730/kernel.v730.macho --ram-package build/poster-cache-r26-20261123/ram --display-profile xz1c-native-2x --screen-evidence --screen-after 150 --output build/新的R26目录
```

独立后处理入口为geometry_layout_evidence.py --directory <原run> --output <新文件>及native_grid_profile_evidence.py --directory <fitted原run> --baseline <original原run> --output <新文件>。完整应用multiset、Dock顺序及Folder内容由原model比较，Root允许原系统分页重排。原标签归属不明仍REVISE；下一只读capture和新helper须再走host／新RAM自身CPU及新的原／拟合对照。详见[DISPLAY-GEOMETRY](DISPLAY-GEOMETRY.md)。

## 原Earth离线链接复现

`replay_render_request_cli.cpp`对本地完整原SGPB request重用现行HalfESSL／fold及XRBlend。先独立核对payload SHA／context-source-config key，再将输出长度／FNV与本LIVE→ACK日志比较；原request／shader只存本地，不能用代表function constants冒充实际原输入。`air_graphics_constants_cli.cpp`接受明确index/type/十六进制紧凑字节表，不提供默认值；代表输入必须单列。

NIR单块原型仅在host override objects中核对，不应用生产Mesa或重编设备GLES：

```sh
python3 -B tools/build/check_nir_straight_inline.py --mesa-source build/my-native-mesa-src --mesa-build build/my-native-mesa --output build/my-nir-inline-check
```

输入须为固定24.3.4的native host build及compile_commands，准确原NIR源SHA被锁定；工具独立实编译debug validator、原／原型函数与作者测试，核对1／64／4096次图的完整序列化字节，原source／archives不变。[本轮报告](../reproducibility/earth-link-host-offline-20261127.json)保留整进程失败和链接阶段计量；局部变快不准入设备。下一轮现有缓存准入仍按OPERATIONS及实际完整native21／PNG6／inventory，不采用该小收益原型。

## 函数局部SSA编译器候选

免费Mesa24.3.4的`gpu/nir-callee-local-ssa.patch`只涉及GLSL／NIR及一个内部SSA声明。原／改host分别用同一准确原Earth重放字节；host tool可加--callee／--validate做内联局部优化及实际debug结构／支配关系检查。新archive overlay只是frontend输入，不能对它运行ninja；所有未修改archive与生成header继续指向原基线，两个compiler archive是独立副本。

```sh
python3 -B tools/build/build_callee_ssa_overlay.py --mesa-build build/mesa-ir3-cache-r1 --output build/my-callee-ssa
python3 -B tools/build/build_frontend_trial.py --mesa-build build/my-callee-ssa/mesa-overlay --output build/my-callee-frontend
```

builder锁定四个源before／after、实际SDK命令、三个ARM64 object、原archive成员和顺序；原source／archive SHA不变。frontend再次核对patch／builder／源binding／objects及24份链接输入。新程序cache身份取实际archive／frontend源码；旧cache不删改。新guest仍须自身CPU→完整GLES9→新Metal／source-map对应native21及PNG6→原UI，不用host的约53%链接阶段改善提升基线。作者局部变量／循环／inout／全局SSBO与half模式输出只做host语义证明；原context destroy失败另列。当前SHA及设备状态只看STATUS。

新compiler overlay的实际源／object／archive校验由`tools/mesa_compiler_overlay.py`统一执行；run_trial再次读取Mac materialized源和ARM64对象、原／当前archive及manifest，不能只验证RAM里声明的SHA。结果写入payload-audit，SHA或CPU类型差异在任何设备操作前拒绝。host验证用`python -B -m unittest discover -s tools/build -p test_mesa_compiler_overlay.py`，fixture只在TEMP，完成后走原生回收站。

当前CA probe另绑定observation revision2：原安装可在constructor完成，probe在六阶段前只读复验准确image／代码锚点／两个实际pointer和case。新decoder必须在本LIVE→ACK看到一次VERIFY；不会把旧revision1安装范围拒绝重新分类。主机复现用`check_ca_matrix.py --output build/新的CA核对目录`和`unittest discover -s tools/device -p test_ca_matrix_evidence.py`；新frontend重新签名和打包后仍须自身CPU／GLES9。原SSA archive overlay继续只作链接输入，不能ninja重建。

新GLES组合的完整native RAM重用当前源码生成的全部21组与PNG6；标准库与Metal仍按实际签名／source-map绑定。新source-scope首次运行只读inventory，不预设存在需要prime的请求。own RAM CPU和该GLES自身9组都需原raw准入；随后新native／PNG原proof才能用于原UI short observer。四个显式单发request已保存在本轮build/planned-requests，未启动；实际SHA和token只由STATUS维护。

非零backlight duty后端host复现用`check_lilac_backlight_duty_backend.py --reference <固定Sony Git> --output <新build目录>`，核对固定DTB／Git blob、65535请求值、epoch／重入／写失败。`link_lilac_backlight_service.py`现复制并核对七份真实support源，新增duty factory只在独立service对象里；旧kernel原对象保持，链接验证副本不可当已准入设备kernel。

非零背光host checker现在另要求epoch_read_positions38／write_positions6／wait_position1的成功总结，覆盖IO内部状态变化，不只事务首尾。ARM64链接仍复用原v731 recipe、源和object库存；source／object／proof绑定新支持头，验证副本不提升为可投递kernel。

## 原壁纸配置只读采集

`PosterConfigSnapshot.h`只读枚举固定五个mobile Library根，限256项／16文件／每文件64KiB／总128KiB／深度4；逐级O_NOFOLLOW，打开前后stat核对，已知图像不读取，其它文件先读取8B签名。`build_exception_observer.py --poster-config-snapshot`只允许完整backboardd native21／PNG6观察包，在完整原生组后执行采集；不调用UI getter或改变选择／paused／mode。新observer及RAM仍须自身CPU和完整native组准入。

主机复现用 `python -B tools/build/check_poster_config_snapshot.py --output build/新的-reader目录`，再以 `check_poster_config_evidence.py --transcript <该目录/run.log> --output <新的decoder报告>` 解码实际C输出。八组真实文件系统及非法API、FIFO打开竞态、EINTR／读失败／读取中变化、symlink与限额已验；解码器拒绝缺块／重复／SHA破坏／错计数和提前结束。生产 `poster_config_evidence.inspect(text, token)`只接受本LIVE→ACK和准确五根。

本轮实际仅采得偏好，当前选择未识别。后续容器探针须先验证MCM身份和元数据格式，独立绑定新源、协议与host／ARM64输入；不得只修改旧manifest或重发已运行包。

### 当前容器采集源码接入

现行完整backboardd native21／PNG6的build_exception_observer命令可添加 `--poster-container-snapshot`；它和 `--poster-config-snapshot`、`--program-cache-prime`互斥。builder先执行同源C reader／decoder主机检查，再编译实际ARM64 observer，绑定8份容器相关源及完整原生源表。最终manifest由poster_container_contract复验；RAM loader复算source-set，signed包投递仍由package_trial的准确SHA准入，目前尚无已审核的新容器RAM。

revision2只枚举四个固定容器根的直接子目录，读取固定 `.com.apple.mobile_container_manager.metadata.plist`；上限256项／128文件／每文件64KiB／总256KiB。逐级nofollow、打开前后stat与原字节SHA保持；命名元数据的链接、特殊文件、未知签名及JSON不作为有效plist静默跳过。错误或截断让observer完整suite返回失败，并使用现有回程，不改变原renderer／paused／mode或读取应用图片。

```powershell
python -B tools/build/check_poster_container_snapshot.py --output build/新的-container-host --fixtures "$env:TEMP\新的-container-fixtures"
python -B tools/build/inspect_poster_container_metadata.py --image assets/root-build/ipsw-x/ios16-root-lilac-gl5.simg --output build/新的-gl5-container.json
```

第一条在Windows用Ubuntu-22.04实编译同一C reader，13组文件系统ASan／UBSan及8类损坏流拒绝；WSL直接运行可把fixture放在其/tmp，结束后无损转Windows原生回收站。第二条先核对完整受保护gl5 SHA，再直接读取HFS catalog及小元数据，不挂载／展开／修改镜像；原镜像MCM元数据0项不代表运行后没有容器。

源码接入测试以实际C输出和旧signed RAM为输入。运行test_poster_container_integration.py前设置LILAC_CONTAINER_HOST_LOG为本host的normal.log，LILAC_CONTAINER_FIXTURES为其实际fixture绝对根；4组验证包括旧signed包字段污染拒绝、4个无效CLI在输出创建前拒绝、21个contract／source／角色破坏和9个LIVE／ACK／PNG6边界拒绝。生产后处理还独立复验全部native21及PNG6原字节，边界标记不代替GPU证明。新ARM64／RAM及实机验收仍待完成。

### WLED APID状态只读接口候选

`xnu/lilac-spmi-lane-observer.patch`只向锁定的bsd/dev/sweet_spmi.c加入LilacSpmiLaneSnapshot.inc。候选构建需把support下同名.h／.inc复制到该源目录并核对SHA；默认lilac-current.patch和当前kernel没有应用它。API只接受两个WLED PPID，在现有IOLock下观察core／cfg与软件lane／poison状态，两份完整结构相同才输出，不调用init／map／read_ppid／write_ppid／set_owner。

```powershell
python -B tools/build/check_lilac_spmi_lane_snapshot.py --output build/新的-spmi-state-host --shadow "$env:TEMP\新的-spmi-state-shadow"
```

checker先按原SHA导出实际SPMI源，关闭本次git apply的autocrlf后仅加入include，复核其余源字节保持；再编译实际.inc和原sp_find_apid代码。14个状态／API用例及7个无效结构覆盖EE0优先／最后EE0选择、非EE0、poison／未映射状态、观察中变化、超限和失败输出不变。新kernel仍需实际ARM64／source及link审计、新版本private／owner／ready和对应全部GPU先验，当前没有可投递镜像。这个112B快照不提供epoch或保留write lease，实际总线回调与服务注册仍须独立接入和验收。
