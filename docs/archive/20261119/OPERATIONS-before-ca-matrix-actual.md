# 真机与相机

用户已授权优先使用串口显示帧取证。v730 的新包可用 `--screen-evidence`；除首次 private 通道验收，其它阶段必须有同 kernel 完整 private 截图先验。只能用通过来源／整帧校验的扫描缓冲区 PNG 替代 UI 内容的相机观察；物理面板、背光／睡眠和光学颜色仍另验。旧相机 STOP 保持拒绝。完整规则见 [SCANOUT-SCREENSHOT](SCANOUT-SCREENSHOT.md)。

GPU截图默认观察开始45秒；`package_trial.py --screen-evidence --screen-after 150`可独立包装较晚的一次捕获，以免只看到早期logo。仅允许45或150秒，plan／artifact时机必须相同且位于观察窗口内。仍只发送一次`frame`，最终取证复用已验帧，失败不补发；若在150秒前需要返回，原失败优先，最终捕获保留真实时机，不冒充150秒截图。不另开串口，不修改已有靴或重发失败bundle。

只BH905ND69G，fastboot带-s；COM>=10且VID1209/PID0001，禁止COM3。PON只0x06；单download/boot、失败不重发。ACK/DRAIN/RESET/FINAL_DRAIN、连续健康30秒、原worker退出后下一轮。

gl5已在设备，不因整理重刷。禁止擅自flash/erase userdata、SD、TA、XFL、救砖固件与备份。新内核对应private/owner/ready，新库对应CPU门槛；无LIVE的早期E1F4仍可能需物理恢复。

相机唯一override→AF(/cam/1/af)→等2秒→MJPEG(/video?1280x720)→每5帧抽1→rotate90。已转照片不再转，frame.jpg仅视频失败单次兜底，grab_cam.py永久禁用。一个controller持COM/camera，不另开。

旧绝对路径控制器归档，正式入口的分阶段准入见 SURFACE-DIAGNOSTICS.md。所有新维护工具不得依赖Patch；boot/source/runtime/return/照片绑定当前token。

正式设备辅助工具现在位于 tools/device/：controller.py、boot_once.py、serial_worker.py、capture.py、return_evidence.py 和 Sony USB guard。fastboot/bash路径从 LILAC_FASTBOOT、LILAC_BASH 或 PATH 获取。原xperable二进制由使用者放到 assets/device-tools/xperable.exe，guard校验其SHA后仅操作Sony；boot仍走无重发的boot_once，不能用xperable内置boot重试。

当前计划强制 max_cycles=1、on_pass/on_failure=stop、return_evidence=final-drain。v725/v726及v728+r4各阶段均实际单发和完整自动回程。36项主机故障回放覆盖传输/版本/RAM绑定/像素生命周期/只读生产路径。投递前复算完整payload、实际Mac源码/内核SHA及对应新前端/Mesa输入；gpu必须使用完全相同RAM库的CPU实际证据。

v727 GPU及v728+r2/r3的原REVISE保留。首次v728 owner缺snap2 slot2，独立验收拒绝；另做90秒采样时机对照取得完整证据后才下一步。控制器owner PASS前检查真实全部元数据，缺项记REVISE并正常回程；不能用总结、另一靴或推测补缺。

最新Surface/登记轮均完整PON06返回。原native探针PASS与原应用missing-selector必须分别报告；run_trial会识别原SweetMTL类的exception detail中unrecognized selector并提前回程。回程ACK后的提交拒绝/未完成样本单独保留，不当作完整样本；ACK前probe失败即使已有成功样本也须拒绝。具体边界见METAL-IOSURFACE.md。

Mac源码同步不是旧 `tools/mac-autobuild.sh` 常驻进程已经完成的保证。2026-11-15核对发现
`~/xnu-autobuild.pause` 从11月3日起保留；旧脚本被该门挡住，且仍调用早期编号patch复制链。
本轮已保留本地差异后将Mac主仓快进到已发布7eb4756。该旧暂停文件保持原样，避免它
突然抢用当前构建树；现行编译入口和完整lilac-current.patch以BUILD.md为准。每轮编译
核对Mac的完整Git提交和实际输入SHA，不把单独复制源码当作Git同步，也不为整理启动设备。
原根镜像gl5、gl4回退、最新raw、原始IPSW、当前GPU构建与失败证据保留；过时镜像移入
原生Trash的原路径/返回路径/完整SHA在配套工作台Y700-Elden的
`maintenance/cleanup-20261115-macos`记录。代理只做原生回收；用户随后确认自行检查并
清空第一批Trash，该批的旧文件现不可从Trash恢复。当前镜像和源码仍保留。

原Aegir短登记可与长probe分离，但必须以同kernel/GLES/Metal/source-map的完整backboard实机raw准入；新half/narrow需自身GPU证明。registry-short-r4原threshold编译成功，原app check-in timeout的REVISE保留，不能当UI成功。后续每次singleboot都保留四回程marker/健康/旧worker退出门槛。
