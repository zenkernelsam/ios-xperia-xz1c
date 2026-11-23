# 磁盘清理方案（2026-10-21）

C盘现状：~47GB 空闲。按收益排序。

## 方案 A — WSL 虚盘瘦身（最大头，~34GB）

`ext4.vhdx` 61.8GB，实际内容仅 ~28GB。`compact`/`fstrim` 对本版 WSL 无效，
**唯一可靠办法是导出再导入**（虚盘重建为真实数据大小）：

```powershell
wsl --shutdown
# 导出到别的盘（tar 未压缩约 28GB，C盘装不下，需 D/E/外接盘）
wsl --export Ubuntu-22.04 D:\wsl-backup-ubuntu2204.tar
wsl --unregister Ubuntu-22.04            # 删除 62GB 虚盘（会警告，确认）
wsl --import Ubuntu-22.04 D:\WSL\Ubuntu-22.04 D:\wsl-backup-ubuntu2204.tar --version 2
# 验证能进：wsl -d Ubuntu-22.04，确认 /root/xz1c、/root/wslk 都在
# 之后可删 tar（或留作 WSL 整体备份）
```

⚠️ import 后默认登录 root 会丢失用户映射；我们的用法全是 root，无影响。
⚠️ 重新进 WSL 后记得 `insmod /root/wslk/fs/hfsplus/hfsplus.ko`（如需要）。

## 方案 B — 立刻可删的项目产物（~15GB，零风险）

| 目标 | 大小 | 说明 |
|---|---|---|
| `ios-xperia-xz1c/research/ipsw-x/os.dmg` | 4.7 GB | 已用于构建 root 镜像，可从 IPSW 重解 |
| `ios-xperia-xz1c/research/ipsw-x/sysos.dmg` | 2.7 GB | 同上 |
| `ios-xperia-xz1c/research/ipsw-x/appos.dmg` | 11 MB | 同上（顺手） |
| WSL `/root/hailo_ai_sw_suite_2024-04_docker.zip` | 8.2 GB | 已解压过的源码包（hailo 项目，用户确认后删） |

```powershell
# DMG 三个（Windows 侧直接删）
del "C:\Users\Cisco He\Desktop\Git\ios-xperia-xz1c\research\ipsw-x\os.dmg"
del "C:\Users\Cisco He\Desktop\Git\ios-xperia-xz1c\research\ipsw-x\sysos.dmg"
del "C:\Users\Cisco He\Desktop\Git\ios-xperia-xz1c\research\ipsw-x\appos.dmg"
# hailo zip（WSL 内）
wsl -d Ubuntu-22.04 -e rm -f /root/hailo_ai_sw_suite_2024-04_docker.zip
```

## 方案 C — 备选（需用户确认）

- **`Ubuntu` 另一个发行版虚盘 15.2 GB**：`wsl -l -v` 确认不用则
  `wsl --unregister Ubuntu`
- **IPSW 本体 6.2 GB**（`research/iPhone10,3,..._Restore.ipsw`）：
  DMG/ADT/kernelcache 都已解出；重下要 ~半小时，空间紧可删
- **`Patch\Xperia\research` 克隆仓库群** ~5.8 GB：都是 git 仓库可再 clone；
  建议保留（常查）
- **不可动**：`flash/fw`（救砖固件）、`unlock/`（XFL 备份）、
  `ios16-root-lilac.img` 16GB（核心产物）、SD 卡相关备份

## 建议执行顺序

1. 先做 B（15G，两分钟零风险）
2. 有外接盘就 A（再 +34G）
3. 还不够再动 C
