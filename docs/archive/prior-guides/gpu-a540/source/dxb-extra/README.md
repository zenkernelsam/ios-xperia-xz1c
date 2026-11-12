# dxb 本地附加文件归档

2026-10-27 从 Mac 的 `dxb` 树导出，均为**未跟踪**文件，不在父仓 `xnu/patches` 中。
它们不参与主构建（`build.sh` 只应用 `xnu/patches/*.patch`），仅作云端归档。

## 内容

- `0095-lilac-msm8998-cpu-gic-bringup.patch` — 实验版；编号与父仓
  `0095-lilac-pon-stage-marks.patch` 冲突，不要同时应用。
- `0200-sagit-machine-config.patch` … `0211-sagit-4k-ios-sized-maps.patch`
  — sagit / msm8998 的 4K 页表实验（2026-10-24），未合入主分支。
- `../dxb-build-local.diff` — `dxb/build.sh` 的本地改动
  （`-j16`、`INCREMENTAL=1` 快速路径）。

## 未归档的上游 patch

`dxb/patches` 里的 `TrustCache`、`machine_routines`、`kas_info`、
`iobuffermemd_monterey`、`skywalk_*`、`syntax_checker_*` 来自
`blacktop/darwin-xnu-build`，可从上游复现，故不重复归档。
