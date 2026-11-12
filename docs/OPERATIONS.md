# 真机与相机

只BH905ND69G，fastboot带-s；COM>=10且VID1209/PID0001，禁止COM3。PON只0x06；单download/boot、失败不重发。ACK/DRAIN/RESET/FINAL_DRAIN、连续健康30秒、原worker退出后下一轮。

gl5已在设备，不因整理重刷。禁止擅自flash/erase userdata、SD、TA、XFL、救砖固件与备份。新内核对应private/owner/ready，新库对应CPU门槛；无LIVE的早期E1F4仍可能需物理恢复。

相机唯一override→AF(/cam/1/af)→等2秒→MJPEG(/video?1280x720)→每5帧抽1→rotate90。已转照片不再转，frame.jpg仅视频失败单次兜底，grab_cam.py永久禁用。一个controller持COM/camera，不另开。

旧绝对路径控制器归档，正式入口的分阶段准入见 SURFACE-DIAGNOSTICS.md。所有新维护工具不得依赖Patch；boot/source/runtime/return/照片绑定当前token。

正式设备辅助工具现在位于 tools/device/：controller.py、boot_once.py、serial_worker.py、capture.py、return_evidence.py 和 Sony USB guard。fastboot/bash路径从 LILAC_FASTBOOT、LILAC_BASH 或 PATH 获取。原xperable二进制由使用者放到 assets/device-tools/xperable.exe，guard校验其SHA后仅操作Sony；boot仍走无重发的boot_once，不能用xperable内置boot重试。

当前计划强制 max_cycles=1、on_pass/on_failure=stop、return_evidence=final-drain。路径迁移的20项主机故障回放通过；正式controller的v725 private/owner/ready已实机单发并完整自动返回。新的诊断准入与像素/生命周期故障回放另有8项；不能忽略SHA或同内核证据去跑旧manifest。
