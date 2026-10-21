# Working on ios-xperia-xz1c (fork of ios-sweet → Sony Xperia XZ1 Compact "lilac" / SO-02K)

- **HOST CPU CAP 70%** (2026-10-21, user requirement): the Windows PC has an
  ES CPU that hard-rebooted the machine overnight under sustained load.
  NEVER run full-core builds/compiles. Envelope: `taskset -c 0-13` (14/20
  cores in WSL) + `nice -n10` + `ionice -c2 -n7` for anything heavy.
  Same spirit on Windows side. `/tmp` in WSL is wiped on every reboot —
  keep anything valuable in the repo or `/root`. The hfsplus kernel
  module is runtime-loaded: after any reboot re-load it with
  `insmod /root/wslk/fs/hfsplus/hfsplus.ko` (built from WSL2-Linux-Kernel
  linux-msft-wsl-6.18.y, exact 6.18.54.1 match; rebuild via
  `KBUILD_MODPOST_WARN=1 make M=fs/hfsplus modules` if ever needed).
- Read `NOTES.md` before starting; it contains the project goal, hardware facts,
  current blocker, local artifact locations and the next concrete action.
- The user switches agents frequently. Update the active handoff in `NOTES.md`
  after each milestone and before stopping. Record commits, CI run IDs, exact
  build/boot results and unfinished work. Distinguish observations from guesses.
- Commit completed changes. Use `gh` for GitHub Actions kernel builds and logs.
  Local single-file cross-compiles are useful before spending a CI run.
- Device bring-up uses temporary `fastboot boot` images. Preserve the Android
  installation and partition backups. The iOS root would go into the existing
  `userdata` partition (no GPT changes). Keep bootloaders, recovery, modem,
  persist and the backups intact, and confirm each destructive step with the
  user before running it.
- Keep device dumps, serial numbers and downloaded Apple artifacts out of git.

# lilac port — HARD FACTS (verify against this before changing constants)

## Device / workspace

- Phone: Sony Xperia XZ1 Compact, Docomo SO-02K, serial BH905ND69G.
  Snapdragon 835 (MSM8998), 4 GB RAM, 720x1280 DSI command-mode panel,
  UFS (SKhynix H28U62301AMR). Android 8.0.0 / 47.1.F.1.105, XFL = O_77,
  bootloader P_114, **unlocked via xperable (CVE-2021-1931)**.
- Working repo (this clone): `C:\Users\Cisco He\Desktop\Git\ios-xperia-xz1c`
  WSL path: `/root/xz1c` (symlink → /mnt/c/...). Build host: WSL2 Ubuntu-22.04
  (`wsl -d Ubuntu-22.04`), cross toolchain `aarch64-linux-gnu-*` installed,
  mkbootimg at `/opt/mkbootimg`.
- Windows side (adb/fastboot): `C:\Users\Cisco He\Desktop\Git\SchoolBox\Tools\platform-tools_ADB`
  Use `adb -s BH905ND69G` (a stale `emulator-5556 offline` entry exists — ignore).
  `MSYS_NO_PATHCONV=1` required in Git Bash for adb paths.
- Original research dir `C:\Users\Cisco He\Desktop\Git\Patch\Xperia` still holds
  firmware, TA/XFL backups, unlock tools and `research/lilac-stock.dtb`.
  Do not delete `unlock/xfl-backup-original.img` or `flash/fw/tadump/`.

## Hardware constants (from stock DTB / live probes)

- Framebuffer: `0x9d400000`, 720x1280x32bpp (ABL splash region, 36 MB).
- UFSHCI regs: `0x01da4000` (NOT sweet's 0x1d84000). Storage LUN 0 = `sda`,
  4 KiB sectors. `appslog` = sda58, 512B-sector start 1666080 → 4K block
  **208260**, size 16 MiB. `userdata` = sda66.
- IMEM restart_reason @ `0x146bf65c`; PSHOLD @ `0x10ac000`.
  RESTART_BOOTLOADER = `0x77665500`; value `0x77665501` → normal Android boot
  (observed, appears in next cmdline as `warmboot=0x77665501`).
  Custom values like `0x1AC0xxxx` → device landed in fastboot (ABL routing
  unknown) — do NOT use arbitrary codes; stick to observed values.
- **ABL mirrors restart_reason into kernel cmdline as `warmboot=0x........`**
  → 32-bit shim→Android telemetry channel, verified.
- GICv3 `0x17a00000` (same base as sweet), timer 19.2 MHz, PSCI via SMC.
- APSS watchdog `qcom,wdt@0x17817000` (msm-watchdog), bark 12000 ms,
  pet 10000 ms. See "Dead-man switch" below.
- Panel: DSI command mode (somc,5_panel), NOT video mode. Touch: Synaptics
  ClearPad in-cell @ i2c `0xc179000` addr 0x2c, own watchdog in DTB.
- RPM (not RPMh) power voting; keys on PM8998 PON (0x74 power / 0x73 vol).

## Telemetry / log channels (THIS IS HOW WE SEE WITHOUT A SCREEN)

- **pstore console record (primary, 256 KiB channel)**: DTB `ramoops` node,
  console-size 0x40000, record-size 0x1000, ecc 0, region `0xffc00000` 1 MiB.
  Zone layout = 192 record prz (0xffc00000..0xffcc0000) then console prz at
  **`0xffcc0000`**. Write `{u32 sig=0x43474244 "DBGC", u32 start, u32 size,
  char data[]}`; after warm boot Android shows it at
  `/sys/fs/pstore/console-ramoops-0`. STATUS: implemented in shim v5,
  **not yet verified on device** (first run aborted by USB drop).
- **warmboot= cmdline**: restart_reason is echoed into next Android cmdline.
  Use for 1-word status if pstore fails. Only observed-safe value 0x77665501.
- **appslog UFS write**: shim has WRITE(10) path targeting LUN0 LBA 208260.
  v3 result: write FAILED (binary-signal test landed in fastboot), stage not
  yet isolated (probe may bail early — host not ready / list not taken).
  ufs.c read probe results will be visible once pstore channel works.
- bindershell (temp root, CVE-2019-2215) lives in
  `Patch\Xperia\unlock\bindershell`; re-push to /data/local/tmp after any
  userdata wipe. Gives uid0 → dd partitions, dmesg, config.gz.

## Dead-man switch / watchdog design (user sleeping = zero hands)

- GOAL: any hang must self-recover to a reachable mode. NEVER power off.
- Shim must ARM the APSS WDT itself (do not assume ABL left it running).
  **Register map VERIFIED** against CAF `drivers/soc/qcom/watchdog_v2.c`
  (compatible `qcom,msm-watchdog`, WDT_HZ = 32765 Hz sleep clock):
  `WDT0_RST +0x04` (write 1 = pet), `WDT0_EN +0x08` (1 = arm, 0 = disarm),
  `WDT0_STS +0x0C`, `WDT0_BARK_TIME +0x10`, `WDT0_BITE_TIME +0x14` —
  all in ticks of WDT_HZ. Implemented in `shim/wdt.c`; shim_main arms
  bark 120 s / bite 125 s right after writing restart_reason 0x77665501,
  so ANY hang self-recovers to Android within ~2 min. CAUTION: on some
  Qualcomm parts only a TZ scm call can truly disable a running secure
  watchdog — if EN writes don't stick, expect bites anyway (pet early).
- Before arming, write the desired recovery restart_reason to IMEM so the
  watchdog reset lands where we want (0x77665501 → Android w/ adb).
- Every shim test ends by explicitly writing restart_reason + pshold reset.
  Choose landing: 0x77665501→Android, 0x77665500→fastboot.
- **Power-off incident (v5 first attempt)**: `fastboot boot` USB drop mid-boot
  left the phone OFF. LESSON: never leave a partial boot; re-run cleanly;
  only reachable states allowed. A powered-off phone needs a physical
  power-button press — treat poweroff as the worst outcome, design against it.
- If adb is lost but Android runs (enumerates e.g. E1F4 storage-only):
  Windows-side `Disable-PnpDevice`/`Enable-PnpDevice` on the VID_0FCE
  instance re-enumerates USB without touching the phone. adb kill-server
  + replug also recovers most cases.

## ALLY INTELLIGENCE (2026-10-21 repo pulls — TOP PRIORITY, do not lose)

Research clones live in `Patch\Xperia\research\`: `ios-topaz`, `ios-sweet`,
`ios-v60`, `UniXNU` — refresh with `git fetch` before planning.

### ios-topaz — THE reference port (SM6225/SD685, ARMv8.0, iOS 16.7.16 iPhone X)

Status 2026-10-20: **boots to iOS home screen, touch+keys+Wi-Fi work**
(patches now 0139). It documents every silent-boot wall we will hit —
their README "How topaz differs from sweet" is our porting checklist.
SD835 lilac is ALSO ARMv8.0 (Kryo 280: no LSE atomics, no ldapr, no PAN) —
sweet is ARMv8.2 and never hit these walls, so our 84-patch series does
NOT cover them. Porting order for lilac:

| Patch | What | Priority |
| --- | --- | --- |
| `0081-topaz-armv8.0-no-lse-no-pan` | `-mcpu=cortex-a53` build flags on SWEET machine config, commpage `casa`→LDAXR/STXR loop, LSE/ldapr userland emulation in `handle_uncategorized` (LDAXR/STLXR on user addr, copyio recovery) | **MANDATORY** — iPhone X userland contains LSE/ldapr; boot dies silently without it. Written against SWEET config so it should apply near-cleanly |
| | **IMPORTED 2026-10-21** as `0085/0086/0087/0088/0089-lilac-*`: topaz 0080+0081+0082+0092+0095 all apply CLEAN (p1) atop our 84-patch tree (verified by real apply). Constants review pending: `0088` still carries `SWEET_SCANOUT_BASE 0xb0000000` (topaz pool) — pick a lilac-safe carve-out before enabling the scanout driver; `0087` assumes Gold MPIDR `0x100-0x103` which matches lilac DTB. | |
| `0082-topaz-boot-on-gold-cpu` | Shim wakes Gold CPU `0x100` via PSCI CPU_ON, Silver cores park in `wfi`; ADT lists only Gold cores. Needed because **Silver cores lack 16K translation granule** (ID_AA64MMFR0.TGran16=0). lilac Silver likely identical — verify with shim's MMFR0 readout | **MANDATORY** for SMP; `cpus=1` covers first boot |
| `0088-topaz-gpt-watchdog` | GPT watchdog handling | medium — we have APSS wdt |
| `0092-0103` | **Direct MDP scanout**: XNU publishes IOSurface-backed surfaces in contiguous RAM <4 GiB, MDP scans them directly, real dcache maintenance, double buffering, copy+flip path | **core display work** — this IS the friend's "hardware scanout, no CPU copy" advice implemented |
| `0094-topaz-cpufreq-max` | pin CPU freq for bring-up stability | medium |
| `0104-0111` | DSI brightness/DCS on-off + `AppleARMBacklight` | medium — cmd-mode panel relevant |
| `0080/0083-0091` | topology skip, FT5452 touch, battery gauge | adapt: our touch is Synaptics not FocalTech |
| `0113-0130` | **TopazModem Wi-Fi stack**: modem PAS boot through TZ from `modem_a`, GLINK/QRTR, pd-mapper/tftp/rmtfs, WLFW, copy engines, HTC/WMI/HTT, scan, WPA2-PSK 4-way IN KERNEL, Ethernet path | later — same Qualcomm modem family, huge head start |
| `0131-0139` | SweetSurface/IOSurface maturity: client address surfaces, detach/allocsize, zombie state, empty IOAcceleratorES, publish IOSurfaceAddress | GUI stage — CoreAnimation GL renderer needs these |

Topaz shim extras ours lacks: `gold_gicr_prepare()` — preps GIC
redistributors for all Gold cores BEFORE XNU starts SMP itself (XNU
does CPU_ON without shim). Our shim has `test_gic()` + PSCI CPU_ON
already; add the redistributor prep when doing SMP.

### UniXNU — separate architecture, mine for DESIGN not code (SM8550 device "sheng")

- **Native 4 KiB kernel**: `BCM2837` machine config (ARM_PGSHIFT=12) +
  patch clusters `native-4k-bootstrap-capacity` (page-table budget),
  `native-4k-user-abi` (PAGE_SHIFT_CONST=hw granule at any RAM size),
  `native-4k-signature-hash-cache` (16K CodeDirectory → 4K kernel
  validation bridge — needed if we ever run 4K kernel with stock
  16K-signed binaries), `pgz-aligned-validation`,
  `zzx-pmap-sync-64bit-pa`, `ArmSmmu4K` kit. Route for using all 8
  cores someday; docs at `research/UniXNU/docs/xnu/four-k-page-tables.md`.
- **Native Adreno GPU WORKING** (A740 only): chain = Metal AIR →
  metal2vulkan → SPIR-V → Turnip → NIR → IR3 → NativeAdreno KMD →
  GMU/CP/SMMU → scanout. Renderserver composites system UI on-device.
  A540 port = v3.0 topic; the Metal→Turnip architecture is universal.
- `NativeUfs` (SMMU + cmd queues), `NativeAES`, `NativeMetal`,
  `ArmDTPlatform` driver designs — reference if sweet's hacked-Apple-driver
  approach stalls.

### ios-v60 — LG V60 (SD865): GUI + touch confirmed on device,
QCA6390 Wi-Fi in progress. Secondary reference.

### NEW ALLIES pulled 2026-10-21 (same-SoC / sibling ports)

- **`ios-sweet-sagit`** (Patch\Xperia\research\ios-sweet-sagit) —
  Xiaomi Mi6 = **MSM8998, our SoC**. Fresh fork of ios-sweet, commits
  so far: device constants in mkadt/shim + xnu-8796 machine config +
  early FB at 0x9d400000. Their constants vs lilac: GICD 0x17a00000 and
  GICR 0x17b00000 MATCH; IMEM 0x146aa65c ≠ ours 0x146bf65c; PSHOLD
  0x0c264000 ≠ ours 0x10ac000; memory = TWO banks (0xb0000000+0x4d000000
  and 0x100000000+0xfcc00000) vs our one contiguous 0x9f800000-0xffb00000.
  Track their commits — every MSM8998 finding transfers.
- **`xnu_kernel_sdm835`** (MC-XiaoXiao) — full **xnu-11417** tree
  (newest-gen XNU, ~iOS 26 era) imported for SDM835. Alternative
  lineage for the far future (post-16 port), watch it.
- **`ios-le-pro3`** (LEX727, SD821) — early probe stage but rigorous
  methodology (observed/source/planned separation). Notable warning:
  their UFS has an **inline-crypto engine (UFSICE) at 0x630000 the DTB
  didn't mention** — check whether lilac UFS likewise has ICE before
  poking; probe must not touch it. **VERIFIED for lilac: we DO have
  `ufsice@0x1db0000` (qcom,ice, crypto=0x83)** — separate node from the
  ufshc@0x1da4000 host, shim probe doesn't touch it, but the XNU UFS
  driver must coexist (never poke the ICE block). Also confirmed "no
  atomics" on their Kryo (ARMv8.0 like us → topaz 0081 applies).
- **`HeliumStudio-Dev/ios-raphael`** — K20 Pro (SM8150/SD855) sibling
  fork, reached iOS lock screen + GT9886 touch verified. Useful for
  touch/HID driver examples.
- `oxhunnx/ios-a5xelte` (user-spotted) — repo 404 / not public (a5xelte
  = Samsung A5 2016, Exynos — different arch anyway).
- `hackios.net` status: HackiOS team (Poco X5 Pro / 11 Lite demos)
  closed-source until release; topaz README says its GPU work will be
  published once HackiOS publishes source — keep watching.

## Display path — friend's field-tested advice (FOLLOW IT)

- Do NOT CPU-copy/scale an IOSurface into the framebuffer — proven very laggy.
- Qualcomm display controller supports address mapping + hardware scaling:
  drive the MDP/MDSS pipe directly instead of software scaling.
- For early debug use the lowest resolution mode (≈640x360-class).
- **Lock framerate ~1 fps until the CPU scheduler is stable** — unthrottled
  compositing starves the scheduler and wedges the device.
- Bring-up order friend recommends: **CPU scheduling first**, then display.

## Boot chain plan

- `fastboot boot` VERIFIED working on this ABL (stock img + shims booted).
- iOS 16.7.16 (20H392), iPhone X (D22AP) identity first; iPhone 8 (D20AP,
  750x1334 ≈ our 720x1280) as second-stage identity experiment.
- XNU = xnu-8796 built by GitHub Actions (blacktop/darwin-xnu-build);
  trigger with `gh workflow run`, artifacts via `gh run download`.
- IPSW 16.7.16 download was started to `Patch\Xperia\research\` — check state.
- arm64e/PAC impossible on SD835 → iOS 17+ is a v2.0 research topic only.

## Automation toolbox (ALL PROVEN ON DEVICE 2026-10-21)

- **usbctl daemon** = SYSTEM scheduled task "XperiaUsbCtl" (installed via
  `unattend\install-usbctl-task.bat`, right-click→admin ONCE). Agent sends
  commands by dropping `unattend/cmd/*.cmd` files (`replug`/`adbreset`/
  `status`/`stop`), reads `out\result.txt` (UTF-16). `replug` VERIFIED:
  dis/en cycle on VID_0FCE revives USB without touching the phone.
  Wake anytime: `schtasks /run /tn XperiaUsbCtl`.
- `ps:<powershell>` = arbitrary admin command channel (v3 script),
  `devreset` = pnputil remove-device + scan-devices (clears the
  "needs reboot" pending flag that made dis/en fail with 常规故障).
- **`unattend\watchdog.sh`** (bash, `nohup bash watchdog.sh &`) — autonomous
  babysitter: polls adb every ~30 s, escalates kill-server → adbreset →
  devreset → hardreplug → replug, logs to `out/watchdog.log`. Start it before
  any unattended test batch.
- **adb flakiness ladder**: `adb kill-server; start-server` → `adbreset` cmd →
  `replug` cmd → only then consider it truly dead.
- **Windows "needs reboot" pending-config flag**: once set on the phone's
  device node, `Disable-PnpDevice` AND `pnputil /remove-device` both fail
  (常规故障 / 需要重新启动系统). Parent is ROOT_HUB30 (uncycleable). Only a
  Windows reboot clears it — avoid triggering it; device IO still works.
- **`/sys/fs/pstore` LIVE**: `pstore: Registered ramoops` confirmed in dmesg,
  console zone at 0xffcc0000 (DTB ramoops node: console-size 0x40000,
  record-size 0x1000, ecc 0). Shim v6 writes prz {DBGC,start,size,data}
  incrementally per con_str — crash-safe shim→Android log channel.
  FIRST RUN PENDING verification (see NOTES.md).
- **sysrq panic caution**: `echo c > /proc/sysrq-trigger` (bindershell)
  FROZE the device ~6+ min (panic_timeout likely 0 → hang, not reboot).
  Recovery = physical power long-press. Check `cat /proc/sys/kernel/panic`
  before ever doing this again.
- **IMEM restart_reason sticky**: values survive warm resets; `adb reboot
  bootloader` leaves 0x77665500 → subsequent unexplained fastboot landings.
  ABL passes it to next cmdline as `warmboot=0x........` (read channel).
- **stale emulator-5556 offline** in adb devices — ignore, always use
  `-s BH905ND69G`.

## USB replug daemon (usbctl) — THE no-hands tool

- `Patch\Xperia\unattend\usbctl.ps1` — elevated PowerShell daemon; polls
  `unattend\cmd\*.cmd` every 2 s, writes `unattend\out\result.txt` +
  `daemon.log`. Commands: `replug` (Disable/Enable-PnpDevice on every
  VID_0FCE device = software cable replug), `adbreset` (kill/start adb
  server), `status` (PnP + adb + fastboot dump), `stop`.
- Start: RIGHT-CLICK `unattend\start-usbctl.bat` → 以管理员身份运行, keep the
  console window open (minimizing is fine). Self-elevating `Start-Process
  -Verb RunAs -WindowStyle Hidden` was unreliable — no visible UAC.
- Send a command from anywhere: `echo replug > unattend/cmd/r1.cmd`,
  read result 3-5 s later from `out/result.txt`.
- Recovery ladder for a lost phone: `adbreset` → `replug` → (worst case)
  ask user. USB transition drops are EXPECTED; the daemon absorbs them.

## Red lines (learned the hard way)

- NEVER power off / leave unreachable states; every test must self-return.
- NEVER force `sys.usb.config=adb` in Android init — it killed USB gadget +
  charging once; only `persist.sys.usb.config` via normal init if ever needed.
- NEVER touch the physical SD card (user's backups live there). Internal
  userdata is fair game ONLY after explicit user confirmation per-step.
- adb offline storms after rapid reboots are normal; poll, don't panic.
