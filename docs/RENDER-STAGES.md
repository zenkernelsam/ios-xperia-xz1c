# 原锁屏绘制阶段 — 2026-11-19

此页保留该能力的原版本实现与实机证据；每个旧“当前”仅指当次日期。现行精确基线／候选／设备只见[STATUS](STATUS.md)，复现与单发分别见[BUILD](BUILD.md)／[OPERATIONS](OPERATIONS.md)。历史阶段结果保留其准确边界，完整原UI／光学需独立验收。

当前同核诊断库为 v730/f258dee9、GLES a7bad22c、Metal 5e94ab57，Metal source-map 3e79503f。GLES 只在原绘制路径增加最多 32 次的阶段计时，原 GL 调用顺序、参数、errno、像素和既有同步保持。完整源码及变更边界见 `gpu/sweetgl/metal_render.c` 和 `reproducibility/render-stage-r1-source-change.json`。

新 GLES 自身 CPU12f3b34d／38.312秒与 GLES1e4c90b4／38.453秒已独立通过。新增共享参考 RAM 自身 CPU0f50a90d／38.375秒、GPU2506ca58／38.000秒也已通过全部 GLES 八项、14组 native、六组共享 PNG、原字节／padding／guard／fence／释放和完整串口截图。当前新库必须使用这些精确先验，旧72d39da5和旧kernel的 proof不能准入新 GLES。

新共享参考的六组 UP 输出仍与原 UIImage 的16位像素、ICC、metadata一致；作者混色6.304→1.746秒、渐变4.323→1.377秒仅为参考数据，不能当原 UI 时耗。原编码 observer72586fe2绑定2506ca58的 native／PNG proof，自身 CPU35c1e99d／38.046秒通过，原Aegir的check-in期限保持。

## 实际原系统与预热

首次原GPU1c45cff4在第二条管线冷链接时超时，未到draw／PNG，37.688秒完整回程。第一条原管线1.828秒返回；第二条的原request15521／5005B保存成功。精确当前GLES重新符号化显示NIR链接流程中的`nir_split_var_copies`／`gl_nir_lower_named_interface_blocks`等，采样位置不证明硬件或VM根因。

后台primer00d17897自身CPUf15246a2／38.266秒通过；它仅处理当前scope缺bin的真实request，不覆盖或删除旧缓存，完整14组仍逐轮核验。

| 原超时靴 | 后台预热靴 | 原request VS／FS | 实际link | 保存binary | 回程含健康 |
| --- | --- | --- | --- | --- | --- |
| 1c45cff4 | 7c3533fb | 15521／5005B | 3.147秒 | 64317B | 38.156秒 |
| 46c0a614 | ba83924a | 4840／73444B | 37.936秒 | 747773B | 38.843秒 |
| 560b5400 | e7e33960 | 4840／9711B | 2.872秒 | 44221B | 38.094秒 |

三个预热靴均完整14组GPU通过。各原靴与primer使用完全相同kernel、GLES、Metal/source-map/cache scope；实际生产XR包装后的VS／FS长度及FNV64相同，link／stored／state2与完整回程均来自各自原raw。配对摘要见 `reproducibility/v730-render-stage-r1-real-request-primes.json`；本次没有导出全部shader字节，不把长度／FNV配对冒充全源码SHA证明。

原46c0a614已出现64421／64317B缓存命中、两条pipeline约195／76ms返回，但第三条冷链接超时，38.719秒回程。原560b5400第三条747773B也命中、约294ms返回，第四条成功，第五条冷链接超时，38.234秒回程。两次原REVISE保持。

最新原GPUe662710b的五条pipeline全部nonnull并命中64421／64317／747773／17437／44221B；对应返回约195／80／236／59／72ms。随后进入真实750×1624、triangle-strip4顶点、3个buffer／1个texture的首个draw。七个阶段已完整返回，`targets`为1.523800秒／线程CPU0.225859秒，buffer与sample各约1.006／0.504ms；`submit`只有BEGIN，随后原扩展check-in超时，未见draw返回、gpu_finish或PNG入口。原REVISE保持，39.172秒四marker／健康30秒／worker退出；当前Sony安全停在fastboot。

本次目标准备包含format555与R8两个实际附件及各自clear。日志中的两次bounded_clear对应不同附件，不能据此删成“一次重复clear”。`targets`包含导入、原位型快照、临时纹理、clear和driver／GPU／调度等待，不能把墙钟减线程CPU直接当GPU时间。render与postdraw计时器的绝对起点不同；各自持续时间分别解释，不跨时钟推算完整扩展预算。ACK前的真实slide0x1f80c000采样落在psynch／CLKUIMetalQuadView快照等待，仅为退出采样。

下一步应分解同一原draw的目标准备与首次提交，保留原附件／像素／padding、Clear／Load语义、原check-in期限及冻结组件。完整壁纸、时间／日期、UI/FPS、睡眠及其它外设继续active。

## 日志与串口截图

`tools/device/render_stage_evidence.py`从原LIVE→ACK范围按进程安装／pid／call绑定REQUEST、12类阶段BEGIN／END和原draw返回；可区分不同进程重启以及两个queue交错等gate。截断、重复、错误call、倒序时间与不匹配draw拒绝，未完成submit保持pending，ACK后的退出期排除。工具只核验已运行日志，不打开COM；先复算原payload、RAM和成功／失败原结果，再核验保存截图。五组故障测试通过。正式当前r2结果为 `v730-render-stage-r1-original-after-prime-r4-stages-r2.json`；初版分析原样保留。

本轮所有截图均为720×1280、BGR888／stride2160的真实扫描缓冲区，PRLE无损，原FNV、每块／记录／帧CRC与保存PNG／BGR逐字节复算。CPU靴power_off=0；原GPU靴45秒截图仍为power_off=1，不能当亮屏或光学验收。UI内容可直接用已校验串口图，背光、LCD色偏、物理行错位仍另验。详见 [SCANOUT-SCREENSHOT](SCANOUT-SCREENSHOT.md)。

## 复现和继续

Mac按正式BUILD在新目录构建；当前GLES源码与固定Mesa归档锁已审计：

```sh
python3 tools/build/build_frontend_trial.py --mesa-build build/mesa-program-cache-r2 --output build/render-stage-r1/frontend
python3 tools/build/package_glram.py --frontend build/render-stage-r1/frontend --output build/render-stage-r1/gles-ram
```

新编译SHA与已验不同，仍须自身CPU／GPU；不得用记录中的a7bad22c proof准入另一份库。全套14组observer沿METAL-BACKBOARD／AIR-STATIC-HALF构建；共享PNG额外`--cif10-png-reference`，随后由`native_prerequisite.py`／`png_encoder_prerequisite.py`从新原raw生成proof。原observer再给`--registration-only --register-device --pipeline-observe --postdraw-observe --snapshot-observe --png-filter-encoder --native-proof ... --png-proof ...`，包装RAM后自身CPU再原GPU。

当前已验原RAM在`build/render-stage-r1/original-ram`，primer在`prime-ram`；CPU原目录分别为TEMP中的`lilac-v730-render-stage-r1-original-ramcpu-20261119`／`...prime-ramcpu-20261119`。当前native／PNG原目录为`...reference-gpu-20261119`，独立GLES原目录为`...gles-gpu-20261119`。所有bundle、token与已发次数必须核对；失败bundle不重发。新单发仍由`package_trial.py --version730 --screen-evidence`和`run_trial.py`完成完整准入、Mac实际源审计、四回程marker及健康／worker退出。

原raw／截图／控制器命令在Windows TEMP，Git只保留源码、SHA和可复算摘要。正式工具锁已补齐v730截图及固定行尾物化依赖，`validate_current.py`覆盖v730增量；不改变boot_once／serial_worker／PON／allocator／SCM／RPC／VM。一次性配对脚本走TEMP，历史失败和当前卡点证据保持。
