# HANDOVER-MOUNTROOT.md — "Waiting for root device" 之后怎么给根

> 写给主线 agent。副线物资已就位，此文件只讲下一阶段的打法。
> 2026-10-25 副线完成汇总见 `HANDOVER-FRONTIER.md` / `RESEARCH-FRONTIER.md`。

## 现状（主线刚打的里程碑）
- 内核到 `mountroot` 循环；屏上 `IOMedia/Apple_HFS` 匹配日志已在
  → **HFS 链路活着**，缺的只是一个能提供 block device 的介质。
- sagit 0118（16K VM over 4K PTE）已移植 → exec 签名二进制的路已通。

## 两条路线（按风险排）

### 路线 A：ramdisk 根（推荐第一步，零写盘）
- **物资已在 Mac**：`~/Desktop/ios-assets/ipsw-unpack/` 里 IPSW 解开时
  带 restore ramdisk DMG（087-*.dmg 系列里那几个小的）。
- 做法：shim 把 ramdisk 装进内存 + boot-arg `rd=md0` → 内核走
  `md0` 设备起 launchd。**完全不碰 UFS**，先验证 userspace 起得来。
- 验证价值：launchd/AMFI/Sandbox/HFS 全套链路在被真实存储变量
  干扰前先跑通；rootfs 写盘的坑和 userspace 的坑分开排。
- 注意：iOS ramdisk 是 dyld/ minimal userspace，适合做"内核↔
  userspace ABI"联调，不等于最终 rootfs。

### 路线 B：UFS userdata 真根（终点）
- **容量已实锤**：userdata = LBA 2056837..7810042 ×4K = **21.95 GiB**，
  Mac 上 `~/Desktop/ios-assets/iosroot-16.7.16.img` 是 16 GiB HFS+
  （fsck 已验，含 launchd/backboardd/cryptex/dyld cache 硬链）。
- **写盘流程**：
  1. Mac：`hdiutil convert -format UDTO -o iosroot.cdr iosroot-16.7.16.img`
     剥 UDRW 壳 → 纯 raw HFS+
  2. Windows 把 raw 拷过来（17GB，走 adb push 或 fastboot 传）
  3. 主线决定写入方式（shim 侧 UFS 写 or Android 侧 dd 到 userdata）
- **UFS 驱动要点**（`RESEARCH-FRONTIER.md` §2 已全）：
  - host: `ufshc@0x1da4000`（0x3000）；QMP v3 PHY @ `0x1da7000`
  - PHY 校准：`phy_cal_table_rate_A[]` ~60 写 → （单 lane，大概率
    不用 second-lane 表）→ `phy_cal_table_rate_B[]` 1 写 → dmb
  - link startup 顺序：SERDES start → PCS ready poll → lane clk
  - **红线：`ufsice@0x1db0000` 别碰**——ICE 配置错会砖数据区
- 分区安全：userdata 是我们要盖的分区（里面现在是 Android
  userdata，可牺牲）；persist/modem/boot 在其他 LBA，不受影响。

## 建议节奏
1. 路线 A 先跑通（快，风险零）→ 证明 launchd/ABI 全链 OK
2. 同时推进 UFS host 驱动（纯 MMIO 轮询可先不碰 PHY，看能不能
   先读到 link/device desc——很多 UFS 控制器上电 PHY 默认态可读）
3. UFS 通了再写 rootfs 到 userdata → 终点 mountroot 成功

## 副线其余可用物资（按需取）
- GPU：vendor.sin 挖出 `a540_zap/gpmu/cpp/ipa`（RESEARCH §9/11）
- WiFi：QCA WCNSS 修正 + `wlanmdsp.mbn`/`bdwlan.*`（RESEARCH §4/12）
- 触摸：RMI4 F11 真布局骨架（research/touch-skeleton/）
- 注意：vfrag 中间 raw 已回收，如需重挖按 FR §9 方法重跑
