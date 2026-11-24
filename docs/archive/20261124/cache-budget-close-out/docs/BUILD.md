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
