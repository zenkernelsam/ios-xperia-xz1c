# AGENTS.md — ios-xperia-xz1c（指路牌）

**唯一权威工作日志与硬事实手册已统一到主目录：**

→ `D:\Desktop\Git\Patch\Xperia\AGENTS.md`（2026-10-29 起迁至 D 盘）

> 2026-10-29 方向校正（师兄六项情报）见主文件 §>>> 2026-10-29（触摸/HID 顺序、xzp-ios APFS/UEFI、cheeseburger iPadOS17.7.11、WDT 四前提、QR 日志、Pagani）。

接手前必读顺序：
1. `Patch/Xperia/AGENTS.md` —— 顶部「当前状态/红线」+ 最新 `## >>>` 条目
   + 文末「附录A」（原本文件的硬事实/规则手册，已逐字并入）。
2. `Patch/Xperia/SUBAGENTS.md` —— 持久约束与检查点（压缩后必读）。
3. 本仓 `NOTES.md` —— 仓库内构建/交接状态。

本文件原有全部内容（硬件常量、遥测通道、WDT/PSHOLD 规则、Magisk/ADB、
自动化工具箱、盟友情报、红线、GitHub token 说明）已**逐字并入**主文件
附录A，未做任何删改；历史版本见本仓 git 记录。

**摘要红线（主文件为准）**：不动 boot/modem/persist 与备份分区；不碰
ufsice@0x1db0000；不写 PSHOLD；不动 SD 卡；破坏性步骤先问用户；
删除一律进回收站；主机 CPU 上限 70%（taskset -c 0-13 + nice + ionice）。
