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

## 2026-11-19 实机更新

新GLES9d065b69（14747328B）、Metal5e94ab57／source-map3e79503f与原kernel v730/f258dee9已完整验收。GLES自身CPUd8e7ff73／38.094秒、GPUa0a24b45／37.687秒；共享参考自身CPU8ef168db／38.672秒、GPU0c563b0d／38.500秒，八项GLES与全部15组native、六组共享PNG和串口完整扫描帧独立复算通过。Clear组两次八附件共262144B原页面／row padding／前后guard零差异，第一次编译及其余15次shader复用实际可见，fence正常、最终资源为0。旧14组不能准入新库。作者PNG混色5.810→1.888秒、渐变4.628→1.461秒只作参考，不是原UI时耗。

新原observer911859cf绑定0c563b0d的native15／PNG proof，自身CPUe281f838／38.250秒通过。后台primerd1339435自身CPU78cd38e1／37.656秒通过；三个真实缺bin request分别由2eafdd90／d8155a3d／1989b9a0预热，VS／FS15521／5005B、4840／73444B、4840／9711B，实际链接2.155／37.677／2.223秒，每靴完整15组和回程通过。原0bbb2ee3、0e7cc253、0795124f的cold-link REVISE保持；0795124f缺`add_image`原方法来源记录，独立postdraw拒绝不修改，原request保存事实单列。

原410e6aa0的五条pipeline全部命中64421／64317／747773／17437／44221B，nonnull。首draw的XR附件import0.465ms、raw snapshot127.821ms、linear纹理24.670ms分别完整返回；进入Clear后未返回，check-in超时、40.031秒完整回程。此时不能宣称全部targets／submit成功或UI提速。

新增`--thread-sample`只改变诊断配置，继续沿用同kernel／GLES／Metal／原RAM与本包CPU／native15／PNG先验。十二个已查证的数值1参数用XNU已支持的无`=1`拼写腾出24B，加入`sweetsample=AegirPoster`；原FairPlay timeout2000、shim／root／GPU／F12／scanout及0x06返回保持。固定`pexpert/gen/bootargs.c`的ARM路径、1／2／4／8B数值和guard48项UBSan检查通过，实际parser SHA1431f1f8。只有原第一次target Clear且本靴LIVE后，单一controller通过既有worker发送一次`sample`，不另开COM、不重发；原采样代码运行在已验内核线程。

实际采样靴ebc8114c／pid299得到15个线程，五条原管线仍命中，原REVISE与38.656秒四marker／健康／worker退出保持。`symbolicate_render_threads.py`用精确GLES9d065b69，在同线程同时匹配`_prepare_targets`的`glDrawArrays`返回0xc944和`_sweetgl_metal_render_draw`的`_prepare_targets`返回0xb764，唯一16K对齐映射；不借主线程崩溃位置猜worker。

原渲染线程t4为`_prepare_targets`→`_mesa_DrawArrays`→`_fd5_draw_vbos`→`_ir3_cache_lookup`→`_ir3_get_shader`→`_util_queue_fence_wait`。t2／t7／t9／t11分别实际位于IR3/NIR优化或compile variant，并带`create_initial_variants_async`。这证明采样时渲染在等初始硬件变体编译，不证明全部等待时长都属于编译，也不证明GPU／VM硬件故障。原raw、具体offset、退出边界和完整回程见v730-clear-cache-r1-thread-sample-r5各记录。

下一步沿固定Mesa／Freedreno的原IR3变体序列化与缓存路径补齐可复用的硬件编译结果；须绑定实际build／原NIR／variant key，保持完整payload校验、冷路径回退、旧缓存及源码／许可证。不得通过降低shader精度、跳过真实绘制、延长check-in或修改冻结内核来替代目标。新硬件变体缓存将需要它自身host和原设备全组证明；当前尚未安装。Clear context复用已实机通过，但壁纸／时间日期／完整UI／FPS／睡眠及其它外设保持active。当前无in-flight，Sony健康停在fastboot。
