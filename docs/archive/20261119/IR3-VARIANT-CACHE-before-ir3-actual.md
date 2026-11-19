# 固定Freedreno IR3硬件变体缓存

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

原完整锁屏、壁纸/UI/FPS、睡眠、声音/无线及广义textures/depth/stencil/MSAA仍active。当前实现尚无新设备准入，以STATUS为准；不凭主机或缓存PASS完成原目标。所有实机继续单Sony/合格COM/PON06/单download-boot、完整四marker、健康30秒/worker退出。新cache不触碰冻结kernel/VM/SCM/PON/allocator或用户镜像分区。
