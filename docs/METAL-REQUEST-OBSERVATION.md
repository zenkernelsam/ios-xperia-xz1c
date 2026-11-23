# 原壁纸请求、environment与启动耗时 — 2026-11-23

当前原壁纸仍未通过完整UI验收。R22原GPU以REVISE／EXTENSION_CHECKIN_TIMEOUT结束，完整0x06返回与worker退出通过；原失败包、raw和唯一失败时scanout保留。精确产物和设备状态统一见[STATUS](STATUS.md)，[R22原阶段](../reproducibility/poster-state-r22-stage-20261123.json)给本轮边界。旧r1..r6全文与SHA已进入[本轮原文归档](archive/20261123/poster-state-r22-close-out/original-document-manifest.json)。

## 原调用观察

QuadRouteObserve只包装原owner与准确ABI的方法，保持原参数、返回、nil、throw及errno。R13的33／33安装、127条完整route、20条request与150秒home扫描已独立核对，原结果仍STOP／OBSERVATION_INCOMPLETE：[R13](../reproducibility/original-mode-swift-owner-r13-stage-20261122.json)。没有强制原mode、解除Vista paused或绘制替代壁纸。

原applyMode跳表为0=active、1=deactive、2=interactive、3=supplemental。原Aegir初始化自己请求1，deactive调用stopAnimation；这一初始状态本身不是驱动故障。Vista原start／updateAnimation还检查原display／animation位。固定20H392代码锚点、method type／IMP及模式元数据见[静态路由](../reproducibility/poster-routing-20H392-offline.json)、[环境方法](../reproducibility/poster-environment-20H392-methods.json)、[33方法](../reproducibility/quad-route-20H392-methods.json)。Apple切片、反汇编与原shader保持本地。

原ClockKitUI sample count同时受options控制，旧原init的0x72不请求MSAA／depth；MSAA等完整能力与黑壁纸的原调用问题分开验证。共享device／queue返回、原add_quad／scene与原paused均按当次raw记录，不从缺少display推断整靴GPU无工作。

## R22有界返回状态

旧route每API8条／总264条的限制保留。新增PosterStateObserve只接收原getter／callback已经返回的位值，按(api, receiver)合并重复；新receiver、位值或outcome变化继续记录。缓存256个key、最多512条状态，atomic_flag不等待。CONFIG／VALUE／BITS／LIMIT成对解码，mach_absolute_time仅保留原ticks，不换算为光学或FPS。

实际host转发／缓存ASan与UBSan、ARM64签名、新RAM自身CPU通过；[投递前离线记录](../reproducibility/poster-state-r22-offline-20261123.json)保持原范围。本轮[state](../reproducibility/poster-state-r22-gpu-original-state-20261123.json)在本LIVE→ACK解出四个PID294／300／302／307，各6条、共24条完整样本，LIMIT为空。

| 原返回 | 四个进程的已捕获值 |
| --- | --- |
| renderer_env | 每个进程返回自己的原environment身份，后续getter与该身份匹配 |
| preview、snapshot | true |
| unlock | 0.0 |
| backlight、linear_backlight | 1.0 |

这些是合并后的已捕获状态，不能当getter总次数、全靴状态或LCD背光测量。未观察到env_init／env_update的完整返回状态。原route中PID300/call14和PID302/call31缺ID，发生在失败帧传输期间；[严格拒绝](../reproducibility/poster-state-r22-route-original-rejection-20261123.json)保留，不补造缺失记录。

## 实际链接栈与下一步

三次原check-in超时位于LIVE→ACK，第四次在ACK后退出期。[失败复算](../reproducibility/poster-state-r22-gpu-failure-20261123.json)未发现新的missing selector或附件字节失败；这不等于完整GPU通过。

按本库实际glLinkProgram返回点0xa8a8及唯一16KiB对齐映射，两份原栈可定位到固定Mesa／Freedreno用户态：[符号化](../reproducibility/poster-state-r22-gpu-link-symbols-20261123.json)。PID294在nir_opt_copy_prop_vars／prelink_lowering，PID300在nir_inline_functions／gl_nir_inline_functions，均通过gl_nir_link_glsl→st_link_shader→原render_create_samples。第三份截断栈没有唯一返回锚点，拒绝推断。单栈不证明整个等待持续时间，也不排除诊断日志对调度的影响。

本LIVE观察到五条成功写入的不可变原请求，原vertex／fragment字节数分别为4840/12006、15521/5005、4840/38924、4840/73444、4840/26549。IR3命中与程序缓存命中均有记录，后续原链接仍进入CPU优化。日志未见完整返回不能直接证明磁盘bin不存在。

下一轮先读取本build／source-map生成的scope，核对实际.req身份、SHA、参数和对应bin是否存在；随后使用现有最多8项的背景primer，仅处理实际缺bin请求。新observer／RAM须自己的CPU与相应先验，保持冻结kernel／GLES／Metal字节；原policy保持，不在扩展constructor里重复长probe。单download／boot和完整回程后再独立观察原壁纸／UI。

## 已验证缓存与原字节卡点

R23后台在完整21组与PNG6成功后读取同来源55文件／29请求／26bin，确认3个缺失。三项原R22 wrapper长度／FNV均匹配，实际链接11.501／15.216／38.674秒，成功保存227981／375861／747773B的标准0x875f bin；A清单58文件／29请求／29bin、missing0，全部原请求与已有bin SHA保持。[R23原阶段](../reproducibility/cache-inventory-r23-stage-20261123.json)仍STOP：原tee漏SpringBoard日志，未提升为native准入。

R24修正tee后GEO_GRID1与936节点布局记录出现，跨重启缓存仍58文件／missing0、实际action=skip，无空primer。原控制器PASS，但独立PNG6丢失case1/pixels offset4272的一块48B，夹在原SpringBoard CA警告之间；[原拒绝](../reproducibility/cache-inventory-r24-original-PNG-rejection-20261123.json)与[原阶段](../reproducibility/cache-inventory-r24-stage-20261123.json)保留，原字节不补造，整轮不准入／不重发。

R25仅给作者诊断导出逐48B范围加1000us节流，范围仍各发一次；kernel／GLES／Metal及原渲染／UI策略不改。Mac新observer／RAM已签名与校验，Windows转运、自身CPU及完整原字节仍待验：[候选](../reproducibility/cache-export-paced-r25-prepared-20261123.json)。原UI、FPS、光学及其它外设目标继续独立验收。

## 只读复验

以下只解已有本轮raw，输出必须新建，不打开COM或启动手机：

```powershell
python -B tools/device/poster_state_evidence.py --directory "$env:TEMP\lilac-v730-poster-state-r22-gpu-live-20261123" --output "$env:TEMP\新的-poster-state.json"
python -B tools/device/scanout_trial_evidence.py --directory "$env:TEMP\lilac-v730-poster-state-r22-gpu-live-20261123" --output "$env:TEMP\新的-failure-scanout.json"
```

本轮唯一720×1280／BGR888／stride2160扫描帧的源、寄存器、3431块、CRC／FNV及保存像素独立通过：[scanout](../reproducibility/poster-state-r22-gpu-scanout-20261123.json)。它在失败时提前请求，没有执行计划150秒及一次上滑；14.781秒为请求后单帧捕获、解码与保存耗时。缓冲区取证与LCD光学、睡眠唤醒、FPS及其它外设分别验收。

历史原请求／ABI拒绝、USB打开拒绝、mode修正及原STOP都保留在原报告和归档。新的构建与单发规则见[BUILD](BUILD.md)、[OPERATIONS](OPERATIONS.md)。
