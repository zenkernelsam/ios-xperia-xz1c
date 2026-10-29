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
- `dxb-extra/` — dxb 树里未跟踪的实验 patch（sagit 4K 等）归档
- `dxb-build-local.diff` — dxb/build.sh 的本地构建参数改动

## dxb/xnu 归档 bundle

- `dxb-xnu-lilac-gpu-1d143d90d.bundle`（1.4MB，SHA256
  `e3c1cff23e3a318d361a73256b99719b7fa32f5b978e2e45a7977d8c758e6b58`）
  包含 `refs/heads/lilac-gpu` 与 `refs/stash`。
- 为缩小体积，基线 `1b191cb58`（xnu-8796.141.3）未打进 bundle，
  还原前需先从 Apple 官方仓库取得该基线。
- 还原示例：

  ```
  git clone https://github.com/apple-oss-distributions/xnu.git dxb-xnu
  cd dxb-xnu
  git fetch /path/to/dxb-xnu-lilac-gpu-1d143d90d.bundle \
      'refs/heads/lilac-gpu:lilac-gpu' 'refs/stash:refs/stash'
  git checkout lilac-gpu
  ```

- 这是 git 对象归档，不是补丁；正确用法是 `git fetch`，不要 `git apply` 或 overlay。
- 不要 apply 到现有 dxb 树。

## 源码包（解压即用）

- `dxb-xnu-lilac-gpu-worktree-20261027.tar.gz`（15MB，SHA256
  `15c1a2f2313a00a55226c906cf024bcbf7177f503206582eec5104340037620d`）
  是 dxb/xnu 当前工作区的源码快照，含 v237，已排除 `.git`、`*.orig`、
  `*.rej` 和构建输出。
- 解压：`tar -xzf dxb-xnu-lilac-gpu-worktree-20261027.tar.gz -C <目标目录>`，
  会得到 `xnu/`。
- 这是同一条源码线，不是补丁；**不要 overlay 到现有 dxb/xnu**，否则会覆盖本地改动。
- 构建仍需 `blacktop/darwin-xnu-build` 的 `build.sh` 与 `xnu/patches`；
  dxb 本地构建参数改动见 `dxb-build-local.diff`。

## 给 Windows 侧

正常 `git pull` 即可，不需要再走 bundle。不要重新应用本目录内容；
如果 Windows 的 dxb 树已经有这些改动，继续直接构建即可。
