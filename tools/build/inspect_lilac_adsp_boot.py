"""Audit the locked ADSP pilot and Sony power prerequisites without device IO."""

import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
COMMIT = "33dc678e7692f04ecc1eeddeb1cca8a809bc1727"
REFERENCES = (
    "drivers/soc/qcom/subsys-pil-tz.c",
    "drivers/soc/qcom/peripheral-loader.c",
    "drivers/clk/msm/clock-gcc-8998.c",
    "arch/arm/boot/dts/qcom/msm8998.dtsi",
    "arch/arm/boot/dts/qcom/msm8998-smp2p.dtsi",
    "include/dt-bindings/regulator/qcom,rpm-smd-regulator.h",
)
TOKENS = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|'
                    r"'(?:\\.|[^'\\])*'|[{}]", re.S)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def block(text, signature):
    matches = list(re.finditer(signature, text, re.M))
    if len(matches) != 1:
        raise ValueError("Expected one source block: " + signature)
    start = matches[0].start()
    opening = text.index("{", start, matches[0].end())
    depth = 0
    for token in TOKENS.finditer(text, opening):
        if token.group() == "{":
            depth += 1
        elif token.group() == "}":
            depth -= 1
            if depth == 0:
                return dict(first_line=text.count("\n", 0, start) + 1,
                            last_line=text.count("\n", 0, token.end()) + 1,
                            source=text[start:token.end()])
    raise ValueError("Unclosed source block")


def function(text, name):
    return block(text, r"\b" + re.escape(name) + r"\s*\([^();{}]*\)\s*\{")


def require(source, *fragments):
    for fragment in fragments:
        if fragment not in source:
            raise ValueError("Locked source contract differs: " + fragment)


def formal_added_file(path, patch, lock):
    prefix = ("a/" + path + " b/" + path + "\n").encode()
    sections = [s for s in patch.split(b"diff --git ") if s.startswith(prefix)]
    if len(sections) != 1 or b"--- /dev/null\n" not in sections[0]:
        raise ValueError("Expected one formal added file: " + path)
    data = b"".join(line[1:] for line in sections[0].splitlines(keepends=True)
                    if line.startswith(b"+") and not line.startswith(b"+++"))
    expected = lock["audited_xnu_files"][path]
    if sha(data) != expected:
        native = data.replace(b"\n", b"\r\n")
        if b"\r" in data or sha(native) != expected:
            raise ValueError("Formal source SHA differs: " + path)
        data = native
    return data


def inspect(reference, output):
    output = Path(output).resolve()
    reference = Path(reference).resolve()
    if output.exists() or not output.is_relative_to(ROOT / "build"):
        raise ValueError("Use a nonexistent formal build subdirectory")
    lock_data = (ROOT / "reproducibility/source-lock.json").read_bytes()
    lock = json.loads(lock_data)
    patch = (ROOT / "xnu/lilac-current.patch").read_bytes()
    if sha(patch) != lock["xnu"]["patch_sha256"]:
        raise ValueError("Formal XNU patch is not the locked input")

    def git(*args):
        return subprocess.check_output(["git", "-C", str(reference), *args], timeout=20)

    if git("rev-parse", "HEAD").decode().strip() != COMMIT:
        raise ValueError("Use the pinned Sony reference checkout")
    upstream, reference_rows = {}, {}
    for path in REFERENCES + ("COPYING",):
        data = git("show", COMMIT + ":" + path)
        reference_rows[path] = dict(git_blob=git("rev-parse", COMMIT + ":" + path).decode().strip(),
                                    sha256=sha(data))
        if path != "COPYING":
            upstream[path] = data.decode()
    alias = "arch/arm64/boot/dts/qcom"
    dts_alias = dict(path=alias, git_blob=git("rev-parse", COMMIT + ":" + alias).decode().strip(),
                     target=git("show", COMMIT + ":" + alias).decode())
    formal = {}
    formal_rows = {}
    for path in ("bsd/dev/sweet_sample.c", "bsd/dev/sweet_rpm.c"):
        data = formal_added_file(path, patch, lock)
        formal[path] = data.decode().replace("\r\n", "\n")
        formal_rows[path] = sha(data)
    sample, rpm = formal.values()
    tz, loader, clock, dt, smp, regulator = upstream.values()
    pilot = function(sample, "adsp_cold_pilot_mapped")
    bootstrap = function(sample, "adsp_bootstrap_once")
    clocks = function(rpm, "sweet_rpm_vote_gpu_clocks")
    ready = function(sample, "adsp_wait_private_ready")
    lpass = block(dt, r"qcom,lpass@17300000\s*\{")
    proxy = function(tz, "pil_make_proxy_vote")
    auth = function(tz, "pil_auth_and_reset")
    init = function(tz, "pil_init_image_trusted")
    linux_boot = function(loader, "pil_boot")
    require(lpass["source"], 'qcom,pas-id = <1>', 'qcom,proxy-clock-names = "xo"',
            'clk_cxo_pil_lpass_clk', 'RPM_SMD_REGULATOR_LEVEL_TURBO',
            'qcom,proxy-timeout-ms = <10000>', 'qcom,firmware-name = "adsp"',
            'qcom,gpio-err-ready = <&smp2pgpio_ssr_smp2p_2_in 1 0>')
    require(regulator, "#define RPM_SMD_REGULATOR_LEVEL_TURBO\t\t384")
    require(clock, "DEFINE_CLK_RPM_SMD_BRANCH(cxo_clk_src", "static DEFINE_CLK_BRANCH_VOTER(cxo_pil_lpass_clk, &cxo_clk_src.c)",
            "DEFINE_CLK_RPM_SMD(ce1_clk", "static DEFINE_CLK_VOTER(mcd_ce1_clk, &ce1_clk.c, 85710000)")
    require(proxy["source"], "enable_regulators", "prepare_enable_clocks", "d->proxy_clks")
    require(init["source"], "scm_pas_enable_bw()", "PAS_INIT_IMAGE_CMD")
    require(auth["source"], "enable_regulators", "prepare_enable_clocks", "scm_pas_enable_bw()", "PAS_AUTH_AND_RESET_CMD")
    boot_src = linux_boot["source"]
    positions = [boot_src.index(s) for s in ("ret = pil_proxy_vote(desc)", "desc->ops->init_image(desc",
                                           "desc->ops->mem_setup(desc", "ret = pil_load_seg(desc",
                                           "ret = desc->ops->auth_and_reset(desc)")]
    if positions != sorted(positions):
        raise ValueError("Sony PIL prerequisite order differs")
    src = pilot["source"]
    require(src, "if(level || sleep)return 43", "if(clocks)return 44", "if(b0||b1||s0||s1)return 45",
            "sweet_adsp_pil_command(1,", "sweet_adsp_pil_command(2,", "sweet_adsp_pil_command(5,",
            "sweet_adsp_pil_command(6,", "adsp_wait_private_ready(&map,10000", "adsp_shutdown_ok=shut==0&&shut_value==0")
    ordered = [src.index(s) for s in ("int level=sweet_rpm_request", "int clocks=sweet_rpm_vote_gpu_clocks",
                                     "int b0=sweet_rpm_bus_request", "sweet_adsp_pil_command(1,",
                                     "sweet_adsp_pil_command(2,", "memcpy((void *)destination",
                                     "sweet_adsp_pil_command(5,", "adsp_wait_private_ready(&map",
                                     "sweet_adsp_pil_command(6,")]
    if ordered != sorted(ordered):
        raise ValueError("Formal cold pilot order differs")
    require(bootstrap["source"], "stage!=3 && stage!=4 && stage!=5 && stage!=6 && stage!=7",
            "if(result==0 && stage>=4)result=adsp_cold_pilot()")
    require(ready["source"], '"slave-kernel"', "if(value&1u)return 72", "if(value&2u)return 0")
    require(clocks["source"], "0x306b6c63u", "sweet_rpm_rate_khz(85710000u)",
            "sweet_rpm_rate_khz(300000000u)")
    findings = {
        "Linux_lpass_proxy": {"PAS_id": 1, "vdd_cx_level": 384, "proxy_xo_Hz": 19200000,
                              "timeout_ms": 10000, "XO_parent_is_RPM_SMD": True},
        "Linux_PIL_order": ["proxy regulators and clocks", "metadata init with SCM bandwidth",
                           "memory setup", "load segments", "auth/reset with SCM bandwidth"],
        "formal_pilot_order": ["RPM vdd_cx active and sleep", "RPM XO/CE1/aggre2 clock helper",
                              "RPM master/slave bandwidth active and sleep", "PIL init",
                              "PIL memory setup", "copy and cache flush", "PIL auth/reset",
                              "SMP2P ready or fatal", "optional GLINK/APR probe", "PIL shutdown"],
        "formal_pre_PIL_failure_codes": {"proxy": 43, "clocks": 44, "bandwidth": 45},
        "formal_clock_helper": {"XO_enable": 1, "CE1_Hz": 85710000, "aggre2_Hz": 300000000,
                                "all_requests_use_RPM": True},
        "CE1_aggre2_are_not_listed_as_LPASS_node_proxy_clocks": True,
        "helper_reuse_does_not_prove_extra_clocks_are_required_for_ADSP": True,
        "SMP2P_slave_kernel": {"fatal_bit": 0, "ready_bit": 1, "proxy_unvote_bit": 2},
        "supported_diagnostic_stages": [3, 4, 5, 6, 7],
        "stage3_does_not_start_DSP": True,
        "stages4_to7_are_transient_pilots_not_retained_audio_endpoints": True,
        "successful_cold_pilot_always_reaches_PIL_shutdown": True,
        "existing_pilot_cannot_supply_a_live_DSP_to_later_PCM_runtime": True,
        "production_audio_needs_explicit_DSP_power_lifetime_and_cleanup": True,
        "source_dependency_does_not_prove_current_RPM_health_or_DSP_state": True,
    }
    report = dict(
        revision=1, inspector="tools/build/inspect_lilac_adsp_boot.py",
        inspector_sha256=sha(Path(__file__).read_bytes()), source_lock_sha256=sha(lock_data),
        formal_patch_sha256=sha(patch), formal_source_files=formal_rows,
        reference_repository="sony-msm8998/android_kernel_sony_msm8998",
        reference_commit=COMMIT, reference_sources=reference_rows, ARM64_DTS_git_symlink=dts_alias,
        source_blocks={"formal_cold_pilot": pilot, "formal_bootstrap": bootstrap,
                       "formal_clock_helper": clocks, "formal_ready": ready,
                       "Sony_LPASS_node": lpass, "Sony_proxy": proxy,
                       "Sony_init": init, "Sony_auth": auth},
        findings=findings, compiled_or_changed_kernel=False, device_operations=0,
        DSP_started_or_PCM_sound_verified=False,
    )
    output.mkdir(parents=True)
    with (output / "manifest.json").open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(report, stream, indent=2)
        stream.write("\n")
    return report


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reference", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    report = inspect(args.reference, args.output)
    print(json.dumps(report["findings"]))
