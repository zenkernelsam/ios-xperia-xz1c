# 当前状态 — 2026-11-22

Sony BH905ND69G已完成R19 CPU、R19 original GPU和R20 fitted GPU三次独立单发，全部经0x06、四回程marker、连续健康30秒和worker退出返回。最新状态为健康fastboot，无controller／串口worker或in-flight boot。本轮原生回收与文档整合已收尾。

完整模型身份保留已验：原六行和拟合五行均有4个模型／41个icon／38个应用，根层29、Dock4、Folder内5；全应用multiset、Dock顺序、Folder稳定身份及内容一致。R20独立UI后处理仍REVISE：同名legibility标签在其它view树也出现，现有记录没有足够的祖先归属，保留WIDGET_ORIGINAL_LABEL_AMBIGUOUS和ORIGINAL_WIDGET_LABEL_INVENTORY_INCOMPLETE。原扫描图可见五行间距改善，数值归属和LCD光学仍需独立验收。

完整原结果见[R19／R20阶段](../reproducibility/native-full-model-r19-r20-stage-20261122.json)、[原模型基线](../reproducibility/native-full-model-r19-original-baseline-20261122.json)、[R20原REVISE与身份对照](../reproducibility/native-full-model-r20-original-profile-20261122.json)和[标签归属卡点](../reproducibility/native-label-ownership-r20-blocker-20261122.json)。原报告、raw、包及像素未覆盖或重发。

## 当前准确输入

| 输入 | 精确身份 | 已验范围 |
| --- | --- | --- |
| v730 kernel／83实际源 | `f258dee9c6d142b90a573f796eeec5e92b39dd65feed6a21b2542d90fb66a88d`，20665112B | private／owner／ready、CPU、原scanout与0x06回程 |
| 固定Mesa | 24.3.4／`769e51468b49b2a42f0a0eaf71cf9eed5ff4e5de`／FD540 GLES3.1 | 免费开源Mesa／Freedreno；Vulkan未移植 |
| 当前GLES | `0766d5c713a883426e801e6d162bbdc46df79c5bc094ca77a7b6d6462599d607` | 自身CPU及完整GLES9，本轮两靴独立复验 |
| 当前Metal／source-map | `8feea4593116ffe9187204f5dd4cf4dd48d3027816d4763ecef20da200f86a27`／`db1605ef8dae8f8ed0a29999dcb2cf7537457aba7ad428677c21e0e914771b78` | R11完整native21与PNG6 |
| R19 geocompat revision3 | `38b282fea44db23c7ce0b4c66c37d1a83a68edf9bc75c36b553ceafd5918c08f`，106544B | 实际五源、host ASan／UBSan、ARM64签名、新RAM自身CPU、完整Root／Dock／Folder捕获 |
| R19 RAM blob | `c851d46882738c0de6f00c46b0c78cf796af5bd3171adb6386221b5c2e7a1900`，3920336B | manifest／signed库及完整payload绑定 |

helper实际编译源为a8d3207，实机期间正式Windows／Mac源码为6a9efe8；两个提交的guest编译输入一致，后者只更新主机preflight。准确五源SHA、manifest、image及原raw／PNG SHA均在本轮stage和各manifest。生产CA基线仍由[run-index](../reproducibility/v730-ca-matrix-r1-run-index.json)复现；本轮不提升为完整原UI基线。

## 本轮单发与独立判定

| 阶段 | token | 实际结果 | 完整返回含健康30秒 |
| --- | --- | --- | ---: |
| R19新RAM CPU | `6632d7b8b96d863d` | PASS，原raw与actual-evidence独立复算 | 38.828秒 |
| R19 original／GEO_GRID0 | `bfb3cb7b79e0cf96` | GLES9、scanout、完整原模型PASS，两页六行 | 39.562秒 |
| R20 fitted／GEO_GRID1 | `8ca68102298d865b` | GLES9、scanout、全模型身份PASS，两页五行；UI归属REVISE | 38.546秒 |

original为927节点／158对VIEW-RECT，fitted为1085节点／204对VIEW-RECT；各3组grid，refused／truncated／write_failures均0。可见主页有12个应用view，第二页尚无应用view但模型有17个应用；完整模型另有Dock4和Folder5，未用view数量代替未显示页的身份。应用启动功能、本轮真人触控、完整safe-area、壁纸、光学与FPS未随身份对照扩大为完成。

最初只读health在主机CIM进程查询12秒超时，download／boot／reset全0，原STOP_ERROR保留。保持原匹配范围的executable/property过滤修正后，第二次只读health连续31.562秒PASS、进程退出、设备操作仍0；两份原报告绑定本轮stage，不将主机查询超时解释成实机启动失败。

## 已验显示与GPU边界

同靴ADT、MobileGestalt、UIScreen、真实扫描和F12范围一致：720×1280像素／360×640点／scale2，面板BGR888／stride2160；Clock圆形和图标保持比例。LegacyPhone平直状态栏已移除红色rdar占位。R20原PNG SHA为`307ce784f15dd6d515ca3d878479e479f18372cb633e03319c1ce383f8aa6224`，仅证明原扫描缓冲区像素。

R18完整捕获曾测得两个widget标签间距17.5点；R20另有其它view树的标签，坐标选择分别得到2／3个候选，祖先中间节点未发出，不能据局部frame或字号挑一个候选。原R17缺VIEW653的REVISE和R18旧15个参考仅覆盖第一页的REFERENCE_INCOMPLETE保持历史原判定；本轮新增了完整原模型对照。配置、恢复和旧阶段入口见[DISPLAY-GEOMETRY](DISPLAY-GEOMETRY.md)。

R11原生呈现五个阶段在各自pool关闭后allocated均0、最终0；完整[native21 proof](../reproducibility/current-native21-proof.json)和[PNG6 proof](../reproducibility/current-PNG6-proof.json)绑定同kernel／GLES／Metal／source-map。native presentedTime仍0；shared-event selectors36／37／38／45仍零输出，未实现。旧R10残留11520B及其它原失败保留。

原Poster的33／33方法、请求和mode已观察；原applyMode后deactiveMode，display／tick／pipeline未观察到，黑壁纸仍STOP／OBSERVATION_INCOMPLETE。按原请求保留paused／mode策略，下一步继续原环境路由，见[请求专题](METAL-REQUEST-OBSERVATION.md)。CPU／launchd、真人F12及音量键已有历史真机证据。

## 下一步

1. 只读、限界地记录每个legibility label的真实SBIconView与root-list祖先及完整／未知状态；先host验证其它view树、缺祖先、循环和越界拒绝。
2. 新helper／RAM须自身CPU；再采新的original完整模型基线和fitted对照，保留本轮REVISE，不重发原包。
3. 继续原壁纸环境、动态UI／FPS、睡眠唤醒、LCD光学／背光、声音及Wi-Fi／蓝牙。扫描图、GPU Completed、合成上滑和power_off分别保持各自证据范围。

只BH905ND69G；COM≥10且VID1209/PID0001；单download／boot，失败不重发，PON只0x06。内核GPU底层／allocator／SCM／PON／RPC／VM及SweetGPU冻结。无本token LIVE的早期E1F4仍可能需要物理恢复；gl5／userdata／SD／TA／XFL／救砖、校准和备份受保护。

## 本周整理

本轮Mac813项／815文件／552748850B及Windows4项／10文件／27009609B已进入各自原生回收站，所有目标内容SHA及保护指纹通过。Mac本周累计2843项／36359文件／14289537269B（约13.31GiB）；当前build约3.79GiB、系统可用快照约87.51GiB。垃圾桶未清空，移入量不表示释放量。

整合前50份原文及SHA已归档，47个docs页面和README／AGENTS／Metal入口统一审核；当前状态只在本页，构建在BUILD、操作在OPERATIONS、恢复在LAYOUT、专题与历史在INDEX。实际回收及整合记录见[ORGANIZATION](ORGANIZATION.md)。research仅89个参考Git仓库，两套Ubuntu垃圾桶均无内容，新源修补只在正式仓库。
