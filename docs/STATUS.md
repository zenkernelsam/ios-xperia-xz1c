# 当前状态 — 2026-11-22

Sony BH905ND69G已完成r18两靴并健康返回fastboot；CPU4d7848005c7b99f4独立PASS／38.641秒，GPU4b02249c29856d10原controller、GLES9、scanout及完整layout捕获PASS／38.890秒。没有controller／串口worker／构建任务或in-flight boot。

r18 revision2原raw有927节点／158对VIEW-RECT／3grid，refused0／truncated0／write_failures0；两处原widget标题框到下一排图标间距均17.5点，原scanout保持native比例、平直状态栏和正常标题间距。原R17缺VIEW653仍为REVISE，未补造或重发。

应用模型已完整读到根层两页32个icon（29应用、2widget、1folder），第一屏12个应用view、第二屏0个view但17个应用model，说明view数量不能证明未显示页的应用数量。旧15个参考只覆盖之前第一页；原profile拒绝文件保留，解释为REFERENCE_INCOMPLETE，不据此宣布应用丢失或全部保留。folder内部／Dock及合格的全模型原基线尚未取证，原UI整体仍REVISE，见[本轮原阶段](../reproducibility/native-layout-model-r18-stage-20261122.json)。

下一轮补原folder／Dock模型及合格的原布局全身份基线，再与native五行的完整身份比较；不能拿DefaultIconState静态模板或未创建view当本机全基线。壁纸仍黑，原策略不强制active/unpause；GPU原UI、FPS／睡眠／光学／声音／无线全部目标保留。v730／GPU底层及GLES／Metal字节冻结。

当前r19源码候选：model revision3读取Root、Dock及原Folder.lists／uniqueIdentifier，记录parent/slot/list和原folder LINK；原35方法／ABI已核对。原布局GEO_GRID0与fitted1由新--native-grid-mode和完整payload/env/source binding区分，合格全模型baseline后再拟合模式比较全应用multiset、Dock顺序、folder身份与内容。7个graph/control故障host拒绝、旧schema维持；新host／ARM64／CPU／GPU尚未准入，当前无在途boot，原R17/R18拒绝及full baseline缺口保留。

## 当前准确输入与验收范围

| 输入 | 精确身份 | 已验范围 |
| --- | --- | --- |
| v730 kernel／83源 | `f258dee9c6d142b90a573f796eeec5e92b39dd65feed6a21b2542d90fb66a88d`，20665112B | private／owner／ready、CPU、真实scanout及0x06回程 |
| 固定Mesa／GPU | 24.3.4／`769e51468b49b2a42f0a0eaf71cf9eed5ff4e5de`／FD540 GLES3.1 | Mesa／Freedreno GLES；Vulkan未移植 |
| 当前GLES | `0766d5c713a883426e801e6d162bbdc46df79c5bc094ca77a7b6d6462599d607` | 自身CPU／完整GLES9，最新r18独立复验 |
| 当前Metal／source-map | `8feea4593116ffe9187204f5dd4cf4dd48d3027816d4763ecef20da200f86a27`／`db1605ef8dae8f8ed0a29999dcb2cf7537457aba7ad428677c21e0e914771b78` | r11完整native21＋PNG6；原UI／光学另验 |
| r18 geocompat | `32fc1868932df48c8350d449963e11564d68d5e043de05a79012f228e19d9a13` | 原native五行、C完整writer、模型与revision2采样；CPU/GLES9及完整捕获通过，全应用基线未准入 |
| r18 RAM blob | `81cb3ee681c9f1c7b990375a510a22f5f0ed747cc711a779f8b728d4d9dd2bc9` | 5源／signed helper／manifest／完整payload和Mac实际源绑定 |
| r18实际源码 | `2092b732e6f9d512bbcd4f164382ab80edcbbcde` | 正式Windows／Mac一致；实际产物按各manifest复现 |

生产CA基线仍由[run-index](../reproducibility/v730-ca-matrix-r1-run-index.json)及准确source manifest复现。当前候选完整probe通过不会自动提升为完整原UI基线。CPU／launchd、真人F12触控与音量键已有原证据；本轮串口合成上滑不代替真人验收。

## 原生屏幕、状态栏与布局

| 独立单发 | token | 原结果／完整回程含健康30秒 |
| --- | --- | --- |
| r15新RAM CPU | `270a90b49369cd96` | PASS，38.672秒 |
| r15新RAM GLES9／原UI扫描 | `2d7d487dbeeb4afe` | PASS，38.906秒 |
| r16新RAM CPU | `2cd8a244ad3a9533` | PASS，39.735秒 |
| r16新RAM GLES9／布局／原UI扫描 | `226d56831b80e600` | PASS，38.484秒 |
| r17新RAM CPU | `ed21f91a773875bc` | PASS，38.968秒 |
| r17新RAM GLES9／原UI扫描 | `217bef7d784b1b5d` | 原controller PASS，39.579秒；UI VIEW/RECT独立REVISE |
| r18新RAM CPU | `4d7848005c7b99f4` | PASS，38.641秒 |
| r18新RAM GLES9／layout/model／scanout | `4b02249c29856d10` | PASS捕获／17.5点间距，38.890秒；全应用身份reference incomplete |

同靴UIScreen为360×640点、nativeBounds720×1280、scale2；MobileGestalt三个main-screen答案为720／1280／2，ADT四属性与native1／sf2x0绑定。150秒真实720×1280／BGR888／stride2160扫描图显示正常比例图标和Clock圆形；LegacyPhone平直状态栏替换原nil／unsupported fallback，红色rdar占位消失。r15原raw SHA为`91e5ab29c4d2d1e9217f283592e6a33aa1f8cf5f814f58a1053f236ed624fa68`。

r15／r16八个窗口的原safeAreaInsets为38／0／29／0，当次Weather／Calendar标签与第一排图标重叠。r17实图间距已改善，但完整布局／应用模型身份、真人全屏触控、光学／背光／FPS未准入，壁纸仍黑。GLES9作者probe仍按原750×1624测试canvas执行，不把它混成原UI尺寸。历史绑定和保存PNG SHA见[r15原阶段](../reproducibility/native2x-flat-r15-stage-20261122.json)，最新原拒绝在上方r17阶段；尺寸契约和恢复方法见[DISPLAY-GEOMETRY](DISPLAY-GEOMETRY.md)。

r16同靴root列表frame360×640，配置listSizeForIconSpacingCalculation却为320×693、6行4列、portrait insets57／19／155／19。原iconSpacing getter返回-123／-123，未用于推算真实间距；原view实测常规行距约74.333点。小组件legibility label y195.167／h21与下一排icon y205.667的bbox纵向重叠10.5点，真实扫描图也保留该重叠；配置差异的具体来源和因果尚待静态核对。

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
| [r16](../reproducibility/native-layout-r16-stage-20261122.json) | 原布局948节点／166view／3grid完整，标签bbox重叠10.5点已定位 |
| [r17](../reproducibility/native-grid-density-r17-stage-20261122.json) | 五行原setter／正常间距实图；缺VIEW653，UI取证REVISE，应用身份待验 |
| [r18](../reproducibility/native-layout-model-r18-stage-20261122.json) | 158对完整捕获／真实间距17.5；根层29应用，原全身份／folder/Dock未验证 |

## 下一步与操作边界

1. 读取准确20H392的SpringBoard布局／icon-grid／safe-area来源，先观察原view与配置，不猜机型／FaceID／notch，也不全局伪造safeAreaInsets。
2. 继续原Poster环境和mode路由观察；维持原暂停策略，不强制active／unpause、不画替代壁纸冒充原UI。
3. FPS、睡眠唤醒、LCD光学／背光、声音、Wi-Fi／蓝牙保持active。共享事件selectors36／37／38／45仍零输出，未编译草稿没有接入当前库。

只BH905ND69G；COM≥10且VID1209/PID0001；单download／boot、失败不重发、PON只0x06，ACK／DRAIN／RESET／FINAL_DRAIN、连续健康30秒及worker退出后才下一轮。内核GPU底层、allocator、SCM、PON、RPC、VM和SweetGPU冻结。无LIVE早期E1F4仍可能需要物理恢复。gl5／userdata／SD／TA／XFL／救砖／校准／备份受保护。

## 整理与文档

本周及追加Mac累计2030项／35544文件／13736788419B（约12.79GiB）已进原生Trash，全部用户内容SHA复核，垃圾桶未清空。最新Mac约87.7GiB可用，正式build约4.22GiB；移入垃圾桶仍占磁盘。r18另有Windows4项／10文件／26970280B进原生回收站，原signed库／blob／manifest／raw／proof／失败和少量当前页面保留。

现行文档只由本页给状态、BUILD给复现、OPERATIONS给规则、LAYOUT给恢复、INDEX联专题。此次整合前50份原文／SHA已归入[归档manifest](archive/20261122/native-r15-close-out/original-document-manifest.json)；旧in-flight只保留为历史。实际回收清单及本周汇总见[ORGANIZATION](ORGANIZATION.md)。所有源修补、工具和锁只在正式ios-xperia-xz1c维护。
