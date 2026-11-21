# 当前状态 — 2026-11-22

Sony BH905ND69G已完成r15两靴并健康返回fastboot；没有controller、串口worker、构建任务或in-flight boot。下一步查原生布局中小组件标签重叠和原壁纸模式。原生720×1280显示、平直状态栏已实机核对，完整UI、光学、FPS和其它外设继续验收。

## 当前准确输入与验收范围

| 输入 | 精确身份 | 已验范围 |
| --- | --- | --- |
| v730 kernel／83源 | `f258dee9c6d142b90a573f796eeec5e92b39dd65feed6a21b2542d90fb66a88d`，20665112B | private／owner／ready、CPU、真实scanout及0x06回程 |
| 固定Mesa／GPU | 24.3.4／`769e51468b49b2a42f0a0eaf71cf9eed5ff4e5de`／FD540 GLES3.1 | Mesa／Freedreno GLES；Vulkan未移植 |
| 当前GLES | `0766d5c713a883426e801e6d162bbdc46df79c5bc094ca77a7b6d6462599d607` | 自身CPU／完整GLES9，最新r15独立复验 |
| 当前Metal／source-map | `8feea4593116ffe9187204f5dd4cf4dd48d3027816d4763ecef20da200f86a27`／`db1605ef8dae8f8ed0a29999dcb2cf7537457aba7ad428677c21e0e914771b78` | r11完整native21＋PNG6；原UI／光学另验 |
| r15 geocompat | `f4416b274a553f4900402f73bd0761462e38f7c968e7ecf912a1be745d2ac3b8` | 精确native屏幕的LegacyPhone provider，host ABI／签名及自身RAM CPU／GLES9通过 |
| r15 RAM blob | `b52f72f05cb14a0b76bcfc8b9986b6c3e51e7dd6ad301442fe81ccf91c9bc27f` | 新helper源／库／manifest绑定，完整payload和Mac实际源复算 |
| r15实际源码 | `cc4e637350c334125d2f3d5124a45fe9ef8d524e` | 正式Windows／Mac一致；产物必须按各manifest重建 |

生产CA基线仍由[run-index](../reproducibility/v730-ca-matrix-r1-run-index.json)及准确source manifest复现。当前候选完整probe通过不会自动提升为完整原UI基线。CPU／launchd、真人F12触控与音量键已有原证据；本轮串口合成上滑不代替真人验收。

## 最新r15：原生屏幕与状态栏

| 独立单发 | token | 原结果／完整回程含健康30秒 |
| --- | --- | --- |
| 新RAM CPU | `270a90b49369cd96` | PASS，38.672秒 |
| 新RAM GLES9／原UI扫描 | `2d7d487dbeeb4afe` | PASS，38.906秒 |

同靴UIScreen为360×640点、nativeBounds720×1280、scale2；MobileGestalt三个main-screen答案为720／1280／2，ADT四属性与native1／sf2x0绑定。150秒真实720×1280／BGR888／stride2160扫描图显示正常比例图标和Clock圆形；LegacyPhone平直状态栏替换原nil／unsupported fallback，红色rdar占位消失。Windows原raw SHA为`91e5ab29c4d2d1e9217f283592e6a33aa1f8cf5f814f58a1053f236ed624fa68`。

八个窗口的原safeAreaInsets为38／0／29／0。Weather／Calendar小组件标签仍与第一排图标重叠，壁纸仍黑；未证明完整布局、真实全屏触控、面板光学、背光或FPS完成。GLES9作者probe仍按原750×1624测试canvas执行，不把它混成原UI尺寸。完整绑定和保存PNG SHA见[r15原阶段](../reproducibility/native2x-flat-r15-stage-20261122.json)，尺寸契约和恢复方法见[DISPLAY-GEOMETRY](DISPLAY-GEOMETRY.md)。

## GPU与原壁纸证据

r11以probe自己创建的native layer补显式CA begin／commit／flush，五个phase在各自pool关闭后actual allocated均0、最终0；原765pixels／padding及present mode／time／options不变。完整[native21 proof](../reproducibility/current-native21-proof.json)及[PNG6 proof](../reproducibility/current-PNG6-proof.json)必须绑定同kernel／GLES／Metal／source-map。native presentedTime仍0，不能当LCD光学completion。原r10 final_allocated11520的失败保留，见[呈现契约](METAL-PRESENTATION.md)。

r13修正known Swift owner名字后33／33方法安装；127条原route、20条request完整，120秒一次合成上滑、150秒真实home scanout完成。原applyMode随后deactiveMode，原display／tick／pipeline未观察到，保持STOP／OBSERVATION_INCOMPLETE。global getter前8次样本preview／snapshot为true、unlock0、backlight1，采样上限不能推整靴或后续新environment。见[原请求](METAL-REQUEST-OBSERVATION.md)和[r13原阶段](../reproducibility/original-mode-swift-owner-r13-stage-20261122.json)。

| 保留的阶段 | 原判定与关键证据 |
| --- | --- |
| [r10](../reproducibility/stencil-private-r10-stage-20261121.json) | 四组GPU附件完整通过，presentation11520B未归零，REVISE |
| [r11](../reproducibility/presentation-lifecycle-r2-stage-20261122.json) | 完整native21／PNG6实机通过 |
| [r12](../reproducibility/original-mode-native21-r12-stage-20261122.json) | 31／33安装，REVISE；原失败包不重发 |
| [r13](../reproducibility/original-mode-swift-owner-r13-stage-20261122.json) | 33／33及真实home通过，原壁纸观察仍STOP |
| [r14](../reproducibility/native2x-r14-stage-20261122.json) | 原生尺寸／MG／F12上滑通过，rdar与标签重叠仍在 |
| [r15](../reproducibility/native2x-flat-r15-stage-20261122.json) | 平直状态栏通过，标签重叠／黑壁纸未解决 |

## 下一步与操作边界

1. 读取准确20H392的SpringBoard布局／icon-grid／safe-area来源，先观察原view与配置，不猜机型／FaceID／notch，也不全局伪造safeAreaInsets。
2. 继续原Poster环境和mode路由观察；维持原暂停策略，不强制active／unpause、不画替代壁纸冒充原UI。
3. FPS、睡眠唤醒、LCD光学／背光、声音、Wi-Fi／蓝牙保持active。共享事件selectors36／37／38／45仍零输出，未编译草稿没有接入当前库。

只BH905ND69G；COM≥10且VID1209/PID0001；单download／boot、失败不重发、PON只0x06，ACK／DRAIN／RESET／FINAL_DRAIN、连续健康30秒及worker退出后才下一轮。内核GPU底层、allocator、SCM、PON、RPC、VM和SweetGPU冻结。无LIVE早期E1F4仍可能需要物理恢复。gl5／userdata／SD／TA／XFL／救砖／校准／备份受保护。

## 整理与文档

本周及追加Mac累计1873项／35381文件／13510835475B（约12.58GiB）已进原生Trash，全部用户内容SHA复核，垃圾桶未清空。最新Mac约91.3GiB可用，正式build约4.16GiB；移入垃圾桶仍占磁盘。r15另有Windows3项／9文件／26918845B进原生回收站，原signed库／blob／manifest／raw／proof／失败和少量当前页面保留。

现行文档只由本页给状态、BUILD给复现、OPERATIONS给规则、LAYOUT给恢复、INDEX联专题。此次整合前50份原文／SHA已归入[归档manifest](archive/20261122/native-r15-close-out/original-document-manifest.json)；旧in-flight只保留为历史。实际回收清单及本周汇总见[ORGANIZATION](ORGANIZATION.md)。所有源修补、工具和锁只在正式ios-xperia-xz1c维护。
