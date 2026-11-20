# 原生 Metal 呈现候选 — 2026-11-20

最新tiled GLES c617f2ee已通过自身CPU／GLES9，reference CPU b2f3fc4f因独立启动日志字节不一致拒绝，native21未投递；本页此前离线及失败记录保持，准入以[STATUS](STATUS.md)为准。详见[ATTACHMENT-HARDWARE](ATTACHMENT-HARDWARE.md)。

2026-11-20恢复新蓝灯会话后的最新实际验收与12个失败页面见[ATTACHMENT-HARDWARE](ATTACHMENT-HARDWARE.md)。当前GLES21de自身CPU／GLES9已通过；完整Metal21的深度／模板／Memoryless／MSAA首阶段原字节失败，原presentation／PNG未执行，不能提升此页旧离线结果为整组通过。全部0x06回程／健康／worker退出完整，失败包保留不重发。

正式源码新增原`CAMetalDrawable`的呈现交接和drawable纹理标志，补上`presentDrawable:`、指定时间、最短间隔及原options四条命令路径。绘制／resolve原IOSurface页面在实际GPU退休后通过原lock／seed发布，随后才发布Scheduled并允许系统呈现。原生对象ASan／UBSan、旧单采样及MSAA回归、iOS ARM64库／完整探针构建通过；新CPU包只offline审计通过，没有投递手机。当前UI／光学和完整native21实机验收仍缺，已验基线仍是v730／CA矩阵r1。

## 本机原调用与开源来源

对象／呈现语义参考[ios_sagit QcomMTL](https://github.com/MC-XiaoXiao/ios_sagit/blob/898998cf1b74ea103e0c16d6dc0016822d8d644b/drivers/gpu/QcomMTL/QcomMTL.mm)，BSD-2-Clause。其iOS18 ABI没有直接套用；本机iOS16.7.16／20H392的Metal、QuartzCore UUID、20个方法type／IMP和8处原调用byte SHA重新核对，原Apple二进制／切片留在本地build。

| 本机身份 | 精确值 |
| --- | --- |
| QuartzCore UUID | `9e40b2eb260c3af1ae45a1a551a8c1b9` |
| Metal UUID | `0f0b7b9690bf3a07b7e185e2fb6160e6` |
| 固定A2S | `5bf04681af1819e2379319942a97be635100d6ca3844c5a22e372ce78a83b83d` |
| 原CAMetalLayer纹理分配 | `0x1884bc2cc`；原setRotation／setIsDrawable／setFramebufferOnly／setUsage→newTextureWithDescriptor:iosurface:plane: |
| ClockKitUI普通呈现 | `0x1c1737740`；presentDrawable→commit |
| ClockKitUI事务呈现 | `0x1c17379d4`；waitUntilScheduled→原drawable present |

原descriptor getter的BOOL为`B`，rotation／protection为`Q`，swizzle为四个`C`；`presentAtTime:`和`presentAfterMinimumDuration:`为`double`。原options字典键为`presentationMode`／`presentTimeInterval`，mode0／1／2分别调用present／presentAtTime／presentAfterMinimumDuration。options按原字典copy后转发，不重写键或时间。

旧queue在执行operation列表之前发布Scheduled，事务呈现的原等待者因此可能拿到尚未写好的IOSurface；当前同步backend先执行并等待所有真实GPU操作及页同步／seed发布，才发布Scheduled和scheduled handlers。该调整只改变用户态command生命周期，原kernel／allocator／SCM／PON／RPC／VM、SweetGPU和Mesa归档不变。它仍使用保守串行退休，没有宣称异步重叠或FPS改善。

## 原页面、标志与交接

`NativePresentation16.h`只声明原系统selector，使用实际runtime method return／argument ABI核对。`MetalRender.inc`保存descriptor的`isDrawable`与`framebufferOnly`，不将普通Surface标成drawable。零rotation、未保护、identity swizzle、单采样／单mip线性RGBA／BGRA8（含sRGB）和匹配IOSurface FourCC才接受原drawable纹理。像素布局、logical origin及已有selector12导入保持；多plane／压缩／未知契约明确拒绝。

framebuffer-only纹理保留原usage，拒绝CPU getBytes／replaceRegion／buffer暴露和mipmap；sampling按原usage核对，参考原Metal把usage和framebufferOnly分开保存。RenderTarget仍可绘制、可作resolve目标。encoder／command保留原页，完成后按实际owner回收。

`MetalPresentation.inc`保留原drawable及其原纹理、不可变binding／时间／options。记录时允许Created或已enqueue未commit的command；重复drawable、错owner／ABI／类型、非法时间或晚提交拒绝。getter在condition锁外执行，再重新检查录制状态。queue执行所有operation后才交接，不额外生成GPU标记或显示完成回调；draw失败不调用present。原present抛异常或纹理改变则command保持Error，原引用在收尾回收。

渲染和resolve返回前对写入Surface沿原GPU synchronize→IOSurface lock/unlock发布内容和seed。该步骤也覆盖没有`presentDrawable`而在waitUntilScheduled后自行呈现的原路径。seed、queue Completed和GPU fence分别只是内容／执行证据，LCD真正扫描、backlight／光学色偏与原UI仍需独立验收。没有调用`didPresentAtTime:`或制造presentedTime。

## 实际离线结果与候选

| 输入 | 精确身份 |
| --- | --- |
| 同核v730 | `f258dee9c6d142b90a573f796eeec5e92b39dd65feed6a21b2542d90fb66a88d`／20665112B |
| GLES保持MSAA候选 | `21de6821910c452f1bda70720f3573b81ac76483a10f1959ef95e7b0bc9447b4`／14784032B |
| 新Metal | `8feea4593116ffe9187204f5dd4cf4dd48d3027816d4763ecef20da200f86a27`／810304B |
| 新Metal source-map | `db1605ef8dae8f8ed0a29999dcb2cf7537457aba7ad428677c21e0e914771b78` |
| 完整reference observer | `33bf5ee4988352c747bb9309b2aa7e790caf13da3b801a81d263cb20d37de4a4`／256656B |
| RAM blob | `7a3361f68dc15c12d274ef0f158766aa6ec0b684ccf036b52172046717ac57d8`／4304475B |
| 新CPU包 | token`367bebc61fc1866b`／image`177af34ae64c230121cfc6b44d228b5cac620a22ded682dd950c83a223cc85b1` |

Mac实际native对象、原queue FIFO／capacity／callbacks／error、四present精确time／options、original Surface持有、Scheduled后真实seed、事务等待路径、duplicate／错ABI／旋转保护／native异常和changed-texture回收ASan／UBSan通过；drawable本身是明确host fixture，不能称手机原生呈现通过。原MSAA作者18阶段回归通过；GLES保持上一版字节。首个host编译因block直接引用C数组失败，原日志保留，改用原数组指针后通过。

新完整RAM只读32MiB HFS、m／g／e硬链接与全部库／blob复算通过；CPU包offline同核private真实截图先验、完整payload及Mac83源audit通过，download／boot／reset0。旧native20的合成host拒绝测试确认新库必须native21；没有伪造新的hardware proof。完整[offline证明](../reproducibility/presentation-r1-offline-proof.json)保存实际输入与边界。

## 正式复现与未来准入

在正式Mac仓库，所有输出目录必须新建，固定Mesa／AIR构建见[BUILD](BUILD.md)和[MSAA](METAL-MULTISAMPLE.md)。保持`~/xnu-autobuild.pause`，原源／库先验和旧失败不覆盖。

```sh
python3 -B tools/build/check_metal_compute.py --llvm-headers deps/llvm-c-14 --output build/my-present/host-native
python3 -B tools/build/build_metal_library.py --ram-package build/multisample-backend-r1/gles-ram-r2 \
  --air-package build/static-initializer-r1/frontend --output build/my-present/library
```

新完整observer使用MSAA专题的全部native20／PNG6 fixture和选项，另加`--presentation-probe`。`package_glram.py --presigned --frontend <exact GLES package> --observer <new full observer> --metal-resources <new library> --output ...`生成自己的新RAM包。首次pull先按正式源重建GLES／AIR和作者fixture，实际路径按输出传参；以上旧build路径只是本轮已有输入的精确复现，新SHA需重新验收。

静态原ABI研究工具只读取已挂载的只读准确gl5：

```sh
python3 -B tools/build/inspect_native_presentation.py --root build/my-readonly-root/root \
  --symbols assets/symbols/ios16-20H392-root.a2s --ipsw /path/to/verified/ipsw --output build/my-present/static-ABI
```

它校验原cache／image／A2S／tool、method-list type／IMP及精确调用锚点，生成metadata／SHA报告，原字节不进Git。本轮复用回收站中已核raw只读挂载，完成后卸载，原gl5 sparse与raw垃圾桶条目保持。

手机`MetalPresentationProbe.h`使用真实原CAMetalLayer获取五个原drawable，核对QuartzCore UUID、SweetMTLTexture原页面／owner／FourCC／pitch／CPU framebuffer拒绝。作者初始化padding为0xa5后GPU Clear＋scissor AIR draw，独立验765像素、padding／raw／pixels hash、实际import-retire-publish与sequence；四命令present和原transaction wait→present分别验证，记录原presentedTime位型而不强制非零。探针未执行手机，无host替身替代原对象；日常原UI仍用v730串口720×1280 PNG独立验收。

严格顺序为同核private／owner／ready→精确GLES自身CPU／GLES9→本reference CPU→完整native21／PNG6→新native／PNG proof→原poster CPU／GPU与150秒scanout。`native_drawable_presentation`强制`Metal_presentation`，旧20组拒绝。探针错误只在本LIVE后按现有PON0x06回程并保留REVISE，低层发送器无修改。用户目前明确要求继续离线，r4／r5失败后未确认新物理fastboot会话，本轮不操作手机。

七份旧手册原字节和SHA在[归档](archive/20261120/presentation-r1/original-documents.json)，原黑壁纸scope没有display／pipeline请求仍保持STOP。呈现是后续真实显示所需的缺口，不把它当作已证黑壁纸原因；完整壁纸／UI／FPS、睡眠、LCD光学及其它外设保持active。

本轮实际回收Windows7项／12文件／34820404B、Mac21项／432文件／300997854B；全部原生回收核验，未清空。候选CPU包、raw／blob／库、原失败和20H392 ABI证据保留，两Ubuntu垃圾桶为空。[回收记录](../reproducibility/presentation-r1-native-recycle.json)。
