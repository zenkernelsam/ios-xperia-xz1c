# 真机与相机

只BH905ND69G，fastboot带-s；COM>=10且VID1209/PID0001，禁止COM3。PON只0x06；单download/boot、失败不重发。ACK/DRAIN/RESET/FINAL_DRAIN、连续健康30秒、原worker退出后下一轮。

gl5已在设备，不因整理重刷。禁止擅自flash/erase userdata、SD、TA、XFL、救砖固件与备份。新内核对应private/owner/ready，新库对应CPU门槛；无LIVE的早期E1F4仍可能需物理恢复。

相机唯一override→AF(/cam/1/af)→等2秒→MJPEG(/video?1280x720)→每5帧抽1→rotate90。已转照片不再转，frame.jpg仅视频失败单次兜底，grab_cam.py永久禁用。一个controller持COM/camera，不另开。

旧绝对路径控制器归档，正式入口的分阶段准入见 SURFACE-DIAGNOSTICS.md。所有新维护工具不得依赖Patch；boot/source/runtime/return/照片绑定当前token。

正式设备辅助工具现在位于 tools/device/：controller.py、boot_once.py、serial_worker.py、capture.py、return_evidence.py 和 Sony USB guard。fastboot/bash路径从 LILAC_FASTBOOT、LILAC_BASH 或 PATH 获取。原xperable二进制由使用者放到 assets/device-tools/xperable.exe，guard校验其SHA后仅操作Sony；boot仍走无重发的boot_once，不能用xperable内置boot重试。

当前计划强制 max_cycles=1、on_pass/on_failure=stop、return_evidence=final-drain。v725/v726及v728+r4各阶段均实际单发和完整自动回程。36项主机故障回放覆盖传输/版本/RAM绑定/像素生命周期/只读生产路径。投递前复算完整payload、实际Mac源码/内核SHA及对应新前端/Mesa输入；gpu必须使用完全相同RAM库的CPU实际证据。

v727 GPU及v728+r2/r3的原REVISE保留。首次v728 owner缺snap2 slot2，独立验收拒绝；另做90秒采样时机对照取得完整证据后才下一步。控制器owner PASS前检查真实全部元数据，缺项记REVISE并正常回程；不能用总结、另一靴或推测补缺。
