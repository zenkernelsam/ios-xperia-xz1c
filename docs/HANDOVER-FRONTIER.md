# HANDOVER-FRONTIER.md — 前沿支线交接（2026-10-25 00:30）

> 副线（不碰手机）全部完成时的交接。主线 Sony boot 状态看 `AGENTS.md` 顶部。
> 详细分析在 `RESEARCH-FRONTIER.md`；副线规则在 `SUBAGENTS.md`。

## 交付清单

### 1. ✅ iOS rootfs 实物（最大物资，Mac 侧）

```
~/Desktop/ios-assets/iosroot-16.7.16.img   17GB UDRW dmg
```

- iPhone X **16.7.16 (20H392)** 完整 rootfs：OS + SystemOS/AppOS cryptex
  + dyld_shared_cache 硬链 + systembag.kb + logd .migrated 目录
- case-sensitive journaled HFS+，fsck_hfs 验证 "appears to be OK"
- 验证过：/sbin/launchd、/usr/libexec/backboardd、dyld cache(114MB) 全在
- **写盘前注意**：这是 UDIF dmg 壳——写 userdata 前要
  `hdiutil convert -format UDTO -o raw.img iosroot-16.7.16.img`
  剥壳成 raw，或在 Mac 上 `hdiutil attach` 后 dd /dev/rdiskNs2。
- 构建脚本：`~/Desktop/ios-assets/mk.sh`（副本在
  `research/firmware-extract/mkiosroot-mac.sh`），改三个 dmg 名可复用。

### 2. ✅ 固件仓 `research/firmware-extract/fw/`

| 路径 | 内容 | 用处 |
|---|---|---|
| `modem/wlanmdsp.mbn` | QCA WiFi 固件 ELF 3.3MB | WiFi（iOS 无 QCA 驱动，走社区路线） |
| `modem/image/adsp.*`+`.mdt` | Hexagon ADSP 全套 ~10MB | 音频 DSP（远期） |
| `modem/image/bdwlan.*` | 板级 RF 数据多 SKU | WiFi 校准（要挑 lilac 的 SKU） |
| `bt/image/*` | BT 固件 tlv+nvm 全套 | 蓝牙（同 QCA 路径） |

**未拿到**：vendor `/firmware/` 真身（sin 增量覆盖只有34%）——
含 a540 GPU 微码。手机空闲时 `adb pull /vendor/firmware` 兜底，
或解 system.sin/oem.sin 的完整分片。

### 3. ✅ 侦察成果（全在 RESEARCH-FRONTIER.md）

- **WiFi 大修正**：lilac = QCA Atheros WCNSS 非博通 → iOS 无原生驱动；
  topaz 的 TopazModem（Qualcomm modem 栈）是**唯一**现成路线。
- **UFS**：QMP v3 PHY 校准表已从 sony-4.4 全量提取（60+19+1 条，
  `phy-qcom-ufs-qmp-v3.h:219-306`），断链保底方案齐。
- **ADT**：隔壁 mkadt.py 常量已全（PHYS_BASE=0xA0000000 对齐结案）。
- **触摸**：Synaptics ClearPad @i2c 0x2c on BLSP1 QUP5(0xc179000)，
  GPIO 87/88+IRQ125；MSM8998 QUP 有 FIFO 模式可**轮询**，不用 GPI-DMA。
- **成色鉴定表**（§8）：哪些 repo 真跑通/哪些是 WIP——抄码铁律落盘。
- **UniXNU-nash**（MSM8998！）board.toml 全模板到手：FB 0x9D400000
  与我们相同、目标 iOS 版本相同、UEFI G1 已过实机。

### 4. ✅ 仓库状态

`research/` 新增：`ios-flame`(SD855+Metal)、`UniXNU-sheng`(官方设备仓)、
`UniXNU-nash`(fcxuhaq3/，MSM8998)。`UniXNU` 本地有未推改动**未动**。
`ios-xperia-xz1c/README.md` 已重写为 lilac 实况。

## 主线移交建议（隔壁 agent 读到这看）

0. **⚠️ 急——sagit 刚解了 commpage 类坑**（MSM8998 同源，commit
   `a3583ff` in `research/ios-sweet-sagit`）：commpage text 随机滑页
   在 PAGE_RATIO=4 下必须落在 4 倍数叶子 → "PTE write is unaligned"。
   **更大坑：纯 4K 内核 exec 不了 iPhone X 签名二进制**（pageSize=14
   vs 12）——sagit 走"16K VM over 4K PTE"路线（PAGE_SHIFT_CONST=14）。
   补丁 `xnu/patches/0118-sagit-16k-vm-pages.patch`，已写进 AGENTS.md 顶部。
1. **commpage 坑**：UniXNU 的 4K 补丁文档
   `research/UniXNU/docs/xnu/four-k-page-tables.md` +
   `patch-reference.md` 对照——他们可能已解 SROL/commpage 类问题。
2. **过 commpage 后的下一个大坎 = UFS root 挂载**：sweet_ufs 的
   `ml_io_map` + 轮询 SCSI 路线可行，寄存器以 §2 为准；
   若 link 断了按 §2.4 PHY 序列重校。
3. **rootfs 已备好**（上述 17GB img）——内核能 mount ufsroot0 时直接用。
4. WiFi 别指望苹果栈——QCA 芯片，长期看 TopazModem 移植。

## 遗留待办（下回副线）

- Synaptics XNU 驱动骨架代码（硬件地图已齐，参考 sheng+topaz）
- `oem.sin`/`system.sin` 解包补 vendor 固件（GPU 微码）
- 手机空闲时 adb pull /vendor/firmware 兜底
- UFS 实机探测（等主线到 mount 阶段）
- UniXNU-nash 的 make-bootimg/adt 脚本细读（若主线决定试 UEFI 链）

---

## 追加 2026-10-25 00:50（副线续）

- **userdata = 21.95 GiB**，rootfs 16 GiB 卷可直写余 6.4GB（GPT 实拆，见 FR §10）。
- **GPU 固件到手**：`research/firmware-extract/fw-vendor/` 86 项 82 完整，
  含 `a540_zap.*`（DSP6 ELF 已验证）、`a540_gpmu.fw2`、CPP/IPA 全套。
  提取法：inode表@vendor.000 + 数据@vendor.005 跨分片拼块（FR §9）。
- **oem.sin 已排查**：只是 OEM 定制 app（键盘皮肤等），无固件，不用再挖。
- **UniXNU GPU 栈研读结论**（FR §11）：架构可抄分层，但 A540 需自写
  PM4-a5xx + 无GMU固件路径（zap+CP 直驱）；sheng 现成代码是 A740 专用。
