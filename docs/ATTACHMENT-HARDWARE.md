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
