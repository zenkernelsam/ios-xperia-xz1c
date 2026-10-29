# AGENTS.md — ios-xperia-xz1c（指路牌）

**唯一权威工作日志与硬事实手册已统一到主目录：**

→ `D:\Desktop\Git\Patch\Xperia\AGENTS.md`（2026-10-29 起迁至 D 盘）

> 2026-10-29 情报二：显示零拷贝 DPU scanout 与 GPU 解耦（CPU 调度→1fps→DPU→GPU），见主文件 §情报二。
> 2026-10-29 方向校正（师兄六项情报）见主文件 §>>> 2026-10-29（触摸/HID 顺序、xzp-ios APFS/UEFI、cheeseburger iPadOS17.7.11、WDT 四前提、QR 日志、Pagani）。

## 🔴 路径硬约束（2026-10-29，永久有效）

- **绝对禁止读写 Windows `C:\Users\Cisco He\Desktop`**（即将被永久清除）。
  不要再 `ls/cat/find/git` 指向该目录，也不要从那里复制任何文件。
- Windows 唯一工作区：
  - `D:\Desktop\Git\Patch\Xperia`
  - `D:\Desktop\Git\ios-xperia-xz1c`
- 临时文件只放 `C:\Users\Cisco He\AppData\Local\Temp`（不是 Desktop）。
- Mac 工作区仍为 `~/Desktop/ios-xperia-xz1c`（Mac 本机路径，不受此限）。
- 若发现脚本/文档里还有 `C:\Users\Cisco He\Desktop`，一律改成 D: 对应路径。

---

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
