# 原生显示比例 — 2026-11-21

用户要求取消最后的横纵不等缩放，让iOS按Sony XZ1C的720×1280原生16:9屏幕布局。目标是**720×1280像素、360×640点、scale2**，图标和圆形保持比例，显示覆盖完整面板。当前设备仍用375×812点／750×1624像素；原生候选尚未投递，不能称比例已经修好。

| 层 | 当前 | 原生候选 |
| --- | --- | --- |
| 面板／扫描stride | 720×1280／BGR888／2160B | 保持实际硬件扫描契约 |
| 渲染canvas | 750×1624 | 720×1280 |
| UIKit点和scale | 375×812／2 | 360×640／2 |
| 原MobileGestalt main-screen | 750／1624／2，原raw已验 | 必须同靴读取为720／1280／2 |
| 最终尺寸变换 | X0.96、Y0.78818 | X1、Y1，无横纵差异 |

旧横向相对纵向扩大1.218倍，渲染像素1,218,000多于面板921,600。原生候选约少24.34%像素；这是几何和工作量计算，不能推相同比例的FPS或CPU改善。采用360×640点会让布局重排、可显示内容减少，是新系统显示配置。

## 配置链与离线实现

实际20H392日志中UIScreen、nativeBounds、scale及MobileGestalt main-screen-width／height／scale已相互一致，当前geocompat只读MG，并在375×812配置的statusbar provider不足时选择已核Split1125。**单改MobileGestalt不能保证ADT／IOMobileFramebuffer／UIKit／safe area同步**，不能将iPhoneX的机型／FaceID／notch身份值全面改成其它型号来猜布局。

tools/build/make_render_adt.py新增`--profile xz1c-native-2x`，只接受三种完整一致的输入：1125×2436／scale3、750×1624／scale2、720×1280／scale2。只改chosen display-scale、product artwork-scale-factor、disp0 canvas-width／height四个原CFData值，恢复它们后必须与整个原ADT逐字节相同。缺失／重复／混合／未知尺寸拒绝，输出不存在才创建。三项host测试通过，不当作手机UI验收。

```powershell
python -B tools/build/make_render_adt.py /path/to/verified/board.adt build/my-native2x/render.adt --profile xz1c-native-2x --manifest build/my-native2x/render-adt.json
```

现有v730 SweetFramebuffer已支持`sweetfb_native=1`发布原面板尺寸；它与`sf2x=1`冲突会拒绝启动显示服务。因此新包需native1／sf2x0和准确新ADT一起绑定，不能仅替换一条尺寸或沿旧RAM CPU proof。panel transform保留已验方向，触控原720×1280坐标需要核对全屏映射。当前package_trial严格复算基线完整payload，尚未接入新ADT选项，不手工改artifact或绕过它。

下一geometry试验需核实statusbar provider／safe area的原ABI和外观，必要的兼容仅针对该精确配置；MG若随ADT自动正确，则保持只读。如有缓存或原答案仍375×812，先取实际来源再修改对应显示答案，原文件／cache保留备份，不猜key。

## 验收与顺序

本轮已先收尾设备、清理Mac及整合文档。随后先解决tiled reference CPU日志一致性门槛及完整native21／PNG6，再单独新geometry包CPU／GLES／原UI验收；不混作同一包修复或重发旧失败。

新geometry实际要求：同靴UIScreen bounds360×640、nativeBounds720×1280、scale2；MG三个main-screen答案一致；真实scanout720×1280／2160、屏幕四角／圆形／文字／状态栏／homebar／锁屏／主页布局完整；F12全屏坐标和一次上滑实际正确；光学／背光另验。所有目标、pixel／source／token／四marker／健康30秒／worker退出完整，缺任一项保持未完成。

正式source、离线计算和边界在[geometry proof](../reproducibility/display-native2x-offline-20261121.json)，当前事实在[STATUS](STATUS.md)，单发与UFS红线在[OPERATIONS](OPERATIONS.md)。
