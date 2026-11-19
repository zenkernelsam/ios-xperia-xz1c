# 私有context中的Clear shader复用

原v730／GLESa7bad22c的五条Aegir管线已预热并命中，但首次750×1624绘制的`targets`为1.523800秒／线程CPU0.225859秒，submit仍未返回。两个目标分别为XR format555与R8；两条Clear记录对应不同附件，不能删成重复操作。

`bounded_clear`每次都会编译相同的shader。新实现把这些成功链接的程序保留在原私有GLES context的`SweetMetalShaderCache`内，普通浮点／UNorm使用一个slot，四种XR整数格式各用独立slot。每次仍重新设置原clear颜色、viewport、scissor和shader draw；原raw snapshot、XR／Load、padding、附件／fence／retire语义保持。编译或链接失败不发布缓存，context释放时删除全部clear程序，跨context不复用GL名字。该优化不影响原失败结果或延长check-in。

新增只读阶段观察把`targets`分成附件import、snapshot、linear和clear，最多跟随原32次draw；原GL顺序与errno保持。分段墙钟包含CPU、driver及GPU／调度等待，不能直接宣称硬件时间。不得把Clear shader复用当完整壁纸／UI／FPS通过。

`check_clear_cache.py`提取当前生产`bind_xr_lut`／`bounded_clear`及真实cache dispose函数，在WSL Mesa运行21组cached／uncached清屏，完整读取结果逐字节相同。覆盖float／UNorm／四XR整数格式、变化clear颜色、编译失败后重试、两个独立真实EGL context及程序释放。UBSan以不恢复模式通过；本轮未运行ASan，不宣称ASan通过。主机结果不能验证FD540导入页、padding或手机时耗。

新后台`--clear-cache-probe`在原14组之后连续执行两轮八附件Clear＋scissor内原MRT draw，检查262144B全部原页面／行padding／前后guard、不同RGBA／BGRA颜色、fence与final_allocated0；第一shader编译、其余15次实际复用必须在原raw可见。全native先验增加第15组`Clear_cache`，旧14组proof拒绝新库，原RAM仍需自身CPU。共享PNG也需新kernel/GLES/Metal及完整15组原raw。

```powershell
python tools/build/check_clear_cache.py --output build/clear-cache-r1/host-gles-r4
python tools/device/test_clear_cache_evidence.py
python tools/device/test_render_stage_evidence.py
```

各输出目录必须不存在。Mac编译GLES沿固定Mesa归档、source-lock和正式BUILD流程；同库先GLES CPU／GPU，再新RAM CPU／完整15组／共享PNG，随后新native与PNG proof、新原observer自身CPU、原GPU验收。当前仅主机检查与源码完成，设备验收状态以STATUS为准。

固定Mesa24.3.4的`ir3_shader.c:create_variant`在原draw时才创建部分变体，`ir3_disk_cache`受本构建`shader-cache=disabled`影响；标准Mesa program binary不能被当作完整硬件变体缓存。这个源码事实与当前首次submit等待相关，但尚无该线程实机细分证据，不据此直接改编译器或冻结内核。下一轮先用原分段数据判定。
