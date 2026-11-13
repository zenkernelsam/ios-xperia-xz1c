# AIR/Metal compute — 2026-11-14

实际v729/774b5efd+r8/c84403c1与Metal/575e0764已通过原生AIR计算全链：MTLB容器的真正bitcode → system LLVM C解析 → 原始开源AIR lowering → ESSL目标适配 → Mesa/FD540 → 当前SweetScatter → 原Metal buffer的GPU/CPU共享页面。使用标准newLibraryWithData/newFunctionWithName/newComputePipelineStateWithFunction/computeCommandEncoder/dispatchThreadgroups/commit/wait；没有用shader名字替代实现，也没有CPU计算填充预期结果。

22份完整/0缺项样本，每份2次64invocation的非空add_seven kernel，完整检查80word/phase含16word保护区。共2816invocation、3520word结果核对及704次保护word检查，hashfce665879360b812与2222334a6653ec69按独立CPU配方重算一致。两次GPU fence属于相同原buffer，CPU地址与IOVA保持；actual compiled ESSL1820B/hash18725e3c75b5d3f5/robust1，原buffer logical320B，未改动后16word。原GLES全链、IOSurface/caller动态共享和960000只读像素仍通过，最后892资源全部released/unmapped，orphan/active/root0。

本靴token379f2d43a9a33390，原日志661202B/SHA0034db7947510b1bff59b211cfe36d97b18c3a6df71ed0b7d4c2606cedaef5d6，单download/boot/retries0，四标记及健康30秒完整38.343秒PON06回程。r8原GLES CPU37.454/GPU38.609，新AIR包CPU37.625秒完整返回。手机当前健康fastboot，无controller/COM/camera/in-flight；面板仍缺时间/日期/壁纸，不能将compute通过当system Metal或完整UI。

参考12文件保留ios_sagit898998cf原字节，来源/许可证/SHA记录air-reference.json。目标iOS16的原libLLVM cache导出核对显示原编译170导入缺11项；LLVM16Compat只用已有C API和原指令表示适配，最终168实际导入均在原版20H392中找到。legacy typed-global/GEP/shuffle/atomic/pass manager的真实C测试与Metal计算对象的ASan/UBSan通过，强制旧接口翻译与可用原函数翻译一致。Windows WSL LLVM14文本/bitcode测试的PIE/ASan运行异常已保留；非PIE ASan/UBSan通过，未将调试器或无sanitizer运行代替该验证。Mac forced-legacy测试也通过。

标准MTLB读取校验4段边界/重叠、函数NAME/TYPE/OFFT/MDSZ/HASH，AIR slice SHA与实际stage一致才准入。authored typed-pointer fixture通过LLVM14组装，2288B bitcode/1d16f801与2722B MTLB/81d495d6已校验；make_air_fixture.py保留原源码和生成配方，不提交Apple shader或设备私有镜像。Metal library与function保留原AIR输入，管线最多16 local-size变体。current encoder结束时释放自身资源引用，由recorded command持有backing/管线直到完成，主机生命周期回归已通过。

原buffer借入Mesa时增加独立持有关系。Mesa视图关闭不释放原Metal lease，原对象先释放则延后到最后视图关闭再退休；C host测试覆盖两种次序、失败边界与全部lease归零。运行使用专用robust GLES上下文，既有EAGL flags不改，保存/恢复原Mesa current和用户态identity。原buffer视图大小/对齐/逻辑范围和dispatch/local bounds都按真实约束核对。

尚未完成system MetalPluginName、原生纹理/render/IOSurface、function constants、一般AIR shader及graphics API覆盖；原Aegir仍因system device nil退出，时间/日期文字路径是否另有问题仍需单独核验。声音/无线/睡眠唤醒等完整目标保持active。下一继续native texture/render及系统device discovery。

复现：tools/build/import_air_reference.py取固定OSS源；materialize_air_sources.py创建build内独立输入；build_air_frontend.py编译及目标导入审计；check_metal_compute.py与check_metal_resources.py核对真实方法体；make_air_fixture.py用LLVM14组装合法fixture；先r8全GLES准入，再build_metal_library.py --air-package与build_exception_observer.py --air-fixture创建新包，package_glram.py/run_trial.py继续同kernel/精确RAM库CPU/原GLES门槛。Git仅保存源码/配方和验收摘要，原日志/照片/镜像在本地archive/build。
