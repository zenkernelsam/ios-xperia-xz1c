# Metal 深度／模板候选 — 2026-11-20

> 历史技术记录：下文的版本、结果、“当前”和“下一步”对应原实验日期。现行产物及设备状态见[STATUS](STATUS.md)，构建与单发规则见[BUILD](BUILD.md)／[OPERATIONS](OPERATIONS.md)。原文与SHA见[本轮归档](archive/20261123/poster-state-r22-close-out/original-document-manifest.json)。

最新tiled GLES c617f2ee已通过自身CPU／GLES9，reference CPU b2f3fc4f因独立启动日志字节不一致拒绝，native21未投递；本页此前离线及失败记录保持，准入以[STATUS](STATUS.md)为准。详见[ATTACHMENT-HARDWARE](ATTACHMENT-HARDWARE.md)。

2026-11-20恢复新蓝灯会话后的最新实际验收与12个失败页面见[ATTACHMENT-HARDWARE](ATTACHMENT-HARDWARE.md)。当前GLES21de自身CPU／GLES9已通过；完整Metal21的深度／模板／Memoryless／MSAA首阶段原字节失败，原presentation／PNG未执行，不能提升此页旧离线结果为整组通过。全部0x06回程／健康／worker退出完整，失败包保留不重发。

此专题保留单采样候选的原结果和适用边界；当前源码已扩展[MSAA附件r1](METAL-MULTISAMPLE.md)，旧native18和此页旧二进制不能准入当前MSAA库，完整新门槛为native20／PNG6。新源码不是此页旧候选的同字节重建。

现行实机基线仍是v730／GLES CA矩阵r1／Metal5e94ab57。正式源码在Depth32候选上增加Stencil8附件，两个原页面、状态快照、GPU独立清除和引用回收已接入，离线构建与主机检查通过。新候选没有投递手机，仍须自己的CPU／GLES9／referenceCPU／native18／PNG6证明；黑壁纸、完整动态UI／FPS／睡眠和其它外设没有因此完成。

## 原页面和开源契约

Depth32Float／252和Stencil8／253均只支持2D、单sample、单level、RenderTarget和完整原buffer范围；offset／stride须64B对齐。颜色／旧Depth32 ABI保持，新增独立`MetalStencilAttachment.h` C ABI。encoder保留原纹理和不可变front／back状态到命令执行，检查owner、格式、尺寸、原范围、颜色／深度／模板重叠；调用者先释放后仍可执行，最后原分配回收。

Depth-stencil descriptor语义对照[开源ios_sagit QcomMTL](https://github.com/MC-XiaoXiao/ios_sagit/blob/898998cf1b74ea103e0c16d6dc0016822d8d644b/drivers/gpu/QcomMTL/QcomMTL.mm)，BSD-2-Clause来源保留。八种compare、八种stencil operation、参考值和掩码按实际Metal16 ABI映射到GLES。nil state恢复Always／不写深度，参考值保持；Apple backFaceStencil=nil后的实际getter给默认Always／Keep，已按原对象验证。

固定[Mesa24.3.4](https://gitlab.freedesktop.org/mesa/mesa/-/tree/769e51468b49b2a42f0a0eaf71cf9eed5ff4e5de) Freedreno原`u_transfer_helper`采用separate-z32s8：primary对外是Z32_FLOAT_S8X24，对内仍是原Z32_FLOAT布局，secondary保留原S8_UINT。新`StencilResourcePair.h`在screen／格式／尺寸／sample等全部一致后仅转交secondary的一个引用，primary销毁时由原Mesa helper回收S8，绝不把Depth32改成8B交错存储。实际固定源码`fd5_gmem.c`也分别编程depth和stencil地址／pitch；主机15类回滚模型核对错误时没有修改或丢失原引用。

仅有stencil附件时，Mesa需要一个primary depth布局，候选建立私有未使用Depth32平面。实际深度测试和写入关闭，caller没有depth页就不读写任何caller深度；模板仍用原S8页面。Depth／Stencil独立Load／Clear，使用实际GPU draw和完整像素scissor，避免fast-clear污染caller padding。stencil clear禁用depth test／write，depth clear禁用stencil，颜色Load与另外一层clear可分别执行。

当前仍拒绝MSAA、sampled-depth／sampled-stencil、mipmap／IOSurface depth-stencil和depth-stencil与XR有序image blend组合。后者的late depth test无法保护fragment image副作用，不能静默忽略深度或模板。kernel GPU底层／allocator／SCM／PON／RPC／VM、SweetGPU、Mesa归档和AIR lowering源保持，只编译正式仓库用户态。

随后标准[Memoryless临时附件](METAL-MEMORYLESS.md)已接入单采样颜色／Depth32／Stencil8，公开纹理无backing、每encoder独立私有页、非法Load／Store与CPU-sampling拒绝、完成／失败回收；原生host／ARM64编译与Mesa读回通过，Metalf4aa2b35／referenceb083c28f只offline。新能力须完整native19原证据，旧r2 native18候选及本专题原字节身份保留；Memoryless和完整原壁纸实机仍未验收。

本轮后续只读20H392原ClockKitUI确认：options0x08开启4×MSAA、0x01开启depth，深度descriptor为252／RenderTarget4／Memoryless3。原r3十条init皆0x72，当前scope未请求MSAA／depth，不将这些缺口当该黑壁纸原因；其它配置真实Memoryless和MSAA须继续实现。来源／九个代码SHA与可复现工具见[原模式静态证明](../reproducibility/poster-routing-20H392-offline.json)。

## 离线身份与证据

| 输入 | 准确身份 |
| --- | --- |
| 已验v730 kernel | `f258dee9c6d142b90a573f796eeec5e92b39dd65feed6a21b2542d90fb66a88d` |
| 候选打包签名GLES | `6211bac248655bad1f1fb3c1386e68037160f8f78bd264893030ad69e60bd5df`，14783008B |
| 候选Metal | `f40f796b7d04d143685a7ac0e9414cfed2aae68d0184249ebaf3cd09c286ec53`，787648B |
| 候选Metal source-map | `b86e29eb0c15237abc95b5ea31e976569ec47fc13513883184e66b3cf47b09d0` |
| reference observer | `7e164ad1c0e54efa6f17f461e604b3554642d1d8c8c9b033b3d30070285c7e9d`，220336B |
| GLES RAM blob | 3899356B／`36d0aeb3b6d466fe69e726596a162c67bf45f6362631573e33ce450cb9cdc918` |
| reference RAM blob | 4274561B／`1c13889d220e5a5f7aa75b4bae3d83ba51e3515cc456dad6ab307707f147978b` |

Mac前端原产物SHA51770f95与打包后GLES6211bac2不同，来自install_name／重签，不能互换准入。Metal准确绑定6211bac2；RAM只读HFS、别名、全部库字节／blob／来源锁已复算。新CPU bundle token`3ec7416adc2cfeea`／image`5567d6561e517b8270c180ccc3a64d1105e93da57ae8179c6679946a7a170fde`完成offline同核private／payload／Mac83实际源audit，发送次数0。

最新[offline proof](../reproducibility/stencil-attachment-r2-offline-proof.json)保留全部身份与验收边界。旧[Depth32 proof](../reproducibility/depth-stencil-r1-offline-proof.json)及[旧专题原文](archive/20261120/stencil-attachment-r2/METAL-DEPTH-STENCIL.md)原样保留，不能把旧库native17或ASan结果准入本轮。

WSL真实Mesa23.2.1／llvmpipe GLES3.2的UBSan执行2390draw、247860像素，覆盖8compare／8operation、front-back、三条操作分支、clamp-wrap、read-write masks、scissor／输出guard和GPU depth-stencil清除；pair helper15类回滚／引用生命周期模型通过。它没有执行FD540 importer，不代表本机原页GPU像素。

本轮WSL ASan测试停在libasan6 startup，实际PC模块offset0xcdf02／`__sanitizer::internal_iserror`，尚未进入Mesa，原失败单列，最终GLES不宣称ASan通过。旧未限长输出产生11,881,859,070B重复DEADLYSIGNAL日志，已实际进入Windows回收站，SHA55931387、首4096B与失败元数据保留，回收站未清空。runner现在限45秒、Python等待55秒、Linux文件1MiB，RLIMIT覆盖shell，实际超量拒绝；ASAN handle_segv=0只停递归handler，不关闭memory checks。

Mac真实Metal对象ASan／UBSan通过：原资源／queue／AIR compute-render-constants-MRT回归、Depth32、Stencil8页面和guard、owner、Private权限、descriptor状态、独立Load-Clear、immutable快照、caller释放及最终回收。GPU传输是host stub，不能称手机绘制。iOS ARM64前端／Metal／完整observer均构建签名成功，源码与候选manifest逐项匹配。

原模式observer源码扩展后，在当前源码重新构建完整attachment reference仍产生observer7e164ad1同字节。新HFS重新打包有自己的raw／blob身份：4274554B、SHA90dc4934c2555bdfd15723cc404e5bde495b114ccbd9631ffdd34ef173877f0e；CPU c10521e2f389104f／image67656595b86727101cd4549007fb8c9e5a7cc6e9a6585b47af0f8f91fbd558f3已offline同核private／payload／Mac83源通过。r2原包和proof保持；新reference与原壁纸包仍须各自CPU和完整native18／PNG6门槛，见[后续offline](../reproducibility/poster-route-r6-offline-proof.json)。

## 可复现构建

输出目录必须新建。Windows／WSL使用最终有界runner：

```powershell
python -B tools/build/check_depth_stencil_gles.py --sanitizers undefined --output build/my-stencil/host-gles
```

要测试ASan时明确选择`--sanitizers address,undefined`，失败保留实际输出，不能替代成PASS。Mac前端、原生对象和库：

```sh
python3 -B tools/build/check_metal_compute.py --output build/my-stencil/host-native --llvm-headers deps/llvm-c-14
python3 -B tools/build/build_frontend_trial.py --mesa-build build/mesa-ir3-cache-r1 --output build/my-stencil/frontend
python3 -B tools/build/package_glram.py --frontend build/my-stencil/frontend --output build/my-stencil/gles-ram
python3 -B tools/build/build_metal_library.py --ram-package build/my-stencil/gles-ram --air-package build/static-initializer-r1/frontend --output build/my-stencil/library
```

完整固定Mesa／AIR输入从[BUILD](BUILD.md)、[IR3 cache](IR3-VARIANT-CACHE.md)和对应source-map重建，重新编译的SHA需自己的实机准入。完整observer命令：

```sh
python3 -B tools/build/build_exception_observer.py --output build/my-stencil/observer \
  --metal-resources build/my-stencil/library --target-process backboardd \
  --gles-ram-package build/my-stencil/gles-ram \
  --air-fixture build/air-r1/fixture --render-fixture build/render-r1/fixture \
  --constants-fixture build/constants-r1/fixture --xr-probe --surface-probe --mip-probe \
  --narrow-fixture build/air-conversion-r1/narrow-fixture --half-fixture build/air-half-r1/fixture \
  --mrt-fixture build/metal-mrt-r2/fixture-final --sampler-fixture build/link-cache-r1/sampler-fixture-lf \
  --static-half-fixture build/static-initializer-r1/fixture --clear-cache-probe --ir3-cache-probe \
  --depth32-probe --stencil8-probe --cif10-png-reference
python3 -B tools/build/package_glram.py --presigned --frontend build/my-stencil/gles-ram \
  --observer build/my-stencil/observer --metal-resources build/my-stencil/library \
  --output build/my-stencil/reference-ram
```

长探针只在backboardd、等待同进程GLES9后执行，原Aegir不注入长suite。打包保留准确GLES6211bac2对应字节；完整reference添加新Metal／observer。

## 原字节实机门槛

Depth32原29phase保留，74240B／38744Bguard／26100B读回。新增`MetalStencil8Probe.h`为99phase，17×9、color1280B／depth1280B／stencil768B、原offset64／pitch128-128-64，覆盖正反面比较、各操作三分支、参考值掩码、stencil-only和两个独立clear。所有329472B、193149Bguard与118800B getBytes读回、原handle／IOVA／offset、递增fence／最终allocated0必须本靴核验。

`stencil8_evidence.py`独立重算原BGR／float32深度／S8字节，不接受缺阶段、错mask／字节、外来页面、错误fence或未回收；ACK后记录不能补齐ACK前缺失。新库须自身RAMCPU→完整GLES9→新referenceCPU→完整native18／PNG6→新native-proof→原Aegir observer自身CPU→原GPU／150秒scanout。native16／17旧proof明确拒绝Stencil8库。

r4／r5失败仍保留，新的健康物理fastboot会话尚待确认，枚举健康不能推定用户已恢复，不换token重发旧失败。按[OPERATIONS](OPERATIONS.md)只Sony、单download／boot、PON0x06、四marker／健康30秒／旧worker退出。日常原UI用已验串口真实720×1280截图，摄像头只用于LCD光学／背光／错行。没有原环境／动画触发／pipeline证据仍保留STOP；完整壁纸／UI／FPS／睡眠和其它外设继续。
