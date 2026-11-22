# Metal Memoryless附件 — 2026-11-20

最新tiled GLES c617f2ee已通过自身CPU／GLES9，reference CPU b2f3fc4f因独立启动日志字节不一致拒绝，native21未投递；本页此前离线及失败记录保持，准入以[STATUS](STATUS.md)为准。详见[ATTACHMENT-HARDWARE](ATTACHMENT-HARDWARE.md)。

2026-11-20恢复新蓝灯会话后的最新实际验收与12个失败页面见[ATTACHMENT-HARDWARE](ATTACHMENT-HARDWARE.md)。当前GLES21de自身CPU／GLES9已通过；完整Metal21的深度／模板／Memoryless／MSAA首阶段原字节失败，原presentation／PNG未执行，不能提升此页旧离线结果为整组通过。全部0x06回程／健康／worker退出完整，失败包保留不重发。

此专题保留单采样候选的原结果和适用边界；当前源码已扩展[MSAA附件r1](METAL-MULTISAMPLE.md)，旧native19和此页旧二进制不能准入当前MSAA库，完整新门槛为native20／PNG6。新源码不是此页旧候选的同字节重建。

正式源码已接入单采样Memoryless颜色／Depth32Float／Stencil8临时附件：公开纹理只保存原descriptor元数据，实际页面在render encoder创建时独立分配，所有本pass的draw／clear保留该页，完成或失败后回收。当前仅离线构建与主机验收，手机未投递，原壁纸、UI／FPS／睡眠／光学和其它外设仍未完成；已验CA基线保持。

## 原请求与开源来源

同版20H392 ClockKitUI在depth选项开启时申请Depth32Float／252、RenderTarget／4、Memoryless／3，来源与9个代码SHA在[原模式静态证明](../reproducibility/poster-routing-20H392-offline.json)。原r3十条init的0x72没有depth／MSAA bit，Memoryless不是该样本黑壁纸的已证原因；它是其它原配置确实需要的API缺口。

语义参考[开源ios_sagit QcomMTL](https://github.com/MC-XiaoXiao/ios_sagit/blob/898998cf1b74ea103e0c16d6dc0016822d8d644b/drivers/gpu/QcomMTL/QcomMTL.mm)，BSD-2-Clause。上游通过私有存储实现A540临时附件；本项目使用已有SweetScatter原分配／Mesa原页导入，每encoder独立存储，避免并发queue共享内容。未改kernel／allocator／SCM／PON／RPC／VM、Mesa归档或AIR lowering。

## 实际语义和边界

`MetalMemoryless.inc`在原descriptor全部符合后复制它，保留公开纹理和本pass实际私有纹理的关联。仅2D、单sample、单level、RenderTarget，尺寸／格式／资源选项检查保持；CPU `getBytes`／`replaceRegion`、sampling、mipmap、IOSurface、跨pass Load和persistent Store拒绝，公有`buffer`和原native binding不暴露页面。

私有页用原64B row stride和offset64，前后各64B；不同encoder即使共用同一公开纹理也获得独立page／handle／IOVA，同encoder多draw保持同页、第一次原Clear后后续Load。原descriptor不被改写；immutable command保留原public-private对应关系，执行时重新核对owner／slot／format／尺寸及DontCare store。页面保留到原GPU fence，partial allocation失败、正常完成和GPU错误均回收。

标准Metal getter实际叫`isFramebufferOnly`，首轮主机异常抓到旧纹理只实现`framebufferOnly`，已补标准getter，旧失败日志保存。新`SweetMTLRetainMemorylessPassBuffer`只租借已经存在的作者probe内部页，不分配或制造像素；它不是公开Memoryless内容读取API。probe租约可在原fence后直接比较完整原buffer和padding，再释放；生产原UI不用该诊断接口。

仍缺MSAA／resolve、sampled depth-stencil、mip／surface-depth和depth-stencil与XR有序image blend正确组合。Memoryless只在这轮界定的单采样通道接入，不宣称片上GMEM合并、性能改善或完全原生Apple GPU族支持。

## 精确离线候选

| 输入 | 身份 |
| --- | --- |
| 已验内核v730 | `f258dee9c6d142b90a573f796eeec5e92b39dd65feed6a21b2542d90fb66a88d` |
| GLES沿Stencil8r2保持 | `6211bac248655bad1f1fb3c1386e68037160f8f78bd264893030ad69e60bd5df`，14783008B |
| 新Metal | `f4aa2b3546f5fe83bc4ab5e40887848c1c9cb078a93eb6b5d16afa4ada33b95d`，789024B |
| Metal source-map | `314789651a86509927834b1a94a05ee91e1d44782272a06aed6864b7b007fdad` |
| 完整reference observer | `b083c28f41d02733c2417f06c2bb29482e7a138595bc0fb0b045398f82984c4c`，237216B |
| RAM blob | 4280557B／`692b62e97d2eecc9b8da354894e74eb660be4d9bdb81fd4f39a4b02731a835ed` |
| CPU bundle | token`a6ed4392a66f7af4`／image`c1539667793c580f88c6292de9894ff0bbd611d8962ffae1560a625726b8483c` |

Mac完整真实Metal compute／render／constants／MRT／depth／stencil旧回归以及新增public metadata／snapshot、实际双queue提交／不同页面／partial allocation／GPU error回收ASan／UBSan通过；GPU传输是host stub，不能称手机像素。WSL真实Mesa八stage、19draw／10404B颜色与depth float32／stencil原值GPU读回UBSan通过，host没有执行FD540 importer。原host getter异常单独保留；最终源码、input hash和[offline proof](../reproducibility/memoryless-attachment-r1-offline-proof.json)绑定。

iOS ARM64 Metal／完整observer构建签名通过，RAM32MiB只读HFS、全部库别名／SHA、完整blob字节核验。新CPU bundle仅offline同核private截图／payload／Mac83实际源audit通过，下载／boot0。旧候选和native18 proof保留，不能用于新native19准入。

## 从正式源码构建

所有输出目录新建，固定Mesa／AIR准备见[BUILD](BUILD.md)和[深度模板](METAL-DEPTH-STENCIL.md)。Windows／WSL原像素配方：

```powershell
python -B tools/build/check_memoryless_gles.py --output build/my-memoryless/host-gles
python -B -m unittest discover -s tools/device -p test_memoryless_evidence.py -v
```

主机只使用有界UBSan，不复用上轮ASan启动失败为本轮通过；host时间45秒、日志1MiB。Mac对象／库：

```sh
python3 -B tools/build/check_metal_compute.py --output build/my-memoryless/host-native --llvm-headers deps/llvm-c-14
python3 -B tools/build/build_metal_library.py --ram-package build/stencil-attachment-r2/gles-ram \
  --air-package build/static-initializer-r1/frontend --output build/my-memoryless/library
```

完整reference沿native18全部fixtures，加`--memoryless-probe`；即`--target-process backboardd --depth32-probe --stencil8-probe --memoryless-probe --cif10-png-reference`及原compute／render／constants／narrow／half／MRT／sampler／static-half／Clear-cache／IR3-cache选项。它等待同process GLES9后执行，原Aegir不注入长suite。`package_glram.py --presigned --frontend ... --observer ... --metal-resources ... --output ...`保持精确GLES，生成自己的新RAM包，具体完整fixture命令见深度模板专题。

## 完整实机准入

新`MetalMemorylessProbe.h`八stage，原17×9、color1280B／depth1280B／stencil704B，11张实际pass页、19456B／10888Bguard。覆盖depth写入后同pass拒绝、stencil-only更新、depth-fail stencil操作、临时color draw／clear、同public texture双queue不同page。原自定义租约、输入SHA、source handle／IOVA／offset、每draw生产import-retire、fence及final allocated0均独立核验，`memoryless_evidence.py`只从当前LIVE到ACK的原raw验证，缺项／错页／错值／ACK后補记录拒绝。

同kernel private／owner／ready→精确新GLES9→本RAM自身CPU→完整native19／PNG6→新native-proof／PNG-proof→原模式observer自身CPU→原GPU／150秒scanout。`native_memoryless_attachments`强制`Metal_memoryless`，旧native18哪怕全通过也拒绝；host或offline bundle不代替手机门槛。

手机当前fastboot、无controller／worker，r4／r5失败后的新物理会话确认尚未到，本轮未发设备。按[OPERATIONS](OPERATIONS.md)只Sony、单download／boot、PON0x06、四marker／健康30秒／旧worker退出；日常UI用串口PNG，LCD光学另验。完整壁纸／动态UI／FPS／睡眠和其它外设继续。
