# Metal 深度／模板候选 — 2026-11-20

现行实机基线仍为v730／GLES CA矩阵r1／Metal5e94ab57。本轮离线补齐了原生Depth32Float附件、深度状态、原页面导入、GPU清深度与命令快照；候选没有投递手机，尚未获得自身CPU／GLES／native17／PNG6准入。模板状态与GLES映射已实现及离线验证，模板附件、MSAA、深度与XR有序混色组合仍未接入。这些改动不代表黑壁纸或其它外设完成。

## 改动与开源来源

原`SweetMTLRenderPipeline`和encoder拒绝全部depth／stencil附件，`SweetMTLDevice`也没有原生depth-stencil state对象。固定Mesa24.3.4／FD540已经有`PIPE_FORMAT_Z32_FLOAT`深度测试，外部纹理helper`st_context_teximage`却按颜色格式描述导入资源。候选按准确Depth32Float／252、2D／单sample／单level／RenderTarget、64B stride及完整原buffer范围，导入原pipe_resource后补真实深度texture image格式，保持原GPU页面和同步／fence通道。

`MetalDepthStencilState.inc`的descriptor semantics对照[开源ios_sagit QcomMTL](https://github.com/MC-XiaoXiao/ios_sagit/blob/898998cf1b74ea103e0c16d6dc0016822d8d644b/drivers/gpu/QcomMTL/QcomMTL.mm)，BSD-2-Clause来源保留。按本机Metal16 ABI验证八种compare值，复制原front／back descriptor及掩码，保留label／device；nil state恢复默认Always／不写深度并保留stencil参考值。Apple descriptor设置backFaceStencil=nil后getter仍提供默认Always／Keep描述符，测试核对实际getter而不是推定缺face。

新增`MetalDepthAttachment.h`提供独立C ABI，旧`SweetRenderPipelineConfig`／`SweetRenderPass`／`SweetRenderDraw`结构和程序cache记录尺寸保持。原native encoder把depth texture与不可变状态快照保留到命令执行，检查device／格式／范围及颜色附件重叠，调用者先释放仍可执行。Depth-only clear可在颜色附件Load时执行；GPU shader写gl_FragDepth并用完整原像素scissor，避免FD5 fast-clear向caller padding写入。真正FD540写入及padding仍需手机证明。

模板附件继续拒绝，Depth32仅RenderTarget；mipmap／sampled-depth／surface-depth尚未实现。XR有序image blend与深度组合继续拒绝，因为late depth test不能保护已有fragment image写入，不能在组合路径静默忽略深度。kernel GPU底层／allocator／SCM／PON／RPC／VM、SweetGPU、Mesa归档及AIR lowering源保持；只编译正式仓库用户态候选。

## 离线证据和准确身份

| 输入 | 准确身份 |
| --- | --- |
| 已验内核v730 | `f258dee9c6d142b90a573f796eeec5e92b39dd65feed6a21b2542d90fb66a88d` |
| 候选GLES／已打包签名 | `1720c1df2b0efeb40412508b94d54d7eb10847ef6b7f5aa560d69520e9718184`，14766032B |
| 候选Metal | `d5c89a2c6adb4e07f62fee175aac8520cbb8a80a0b41b164bb23b25f17fef2b3`，786976B |
| 候选Metal source-map | `36e8fae9163c35ca09f83abcf3277ab7a53c43b53174e9ad53b84da7ed682774` |
| 完整reference observer | `97e03083e11caddd351ed4996d07807e49a3adab0e5f517ea90325ffa113911b` |
| 新reference RAM blob | 4267574B，原生HFS只读／库别名及完整像素源码锁已验 |

Mac前端原产物SHA6a27ca64与上述打包后GLES1720c1df不同，来自install_name／重签步骤，不能互换准入。实际Metal绑定打包后1720c1df。所有输入、源码SHA、主机结果见[offline-proof](../reproducibility/depth-stencil-r1-offline-proof.json)。

WSL实际Mesa23.2.1／llvmpipe GLES3.2＋ASan／UBSan执行2382次draw、246636像素检查，覆盖八种depth compare／八种stencil operation、front／back、三条stencil操作分支、clamp／wrap边界、read／write masks、scissor与输出guard；额外验证真实GPU shader深度清除及缓存。它没有执行FD540原page importer，不能称为手机GPU像素。

Mac真实Metal资源／queue／AIR compute-render-constants-MRT回归与Depth32对象ASan／UBSan通过；新对象验证原descriptor／label／owner、原depth buffer范围／row padding、Private CPU拒绝、unsupported capability回滚、immutable状态／attachment快照、caller释放及最终回收。GPU传输是host stub，单列原生对象验收，不冒充手机深度绘制。首次host断言因错误假设backFaceStencil=nil而拒绝，原失败与getter原测试保留；修正测试后完整通过。

Mac实际用户态前端、native Metal和带原native16／PNG6＋depth29新probe的observer均iOS ARM64编译／签名成功；新RAM包完整byte／SHA／只读HFS核验通过。Windows新CPU bundle token`e57b450de07ac3fe`、image`295d83dc56e16588f307fc215c161051875148087f8287b16edb2b88bf4c0006`完成offline run_trial检查和Mac83源audit，尚未投递。既有Sony仍fastboot，r4／r5拒绝包不重发。

## 构建与完整实机准入

全部输出目录必须新建。离线host检查：

```powershell
python -B tools/build/check_depth_stencil_gles.py --output build/my-depth/host-gles
```

Mac原生host及前端构建：

```sh
python3 -B tools/build/check_metal_compute.py --output build/my-depth/host-native --llvm-headers deps/llvm-c-14
python3 -B tools/build/build_frontend_trial.py --mesa-build build/mesa-ir3-cache-r1 --output build/my-depth/frontend
python3 -B tools/build/package_glram.py --frontend build/my-depth/frontend --output build/my-depth/gles-ram
python3 -B tools/build/build_metal_library.py --ram-package build/my-depth/gles-ram --air-package build/static-initializer-r1/frontend --output build/my-depth/library
```

固定AIR来源按[BUILD](BUILD.md)与对应source-map重建，不能以另一份产物冒充相同SHA。新reference observer沿现行完整native16全部fixtures／`--clear-cache-probe --ir3-cache-probe --cif10-png-reference`，增加`--depth32-probe`，只在backboardd并先等待同进程GLES9通过后执行，原Aegir不注入长探针。`package_glram --presigned --observer --metal-resources`保持已经验的准确新GLES。

`MetalDepth32Probe.h`实际用标准Metal库／position-color AIR、原17×9两张1280B buffer纹理、offset64／pitch128。29phase覆盖24种比较、Less写深度／Load后的后续比较、nil默认不写、Never拒绝及只有depth clear，全部74240B／38744B guard及26100B getBytes输出核验；输入SHA、原页面、IOVA、严格递增fence和最后allocated0绑定同靴。`depth32_evidence.py`独立重算每个原BGR像素与float32 depth字节，错块、错handle／offset、错fence、缺阶段／guard／回收都拒绝。

新GLES包CPU→完整GLES9→新reference RAMCPU→完整native17／PNG6→新native-proof→原Aegir observer自身CPU→原GPU／150秒scanout，严格顺序保持。新`native_depth32_attachment`必须`Metal_depth32`原raw证明，旧native16即使全通过也拒绝。host与offline bundle不替代这些准入；手机恢复到新的健康fastboot会话后才按OPERATIONS单发，四marker／0x06／健康30秒／worker退出仍强制。

后续完整方向仍包含模板附件与MSAA、深度texture sampling及XR正确组合、原环境／动画触发、完整壁纸／UI／FPS／睡眠／光学／声音无线和其它外设。
