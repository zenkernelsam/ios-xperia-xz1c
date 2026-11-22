# 原生显示比例与布局 — 2026-11-22

XZ1C原生**720×1280像素、360×640点、scale2**已在r14／r15同靴核对：ADT、MobileGestalt、UIScreen和真实面板扫描尺寸一致，Clock圆形／应用图标保持比例。r15已移除红色rdar状态栏占位；小组件标签重叠、safe-area完整配置、真人全屏触控与光学仍待验。当前设备和结果统一看[STATUS](STATUS.md)。

| 层 | 历史750×1624配置 | r14／r15实际native配置 |
| --- | --- | --- |
| 面板／stride | 720×1280／BGR888／2160B | 保持真实硬件扫描契约 |
| 渲染canvas | 750×1624 | 720×1280 |
| UIKit points／scale | 375×812／2 | 360×640／2 |
| MG width／height／scale | 750／1624／2 | 720／1280／2 |
| 最终X／Y尺寸变换 | 0.96／0.78818 | 1／1 |

原配置横向相对纵向扩大1.218倍。native像素921600比旧1218000少24.34%；这是尺寸与工作量计算，未实测对应FPS改善。较少的UIKit点要求原布局重排，不能只修最终scaler。

## 现行配置链

`tools/build/make_render_adt.py --profile xz1c-native-2x`接受三种一致输入：1125×2436／scale3、750×1624／scale2、720×1280／scale2。只转换chosen.display-scale、product.artwork-scale-factor、disp0.canvas-width／height四个CFData。恢复四属性后必须与整个原ADT逐字节相同；缺失、重复、混合及未知尺寸拒绝。

`tools/device/package_trial.py --display-profile xz1c-native-2x`在完整基线重建后转换ADT，强制sweetfb_native1／sf2x0，绑定输入／目标／converter／numeric parser SHA；`prove_payload`独立重建整个新image。现有sf2x1与native1冲突，不混用。原GPU／ADSP／RAM及其它ADT属性不随几何转换修改，内核不重编。

CPU命令区607B，GPU602B；CPU部分numeric-one压缩以准确PE parser等值解析，shim参数不压缩。原profile保持原字节；六类实际包故障已拒绝，见[打包离线证明](../reproducibility/native2x-package-r1-offline-20261122.json)与[最初ADT证明](../reproducibility/display-native2x-offline-20261121.json)。新RAM仍需自身CPU，不能沿旧RAM CPU proof。

## 状态栏适配与准确证据

精确360×640／native720×1280／scale2且原provider nil／Unsupported／Fallback时，`gpu/geocompat/geocompat.mm`返回准确20H392已有的`_UIStatusBarVisualProvider_LegacyPhone`。其它有效返回保持；旧375×812配置的Split1125适配保持原契约。四参数ObjC ABI及原info转发经host ASan／UBSan和真实ARM64核验，helper source／signed library由manifest绑定。

r15实际from=nil、to=LegacyPhone，串口真实scanout中“iPhone／时间／电池”平直状态栏显示，红色rdar占位消失。窗口0..7原safeAreaInsets均38／0／29／0；只读观察采用准确UIEdgeInsets64 ABI，没有修改其返回。Weather／Calendar标签与第一排图标重叠，下一步读取原SpringBoard布局和网格来源，不能靠任意机型或全局safe-area覆写猜修。

只读layout诊断在精确native配置、main queue、constructor后135秒执行一次。原frame／bounds和convertRect:toView:使用准确CGRect64 ABI；grid仅已知SBIconListView／GridLayout／Configuration的原无参getter。每次最多2048节点、384个view记录、8组grid，循环／未知ABI／异常／截断照实标记；不读标签文字、不改frame／safe area／行列数，不请求重排。host fixture只测ABI、原值、边界及循环拒绝，不能当手机布局证据。

[R14原阶段](../reproducibility/native2x-r14-stage-20261122.json)和[R15原阶段](../reproducibility/native2x-flat-r15-stage-20261122.json)保存各自token、image／raw／PNG SHA、MG／UIScreen、扫描寄存器／CRC／FNV及完整回程。r15扫描PNG是本靴150秒720×1280原图；GLES9作者probe的750×1624离屏canvas独立保留，不把两者混为一项。

## 复现与验收

按[BUILD](BUILD.md)建立自己的固定输入和新helper／RAM输出，CPU包和GPU包均使用`--display-profile xz1c-native-2x`，运行参数和先验见[OPERATIONS](OPERATIONS.md)。不得手工改artifact／plan、复发原失败image或覆盖原manifest／proof。

完整验收还需四角／文字／状态栏／homebar／锁屏／home布局一致、F12全屏映射与真人触控、LCD光学／背光及动态UI／FPS。已验F12 max719×1279和120秒一次合成上滑，但合成输入不能替代真人测试。单frame缺块不补发；截图不是背光或睡眠证明。

原GPU／allocator／SCM／PON／RPC／VM保持冻结；gl5及用户输入不刷写。原750×1624生产基线和旧helper保留，可按原manifest恢复，不能把native几何包覆盖成旧包或以恢复名义重发失败。

## r16原布局与标签bbox

同靴948节点／166相关view／3组grid完整，原root list frame360×640、其listSizeForIconSpacingCalculation320×693、6行4列、portrait insets57／19／155／19。iconSpacing返回-123／-123照实保留，不当实际间距；常规图标view的y205.667、280、354.333、429给出约74.333点行距。小组件label原框y195.167／h21，与下一排图标y205.667重叠10.5点；窗口坐标来自原convertRect:toView:nil，同靴scanout也有可见重叠。

配置来源、有效layoutMetrics和密度策略尚待核对，不能凭异常getter猜值或改全局safe area。完整[原阶段](../reproducibility/native-layout-r16-stage-20261122.json)绑定本token／原raw／helper；[decoder](../tools/device/geometry_layout_evidence.py)拒绝缺RECT、重复VIEW、错summary、foreign token及重复snapshot，原bytes不改。

## r17原生密度候选

从gl5原HFSX稀疏输入按范围读取，未展开16GiB raw、未挂载。27个原方法／ABI、factory及cache路径已核对；8个之前独立验证的原call范围SHA再次一致。原iconSpacing只是缓存字段读取，原layoutMetrics才获取有效布局对象。证据见[20H392原数据](../reproducibility/native-layout-20H392-static-r17.json)，源码见tools/build/inspect_native_layout.py和hfs_sparse_reader.py。

新候选仅在精确native屏幕和已验root配置signature，使用原setNumberOfPortraitRows:更改真实配置：以原image56、widget141、label-origin143.5／height21和原insets57／19／155／19计算最大可容行数，六行约-10.4点bbox间距，五行17.5点，要求至少8点。factory与cache两原入口保证原对象在调用者使用前一致；字号／图标／widget及原system分页继续使用。其它配置／屏幕不参与该profile。源码已准备，host与真实图标保留／layout／光学仍待验，不能据计算宣布修好。
