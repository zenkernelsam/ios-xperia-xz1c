# 原壁纸请求、environment与启动耗时 — 2026-11-23

原壁纸仍未通过完整UI验收。最新 [2026-11-25 原观察](../reproducibility/poster-cache-budget-wallpaper-observation-20261125.json)为 STOP／OBSERVATION_INCOMPLETE，登记和库返回成功，但本次没有原管线／快照绘制调用；精确产物和设备统一见[STATUS](STATUS.md)。R22 等旧原失败、raw 和少量必要扫描帧保留，旧r1..r6全文与SHA已进入[原文归档](archive/20261123/poster-state-r22-close-out/original-document-manifest.json)。

新库观察的20条原request、128条route、99条environment记录完整，原preview／snapshot返回true、unlock返回0.0、背光progress返回1.0。五次原paused=true、applyMode1沿deactive分支返回；没有新增getter或绘制，没有强制策略。本次唯一缺失准入目标是原pipeline第1次返回，原pipeline BEGIN也未出现，snapshotImage／PNG和draw API均无记录。未观察到调用不推断整个系统没有GPU工作，缓存与原请求链分别验收。该轮原STOP、702521 B raw和39.438秒完整回程保持，失败包不重发。

[同靴扫描](../reproducibility/poster-cache-budget-wallpaper-scanout-actual-20261125.json)与[较小观察包](../reproducibility/poster-cache-budget-minimal-gpu-actual-20261125.json)均复现原pipeline目标未出现，150秒真实帧完整、背景仍黑、原STOP保留。较小包未编译request／route包装，7条库／ColorSync返回完整，仍无snapshot／PNG。不能据此归因为额外观察包装或底层GPU；后续集中核对系统原快照请求入口。两靴分别38.469／38.468秒完整安全回程，失败不重发。

[单次上滑后的原主页帧](../reproducibility/poster-cache-budget-minimal-swipe-actual-20261125.json)确认系统已切到主页，图标／小组件／Dock像素正常，背景仍黑；原pipeline／snapshot／PNG调用仍未出现。原STOP与完整38.766秒回程保持，合成上滑不代替真人触控。这样原请求停点也在主页状态下复现，下一步独立观察PosterBoard到Aegir的快照请求及回调。

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

R25给作者诊断导出逐48B范围加1000us节流，范围仍各发一次；kernel／GLES／Metal及原渲染／UI策略不改。[自身CPU](../reproducibility/cache-export-paced-r25-cpu-actual-20261123.json)与[完整 GPU 原字节](../reproducibility/cache-export-paced-r25-gpu-actual-20261123.json)均独立通过，含 native21、PNG6 和跨靴缓存 `missing=0`。背景用例的缓存状态不代表原 Aegir 管线命中。

[R26 离线包](../reproducibility/poster-cache-r26-offline-20261123.json)使用同一份 R25 原日志重算的 [native21](../reproducibility/cache-export-paced-r25-native21-proof-20261123.json) 和 [PNG6](../reproducibility/cache-export-paced-r25-png6-proof-20261123.json) 证明。诊断记录 Aegir 原路径的请求、快照和环境返回，沿用已验缓存来源；ARM64 签名、实际源码 SHA 和只读 RAM 审计已通过。自身 CPU 及 GPU 打包拒绝见 [STATUS](STATUS.md) 和[原阶段记录](../reproducibility/poster-cache-r26-stage-20261124.json)；尚无本包的原壁纸、FPS 或光学证据。

[R22 与 R25 缓存对照](../reproducibility/poster-cache-r22-r25-comparison-20261124.json)从两份真实原日志复算：同一缓存 scope 内，R22 的五条新请求按两段源码长度各有一个 R25 请求候选，且都存在同键二进制；五条 R22 导入命中也各有一个尺寸与格式相符的候选。这些是跨靴元数据匹配，原运行日志没有记录键，不能据此证明 R26 会导入这些二进制。缓存范围校验按 `exec AegirPoster` 分段，各段须先确认自己的范围，再观察登记或编译；同一次启动内重复初始化仍拒绝。

R26 实机的[原日志复算](../reproducibility/poster-cache-r26-gpu-actual-20261124.json)记录到两条新原请求（顶点／片元 4840／7830 和 4840／9711 B）、两次成功写入（17437、44221 B），以及 17 次实际导入命中。同一缓存范围中仍有两次 Aegir 扩展签到超时，说明旧清单齐全和本靴有缓存命中都不足以排除原路径的签到问题。[实际调用点符号化](../reproducibility/poster-cache-r26-gpu-link-symbols-20261124.json)把第一个 PID 的终止栈定位到 `nir_opt_deref_impl → gl_nir_inline_functions → gl_nir_link_glsl`；第二个 PID 没有唯一的已验返回地址，仍不归因。38 条状态和 206 条完整原调用记录可作后续定位；另有两条未闭合调用。提前取证的帧在偏移 355968 首次发现缺块，未保存完整像素。下一步按各进程原始启动与崩溃记录区分新链接、后续渲染和签到，并把失败期高流量截图与壁纸启动观察分开。

[R26 对 R25 缓存对照](../reproducibility/poster-cache-r26-r25-comparison-20261124.json)复现了原扫描块拒绝，并独立复验串口与回程；两条新请求按源码长度在 R25 清单中均无候选，另外五次成功导入的负载大小也不见于 R25 清单。比较仍只提供元数据线索。随后 [R27 背景实机](../reproducibility/poster-cache-r27-inventory-actual-20261124.json)独立验出新增的两对请求／二进制和全部旧文件 SHA 未变，`missing=0`、`action=skip`。[R26 与 R27 对照](../reproducibility/poster-cache-r26-r27-comparison-20261124.json)中，新增键的请求长度与负载字节数都和 R26 吻合，17 次导入也均有唯一大小／格式候选；这不是原调用记录的键。下次原路径试验应隔离 Aegir 扩展签到与后续渲染，并观察暖缓存下是否仍复现，不能重发 R26 的失败镜像。

[v731 原壁纸启动诊断](../reproducibility/lilac-wled-v731-poster-startup-actual-20261124.json)使用同核 Metal21／PNG6 先验，缩减 Aegir 观察项，并取消本轮扫描和相机。原日志中无扩展签到超时，14 次二进制命中，64 条完整 postdraw 调用。三份快照分别用时 1.769、1.367 和 12.911 秒，三个 PNG 编码调用均返回 27208 字节。最后一次快照的 CPU 时间为 12.135 秒；同一范围内反复记录二进制写入 `-28`。这与现有缓存预算逻辑一致：已有 62 个文件时，第 63 个请求可落盘，但下一次写入要求 `63 + 2 <= 64`。仍需用后续清单确认实际文件数；错误码本身不证明磁盘已满。该轮 fastboot 枚举中断，原结果 `STOP_RESET_UNCONFIRMED` 保留，不重发。未取得原壁纸像素。

缓存写入预算修复仍保留 64 文件／32 MiB 上限和独占发布。macOS 的独占重命名只占一个名字，未配对请求在检查时为后续二进制留位；WSL 的硬链接测试路径会保留 staging 名，因此两者都计入预算。两平台已执行真实文件写入边界测试，WSL 另对照了 GLES 二进制重载前后的像素。这里验证的是固定串行写入边界；没有把宿主结果当作手机原清单或壁纸显示证据。新库身份和待跑准入见 [STATUS](STATUS.md)。

## 只读复验

以下只解已有本轮raw，输出必须新建，不打开COM或启动手机：

```powershell
python -B tools/device/poster_state_evidence.py --directory "$env:TEMP\lilac-v730-poster-state-r22-gpu-live-20261123" --output "$env:TEMP\新的-poster-state.json"
python -B tools/device/scanout_trial_evidence.py --directory "$env:TEMP\lilac-v730-poster-state-r22-gpu-live-20261123" --output "$env:TEMP\新的-failure-scanout.json"
python -B tools/device/poster_cache_comparison.py --observation "$env:TEMP\lilac-v730-poster-state-r22-gpu-live-20261123" --inventory "$env:TEMP\lilac-v730-cache-export-paced-r25-gpu-live-20261123" --output build/新的-cache-comparison.json
```

R26 没有有效扫描帧，须使用 `--failed-capture` 重放原始解码拒绝。该模式复算完整 payload、原串口与四段回程，只生成缓存比较，保持原 `STOP`。

```powershell
python -B tools/device/poster_cache_comparison.py --observation "$env:TEMP\lilac-v730-poster-cache-r26-gpu-live-20261124" --inventory "$env:TEMP\lilac-v730-poster-cache-r27-inventory-live-20261124" --failed-capture --output build/新的-r26-r27-对照.json
```

本轮唯一720×1280／BGR888／stride2160扫描帧的源、寄存器、3431块、CRC／FNV及保存像素独立通过：[scanout](../reproducibility/poster-state-r22-gpu-scanout-20261123.json)。它在失败时提前请求，没有执行计划150秒及一次上滑；14.781秒为请求后单帧捕获、解码与保存耗时。缓冲区取证与LCD光学、睡眠唤醒、FPS及其它外设分别验收。

新缓存末槽修复后的 GLES 已完成自身 CPU／GLES9；[新 Metal21／PNG6 原字节验收](../reproducibility/poster-cache-budget-native21-prime-actual-20261124.json)另绑定新库及本靴 token。实际新来源清单为24对／48文件、missing0、条件预热跳过，150秒扫描完整。该背景包没有原 Aegir 登记，因此锁屏黑背景仍保留为下一轮原请求问题；原 Vista paused／mode 保持，不替换壁纸或强行解除策略。不同 build 的旧 cache 未改、旧失败未重发。

历史原请求／ABI拒绝、USB打开拒绝、mode修正及原STOP都保留在原报告和归档。新的构建与单发规则见[BUILD](BUILD.md)、[OPERATIONS](OPERATIONS.md)。
