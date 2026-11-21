# 附件真机验收与布局卡点 — 2026-11-20

用户确认重新按音量上插线并亮蓝灯后，Sony BH905ND69G 的新会话先通过独立健康31.281秒。已备好的新GLES自身CPU、GLES9及presentation reference自身CPU均实机通过；完整reference GPU被深度／模板／Memoryless／MSAA的真实字节检查拒绝。所有轮次单download／boot，四marker／PON0x06／健康30秒及worker退出均完整；失败包不重发。完整UI、壁纸、FPS、光学和其它外设目标保持。

| 试验 | token | 实际结果 | 回程含健康 |
| --- | --- | --- | --- |
| GLES21de自身CPU | `87c89bcff0f44fed` | PASS | 38.532秒 |
| GLES21de全部9组 | `929a353138003730` | PASS，CA矩阵max_diff0 | 38.375秒 |
| Metal8feea459 reference CPU | `367bebc61fc1866b` | PASS | 38.953秒 |
| 完整native21 reference GPU | `011e16b3caf5b1a6` | REVISE／DEPTH32_PROBE_FAILED | 39.625秒 |
| 失败页面observer自身CPU | `b798b65bc0e997fa` | PASS | 38.094秒 |
| 失败页面独立GPU诊断 | `62fb2e559abcbe3d` | REVISE，12页完整保存 | 38.922秒 |

原presentation observer33bf5ee4和诊断observer7fdb9f19均绑定同一GLES21de6821、Metal8feea459／source-mapdb1605ef、v730/f258dee9及固定Mesa24.3.4。完整native21与六组PNG没有通过，不能生成新的准入proof或登记原poster。旧已验CA基线d6d8fcfa／Metal5e94ab57仍保留。全部原token／图像／日志／失败和前序手册原文保持；四轮首批摘要见 [原实机报告](../reproducibility/attachment-hardware-r1-20261120.json)。

GLES本轮150.047秒的完整720×1280／BGR888／stride2160扫描图保留灰色时间日期、绿色电池及白homebar，背景黑，PNG SHA `b28204baf76dd272e0e12acdfc7cf663b39844d50f5ca31dba877490d6db495e`。它是11月20日实际捕获；reference失败后的提前单帧仍logo，不冒充150秒UI样本。扫描图不能证明LCD光学／背光。

## 实际错误与取证

最早Depth32 phase0：GPU操作／fence均返回成功，但1280B颜色＋1280B深度有560B差异，getBytes有416B差异。颜色原哈希 `f6c90a96ef7f6194` 精确等于独立CPU配方，全部153像素和668B guard正确。深度 `bffad8a3e7484790` 在104个逻辑像素仍是初始`b6b6b6b6`，49个为正确float32 0.5；另有144B逻辑guard被写成0.5。正确数值写进错误位置，不能称depth通过。

Stencil8 phase0有765B／getBytes527B差异；其Depth32页与上述错误逐字节相同。S8的153个逻辑像素只有42个写成3，其余111个仍是0xc7，94B逻辑guard被写成3。Memoryless phase0的颜色55个box像素／全部guard正确，深度页仍非线性。MSAA2 phase0颜色306个原sample及632B guard正确；深度／模板不符。两个resolve目标仍保持0xd8／0xe9，因为phase0是clear-only、没有请求resolve；这一事实不代表resolve已执行失败。每个资源最终allocated0，图像问题不提升为allocator故障。

独立diagnostic只在原作者字节检查失败、command已Completed且实际fence存在时读取已有测试page，24B每块、每页≤4096B、两次FNV稳定。不读原UI page、不生成GPU请求、不改数据、不吞原失败。主机ASan／UBSan、iOS ARM64编译和严格缺块／乱序／重复／外来slot／损坏hash拒绝通过。12页与原handle／GPU地址／fence／LIVE→ACK原raw绑定；原失败没有改为PASS。源码为`gpu/diagnostics/AttachmentPageSnapshot.h`和`tools/device/attachment_snapshot_evidence.py`。

## 开源依据与下一修正

固定Mesa的 [A5xx线性ZS导入提交939486d3](https://github.com/chaotic-cx/mesa-mirror/commit/939486d3d30caf434b93e1d2ccd10f5b2a06ed4d)明确指出GMEM的ZS为tiled，线性sysmem需从MRT0恢复；注释还说明bypass深度需要tiled页及对应采样状态。后续 [20d4c592](https://github.com/chaotic-cx/mesa-mirror/commit/20d4c592c9686374209796ea7ba17fc562279c85)说明depth／stencil附件不能在线性sysmem布局工作，其实际修改主要是A6xx，不能直接称本机A5xx补丁已具备。

当前用户态强制sysmem以保护未为32行GMEM overfetch留空间的原IOSurface，这一已验策略不能全局解除。新深度／模板导入却声明线性页，和A5xx的深度写入契约不符。下一步仅在用户态创建正确tiled附件工作页并通过实际GPU布局交接恢复原线性页面，完整Load／Clear／compare／front-back／guard／fence和2×／4×原sample需重新验收；不CPU修补原页、不把tile数据当线性像素。内核／SweetGPU／allocator／SCM／PON／RPC／VM冻结。

本轮另从v730实际已编译SweetSurface.cpp复验SHA `ad5b7234448525b6568de4fee9ea72806da838a9766612593c21a1c032f83a4d`：IOSurface selectors36／37／38／45仍走zero-output／success默认分支。原20H392共享事件需要真正port及映射页，当前不得以零端口放行。事件草稿未进入当前GPU库；此缺口与本次作者附件字节失败分开记录。Mac只读原root已卸载；当前准确gl5 sparse原输入保持，需要raw时按ROOT-GL5在新目录重建。

## 复现与验证

在新完整reference observer的原构建命令中保留全部fixture／native21／PNG6，只更新当前source。GLES和Metal保持字节时仍先该RAM自己的CPU；新GPU包继续使用`--screen-evidence --screen-after 150`和完整同kernel／精确GLES先验。一次失败只在本LIVE走原0x06回程。

```powershell
python -B -m unittest discover -s tools/device -p test_attachment_snapshot_evidence.py
python -B tools/device/failed_trial_evidence.py "$env:TEMP\本轮目录" --output build/新目录/failure.json
python -B tools/device/attachment_snapshot_evidence.py "$env:TEMP\本轮目录" --output build/新目录/actual-pages
```

独立decoder保存原二进制页、FNV／SHA、真实失败scope。只有完整失败记录可以调用，不能以该输出做native准入。Windows／Mac只在正式ios-xperia-xz1c维护；原文和SHA已保存在 [本轮文档归档](archive/20261120/attachment-hardware-r1/original-documents.json)。所有一次性staging按原生回收规则走回收站，原failure／exact inputs受保护。

## tiled候选收尾 — 2026-11-21

正式DepthTiledPlane.h新增FDL5 TILE5_2独立工作页，原线性backing、offset、stride及guard保持，通过既有Freedreno 2D GPU raw-copy进行Load／Store；原颜色／IOSurface仍sysmem，不CPU转换。ARM64前端GLES c617f2ee／14784032B构建完成，自身CPU122537a0／39.188秒与GLES9 16c49c82／39.297秒实机通过。原Depth／Stencil／Memoryless／MSAA像素及GPU resolve尚未验。

新reference observer4d1f7e9c／blob33af7c5b配该GLES，CPU b2f3fc4f原controller PASS，38.594秒完整0x06回程／健康30秒／worker退出；独立ramcpu-prefix核验发现READY重放第3232B不同，退出1，没有准入，不打包或发送GPU。原stream该处进入Darwin banner，replay进入GLRAM mount，原raw与结果不可改写。下一次先查日志交错／retained-log来源，失败包不复发。见[阶段](../reproducibility/depth-tiled-r1-stage-20261121.json)与[拒绝](../reproducibility/depth-tiled-r1-reference-cpu-refused-20261121.json)。

用户要求这一轮后先清理Mac／整合文档，现已结束实机并整理；显示原生720×1280／360×640profile离线完成，当前device geometry保持，后续独立试验见[DISPLAY-GEOMETRY](DISPLAY-GEOMETRY.md)。

## r2单流及附件回程 — 2026-11-21

主机打开串口DTR提前开启、随后replay会交错尚未完成的IN及重新归零的retained cursor。新capture revision1先DTR低电平打开COM17、以xb新建raw、一次loop token／status后连续读取；原b2f3fc4f字节拒绝保持。独立d0c3a52c CPU一份READY／banner／LIVE和库读取、无replay、38.844秒完整回程，见[原CPU报告](../reproducibility/serial-capture-r1-reference-cpu-20261121.json)。

FDL5 r2新增原/work IOVA、真实sample／cpp、Load／Store copy fence、Z32/S8配对及引用释放记录。仅清一个aspect时同步并记录另一个未改变原页，原byte／guard验证保持。新GLES `0c71a9f6bb36c877acb1cf282a429b74ca68c0dcd88d717eeed67288dd57b158`／14784032B；Metal8feea459及source-mapdb1605ef保持，observer48325a9f。

| r2独立阶段 | token | 结果 | 回程含健康 |
| --- | --- | --- | --- |
| 自身CPU | `3c1d50d57f570af9` | PASS | 38.407秒 |
| 完整GLES9 | `c2ee83f72bd0c04d` | PASS，CA max_diff0 | 38.188秒 |
| reference CPU | `3fd7fdb36cfc8a1e` | PASS | 38.500秒 |
| 完整native21 | `770f8d4a8ae8676d` | REVISE／DEPTH32_PROBE_FAILED | 40.000秒 |

原raw695308B／SHA8fab5b22中创建6个工作页、两条Z/S配对、6条释放；depth原页导入和shader清屏成功，随后复制入口-45（Darwin ENOTSUP），sequence0／operations0／COPY0。D32／S8／Memoryless／MSAA完成phase均0，final_allocated0。没有已完成的FD5 copy，不能声称新字节布局已验，也不能由图像错误推allocator故障。下一步区分context／generation检查与fd5_blitter准入拒绝；当前Mesa编译defines／归档SHA和实际headers已核对，未直接改冻结组件。前序native16本靴输出成功只是局部记录；presentation／PNG没有完整执行，不能生成21组proof。

GLES9的150秒单帧为灰字黑壁纸锁屏／绿色电池／白homebar，PNG90c83f11；reference提前失败帧为logo，PNGba36346e，单帧来源和原REVISE独立保留。完整原报告、精确输入和光学边界见[本轮阶段](../reproducibility/depth-tiled-r2-stage-20261121.json)。完成回程后本轮重复RAM和host产物已原生回收，手机健康fastboot、无worker，失败包不复发。

## r3矩形修复与原depth字节 — 2026-11-21

固定Mesa src/util/box.h的实际pipe_box为x,width,y,height,z,depth。旧位置初始化被解释为X/Y/Z/W/H/D=(0,0,9,0,17,1)，原fd5_blitter.ok_dims拒绝它。DepthTiledCopyBox.h明确字段生成(0,0,0,17,9,1)，Mac主机直接编译原头及未改写ok_dims，ASan／UBSan、边界拒绝通过；原3次host头文件适配失败没有手机操作。r3加入实际BLIT／BOX及context／generation／fd5拒绝记录，核验绑定本revision，不放宽原页字节。

GLESfdcb01f6自身CPU3df033e4／38.703秒、GLES9 3c5743d6／38.906秒、reference CPU79dcf929／37.797秒均通过。native21 06021b67为REVISE／DEPTH32_PROBE_FAILED，42.110秒完整回程。深度原handle57／IOVA0x40090000、work59／0x400a0000、TILE5_2／pitch256；复制BOX17×9×1、mask15／src-dst sample1、gen5，Store fence901实际完成，draw902返回0，原页面1280B／FNVfbced1bfc4d58b90。

颜色page153原像素与guard正确；深度guard也全部0xb6正确，但69像素float0.5、84像素0，phase0 mismatches84／read_mismatches84。两页原SHA／handle／sequence与原rawcdb5ddb0严格绑定，不能以copy Completed代替正确布局。完整S8／Memoryless／MSAA／presentation／PNG未通过；原失败不重发，报告见[r3阶段](../reproducibility/depth-tiled-r3-stage-20261121.json)与[原Mesa主机证据](../reproducibility/depth-tiled-r3-offline-20261121.json)。

新r4离线候选调用固定Mesa fd5_tile_mode取得TILE5_3（sysmem纹理），代替TILE5_2（on-chip GMEM）；只改用户态独立附件work布局选择，不改原线性页、底层／allocator／SCM／PON／RPC／VM。原copy／fence／作者byte和guard门槛继续，尚未实机准入。

## r4独立Depth32／Memoryless及r5候选

r4 GLES `e1ed77e8b77a20329d2d56bbe17ced3bbdd347ba339eb6a3b486b5a3b86e68e7` 自身CPU f95228e8／39.438秒、GLES9 c584111f／37.766秒、reference CPU27205e26／39.031秒通过。native21 4ffc8fd7 REVISE／STENCIL8_PROBE_FAILED，39.578秒四marker／健康30秒／worker退出完整。Depth32全部29阶段74240原byte＋38744guard＋26100getBytes，Memoryless全部8阶段19456原byte＋10888guard及双queue独立页，均已用独立Python及tiled revision4核对真实work IOVA／CPP／BOX／copy fence／引用释放；这些是整组失败靴中的完整局部证明，不能生成native21 proof或提升生产基线。

S8 phase2原color／depth／guard正确，153个模板值全0，应该仍为3。MSAA phase1仅depth34B不同，actual0.5／expected0.75出现在box外；原color／stencil、未请求的resolve目标及全部guard正确。8原作者page／handle／GPU／sequence严格绑定raw c9747676；完整S8／MSAA／presentation／PNG未通过。原阶段见[r4报告](../reproducibility/depth-tiled-r4-stage-20261121.json)。

r5源码候选的Stencil8ClearGLES为combined Z32/S8图像显式绑定depth primary，depth test／write继续关闭。DepthMultisampleCopyGLES用原linear页的整数2D视图逐sample-mask写入真实depth MSAA，Store以实际sampler2DMS读每个样本并GPU bitcast回原integer页；不CPU转换、不平均，原offset／stride和作者样本／guard保持。GL helper在Depth-only及combined 2×／4×主机Mesa上60次Load／Store、53760整数词／外围像素、13770 stencil samples通过，shared scalar状态另2390draw／247860pixels通过UBSan。只是host语义证明，FD540 importer与整组native21需新候选自己的实机；见[r5主机报告](../reproducibility/depth-msaa-sample-r5-host-20261121.json)。

## r6／r7模板Clear与证据长度边界 — 2026-11-21

r6 GLES79184877自身CPU／GLES9／reference CPU通过，native21为REVISE。实际depth MSAA shader使phase1通过，phase2颜色、depth和guard正确，只有55个stencil字节错误：sample0 box正确为90，sample1 box由应有41退回17。Memoryless8通过；Depth29原byte／guard通过，但32COPY／BOX只有31BLIT，严格work协议拒绝，不能将旧摘要中的“完整局部证明”扩为全部元数据已验。原八page及回程见[r6原报告](../reproducibility/depth-msaa-state-r6-stage-20261121.json)。

固定Mesa的fd_context_all_dirty把common FD_DIRTY_*全部置位，并将gen_dirty赋值gen_all_dirty；FD5未安装gen maps，实际0／0合法。核验器保留all_dirty／stencil_ref／sample_mask／scissor=1和gen相等，已补0／0与损坏值故障检查。此修正不补回缺失BLIT，也不提升旧失败。

r7 GPU逐位StencilSampleCopyGLES Load／Store与native Clear在主机通过，实际源码b9afd391／GLES681c4a8c有独立CPU／GLES9／reference CPU准入。完整native21 71d9ff63仍STENCIL8_PROBE_FAILED，40.813秒完整回程。S8 phase2色／depth／guard正确，153值全0／expected3；Memoryless phase2的颜色正确，stencil为0／17而预期不符。五个作者page共5312B，逐块FNV／SHA与原raw严格一致，不CPU修补。见[r7阶段](../reproducibility/stencil-sample-r7-stage-20261121.json)和[真实helper主机检查](../reproducibility/stencil-sample-r7-host-20261121.json)。

r7把BLIT／BOX合成一次frontend write，实机32个BLIT和COPY完整，但32个BOX均在src处被256B tee拆开。Depth29 byte／guard／getBytes仍通过，严格work协议拒绝。单条实际长度边界需要从已编译console源确认，下一版本恢复有界记录；原raw不编辑、不按BOX预期重建。MSAA／presentation在ACK前没有完整END，PNG6不准入。

下一步查实际FBO／Mesa／FD5 stencil Clear状态和清屏前后页面，区分清屏、模板绘制与纹理Store。Memoryless局部出现17说明有实际模板写入，但不证明完整Clear／Load／Store。继续开源用户态路径，内核GPU底层／allocator／SCM／PON／RPC／VM冻结。手机健康fastboot，旧包不重发；最新事实统一[STATUS](STATUS.md)。

本轮结束后已定位固定bsd/dev/mem.c的sweet_null_tee：单write按MIN(uio_curriovlen,256)分块，原raw有带进程前缀的后续字段，合并BOX被换行拆开。因此恢复BLIT／BOX独立write，仅改用户态诊断源码，不改冻结kernel、不补旧raw。新frontend编译与未来实机结果分别记录；原r7仍REVISE。

r8离线候选直接遵循固定u_blitter的每位writemask／REPLACE思想：清零使用GL_ZERO全覆盖draw，随后ref255／1<<bit／fragment discard逐位写回；Clear每次9draw，Load为1+8*samples。避免依赖FD5明确拒绝Z32的硬件快清及空fragment路径，不CPU重排、不改底层。详见r8主机报告，仍需新库完整native21和PNG。
