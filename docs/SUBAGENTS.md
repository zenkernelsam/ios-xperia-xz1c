# SUBAGENTS.md — 前沿研究支线（每次上下文压缩后必读）

> 本文件是**副线任务**的权威记录（主线 Sony boot 在 `AGENTS.md`）。
> 压缩后第一件事：读本文件 + `RESEARCH-FRONTIER.md` 末尾的"当前进度"。

## 用户硬性约束（不可违反）

1. **不碰 Sony 手机**——主线 agent 在用它调试内核。一切只读操作也要克制。
2. **删除一律走回收站**，禁止 `rm` 真删（Mac: `osascript`/`trash`，Win: PowerShell `Microsoft.VisualBasic.FileIO.FileSystem` 或拖到回收站）。
3. **Mac 工作全部收在 `~/Desktop/ios-assets/`**，不许碎片撒到 /tmp、~/Desktop 根等别处。
4. **能放 Mac 编译的都放 Mac**，不用 GitHub CI。
5. **分析别人的 repo 要分清真伪**：代码是跑通过的？还是 WIP/dummy/failure？不要被带歪（用户原话：避免兜大圈落泪）。
6. **进度定时归档**：持续把中间结果写进 `RESEARCH-FRONTIER.md`，压缩上下文前必须落盘。
7. 最终成果要并回 `ios-xperia-xz1c` 仓库（artifacts/脚本/文档）。
8. **中文交流**。

## 当前支线任务清单（goal）— 2026-10-25 00:30 快照

| # | 任务 | 状态 |
|---|------|------|
| 1 | IPSW+rootfs（Mac） | ✅ `iosroot-16.7.16.img` 17GB UDRW fsck OK |
| 2 | UFS 摸底+PHY 提取 | ✅ 完成 |
| 3 | ADT 常量表 | ✅ mkadt.py 已全 |
| 4 | WiFi/BT 固件提取 | ✅ modem FAT+BT 全套；vendor 残量待 adb pull |
| 5 | Synaptics 触摸 | 🟡 硬件地图齐（§5）；骨架代码未写 |
| 6 | 仓库更新+clone | ✅ sheng/nash/iphoneParty 已补；UniXNU 有本地改动未动 |
| 7 | 文档归档 | ✅ RESEARCH-FRONTIER/HANDOVER-FRONTIER/AGENTS.md 已写 |

**下回副线起手指引**：读 HANDOVER-FRONTIER.md「遗留待办」；
压缩后先读本文件。

## 关键已验证事实（别重复验证）

- Sony `.sin` = `gzip(tar{cms + NNN sparse分片})`；分片是同 fs 的**增量变体**，
  每片只覆盖部分区间，union 也凑不全 → 完整文件靠 `modem.000`(FAT完整) 或 adb pull。
- lilac WiFi = **QCA Atheros**（wlanmdsp.mbn+bdwlan 在 modem 分区）→ iOS 无原生驱动。
- `hdiutil attach` 输出**中文进度行**且顺序乱（synthesized 在前 physical 在后）；
  找挂载点 = 抓第3字段是 `/` 开头的行；容器名在 `Apple_APFS Container diskN` 行。
- `hdiutil attach -nomount` + `mount_apfs`/`mount -t hfs` 手动挂**都失败**
  （"Permission denied"/"No such file"）——**靠谱做法 = plain
  `hdiutil attach -owners on -nobrowse` 让它自动挂 /Volumes/xxx**，
  `-owners on` 保留真实 uid/gid（必须，否则 rsync 拿不到属主）。
- **输出镜像必须用 UDRW 扁平 dmg**（`hdiutil create` 不加 -type 默认=UDRW），
  SPARSE 格式挂出来是 read-only；UDRW 挂出来 journaled+可写。
- macOS sudo：`~/.supw` 存了密码（600权限），`sudo -S -k cmd < ~/.supw`；
  整脚本以 root 跑最稳（内部 sudo 自然变 no-op）。
- mk.sh 在 Mac：`~/Desktop/ios-assets/mk.sh`（源:ios-sweet/tools/mkiosroot.sh，
  已修 CRLF/realpath/hdiutil解析/UDRW/owners）。产物 `iosroot-16.7.16.img`(.dmg)。

## 环境

- Mac: `ssh ciscohe@192.168.68.59`（免密已配），macOS 15.8.1。
- IPSW: `~/Desktop/ios-assets/ipsw/iPhoneX_16.7.16_20H392_Restore.ipsw` (6.17GB 已下完)
- 解包: `~/Desktop/ios-assets/ipsw-unpack/` →
  OS=`087-86698-044.dmg`(4.7G) SystemOS=`087-86593-047.dmg`(2.7G) AppOS=`087-86715-047.dmg`(10M)
- 构建脚本: `~/Desktop/ios-assets/mk.sh`（CRLF已修，参照 ios-sweet/tools/mkiosroot.sh）
- 固件仓库: `Patch/Xperia/research/firmware-extract/fw/{modem,bt}/`
- 工作日志: `Patch/Xperia/RESEARCH-FRONTIER.md`

## 2026-10-25 00:55 检查点（压缩后先读本文件！）
- 已完成：userdata=21.95GiB（rootfs 16GiB 可放）；vendor.sin GPU 固件
  86 项提取于 research/firmware-extract/fw-vendor/（a540_zap 已验证）；
  UniXNU GPU 栈研读（A740 专用，A540 需 zap+CP 自研）；
  文档已提交 ios-xperia-xz1c（commit: frontier research docs）。
- bdwlan SKU：102/104 两变体已保留，待机器空闲时对照 persist 定版。
- 触摸骨架仍是设计稿；QUP FIFO/F11 细化是下一个可并行项。
- 待办：rootfs 写盘流程预案（UDTO 剥壳+dd，待主线授权）；
  隔壁主线在 v106+ 调 commpage 后阶段，别碰手机。
