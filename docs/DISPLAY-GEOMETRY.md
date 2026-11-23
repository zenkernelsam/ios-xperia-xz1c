# 原生显示比例、布局与模型

XZ1C已在同靴核对720×1280像素／360×640点／scale2：ADT、MobileGestalt、UIScreen、原扫描与F12范围一致，Clock圆形／图标保持比例。平直LegacyPhone状态栏已验。R21原六行与拟合五行的全部38个应用、Dock顺序及Folder内容通过独立对照；标签原祖先与两处17.5点间距也通过。原R20 REVISE保留。精确设备、产物与结果只看[STATUS](STATUS.md)。

| 层 | 历史750×1624配置 | 原生配置 |
| --- | --- | --- |
| 面板／stride | 720×1280／BGR888／2160B | 原硬件扫描契约保持 |
| 渲染canvas | 750×1624 | 720×1280 |
| UIKit points／scale | 375×812／2 | 360×640／2 |
| MG width／height／scale | 750／1624／2 | 720／1280／2 |
| 最终X／Y尺寸变换 | 0.96／0.78818 | 1／1 |

原配置横向相对纵向扩大1.218倍。native像素921600比旧1218000少24.34%；这是几何和工作量计算，FPS尚需实测。

## 配置与原生状态栏

make_render_adt.py的xz1c-native-2x只转换chosen.display-scale、product.artwork-scale-factor、disp0.canvas-width／height四个CFData。接受1125×2436／scale3、750×1624／scale2或原生720×1280／scale2的一致输入；恢复四属性后与整份原ADT逐字节相同。缺失、重复、混合或未知尺寸拒绝。

package_trial.py绑定完整输入／目标／converter／PE parser SHA，并独立重建整个payload，使用native1／sf2x0。原GPU／ADSP／RAM及其它ADT属性保持；shim参数不压缩。R21三个包命令区均607B，R22原GPU604B；限定同v730实际consumer／parser的默认0省略保持sf2x0语义，具体以各artifact为准。新RAM先自身CPU，内核保持v730。

精确native屏幕的原provider为nil／Unsupported／Fallback时，geocompat返回准确20H392已有的_UIStatusBarVisualProvider_LegacyPhone，原info及其它有效返回保持。四参数ObjC转发、opaque返回、errno经host ASan／UBSan及ARM64核验。safeAreaInsets只读，未全局覆写；完整safe-area仍需验收。

## 五行密度与完整模型

原布局配置listSizeForIconSpacingCalculation320×693／六行四列和360×640窗口不一致，R16原view与扫描图有10.5点标签重叠。固定原factory／cache两入口只在精确native root signature使用原setNumberOfPortraitRows:，按原图标、widget、label和insets计算五行；字号、图标及原system分页保持。其它屏幕和非root配置不参与。

revision3 helper由GEO_GRID控制：original0保留原六行配置，fitted1应用五行。package_trial.py --native-grid-mode original|fitted绑定完整SpringBoard环境、control源／值、plan和实际payload；mode marker缺失或不符拒绝。构建和参数见[BUILD](BUILD.md)，投递和原基线准入见[OPERATIONS](OPERATIONS.md)。

constructor后120秒仅一次main-queue只读捕获；VIEW／RECT成对，原convertRect:toView:nil取得窗口坐标，C LogWriteAll完整写入并保留errno。最多2048节点、384个view、8组grid、32个model和8层folder；REFUSE、截断、写失败、缺slot／LINK／child照实拒绝。原Root、Dock和Folder.lists／uniqueIdentifier的准确ABI已核对，不读取标签文字或个人内容。

完整模型比较要求同kernel、同精确helper、original0与fitted1。Root允许原系统分页重排；完整应用multiset、Dock顺序、Folder身份和各list内容须保持。未创建view的页面按模型核对，静态DefaultIconState或旧第一页15个应用不足以充当本机全基线。

## 标签归属卡点

revision4候选在原只读遍历中发出每个节点的TREE父边、depth和原Class层级角色，再发每个label的最近icon／list、hops及完整状态。独立decoder验证全部父边、view对应关系及计数，并复算最近祖先；opaque命名的原label子类按真实Class层级识别。同一个owner仍有多标签就拒绝，选定唯一owner后再验标签位置与间距。当前host／新产物准入以STATUS为准，旧revision3结果保持原判定。

R21自身CPU、original及fitted全部独立通过并完整返回。两靴各927 TREE／158对VIEW-RECT／14 OWNER、38应用，所有父边与最近祖先可复算，写失败／refused／truncated0；fitted原widget标题与下一排图标的两处间距都17.5点。完整[新stage](../reproducibility/native-label-ownership-r21-stage-20261122.json)与[原profile](../reproducibility/native-label-ownership-r21-profile-20261122.json)保持本token和原字节。旧段落描述R20当时的缺口，不改变旧REVISE。

[R19／R20原阶段](../reproducibility/native-full-model-r19-r20-stage-20261122.json)证实完整38应用身份；[R20原UI报告](../reproducibility/native-full-model-r20-original-profile-20261122.json)仍REVISE。两个widget按坐标各得到2／3个同名标签候选，部分来自其它view树；即时父节点未发出，旧raw不能严格还原祖先。

R21已完成该归属capture，并由独立decoder复算最近真实SBIconView／root-list祖先和17.5点间距。后续新capture继续按原祖先与完整状态验收，不按局部frame／字号挑选标签。真人触控、完整safe-area、实际应用功能、LCD光学／背光、FPS、壁纸和其它外设继续各自验收。

历史[R16布局](../reproducibility/native-layout-r16-stage-20261122.json)、[R17缺VIEW653](../reproducibility/native-grid-density-r17-stage-20261122.json)、[R18完整捕获与旧参考范围](../reproducibility/native-layout-model-r18-stage-20261122.json)保留原判定。整合前全文／SHA见[原文manifest](archive/20261122/native-full-model-r19-close-out/original-document-manifest.json)。旧失败包不重发，原750×1624基线及manifest保留。
