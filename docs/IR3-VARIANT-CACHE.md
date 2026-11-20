# 固定Freedreno IR3硬件变体缓存

此页保留该能力的原版本实现与实机证据；每个旧“当前”仅指当次日期。现行精确基线／候选／设备只见[STATUS](STATUS.md)，复现与单发分别见[BUILD](BUILD.md)／[OPERATIONS](OPERATIONS.md)。新附件native21尚未准入，旧阶段成功不扩展为完整UI。

2026-11-19现行GLES为CA矩阵d6d8fcfa，IR3/Mesa原源码和归档保持但build-ID变化；它已自身GLES原8＋CA1组／完整native16与PNG6组重新通过，proof token46747c46。当前锁屏时间日期／上滑主页有完整scanout证据，壁纸黑／原Aegir无pipeline请求的STOP保持，详见 [CA-MATRIX](CA-MATRIX.md) 和STATUS。下方166c08a3／87397739等是原适用库的保留比较，不能准入当前库。

原v730/GLES9d065b69的15线程采样已定位Clear的`glDrawArrays`等待初始IR3编译队列。Mesa标准program binary保存的NIR与此前后台预热可以减少glLink开销，但仍可能在原进程重新创建硬件变体。本实现继续使用固定Mesa24.3.4/769e5146的IR3布局和原编译器，不降低精度或延长check-in。

增量`gpu/mesa-ir3-variant-cache.patch`只接入原`ir3_disk_cache_init_shader_key/retrieve/store`并在shader对象中保存原NIR/options key；`gpu/mesa/IR3VariantCache.h`使用原固定suffix、bin、const_state与immediates布局，draw与binning两份成组保存。存储前清除assembler临时指针和不再使用的NIR指针，恢复后重新绑定本进程对象；未界定的global UBO路径保留冷编译。恢复先在独立ralloc临时域核验尺寸、数目、原options/streamout及整个payload消费，全部完成后发布，两份中任何失败不修改原对象。

原编译result由`gpu/sweetgl/ir3_cache.c`保存为`sgiv-<实际build>-<key>.bin`。112B外层记录绑定全部frontend、实际Mesa归档/defines生成的build ID、原序列化NIR/完整shader options/streamout、完整variant key、stage/ABI/GPU/debug及payload SHA256。最高8MiB/entry、512项/64MiB；独占发布，不覆盖、不删除旧文件；不适用、缺失、损坏、截断、部分恢复都返回原编译流程。只明确启用FD540，其它Mesa后端不改变。

主机持久化已用真实读写与子进程完整回读验证，错误build/key/digest/size拒绝，errno保持。真实Mesa的IR3结构、blob/ralloc下完整draw/binning恢复、pointer rebase和partial不发布已Mac ASan/UBSan通过。初版Mac host编译因沿用iphone SDK的`HAVE_ENDIAN_H`而失败，旧输出保留；host改用实际macOS可用头定义，生产iOS flags不变。主机测试不能证明手机硬件cache命中、精确像素/fence或完整UI。

```sh
python3 tools/build/build_gles.py --mesa-build build/mesa-ir3-cache-r1 --frontend-output build/ir3-cache-r1/base-frontend --ir3-cache
python3 tools/build/build_frontend_trial.py --mesa-build build/mesa-ir3-cache-r1 --output build/ir3-cache-r1/frontend
python3 tools/build/package_glram.py --frontend build/ir3-cache-r1/frontend --output build/ir3-cache-r1/gles-ram
```

必须新Mesa输出；原Mesa归档/旧构建目录保留。installer逐源验证固定base、增量patch和header，frontend manifest记录真实新增输入，投递前独立复算Mac当前源码/header/归档。新GLES先自身CPU/八项GPU，再新RAM自身CPU，后台原15组后新增`--ir3-cache-probe`第16组：三份新计算pipeline分别specialize13/29/13，原320B页及guard逐组核验，最后13必须实际命中硬件variant并保证结果一致。旧15组proof不能准入新库。共享PNG、原Aegir短登记及其自身CPU仍依精确同kernel/GLES/Metal/source-map的完整先验。

2026-11-19新GLES166c08a3／14764656B已通过自身CPU与八项GPU，八项实机日志含107条accepted／18条stored且原像素零差异。reference RAM自身CPU与完整16组／六组共享PNG已通过，token0852dc25cd348667、38.391秒完整回程。新增IR3组三份fresh specialize13／29／13分别hits5／bytes_read18584／rejected0，原960B结果／guard一致，final_allocated0；这是进程统计，不将全部hit逐一归属该kernel。公开proof绑定Metal5e94ab57及新source-map7b5a083a，旧15组不能准入。

原RAM CPUcabd5297和primer CPU4c74f94e分别38.094／37.890秒完整回程。原实际缺binary请求通过三轮完整16组后台primer生成，原request／生产wrapper长度与FNV逐条配对：15521／5005B→link2.156秒／64317B；4840／73444B→45.721秒／747773B；4840／9711B→2.100秒／44221B。配对证据为`v730-ir3-cache-r1-prime-{original,earth,cloud}-paired.json`。三个prime_begin→ACK内均无IR3 stored／accepted日志，标准NIR program binary完成不等于其异步硬件变体全部完成。固定`util_queue_drop_job`会取消尚未运行的任务，已有variant才可检查真实completion；这一生命周期改进尚未安装，现有生产源保持。

原5398eff3已五条pipeline命中，两个Clear均返回，实际750×1624 draw完成，snapshot1.935秒、ImageIO_UP PNG0.760秒／35558B，但随后extension check-in timeout，REVISE及38.281秒回程保持。下一原87397739在观察窗口内无该timeout，26pipeline全部nonnull（包含月球与sprite等新路径）、三次原draw result0／三次PNG35558B返回，登记阶段PASS。53个postdraw样本完整／0未完成，snapshot10.030／1.123／4.295秒、PNG0.843／0.396／0.690秒；首个snapshot含月球冷link8.498秒，不能宣称统一1.123秒或FPS完成。39.625秒四marker／健康30秒／worker退出通过。所有旧cold-link与失败结果均保留。

45秒原scanout仍logo／power_off1，不代替较晚锁屏、完整壁纸或亮屏证据。电脑端新增150秒单次截帧选项保持当前内核与GPU字节，详细使用见SCANOUT-SCREENSHOT。原完整锁屏、壁纸/UI/FPS、睡眠、声音/无线及广义textures/depth/stencil/MSAA仍active，以STATUS为准；不凭主机、缓存或controller PASS完成原目标。所有实机继续单Sony/合格COM/PON06/单download-boot、完整四marker、健康30秒/worker退出。新cache不触碰冻结kernel/VM/SCM/PON/allocator或用户镜像分区。

最新较晚截图4dd119fa／150.047秒实际完整传回、power_off0，图中电池／相机／homebar可见，时间／日期／壁纸仍缺失。该靴只有原device／metallib返回，未出现pipeline请求，原controller保持STOP／OBSERVATION_INCOMPLETE；38.172秒完整回程／健康／worker退出。不能因上一靴三次draw或本靴读屏通过补造此靴GPU结果，当前Sony健康fastboot／无in-flight。
