# Half 舍入与真实 Earth 冷链接 — 2026-11-16

原static-half-r1的真实Earth VS/FS已编译成功，但后台240秒内链接未返回。只读request-export-r2接回两个实际请求共142493B，4454段、完整record/payload/key/identity零差异；生产XR包装重建的长度与FNV精确匹配原靴。原Apple源码仅保留在本机TEMP，不提交Git。

相同主机Mesa23.2.1/llvmpipe、禁用shader disk cache的对照中，两份原着色器都超过60秒未链接完成。half位运算改为无分支选择后为17.072/5.041秒；再折叠直接重复舍入、完整half标量字面值和精确0/1构造后为6.934/2.096秒。见reproducibility/earth-link-half-*-host-r1.json。这是主机编译证据，不是手机时耗或UI成功。

生产HalfESSL保留每个真正算术结果的binary16舍入。动态pack/unpack/round使用clamped shift与整数选择，不用未验证的原生FP16快捷路径。仅移除Round(Round(x))，不会跨过加减等运算；仅将完整canonical decimal uint16 literal转换为相同float32位型。超范围、非canonical/octal/hex、其它swizzle、附加运算保持原路径。0/1构造均可精确表示。

独立旧实现保存在gpu/metal/air/half/reference/lilac_half_shader_before_selection.glsl。复现：

    python tools/build/check_half_selection.py --distribution Ubuntu --output <Windows-TEMP内的新目录>

全部4294967296种float32输入，pack/round等价及round幂等共12884901888比较，加65536种unpack均零差异/UBSan通过；65536种literal位型、嵌套/间隔运算/非canonical拒绝和131072个主机GLES样本通过。Mac对完全同SHA的target测试另做ASan/UBSan。WSL ASan启动失败单列，不能把UBSan写成GLES ASan通过。证据见half-selection-r1-host-proof.json、half-selection-r1-mac-target-proof.json和host-asan-infrastructure-r1.json；原1.14GB信号重复日志已原生回收，完整SHA、前64KiB及回收位置保留。

新Metal的RAM包强制使用按GLES库和实际Metal源码map派生的/private/var/tmp/sgm-<identity前16B>目录。observer在任何native/original工作之前创建新1777 sticky共享目录；既有文件/目录不改写不删，拒绝不同override、symlink和错误mode。Backboard与原extension可能有不同UID，文件发布仍走现有不可覆盖/no-follow/摘要codec。单prefix仍限制64文件/32MiB。旧目录保持，旧版缺bin的超时request不会被新primer选中。

程序binary身份仍绑定实际GLES/Mesa/context，key仍绑定全部真实source/config/payload；源码目录隔离不替代这些校验。build_metal_library、build_exception_observer、ram_library和实际日志分别强制相同scope。Scope失败正常回程，不延长check-in或修改冻结PON/RPC/VM/SCM/allocator。

当前新Metal5e94ab57已编译，kernel v729/774b5efd与GLES0a2c7e69保持。先本包CPU，再完整14组原字节/fence/生命周期/回程证明，随后新proof准入原Aegir。旧be56ad13证明不准入新库。真实draw/present、完整锁屏/壁纸/UI、FPS和睡眠继续独立验收，以STATUS为准。

完整14组新库实机已通过：token45a839fc4c105e01、rawf9b13434…、39.062秒完整回程/健康/worker退出，见v729-half-selection-r1-native-proof.json及actual.json。全部原页面/低位/保护区与fence、sampler和三次fresh binary恢复保持。原系统/UI继续单独验收。
