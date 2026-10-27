# lilac-gpu 源码快照（只读，非构建输入）

这些文件是 `dxb/xnu` 分支 `lilac-gpu` 的源码快照，用于云端备份与回溯。
**`build.sh` 不会读取本目录**；构建只应用 `xnu/patches/*.patch`。
请不要把这些文件或 `.diff` 当作 patch 再次应用，否则会与 dxb 树里的改动重复。

## 来源

- 基线：`xnu-8796.141.3`（`1b191cb58`）
- 分支 HEAD：`1d143d90d`（2026-10-27 10:43，GPU CP v222–v236）
- 额外包含：工作区未提交的 v237 SMMU 2MB 页表实验
- 导出时间：2026-10-27，Mac 侧

## 文件

- `sweet_gpu.c` — 含 v222–v236 与 v237 工作区改动
- `sweet_spmi.c` / `sweet_spmi.h`
- `vermeer_debug.c` / `vermeer_log_ring.h`
- `vermeer_usb_debug.c` / `vermeer_usb_protocol.h`
- `sweet-gpu-v237-smmu-2m.diff` — v237 未提交改动（参考）
- `usb-cdc-integration.diff` — USB CDC 的 `bsd/conf/files` + `bsd/kern/bsd_init.c` 接入改动（参考）

## 给 Windows 侧

正常 `git pull` 即可，不需要再走 bundle。不要重新应用本目录内容；
如果 Windows 的 dxb 树已经有这些改动，继续直接构建即可。
