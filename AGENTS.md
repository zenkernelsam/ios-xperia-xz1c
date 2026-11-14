# AGENTS.md — Xperia XZ1 Compact / lilac

唯一正式源码仓库是本仓库 ios-xperia-xz1c；所有修补、构建脚本、源码锁和验收结论必须在这里维护并提交。Patch/Xperia 仅保留参考仓库、设备资料和历史归档，不再作为修补主线。

开始工作先读 README.md、docs/STATUS.md、docs/BUILD.md、docs/OPERATIONS.md。

## 硬件与恢复

- 最新v729/774b5efd+GLES7944112d/Metal79eb8207已完成native function constants的14完整/0缺项实机样本：name/index/range/reset、required/type拒绝、创建后改99保持13/29/37三phase实际GPU结果/原buffer/保护区零差异；38.750秒完整PON06返回。原compute/render/queue各14完整。allocated=released1110/unmapped1105，active/orphan/root0，不能称1110全map/unmap；资源probe14完整/1缺项保留。Sony健康fastboot无worker，system Metal/一般shader/IOSurface texture/完整UI和外设active，见docs/METAL-CONSTANTS.md。

- 最新v729/774b5efd+GLES render-r4/7944112d与Metal1b8d1f5d已完成原生AIR顶点/片元与原texture pages实机验收：10完整/0缺项、4phase清屏/viewport/scissor/cull/纹理slot13和sampler3/宽高query/动态上传，171520B全范围包含65600B保护区零差异，830资源全撤销，38.594秒PON06完整返回。CPU/GLES门槛及原compute/queue保持。Sony健康fastboot，无worker/in-flight。system Metal、一般shader/IOSurface texture/完整时间日期壁纸UI及外设仍未完成，目标active；边界见docs/METAL-RENDER.md。r1/r2/r3失败不覆盖。

- 最新v729/774b5efd+r8/c84403c1和Metal/575e0764已完成非空MTLB/AIR→system LLVM C→ESSL/Mesa→FD540→原buffer compute实机验收，22完整/0缺项、2phase×64invocation、完整结果/保护区零差异；原GLES/960000只读像素及892资源全撤销仍通过，38.343秒完整PON06返回。Sony健康fastboot，无worker/in-flight。168导入逐项原20H392 audit/host forced legacy/ASan/UBSan通过。system Metal/texture/render/function constants/完整UI及其他外设仍未完成，目标active。

- 最新v729/774b5efd+r7/db47937b和Metal/a2b11a46已完成原生队列r3实机验收：20完整/0缺项、80次chunk提交/等待、660480B全区域字节/保护、FIFO和回调一致，38.454秒完整自动PON06返回；原GLES/960000只读像素及503资源全撤销仍通过。Sony健康fastboot，无worker/in-flight。原队列r2被32次kernel日志上限影响，独立fence未过保留。system Metal/AIR/texture/render及完整UI/其他外设仍未完成，目标active。

- v729/774b5efd+r5/325810已完成private/owner/ready/r5 RAM CPU/GPU及新Metal资源RAM CPU/GPU实际单发和完整自动PON06返回。Metal资源12组×3实际buffer（432/16384/16385B）的CPU字节/原device释放后持有/最终归零通过；资源GPU38.657秒，原GLES/960000只读像素/340资源全撤销仍通过。Sony健康fastboot，无worker/in-flight。原v728缺记录失败保持；system Metal/队列/AIR/渲染及完整UI/声音/无线等仍未完成，目标active。

- r2 Aegir真实20次_device=nil、原生index ABI=Q；CPU/GPU原GLES和960000只读像素通过，308资源全撤销，自动PON06回程37.188/37.641秒。Sony健康fastboot，无worker/in-flight，完整UI仍缺时间/日期/壁纸。gpu/metal开始iOS16原生前端移植，limits64字段/264B已核对编译；还没有Metal device或render。多设备库存只验证Sony目标行及USB身份，所有操作仍-s BH905ND69G。

- 只操作 Sony BH905ND69G，fastboot 必须带 -s BH905ND69G。
- 串口仅 VID1209/PID0001 且 COM>=10；禁止 COM3 和其它设备。
- PON 只用 0x06。单次 download/boot；失败不重发。ACK/DRAIN/RESET/FINAL_DRAIN、连续健康30秒、旧 worker 退出后才能下一轮。
- 目前 gl5 已在设备；不得擅自 flash/erase userdata、SD、TA、XFL、救砖固件或备份。早期 LIVE 前 E1F4 仍可能没有自动返回通道。
- v724/da6ddc44仍为源码基线；v725/3a9a27b3诊断增量已通过private/owner/ready/RAM CPU/GPU和全部自动回程，GLES库仍9a59820c。GPU allocator/SCM/PON/RPC/VM 的既有约束保留；v679不部署，不根据主机模型宣称实机通过。
- 最新成功内核v728/33580458与前端r4/b035a887已通过private/owner/ready/RAM CPU/GPU及自动回程。只读LayerKit源按只读descriptor/CPU/GPU映射持有；纹理base/stride不满足64B对齐走seed刷新上传，三个只读offset共960000像素PASS。时间/日期/壁纸仍缺，AegirPoster仍退出；全目标active。diagnostic_candidate.py审计79源，新投递强制完整payload复算与同版本/同内核/同RAM库前序证据。

## 目录

- gpu/sweetgl：当前 GLES/CA 源码；gpu/mesa-lilac.patch：Mesa修补。
- xnu/lilac-current.patch：完整当前内核修补；reproducibility/source-lock.json：固定上游及78源校验。
- tools/build、tools/maintenance：维护工具。构建输出仅 build/；外部源码仅 deps/。
- research 只允许参考 Git 仓库，不能塞实验、镜像和临时文件。
- archive、assets、organization 是本地资料，勿提交镜像、备份、私有设备数据。
- 一次性 Windows 工作文件放 %TEMP%；禁止 C:/Users/Cisco He/Desktop。Mac工作在 ~/Desktop/ios-xperia-xz1c。旧路径迁移见 docs/LAYOUT.md。

## 清理

只能归档或原生回收站，不得永久删除。Windows用原生Recycle Bin；WSL废弃文件转Windows回收站；Mac用NSWorkspace/Finder Trash。不能清空Windows/Mac现有回收站替用户决定；Ubuntu垃圾桶非空时先把内容转到Windows回收站再恢复空目录。

## 相机

每次 override → AF(/cam/1/af) → 等2秒 → MJPEG → 每5帧抽1 → rotate90。已旋转的照片不重复旋转。grab_cam.py永久禁用，frame.jpg只允许视频失败后单次兜底。一个controller持有COM/camera，禁止另开。

## 证据与协作

保持完整GPU/显示/声音/无线等目标；当前锁屏时间/日期/壁纸仍缺，不能把离屏PASS当完整UI。失败记录不覆盖，历史证据内容不改。源代码与复现步骤提交正式仓库；大文件留本地并提供输入获取/校验说明。遵循用户的无人值守授权，不主动派子代理。
