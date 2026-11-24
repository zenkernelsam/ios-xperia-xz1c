"""Check the Lilac-DCM DTB against stock firmware and report peripheral bindings."""

import argparse
import hashlib
import io
import json
from pathlib import Path
import struct
import subprocess
import tarfile
import zipfile


KERNEL_MEMBER = "kernel.sin"
BOOT_MEMBER = "boot.000"
MAX_KERNEL_ARCHIVE = 32 << 20
MAX_DTB = 1 << 20
STOCK_SIN_SHA256 = "d12b57e8f1e628d400051750c320c0c4671f9e06f4642e16b882376f966efe63"
STOCK_DTB_SHA256 = "06f4c3493aad91d2240508b0c5788d4799c9c2a58357462507b3bc5e28bf6669"
PANEL_PARENT = "/soc/qcom,mdss_mdp@c900000"
PANEL_NODES = ("somc,5_panel", "somc,6_panel", "somc,8_panel", "somc,9_panel", "somc,default_cmd_panel")
WLED_PARENT = "/soc/qcom,spmi@800f000/qcom,pmi8998@3"
WLED_NODE = WLED_PARENT + "/qcom,leds@d800"
OTHER_NODES = {
    "wlan": "/soc/qcom,icnss@18800000",
    "wlan_alternative": "/soc/qcom,msm_ath10k_wlan",
    "bluetooth": "/vendor/bt_wcn3990",
    "audio_tasha": "/soc/sound-9335",
    "audio_tavil": "/soc/sound-tavil",
}


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def stock_binding(firmware, dtb_bytes):
    if len(dtb_bytes) < 40 or len(dtb_bytes) > MAX_DTB:
        raise ValueError("DTB size out of range")
    magic, size = struct.unpack_from(">II", dtb_bytes)
    if magic != 0xd00dfeed or size != len(dtb_bytes) or sha256(dtb_bytes) != STOCK_DTB_SHA256:
        raise ValueError("DTB does not match the pinned SO-02K firmware input")

    with zipfile.ZipFile(firmware) as archive:
        members = [item for item in archive.infolist() if item.filename == KERNEL_MEMBER]
        if len(members) != 1 or not 0 < members[0].file_size <= MAX_KERNEL_ARCHIVE:
            raise ValueError("Expected one bounded kernel.sin member")
        sin = archive.read(members[0])
    if sha256(sin) != STOCK_SIN_SHA256:
        raise ValueError("kernel.sin does not match pinned 47.1.F.1.105")

    with tarfile.open(fileobj=io.BytesIO(sin), mode="r:gz") as archive:
        members = [item for item in archive if item.name == BOOT_MEMBER]
        if len(members) != 1 or not members[0].isfile() or not 0 < members[0].size <= MAX_KERNEL_ARCHIVE:
            raise ValueError("Expected one bounded boot.000 member")
        with archive.extractfile(members[0]) as stream:
            boot = stream.read()

    if len(boot) < 4096 or boot[:8] != b"ANDROID!":
        raise ValueError("Unexpected stock boot header")
    kernel_size = struct.unpack_from("<I", boot, 8)[0]
    page_size = struct.unpack_from("<I", boot, 36)[0]
    kernel_end = page_size + kernel_size
    if page_size != 4096 or not page_size < kernel_end <= len(boot):
        raise ValueError("Stock boot kernel bounds differ")
    offset = boot.find(dtb_bytes, page_size, kernel_end)
    if offset < 0 or boot.find(dtb_bytes, offset + 1, kernel_end) >= 0:
        raise ValueError("Expected exactly one byte-identical DTB in the boot kernel section")
    return {
        "firmware": "SO-02K 47.1.F.1.105",
        "kernel_sin_sha256": sha256(sin),
        "boot_member_sha256": sha256(boot),
        "dtb_sha256": sha256(dtb_bytes),
        "dtb_bytes": len(dtb_bytes),
        "dtb_offset_in_boot_member": offset,
    }


class Fdt:
    def __init__(self, path, executable):
        self.path = path
        self.executable = executable

    def query(self, *args, default=None):
        result = subprocess.run(
            [self.executable, *args], capture_output=True, text=True, timeout=5,
        )
        if result.returncode == 0:
            return result.stdout.strip()
        if default is not None and result.stderr.strip().endswith("FDT_ERR_NOTFOUND"):
            return default
        raise ValueError(result.stderr.strip() or "fdtget failed")

    def prop(self, node, name, kind="s", default=None):
        return self.query("-t", kind, self.path, node, name, default=default)

    def cells(self, node, name, kind="u"):
        return [int(value) for value in self.prop(node, name, kind).split()]

    def node(self, path):
        compatible = self.prop(path, "compatible").split()
        return {
            "path": path,
            "compatible": compatible,
            "status": self.prop(path, "status", default="unspecified"),
        }


def peripheral_binding(tree):
    model = tree.prop("/", "model")
    compatible = tree.prop("/", "compatible").split()
    if model != "SoMC Lilac-DCM(MSM8998 v2.1)" or compatible != ["somc,lilac-dcm", "qcom,msm8998"]:
        raise ValueError("DTB is not the expected Lilac-DCM board")

    wled = tree.node(WLED_NODE)
    resource_names = tree.prop(WLED_NODE, "reg-names").split()
    resources = tree.cells(WLED_NODE, "reg")
    strings = tree.cells(WLED_NODE, "qcom,led-strings-list", "bu")
    if (wled["compatible"] != ["qcom,qpnp-wled"] or wled["status"] != "okay"
            or resource_names != ["qpnp-wled-ctrl-base", "qpnp-wled-sink-base"]
            or len(resources) != 4 or strings != [0, 1]):
        raise ValueError("Unexpected Lilac-DCM WLED provider binding")
    area_table = tree.cells(WLED_NODE, "somc,area_count_table")
    if (tree.cells(WLED_NODE, "somc,area_count_table_size") != [len(area_table)]
            or not area_table or sorted(set(area_table)) != area_table
            or area_table[0] != 0 or area_table[-1] != 4095):
        raise ValueError("Invalid vendor brightness area table")
    wled.update(resource_names=resource_names, led_strings=strings,
                brightness_area_table=area_table,
                full_scale_current_microamps=tree.cells(WLED_NODE, "qcom,fs-curr-ua")[0],
                initial_current_microamps=tree.cells(WLED_NODE, "somc,init-br-ua")[0])

    panels = {}
    for name in PANEL_NODES:
        path = PANEL_PARENT + "/" + name
        width = tree.cells(path, "qcom,mdss-dsi-panel-width")
        height = tree.cells(path, "qcom,mdss-dsi-panel-height")
        minimum = tree.cells(path, "qcom,mdss-dsi-bl-min-level")
        maximum = tree.cells(path, "qcom,mdss-dsi-bl-max-level")
        control = tree.prop(path, "qcom,mdss-dsi-bl-pmic-control-type")
        if (width, height, minimum, maximum, control) != ([720], [1280], [1], [4095], "bl_ctrl_wled"):
            raise ValueError(f"Unexpected WLED contract for {name}")
        panels[name] = {"pixels": [width[0], height[0]], "nonzero_level_range": [minimum[0], maximum[0]],
                        "control": control}

    other = {name: tree.node(path) for name, path in OTHER_NODES.items()}
    for name in ("audio_tasha", "audio_tavil"):
        other[name]["model"] = tree.prop(other[name]["path"], "qcom,model")
    return {
        "model": model,
        "compatible": compatible,
        "wled": wled,
        "panel_candidates": panels,
        "active_panel_verified": False,
        "other_peripheral_nodes": other,
    }


def inspect(firmware, dtb, executable):
    with dtb.open("rb") as stream:
        dtb_bytes = stream.read(MAX_DTB + 1)
    binding = stock_binding(firmware, dtb_bytes)
    peripherals = peripheral_binding(Fdt(dtb, executable))
    if dtb.read_bytes() != dtb_bytes:
        raise ValueError("DTB changed during inspection")
    return {
        "revision": 1,
        "firmware_binding": binding,
        **peripherals,
        "hardware_function_verified": False,
        "device_operations": 0,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--firmware", type=Path, required=True)
    parser.add_argument("--dtb", type=Path, required=True)
    parser.add_argument("--fdtget", default="fdtget")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    report = inspect(args.firmware, args.dtb, args.fdtget)
    with args.output.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(report, stream, indent=2)
        stream.write("\n")
    print(json.dumps({"model": report["model"], "panels": len(report["panel_candidates"]),
                      "wled": report["wled"]["compatible"], "device_operations": 0}))


if __name__ == "__main__":
    main()
