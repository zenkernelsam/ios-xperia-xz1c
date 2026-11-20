# 只读 LayerKit 背板

v726/region-r1实机在原样转发initWithProperties之前读取VM元数据。12次400×400请求的IOSurfaceAddress指向LayerKit（tag51）映射，当前权限和最大权限均VM_PROT_READ=1，raw640000、地址偏移32B、16KiB页覆盖655360。原kIODirectionInOut的memoryReferenceCreate请求VM_PROT_READ|WRITE，全部拒绝KERN_PROTECTION_FAILURE=2。69×69也复现相同冲突；可写共享自检的源区域为prot3/max7。

region-r1新库18a97e2c在v726内核上CPU/GPU交付通过，原共享/纹理/CA/生命周期数据通过，但本靴1126次400×400创建失败、6次AegirPoster退出，面板仍缺时间/日期/壁纸。原controller的CPU独立核验首先被错误的“库必须在LIVE后读出”假设拒绝；真实库读出位于当前READY与LIVE之间，原日志重新独立核验通过，原结果未改。

v727沿用真实VM区域保护权限选择描述符方向。可写源保持kIODirectionInOut；有读取权限但不能写的源使用kIODirectionOut。原页偏移、页面持有、descriptor prepare/complete配对、回程及GPU分配器保持。CPU映射使用kIOMapReadOnly，非只读锁被拒绝，GPU共享mapper将AP bit7设为只读；每次发布和撤销的叶检查都包括对应权限。没有扩大RAM、改变源权限或将只读源偷偷转换成可写页面。

增量xnu/lilac-surface-readonly.patch应用于v726。prepare_readonly_surface.py生成固定SHA候选；check_readonly_surface.py编译真实方向函数和共享mapper进行ASan/UBSan回放。覆盖只读/可写/混合区域、空洞/无读取/截断/子图/查询错误/64区域边界，以及两类页权限、每个4KiB叶、配对撤销和非法权限拒绝。另用test_shared_regression.cpp复跑原5/12MiB并发/L3/VA/失败保留回归。主机成功不代表实机通过。

Mac安装/构建入口：

```bash
python3 tools/build/check_readonly_surface.py
python3 tools/build/install_readonly_surface.py
bash tools/build/build_diagnostics.sh 727
```

固定输入与前后SHA见reproducibility/surface-diag-v727.json。首编因IOKit不可见vm_map_region_recurse_64声明失败，原源码/日志已保存；修订使用XNU公开vm_region_recurse_64包装，重新主机验证和真实编译通过，kernel4b17a8e56e6905384404789d7d55af7e1a06fb0ab6a105f857ac972f5153bd03。

新前端readonly_probe创建只读最大权限的400×400源，预期CPU只读视图、写锁被拒绝，检查160000像素GPU采样及原VM解除后的第二次采样。它不尝试CPU/GPU写入只读页面。probe库b8332e8b的RAM包单独绑定大小、FNV、SHA、Mesa归档输入和源码，先RAM CPU再GPU。private/owner/ready必须使用同一新内核完整回程证据；gpu还需真实RPM和完全相同RAM库CPU证据。

打包：package_trial.py --version 727 --ram-package build/surface-readonly-r2/ramfs。镜像中完整kernel/ADT/blob逐字节复算、所有fastboot仅BH905ND69G、单download/boot、PON06、四回程标记与健康30秒仍为门槛。完整UI/FPS、声音、无线、睡眠唤醒继续独立验收。

## v727 实际回归与 v728 修订

v727 private/owner/ready/RAM CPU四阶段完整回程通过。GPU阶段全尺寸绘制和普通Surface通过，caller selftest在layout失败，创建返回kIOReturnNotReadable。原结果为REVISE，正常回程38.687秒含健康30秒；只读探针未执行，不能称只读共享通过。

XNU vm_user.c 的旧 vm_region_recurse_64 包装在调用底层后检查map_addr+map_size是否超过VM_MAX_ADDRESS；ARM此上限是0xF0000000。可写caller自检源地址为0x10cf94080，超过此限制。v727的主机stub只模拟底层区域，漏了真实包装检查。

v728单文件增量 lilac-surface-region64.patch 改用真正64位 mach_vm_region_recurse，签名与XNU生成的mach_vm_server.h一致，调用同一底层区域查询。check_region64_surface.py直接提取两份XNU原包装函数，先复现旧包装在高地址的失败，再验证新包装的高/低地址只读/可写、混合区域和错误边界，ASan/UBSan通过。没有跳过保护权限检查或改源权限。编译内核335804584b9e593ffb1b9c153b6309cdd4b4602e8e0b980757aac82647faa3eb已完成79源审计，后续需重新走同内核五阶段准入。

```bash
python3 tools/build/check_region64_surface.py
python3 tools/build/install_region64_surface.py
bash tools/build/build_diagnostics.sh 728
```

host源码提取和候选文件保存在build/surface-readonly-v728；正式修补/锁在xnu与reproducibility。旧失败记录保留且不会重发原镜像。

## r3 实机偏移对比与 r4 生产路径

r3沿用v728内核，三个只读400×400偏移32/64/128完整实测。Mesa记录的bo_iova保持内核偏移；offset32读回hash cb385e665dfd88dd，独立重算恰为从页首采样而少32B。offset64/128各两次160000像素零差异，原VM解除后页面仍可采样。该试验原REVISE和39.687秒回程证据保留，XML基址字段允许32B不能替代真实实现对齐结果。

r4在production bind_surface_storage增加SurfaceTextureLayout.h判断：base或pitch不满足64B时，接入原copy_surface/seed刷新上传；对齐源保留直接共享。该复制源仍是实际IOSurface页面，通过现有锁/seed/快照/上传机制更新；不删IOSurfaceAddress、不修改源权限。readonly_probe通过同一production bind_surface路径，32B复制和64/128B共享的两次采样（含原VM解除后再绑定）累计960000像素全部实机通过。CPU视图始终只读，写锁拒绝。

r4库 b035a887832513f977d8857c4224419b409ee04b64cf90d69ad4a5adba4ce117 与v728内核通过新库CPU37.125秒/GPU38.25秒完整回程，原完整共享/生命周期全部通过。真实UI400×400创建/上传恢复，本靴0个VMError和MEMREF错误；AegirPoster仍6次退出，真实锁屏时间/日期/壁纸未恢复，完整目标仍active。

readonly_pixels.py独立验证每种offset、CPU保护/写锁拒绝、production复制/共享路径、两次精确哈希及原VM解除标记。新RAM库大小/FNV/SHA、完整HFS/blob字节和编译源/Mesa归档输入单独绑定；不能复用旧RAM库CPU结果。首次v728 owner缺一条STREAM的原日志保持，90秒采样时机对照的完整结果用于后续准入。

主机检查输入可从已应用增量的树重建：materialize_surface_candidate.py --version 728；v727检查在v726树上执行 --version 727，再安装。源码生成器仅用于生成固定增量，重新复现直接使用发布增量和此物化入口。所有真实失败与各阶段原日志保持不变。
