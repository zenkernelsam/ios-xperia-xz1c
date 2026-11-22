# 当前状态 — 2026-11-22

Sony BH905ND69G已完成R21自身CPU、original六行和fitted五行三次独立单发，全部四回程marker、PON0x06、连续健康30秒及worker退出通过。最新为健康fastboot，无controller／串口worker／in-flight boot。

原生布局独立PASS：两靴各927个TREE节点／158对VIEW-RECT／14个OWNER／3组grid、4模型／41icon／38应用，refused／truncated／write_failures均0。全部父边及最近原Class祖先经decoder复算；fitted两处widget标题到下一排图标间距都17.5点。全应用multiset、Dock顺序和Folder稳定身份／内容一致，根层29、Dock4、Folder内5。可见第一页12个应用view，第二页17个应用model尚无view，身份从完整model核对。

原证据见[R21 stage](../reproducibility/native-label-ownership-r21-stage-20261122.json)、[新original基线](../reproducibility/native-label-ownership-r21-original-baseline-20261122.json)、[新fitted完整profile](../reproducibility/native-label-ownership-r21-profile-20261122.json)。这是原生布局／模型身份和原scanout的通过；完整safe-area、应用启动功能、真人全屏触控、壁纸、FPS、光学／背光、睡眠唤醒、声音和无线继续独立验收。

## 当前准确输入

| 输入 | 精确身份 | 已验范围 |
| --- | --- | --- |
| v730 kernel／83实际源 | `f258dee9c6d142b90a573f796eeec5e92b39dd65feed6a21b2542d90fb66a88d`，20665112B | private／owner／ready、CPU、原scanout及0x06回程 |
| 固定Mesa | 24.3.4／`769e51468b49b2a42f0a0eaf71cf9eed5ff4e5de`／FD540 GLES3.1 | 免费开源Mesa／Freedreno；Vulkan未移植 |
| 当前GLES | `0766d5c713a883426e801e6d162bbdc46df79c5bc094ca77a7b6d6462599d607` | 自身CPU和完整GLES9，本轮两靴独立复验 |
| 当前Metal／source-map | `8feea4593116ffe9187204f5dd4cf4dd48d3027816d4763ecef20da200f86a27`／`db1605ef8dae8f8ed0a29999dcb2cf7537457aba7ad428677c21e0e914771b78` | R11完整native21及PNG6 |
| R21 geocompat revision4 | `ee7a3207f26af5e9106cbd98c74392153b11a2d345386964db131110a2a6153c`，106544B | 真实Class树／最近祖先、host ASan／UBSan与实际wire解码、ARM64签名、新RAM自身CPU及原／拟合布局 |
| R21 RAM blob | `4277931a612742a2eb2e02f67af8c2083116a13686c7594e7d0a3b5e87f0cf27`，3920885B | 完整payload、readonly HFS、signed库／manifest、原raw和模型绑定 |

实际构建／实机源码91f81a2。五个guest构建输入与三个host decoder源在manifest锁定，Mac／Windows实际SHA一致；三包完整payload和5个实际control故障拒绝通过。首个builder只读stdout遗漏stderr的主机失败保留，修正后helper-r2通过，无设备启动；[离线原记录](../reproducibility/native-label-ownership-r21-offline-20261122.json)不改写为实机结果。新签名／SDK仍可能改变二进制SHA，须自己的准入。

生产CA基线按[run-index](../reproducibility/v730-ca-matrix-r1-run-index.json)及准确源manifest复现；R21布局通过不自动提升为完整原UI基线。

## 本轮单发

| 阶段 | token | 独立结果 | 完整返回含健康30秒 |
| --- | --- | --- | ---: |
| R21新RAM CPU | `960521e563270307` | PASS，原raw与actual-evidence一致 | 39.266秒 |
| R21 original／GEO_GRID0 | `3e28639a5012be41` | GLES9、scanout、完整祖先／模型基线PASS，两页六行 | 39.438秒 |
| R21 fitted／GEO_GRID1 | `41797ddbf4c8b947` | GLES9、scanout、完整38应用／Dock／Folder及17.5点间距PASS，两页五行 | 38.859秒 |

同靴ADT、MobileGestalt、UIScreen、真实扫描和F12范围一致：720×1280像素／360×640点／scale2，面板BGR888／stride2160，Clock圆形与图标比例保持。LegacyPhone平直状态栏已移除红色rdar占位。R21 fitted原PNG SHA为`88d1327bb7e979154583885447c0b5246ba547165f51fca9298aadb596afca48`，只证明扫描缓冲区像素。120秒一次合成上滑不代替真人测试。

revision4发完整TREE父边／原Class角色和OWNER的icon/list／hops／完整状态，不用坐标或字号选择标签。opaque原子类按真实Class继承识别；独立decoder核对所有边、类型与计数，复算最近祖先，唯一owner后再验位置与间距。同一owner内重复标签仍REVISE。17个host故障、真实C++ fixture和ARM64均通过；[几何专题](DISPLAY-GEOMETRY.md)给参数与恢复。

原R20坐标候选2／3个的REVISE、R17缺VIEW653和R18旧15个reference只覆盖第一页的REFERENCE_INCOMPLETE保留，未补记录或重发；新的合格捕获独立解决取证缺口。历史入口见[INDEX](INDEX.md)。

## GPU、壁纸与其它外设

R11原生呈现五个阶段在各自pool关闭后allocated均0、最终0；完整[native21 proof](../reproducibility/current-native21-proof.json)与[PNG6 proof](../reproducibility/current-PNG6-proof.json)绑定同kernel／GLES／Metal／source-map。native presentedTime仍0；shared-event selectors36／37／38／45仍零输出，未实现。旧R10残留11520B及所有原失败保留。

原Poster的33／33方法、request及mode已观察；原applyMode后deactiveMode，display／tick／pipeline未观察到，黑壁纸仍STOP／OBSERVATION_INCOMPLETE。保留原paused／mode，继续环境与请求来源，见[请求专题](METAL-REQUEST-OBSERVATION.md)。CPU／launchd、真人F12与音量键已有历史实机证据。

下一步继续原壁纸environment／mode路由、safe-area和真实应用、动态UI／FPS、睡眠唤醒、LCD光学／背光、声音及Wi-Fi／蓝牙。原snapshot、GPU Completed、presentedTime、power_off和合成输入分别保持证据范围；完整目标仍active。

## 操作与整理

仅BH905ND69G；所有fastboot设备命令-s，COM≥10且VID1209/PID0001、一个worker、单download／boot、失败不重发、PON只0x06。四回程marker、连续健康30秒及worker退出后才下一轮。内核GPU底层／allocator／SCM／PON／RPC／VM、SweetGPU、当前GLES／Metal保持冻结。无本token LIVE的早期E1F4仍可能需物理恢复；gl5／userdata／SD／TA／XFL／救砖、校准和备份受保护。

R21追加Mac10项／14文件／75595606B及Windows4项／14文件／27021937B进入原生回收站，所有内容及131／366个保护文件SHA通过，未清空。本周Mac累计2853项／36373文件／14365132875B（约13.38GiB），最新build约3.81GiB、系统可用快照约86.83GiB；移入垃圾桶仍占磁盘。当前源、signed库／blob、原raw／失败／proof和少量原图保留。

本周50份入口原文与过时手册均已按SHA归档；状态只在本页、构建在BUILD、操作在OPERATIONS、恢复在LAYOUT、专题与历史在INDEX。research仅89个参考Git仓库，两套Ubuntu垃圾桶均无内容。实际回收和原文manifest见[ORGANIZATION](ORGANIZATION.md)，新修补只在正式ios-xperia-xz1c维护并同步Mac。
